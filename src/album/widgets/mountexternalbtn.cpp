// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mountexternalbtn.h"
#include <QMouseEvent>

MountExternalBtn::MountExternalBtn(DLabel *parent) : DLabel(parent)
{

}



void MountExternalBtn::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event)
    emit sigMountExternalBtnClicked();
    event->accept();
}
