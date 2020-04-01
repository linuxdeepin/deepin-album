/*
* Copyright (C) 2019 ~ 2020 Deepin Technology Co., Ltd.
*
* Author: Deng jinhui<dengjinhui@uniontech.com>
*
* Maintainer: Deng jinhui <dengjinhui@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include "waitdevicedialog.h"

Waitdevicedialog::Waitdevicedialog(QWidget *parent): DDialog(parent)
{

}

void Waitdevicedialog::iniwaitdialog()
{
    QPixmap iconImage = QPixmap(":/icons/deepin/builtin/icons/Bullet_window_warning.svg");
    QPixmap iconI = iconImage.scaled(30, 30);
    QIcon icon(iconImage);
    this->setIcon(icon);
    //m_waitDeviceScandialog->setWindowFlag(Qt::WindowTitleHint);
    waitTips->setAlignment(Qt::AlignCenter);
    this->insertContent(0, waitTips);
    this->insertButton(1, m_closeDeviceScan);
    this->insertButton(2, m_ignoreDeviceScan);
    //this->setWindowFlag(Qt::WindowStaysOnTopHint);
}

void Waitdevicedialog::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    emit closed();
}

void Waitdevicedialog::moveEvent(QMoveEvent *event)
{

}
