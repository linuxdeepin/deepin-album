// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MOUNTEXTERNALBTN_H
#define MOUNTEXTERNALBTN_H

#include <QObject>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class MountExternalBtn : public DLabel
{
    Q_OBJECT
public:
    explicit MountExternalBtn(DLabel *parent = nullptr);

    void mousePressEvent(QMouseEvent *event) override;

signals:
    void sigMountExternalBtnClicked();
};

#endif // MOUNTEXTERNALBTN_H
