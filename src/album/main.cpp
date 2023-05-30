// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <DVtableHook>
#define protected public
#include <DApplication>
#undef protected

#include "application.h"
#include "mainwindow.h"
#include "imageengine/imageengineapi.h"
#include "accessibledefine.h"
#include "accessible.h"

#include <DMainWindow>
#include <DWidgetUtil>
#include <DApplicationSettings>
#include <DLog>
#include <DStandardPaths>

#include <QMessageBox>
#include <QStandardPaths>
#include <QSettings>
#include <QThread>
#include "eventlogutils.h"
#include "imagedataservice.h"
#include "baseutils.h"
#include "config.h"

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

int main(int argc, char *argv[])
{
    //非wayland平台需要添加xcb
    if (Application::isWaylandPlatform()) {
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "kwayland-shell"); //add wayland parameter
    }

    //for qt5platform-plugins load DPlatformIntegration or DPlatformIntegrationParent
    if (!QString(qgetenv("XDG_CURRENT_DESKTOP")).toLower().startsWith("deepin")) {
        setenv("XDG_CURRENT_DESKTOP", "Deepin", 1);
    }

    /*#if (DTK_VERSION < DTK_VERSION_CHECK(5, 4, 0, 0))
        DApplication *dAppNew = new DApplication(argc, argv);
    #else
        DApplication *dAppNew = DApplication::globalApplication(argc, argv);
    #endif*/

    CustomDApplication *dAppNew = new CustomDApplication(argc, argv);

#ifdef ENABLE_ACCESSIBILITY
#endif
    setlocale(LC_NUMERIC, "C");
    dAppNew->setAttribute(Qt::AA_UseHighDpiPixmaps);
    QAccessible::installFactory(accessibleFactory);
    dAppNew->setOrganizationName("deepin");
    dAppNew->setApplicationName("deepin-album");
    dAppNew->loadTranslator(QList<QLocale>() << QLocale::system());

    Application::getApp()->setApp(dAppNew);

    qputenv("DTK_USE_SEMAPHORE_SINGLEINSTANCE", "1");

    QCommandLineParser parser;
    parser.process(*dAppNew);

    QStringList urls;
    QStringList arguments = parser.positionalArguments();
    QString filepath = "";
    bool bneedexit = true;
    for (const QString &path : arguments) {
        filepath = utils::base::UrlInfo(path).toLocalFile();

        QFileInfo info(filepath);
        QString str = info.suffix().toLower();
        qDebug() << __FUNCTION__ << "---" << str;
        if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive) ||
                utils::base::isVideo(filepath)) { //增加对视频格式的支持
            bneedexit = false;
            break;
        } else if (str.isEmpty()) {
            bneedexit = false;
            break;
        }
    }

    if ("" != filepath && bneedexit) {
        exit(0);
    }

    if (!bneedexit) {
        if (bfirstopen) {
            bfirstandviewimage = true;
        }
    }
    //save theme
    DApplicationSettings savetheme;

    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();
    if (!DGuiApplicationHelper::instance()->setSingleInstance(dAppNew->applicationName(), DGuiApplicationHelper::UserScope)) {
        exit(0);
    }

    // LMH0420判断是否相同进程启动
    if (dApp->isRunning()) {
        return 0;
    }

    QString userConfigPath = DStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                             + "/config.conf";
    QSettings *settings = new QSettings(userConfigPath, QSettings::IniFormat);
    const QByteArray geometry = settings->value("album-geometry").toByteArray();
    int num = settings->value("album-zoomratio").toInt();
    settings->deleteLater();
    settings = nullptr;
    QRect restoredFrameGeometry;
    if (!geometry.isEmpty()) {
        QDataStream stream(geometry);
        stream.setVersion(QDataStream::Qt_4_0);

        quint32 storedMagicNumber;
        stream >> storedMagicNumber;

        quint16 majorVersion = 0;
        quint16 minorVersion = 0;

        stream >> majorVersion >> minorVersion;
        QRect restoredNormalGeometry;
        qint32 restoredScreenNumber;
        quint8 maximized;
        quint8 fullScreen;

        stream >> restoredFrameGeometry
               >> restoredNormalGeometry
               >> restoredScreenNumber
               >> maximized
               >> fullScreen;
    }
    //计算当前一屏照片数量
    if (num < 0 || num > 9) {
        num = 4;
    }
    int picsize = 80 + num * 10;
    int number = ((restoredFrameGeometry.width() - 50) * (restoredFrameGeometry.height() - 50)) / (picsize * picsize);

    DBManager::instance();
    ImageDataService::instance();
    ImageEngineApi::instance(dAppNew);
    // 加载第一屏图片
    qDebug() << "------" << __FUNCTION__ << "" << QThread::currentThreadId();
    ImageEngineApi::instance()->loadFirstPageThumbnails(number);

    //埋点记录启动数据
    QJsonObject objStartEvent{
        {"tid", Eventlogutils::StartUp},
        {"vsersion", VERSION},
        {"mode", 1},
    };
    Eventlogutils::GetInstance()->writeLogs(objStartEvent);
    MainWindow w;
    bool titleVisible = w.titlebar()->isVisible();
    dApp->setMainWindow(&w);
    w.show();
    dAppNew->setMainWindow(&w);
    Dtk::Widget::moveToCenter(&w);

    // show()会重置标题栏显示状态
    w.titlebar()->setVisible(titleVisible);

//#if (DTK_VERSION >= DTK_VERSION_CHECK(5, 5, 0, 0))
//    dAppNew->ignoreVirtualKeyboard(&w); //禁止虚拟键盘顶起主界面标题栏（DTK5.5有效）
//#endif

    if (bneedexit)
        bfirstopen = false;

    return dAppNew->exec();
}
