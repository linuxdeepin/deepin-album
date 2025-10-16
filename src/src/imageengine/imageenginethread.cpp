// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imageenginethread.h"
#include "dbmanager/dbmanager.h"
#include "unionimage/unionimage.h"
#include "unionimage/baseutils.h"
#include "albumControl.h"
#include "utils/classifyutils.h"
#include <QDebug>

#include <QDirIterator>

ImageEngineThreadObject::ImageEngineThreadObject()
{
    setAutoDelete(false); //从根源上禁止auto delete
    qDebug() << "ImageEngineThreadObject initialized";
}

void ImageEngineThreadObject::needStop(void *imageobject)
{
    if (nullptr == imageobject || ifCanStopThread(imageobject)) {
        bneedstop = true;
        bbackstop = true;
        qDebug() << "Thread stop requested";
    }
}

bool ImageEngineThreadObject::ifCanStopThread(void *imgobject)
{
    Q_UNUSED(imgobject);
    return true;
}

void ImageEngineThreadObject::run()
{
    qDebug() << "Starting thread execution";
    runDetail(); //原本要run的内容
    emit runFinished(); //告诉m_obj我run完了，这里不再像之前那样直接判断不为nullptr然后解引用m_obj，因为m_obj销毁后不会自动置为nullptr
    qDebug() << "Thread execution completed";
    this->deleteLater(); //在消息传达后销毁自己
}

ImportImagesThread::ImportImagesThread()
{
    qDebug() << "Initializing ImportImagesThread";
    connect(this, &ImportImagesThread::sigRepeatUrls, AlbumControl::instance(), &AlbumControl::sigRepeatUrls);
    connect(this, &ImportImagesThread::sigImportProgress, AlbumControl::instance(), &AlbumControl::sigImportProgress);
    connect(this, &ImportImagesThread::sigImportFinished, AlbumControl::instance(), &AlbumControl::sigImportFinished);
    connect(this, &ImportImagesThread::sigImportFailed, AlbumControl::instance(), &AlbumControl::sigImportFailed);
    //通知前端刷新相关界面
    connect(this, &ImportImagesThread::sigImportFinished, AlbumControl::instance(), &AlbumControl::sigRefreshAllCollection);
    connect(this, &ImportImagesThread::sigImportFinished, AlbumControl::instance(), &AlbumControl::sigRefreshImportAlbum);
    connect(this, &ImportImagesThread::sigImportFinished, AlbumControl::instance(), &AlbumControl::sigRefreshSearchView);
    connect(this, &ImportImagesThread::sigImportFinished, [=]() {
        emit AlbumControl::instance()->sigRefreshCustomAlbum(-1);
    });
}

ImportImagesThread::~ImportImagesThread()
{
    qDebug() << "ImportImagesThread destroyed";
}

void ImportImagesThread::setData(const QStringList &paths, const int UID)
{
    qDebug() << "Setting string list data with" << paths.size() << "paths for UID:" << UID;
    for (QUrl path : paths) {
        m_paths << LibUnionImage_NameSpace::localPath(path);
    }
    m_UID = UID;
    m_type = DataType_String;
}

void ImportImagesThread::setData(const QList<QUrl> &paths, const int UID, const bool checkRepeat)
{
    qDebug() << "Setting URL list data with" << paths.size() << "paths for UID:" << UID << "checkRepeat:" << checkRepeat;
    for (QUrl path : paths) {
        m_paths << LibUnionImage_NameSpace::localPath(path);
    }
    m_UID = UID;
    m_checkRepeat = checkRepeat;
    m_type = DataType_Url;
}

void ImportImagesThread::setNotifyUI(bool bValue)
{
    qDebug() << "Setting notify UI flag to:" << bValue;
    m_notifyUI = bValue;
}

bool ImportImagesThread::ifCanStopThread(void *imgobject)
{
    Q_UNUSED(imgobject);
    return true;
}

void ImportImagesThread::runDetail()
{
    qDebug() << "Starting import process for UID:" << m_UID;
    //相册中本次导入之前已导入的所有路径
    DBImgInfoList oldInfos = AlbumControl::instance()->getAllInfosByUID(QString::number(m_UID));
    QStringList allOldImportedPaths;
    for (DBImgInfo info : oldInfos) {
        allOldImportedPaths.push_back(info.filePath);
    }
    qDebug() << "Found" << allOldImportedPaths.size() << "previously imported paths";

    QStringList tempPaths;
    QStringList filePaths;
    DBImgInfoList dbInfos;
    //判断是否含有目录
    for (QString path : m_paths) {
        //是目录，向下遍历,得到所有文件
        if (QDir(path).exists()) {
            qDebug() << "Processing directory:" << path;
            QFileInfoList infos = LibUnionImage_NameSpace::getImagesAndVideoInfo(path, true);

            std::transform(infos.begin(), infos.end(), std::back_inserter(tempPaths), [](const QFileInfo & info) {
                return info.absoluteFilePath();
            });
            qDebug() << "Found" << infos.size() << "files in directory";
        } else {//非目录
            qDebug() << "Processing file:" << path;
            tempPaths << path;
        }
    }

    //条件过滤
    int noReadCount = 0; //记录已存在于相册中的数量，若全部存在，则不进行导入操作
    int i = 0;
    for (QString imagePath : tempPaths) {
        i++;
        //已导入
        if (allOldImportedPaths.contains(imagePath)) {
            qDebug() << "Skipping already imported file:" << imagePath;
            m_checkRepeat = true;
            noReadCount++;
            continue;
        }

        //当前文件存在和可读
        QFileInfo info(imagePath);
        if (info.exists() && info.isReadable()) {
            //去掉不支持的图片和视频
            bool bIsVideo = LibUnionImage_NameSpace::isVideo(imagePath);
            if (!bIsVideo && !LibUnionImage_NameSpace::imageSupportRead(imagePath)) {
                qWarning() << "Skipping unsupported file:" << imagePath;
                continue;
            }

            //去掉格式错误无法解析的图片和视频
            auto dbInfo = AlbumControl::instance()->getDBInfo(imagePath, bIsVideo);
            if (ItemType::ItemTypeNull == dbInfo.itemType) {
                qWarning() << "Skipping file with invalid format:" << imagePath;
                continue;
            }
            dbInfo.albumUID = QString::number(m_UID);
            dbInfos << dbInfo;

            filePaths << imagePath;
            qDebug() << "Added file to import list:" << imagePath;
        } else {
            qWarning() << "Skipping inaccessible file:" << imagePath;
        }
        emit sigImportProgress(i, tempPaths.size());
    }

    //已全部存在，无需导入
    if (noReadCount == tempPaths.size() && tempPaths.size() > 0 && m_checkRepeat) {
        qDebug() << "All files already exist in album, skipping import";
        QStringList urlPaths;
        for (QString path : tempPaths) {
            urlPaths.push_back("file://" + path);
        }
        emit sigRepeatUrls(urlPaths);
        return;
    }
    if (filePaths.isEmpty()) {
        // 存在无法导入
        int skiped = tempPaths.size() - noReadCount;
        qWarning() << "No valid files to import, skipped:" << skiped;
        emit sigImportFailed(skiped);
        return;
    }

    qDebug() << "Sorting" << dbInfos.size() << "files by change time";
    std::sort(dbInfos.begin(), dbInfos.end(), [](const DBImgInfo & lhs, const DBImgInfo & rhs) {
        return lhs.changeTime > rhs.changeTime;
    });

    //导入图片数据库ImageTable3
    qDebug() << "Inserting" << dbInfos.size() << "images into database";
    DBManager::instance()->insertImgInfos(dbInfos);

    //导入图片数据库AlbumTable3
    if (m_UID >= 0) {
        AlbumDBType atype = AlbumDBType::AutoImport;
        if (m_UID == 0) {
            atype = AlbumDBType::Favourite;
        }
        qDebug() << "Inserting" << filePaths.size() << "files into album" << m_UID << "type:" << static_cast<int>(atype);
        DBManager::instance()->insertIntoAlbum(m_UID, filePaths, atype);
    }

    //原createNewCustomAutoImportAlbum逻辑
    if (m_UID > 0) {
        qDebug() << "Refreshing UI for custom album" << m_UID;
        emit AlbumControl::instance()->sigRefreshSlider();
        emit AlbumControl::instance()->sigAddCustomAlbum(m_UID);
    }

    QThread::msleep(100);
    //发送导入完成信号
    if (m_notifyUI) {
        qDebug() << "Import process completed, notifying UI";
        emit sigImportFinished();
    } else {
        qDebug() << "Import process completed without UI notification";
    }
}

ImagesClassifyThread::ImagesClassifyThread()
{

}

ImagesClassifyThread::~ImagesClassifyThread()
{

}

void ImagesClassifyThread::setData(const DBImgInfoList &infos)
{
    m_infos = infos;
}

void ImagesClassifyThread::runDetail()
{
    int infoCount = m_infos.size();

    emit AlbumControl::instance()->progressOfImageClassify(infoCount, 0);
    int i = 0 ;
    for (auto &info : m_infos) {
        if (info.className.isEmpty()) {
            QFileInfo srcfi(info.filePath);
            if (srcfi.exists() && Libutils::base::isSupportClassify(info.filePath) && srcfi.isReadable())
                info.className = Classifyutils::GetInstance()->imageClassify(info.filePath.toStdString().c_str());
            else
                info.className = "";
            if (info.className.isEmpty() && srcfi.isReadable())
                info.className = "Other";

            emit AlbumControl::instance()->progressOfImageClassify(infoCount, i++);
        }
    }
    DBManager::instance()->updateClassName2DB(m_infos);

    m_infos.clear();

    emit AlbumControl::instance()->sigImageClassifyFinished();
}
