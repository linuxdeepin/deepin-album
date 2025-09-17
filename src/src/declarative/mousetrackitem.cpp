// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mousetrackitem.h"
#include <QDebug>

/**
   @brief 鼠标事件跟踪处理，用于处理 MouseArea 捕获 press 事件后无法向下传递的问题
 */
MouseTrackItem::MouseTrackItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    qDebug() << "MouseTrackItem::MouseTrackItem - Function entry";
    setFiltersChildMouseEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    qDebug() << "MouseTrackItem::MouseTrackItem - Function exit";
}

/**
   @brief 设置当前是否处于 \a press 状态
 */
void MouseTrackItem::setPressed(bool press)
{
    qDebug() << "MouseTrackItem::setPressed - Function entry, press:" << press;
    if (isPressed != press) {
        qDebug() << "MouseTrackItem::setPressed - Branch: state changed, emitting signal";
        isPressed = press;
        Q_EMIT pressedChanged();
    }
    qDebug() << "MouseTrackItem::setPressed - Function exit";
}

/**
   @return 返回当前是否处于点击状态
 */
bool MouseTrackItem::pressed() const
{
    // qDebug() << "MouseTrackItem::pressed - Function entry/exit";
    return isPressed;
}

/**
   @brief 捕获鼠标点击事件 \a event
 */
void MouseTrackItem::mousePressEvent(QMouseEvent *event)
{
    // qDebug() << "MouseTrackItem::mousePressEvent - Function entry";
    setPressed(true);
    event->accept();
    // qDebug() << "MouseTrackItem::mousePressEvent - Function exit";
}

/**
   @brief 捕获鼠标释放事件 \a event
 */
void MouseTrackItem::mouseReleaseEvent(QMouseEvent *event)
{
    // qDebug() << "MouseTrackItem::mouseReleaseEvent - Function entry";
    setPressed(false);
    event->accept();
    // qDebug() << "MouseTrackItem::mouseReleaseEvent - Function exit";
}

/**
   @brief 捕获鼠标双击事件 \a event
 */
void MouseTrackItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    // qDebug() << "MouseTrackItem::mouseDoubleClickEvent - Function entry";
    Q_EMIT doubleClicked();
    event->accept();
    // qDebug() << "MouseTrackItem::mouseDoubleClickEvent - Function exit";
}
