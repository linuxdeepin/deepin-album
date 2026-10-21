// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mousetrackitem.h"
#include "ddlog.h"
#include <QDebug>

/**
   @brief 鼠标事件跟踪处理，用于处理 MouseArea 捕获 press 事件后无法向下传递的问题
 */
MouseTrackItem::MouseTrackItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    qCDebug(albumLog) << "MouseTrackItem::MouseTrackItem - Function entry";
    setFiltersChildMouseEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
    qCDebug(albumLog) << "MouseTrackItem::MouseTrackItem - Function exit";
}

/**
   @brief 设置当前是否处于 \a press 状态
 */
void MouseTrackItem::setPressed(bool press)
{
    qCDebug(albumLog) << "MouseTrackItem::setPressed - Function entry, press:" << press;
    if (isPressed != press) {
        qCDebug(albumLog) << "MouseTrackItem::setPressed - Branch: state changed, emitting signal";
        isPressed = press;
        Q_EMIT pressedChanged();
    }
    qCDebug(albumLog) << "MouseTrackItem::setPressed - Function exit";
}

/**
   @return 返回当前是否处于点击状态
 */
bool MouseTrackItem::pressed() const
{
    // qCDebug(albumLog) << "MouseTrackItem::pressed - Function entry/exit";
    return isPressed;
}

/**
   @brief 捕获鼠标点击事件 \a event
 */
void MouseTrackItem::mousePressEvent(QMouseEvent *event)
{
    // qCDebug(albumLog) << "MouseTrackItem::mousePressEvent - Function entry";
    setPressed(true);
    event->accept();
    // qCDebug(albumLog) << "MouseTrackItem::mousePressEvent - Function exit";
}

/**
   @brief 捕获鼠标释放事件 \a event
 */
void MouseTrackItem::mouseReleaseEvent(QMouseEvent *event)
{
    // qCDebug(albumLog) << "MouseTrackItem::mouseReleaseEvent - Function entry";
    setPressed(false);
    event->accept();
    // qCDebug(albumLog) << "MouseTrackItem::mouseReleaseEvent - Function exit";
}

/**
   @brief 捕获鼠标双击事件 \a event
 */
void MouseTrackItem::mouseDoubleClickEvent(QMouseEvent *event)
{
    // qCDebug(albumLog) << "MouseTrackItem::mouseDoubleClickEvent - Function entry";
    Q_EMIT doubleClicked();
    event->accept();
    // qCDebug(albumLog) << "MouseTrackItem::mouseDoubleClickEvent - Function exit";
}
