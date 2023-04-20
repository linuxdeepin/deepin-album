// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FLATBUTTON_H
#define FLATBUTTON_H

#include <QEvent>

#include <DPushButton>
#include <DPalette>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

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

private:
    DPalette m_oldPa;

};

#endif // FLATBUTTON_H
