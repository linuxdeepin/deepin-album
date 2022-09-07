// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
