// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VIEWERTHEMEMANAGER_H
#define VIEWERTHEMEMANAGER_H

#include <QObject>

class ViewerThemeManager : public QObject
{
    Q_OBJECT
    explicit ViewerThemeManager(QObject *parent = nullptr);
public:
    enum AppTheme {
        Dark,
        Light,
    };

    static ViewerThemeManager *instance();
signals:
    void viewerThemeChanged(AppTheme theme);
public slots:
    AppTheme getCurrentTheme();
    void setCurrentTheme(AppTheme theme);

private:
    static ViewerThemeManager *m_viewerTheme;
    AppTheme m_currentTheme;
};
#endif // VIEWERTHEMEMANAGER_H
