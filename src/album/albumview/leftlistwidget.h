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
#ifndef LEFTLISTWIDGET_H
#define LEFTLISTWIDGET_H
#include <DListWidget>
#include "utils/baseutils.h"

DWIDGET_USE_NAMESPACE

class LeftListWidget : public DListWidget
{
    Q_OBJECT
public:
    LeftListWidget();
    void mouseMoveEvent(QMouseEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    QStyleOptionViewItem viewOptions() const override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    //重命名状态下，接收外层ListView的点击事件确认保存已修改的内容
    void SaveRename(QPoint p);

protected:
    void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;

    void dragEnterEvent(QDragEnterEvent *event) override;

signals:
    void signalDropEvent(QModelIndex index);
    void sigMousePressIsNoValid();
    void sigMouseMoveEvent();
    void sigMouseReleaseEvent(QModelIndex index);

};

#endif  // LEFTLISTWIDGET_H
