// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | KDeclarativeWheelEvent ctor | high | getters | 1 | 1 |
// | delta == deltaY (Q_PROPERTY) | high | property-alias | 1 | 1 |
// | setX | medium | setter | 1 | 1 |
// | setY | medium | setter | 1 | 1 |

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QPointF>
#include <QPoint>

#include "thumbnailview/mouseeventlistener.h"

class KDeclarativeWheelEventTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }
    }
};

// Constructor stores all parameters correctly in getters
TEST_F(KDeclarativeWheelEventTest, Constructor_AllGettersMatch)
{
    QPointF pos(10.5, 20.5);
    QPoint screenPos(30, 40);
    QPoint angleDelta(1, 15);  // typical wheel: deltaX=1, deltaY=15
    Qt::MouseButtons buttons = Qt::LeftButton | Qt::MiddleButton;
    Qt::KeyboardModifiers modifiers = Qt::ShiftModifier;
    Qt::Orientation orientation = Qt::Vertical;

    KDeclarativeWheelEvent ev(pos, screenPos, angleDelta, buttons, modifiers, orientation);

    EXPECT_EQ(ev.x(), 10);   // pos.x() truncated to int
    EXPECT_EQ(ev.y(), 20);   // pos.y() truncated to int
    EXPECT_EQ(ev.screenX(), 30);
    EXPECT_EQ(ev.screenY(), 40);
    EXPECT_EQ(ev.deltaX(), 1);
    EXPECT_EQ(ev.deltaY(), 15);
    EXPECT_EQ(ev.buttons(), buttons);
    EXPECT_EQ(ev.modifiers(), modifiers);
    EXPECT_EQ(ev.orientation(), orientation);
}

// Q_PROPERTY "delta" reads the same as deltaY
TEST_F(KDeclarativeWheelEventTest, DeltaProperty_EqualsDeltaY)
{
    QPointF pos(0, 0);
    QPoint screenPos(0, 0);
    QPoint angleDelta(0, 120);
    KDeclarativeWheelEvent ev(pos, screenPos, angleDelta, Qt::NoButton, Qt::NoModifier, Qt::Vertical);

    int deltaProp = ev.property("delta").toInt();
    EXPECT_EQ(deltaProp, ev.deltaY());
    EXPECT_EQ(deltaProp, 120);
}

// setX / setY update coordinate getters
TEST_F(KDeclarativeWheelEventTest, SetX_SetY_UpdateValues)
{
    QPointF pos(1, 2);
    QPoint screenPos(3, 4);
    QPoint angleDelta(0, 0);
    KDeclarativeWheelEvent ev(pos, screenPos, angleDelta, Qt::NoButton, Qt::NoModifier, Qt::Vertical);

    ev.setX(100);
    EXPECT_EQ(ev.x(), 100);

    ev.setY(200);
    EXPECT_EQ(ev.y(), 200);
}
