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
#ifndef DDLABEL_H
#define DDLABEL_H

#include <DLabel>
#include <DWidget>
#include <DFontSizeManager>
#include <QDebug>
DWIDGET_BEGIN_NAMESPACE

class DDlabel : public DLabel
{
    Q_OBJECT
public:
    DDlabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
    void Settext(const QString &);
    QString oldstr;

private:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;



private:
    QString str;

};

DWIDGET_END_NAMESPACE
#endif // DDLABEL_H
