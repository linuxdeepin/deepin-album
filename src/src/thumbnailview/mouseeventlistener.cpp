/*
    SPDX-FileCopyrightText: 2011 Marco Martin <notmart@gmail.com>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later

    SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.

    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "mouseeventlistener.h"
#include "ddlog.h"

#include <QDebug>
#include <QEvent>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QScreen>
#include <QStyleHints>
#include <QTimer>

MouseEventListener::MouseEventListener(QQuickItem *parent)
    : QQuickItem(parent)
    , m_pressed(false)
    , m_pressAndHoldEvent(nullptr)
    , m_lastEvent(nullptr)
    , m_containsMouse(false)
    , m_enableMouse(true)
    , m_acceptedButtons(Qt::LeftButton)
{
    qCDebug(albumLog) << "Initializing MouseEventListener";
    m_pressAndHoldTimer = new QTimer(this);
    m_pressAndHoldTimer->setSingleShot(true);
    connect(m_pressAndHoldTimer, &QTimer::timeout, this, &MouseEventListener::handlePressAndHold);
    setFiltersChildMouseEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton | Qt::XButton1 | Qt::XButton2);
}

MouseEventListener::~MouseEventListener()
{
    qCDebug(albumLog) << "Destroying MouseEventListener";
}

Qt::MouseButtons MouseEventListener::acceptedButtons() const
{
    // qCDebug(albumLog) << "MouseEventListener::acceptedButtons - Function entry, returning:" << m_acceptedButtons;
    return m_acceptedButtons;
}

Qt::CursorShape MouseEventListener::cursorShape() const
{
    // qCDebug(albumLog) << "MouseEventListener::cursorShape - Function entry, returning:" << cursor().shape();
    return cursor().shape();
}

void MouseEventListener::setCursorShape(Qt::CursorShape shape)
{
    // qCDebug(albumLog) << "MouseEventListener::setCursorShape - Function entry, shape:" << shape;
    if (cursor().shape() == shape || !m_enableMouse) {
        return;
    }

    qCDebug(albumLog) << "Setting cursor shape from" << cursor().shape() << "to" << shape;
    setCursor(shape);

    Q_EMIT cursorShapeChanged();
}

bool MouseEventListener::enableMouse()
{
    // qCDebug(albumLog) << "MouseEventListener::enableMouse - Function entry, returning:" << m_enableMouse;
    return m_enableMouse;
}

void MouseEventListener::setEnableMouse(bool enable)
{
    // qCDebug(albumLog) << "MouseEventListener::setEnableMouse - Function entry, enable:" << enable;
    m_enableMouse = enable;
}

void MouseEventListener::setAcceptedButtons(Qt::MouseButtons buttons)
{
    // qCDebug(albumLog) << "MouseEventListener::setAcceptedButtons - Function entry, buttons:" << buttons;
    if (buttons == m_acceptedButtons) {
        // qCDebug(albumLog) << "MouseEventListener::setAcceptedButtons - Branch: buttons unchanged";
        return;
    }

    qCDebug(albumLog) << "Setting accepted buttons from" << m_acceptedButtons << "to" << buttons;
    m_acceptedButtons = buttons;
    Q_EMIT acceptedButtonsChanged();
}

void MouseEventListener::setHoverEnabled(bool enable)
{
    // qCDebug(albumLog) << "MouseEventListener::setHoverEnabled - Function entry, enable:" << enable;
    if (enable == acceptHoverEvents()) {
        // qCDebug(albumLog) << "MouseEventListener::setHoverEnabled - Branch: enable unchanged";
        return;
    }

    qCDebug(albumLog) << "Setting hover enabled from" << acceptHoverEvents() << "to" << enable;
    setAcceptHoverEvents(enable);
    Q_EMIT hoverEnabledChanged(enable);
}

bool MouseEventListener::hoverEnabled() const
{
    // qCDebug(albumLog) << "MouseEventListener::hoverEnabled - Function entry, returning:" << acceptHoverEvents();
    return acceptHoverEvents();
}

bool MouseEventListener::isPressed() const
{
    // qCDebug(albumLog) << "MouseEventListener::isPressed - Function entry, returning:" << m_pressed;
    return m_pressed;
}

void MouseEventListener::hoverEnterEvent(QHoverEvent *event)
{
    // qCDebug(albumLog) << "MouseEventListener::hoverEnterEvent - Function entry, event:" << event;
    Q_UNUSED(event);

    if (!m_enableMouse)
        return;

    qCDebug(albumLog) << "Mouse entered at position:" << event->pos();
    m_containsMouse = true;
    Q_EMIT containsMouseChanged(true);
}

void MouseEventListener::hoverLeaveEvent(QHoverEvent *event)
{
    // qCDebug(albumLog) << "MouseEventListener::hoverLeaveEvent - Function entry, event:" << event;
    Q_UNUSED(event);

    if (!m_enableMouse)
        return;

    m_containsMouse = false;
    Q_EMIT containsMouseChanged(false);
}

void MouseEventListener::hoverMoveEvent(QHoverEvent *event)
{
    // qCDebug(albumLog) << "MouseEventListener::hoverMoveEvent - Function entry, event:" << event;
    if (m_lastEvent == event || !m_enableMouse) {
        // qCDebug(albumLog) << "MouseEventListener::hoverMoveEvent - Branch: event or enableMouse is false";
        return;
    }

    QQuickWindow *w = window();
    QPoint screenPos;
    if (w) {
        screenPos = w->mapToGlobal(event->pos());
    }

    KDeclarativeMouseEvent dme(event->pos().x(),
                               event->pos().y(),
                               screenPos.x(),
                               screenPos.y(),
                               Qt::NoButton,
                               Qt::NoButton,
                               event->modifiers(),
                               nullptr,
                               Qt::MouseEventNotSynthesized);
    Q_EMIT positionChanged(&dme);
    // qCDebug(albumLog) << "MouseEventListener::hoverMoveEvent - Function exit";
}

bool MouseEventListener::containsMouse() const
{
    // qCDebug(albumLog) << "MouseEventListener::containsMouse - Function entry, returning:" << m_containsMouse;
    return m_containsMouse;
}

void MouseEventListener::mousePressEvent(QMouseEvent *me)
{
    // qCDebug(albumLog) << "MouseEventListener::mousePressEvent - Function entry, me:" << me;
    if (m_lastEvent == me || !(me->buttons() & m_acceptedButtons) || !m_enableMouse) {
        // qCDebug(albumLog) << "MouseEventListener::mousePressEvent - Branch: event or acceptedButtons or enableMouse is false";
        me->setAccepted(false);
        return;
    }

    // FIXME: when a popup window is visible: a click anywhere hides it: but the old qquickitem will continue to think it's under the mouse
    // doesn't seem to be any good way to properly reset this.
    // this msolution will still caused a missed click after the popup is gone, but gets the situation unblocked.
    QPoint viewPosition;
    if (window()) {
        viewPosition = window()->position();
    }

    if (!QRectF(mapToScene(QPoint(0, 0)) + viewPosition, QSizeF(width(), height())).contains(me->screenPos())) {
        qCDebug(albumLog) << "Mouse press ignored - outside bounds at position:" << me->screenPos();
        me->ignore();
        return;
    }
    m_buttonDownPos = me->screenPos();

    KDeclarativeMouseEvent dme(me->pos().x(),
                               me->pos().y(),
                               me->screenPos().x(),
                               me->screenPos().y(),
                               me->button(),
                               me->buttons(),
                               me->modifiers(),
                               screenForGlobalPos(me->globalPos()),
                               me->source());
    if (!m_pressAndHoldEvent) {
        m_pressAndHoldEvent = new KDeclarativeMouseEvent(me->pos().x(),
                                                         me->pos().y(),
                                                         me->screenPos().x(),
                                                         me->screenPos().y(),
                                                         me->button(),
                                                         me->buttons(),
                                                         me->modifiers(),
                                                         screenForGlobalPos(me->globalPos()),
                                                         me->source());
    }

    m_pressed = true;
    Q_EMIT pressed(&dme);
    Q_EMIT pressedChanged();

    if (dme.isAccepted()) {
        me->setAccepted(true);
        return;
    }

    m_pressAndHoldTimer->start(QGuiApplication::styleHints()->mousePressAndHoldInterval());
    // qCDebug(albumLog) << "MouseEventListener::mousePressEvent - Function exit";
}

void MouseEventListener::mouseMoveEvent(QMouseEvent *me)
{
    // qCDebug(albumLog) << "MouseEventListener::mouseMoveEvent - Function entry, me:" << me;
    if (m_lastEvent == me || !(me->buttons() & m_acceptedButtons) || !m_enableMouse) {
        // qCDebug(albumLog) << "MouseEventListener::mouseMoveEvent - Branch: event or acceptedButtons or enableMouse is false";
        me->setAccepted(false);
        return;
    }

    if (QPointF(me->screenPos() - m_buttonDownPos).manhattanLength() > QGuiApplication::styleHints()->startDragDistance() && m_pressAndHoldTimer->isActive()) {
        qCDebug(albumLog) << "Cancelling press and hold - mouse moved beyond drag distance";
        m_pressAndHoldTimer->stop();
    }

    KDeclarativeMouseEvent dme(me->pos().x(),
                               me->pos().y(),
                               me->screenPos().x(),
                               me->screenPos().y(),
                               me->button(),
                               me->buttons(),
                               me->modifiers(),
                               screenForGlobalPos(me->globalPos()),
                               me->source());
    Q_EMIT positionChanged(&dme);

    if (dme.isAccepted()) {
        // qCDebug(albumLog) << "MouseEventListener::mouseMoveEvent - Branch: dme is accepted";
        me->setAccepted(true);
    }
    // qCDebug(albumLog) << "MouseEventListener::mouseMoveEvent - Function exit";
}

void MouseEventListener::mouseReleaseEvent(QMouseEvent *me)
{
    // qCDebug(albumLog) << "MouseEventListener::mouseReleaseEvent - Function entry, me:" << me;
    if (m_lastEvent == me || !m_enableMouse) {
        // qCDebug(albumLog) << "MouseEventListener::mouseReleaseEvent - Branch: event or enableMouse is false";
        me->setAccepted(false);
        return;
    }

    KDeclarativeMouseEvent dme(me->pos().x(),
                               me->pos().y(),
                               me->screenPos().x(),
                               me->screenPos().y(),
                               me->button(),
                               me->buttons(),
                               me->modifiers(),
                               screenForGlobalPos(me->globalPos()),
                               me->source());
    m_pressed = false;
    Q_EMIT released(&dme);
    Q_EMIT pressedChanged();

    if (boundingRect().contains(me->pos()) && m_pressAndHoldTimer->isActive()) {
        qCDebug(albumLog) << "Mouse clicked at position:" << me->pos();
        Q_EMIT clicked(&dme);
        m_pressAndHoldTimer->stop();
    }

    if (dme.isAccepted()) {
        me->setAccepted(true);
    }
    // qCDebug(albumLog) << "MouseEventListener::mouseReleaseEvent - Function exit";
}

void MouseEventListener::wheelEvent(QWheelEvent *we)
{
    // qCDebug(albumLog) << "MouseEventListener::wheelEvent - Function entry, we:" << we;
    if (m_lastEvent == we || !m_enableMouse) {
        // qCDebug(albumLog) << "MouseEventListener::wheelEvent - Branch: event or enableMouse is false";
        we->setAccepted(false);
        return;
    }

#if (QT_VERSION >= QT_VERSION_CHECK(5,15,2))
    KDeclarativeWheelEvent dwe(we->position().toPoint(),
                               we->globalPosition().toPoint(),
                               we->angleDelta(),
                               we->buttons(),
                               we->modifiers(),
                               Qt::Vertical /* HACK, deprecated, remove */);
#elif (QT_VERSION <= QT_VERSION_CHECK(5,11,3))
    KDeclarativeWheelEvent dwe(we->pos(),
                               we->globalPos(),
                               we->angleDelta(),
                               we->buttons(),
                               we->modifiers(),
                               Qt::Vertical /* HACK, deprecated, remove */);
#endif
    Q_EMIT wheelMoved(&dwe);
    // qCDebug(albumLog) << "MouseEventListener::wheelEvent - Function exit";
}

void MouseEventListener::handlePressAndHold()
{
    // qCDebug(albumLog) << "MouseEventListener::handlePressAndHold - Function entry";
    if (!m_enableMouse)
        return;

    if (m_pressed) {
        Q_EMIT pressAndHold(m_pressAndHoldEvent);

        delete m_pressAndHoldEvent;
        m_pressAndHoldEvent = nullptr;
    }
}

bool MouseEventListener::childMouseEventFilter(QQuickItem *item, QEvent *event)
{
    // qCDebug(albumLog) << "MouseEventListener::childMouseEventFilter - Function entry, item:" << item << "event:" << event;
    if (!isEnabled() || !m_enableMouse) {
        // qCDebug(albumLog) << "MouseEventListener::childMouseEventFilter - Branch: isEnabled or enableMouse is false";
        return false;
    }

    // don't filter other mouseeventlisteners
    if (qobject_cast<MouseEventListener *>(item)) {
        // qCDebug(albumLog) << "MouseEventListener::childMouseEventFilter - Branch: item is a MouseEventListener";
        return false;
    }

    switch (event->type()) {
    case QEvent::MouseButtonPress: {
        // qCDebug(albumLog) << "MouseEventListener::childMouseEventFilter - Branch: MouseButtonPress";
        m_lastEvent = event;
        QMouseEvent *me = static_cast<QMouseEvent *>(event);

        if (!(me->buttons() & m_acceptedButtons)) {
            // qCDebug(albumLog) << "MouseEventListener::childMouseEventFilter - Branch: buttons not accepted";
            break;
        }

        // the parent will receive events in its own coordinates
        const QPointF myPos = mapFromScene(me->windowPos());

        KDeclarativeMouseEvent dme(myPos.x(),
                                   myPos.y(),
                                   me->screenPos().x(),
                                   me->screenPos().y(),
                                   me->button(),
                                   me->buttons(),
                                   me->modifiers(),
                                   screenForGlobalPos(me->globalPos()),
                                   me->source());
        delete m_pressAndHoldEvent;
        m_pressAndHoldEvent = new KDeclarativeMouseEvent(myPos.x(),
                                                         myPos.y(),
                                                         me->screenPos().x(),
                                                         me->screenPos().y(),
                                                         me->button(),
                                                         me->buttons(),
                                                         me->modifiers(),
                                                         screenForGlobalPos(me->globalPos()),
                                                         me->source());

        m_buttonDownPos = me->screenPos();
        m_pressed = true;
        Q_EMIT pressed(&dme);
        Q_EMIT pressedChanged();

        if (dme.isAccepted()) {
            return true;
        }

        m_pressAndHoldTimer->start(QGuiApplication::styleHints()->mousePressAndHoldInterval());

        break;
    }
    case QEvent::HoverMove: {
        // qCDebug(albumLog) << "MouseEventListener::childMouseEventFilter - Branch: HoverMove";
        if (!acceptHoverEvents()) {
            break;
        }
        m_lastEvent = event;
        QHoverEvent *he = static_cast<QHoverEvent *>(event);
        const QPointF myPos = item->mapToItem(this, he->pos());

        QQuickWindow *w = window();
        QPoint screenPos;
        if (w) {
            screenPos = w->mapToGlobal(myPos.toPoint());
        }

        KDeclarativeMouseEvent
        dme(myPos.x(), myPos.y(), screenPos.x(), screenPos.y(), Qt::NoButton, Qt::NoButton, he->modifiers(), nullptr, Qt::MouseEventNotSynthesized);
        // qCDebug(albumLog) << "positionChanged..." << dme.x() << dme.y();
        Q_EMIT positionChanged(&dme);

        if (dme.isAccepted()) {
            return true;
        }
        break;
    }
    case QEvent::MouseMove: {
        // qCDebug(albumLog) << "MouseEventListener::childMouseEventFilter - Branch: MouseMove";
        m_lastEvent = event;
        QMouseEvent *me = static_cast<QMouseEvent *>(event);
        if (!(me->buttons() & m_acceptedButtons)) {
            break;
        }

        const QPointF myPos = mapFromScene(me->windowPos());
        KDeclarativeMouseEvent dme(myPos.x(),
                                   myPos.y(),
                                   me->screenPos().x(),
                                   me->screenPos().y(),
                                   me->button(),
                                   me->buttons(),
                                   me->modifiers(),
                                   screenForGlobalPos(me->globalPos()),
                                   me->source());

        // stop the pressandhold if mouse moved enough
        if (QPointF(me->screenPos() - m_buttonDownPos).manhattanLength() > QGuiApplication::styleHints()->startDragDistance()
                && m_pressAndHoldTimer->isActive()) {
            qCDebug(albumLog) << "Cancelling press and hold - mouse moved beyond drag distance";
            m_pressAndHoldTimer->stop();

            // if the mouse moves and we are waiting to emit a press and hold event, update the coordinates
            // as there is no update function, delete the old event and create a new one
        } else if (m_pressAndHoldEvent) {
            delete m_pressAndHoldEvent;
            m_pressAndHoldEvent = new KDeclarativeMouseEvent(myPos.x(),
                                                             myPos.y(),
                                                             me->screenPos().x(),
                                                             me->screenPos().y(),
                                                             me->button(),
                                                             me->buttons(),
                                                             me->modifiers(),
                                                             screenForGlobalPos(me->globalPos()),
                                                             me->source());
        }
        Q_EMIT positionChanged(&dme);

        if (dme.isAccepted()) {
            return true;
        }
        break;
    }
    case QEvent::MouseButtonRelease: {
        // qCDebug(albumLog) << "MouseEventListener::childMouseEventFilter - Branch: MouseButtonRelease";
        m_lastEvent = event;
        QMouseEvent *me = static_cast<QMouseEvent *>(event);

        const QPointF myPos = mapFromScene(me->windowPos());
        KDeclarativeMouseEvent dme(myPos.x(),
                                   myPos.y(),
                                   me->screenPos().x(),
                                   me->screenPos().y(),
                                   me->button(),
                                   me->buttons(),
                                   me->modifiers(),
                                   screenForGlobalPos(me->globalPos()),
                                   me->source());
        m_pressed = false;

        Q_EMIT released(&dme);
        Q_EMIT pressedChanged();

        if (QPointF(me->screenPos() - m_buttonDownPos).manhattanLength() <= QGuiApplication::styleHints()->startDragDistance()
                && m_pressAndHoldTimer->isActive()) {
            qCDebug(albumLog) << "Child mouse clicked at position:" << myPos;
            Q_EMIT clicked(&dme);
            m_pressAndHoldTimer->stop();
        }

        if (dme.isAccepted()) {
            return true;
        }
        break;
    }
    case QEvent::UngrabMouse: {
        // qCDebug(albumLog) << "MouseEventListener::childMouseEventFilter - Branch: UngrabMouse";
        m_lastEvent = event;
        qCDebug(albumLog) << "Mouse ungrab event received";
        handleUngrab();
        break;
    }
    case QEvent::Wheel: {
        // qCDebug(albumLog) << "MouseEventListener::childMouseEventFilter - Branch: Wheel";
        m_lastEvent = event;
        QWheelEvent *we = static_cast<QWheelEvent *>(event);
#if (QT_VERSION >= QT_VERSION_CHECK(5,15,2))
        KDeclarativeWheelEvent dwe(we->position().toPoint(),
                                   we->globalPosition().toPoint(),
                                   we->angleDelta(),
                                   we->buttons(),
                                   we->modifiers(),
                                   Qt::Vertical /* HACK, deprecated, remove */);
#elif (QT_VERSION <= QT_VERSION_CHECK(5,11,3))
        KDeclarativeWheelEvent dwe(we->pos(),
                                   we->globalPos(),
                                   we->angleDelta(),
                                   we->buttons(),
                                   we->modifiers(),
                                   Qt::Vertical /* HACK, deprecated, remove */);
#endif
        Q_EMIT wheelMoved(&dwe);
        break;
    }
    default:
        break;
    }

    // qCDebug(albumLog) << "MouseEventListener::childMouseEventFilter - Function exit";
    return QQuickItem::childMouseEventFilter(item, event);
}

QScreen *MouseEventListener::screenForGlobalPos(const QPoint &globalPos)
{
    // qCDebug(albumLog) << "MouseEventListener::screenForGlobalPos - Function entry, globalPos:" << globalPos;
    const auto screens = QGuiApplication::screens();
    for (QScreen *screen : screens) {
        if (screen->geometry().contains(globalPos)) {
            // qCDebug(albumLog) << "MouseEventListener::screenForGlobalPos - Branch: found screen for position";
            return screen;
        }
    }
    // qCDebug(albumLog) << "MouseEventListener::screenForGlobalPos - Function exit, returning nullptr";
    return nullptr;
}

void MouseEventListener::mouseUngrabEvent()
{
    qCDebug(albumLog) << "Mouse ungrab event received";
    handleUngrab();

    QQuickItem::mouseUngrabEvent();
}

void MouseEventListener::touchUngrabEvent()
{
    qCDebug(albumLog) << "Touch ungrab event received";
    handleUngrab();

    QQuickItem::touchUngrabEvent();
}

void MouseEventListener::handleUngrab()
{
    qCDebug(albumLog) << "MouseEventListener::handleUngrab - Function entry";
    if (!m_enableMouse)
        return;

    if (m_pressed) {
        qCDebug(albumLog) << "Handling ungrab while pressed";
        m_pressAndHoldTimer->stop();

        m_pressed = false;
        Q_EMIT pressedChanged();

        Q_EMIT canceled();
    }
    qCDebug(albumLog) << "MouseEventListener::handleUngrab - Function exit";
}
