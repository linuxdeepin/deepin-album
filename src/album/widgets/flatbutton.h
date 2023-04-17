// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FLATBUTTON_H
#define FLATBUTTON_H

#include <DPushButton>
#include <QEvent>

DWIDGET_USE_NAMESPACE

class FlatButton : public DPushButton
{
    Q_OBJECT
public:

    /**
     * @description: FlatButton 构造函数
    */
    explicit FlatButton(QWidget *parent = nullptr);

protected:
    bool event(QEvent * event) override;

};

#endif // FLATBUTTON_H
