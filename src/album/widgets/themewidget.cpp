// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "themewidget.h"
#include "utils/baseutils.h"

#include "application.h"

ThemeWidget::ThemeWidget(const QString &darkFile, const QString &lightFile, QWidget *parent)
    : QWidget(parent)
    , m_darkStyle(utils::base::getFileContent(darkFile))
    , m_lightStyle(utils::base::getFileContent(lightFile))
    , m_deepMode(false)
{

//    m_darkStyle = utils::base::getFileContent(darkFile);
//    m_lightStyle = utils::base::getFileContent(lightFile);
    onThemeChanged(dApp->viewerTheme->getCurrentTheme());

    connect(dApp->viewerTheme, &ViewerThemeManager::viewerThemeChanged, this,
            &ThemeWidget::onThemeChanged);
}

//bool ThemeWidget::isDeepMode()
//{
//    return m_deepMode;
//}

ThemeWidget::~ThemeWidget() {}

void ThemeWidget::onThemeChanged(ViewerThemeManager::AppTheme theme)
{
    if (theme == ViewerThemeManager::Dark) {
        m_deepMode = true;
    } else {
        m_deepMode = false;
    }
}

//ThemeScrollArea::ThemeScrollArea(const QString &darkFile, const QString &lightFile, QWidget *parent)
//    : QScrollArea(parent)
//    , m_darkStyle(utils::base::getFileContent(darkFile))
//    , m_lightStyle(utils::base::getFileContent(lightFile))
//    , m_deepMode(false)
//{
////    m_darkStyle = utils::base::getFileContent(darkFile);
////    m_lightStyle = utils::base::getFileContent(lightFile);
//    onThemeChanged(dApp->viewerTheme->getCurrentTheme());

//    connect(dApp->viewerTheme, &ViewerThemeManager::viewerThemeChanged, this,
//            &ThemeScrollArea::onThemeChanged);
//}

//ThemeScrollArea::~ThemeScrollArea() {}

//void ThemeScrollArea::onThemeChanged(ViewerThemeManager::AppTheme theme)
//{
//    if (theme == ViewerThemeManager::Dark) {
//        m_deepMode = true;
//    } else {
//        m_deepMode = false;
//    }
//}

//bool ThemeScrollArea::isDeepMode()
//{
//    return m_deepMode;
//}
