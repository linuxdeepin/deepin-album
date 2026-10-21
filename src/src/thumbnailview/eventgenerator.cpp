/*
    SPDX-FileCopyrightText: 2015 Eike Hein <hein@kde.org>
    SPDX-FileCopyrightText: 2015 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "eventgenerator.h"
#include "ddlog.h"

#include <QGuiApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QDebug>

EventGenerator::EventGenerator(QObject *parent)
    : QObject(parent)
{
    qCDebug(albumLog) << "Initializing EventGenerator";
}

EventGenerator::~EventGenerator()
{
    qCDebug(albumLog) << "Destroying EventGenerator";
}

void EventGenerator::sendMouseEvent(QQuickItem *item,
                                    EventGenerator::MouseEvent type,
                                    int x,
                                    int y,
                                    int button,
                                    Qt::MouseButtons buttons,
                                    Qt::KeyboardModifiers modifiers)
{
    if (!item) {
        qCWarning(albumLog) << "Attempted to send mouse event to null item";
        return;
    }

    QEvent::Type eventType;
    switch (type) {
    case MouseButtonPress:
        eventType = QEvent::MouseButtonPress;
        qCDebug(albumLog) << "Sending mouse press event at position:" << x << y << "button:" << button;
        break;
    case MouseButtonRelease:
        eventType = QEvent::MouseButtonRelease;
        qCDebug(albumLog) << "Sending mouse release event at position:" << x << y << "button:" << button;
        break;
    case MouseMove:
        eventType = QEvent::MouseMove;
        qCDebug(albumLog) << "Sending mouse move event to position:" << x << y;
        break;
    default:
        qCWarning(albumLog) << "Unknown mouse event type:" << type;
        return;
    }
    QMouseEvent ev(eventType, QPointF(x, y), static_cast<Qt::MouseButton>(button), buttons, modifiers);

    QGuiApplication::sendEvent(item, &ev);
}

void EventGenerator::sendMouseEventRecursive(QQuickItem *parentItem,
                                             EventGenerator::MouseEvent type,
                                             int x,
                                             int y,
                                             int button,
                                             Qt::MouseButtons buttons,
                                             Qt::KeyboardModifiers modifiers)
{
    if (!parentItem) {
        qCWarning(albumLog) << "Attempted to send recursive mouse event to null parent item";
        return;
    }

    qCDebug(albumLog) << "Sending recursive mouse event to all child items";
    const QList<QQuickItem *> items = allChildItemsRecursive(parentItem);
    qCDebug(albumLog) << "Found" << items.size() << "child items to process";

    for (QQuickItem *item : items) {
        sendMouseEvent(item, type, x, y, button, buttons, modifiers);
    }
}

void EventGenerator::sendWheelEvent(QQuickItem *item,
                                    int x,
                                    int y,
                                    const QPoint &pixelDelta,
                                    const QPoint &angleDelta,
                                    Qt::MouseButtons buttons,
                                    Qt::KeyboardModifiers modifiers)
{
    if (!item || !item->window()) {
        qCWarning(albumLog) << "Attempted to send wheel event to invalid item or item without window";
        return;
    }

    qCDebug(albumLog) << "Sending wheel event at position:" << x << y 
             << "pixelDelta:" << pixelDelta << "angleDelta:" << angleDelta;
    
    QPointF pos(x, y);
    QPointF globalPos(item->window()->mapToGlobal(item->mapToScene(pos).toPoint()));
    QWheelEvent ev(pos, globalPos, pixelDelta, angleDelta, buttons, modifiers, Qt::ScrollUpdate, false /*not inverted*/);
    QGuiApplication::sendEvent(item, &ev);
}

void EventGenerator::sendWheelEventRecursive(QQuickItem *parentItem,
                                             int x,
                                             int y,
                                             const QPoint &pixelDelta,
                                             const QPoint &angleDelta,
                                             Qt::MouseButtons buttons,
                                             Qt::KeyboardModifiers modifiers)
{
    if (!parentItem) {
        qCWarning(albumLog) << "Attempted to send recursive wheel event to null parent item";
        return;
    }

    qCDebug(albumLog) << "Sending recursive wheel event to all child items";
    const QList<QQuickItem *> items = allChildItemsRecursive(parentItem);
    qCDebug(albumLog) << "Found" << items.size() << "child items to process";

    for (QQuickItem *item : items) {
        sendWheelEvent(item, x, y, pixelDelta, angleDelta, buttons, modifiers);
    }
}

void EventGenerator::sendGrabEvent(QQuickItem *item, EventGenerator::GrabEvent type)
{
    if (!item) {
        qCWarning(albumLog) << "Attempted to send grab event to null item";
        return;
    }

    switch (type) {
    case GrabMouse:
        qCDebug(albumLog) << "Grabbing mouse for item";
        item->grabMouse();
        break;
    case UngrabMouse: {
        qCDebug(albumLog) << "Ungrabbing mouse for item";
        QEvent ev(QEvent::UngrabMouse);
        QGuiApplication::sendEvent(item, &ev);
        return;
    }
    default:
        qCWarning(albumLog) << "Unknown grab event type:" << type;
        return;
    }
}

void EventGenerator::sendGrabEventRecursive(QQuickItem *parentItem, EventGenerator::GrabEvent type)
{
    if (!parentItem) {
        qCWarning(albumLog) << "Attempted to send recursive grab event to null parent item";
        return;
    }

    qCDebug(albumLog) << "Sending recursive grab event to all child items";
    const QList<QQuickItem *> items = allChildItemsRecursive(parentItem);
    qCDebug(albumLog) << "Found" << items.size() << "child items to process";

    for (QQuickItem *item : items) {
        sendGrabEvent(item, type);
    }
}

QList<QQuickItem *> EventGenerator::allChildItemsRecursive(QQuickItem *parentItem)
{
    QList<QQuickItem *> itemList;

    const auto childsItems = parentItem->childItems();
    itemList.append(childsItems);

    for (QQuickItem *childItem : childsItems) {
        itemList.append(allChildItemsRecursive(childItem));
    }

    qCDebug(albumLog) << "Found" << itemList.size() << "total child items recursively";
    return itemList;
}
