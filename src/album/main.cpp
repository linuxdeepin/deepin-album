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

#include "imagedataservice.h"

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE


QUrl UrlInfo(QString path)
{
    QUrl url;
    // Just check if the path is an existing file.
    if (QFile::exists(path)) {
        url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
        return url;
    }

    const auto match = QRegularExpression(QStringLiteral(":(\\d+)(?::(\\d+))?:?$")).match(path);

    if (match.isValid()) {
        // cut away line/column specification from the path.
        path.chop(match.capturedLength());
    }

    // make relative paths absolute using the current working directory
    // prefer local file, if in doubt!
    url = QUrl::fromUserInput(path, QDir::currentPath(), QUrl::AssumeLocalFile);

    // in some cases, this will fail, e.g.
    // assume a local file and just convert it to an url.
    if (!url.isValid()) {
        // create absolute file path, we will e.g. pass this over dbus to other processes
        url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
    }
    return url;
}

int main(int argc, char *argv[])
{
    //for qt5platform-plugins load DPlatformIntegration or DPlatformIntegrationParent
    if (!QString(qgetenv("XDG_CURRENT_DESKTOP")).toLower().startsWith("deepin")) {
        setenv("XDG_CURRENT_DESKTOP", "Deepin", 1);
    }

#if (DTK_VERSION < DTK_VERSION_CHECK(5, 4, 0, 0))
    QScopedPointer<DApplication> dAppNew(new DApplication(argc, argv));
#else
    DApplication *dAppNew = DApplication::globalApplication(argc, argv);
#endif

#ifdef ENABLE_ACCESSIBILITY
#endif

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
        filepath = UrlInfo(path).toLocalFile();

        QFileInfo info(filepath);
        QMimeDatabase db;
        QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
        QMimeType mt1 = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchExtension);

        QString str = info.suffix().toLower();
        if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng")
                || mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
            if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
                bneedexit = false;
                break;
            } else if (str.isEmpty()) {
                bneedexit = false;
                break;
            }
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
    QMap<int, int> picSize;
    picSize.insert(0, 80);
    picSize.insert(1, 90);
    picSize.insert(2, 100);
    picSize.insert(3, 110);
    picSize.insert(4, 120);
    picSize.insert(5, 130);
    picSize.insert(6, 140);
    picSize.insert(7, 150);
    picSize.insert(8, 160);
    picSize.insert(9, 170);
    if (num < 0 || num > 9) {
        num = 4;
    }
    int picsize = picSize.value(num);
    int number = ((restoredFrameGeometry.width() - 50) * (restoredFrameGeometry.height() - 50)) / (picsize * picsize);

    DBManager::instance();
    ImageDataService::instance();
    ImageEngineApi::instance(dAppNew);
    // 加载第一屏图片
    qDebug() << "------" << __FUNCTION__ << "" << QThread::currentThreadId();
    ImageEngineApi::instance()->loadFirstPageThumbnails(number);
    auto &w = MainWindow::instance(); //修改为从单例获取

    dApp->setMainWindow(&w);
    w.show();
    Dtk::Widget::moveToCenter(&w);
    w.startMonitor();

#if (DTK_VERSION >= DTK_VERSION_CHECK(5, 5, 0, 0))
    dAppNew->ignoreVirtualKeyboard(&w); //禁止虚拟键盘顶起主界面标题栏（DTK5.5有效）
#endif

    if (bneedexit)
        bfirstopen = false;

    //警告：这个函数只会将&DApplication::handleQuitAction的地址替换为&MainWindow::closeFromMenu
    //     并不会将dAppNew.get()和&w两个实体变量绑定上去
    //     因为C++只会为同一个函数生成一份拷贝，实现这个函数的人明显以为每个实体变量都会拥有单独的函数拷贝
    Dtk::Core::DVtableHook::overrideVfptrFun(dAppNew, &DApplication::handleQuitAction,
                                             &w, &MainWindow::closeFromMenu);

    return dAppNew->exec();
}
