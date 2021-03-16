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
#include "ddlabel.h"

Dtk::Widget::DDlabel::DDlabel(QWidget *parent, Qt::WindowFlags f) : DLabel(parent, f)
{

}

void Dtk::Widget::DDlabel::Settext(const QString &text)
{
    QFontMetrics elideFont(this->font());

    str = text;
    oldstr = text;

    DLabel::setText(elideFont.elidedText(str, Qt::ElideRight, 85));
}

void Dtk::Widget::DDlabel::paintEvent(QPaintEvent *event)
{
//    Q_UNUSED(event)
//    qDebug() << "this->text()" << this->text();
//    DLabel::paintEvent(event);
//    QString str  = this->text();
    QFontMetrics elideFont(this->font());
    this->setText(elideFont.elidedText(str, Qt::ElideRight, 85));
//    this->Settext(str);
    DLabel::paintEvent(event);
}
