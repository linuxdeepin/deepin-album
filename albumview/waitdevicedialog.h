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
#ifndef WAITDEVICEDIALOG_H
#define WAITDEVICEDIALOG_H

#include "ddialog.h"

#include <QObject>
#include <DDialog>
#include <QCloseEvent>
#include <QMoveEvent>
#include <DLabel>
#include <DPushButton>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

class Waitdevicedialog : public DDialog
{

public:
    explicit Waitdevicedialog(QWidget *parent = nullptr);
    void iniwaitdialog();
    void closeEvent(QCloseEvent *event) override;
    void moveEvent(QMoveEvent *event) override;
signals:
    void sigclose();
public:
    DPushButton *m_closeDeviceScan;
    DPushButton *m_ignoreDeviceScan;
    DLabel *waitTips;
};

#endif // WAITDEVICEDIALOG_H
