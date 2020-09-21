#include "wallpapersetter.h"
#include "application.h"
#include <QTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QImage>
#include <QDebug>
#include <QDBusInterface>
#include <QScreen>

#include <unistd.h>

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

void WallpaperSetter::setWallpaper(const QString &path)
{
    qDebug() << "SettingWallpaper 2";
    QDBusInterface interface("com.deepin.daemon.Appearance",
                                 "/com/deepin/daemon/Appearance",
                                 "com.deepin.daemon.Appearance");
    if (interface.isValid()) {
        QString mm;
        if (Application::isWaylandPlatform()) {
            QDBusInterface interface("com.deepin.daemon.Display", "/com/deepin/daemon/Display", "com.deepin.daemon.Display");
            mm = qvariant_cast< QString >(interface.property("Primary"));
        } else {
            mm = dApp->primaryScreen()->name();
        }
        QDBusMessage reply = interface.call(QStringLiteral("SetMonitorBackground"), mm, path);
        qDebug() << "SettingWallpaper: replay" << reply.errorMessage();
    } else {
        qWarning() << "SettingWallpaper failed" << interface.lastError();
    }
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
