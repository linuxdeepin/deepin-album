// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "waitdevicedialog.h"

Waitdevicedialog::Waitdevicedialog(QWidget *parent)
    : DDialog(parent), m_closeDeviceScan(nullptr), m_ignoreDeviceScan(nullptr)
    , waitTips(nullptr)
{

}

void Waitdevicedialog::iniwaitdialog()
{
    QPixmap iconImage = QPixmap(":/icons/deepin/builtin/icons/Bullet_window_warning.svg");
    QIcon icon(iconImage);
    this->setIcon(icon);
    waitTips->setAlignment(Qt::AlignCenter);
    this->insertContent(0, waitTips);
    this->insertButton(1, m_closeDeviceScan);
    this->insertButton(2, m_ignoreDeviceScan);
}

void Waitdevicedialog::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    emit closed();
}

void Waitdevicedialog::moveEvent(QMoveEvent *event)
{
    Q_UNUSED(event);
}
