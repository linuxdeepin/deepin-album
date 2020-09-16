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

namespace {
const QString CACHE_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                           + QDir::separator() + "deepin" + QDir::separator() + "deepin-album"/* + QDir::separator()*/;
}

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
    qDebug() << "xigou current Threads:" << QThreadPool::globalInstance()->activeThreadCount();
#endif
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
#ifdef NOGLOBAL
    m_qtpool.setMaxThreadCount(4);
    cacheThreadPool.setMaxThreadCount(4);
#else
    QThreadPool::globalInstance()->setMaxThreadCount(12);
    QThreadPool::globalInstance()->setExpiryTimeout(10);
#endif
    for (int i = 0; i < 3; i++) {
        QThread *workerThread = new QThread(this);
        DBandImgOperate *worker = new DBandImgOperate(workerThread);
        if (i == 0) {
            worker->m_loadBegin = 0;
            worker->m_loadEnd = 14;
        } else if (i == 1) {
            worker->m_loadBegin = 14;
            worker->m_loadEnd = 33;
        } else if (i == 2) {
            worker->m_loadBegin = 33;
            worker->m_loadEnd = 50;
        }

        worker->moveToThread(workerThread);
        //开始录制
        connect(this, SIGNAL(sigLoad80Thumbnails(DBImgInfoList)),
                worker, SLOT(threadSltLoad80Thumbnail(DBImgInfoList)));
        //收到获取全部照片信息成功信号
        connect(worker, &DBandImgOperate::sig80ImgInfosReady, this, &ImageEngineApi::slt80ImgInfosReady);
        workerThread->start();
    }
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
    QMap<void *, void *>::iterator it;
    it = m_AllObject.find(obj);
    if (it != m_AllObject.end()) {
        return true;
    }
    return false;
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
    QMap<QString, ImageDataSt>::iterator it;
    it = m_AllImageData.find(imagepath);
    if (it != m_AllImageData.end()) {
        m_AllImageData.erase(it);
        return true;
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

bool ImageEngineApi::insertImage(QString imagepath, QString remainDay)
{
    QMap<QString, ImageDataSt>::iterator it;
    it = m_AllImageData.find(imagepath);
    ImageDataSt data;
    if (it != m_AllImageData.end()) {
        if ("" == remainDay) {
            return false;
        }
        data = it.value();
    }
    if ("" != remainDay)
        data.remainDays = remainDay;
    m_AllImageData.insert(imagepath, data);
    return true;
}

void ImageEngineApi::sltInsert(QString imagepath, QString remainDay)
{
    insertImage(imagepath, remainDay);
}

bool ImageEngineApi::updateImageDataPixmap(QString imagepath, QPixmap &pix)
{
    ImageDataSt data;
    if (getImageData(imagepath, data)) {
        data.imgpixmap = pix;
        m_AllImageData[imagepath] = data;

        QFileInfo file(CACHE_PATH + imagepath);
        if (file.exists()) {
            QFile::remove(CACHE_PATH + imagepath);
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

//载入图片实际位置
bool ImageEngineApi::reQuestImageData(QString imagepath, ImageEngineObject *obj, bool needcache, bool useGlobalThreadPool)
{
    if (nullptr == obj) {
        return false;
    }
    ImageDataSt data;
    if (!getImageData(imagepath, data)) {
        return false;
    }
    dynamic_cast<ImageEngineObject *>(obj)->addCheckPath(imagepath);
    if (ImageLoadStatu_Loaded == data.loaded) {
        dynamic_cast<ImageEngineObject *>(obj)->checkAndReturnPath(imagepath);
    } else if (ImageLoadStatu_PreLoaded == data.loaded) {
        data.dbi = getDBInfo(imagepath);
        data.loaded = ImageLoadStatu_Loaded;
        m_AllImageData[imagepath] = data;
        //DBManager::instance()->insertImgInfos(DBImgInfoList() << dbi);
        dynamic_cast<ImageEngineObject *>(obj)->checkAndReturnPath(imagepath);
    } else if (ImageLoadStatu_BeLoading == data.loaded && nullptr != data.thread && ifObjectExist(data.thread)) {
        obj->addThread(dynamic_cast<ImageEngineThreadObject *>(data.thread));
        dynamic_cast<ImageEngineThread *>(data.thread)->addObject(obj);
    } else {
        ImageEngineThread *imagethread = new ImageEngineThread;
        connect(imagethread, &ImageEngineThread::sigImageLoaded, this, &ImageEngineApi::sltImageLoaded);
        connect(imagethread, &ImageEngineThread::sigAborted, this, &ImageEngineApi::sltAborted);
        data.thread = imagethread;
        data.loaded = ImageLoadStatu_BeLoading;
        m_AllImageData[imagepath] = data;
        imagethread->setData(imagepath, obj, data, needcache);
        obj->addThread(imagethread);
#ifdef NOGLOBAL
        m_qtpool.start(imagethread);
#else
        if (useGlobalThreadPool) {
            QThreadPool::globalInstance()->start(imagethread);
        } else {
            if (m_pool == nullptr) {
                m_pool = new QThreadPool(this);
                m_pool->setMaxThreadCount(1);
            }
            m_pool->start(imagethread);
        }
#endif
    }
    return true;
}

bool ImageEngineApi::imageNeedReload(QString imagepath)
{
    QMap<QString, ImageDataSt>::iterator it;
    it = m_AllImageData.find(imagepath);
    if (it == m_AllImageData.end()) {
        return false;
    }
    ImageDataSt data = it.value();
    data.loaded = ImageLoadStatu_False;
    m_AllImageData[imagepath] = data;
    return true;
}

void ImageEngineApi::sltAborted(QString path)
{
    removeImage(path);
    ImageEngineThread *thread = dynamic_cast<ImageEngineThread *>(sender());
    if (nullptr != thread)
        thread->needStop(nullptr);
}

void ImageEngineApi::sltImageLoaded(void *imgobject, QString path, ImageDataSt &data)
{
    m_AllImageData[path] = data;
//    ImageEngineThread *thread = dynamic_cast<ImageEngineThread *>(sender());
//    if (nullptr != thread)
//        thread->needStop(imgobject);
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        static_cast<ImageEngineObject *>(imgobject)->checkAndReturnPath(path);
    }
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

void ImageEngineApi::sigImageBackLoaded(QString path, ImageDataSt data)
{
    m_AllImageData[path] = data;
}
int static loadCount = 0;
void ImageEngineApi::slt80ImgInfosReady(QMap<QString, ImageDataSt> ImageData)
{
    loadCount++;
    m_AllImageData.unite(ImageData);
    if (loadCount == 3) {
        m_80isLoaded = true;
        emit sigLoad80ThumbnailsToView();
    }
}

bool ImageEngineApi::loadImagesFromTrash(DBImgInfoList files, ImageEngineObject *obj)
{
    ImageLoadFromLocalThread *imagethread = new ImageLoadFromLocalThread;
    connect(imagethread, &ImageLoadFromLocalThread::sigImageLoaded, this, &ImageEngineApi::sltImageLocalLoaded);
    connect(imagethread, &ImageLoadFromLocalThread::sigInsert, this, &ImageEngineApi::sltInsert);
    imagethread->setData(files, obj, true, ImageLoadFromLocalThread::DataType_TrashList);
    obj->addThread(imagethread);
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
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
    emit dApp->signalM->popupWaitDialog(tr("Importing..."));
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
    emit dApp->signalM->popupWaitDialog(tr("Importing..."));
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
bool ImageEngineApi::loadImagesFromPath(ImageEngineObject *obj, QString path)
{
    sltImageDBLoaded(obj, QStringList() << path);
    insertImage(path, "30");
    return true;
}

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

void ImageEngineApi::load80Thumbnails()
{
    DBImgInfoList infos;
    QSqlDatabase db = DBManager::instance()->getDatabase();
    if (!db.isValid()) {
        return;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime FROM ImageTable3 order by Time desc limit 50");
    if (!query.exec()) {
        qDebug() << "zy------50 Get data from ImageTable3 failed: " << query.lastError();
        //emit sigAllImgInfosReady(infos);
        return;
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(query.value(5).toString(), DATETIME_FORMAT_DATABASE);
            infos << info;
        }
    }
    emit sigLoad80Thumbnails(infos);
    db.close();
}
bool ImageEngineApi::loadImagesFromDB(ThumbnailDelegate::DelegateType type, ImageEngineObject *obj, QString name, int loadCount)
{
    ImageLoadFromDBThread *imagethread = new ImageLoadFromDBThread(loadCount);
    connect(imagethread, &ImageLoadFromDBThread::sigImageLoaded, this, &ImageEngineApi::sltImageDBLoaded);
    connect(imagethread, &ImageLoadFromDBThread::sigInsert, this, &ImageEngineApi::sltInsert);
    imagethread->setData(type, obj, name);
    obj->addThread(imagethread);
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
}

bool ImageEngineApi::SaveImagesCache(QStringList files)
{
    if (!m_imageCacheSaveobj) {
        m_imageCacheSaveobj = new ImageCacheSaveObject;
        connect(dApp->signalM, &SignalManager::cacheThreadStop, this, &ImageEngineApi::sltstopCacheSave);
    }
    m_imageCacheSaveobj->add(files);
    int needCoreCounts = static_cast<int>(std::thread::hardware_concurrency());
    if (needCoreCounts * 100 > files.size()) {
        if (files.empty()) {
            needCoreCounts = 0;
        } else {
#ifdef NOGLOABL
            needCoreCounts = (files.size() / 100) + 1 - cacheThreadPool.activeThreadCount();
#else
            needCoreCounts = (files.size() / 100) + 1 - QThreadPool::globalInstance()->activeThreadCount();
#endif
        }
    }
    if (needCoreCounts < 1)
        needCoreCounts = 1;
    for (int i = 0; i < needCoreCounts; i++) {
        ImageCacheQueuePopThread *thread = new ImageCacheQueuePopThread;
        thread->setObject(m_imageCacheSaveobj);
#ifdef NOGLOBAL
        cacheThreadPool.start(thread);
#else
        QThreadPool::globalInstance()->start(thread);
#endif
        qDebug() << "current Threads:" << QThreadPool::globalInstance()->activeThreadCount();
    }
    return true;
}

int ImageEngineApi::CacheThreadNum()
{
#ifdef  NOGLOBAL
    return cacheThreadPool.activeThreadCount();
#else
    return  QThreadPool::globalInstance()->activeThreadCount();
#endif
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
bool ImageEngineApi::getImageFilesFromMount(QString mountname, QString path, ImageMountGetPathsObject *obj)
{
    ImageGetFilesFromMountThread *imagethread = new ImageGetFilesFromMountThread;
    connect(imagethread, &ImageGetFilesFromMountThread::sigImageFilesGeted, this, &ImageEngineApi::sltImageFilesGeted);
    imagethread->setData(mountname, path, obj);
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
    emit dApp->signalM->popupWaitDialog(tr("Importing..."));
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
    emit dApp->signalM->popupWaitDialog(tr("Deleting..."), bneedprogress); //autor : jia.dong
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
    return m_AllImageData.keys();
}
