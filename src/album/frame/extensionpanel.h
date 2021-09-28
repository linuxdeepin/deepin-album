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

#ifndef EXTENSIONPANEL_H
#define EXTENSIONPANEL_H

#include <DDialog>
#include <DFloatingWidget>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QScrollArea>
#include "controller/viewerthememanager.h"

// class ExtensionPanel : public DFloatingWidget

DWIDGET_USE_NAMESPACE
class ExtensionPanel : public DDialog
{
    Q_OBJECT
public:
    static ExtensionPanel *getInstance(QWidget *parent);
    void setContent(QWidget *content);
    void updateRectWithContent();

signals:
    void requestStopAnimation();

private:
    static ExtensionPanel *instance;
    explicit ExtensionPanel(QWidget *parent);
    QColor m_coverBrush;
    QWidget *m_content;
    QVBoxLayout *m_contentLayout;
    QVBoxLayout *m_mainLayout {nullptr};
    QScrollArea *m_scrollArea {nullptr};
};

#endif // EXTENSIONPANEL_H
