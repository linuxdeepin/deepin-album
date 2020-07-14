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
#include "config.h"

#include "controller/configsetter.h"
#include "controller/globaleventfilter.h"
#include "controller/signalmanager.h"
#include "controller/viewerthememanager.h"
#include "controller/wallpapersetter.h"
#include "utils/snifferimageformat.h"
#include "utils/unionimage.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "thumbnail/thumbnaillistview.h"
#include "imageengine/imageengineapi.h"
#include "mainwindow.h"

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
const QString CACHE_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + "deepin" + QDir::separator() + "deepin-album"/* + QDir::separator()*/;
}
// namespace

#define IMAGE_HEIGHT_DEFAULT    100
#define IMAGE_LOAD_DEFAULT    100

ImageLoader::ImageLoader(Application *parent, QStringList pathlist, QStringList pathlisttrash)
    : m_parent(parent), m_pathlist(pathlist), m_pathlisttrash(pathlisttrash)
{

}


void ImageLoader::ImportImageLoader(DBImgInfoList dbInfos, QString albumname)
{
    DBImgInfoList dbInfoList;
    QStringList pathlist;

    for (auto info : dbInfos) {
        pathlist << info.filePath;
        dbInfoList << info;
    }

    bool bcustalbum = false;
    if (albumname.length() > 0) {
        DBManager::instance()->insertIntoAlbumNoSignal(albumname, pathlist);
        bcustalbum = true;
    }
    DBManager::instance()->insertImgInfos(dbInfoList);
    if (pathlist.size() > 0) {
        emit dApp->signalM->updateStatusBarImportLabel(pathlist, 1, bcustalbum, albumname);
    } else {
        emit dApp->signalM->ImportFailed();
    }
}


void ImageLoader::updateImageLoader(QStringList pathlist)
{

    for (QString path : pathlist) {
        QImage tImg;
        QString errMsg;
        if (!UnionImage_NameSpace::loadStaticImageFromFile(path, tImg, errMsg)) {
            qDebug()  << errMsg;
            continue;
        }
        QPixmap pixmap = QPixmap::fromImage(tImg);
        if (0 != pixmap.height() && 0 != pixmap.width() && (pixmap.height() / pixmap.width()) < 10 && (pixmap.width() / pixmap.height()) < 10) {
            if (pixmap.height() != 100 && pixmap.width() != 100) {
                if (pixmap.height() >= pixmap.width()) {
                    pixmap = pixmap.scaledToWidth(100,  Qt::FastTransformation);
                } else if (pixmap.height() <= pixmap.width()) {
                    pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
                }
            }
        }
        if (pixmap.isNull()) {
            pixmap = QPixmap::fromImage(tImg);
        }

        //            QBuffer buffer(&m_baThumb);
        //            buffer.open(QIODevice::WriteOnly);
        QString spath = CACHE_PATH + path;
        utils::base::mkMutiDir(spath.mid(0, spath.lastIndexOf('/')));
        pixmap.save(spath, "PNG");
        if (!ImageEngineApi::instance()->updateImageDataPixmap(path, pixmap)) {
            continue;
        }
    }
    emit dApp->signalM->sigUpdateImageLoader(pathlist);
    // m_parent->m_imagemap[path] = pixmap;
}


Application::Application(int &argc, char **argv)
    : DApplication(argc, argv)
{
    //设置单例
    setupsinglecase();
    initI18n();

    setApplicationDisplayName(tr("Album"));
    setProductIcon(QIcon::fromTheme("deepin-album"));
    setApplicationVersion(DApplication::buildVersion(VERSION));
    //setApplicationVersion(ALBUM_VERSION);

//    setApplicationDescription(DApplication::translate("Main","相册是一款可多种方式浏览照片、整理照片和简单编辑的相册管理工具。"));
    setApplicationDescription(DApplication::translate("Main", "Album is a fashion photo manager for viewing and organizing pictures."));
    installEventFilter(new GlobalEventFilter(this));
    initChildren();
//    initDB();

}

Application::~Application()
{
//    if (m_imageloader) {
//        delete m_imageloader;
//        m_imageloader = nullptr;
//    }
}


void Application::finishLoadSlot()
{
    qDebug() << "finishLoadSlot";
    emit sigFinishLoad();
}

void Application::initChildren()
{
    viewerTheme = ViewerThemeManager::instance();
    setter = ConfigSetter::instance();
    signalM = SignalManager::instance();
    wpSetter = WallpaperSetter::instance();
}

void Application::setupsinglecase()
{
    QSharedMemory mem("deepin-album");
    // 尝试将进程附加到共享内存段
    if (mem.attach()) {
        // 将共享内存与主进程分离, 如果此进程是附加到共享存储器段的最后一个进程，则系统释放共享存储器段，即销毁内容
        mem.detach();
    }
    sharedMemory.setKey("deepin-album");


    if (sharedMemory.attach())
        _isRunning = true;
    else {
        _isRunning = false;
        // attach data to shared memory.
        QByteArray byteArray("0"); // default value to note that no message is available.
        if (!sharedMemory.create(byteArray.size())) {
            qDebug("无法创建实例化");
            return;
        }
        sharedMemory.lock();
        char *to = static_cast<char *>(sharedMemory.data());
        const char *from = byteArray.data();
        memcpy(to, from, static_cast<size_t>(qMin(sharedMemory.size(), byteArray.size())));
        sharedMemory.unlock();

        // start checking for messages of other instances.
        QTimer *timer = new QTimer(this);
        connect(timer, SIGNAL(timeout()), this, SLOT(checkForMessage()));
        timer->start(1000);
    }
}

bool Application::isRunning()
{
    return _isRunning;
}

bool Application::sendMessage(const QString &message)
{
    if (!_isRunning)
        return false;

    QByteArray byteArray("1");
    byteArray.append(message.toUtf8());
    byteArray.append("0"); // < should be as char here, not a string!
    sharedMemory.lock();
    char *to = static_cast<char *>(sharedMemory.data());
    const char *from = byteArray.data();
    memcpy(to, from, static_cast<size_t>(qMin(sharedMemory.size(), byteArray.size())));
    sharedMemory.unlock();
    return true;
}

void Application::checkForMessage()
{
    sharedMemory.lock();
    QByteArray byteArray = QByteArray(static_cast<const char *>(sharedMemory.constData()), sharedMemory.size());
    sharedMemory.unlock();
    if (byteArray.left(1) == "0")
        return;
    byteArray.remove(0, 1);
    QString message = QString::fromUtf8(byteArray.constData());
    emit messageAvailable(message);

    // remove message from shared memory.
    byteArray = "0";
    sharedMemory.lock();
    char *to = static_cast<char *>(sharedMemory.data());
    const char *from = byteArray.data();
    memcpy(to, from, static_cast<size_t>(qMin(sharedMemory.size(), byteArray.size())));
    sharedMemory.unlock();
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
