// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "pathviewrangehandler.h"
#include "ddlog.h"
#include <QDebug>

#include <QEvent>
#include <QMouseEvent>

/**
   @class PathViewRangeHandler
   @brief 用于 PathView 范围控制的辅助类，限制 PathView 在识别到图片列表头尾时
    不会拖拽超过边界。
 */
PathViewRangeHandler::PathViewRangeHandler(QObject *parent)
    : QObject(parent)
{
    qCDebug(albumLog) << "PathViewRangeHandler initialized";
}

QQuickItem *PathViewRangeHandler::target() const
{
    return targetView;
}

void PathViewRangeHandler::setTarget(QQuickItem *view)
{
    if (view != targetView) {
        if (targetView) {
            targetView->removeEventFilter(this);
            qCDebug(albumLog) << "Removed event filter from previous target view";
        }

        targetView = view;

        if (targetView) {
            targetView->installEventFilter(this);
            qCDebug(albumLog) << "Installed event filter on new target view";
        }
        Q_EMIT targetChanged();
    }
}

bool PathViewRangeHandler::enableForward() const
{
    return enableForwardFlag;
}

void PathViewRangeHandler::setEnableForward(bool b)
{
    if (b != enableForwardFlag) {
        enableForwardFlag = b;
        qCDebug(albumLog) << "Forward movement" << (b ? "enabled" : "disabled");
        Q_EMIT enableForwardChanged();
    }
}

bool PathViewRangeHandler::enableBackward() const
{
    return enableBackwardFlag;
}

void PathViewRangeHandler::setEnableBackward(bool b)
{
    if (b != enableBackwardFlag) {
        enableBackwardFlag = b;
        qCDebug(albumLog) << "Backward movement" << (b ? "enabled" : "disabled");
        Q_EMIT enableBackwardChanged();
    }
}

bool PathViewRangeHandler::eventFilter(QObject *obj, QEvent *event)
{
    if (enableForwardFlag && enableBackwardFlag && obj == targetView) {
        return false;
    }

    switch (event->type()) {
        case QEvent::MouseButtonRelease: {
            basePoint = QPointF();
            qCDebug(albumLog) << "Mouse button released, reset base point";
            break;
        }
        case QEvent::MouseMove: {
            auto mouseEvent = dynamic_cast<QMouseEvent *>(event);

            if (basePoint.isNull()) {
                // currentIndex 会在拖动时动态变更，因此在切换到受限图片时更新触发限制状态
                basePoint = mouseEvent->position();
                qCDebug(albumLog) << "Set new base point at:" << basePoint;
            } else {
                bool filter = false;
                auto newPoint = mouseEvent->position();
                if (!enableForwardFlag && newPoint.x() > (basePoint.x())) {
                    filter = true;
                    qCDebug(albumLog) << "Filtered forward movement from" << basePoint << "to" << newPoint;
                }
                if (!enableBackwardFlag && newPoint.x() < (basePoint.x())) {
                    filter = true;
                    qCDebug(albumLog) << "Filtered backward movement from" << basePoint << "to" << newPoint;
                }

                if (filter) {
                    event->ignore();
                    return true;
                }
            }

            break;
        }
        default:
            break;
    }

    return false;
}
