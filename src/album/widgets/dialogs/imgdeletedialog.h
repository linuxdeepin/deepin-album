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
    explicit ImgDeleteDialog(DWidget *parent, int imgCount, int videoCount);

    //键盘交互
    QList<QWidget *> m_allTabOrder;
    bool m_first = true;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
};

#endif // IMGDELETEDIALOG_H
