/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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
#include "application.h"

#include "controller/configsetter.h"
#include "controller/globaleventfilter.h"
#include "controller/signalmanager.h"
#include "controller/viewerthememanager.h"
#include "controller/wallpapersetter.h"
#include "utils/snifferimageformat.h"

#include <QDebug>
#include <QTranslator>
#include <QIcon>
#include <QImageReader>
#include <sys/time.h>
#include <DApplicationSettings>
#include <QFile>
#include <DLog>
namespace {

}  // namespace

#define IMAGE_HEIGHT_DEFAULT    100

ImageLoader::ImageLoader(Application* parent, QStringList pathlist, QStringList pathlisttrash)
{
    m_parent = parent;
    m_pathlist = pathlist;
    m_pathlisttrash = pathlisttrash;
}


void ImageLoader::startLoading()
{
    struct timeval tv;
    long long ms;
    gettimeofday(&tv,NULL);
    ms = (long long)tv.tv_sec*1000 + tv.tv_usec/1000;
    qDebug()<<"startLoading start time: "<<ms;


    for(QString path : m_pathlist)
    {
        QImage tImg;

        QString format = DetectImageFormat(path);
        if (format.isEmpty()) {
            QImageReader reader(path);
            reader.setAutoTransform(true);
            if (reader.canRead()) {
                tImg = reader.read();
            }
        } else {
            QImageReader readerF(path, format.toLatin1());
            readerF.setAutoTransform(true);
            if (readerF.canRead()) {
                tImg = readerF.read();
            } else {
                qWarning() << "can't read image:" << readerF.errorString()
                           << format;

                tImg = QImage(path);
            }
        }
        QPixmap pixmap = QPixmap::fromImage(tImg);

        m_parent->m_imagemap.insert(path, pixmap.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation));
    }

    for(QString path : m_pathlisttrash)
    {
        QImage tImg;

        QString format = DetectImageFormat(path);
        if (format.isEmpty()) {
            QImageReader reader(path);
            reader.setAutoTransform(true);
            if (reader.canRead()) {
                tImg = reader.read();
            }
        } else {
            QImageReader readerF(path, format.toLatin1());
            readerF.setAutoTransform(true);
            if (readerF.canRead()) {
                tImg = readerF.read();
            } else {
                qWarning() << "can't read image:" << readerF.errorString()
                           << format;

                tImg = QImage(path);
            }
        }
        QPixmap pixmaptrash = QPixmap::fromImage(tImg);

        m_parent->m_imagetrashmap.insert(path, pixmaptrash.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation));
    }

    qDebug()<<m_parent->m_imagemap.keys();
    emit sigFinishiLoad();

    gettimeofday(&tv,NULL);
    ms = (long long)tv.tv_sec*1000 + tv.tv_usec/1000;
    qDebug()<<"startLoading end time: "<<ms;
}

void ImageLoader::addImageLoader(QStringList pathlist)
{
    for(QString path : pathlist)
    {
        QImage tImg;

        QString format = DetectImageFormat(path);
        if (format.isEmpty()) {
            QImageReader reader(path);
            reader.setAutoTransform(true);
            if (reader.canRead()) {
                tImg = reader.read();
            }
        } else {
            QImageReader readerF(path, format.toLatin1());
            readerF.setAutoTransform(true);
            if (readerF.canRead()) {
                tImg = readerF.read();
            } else {
                qWarning() << "can't read image:" << readerF.errorString()
                           << format;

                tImg = QImage(path);
            }
        }
        QPixmap pixmap = QPixmap::fromImage(tImg);

        m_parent->m_imagemap.insert(path, pixmap.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation));
    }
}

void ImageLoader::updateImageLoader(QStringList pathlist)
{
    for(QString path : pathlist)
    {
        QPixmap pixmap(path);

        m_parent->m_imagemap[path] = pixmap.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation);
    }
}

void ImageLoader::addTrashImageLoader(QStringList trashpathlist)
{
    for(QString path : trashpathlist)
    {
        QImage tImg;

        QString format = DetectImageFormat(path);
        if (format.isEmpty()) {
            QImageReader reader(path);
            reader.setAutoTransform(true);
            if (reader.canRead()) {
                tImg = reader.read();
            }
        } else {
            QImageReader readerF(path, format.toLatin1());
            readerF.setAutoTransform(true);
            if (readerF.canRead()) {
                tImg = readerF.read();
            } else {
                qWarning() << "can't read image:" << readerF.errorString()
                           << format;

                tImg = QImage(path);
            }
        }
        QPixmap pixmaptrash = QPixmap::fromImage(tImg);

        m_parent->m_imagetrashmap.insert(path, pixmaptrash.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation));
    }
}

void ImageLoader::updateTrashImageLoader(QStringList trashpathlist)
{
    for(QString path : trashpathlist)
    {
        QPixmap pixmaptrash(path);

        m_parent->m_imagetrashmap[path] = pixmaptrash.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation);
    }
}

Application::Application(int& argc, char** argv)
    : DApplication(argc, argv)
{
    initI18n();
    setApplicationDisplayName(tr("深度相册"));
    setProductIcon(QIcon::fromTheme("deepin-album"));
    setApplicationVersion(DApplication::buildVersion("20191011"));
    setApplicationDescription(QString("%1\n%2\n").arg(tr("深度相册是深度操作系统自带的相册软件。")).arg(tr("满足对照片的常用功能，快速、轻巧、使用简单。")));



    installEventFilter(new GlobalEventFilter());

    initChildren();

    initDB();

}

void Application::LoadDbImage()
{
    auto infos = DBManager::instance()->getAllInfos();
    QStringList pathlist;
    foreach(auto info, infos)
    {
        pathlist.append(info.filePath);
    }

    auto infostrash = DBManager::instance()->getAllTrashInfos();
    QStringList pathlisttrash;
    foreach(auto info, infostrash)
    {
        pathlisttrash.append(info.filePath);
    }

    m_imageloader= new ImageLoader(this, pathlist, pathlisttrash);
    m_LoadThread = new QThread();

    m_imageloader->moveToThread(m_LoadThread);
    m_LoadThread->start();

    connect(this, SIGNAL(sigstartLoad()), m_imageloader, SLOT(startLoading()));
    connect(m_imageloader, SIGNAL(sigFinishiLoad()), this, SLOT(finishLoadSlot()));
    qDebug()<<"emit sigstartLoad();";
    emit sigstartLoad();
}

void Application::finishLoadSlot()
{
    qDebug()<<"finishLoadSlot";
    emit sigFinishLoad();
}

void Application::initChildren()
{
    viewerTheme = ViewerThemeManager::instance();
    setter = ConfigSetter::instance();
    signalM = SignalManager::instance();
    wpSetter = WallpaperSetter::instance();
}

void Application::initDB()
{
    QStringList removePaths;
    QStringList removeTrashPaths;

    auto infos = DBManager::instance()->getAllInfos();
    for(auto info : infos)
    {
        QFile file(info.filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "DetectImageFormat() failed to open file:" << info.filePath;
            removePaths<<info.filePath;
        }
    }

    auto trashInfos = DBManager::instance()->getAllTrashInfos();
    for(auto info : trashInfos)
    {
        QFile file(info.filePath);
        if (!file.open(QIODevice::ReadOnly)) {
            qWarning() << "DetectImageFormat() failed to open file:" << info.filePath;
            removeTrashPaths<<info.filePath;
        }
    }

    DBManager::instance()->removeImgInfosNoSignal(removePaths);
    DBManager::instance()->removeTrashImgInfosNoSignal(removeTrashPaths);
}

void Application::initI18n()
{
    // install translators
//    QTranslator *translator = new QTranslator;
//    translator->load(APPSHAREDIR"/translations/deepin-image-viewer_"
//                     + QLocale::system().name() + ".qm");
//    installTranslator(translator);
    loadTranslator(QList<QLocale>() << QLocale::system());
}
