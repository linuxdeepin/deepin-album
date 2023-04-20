// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "flatbutton.h"

#include <DApplicationHelper>

DGUI_USE_NAMESPACE

FlatButton::FlatButton(QWidget *parent) : DPushButton(parent)
{
    setFlat(true);

    m_oldPa = DApplicationHelper::instance()->palette(this);
}

bool FlatButton::event(QEvent *event)
{
    if (event->type() == QEvent::HoverEnter) {
        setFlat(false);
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        // 修改按钮背景色，以便hover状态颜色与侧边栏悬浮色一致。
        if (themeType == DGuiApplicationHelper::LightType) {
            DPalette pa;
            pa = DApplicationHelper::instance()->palette(this);
            QColor clr = pa.color(QPalette::Base);
            pa.setColor(QPalette::Light, clr);
            pa.setColor(QPalette::Midlight, clr);
            pa.setColor(QPalette::Dark, clr);
            pa.setColor(QPalette::Mid, clr);
            pa.setColor(QPalette::Shadow, clr);
            this->setPalette(pa);
        } else {
            this->setPalette(m_oldPa);
        }
    } else if (event->type() == QEvent::HoverLeave) {
        setFlat(true);
    }

    return QPushButton::event(event);
}
