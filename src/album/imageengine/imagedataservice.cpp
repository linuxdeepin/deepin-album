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

ImageDataService *ImageDataService::s_ImageDataService = nullptr;

ImageDataService *ImageDataService::instance(QObject *parent)
{
    Q_UNUSED(parent);
    if (!s_ImageDataService) {
        s_ImageDataService = new ImageDataService();
    }
    return s_ImageDataService;
}

ImageDataService::~ImageDataService()
{
}

bool ImageDataService::add(const QStringList &paths)
{
    QMutexLocker locker(&m_imgDataMutex);
    m_requestQueue.clear();
    for (int i = 0; i < paths.size(); i++) {
        if (!m_AllImageMap.contains(paths.at(i))) {
            m_requestQueue.append(paths.at(i));
        }
    }
    return true;
}

bool ImageDataService::add(const QString &path)
{
    QMutexLocker locker(&m_imgDataMutex);
    if (!path.isEmpty()) {
        if (!m_AllImageMap.contains(path)) {
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
    return m_AllImageMap.count();
}

bool ImageDataService::readThumbnailByPaths(QStringList files)
{
    QStringList image_video_list;
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

    bool empty = isRequestQueueEmpty();

    if (empty) {
        ImageDataService::instance()->add(image_video_list);
        int needCoreCounts = static_cast<int>(std::thread::hardware_concurrency());
        needCoreCounts = needCoreCounts / 2;
        if (image_video_list.size() < needCoreCounts) {
            needCoreCounts = image_video_list.size();
        }
        if (needCoreCounts < 1)
            needCoreCounts = 1;
        QList<QThread *> threads;
        for (int i = 0; i < needCoreCounts; i++) {
            readThumbnailThread *thread = new readThumbnailThread;
            thread->start();
            threads.append(thread);
        }
        for (auto thread : threads) {
            thread->wait();
            thread->deleteLater();
        }
    } else {
        ImageDataService::instance()->add(image_video_list);
    }
    return true;
}

void ImageDataService::addImage(const QString &path, const QImage &image)
{
    QMutexLocker locker(&m_imgDataMutex);
    m_AllImageMap[path] = image;

//    if (!m_AllImageMap.contains(path)) {
//        m_AllImageMap[path] = image;
//        while (m_AllImageMap.size() > 1000) {
//            //保证缓存占用，始终只占用1000张缩略图缓存
//            QString res = m_imageKey.first();
//            m_imageKey.pop_front();
//            m_AllImageMap.remove(res);
//        }
//    }
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

void ImageDataService::setAllDataKeys(const QStringList &paths, bool single)
{
//    QMutexLocker locker(&m_imgDataMutex);
//    if (!single)
//        m_imageKeys.clear();
//    m_imageKeys.append(paths);
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
    return m_AllImageMap.contains(path) ? m_AllImageMap[path] : QImage();
}

bool ImageDataService::imageIsLoaded(const QString &path)
{
    QMutexLocker locker(&m_imgDataMutex);
    return m_AllImageMap.contains(path);
}

ImageDataService::ImageDataService(QObject *parent)
{
    Q_UNUSED(parent);
}

//缩略图读取线程
readThumbnailThread::readThumbnailThread(QObject *parent): QThread(parent)
{
}

readThumbnailThread::~readThumbnailThread()
{
    if (m_playlistModel) {
        delete  m_playlistModel;
        m_playlistModel = nullptr;
    }
}

void readThumbnailThread::readThumbnail(QString path)
{
    if (!QFileInfo(path).exists()) {
        return;
    }
    using namespace UnionImage_NameSpace;
    QImage tImg;
    QString srcPath = path;
    QString thumbnailPath = utils::base::filePathToThumbnailPath(path);

    QFileInfo file(thumbnailPath);
    QString errMsg;
    if (file.exists()) {
        if (!loadStaticImageFromFile(thumbnailPath, tImg, errMsg, "PNG")) {
            qDebug() << errMsg;
        }
        bool is = false;
        //获取视频信息 demo
        dmr::MovieInfo mi = m_playlistModel->getMovieInfo(QUrl::fromLocalFile(path), &is);
        ImageDataService::instance()->addMovieDurationStr(path, mi.durationStr());
    } else {
        //读图
        if (utils::base::isVideo(path)) {
            if (m_playlistModel == nullptr) {
                m_playlistModel = new dmr::PlaylistModel(nullptr);
            }

            tImg = m_playlistModel->getMovieCover(QUrl::fromLocalFile(path));

            bool is = false;
            //获取视频信息 demo
            dmr::MovieInfo mi = m_playlistModel->getMovieInfo(QUrl::fromLocalFile(path), &is);
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
        tImg.save(thumbnailPath, "PNG");
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
    }
    ImageDataService::instance()->addImage(path, tImg);
}

void readThumbnailThread::setQuit(bool quit)
{
    m_quit = quit;
}

void readThumbnailThread::run()
{
    while (!ImageDataService::instance()->isRequestQueueEmpty()) {
        if (m_quit) {
            break;
        }
        QString res = ImageDataService::instance()->pop();
        if (!res.isEmpty()) {
            readThumbnail(res);
        }
    }
    emit ImageDataService::instance()->sigeUpdateListview();
}
