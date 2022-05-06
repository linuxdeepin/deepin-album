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
#include "imagedataservice.h"
#include "application.h"
#include <QMetaType>
#include <QDirIterator>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include "utils/unionimage.h"
#include "utils/baseutils.h"
#include "albumgloabl.h"
#include "imageengineapi.h"
#include "baseutils.h"
#include "imageutils.h"
#include "movieservice.h"
#include "signalmanager.h"

ImageDataService *ImageDataService::s_ImageDataService = nullptr;

ImageDataService *ImageDataService::instance(QObject *parent)
{
    Q_UNUSED(parent);
    if (!s_ImageDataService) {
        s_ImageDataService = new ImageDataService();
    }
    return s_ImageDataService;
}

bool ImageDataService::pathInMap(const QString &path)
{
    auto iter = std::find_if(m_AllImageMap.begin(), m_AllImageMap.end(), [path](const std::pair<QString, QImage> &pr) {
        return pr.first == path;
    });
    return iter != m_AllImageMap.end();
}

std::pair<QImage, bool> ImageDataService::getImageFromMap(const QString &path)
{
    QMutexLocker locker(&m_imgDataMutex);

    auto iter = std::find_if(m_AllImageMap.begin(), m_AllImageMap.end(), [path](const std::pair<QString, QImage> &pr) {
        return pr.first == path;
    });
    if (iter != m_AllImageMap.end()) {
        return std::make_pair(iter->second, true);
    } else {
        return std::make_pair(QImage(), false);
    }
}

void ImageDataService::addImage(const QString &path, const QImage &image)
{
    QMutexLocker locker(&m_imgDataMutex);

    auto iter = std::find_if(m_AllImageMap.begin(), m_AllImageMap.end(), [path](const std::pair<QString, QImage> &pr) {
        if (pr.first != path) {
            return false;
        }
        return true;
    });
    if (iter != m_AllImageMap.end()) {
        iter->second = image;
    } else {
        m_AllImageMap.push_back(std::make_pair(path, image));
        if (m_AllImageMap.size() > 500) {
            m_AllImageMap.pop_front();
        }
    }
}

void ImageDataService::addMovieDurationStr(const QString &path, const QString &durationStr)
{
    QMutexLocker locker(&m_imgDataMutex);
    m_movieDurationStrMap[path] = durationStr;
}

QString ImageDataService::getMovieDurationStrByPath(const QString &path)
{
    QMutexLocker locker(&m_imgDataMutex);
    return m_movieDurationStrMap.contains(path) ? m_movieDurationStrMap[path] : QString() ;
}

bool ImageDataService::imageIsLoaded(const QString &path, bool isTrashFile)
{
    QMutexLocker locker(&m_imgDataMutex);

    if (isTrashFile) {
        QString realPath = utils::base::getDeleteFullPath(utils::base::hashByString(path), DBImgInfo::getFileNameFromFilePath(path));
        return pathInMap(realPath) || pathInMap(path);
    } else {
        return pathInMap(path);
    }
}

ImageDataService::ImageDataService(QObject *parent) : QObject (parent)
{
    readThumbnailManager = new ReadThumbnailManager;
    readThread = new QThread;
    readThumbnailManager->moveToThread(readThread);
    readThread->start();
    connect(this, &ImageDataService::startImageLoad, readThumbnailManager, &ReadThumbnailManager::readThumbnail);
    connect(dApp->signalM, &SignalManager::needReflushThumbnail, this, &ImageDataService::onNeedReflushThumbnail, Qt::QueuedConnection);
}

void ImageDataService::onNeedReflushThumbnail(const QStringList &paths)
{
    for (const auto &path : paths) {
        //1.删除缩略图
        QString thumbnailPath = utils::base::filePathToThumbnailPath(path);
        QFile::remove(thumbnailPath);

        //2.加进load队列
        if (imageIsLoaded(path, false)) {
            readThumbnailManager->addLoadPath(path);
        }

        //3.激活加载队列
        if (!readThumbnailManager->isRunning()) {
            emit startImageLoad();
        }
    }
}

void ImageDataService::stopFlushThumbnail()
{
    readThumbnailManager->stopRead();
}

void ImageDataService::waitFlushThumbnailFinish()
{
    while (ImageDataService::instance()->readThumbnailManager->isRunning());
}

bool ImageDataService::readerIsRunning()
{
    return readThumbnailManager->isRunning();
}

QImage ImageDataService::getThumnailImageByPathRealTime(const QString &path, bool isTrashFile)
{
    QString realPath;

    if (!isTrashFile) {
        realPath = path;
        if (!QFile::exists(realPath)) {
            return QImage();
        }
    } else {
        realPath = utils::base::getDeleteFullPath(utils::base::hashByString(path), DBImgInfo::getFileNameFromFilePath(path));
        if (!QFile::exists(realPath)) {
            if (!QFile::exists(path)) {
                return QImage();
            } else {
                realPath = path;
            }
        }
    }

    //尝试在缓存里面找图
    auto bufferImage = getImageFromMap(realPath);
    if (bufferImage.second) {
        return bufferImage.first;
    }

    //缓存没找到则加入图片加载队列
    readThumbnailManager->addLoadPath(realPath);

    //如果加载队列正在休眠，则发信号唤醒，反之不去反复发信号激活队列
    if (!readThumbnailManager->isRunning()) {
        emit startImageLoad();
    }

    return QImage();
}

ReadThumbnailManager::ReadThumbnailManager(QObject *parent)
    : QObject (parent)
    , runningFlag(false)
    , stopFlag(false)
{
}

void ReadThumbnailManager::addLoadPath(const QString &path)
{
    mutex.lock();
    needLoadPath.push_back(path);
    if (needLoadPath.size() > 100) {
        needLoadPath.pop_front();
    }
    mutex.unlock();
}

void ReadThumbnailManager::readThumbnail()
{
    int sendCounter = 0; //刷新上层界面指示
    runningFlag = true;  //告诉外面加载队列处于激活状态

    while (1) {
        //尝试读取队列数据
        mutex.lock();

        if (needLoadPath.empty() || stopFlag) {
            mutex.unlock();
            break;
        }

        //锁定文件操作权限
        DBManager::m_fileMutex.lockForRead();

        auto path = needLoadPath[needLoadPath.size() - 1];
        needLoadPath.pop_back();

        mutex.unlock();

        sendCounter++;
        if (sendCounter == 5) { //每加载5张图，就让上层界面主动刷新一次
            sendCounter = 0;
            emit ImageDataService::instance()->sigeUpdateListview();
        }

        if (!QFileInfo(path).exists()) {
            DBManager::m_fileMutex.unlock();
            continue;
        }
        using namespace UnionImage_NameSpace;
        QImage tImg;
        QString srcPath = path;
        QString thumbnailPath = utils::base::filePathToThumbnailPath(path);

        QFileInfo thumbnailFile(thumbnailPath);
        QString errMsg;
        if (thumbnailFile.exists()) {
            if (!loadStaticImageFromFile(thumbnailPath, tImg, errMsg, "PNG")) {
                qDebug() << errMsg;
                //不正常退出导致的缩略图损坏，删除原文件后重新尝试制作
                QFile::remove(thumbnailPath);
                if (!loadStaticImageFromFile(srcPath, tImg, errMsg)) {
                    qDebug() << errMsg;
                }
            }

            if (utils::base::isVideo(path)) {
                //获取视频信息 demo
                MovieInfo mi = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(path));
                ImageDataService::instance()->addMovieDurationStr(path, mi.duration);
            }
        } else {
            //读图
            if (utils::base::isVideo(path)) {
                tImg = MovieService::instance()->getMovieCover(QUrl::fromLocalFile(path));

                //获取视频信息 demo
                MovieInfo mi = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(path));
                ImageDataService::instance()->addMovieDurationStr(path, mi.duration);
            } else {
                if (!loadStaticImageFromFile(srcPath, tImg, errMsg)) {
                    qDebug() << errMsg;
                    ImageDataService::instance()->addImage(path, tImg);
                    DBManager::m_fileMutex.unlock();
                    continue;
                }
            }
            //裁切
            if (!tImg.isNull() && 0 != tImg.height() && 0 != tImg.width() && (tImg.height() / tImg.width()) < 10 && (tImg.width() / tImg.height()) < 10) {
                bool cache_exist = false;
                if (tImg.height() != 200 && tImg.width() != 200) {
                    if (tImg.height() >= tImg.width()) {
                        cache_exist = true;
                        tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
                    } else if (tImg.height() <= tImg.width()) {
                        cache_exist = true;
                        tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
                    }
                }
                if (!cache_exist) {
                    if ((static_cast<float>(tImg.height()) / (static_cast<float>(tImg.width()))) > 3) {
                        tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
                    } else {
                        tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
                    }
                }
            }
            utils::base::mkMutiDir(thumbnailPath.mid(0, thumbnailPath.lastIndexOf('/')));
        }
        if (!tImg.isNull()) {
            int width = tImg.width();
            int height = tImg.height();
            if (abs((width - height) * 10 / width) >= 1) {
                QRect rect = tImg.rect();
                int x = rect.x() + width / 2;
                int y = rect.y() + height / 2;
                if (width > height) {
                    x = x - height / 2;
                    y = 0;
                    tImg = tImg.copy(x, y, height, height);
                } else {
                    y = y - width / 2;
                    x = 0;
                    tImg = tImg.copy(x, y, width, width);
                }
            }
            if (!thumbnailFile.exists()) {
                tImg.save(thumbnailPath, "PNG"); //保存裁好的缩略图，下次读的时候直接刷进去
            }
        }
        ImageDataService::instance()->addImage(path, tImg);
        DBManager::m_fileMutex.unlock();
    }

    if (!stopFlag) {
        emit ImageDataService::instance()->sigeUpdateListview(); //最后让上层界面刷新
    }

    runningFlag = false; //告诉外面加载队列处于休眠状态
}
