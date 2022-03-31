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

#include <unistd.h>

namespace {
const QString WALLPAPER_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + "deepin" + QDir::separator() + "deepin-album" + QDir::separator() + "wallpapers" + QDir::separator();
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

    return true;
}
