// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
