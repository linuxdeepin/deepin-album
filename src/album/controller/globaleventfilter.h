// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GLOBALEVENTFILTER_H
#define GLOBALEVENTFILTER_H

#include <QObject>

class GlobalEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit GlobalEventFilter(QObject *parent = nullptr);

protected:
    bool eventFilter(QObject *obj, QEvent *e) Q_DECL_OVERRIDE;
};

#endif // GLOBALEVENTFILTER_H
