// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "flatbutton.h"

#include <DPalette>
#include <DApplicationHelper>

DGUI_USE_NAMESPACE

FlatButton::FlatButton(QWidget *parent) : DPushButton(parent)
{
    setFlat(true);

    // 修改按钮背景色，以便hover状态颜色与侧边栏悬浮色一致。
    DPalette pa;
    pa = DApplicationHelper::instance()->palette(this);
    QColor clr = QColor(255, 255, 255, 255);
    pa.setColor(QPalette::Light, clr);
    pa.setColor(QPalette::Midlight, clr);
    pa.setColor(QPalette::Dark, clr);
    pa.setColor(QPalette::Mid, clr);
    pa.setColor(QPalette::Shadow, clr);
    this->setPalette(pa);

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
