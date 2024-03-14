// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "wallpapersetter.h"
#include "application.h"
#include "unionimage.h"
#include "baseutils.h"
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QImage>
#include <QDebug>
#include <QDBusInterface>
#include <QtDBus>
#include <QGuiApplication>
#include <QScreen>
#include <QDir>

#include <unistd.h>

namespace {
const QString WALLPAPER_PATH = QDir::homePath() + "/.local/share/deepin/deepin-album/wallpapers/";
}

WallpaperSetter *WallpaperSetter::m_setter = nullptr;
WallpaperSetter *WallpaperSetter::instance()
{
    if (! m_setter) {
        m_setter = new WallpaperSetter();
    }

    return m_setter;
}

WallpaperSetter::WallpaperSetter(QObject *parent) : QObject(parent)
{

}

bool WallpaperSetter::setBackground(const QString &pictureFilePath)
{
    QImage tImg;
    QString errMsg;
    QFileInfo info(pictureFilePath);
    QString tempWallPaperpath;
    tempWallPaperpath = WALLPAPER_PATH + info.baseName() + ".png";
    QFileInfo tempInfo(tempWallPaperpath);
    if (!UnionImage_NameSpace::loadStaticImageFromFile(pictureFilePath, tImg, errMsg)) {
        return false;
    }
    //临时文件目录不存在，先创建临时文件目录
    QDir tempImgDir(WALLPAPER_PATH);
    if (!tempImgDir.exists() && !tempImgDir.mkdir(tempImgDir.path())) {
        return false;
    }

    if (!tImg.save(tempWallPaperpath, "PNG", 100)) {
        return false;
    }
    if (!tempInfo.exists()) {
        return false;
    }
    QDBusMessage msgIntrospect = QDBusMessage::createMethodCall("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", "org.freedesktop.DBus.Introspectable", "Introspect");
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msgIntrospect);
    call.waitForFinished();

    QDBusReply<QString> reply = call.reply();
    QString value = reply.value();
    if (value.contains("SetMonitorBackground")) {
        QDBusMessage msg = QDBusMessage::createMethodCall("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", "com.deepin.daemon.Appearance", "SetMonitorBackground");
        QString mm;
        if (Application::isWaylandPlatform()) {
            QDBusInterface interface("com.deepin.daemon.Display", "/com/deepin/daemon/Display", "com.deepin.daemon.Display");
            mm = qvariant_cast< QString >(interface.property("Primary"));
        } else {
            mm = qApp->primaryScreen()->name();
        }
        msg.setArguments({mm, tempWallPaperpath});
        QDBusConnection::sessionBus().asyncCall(msg);
        qDebug() << "FileUtils::setBackground call Appearance SetMonitorBackground";
    } else {
        QDBusMessage msg = QDBusMessage::createMethodCall("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", "com.deepin.daemon.Appearance", "Set");
        msg.setArguments({"Background", tempWallPaperpath});
        QDBusConnection::sessionBus().asyncCall(msg);
    }

    // Task 32367: 设置壁纸同步设置锁屏壁纸
    QDBusMessage setGretterMsg = QDBusMessage::createMethodCall("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", "com.deepin.daemon.Appearance", "Set");
    setGretterMsg.setArguments({"greeterbackground", tempWallPaperpath});
    QDBusConnection::sessionBus().asyncCall(setGretterMsg);

    return true;
}
