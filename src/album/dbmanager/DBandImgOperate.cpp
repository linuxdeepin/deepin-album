// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DBandImgOperate.h"
#include "dbmanager.h"
#include "application.h"
#include "controller/signalmanager.h"
#include "utils/baseutils.h"
#include "utils/unionimage.h"
#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QDirIterator>
#include <QElapsedTimer>

#include "movieservice.h"
#include "imagedataservice.h"

DBandImgOperate::DBandImgOperate(QObject *parent)
{
    Q_UNUSED(parent);
    m_ImgPaths.clear();
    m_couldRun.store(true);
    m_rotateNeedStop = false;
    m_rotateIsRunning = false;
}

DBandImgOperate::~DBandImgOperate()
{
}

void DBandImgOperate::setThreadShouldStop()
{
    m_couldRun.store(false);
}

//QPixmap DBandImgOperate::loadOneThumbnail(const QString &imagepath)
//{
//    if (!QFileInfo(imagepath).exists()) {
//        emit fileIsNotExist(imagepath);
//        return QPixmap();
//    }
//    using namespace UnionImage_NameSpace;
//    QImage tImg;
//    QString srcPath = imagepath;
//    QString thumbnailPath = albumGlobal::CACHE_PATH + imagepath;
//    QFileInfo file(thumbnailPath);
//    QString errMsg;
//    QFileInfo srcfi(srcPath);
//    if (file.exists()) {
//        if (!loadStaticImageFromFile(thumbnailPath, tImg, errMsg, "PNG")) {
//            qDebug() << errMsg;
//        }
//    } else {
//        if (!loadStaticImageFromFile(srcPath, tImg, errMsg)) {
//            qDebug() << errMsg;
//        }
//    }
//    if (0 != tImg.height() && 0 != tImg.width() && (tImg.height() / tImg.width()) < 10 && (tImg.width() / tImg.height()) < 10) {
//        bool cache_exist = false;
//        if (tImg.height() != 200 && tImg.width() != 200) {
//            if (tImg.height() >= tImg.width()) {
//                cache_exist = true;
//                tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
//            } else if (tImg.height() <= tImg.width()) {
//                cache_exist = true;
//                tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
//            }
//        }
//        if (!cache_exist) {
//            if ((static_cast<float>(tImg.height()) / (static_cast<float>(tImg.width()))) > 3) {
//                tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
//            } else {
//                tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
//            }
//        }
//    }

//    if (!file.exists()) {
//        utils::base::mkMutiDir(thumbnailPath.mid(0, thumbnailPath.lastIndexOf('/')));
//        tImg.save(thumbnailPath, "PNG");
//    }

//    QPixmap pixmap = QPixmap::fromImage(tImg);
//    return pixmap;
//}

//void DBandImgOperate::loadOneImg(QString imagepath)
//{
//    loadOneImgForce(imagepath, false);
//}

void DBandImgOperate::loadOneImgForce(QString imagepath, bool refresh)
{
    if (!QFileInfo(imagepath).exists()) {
        emit fileIsNotExist(imagepath);
        return;
    }
    using namespace UnionImage_NameSpace;
    QImage tImg;
    QString srcPath = imagepath;
    //缩略图路径
    QString thumbnailPath = utils::base::filePathToThumbnailPath(imagepath);
    QFileInfo file(thumbnailPath);
    QString errMsg;
    QFileInfo srcfi(srcPath);
    if (file.exists() && !refresh) {
        if (!loadStaticImageFromFile(thumbnailPath, tImg, errMsg, "PNG")) {
            qDebug() << errMsg;
        }
        if (utils::base::isVideo(imagepath)) {
            //获取视频信息 demo
            MovieInfo mi = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(imagepath));
            ImageDataService::instance()->addMovieDurationStr(imagepath, mi.duration);
        }
    } else {
        if (utils::base::isVideo(imagepath)) {
            tImg = MovieService::instance()->getMovieCover(QUrl::fromLocalFile(imagepath));
            //获取视频信息 demo
            MovieInfo mi = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(imagepath));
            ImageDataService::instance()->addMovieDurationStr(imagepath, mi.duration);
        } else {
            if (!loadStaticImageFromFile(srcPath, tImg, errMsg)) {
                qDebug() << errMsg;
                return;
            }
        }
    }

    tImg = utils::base::getThumbnailFromImage(tImg, 200);

    if (!file.exists()) {
        utils::base::mkMutiDir(thumbnailPath.mid(0, thumbnailPath.lastIndexOf('/')));
        tImg.save(thumbnailPath, "PNG");
    }

    QPixmap pixmap = QPixmap::fromImage(tImg);
    emit sigOneImgReady(imagepath, pixmap);
}

void DBandImgOperate::rotateImageFile(int angel, const QStringList &paths)
{
    QString errMsg;
    //如果角度为0，不选择，重新加载
    if (angel != 0) {
        m_rotateIsRunning = true;

        QElapsedTimer timer;
        timer.start();
        QStringList loadedPaths;
        int sendCount = 0;
        for (const auto &eachPath : paths) {
            if (!UnionImage_NameSpace::rotateImageFile(angel, eachPath, errMsg)) {
                qDebug() << errMsg;
            } else {
                loadedPaths.push_back(eachPath);
                if (timer.elapsed() > 500) {
                    timer.restart();
                    sendCount += loadedPaths.size();
                    emit dApp->signalM->needReflushThumbnail(loadedPaths);
                    loadedPaths.clear();
                }
            }
            if (m_rotateNeedStop) {
                break;
            }
        }
        if (sendCount != paths.size()) {
            emit dApp->signalM->needReflushThumbnail(loadedPaths);
        }

        m_rotateIsRunning = false;
    }
}

bool DBandImgOperate::isRotating()
{
    return m_rotateIsRunning;
}

void DBandImgOperate::waitRotateStop()
{
    while (m_rotateIsRunning);
}

void DBandImgOperate::stopRotate()
{
    m_rotateNeedStop = true;
}

void DBandImgOperate::sltLoadMountFileList(const QString &path)
{
    QString strPath = path;
    if (!m_PhonePicFileMap.contains(strPath)) {
        m_couldRun.store(true);
        //获取所选文件类型过滤器
        QStringList filters;
        for (QString i : UnionImage_NameSpace::unionImageSupportFormat()) {
            filters << "*." + i;
        }

        for (QString i : utils::base::m_videoFiletypes) {
            filters << "*." + i;
        }
        //定义迭代器并设置过滤器，包括子目录：QDirIterator::Subdirectories
        QDirIterator dir_iterator(strPath,
                                  filters,
                                  QDir::Files | QDir::NoSymLinks,
                                  QDirIterator::Subdirectories);
        QStringList allfiles;
        while (dir_iterator.hasNext()) {
            //需要停止则跳出循环
            if (!m_couldRun.load()) {
                break;
            }
            dir_iterator.next();
            allfiles << dir_iterator.filePath();
            if (allfiles.size() == 50) {
                emit sigMountFileListLoadReady(strPath, allfiles);
            }
        }
        //重置标志位，可以执行线程
        m_couldRun.store(true);
        m_PhonePicFileMap[strPath] = allfiles;
        emit sigMountFileListLoadReady(strPath, allfiles);
    } else {
        //已加载过的设备，直接发送缓存的路径
        emit sigMountFileListLoadReady(strPath, m_PhonePicFileMap[strPath]);
    }
}

void DBandImgOperate::sltDeciveUnMount(const QString &path)
{
    if (m_PhonePicFileMap.contains(path)) {
        qDebug() << "------DBandImgOperate::sltDeciveUnMount同步状态-----";
        m_PhonePicFileMap.remove(path);
    }
}
