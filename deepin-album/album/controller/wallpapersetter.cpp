#include "wallpapersetter.h"
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
    tempWallPaperpath = WALLPAPER_PATH + info.fileName();
    QFileInfo tempInfo(tempWallPaperpath);
    if (!UnionImage_NameSpace::loadStaticImageFromFile(pictureFilePath, tImg, errMsg)) {
        qDebug() << "load wallpaper path error!";
        return false;
    }
    //临时文件目录不存在，先创建临时文件目录
    QDir tempImgDir(WALLPAPER_PATH);
    if (!tempImgDir.exists() && tempImgDir.mkdir(tempImgDir.path())) {
        qDebug() << "save temp wallpaper path error!";
        return false;
    }
    if (!tImg.save(tempWallPaperpath, "JPG", 100)) {
        qDebug() << "save temp wallpaper path error!";
        return false;
    }
    if (!tempInfo.exists()) {
        qDebug() << "save temp wallpaper path error!";
        return false;
    }
    QDBusMessage msgIntrospect = QDBusMessage::createMethodCall("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", "org.freedesktop.DBus.Introspectable", "Introspect");
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msgIntrospect);
    call.waitForFinished();
    if (call.isFinished()) {
        QDBusReply<QString> reply = call.reply();
        QString value = reply.value();
        if (value.contains("SetMonitorBackground")) {
            QDBusMessage msg = QDBusMessage::createMethodCall("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", "com.deepin.daemon.Appearance", "SetMonitorBackground");
            msg.setArguments({qApp->primaryScreen()->name(), tempWallPaperpath});
            QDBusConnection::sessionBus().asyncCall(msg);
            qDebug() << "FileUtils::setBackground call Appearance SetMonitorBackground";
            return true;
        }
    }
    QDBusMessage msg = QDBusMessage::createMethodCall("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", "com.deepin.daemon.Appearance", "Set");
    msg.setArguments({"Background", tempWallPaperpath});
    QDBusConnection::sessionBus().asyncCall(msg);
    qDebug() << "FileUtils::setBackground call Appearance Set";
    return true;
}

void WallpaperSetter::setDeepinWallpaper(const QString &path)
{
    QString comm = QString("gsettings set "
                           "com.deepin.wrap.gnome.desktop.background picture-uri %1").arg(path);
    QProcess::execute(comm);
}

void WallpaperSetter::setGNOMEWallpaper(const QString &path)
{
    QString comm = QString("gconftool-2 --type=string --set "
                           "/desktop/gnome/background/picture_filename %1").arg(path);
    QProcess::execute(comm);
}

void WallpaperSetter::setGNOMEShellWallpaper(const QString &path)
{
    QString comm = QString("gsettings set "
                           "org.gnome.desktop.background picture-uri %1").arg(path);
    QProcess::execute(comm);
}

void WallpaperSetter::setKDEWallpaper(const QString &path)
{
    QString comm = QString("dbus-send --session "
                           "--dest=org.new_wallpaper.Plasmoid --type=method_call "
                           "/org/new_wallpaper/Plasmoid/0 org.new_wallpaper.Plasmoid.SetWallpaper "
                           "string:%1").arg(path);
    QProcess::execute(comm);
}

void WallpaperSetter::setLXDEWallpaper(const QString &path)
{
    QString comm = QString("pcmanfm --set-wallpaper %1").arg(path);
    QProcess::execute(comm);
}

void WallpaperSetter::setXfaceWallpaper(const QString &path)
{
    QString comm = QString("xfconf-query -c xfce4-desktop -p "
                           "/backdrop/screen0/monitor0/image-path -s %1").arg(path);
    QProcess::execute(comm);
}

WallpaperSetter::DE WallpaperSetter::getDE()
{
    if (testDE("startdde")) {
        return Deepin;
    } else if (testDE("plasma-desktop")) {
        return KDE;
    } else if (testDE("gnome-panel") && qgetenv("DESKTOP_SESSION") == "gnome") {
        return GNOME;
    } else if (testDE("xfce4-panel")) {
        return Xfce;
    } else if (testDE("mate-panel")) {
        return MATE;
    } else if (testDE("lxpanel")) {
        return LXDE;
    } else {
        return OthersDE;
    }
}

bool WallpaperSetter::testDE(const QString &app)
{
    bool v = false;
    QProcess p;
    p.start(QString("ps -A"));
    while (p.waitForReadyRead(3000)) {
        v = QString(p.readAllStandardOutput()).contains(app);
        p.kill();
    }

    return v;
}
