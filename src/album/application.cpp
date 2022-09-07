// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "application.h"
#include "config.h"

#include "controller/configsetter.h"
#include "controller/globaleventfilter.h"
#include "controller/signalmanager.h"
#include "controller/viewerthememanager.h"
#include "controller/wallpapersetter.h"
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

// namespace

#define IMAGE_HEIGHT_DEFAULT    100
#define IMAGE_LOAD_DEFAULT    100

ImageLoader::ImageLoader(/*Application *parent, QStringList pathlist, QStringList pathlisttrash)
    : m_parent(parent), m_pathlist(pathlist), m_pathlisttrash(pathlisttrash*/)
{

}

//这个函数只有右键使用相册打开的流程在用，所以这里直接流程简化
void ImageLoader::ImportImageLoader(const DBImgInfoList &dbInfos)
{
    QStringList pathlist;

    for (auto info : dbInfos) {
        pathlist << info.filePath;
    }

    //导入图片数据库ImageTable3
    DBManager::instance()->insertImgInfos(dbInfos);
    if (pathlist.size() > 0) {
        emit dApp->signalM->updateStatusBarImportLabel(pathlist, 1, "");
    } else {
        emit dApp->signalM->ImportFailed();
    }
}

void ImageLoader::updateImageLoader(const QStringList &pathlist, const QList<QImage> &images)
{
    for (int i = 0; i != pathlist.size(); ++i) {
        QImage tImg;
        if (images.isEmpty() || images.size() < i || images.at(i).isNull()) {
            QString errMsg;
            if (!UnionImage_NameSpace::loadStaticImageFromFile(pathlist.at(i), tImg, errMsg)) {
                qDebug()  << errMsg;
                continue;
            }
        } else {
            tImg = images.at(i);
        }

        tImg = utils::base::getThumbnailFromImage(tImg, 200);
        QString spath = albumGlobal::CACHE_PATH + pathlist.at(i);
        utils::base::mkMutiDir(spath.mid(0, spath.lastIndexOf('/')));
        tImg.save(spath, "PNG");

        QPixmap pixmap = QPixmap::fromImage(tImg);
        //不需要在此处更新图片
        //ImageEngineApi::instance()->updateImageDataPixmap(pathlist.at(i), pixmap);
        emit ImageEngineApi::instance()->sigOneImgReady(pathlist.at(i), pixmap);
    }

    emit dApp->signalM->sigUpdateImageLoader(pathlist);
}

CustomDApplication::CustomDApplication(int &argc, char *argv[])
    : DApplication(argc, argv)
{
}

void CustomDApplication::handleQuitAction()
{
    window->close();
}

void CustomDApplication::setMainWindow(MainWindow *newWindow)
{
    window = newWindow;
}

DApplication *Application::dAppNew = nullptr;
Application *Application::dApp1 = nullptr;
Application::Application()
{
    //设置单例
    setupsinglecase();

    installEventFilter(new GlobalEventFilter(this));
    initChildren();
    m_imageloader = new ImageLoader();
    m_mainwindow = nullptr;
}

Application::~Application()
{
//    if (m_imageloader) {
//        delete m_imageloader;
//        m_imageloader = nullptr;
    //    }
}

DApplication *Application::getDAppNew()
{
    return dAppNew;
}

Application *Application::getApp()
{
    if (dApp1 == nullptr)
        dApp1 = new Application;
    return dApp1;
}

void Application::setApp(DApplication *app)
{
    dAppNew = app;
    dAppNew->setApplicationDisplayName(tr("Album"));
    dAppNew->setProductIcon(QIcon::fromTheme("deepin-album"));
    dAppNew->setApplicationVersion(DApplication::buildVersion(VERSION));
    dAppNew->setApplicationDescription(DApplication::translate("Main", "Album is a fashion manager for viewing and organizing photos and videos."));
}

bool Application::isWaylandPlatform()
{
    auto e = QProcessEnvironment::systemEnvironment();
    QString XDG_SESSION_TYPE = e.value(QStringLiteral("XDG_SESSION_TYPE"));
    QString WAYLAND_DISPLAY = e.value(QStringLiteral("WAYLAND_DISPLAY"));

    //判断wayland
    if (XDG_SESSION_TYPE != QLatin1String("wayland") && !WAYLAND_DISPLAY.contains(QLatin1String("wayland"), Qt::CaseInsensitive)) {
        return false;
    }
    return true;
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
    //既然封了函数，就把变量改成函数
    if (!isRunning())
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

void Application::setMainWindow(MainWindow *window)
{
    if (nullptr != window) {
        m_mainwindow = window;
    }
}

MainWindow *Application::getMainWindow()
{
    return m_mainwindow;
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
