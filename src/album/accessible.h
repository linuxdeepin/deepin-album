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
#include "accessibledefine.h"
#include "mainwindow.h"
#include "albumview.h"

#include <DImageButton>
#include <DSwitchButton>
#include <DPushButton>
DWIDGET_USE_NAMESPACE

// 添加accessible

SET_BUTTON_ACCESSIBLE(DPushButton, "dpushbutton");
SET_MENU_ACCESSIBLE(DMenu, "dmenu");
SET_WIDGET_ACCESSIBLE(DMainWindow, QAccessible::Form, "main");

QAccessibleInterface *accessibleFactory(const QString &classname, QObject *object)
{
    QAccessibleInterface *interface = nullptr;
    USE_ACCESSIBLE(classname, DPushButton);
    USE_ACCESSIBLE(classname, DMenu);

    return interface;
}
