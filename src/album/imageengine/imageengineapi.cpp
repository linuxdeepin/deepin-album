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
#include "imagedataservice.h"

#define MINI_NEED_IDEAL_THREAD_COUNT 4

ImageEngineApi *ImageEngineApi::s_ImageEngine = nullptr;
static std::once_flag imageEngineFlag;

ImageEngineApi *ImageEngineApi::instance(QObject *parent)
{
    std::call_once(imageEngineFlag, [parent]() {
        s_ImageEngine = new ImageEngineApi(parent);
    });

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
    m_worker->stopRotate();
    m_worker->waitRotateStop();
#endif
}

ImageEngineApi::ImageEngineApi(QObject *parent)
{
    Q_UNUSED(parent);
    //文件加载线程池上限

    qRegisterMetaType<QStringList>("QStringList &");
    qRegisterMetaType<DBImgInfo>("ImageDataSt &");
    qRegisterMetaType<DBImgInfo>("ImageDataSt");
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList");
    qRegisterMetaType<QMap<QString, DBImgInfo>>("QMap<QString,DBImgInfo>");
    qRegisterMetaType<QVector<DBImgInfo>>("QVector<ImageDataSt>");
#ifdef NOGLOBAL
    m_qtpool.setMaxThreadCount(4);
    cacheThreadPool.setMaxThreadCount(4);
#else
    if (QThread::idealThreadCount() < MINI_NEED_IDEAL_THREAD_COUNT)
        QThreadPool::globalInstance()->setMaxThreadCount(MINI_NEED_IDEAL_THREAD_COUNT);
    else
        QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount() - 1);
    QThreadPool::globalInstance()->setExpiryTimeout(10);
#endif

    bcloseFg = false;
}

bool ImageEngineApi::insertObject(void *obj)
{
    m_AllObject.push_back(obj);
    return true;
}

bool ImageEngineApi::removeObject(void *obj)
{
    auto it = std::find(m_AllObject.begin(), m_AllObject.end(), obj);
    if (it != m_AllObject.end()) {
        m_AllObject.erase(it);
        return true;
    }
    return false;
}

bool ImageEngineApi::ifObjectExist(void *obj)
{
    return std::find(m_AllObject.begin(), m_AllObject.end(), obj) != m_AllObject.end();
}

void ImageEngineApi::sltImageFilesImported(void *imgobject, QStringList &filelist)
{
    if (nullptr != imgobject && ifObjectExist(imgobject)) {
        static_cast<ImageMountImportPathsObject *>(imgobject)->imageMountImported(filelist);
    }
}

bool ImageEngineApi::ImportImagesFromUrlList(QList<QUrl> files, const QString &albumname, int UID, ImageEngineImportObject *obj, bool bdialogselect, AlbumDBType dbType, bool isFirst)
{
    if (dbType != AutoImport || isFirst) {
        emit dApp->signalM->popupWaitDialog(QObject::tr("Importing..."));
    }

    ImportImagesThread *imagethread = new ImportImagesThread;
    imagethread->setData(files, albumname, UID, obj, bdialogselect, dbType, isFirst);
    obj->addThread(imagethread);
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
}

bool ImageEngineApi::ImportImagesFromFileList(QStringList files, const QString &albumname, int UID, ImageEngineImportObject *obj, bool bdialogselect, AlbumDBType dbType, bool isFirst)
{
    if (dbType != AutoImport || isFirst) {
        emit dApp->signalM->popupWaitDialog(QObject::tr("Importing..."));
    }

    ImportImagesThread *imagethread = new ImportImagesThread;
    imagethread->setData(files, albumname, UID, obj, bdialogselect, dbType, isFirst);
    obj->addThread(imagethread);
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
}

bool ImageEngineApi::removeImageFromAutoImport(const QStringList &files)
{
    //直接删除图片
    DBManager::instance()->removeImgInfos(files);

    return true;
}

void ImageEngineApi::loadFirstPageThumbnails(int num)
{
    qDebug() << __FUNCTION__ << "---";

    m_FirstPageScreen = num;

    thumbnailLoadThread();

    m_firstPageIsLoaded = true;
    emit sigLoadFirstPageThumbnailsToView();
}

void ImageEngineApi::stopRotate()
{
    m_worker->stopRotate();
}

void ImageEngineApi::waitRotateStop()
{
    m_worker->waitRotateStop();
}

void ImageEngineApi::thumbnailLoadThread()
{
    if (m_worker != nullptr) {
        return;
    }
    QThread *workerThread = new QThread(this);
    m_worker = new DBandImgOperate(workerThread);

    m_worker->moveToThread(workerThread);
    //加载设备中文件列表
    connect(this, &ImageEngineApi::sigLoadMountFileList, m_worker, &DBandImgOperate::sltLoadMountFileList);
    //同步设备卸载
    connect(this, &ImageEngineApi::sigDeciveUnMount, m_worker, &DBandImgOperate::sltDeciveUnMount);
    //旋转一张图片
    connect(this, &ImageEngineApi::sigRotateImageFile, m_worker, &DBandImgOperate::rotateImageFile);

    //收到获取全部照片信息成功信号
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

void ImageEngineApi::cleanUpTrash(const DBImgInfoList &list)
{
    RefreshTrashThread *imagethread = new RefreshTrashThread();
    imagethread->setData(list);
    QThreadPool::globalInstance()->start(imagethread);
}

bool ImageEngineApi::reloadAfterFilterUnExistImage()
{
    ImageLoadFromDBThread *imagethread = new ImageLoadFromDBThread();
#ifdef NOGLOBAL
    m_qtpool.start(imagethread);
#else
    QThreadPool::globalInstance()->start(imagethread);
#endif
    return true;
}

bool ImageEngineApi::importImageFilesFromMount(QString albumname, int UID, QStringList paths, ImageMountImportPathsObject *obj)
{
    emit dApp->signalM->popupWaitDialog(QObject::tr("Importing..."));
    ImageImportFilesFromMountThread *imagethread = new ImageImportFilesFromMountThread;
    connect(imagethread, &ImageImportFilesFromMountThread::sigImageFilesImported, this, &ImageEngineApi::sltImageFilesImported);
//    if (albumname == tr("Gallery")) {
//        albumname = "";
//    }
    imagethread->setData(albumname, UID, paths, obj);
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

    //非最近删除进来的，需要剔除存在且没有权限的部分
    if (!typetrash) {
        auto iter = std::remove_if(files.begin(), files.end(), [](const QString & eachFile) {
            QFileInfo info(eachFile);
            if (info.isSymLink()) {
                info = QFileInfo(info.readLink());
            }
            return !QFileInfo(info.dir(), info.dir().path()).isWritable() && info.exists();
        });
        files.erase(iter, files.end());
        if (files.isEmpty()) {
            return true;
        }
    }

    emit dApp->signalM->popupWaitDialog(tr("Deleting..."), bneedprogress); //author : jia.dong
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
