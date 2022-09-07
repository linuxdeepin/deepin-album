// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMGDELETEDIALOG_H
#define IMGDELETEDIALOG_H

#include <DDialog>
#include "controller/signalmanager.h"
#include "application.h"

DWIDGET_USE_NAMESPACE

class ImgDeleteDialog : public DDialog
{
    Q_OBJECT
public:
    explicit ImgDeleteDialog(DWidget *parent, int count, bool isTrash);

    //键盘交互
    QList<QWidget *> m_allTabOrder;
    bool m_first = true;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // IMGDELETEDIALOG_H
