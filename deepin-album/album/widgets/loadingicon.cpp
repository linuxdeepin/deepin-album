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
#include "loadingicon.h"
#include "application.h"
#include "controller/viewerthememanager.h"

LoadingIcon::LoadingIcon()
{
    setStyleSheet("background:red");
    updateIconPath();
    setPictureSequence(m_iconPaths, true);
    setFixedSize(40, 40);
    setSpeed(40);
    connect(dApp->viewerTheme, &ViewerThemeManager::viewerThemeChanged,
            this, &LoadingIcon::updateIconPath);
}


void LoadingIcon::updateIconPath()
{
    QString iconPath;
//    if (dApp->viewerTheme->getCurrentTheme() == ViewerThemeManager::Dark) {
        iconPath = ":/images/logo/resources/images/other/icon_toast_sucess.svg";
//    }
//    else {
//        iconPath = ":/images/loadings/resources/light/images/white_loading/loading_%1.png";
//    }
    m_iconPaths.clear();

    for (int i = 1; i < 45; ++i)
    {
        QPixmap aaaa(iconPath.arg(i));
        m_iconPaths<<aaaa;
    }
}
