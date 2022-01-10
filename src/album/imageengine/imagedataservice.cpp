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
        if (pr.first.size() != path.size()) {
            return false;
        }
        for (auto rIter_lhs = pr.first.rbegin(), rIter_rhs = path.rbegin(); rIter_lhs != pr.first.rend() && rIter_rhs != path.rend(); ++rIter_lhs, ++rIter_rhs) {
            if (*rIter_lhs != *rIter_rhs) {
                return false;
            }
        }
        return true;
    });
    if (iter != m_AllImageMap.end()) {
        return true;
    } else {
        return false;
    }
}

QImage ImageDataService::getImageFromMap(const QString &path)
{
    auto iter = std::find_if(m_AllImageMap.begin(), m_AllImageMap.end(), [path](const std::pair<QString, QImage> &pr) {
        if (pr.first.size() != path.size()) {
            return false;
        }
        for (auto rIter_lhs = pr.first.rbegin(), rIter_rhs = path.rbegin(); rIter_lhs != pr.first.rend() && rIter_rhs != path.rend(); ++rIter_lhs, ++rIter_rhs) {
            if (*rIter_lhs != *rIter_rhs) {
                return false;
            }
        }
        return true;
    });
    if (iter != m_AllImageMap.end()) {
        return iter->second;
    } else {
        return QImage();
    }
}

bool ImageDataService::add(const QStringList &paths, bool reLoadThumbnail)
{
    QMutexLocker locker(&m_imgDataMutex);
    if (reLoadThumbnail) {
        m_requestQueue.clear();
        qDebug() << "---m_requestQueue 清空,重新加载---" << paths.size();
    }
    for (int i = 0; i < paths.size(); i++) {
        if (reLoadThumbnail || !pathInMap(paths.at(i))) {
            m_requestQueue.append(paths.at(i));
        }
    }
    return true;
}

bool ImageDataService::add(const QString &path)
{
    QMutexLocker locker(&m_imgDataMutex);
    if (!path.isEmpty()) {
        if (!pathInMap(path)) {
            m_requestQueue.append(path);
        }
    }
    return true;
}

QString ImageDataService::pop()
{
    QMutexLocker locker(&m_imgDataMutex);
    if (m_requestQueue.empty())
        return QString();
    QString res = m_requestQueue.first();
    m_requestQueue.pop_front();
    return res;
}

bool ImageDataService::isRequestQueueEmpty()
{
    QMutexLocker locker(&m_imgDataMutex);
    return m_requestQueue.isEmpty();
}

int ImageDataService::getCount()
{
    QMutexLocker locker(&m_imgDataMutex);

    return static_cast<int>(m_AllImageMap.size());
}

bool ImageDataService::readThumbnailByPaths(QStringList files, bool isFinishFilter, bool reLoadThumbnail)
{
#if 1
    Q_UNUSED(files)
    Q_UNUSED(isFinishFilter)
    Q_UNUSED(reLoadThumbnail)
    return true;
#else
    QStringList image_video_list;
    if (!isFinishFilter) {
        foreach (QString path, files) {
            QFileInfo file(path);
            if (file.isDir()) {
                QFileInfoList infos;
                QDirIterator dirIterator(path,
                                         QDir::Files,
                                         QDirIterator::Subdirectories);
                while (dirIterator.hasNext()) {
                    dirIterator.next();
                    QFileInfo fi = dirIterator.fileInfo();
                    image_video_list << fi.absoluteFilePath();
                }
            } else if (file.exists()) { //文件存在
                image_video_list << path;
            }
        }
    } else {
        image_video_list = files;
    }

    if (image_video_list.isEmpty())
        return true;

    bool empty = isRequestQueueEmpty();

    if (empty) {
        ImageDataService::instance()->add(image_video_list, reLoadThumbnail);
        int needCoreCounts = static_cast<int>(std::thread::hardware_concurrency()) - 1;
        if (image_video_list.size() < needCoreCounts) {
            needCoreCounts = image_video_list.size();
        }
        if (needCoreCounts < 1)
            needCoreCounts = 1;
        for (int i = 0; i < needCoreCounts; i++) {
            readThumbnailThread *thread = new readThumbnailThread;
            thread->start();
            connect(thread, &readThumbnailThread::finished, thread, &readThumbnailThread::deleteLater);
        }
    } else {
        ImageDataService::instance()->add(image_video_list, reLoadThumbnail);
    }
    return true;
#endif
}

void ImageDataService::addImage(const QString &path, const QImage &image)
{
    QMutexLocker locker(&m_imgDataMutex);

    auto iter = std::find_if(m_AllImageMap.begin(), m_AllImageMap.end(), [path](const std::pair<QString, QImage> &pr) {
        if (pr.first.size() != path.size()) {
            return false;
        }
        for (auto rIter_lhs = pr.first.rbegin(), rIter_rhs = path.rbegin(); rIter_lhs != pr.first.rend() && rIter_rhs != path.rend(); ++rIter_lhs, ++rIter_rhs) {
            if (*rIter_lhs != *rIter_rhs) {
                return false;
            }
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

void ImageDataService::setVisualIndex(int row)
{
    QMutexLocker locker(&m_imgDataMutex);
    m_visualIndex = row;
}

int ImageDataService::getVisualIndex()
{
    QMutexLocker locker(&m_imgDataMutex);
    return m_visualIndex;
}

QImage ImageDataService::getThumnailImageByPath(const QString &path)
{
    QMutexLocker locker(&m_imgDataMutex);
    if (path.isEmpty()) {
        return  QImage();
    }
    return getImageFromMap(path);
}

bool ImageDataService::imageIsLoaded(const QString &path)
{
    QMutexLocker locker(&m_imgDataMutex);
    return pathInMap(path);
}

ImageDataService::ImageDataService(QObject *parent) : QObject (parent)
{
    readThumbnailManager = new ReadThumbnailManager;
    readThread = new QThread;
    readThumbnailManager->moveToThread(readThread);
    readThread->start();
    connect(this, &ImageDataService::startImageLoad, readThumbnailManager, &ReadThumbnailManager::readThumbnail);
}

QImage ImageDataService::getThumnailImageByPathRealTime(const QString &path)
{
    if (!QFileInfo(path).exists()) {
        return QImage();
    }

    auto bufferImage = getImageFromMap(path);
    if (!bufferImage.isNull()) {
        return bufferImage;
    }

    readThumbnailManager->addLoadPath(path);
    emit startImageLoad();

    return QImage();
}

ReadThumbnailManager::ReadThumbnailManager(QObject *parent) : QObject (parent)
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
    int sendCounter = 0;

    while (1) {
        mutex.lock();

        if (needLoadPath.empty()) {
            mutex.unlock();
            break;
        }

        auto path = needLoadPath[needLoadPath.size() - 1];
        needLoadPath.pop_back();

        mutex.unlock();

        sendCounter++;
        if (sendCounter == 5) {
            sendCounter = 0;
            emit ImageDataService::instance()->sigeUpdateListview();
        }

        if (!QFileInfo(path).exists()) {
            return;
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
            }

            if (utils::base::isVideo(path)) {
                //获取视频信息 demo
                MovieInfo mi = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(path));
                ImageDataService::instance()->addMovieDurationStr(path, mi.durationStr());
            }
        } else {
            //读图
            if (utils::base::isVideo(path)) {
                tImg = MovieService::instance()->getMovieCover(QUrl::fromLocalFile(path));

                //获取视频信息 demo
                MovieInfo mi = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(path));
                ImageDataService::instance()->addMovieDurationStr(path, mi.durationStr());
            } else {
                if (!loadStaticImageFromFile(srcPath, tImg, errMsg)) {
                    qDebug() << errMsg;
                    ImageDataService::instance()->addImage(path, tImg);
                    return;
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
    }

    emit ImageDataService::instance()->sigeUpdateListview();
}
