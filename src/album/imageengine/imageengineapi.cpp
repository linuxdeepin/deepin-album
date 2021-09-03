/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "imageengineapi.h"
#include "DBandImgOperate.h"
#include "controller/signalmanager.h"
#include "application.h"
#include "imageengineapi.h"
#include <QMetaType>
#include <QDirIterator>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include "utils/unionimage.h"
#include "utils/baseutils.h"
#include "albumgloabl.h"
#include "player_engine.h"
#include "imagedataservice.h"

extern QStringList VideoSupportTypeList;
ImageEngineApi *ImageEngineApi::s_ImageEngine = nullptr;

ImageEngineApi *ImageEngineApi::instance(QObject *parent)
{
    Q_UNUSED(parent);
    if (!s_ImageEngine) {
        s_ImageEngine = new ImageEngineApi();
    }
    return s_ImageEngine;
}

ImageEngineApi::~ImageEngineApi()
{
#ifdef NOGLOBAL
    m_qtpool.clear();
    m_qtpool.waitForDone();
    cacheThreadPool.clear();
    cacheThreadPool.waitForDone();
#else
    QThreadPool::globalInstance()->clear();     //清除队列
    QThreadPool::globalInstance()->waitForDone();
#endif

    //销毁
    delete m_imageCacheSaveobj;
}

ImageEngineApi::ImageEngineApi(QObject *parent)
{
    Q_UNUSED(parent);
    //文件加载线程池上限

    qRegisterMetaType<QStringList>("QStringList &");
    qRegisterMetaType<ImageDataSt>("ImageDataSt &");
    qRegisterMetaType<ImageDataSt>("ImageDataSt");
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList");
    qRegisterMetaType<QMap<QString, ImageDataSt>>("QMap<QString,ImageDataSt>");
    qRegisterMetaType<QVector<ImageDataSt>>("QVector<ImageDataSt>");
#ifdef NOGLOBAL
    m_qtpool.setMaxThreadCount(4);
    cacheThreadPool.setMaxThreadCount(4);
#else
    QThreadPool::globalInstance()->setMaxThreadCount(12);
    QThreadPool::globalInstance()->setExpiryTimeout(10);
#endif
}

bool ImageEngineApi::insertObject(void *obj)
{
    m_AllObject.insert(obj, obj);
    return true;
}
bool ImageEngineApi::removeObject(void *obj)
{
    QMap<void *, void *>::iterator it;
    it = m_AllObject.find(obj);
    if (it != m_AllObject.end()) {
        m_AllObject.erase(it);
        return true;
    }
    return false;
}

bool ImageEngineApi::ifObjectExist(void *obj)
{
    return m_AllObject.contains(obj);
}

bool ImageEngineApi::removeImage(QString imagepath)
{
    static QStringList dbremovelist;
    dbremovelist.append(imagepath);
    if (QThreadPool::globalInstance()->activeThreadCount() < 1) {
        DBManager::instance()->removeImgInfos(dbremovelist);
        dbremovelist.clear();
        emit dApp->signalM->updatePicView(0);
    }

    if (m_AllImageData.contains(imagepath)) {
        return m_AllImageData.remove(imagepath);
    }
    return false;
}

bool ImageEngineApi::removeImage(QStringList imagepathList)
{
    for (const auto &imagepath : imagepathList) {
        m_AllImageData.remove(imagepath);
    }
    return true;
}

bool ImageEngineApi::insertImage(const QString &imagepath, const QString &remainDay, bool reLoadIsvideo)
{
    bool bexsit = m_AllImageData.contains(imagepath);
    if (bexsit && remainDay.isEmpty()) {
        return false;
    }

    ImageDataSt data;
    if (bexsit) {
        data = m_AllImageData[imagepath];
    }

    if (!remainDay.isEmpty())
        data.remainDays = remainDay;
    if (reLoadIsvideo) {
        bool isVideo = utils::base::isVideo(imagepath);
        if (isVideo) {
            data.dbi.itemType = ItemTypeVideo;
        } else {
            data.dbi.itemType = ItemTypePic;
        }
    }
    addImageData(imagepath, data);
    return true;
}

void ImageEngineApi::sltInsert(const QStringList &imagepaths, const QString &remainDay)
{
    foreach (QString path, imagepaths) {
        insertImage(path, remainDay);
    }
}

bool ImageEngineApi::updateImageDataPixmap(QString imagepath, QPixmap &pix)
{
    ImageDataSt data;
    if (getImageData(imagepath, data)) {
        data.imgpixmap = pix;
        addImageData(imagepath, data);

        QFileInfo file(albumGlobal::CACHE_PATH + imagepath);
        if (file.exists()) {
            QFile::remove(albumGlobal::CACHE_PATH + imagepath);
        }
        return true;
    }
    return false;
}

bool ImageEngineApi::getImageData(QString imagepath, ImageDataSt &data)
{
    QMap<QString, ImageDataSt>::iterator it;
    it = m_AllImageData.find(imagepath);
    if (it == m_AllImageData.end()) {
        return false;
    }
    data = it.value();
    return true;
}

void ImageEngineApi::sltImageLocalLoaded(void *imgobject, QStringList &filelist)
{
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        static_cast<ImageEngineObject *>(imgobject)->imageLocalLoaded(filelist);
    }
}

void ImageEngineApi::sltImageDBLoaded(void *imgobject, QStringList &filelist)
{
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        static_cast<ImageEngineObject *>(imgobject)->imageFromDBLoaded(filelist);
    }
}

void ImageEngineApi::sltImageFilesGeted(void *imgobject, QStringList &filelist, QString path)
{
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        static_cast<ImageMountGetPathsObject *>(imgobject)->imageGeted(filelist, path);
    }
}

void ImageEngineApi::sltImageFilesImported(void *imgobject, QStringList &filelist)
{
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        static_cast<ImageMountImportPathsObject *>(imgobject)->imageMountImported(filelist);
    }
}

void ImageEngineApi::sltstopCacheSave()
{
#ifdef NOGLOBAL
    cacheThreadPool.waitForDone();
#else
    qDebug() << "析构缓存对象线程";
    QThreadPool::globalInstance()->clear();
    QThreadPool::globalInstance()->waitForDone();

#endif
}

void ImageEngineApi::sigImageBackLoaded(QString path, const ImageDataSt &data)
{
    addImageData(path, data);
}

void ImageEngineApi::slt80ImgInfosReady(QVector<ImageDataSt> ImageDatas)
{
    m_AllImageDataVector = ImageDatas;
    for (int i = 0; i < ImageDatas.size(); i++) {
        ImageDataSt data = ImageDatas.at(i);
        addImageData(data.dbi.filePath, data);
    }
    emit sigLoadFirstPageThumbnailsToView();
}

bool ImageEngineApi::loadImagesFromLocal(DBImgInfoList files, ImageEngineObject *obj, bool needcheck)
{
    ImageLoadFromLocalThread *imagethread = new ImageLoadFromLocalThread;
    connect(imagethread, &ImageLoadFromLocalThread::sigImageLoaded, this, &ImageEngineApi::sltImageLocalLoaded);
    connect(imagethread, &ImageLoadFromLocalThread::sigInsert, this, &ImageEngineApi::sltInsert);
    imagethread->setData(files, obj, needcheck);
    obj->addThread(imagethread);
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
}
bool ImageEngineApi::ImportImagesFromUrlList(QList<QUrl> files, QString albumname, ImageEngineImportObject *obj, bool bdialogselect)
{
    emit dApp->signalM->popupWaitDialog(QObject::tr("Importing..."));
    ImportImagesThread *imagethread = new ImportImagesThread;
    imagethread->setData(files, albumname, obj, bdialogselect);
    obj->addThread(imagethread);
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
}

bool ImageEngineApi::ImportImagesFromFileList(QStringList files, QString albumname, ImageEngineImportObject *obj, bool bdialogselect)
{
    emit dApp->signalM->popupWaitDialog(QObject::tr("Importing..."));
    ImportImagesThread *imagethread = new ImportImagesThread;
    imagethread->setData(files, albumname, obj, bdialogselect);
    obj->addThread(imagethread);
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
}

bool ImageEngineApi::loadImagesFromLocal(QStringList files, ImageEngineObject *obj, bool needcheck)
{
    ImageLoadFromLocalThread *imagethread = new ImageLoadFromLocalThread;
    connect(imagethread, &ImageLoadFromLocalThread::sigImageLoaded, this, &ImageEngineApi::sltImageLocalLoaded);
    connect(imagethread, &ImageLoadFromLocalThread::sigInsert, this, &ImageEngineApi::sltInsert);
    imagethread->setData(files, obj, needcheck);
    obj->addThread(imagethread);
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
}
//bool ImageEngineApi::loadImagesFromPath(ImageEngineObject *obj, QString path)
//{
//    sltImageDBLoaded(obj, QStringList() << path);
//    insertImage(path, "30");
//    return true;
//}

bool ImageEngineApi::loadImageDateToMemory(QStringList pathlist, QString devName)
{
    bool iRet = false;
    //判断是否已经在线程中加载LMH0426
    QStringList tmpPathlist = pathlist;
    if (m_AllImageData.count() > 0) {
        for (auto imagepath : pathlist) {
            if (m_AllImageData.contains(imagepath)) {
                tmpPathlist.removeOne(imagepath);
            }
        }
    }
    if (tmpPathlist.count() > 0) {
        ImageEngineBackThread *imagethread = new ImageEngineBackThread;
        imagethread->setData(tmpPathlist, devName);
        connect(imagethread, &ImageEngineBackThread::sigImageBackLoaded, this, &ImageEngineApi::sigImageBackLoaded, Qt::QueuedConnection);
#ifdef NOGLOBAL
        m_qtpool.start(imagethread);
#else
        QThreadPool::globalInstance()->start(imagethread);
#endif
        iRet = true;
    } else {
        iRet = false;
    }
    return iRet;
}

void ImageEngineApi::loadFirstPageThumbnails(int num)
{
    qDebug() << __FUNCTION__ << "---";

    m_FirstPageScreen = num;
    m_AllImageDataVector.clear();
    thumbnailLoadThread(num);

    QSqlDatabase db = DBManager::instance()->getDatabase();
    if (!db.isValid()) {
        return;
    }
    QStringList list;
    int count = 0;
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType FROM ImageTable3 order by Time desc limit %1").arg(QString::number(num)));
//    bool b = query.prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType FROM ImageTable3 order by Time desc"));
    if (!b || !query.exec()) {
        qDebug() << "------" << __FUNCTION__ <<  query.lastError();
        return;
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            if (count < num) {
                list << info.filePath;
                count++;
            }
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(query.value(5).toString(), DATETIME_FORMAT_DATABASE);
            info.itemType = static_cast<ItemType>(query.value(6).toInt());
            ImageDataSt imgData;
            imgData.dbi = info;
            addImageData(info.filePath, imgData);
            m_AllImageDataVector.append(imgData);
        }
    }

    db.close();
    qDebug() << "------" << __FUNCTION__ << "" << m_AllImageDataVector.size();
    m_firstPageIsLoaded = true;
    ImageDataService::instance()->readThumbnailByPaths(list);

    emit sigLoadFirstPageThumbnailsToView();
}

void ImageEngineApi::thumbnailLoadThread(int num)
{
    Q_UNUSED(num)
    QThread *workerThread = new QThread(this);
    m_worker = new DBandImgOperate(workerThread);

    m_worker->moveToThread(workerThread);
    //开始录制
    connect(this, &ImageEngineApi::sigLoadThumbnailsByNum, m_worker, &DBandImgOperate::sltLoadThumbnailByNum);
//    connect(this, &ImageEngineApi::sigLoadThumbnailIMG, m_worker, &DBandImgOperate::loadOneImg);
    //加载设备中文件列表
    connect(this, &ImageEngineApi::sigLoadMountFileList, m_worker, &DBandImgOperate::sltLoadMountFileList);
    //旋转一张图片
    connect(this, &ImageEngineApi::sigRotateImageFile, m_worker, &DBandImgOperate::rotateImageFile);

    //收到获取全部照片信息成功信号
    connect(m_worker, &DBandImgOperate::sig80ImgInfosReady, this, &ImageEngineApi::slt80ImgInfosReady);
    connect(m_worker, &DBandImgOperate::sigOneImgReady, this, &ImageEngineApi::sigOneImgReady);
    //加载设备中文件列表完成，发送到主线程
    connect(m_worker, &DBandImgOperate::sigMountFileListLoadReady, this, &ImageEngineApi::sigMountFileListLoadReady);
    workerThread->start();
}

void ImageEngineApi::setThreadShouldStop()
{
    if (nullptr != m_worker) {
        m_worker->setThreadShouldStop();
    }
}
//根据路径制作缩略图，并保存到指定位置
bool ImageEngineApi::makeThumbnailByPaths(QStringList files)
{
    if (!m_imageCacheSaveobj) {
        m_imageCacheSaveobj = new ImageCacheSaveObject;
        connect(dApp->signalM, &SignalManager::cacheThreadStop, this, &ImageEngineApi::sltstopCacheSave);
    }
    m_imageCacheSaveobj->add(files);
    int needCoreCounts = static_cast<int>(std::thread::hardware_concurrency());
    needCoreCounts = needCoreCounts / 2;
    if (needCoreCounts < 1)
        needCoreCounts = 1;
    QList<QThread *> threads;
    for (int i = 0; i < needCoreCounts; i++) {
        makeThumbnailThread *thread = new makeThumbnailThread;
        thread->setObject(m_imageCacheSaveobj);
        thread->start();
        threads.append(thread);
    }
    for (auto thread : threads) {
        thread->wait();
        thread->deleteLater();
    }
    return true;
}

void ImageEngineApi::setImgPathAndAlbumNames(const QMultiMap<QString, QString> &imgPahtAlbums)
{
    m_allPathAndAlbumNames.clear();
    m_allPathAndAlbumNames = imgPahtAlbums;
}

const QMultiMap<QString, QString> &ImageEngineApi::getImgPathAndAlbumNames()
{
    return m_allPathAndAlbumNames;
}

bool ImageEngineApi::reloadAfterFilterUnExistImage()
{
    ImageLoadFromDBThread *imagethread = new ImageLoadFromDBThread();
//    connect(imagethread, &ImageLoadFromDBThread::sigImageLoaded, this, &ImageEngineApi::sltImageDBLoaded);
//    connect(imagethread, &ImageLoadFromDBThread::sigInsert, this, &ImageEngineApi::sltInsert);
//    imagethread->setData(type, obj, name);
//    obj->addThread(imagethread);
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
}

bool ImageEngineApi::isVideo(QString path)
{
    QMutexLocker locker(&m_dataMutex);
    bool is = false;
    if (m_AllImageData.contains(path)) {
        ImageDataSt info = m_AllImageData[path];
        if (info.dbi.itemType == ItemTypeVideo) {
            is = true;
        }
    }
    return is;
}

int ImageEngineApi::getAllImageDataCount()
{
    QMutexLocker locker(&m_dataMutex);
    return m_AllImageData.size();
}

void ImageEngineApi::addImageData(QString path, ImageDataSt data)
{
    QMutexLocker locker(&m_dataMutex);
    m_AllImageData[path] = data;
}

void ImageEngineApi::clearAllImageData()
{
    QMutexLocker locker(&m_dataMutex);
    m_AllImageData.clear();
}

bool ImageEngineApi::isItemLoadedFromDB(QString path)
{
    QMutexLocker locker(&m_dataMutex);
    return m_AllImageData.contains(path);
}

//从外部启动，启用线程加载图片
bool ImageEngineApi::loadImagesFromNewAPP(QStringList files, ImageEngineImportObject *obj)
{
    ImageFromNewAppThread *imagethread = new ImageFromNewAppThread;
    imagethread->setDate(files, obj);
    obj->addThread(imagethread);
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
}

bool ImageEngineApi::importImageFilesFromMount(QString albumname, QStringList paths, ImageMountImportPathsObject *obj)
{
    emit dApp->signalM->popupWaitDialog(QObject::tr("Importing..."));
    ImageImportFilesFromMountThread *imagethread = new ImageImportFilesFromMountThread;
    connect(imagethread, &ImageImportFilesFromMountThread::sigImageFilesImported, this, &ImageEngineApi::sltImageFilesImported);
//    if (albumname == tr("Gallery")) {
//        albumname = "";
//    }
    imagethread->setData(albumname, paths, obj);
    obj->addThread(imagethread);
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
}
bool ImageEngineApi::moveImagesToTrash(QStringList files, bool typetrash, bool bneedprogress)
{
    if (files.size() == 0) {
        return false;
    }

    emit dApp->signalM->popupWaitDialog(tr("Deleting..."), bneedprogress); //author : jia.dong
    if (typetrash)  //如果为回收站删除，则删除内存数据
        removeImage(files);
    ImageMoveImagesToTrashThread *imagethread = new ImageMoveImagesToTrashThread;
    imagethread->setData(files, typetrash);
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
}
bool ImageEngineApi::recoveryImagesFromTrash(QStringList files)
{
    emit dApp->signalM->popupWaitDialog(tr("Restoring..."), false);
    ImageRecoveryImagesFromTrashThread *imagethread = new ImageRecoveryImagesFromTrashThread;
    imagethread->setData(files);
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
}

QStringList ImageEngineApi::get_AllImagePath()
{
    if (m_AllImageData.size() > 0)
        return m_AllImageData.keys();
    return QStringList();
}
