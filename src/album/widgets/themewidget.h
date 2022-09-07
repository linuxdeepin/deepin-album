// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef THEMEWIDGET_H
#define THEMEWIDGET_H

#include "controller/viewerthememanager.h"

#include <QWidget>
#include <QScrollArea>
#include <DScrollArea>
#include <QFile>

class ThemeWidget : public QWidget
{
    Q_OBJECT
public:
    ThemeWidget(const QString &darkFile, const QString &lightFile,
                QWidget *parent = nullptr);
    ~ThemeWidget();

public slots:
    void onThemeChanged(ViewerThemeManager::AppTheme theme);
//    bool isDeepMode();
private:
    QString m_darkStyle;
    QString m_lightStyle;
    bool m_deepMode;
};

//TODO: if a widget Multiple Inheritance from ThemeWidget and
//      QScrollArea. warning(QWidget is an ambiguous base).
//class ThemeScrollArea : public QScrollArea
//{
//    Q_OBJECT
//public:
//    ThemeScrollArea(const QString &darkFile, const QString &lightFile,
//                    QWidget *parent = nullptr);
//    ~ThemeScrollArea();

//public slots:
//    void onThemeChanged(ViewerThemeManager::AppTheme theme);
//    bool isDeepMode();
//private:
//    QString m_darkStyle;
//    QString m_lightStyle;
//    bool m_deepMode;
//};
#endif // THEMEWIDGET_H
