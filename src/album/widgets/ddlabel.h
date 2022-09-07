// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
