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
#include "commandline.h"
#include <QHBoxLayout>
#include <QCommandLineOption>
#include <QDBusConnection>
#include <QDesktopWidget>
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <QMimeDatabase>

#include "application.h"
#include "controller/signalmanager.h"
#include "controller/wallpapersetter.h"
#include "controller/configsetter.h"
#include "frame/mainwidget.h"
#include "utils/imageutils.h"
#include "utils/baseutils.h"
#include "utils/unionimage.h"
#include "dthememanager.h"
#include "ac-desktop-define.h"
#include "imageviewer.h"

using namespace Dtk::Widget;

namespace {

const QString DBUS_PATH = "/com/deepin/ImageViewer";
const QString DBUS_NAME = "com.deepin.ImageViewer";
const QString THEME_GROUP = "APP";
const QString THEME_TEXT = "AppTheme";
}

struct CMOption {
    QString shortOption;
    QString longOption;
    QString description;
    QString valueName;
};

static CMOption options[] = {
    {"o", "open", "Open the specified <image-file>.", "image-file"},
    {"a", "album", "Enter the album <album-name>.", "album-name"},
//    {"s", "search", "Go to search view and search image by <word>.", "word"},
//    {"e", "edit", "Go to edit view and begin editing <image-file>.", "image-file"},
    {"w", "wallpaper", "Set <image-file> as wallpaper.", "image-file"},
    {"new-window", "new-window", "Display a window.", ""},
    {"", "", "", ""}
};

CommandLine *CommandLine::m_commandLine = nullptr;
CommandLine *CommandLine::instance()
{
    if (! m_commandLine) {
        m_commandLine = new CommandLine();
    }

    return m_commandLine;
}

CommandLine::CommandLine()
    : m_pwidget(nullptr)
{
    m_cmdParser.addHelpOption();
//    m_cmdParser.addVersionOption();
//    m_cmdParser.addPositionalArgument("value", QCoreApplication::translate(
//        "main", "Value that use for options."), "[value]");
    for (const CMOption *i = options; ! i->shortOption.isEmpty(); ++i) {
        addOption(i);
    }

    m_pwidget = new QWidget(this);
    m_pwidget->setAttribute(Qt::WA_TransparentForMouseEvents);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
//    m_mainWidget = new MainWidget(false, this);
    m_mainWidget = new ImageViewer(imageViewerSpace::ImgViewerType::ImgViewerTypeLocal, albumGlobal::CACHE_PATH, nullptr, m_pwidget);
    m_mainWidget->setObjectName("MainWidget");
    m_layout->addWidget(m_mainWidget);
    setLayout(m_layout);
}

CommandLine::~CommandLine()
{

}

void CommandLine::addOption(const CMOption *option)
{
    QStringList ol;
    ol << option->shortOption;
    ol << option->longOption;
    QCommandLineOption cm(ol, option->description, option->valueName);

    m_cmdParser.addOption(cm);
}

/*!
 * \brief CommandLine::showHelp
 * QCommandLineParser::showHelp(int exitCode = 0) Will displays the help
 * information, and exits application automatically. However,
 * DApplication::loadDXcbPlugin() need exit by calling quick_exit(a.exec()).
 * So we should show the help message only by calling this function.
 */
void CommandLine::showHelp()
{
    fputs(qPrintable(m_cmdParser.helpText()), stdout);
}

//设置管理线程的对象
void CommandLine::setThreads(ImageEngineImportObject *obj)
{
    m_obj = obj;
}

void CommandLine::viewImage(const QString &path, const QStringList &paths)
{
    emit dApp->signalM->enableMainMenu(false);
    if (paths.count() == 1) {
        using namespace UnionImage_NameSpace;
        QImage tImg;
        bool cache_exist = false;
        QFileInfo file(albumGlobal::CACHE_PATH + path);
        QString errMsg;
        QString dimension;
        QString tpath;
        bool breloadCache = false;
        QFileInfo srcfi(path);
        if (file.exists()) {
            QDateTime cachetime = file.metadataChangeTime();    //缓存修改时间
            QDateTime srctime = srcfi.metadataChangeTime();     //源数据修改时间
            if (srctime.toTime_t() > cachetime.toTime_t()) {  //源文件近期修改过，重新生成缓存文件
                cache_exist = false;
                breloadCache = true;
                tpath = path;
                if (!loadStaticImageFromFile(tpath, tImg, errMsg)) {
                    qDebug() << errMsg;
                }
                dimension = QString::number(tImg.width()) + "x" + QString::number(tImg.height());
            } else {
                cache_exist = true;
                tpath = albumGlobal::CACHE_PATH + path;
                if (!loadStaticImageFromFile(tpath, tImg, errMsg, "PNG")) {
                    qDebug() << errMsg;
                }
            }
        } else {
            if (!loadStaticImageFromFile(path, tImg, errMsg)) {
                qDebug() << errMsg;
            }
            dimension = QString::number(tImg.width()) + "x" + QString::number(tImg.height());
        }
        if (0 != tImg.height() && 0 != tImg.width() && (tImg.height() / tImg.width()) < 10 && (tImg.width() / tImg.height()) < 10) {
            if (tImg.height() != 200 && tImg.width() != 200) {
                if (tImg.height() >= tImg.width()) {
                    cache_exist = true;
                    tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
                } else if (tImg.height() <= tImg.width()) {
                    cache_exist = true;
                    tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
                }
            }
            if (!cache_exist) {
                if ((static_cast<float>(tImg.height()) / (static_cast<float>(tImg.width()))) > 3) {
                    tImg = tImg.scaledToWidth(200,  Qt::FastTransformation);
                } else {
                    tImg = tImg.scaledToHeight(200,  Qt::FastTransformation);
                }
            }
        }
        QPixmap pixmap = QPixmap::fromImage(tImg);
        DBImgInfo dbi = getDBInfo(path, utils::base::isVideo(path));
        if (breloadCache) { //更新缓存文件
            QString spath = albumGlobal::CACHE_PATH + path;
            utils::base::mkMutiDir(spath.mid(0, spath.lastIndexOf('/')));
            pixmap.save(spath, "PNG");
        }
        ImageEngineApi::instance()->addImageData(path, dbi);
        SignalManager::ViewInfo info;
        info.album = "";
//        info.lastPanel = nullptr; //todo imageviewer
        info.path = path;
        info.paths = paths;
        info.dBImgInfos << dbi;
        // 未启动相册，从外部打开图片时，延迟发送查看图片
        if (dApp->getMainWindow() == nullptr) {
            QTimer::singleShot(100, this, [ = ] {
                emit dApp->signalM->viewImage(info);
                emit dApp->signalM->showImageView(0);
                DBManager::instance()->insertImgInfos(DBImgInfoList() << dbi);
            });
        } else {
            emit dApp->signalM->viewImage(info);
            emit dApp->signalM->showImageView(0);
            DBManager::instance()->insertImgInfos(DBImgInfoList() << dbi);
        }
    } else {
        QTimer::singleShot(300, this, [ = ] {
            if (paths.count() > 0)
            {
                SignalManager::ViewInfo info;
                info.album = "";
//                info.lastPanel = nullptr;  //todo imageviewer
                info.path = path;
                info.paths = paths;
                emit dApp->signalM->viewImage(info);
                emit dApp->signalM->showImageView(0);
                ImageEngineApi::instance()->loadImagesFromNewAPP(paths, m_obj);
            }
        });
    }
}

bool CommandLine::processOption(QStringList &paslist)
{
    if (! m_cmdParser.parse(dApp->getDAppNew()->arguments())) {
        showHelp();
        return false;
    }

    QString defaulttheme = dApp->setter->value(THEME_GROUP,
                                               THEME_TEXT).toString();

    if (DGuiApplicationHelper::LightType == DGuiApplicationHelper::instance()->themeType()) {
        dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Light);
    } else {
        dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Dark);
    }

    QStringList arguments = m_cmdParser.positionalArguments();
    QString filepath = "";
    bool bneedexit = true;
    for (const QString &path : arguments) {
        filepath = utils::base::UrlInfo(path).toLocalFile();
        QFileInfo info(filepath);
        QMimeDatabase db;
        QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
        QMimeType mt1 = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchExtension);

        QString str = info.suffix().toLower();
        if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)
                || utils::base::isVideo(filepath)) {
            bneedexit = false;
            paslist << info.filePath();
            ImageEngineApi::instance()->insertImage(info.filePath(), "", true);
        } else if (str.isEmpty()) {
            bneedexit = false;
            paslist << info.filePath();
            ImageEngineApi::instance()->insertImage(info.filePath(), "");
        }
    }
    if ("" != filepath && bneedexit) {
        exit(0);
    }
    return false;
}

void CommandLine::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    m_pwidget->setFixedHeight(this->height() - 23);
    m_pwidget->setFixedWidth(this->width());
    m_pwidget->move(0, 0);
}
