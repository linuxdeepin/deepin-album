#include "wallpapersetter.h"
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

//void WallpaperSetter::setWallpaper(const QString &path)
//{
//    // gsettings unsupported unicode character
//    QString tmpImg = QString("/tmp/DIVIMG.%1").arg(QFileInfo(path).suffix());
//    QFile(path).copy(tmpImg);
//    QImage img(tmpImg);
//    if (img.format() != QImage::Format_Invalid) {
//        if (img.save(QString("/tmp/DIVIMG.%1").arg("JPG"))) {
//            //if convert image succeed,remove copy image.
//            QFile(tmpImg).remove();
//            //change value of tmpImg
//            tmpImg = QString("/tmp/DIVIMG.%1").arg("JPG");
//        }
//    }

//    if (!qEnvironmentVariableIsEmpty("FLATPAK_APPID")) {
//        // gdbus call -e -d com.deepin.daemon.Appearance -o /com/deepin/daemon/Appearance -m com.deepin.daemon.Appearance.Set background /home/test/test.png
//        //   qDebug() << "SettingWallpaper: " << "flatpak" << path;
//        QDBusInterface interface("com.deepin.daemon.Appearance",
//                                     "/com/deepin/daemon/Appearance",
//                                     "com.deepin.daemon.Appearance");
//        if (interface.isValid()) {
//            QDBusMessage reply = interface.call("Set", "background", path);
//            //   qDebug() << "SettingWallpaper: replay" << reply.errorMessage();
//        } else {
//            //    qWarning() << "SettingWallpaper failed" << interface.lastError();
//        }
//    } else {
//        DE de = getDE();
//        //   qDebug() << "SettingWallpaper: " << de << path;
//        switch (de) {
//        case Deepin:
//            setDeepinWallpaper(tmpImg);
//            break;
//        case KDE:
//            setKDEWallpaper(tmpImg);
//            break;
//        case GNOME:
//            setGNOMEWallpaper(tmpImg);
//            break;
//        case LXDE:
//            setLXDEWallpaper(tmpImg);
//            break;
//        case Xfce:
//            setXfaceWallpaper(tmpImg);
//            break;
//        default:
//            setGNOMEShellWallpaper(tmpImg);
//        }
//    }

//    // Remove the tmp file
//    QTimer *t = new QTimer(this);
//    t->setSingleShot(true);
//    connect(t, &QTimer::timeout, this, [t, tmpImg] {
//        QFile(tmpImg).remove();
//        t->deleteLater();
//    });
//    t->start(1000);
//}

bool WallpaperSetter::setBackground(const QString &pictureFilePath)
{
    QDBusMessage msgIntrospect = QDBusMessage::createMethodCall("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", "org.freedesktop.DBus.Introspectable", "Introspect");
    QDBusPendingCall call = QDBusConnection::sessionBus().asyncCall(msgIntrospect);
    call.waitForFinished();
    if (call.isFinished()) {
        QDBusReply<QString> reply = call.reply();
        QString value = reply.value();

        if (value.contains("SetMonitorBackground")) {
            QDBusMessage msg = QDBusMessage::createMethodCall("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", "com.deepin.daemon.Appearance", "SetMonitorBackground");
            msg.setArguments({qApp->primaryScreen()->name(), pictureFilePath});
            QDBusConnection::sessionBus().asyncCall(msg);

            qDebug() << "FileUtils::setBackground call Appearance SetMonitorBackground";
            return true;
        }
    }

    QDBusMessage msg = QDBusMessage::createMethodCall("com.deepin.daemon.Appearance", "/com/deepin/daemon/Appearance", "com.deepin.daemon.Appearance", "Set");
    msg.setArguments({"Background", pictureFilePath});
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
