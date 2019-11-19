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
#include "utils/baseutils.h"
#include "utils/imageutils.h"

#include <QDebug>
#include <QTranslator>
#include <QIcon>
#include <QImageReader>
#include <sys/time.h>
#include <DApplicationSettings>
#include <QFile>
#include <DLog>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <DAboutDialog>

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
            else if (path.contains(".tga")) {
                bool ret = false;
                tImg = utils::image::loadTga(path, ret);
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
        pixmaptrash = pixmaptrash.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation);

        if (800 < pixmaptrash.width())
        {
            pixmaptrash = pixmaptrash.scaledToWidth(800,  Qt::FastTransformation);
        }

        m_parent->m_imagetrashmap.insert(path, pixmaptrash);
    }

    int num = 0;
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
            else if (path.contains(".tga")) {
                bool ret = false;
                tImg = utils::image::loadTga(path, ret);
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
        pixmap = pixmap.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation);

        if (800 < pixmap.width())
        {
            pixmap = pixmap.scaledToWidth(800,  Qt::FastTransformation);
        }

        m_parent->m_imagemap.insert(path, pixmap);
        num += 1;
        if (0 == num%100)
        {
            emit sigFinishiLoad();
        }
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
            else if (path.contains(".tga")) {
                bool ret = false;
                tImg = utils::image::loadTga(path, ret);
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
        pixmap = pixmap.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation);

        if (800 < pixmap.width())
        {
            pixmap = pixmap.scaledToWidth(800,  Qt::FastTransformation);
        }
        m_parent->m_imagemap.insert(path, pixmap);
    }
}

void ImageLoader::updateImageLoader(QStringList pathlist)
{
    for(QString path : pathlist)
    {
        QPixmap pixmap(path);
        pixmap = pixmap.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation);

        if (800 < pixmap.width())
        {
            pixmap = pixmap.scaledToWidth(800,  Qt::FastTransformation);
        }

        m_parent->m_imagemap[path] = pixmap;
    }

    emit dApp->signalM->sigUpdateImageLoader();
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
            else if (path.contains(".tga")) {
                bool ret = false;
                tImg = utils::image::loadTga(path, ret);
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
        pixmaptrash = pixmaptrash.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation);

        if (800 < pixmaptrash.width())
        {
            pixmaptrash = pixmaptrash.scaledToWidth(800,  Qt::FastTransformation);
        }

        m_parent->m_imagetrashmap.insert(path, pixmaptrash);
    }
}

void ImageLoader::updateTrashImageLoader(QStringList trashpathlist)
{
    for(QString path : trashpathlist)
    {
        QPixmap pixmaptrash(path);

        pixmaptrash = pixmaptrash.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation);

        if (800 < pixmaptrash.width())
        {
            pixmaptrash = pixmaptrash.scaledToWidth(800,  Qt::FastTransformation);
        }
        m_parent->m_imagetrashmap[path] = pixmaptrash;
    }
	emit dApp->signalM->sigUpdateTrashImageLoader();
}

void ImageLoader::onLoadMountImagesStart(QString mountName, QString path)
{
    //判断路径是否存在
    QDir dir(path);
    if (!dir.exists()) return;

    //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
    if(!path.contains("/media/")) {
        bool bFind = findPicturePathByPhone(path);
        if(!bFind) return;
    }

    //获取所选文件类型过滤器
    QStringList filters;
    filters << QString("*.jpeg") << QString("*.jpg");

    //定义迭代器并设置过滤器
    QDirIterator dir_iterator(path,
                              filters,
                              QDir::Files | QDir::NoSymLinks,
                              QDirIterator::Subdirectories);

    m_phoneImgPathList.clear();

    while (dir_iterator.hasNext()) {
        dir_iterator.next();
        QFileInfo fileInfo = dir_iterator.fileInfo();

        QImage tImg;

        QString format = DetectImageFormat(fileInfo.filePath());
        if (format.isEmpty()) {
            QImageReader reader(fileInfo.filePath());
            reader.setAutoTransform(true);
            if (reader.canRead()) {
                tImg = reader.read();
            }
            else if (path.contains(".tga")) {
                bool ret = false;
                tImg = utils::image::loadTga(path, ret);
            }
        } else {
            QImageReader readerF(fileInfo.filePath(), format.toLatin1());
            readerF.setAutoTransform(true);
            if (readerF.canRead()) {
                tImg = readerF.read();
            } else {
                qWarning() << "can't read image:" << readerF.errorString()
                           << format;

                tImg = QImage(fileInfo.filePath());
            }
        }

        QPixmap pixmap = QPixmap::fromImage(tImg);
        pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
        if(800 < pixmap.width())
        {
            pixmap = pixmap.scaledToWidth(800,  Qt::FastTransformation);
        }

        m_parent->m_phonePathAndImage.insert(fileInfo.filePath(), pixmap);

        m_phoneImgPathList<<fileInfo.filePath();
    }


    if (0 < m_phoneImgPathList.length())
    {
        m_parent->m_phoneNameAndPathlist.insert(mountName, m_phoneImgPathList);

        dApp->signalM->sigLoadMountImagesEnd(mountName);
    }
}

//搜索手机中存储相机照片文件的路径，采用两级文件目录深度，找"DCIM"文件目录
//经过调研，安卓手机在path/外部存储设备/DCIM下，iPhone在patn/DCIM下
bool ImageLoader::findPicturePathByPhone(QString &path)
{
    QDir dir(path);
    if (!dir.exists()) return false;

    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfo tempFileInfo;
    foreach (tempFileInfo, fileInfoList) {
        if (tempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0)
        {
            path = tempFileInfo.absoluteFilePath();
            return true;
        } else {
            QDir subDir;
            subDir.setPath(tempFileInfo.absoluteFilePath());

            QFileInfoList subFileInfoList = subDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            QFileInfo subTempFileInfo;
            foreach (subTempFileInfo, subFileInfoList) {
                if (subTempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0)
                {
                    path = subTempFileInfo.absoluteFilePath();
                    return true;
                }
            }
            return false;
        }
    }

    return false;
}


Application::Application(int& argc, char** argv)
    : DApplication(argc, argv)
{
    initI18n();
    setApplicationDisplayName(tr("相册"));
    setProductIcon(QIcon::fromTheme("deepin-album"));
    setApplicationVersion(DApplication::buildVersion("20191011"));
//    setApplicationDescription(QString("%1\n%2\n").arg(tr("相册是一款可多种方式浏览照片、")).arg(tr("整理照片和简单编辑的相册管理工具。")));
    setApplicationDescription(DApplication::translate("Main","相册是一款可多种方式浏览照片、整理照片和简单编辑的相册管理工具。"));
    installEventFilter(new GlobalEventFilter());
    initChildren();
    initDB();
}

Application::~Application()
{
    if(m_imageloader)
    {
        delete m_imageloader;
        m_imageloader = nullptr;
    }
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
    connect(this, SIGNAL(sigLoadMountImagesStart(QString, QString)), m_imageloader, SLOT(onLoadMountImagesStart(QString, QString)));
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
