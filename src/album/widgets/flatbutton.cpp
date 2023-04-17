// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "flatbutton.h"

FlatButton::FlatButton(QWidget *parent) : DPushButton(parent)
{
    setFlat(true);
}

bool FlatButton::event(QEvent *event)
{
    if (event->type() == QEvent::HoverEnter) {
        setFlat(false);
    }

    if (event->type() == QEvent::HoverLeave) {
        setFlat(true);
    }

    return QPushButton::event(event);
}
