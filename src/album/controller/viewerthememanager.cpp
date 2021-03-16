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
#include "viewerthememanager.h"
#include "application.h"
#include "configsetter.h"

#include <DApplicationHelper>

namespace {
const QString THEME_GROUP = "APP";
const QString THEME_TEXT = "AppTheme";
}

ViewerThemeManager *ViewerThemeManager::m_viewerTheme = nullptr;
ViewerThemeManager *ViewerThemeManager::instance()
{
    if (m_viewerTheme == nullptr) {
        m_viewerTheme = new ViewerThemeManager;
    }

    return m_viewerTheme;
}

ViewerThemeManager::ViewerThemeManager(QObject *parent) : QObject(parent)
    , m_currentTheme(AppTheme::Light)
{
}

ViewerThemeManager::AppTheme ViewerThemeManager::getCurrentTheme()
{
    return m_currentTheme;
}

void ViewerThemeManager::setCurrentTheme(AppTheme theme)
{
    m_currentTheme = theme;
    DGuiApplicationHelper::ColorType colorType;
    if (m_currentTheme == Dark) {
        dApp->setter->setValue(THEME_GROUP, THEME_TEXT, QVariant("Dark"));
        colorType = DGuiApplicationHelper::ColorType::DarkType;
    } else {
        dApp->setter->setValue(THEME_GROUP, THEME_TEXT, QVariant("Light"));
        colorType = DGuiApplicationHelper::ColorType::LightType;
    }
    emit viewerThemeChanged(m_currentTheme);
    emit DGuiApplicationHelper::instance()->themeTypeChanged(colorType);
}
