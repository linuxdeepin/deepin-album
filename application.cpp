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
#include "application.h"

#include "controller/configsetter.h"
#include "controller/globaleventfilter.h"
#include "controller/signalmanager.h"
#include "controller/viewerthememanager.h"
#include "controller/wallpapersetter.h"

#include <QDebug>
#include <QTranslator>
#include <QIcon>

namespace {

}  // namespace

Application::Application(int& argc, char** argv)
    : DApplication(argc, argv)
{
    initI18n();
    setApplicationDisplayName(tr("Deepin album"));
    setProductIcon(QIcon(":/resources/images/other/deepin-album.svg"));
    setApplicationVersion(DApplication::buildVersion("20190925"));
    setApplicationDescription(QString("%1\n%2\n").arg(tr("深度相册是深度操作系统自带的相册软件。")).arg(tr("满足对照片的常用功能，快速、轻巧、使用简单。")));

    installEventFilter(new GlobalEventFilter());

    initChildren();
}

void Application::initChildren()
{
    viewerTheme = ViewerThemeManager::instance();
    setter = ConfigSetter::instance();
    signalM = SignalManager::instance();
    wpSetter = WallpaperSetter::instance();
}

void Application::initI18n()
{
    // install translators
//    QTranslator *translator = new QTranslator;
//    translator->load(APPSHAREDIR"/translations/deepin-image-viewer_"
//                     + QLocale::system().name() + ".qm");
//    installTranslator(translator);
    loadTranslator(QList<QLocale>() << QLocale::system());

}
