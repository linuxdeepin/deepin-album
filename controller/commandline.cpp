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
#include "commandline.h"
#include "application.h"
#include "controller/signalmanager.h"
#include "controller/wallpapersetter.h"
#include "controller/divdbuscontroller.h"
#include "controller/configsetter.h"
#include "service/deepinimageviewerdbus.h"
#include "frame/viewmainwindow.h"
#include "utils/imageutils.h"
#include "utils/baseutils.h"

#include "dthememanager.h"

#include <QCommandLineOption>
#include <QDBusConnection>
#include <QDesktopWidget>
#include <QDebug>
#include <QFileInfo>

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
{
    m_cmdParser.addHelpOption();
//    m_cmdParser.addVersionOption();
//    m_cmdParser.addPositionalArgument("value", QCoreApplication::translate(
//        "main", "Value that use for options."), "[value]");
    for (const CMOption *i = options; ! i->shortOption.isEmpty(); ++i) {
        addOption(i);
    }

//    m_pwidget = new QWidget(this);
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

void CommandLine::viewImage(const QString &path, const QStringList &paths)
{
//    ViewMainWindow *w = new ViewMainWindow(false);
    QHBoxLayout *m_layout = new QHBoxLayout;
    m_layout->setContentsMargins(0, 0, 0, 0);
    MainWidget *m_mainWidget = new MainWidget(false);
    m_layout->addWidget(m_mainWidget);
    setLayout(m_layout);
//    w->setWindowRadius(18);
//    w->setBorderWidth(0);
//    w->show();

    // Load image after all UI elements has been init
    // BottomToolbar pos not correct on init
//    emit dApp->signalM->hideBottomToolbar(true);
    emit dApp->signalM->enableMainMenu(false);

    QTimer::singleShot(300, this, [ = ] {

        if (paths.count() > 0)
        {
            SignalManager::ViewInfo info;
            info.album = "";
#ifndef LITE_DIV
            info.inDatabase = false;
#endif
            info.lastPanel = nullptr;
            info.path = path;
            info.paths = paths;

            emit dApp->signalM->viewImage(info);
            emit dApp->signalM->showImageView(0);

            DBImgInfoList dbInfos;
            using namespace utils::image;
            for (auto path : paths) {
                qDebug() << path;
                if (! imageSupportRead(path)) {
                    continue;
                }

                QFileInfo fi(path);
                DBImgInfo dbi;
                dbi.fileName = fi.fileName();
                dbi.filePath = path;
                dbi.dirHash = utils::base::hash(QString());
                if (fi.birthTime().isValid()) {
                    dbi.time = fi.birthTime();
                } else if (fi.metadataChangeTime().isValid()) {
                    dbi.time = fi.metadataChangeTime();
                } else {
                    dbi.time = QDateTime::currentDateTime();
                }
                dbi.changeTime = QDateTime::currentDateTime();

                qDebug() << path;
                dbInfos << dbi;
            }

            if (! dbInfos.isEmpty()) {
                qDebug() << "DBManager::instance()->insertImgInfos(dbInfos)";
                DBManager::instance()->insertImgInfos(dbInfos);
            }
        }

        dApp->LoadDbImage();
    });
}

bool CommandLine::processOption(QStringList &paslist)
{

    if (! m_cmdParser.parse(dApp->arguments())) {
        showHelp();
        return false;
    }

    QString defaulttheme = dApp->setter->value(THEME_GROUP,
                                               THEME_TEXT).toString();

    if (DGuiApplicationHelper::LightType == DGuiApplicationHelper::instance()->themeType() ) {
        dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Light);
    } else {
        dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Dark);
    }

    QStringList names = m_cmdParser.optionNames();
    QStringList pas = m_cmdParser.positionalArguments();
    qDebug() << "processOption()" << names << pas;

    QImage *pimg = new QImage();

    if (pas.count() > 0) {
        for (int i = 0; i < pas.count(); i++) {
            if (QFileInfo(pas.at(i)).isDir()) {
                continue;
//                if(!pas.at(i).isEmpty())
//                {
//                    //checkFileType(QDir(pas.at(i)).entryList(), paslist);

//                }
//                else
//                {
//                    continue;
//                }
            } else if (pimg->load(pas.at(i))) {
                paslist.append(pas.at(i));
            } else {
                continue;
            }
        }
        if (paslist.isEmpty()) {
            exit(0);
        }
    }

    delete pimg;
    //paslist = pas;
//    if (pas.length() > 0) {
//        viewImage(QFileInfo(pas.at(0)).absoluteFilePath(), pas);
//    }
//    else {
//        viewImage("", {});
//    }





//    using namespace utils::image;
//    QString name;
//    QString value;
//    QStringList values;
//    if (! names.isEmpty()) {
//        name = names.first();
//        value = m_cmdParser.value(name);
//        values = m_cmdParser.values(name);
//    }

//    if (values.isEmpty() && ! pas.isEmpty()){
//        name = "o"; // Default operation is open image file
//        value = pas.first();

//        if (QUrl(value).isLocalFile()) {
//            value =  QUrl(value).toLocalFile();
//        }
//        values = pas;
//    }

//    bool support = imageSupportRead(value);

//    if (name == "o" || name == "open") {
//        if (values.length() > 1) {
//            QStringList aps;
//            for (QString path : values) {
//                if (QUrl(value).isLocalFile())
//                    path =  QUrl(value).toLocalFile();
//                const QString ap = QFileInfo(path).absoluteFilePath();
//                if (QFileInfo(path).exists() && imageSupportRead(ap)) {
//                    aps << ap;
//                }
//            }
//            if (! aps.isEmpty()) {
//                viewImage(aps.first(), aps);
//                return true;
//            }
//            else {
//                return false;
//            }
//        }
//        else if (support) {
//            viewImage(QFileInfo(value).absoluteFilePath(), QStringList());
//            return true;
//        }
//        else {
//            return false;
//        }
//    }

//    else if ((name == "w" || name == "wallpaper") && support) {
//        qDebug() << "Set " << value << " as wallpaper.";
//        dApp->wpSetter->setWallpaper(QFileInfo(value).absoluteFilePath());
//    }
//    else if (name.isEmpty() || name == "new-window") {
//        viewImage("", {});
//        return true;
//    } else {
//        showHelp();
//    }

    return false;
}

//void CommandLine::checkFileType(QStringList pas, QStringList &paslist)
//{
//    QImage* pimg = new QImage();

//    for(int i = 2; i < pas.count(); i++)
//    {
//        if(QFileInfo(pas.at(i)).isDir())
//        {
//            break;
//        }
//        else if(pimg->load(pas.at(i)))
//        {
//            paslist.append(pas.at(i));
//        }
//        else
//        {
//            continue;
//        }
//    }
//}

void CommandLine::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
//    m_spinner->move(width()/2 - 20, (height()-50)/2 - 20);
//    m_pwidget->setFixedWidth(this->width() / 2);
//    m_pwidget->setFixedHeight(54);
//    m_pwidget->move(this->width() / 4, this->height() - 81);
}
