// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

    virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

protected:
    void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;

    void dragEnterEvent(QDragEnterEvent *event) override;

    /**@brief:事件重写*/
    void paintEvent(QPaintEvent *event) override;

signals:
    void signalDropEvent(QModelIndex index);
    void sigMousePressIsNoValid();
    void sigMouseMoveEvent();
    void sigMouseReleaseEvent(QModelIndex index);
private:
    //最近点击item的索引
    int m_indexLastPress = -1;
};

#endif  // LEFTLISTWIDGET_H
