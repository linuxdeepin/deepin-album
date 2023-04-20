// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ddlabel.h"

Dtk::Widget::DDlabel::DDlabel(QWidget *parent, Qt::WindowFlags f) : DLabel(parent, f)
{

}

void Dtk::Widget::DDlabel::Settext(const QString &text)
{
    QFontMetrics elideFont(this->font());

    str = text;
    oldstr = text;

    QString elidedText = elideFont.elidedText(str, Qt::ElideRight, 85);
    DLabel::setText(elideFont.elidedText(str, Qt::ElideRight, 85));
    if (str != elidedText)
        DLabel::setToolTip(str);
    else
        DLabel::setToolTip("");
}

void Dtk::Widget::DDlabel::paintEvent(QPaintEvent *event)
{
    QFontMetrics elideFont(this->font());
    this->setText(elideFont.elidedText(str, Qt::ElideRight, 85));
    DLabel::paintEvent(event);
}
