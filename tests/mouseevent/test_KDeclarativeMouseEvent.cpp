// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | KDeclarativeMouseEvent ctor | high | getters | 1 | 1 |
// | isAccepted | high | default+change | 1 | 1 |
// | setAccepted | high | signal-emission | 2 | 3 |
// | setX | medium | setter | 1 | 1 |
// | setY | medium | setter | 1 | 1 |

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QSignalSpy>
#include <QScreen>

#include "thumbnailview/mouseeventlistener.h"

class KDeclarativeMouseEventTest : public ::testing::Test
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
TEST_F(KDeclarativeMouseEventTest, Constructor_AllGettersMatch)
{
    QScreen *screen = QGuiApplication::primaryScreen();
    KDeclarativeMouseEvent ev(10, 20, 30, 40, Qt::LeftButton, Qt::LeftButton | Qt::RightButton,
                              Qt::ShiftModifier | Qt::ControlModifier, screen,
                              Qt::MouseEventSynthesizedByApplication);

    EXPECT_EQ(ev.x(), 10);
    EXPECT_EQ(ev.y(), 20);
    EXPECT_EQ(ev.screenX(), 30);
    EXPECT_EQ(ev.screenY(), 40);
    EXPECT_EQ(ev.button(), static_cast<int>(Qt::LeftButton));
    EXPECT_EQ(ev.buttons(), Qt::LeftButton | Qt::RightButton);
    EXPECT_EQ(ev.modifiers(), Qt::ShiftModifier | Qt::ControlModifier);
    EXPECT_EQ(ev.screen(), screen);
    EXPECT_EQ(ev.source(), static_cast<int>(Qt::MouseEventSynthesizedByApplication));
}

// isAccepted defaults to false
TEST_F(KDeclarativeMouseEventTest, IsAccepted_DefaultFalse)
{
    KDeclarativeMouseEvent ev(0, 0, 0, 0, Qt::NoButton, Qt::NoButton, Qt::NoModifier, nullptr,
                              Qt::MouseEventNotSynthesized);
    EXPECT_FALSE(ev.isAccepted());
}

// setAccepted(true) changes value and emits acceptedChanged; repeated set emits nothing
TEST_F(KDeclarativeMouseEventTest, SetAccepted_TrueEmitsSignal_NoRepeat)
{
    KDeclarativeMouseEvent ev(0, 0, 0, 0, Qt::NoButton, Qt::NoButton, Qt::NoModifier, nullptr,
                              Qt::MouseEventNotSynthesized);
    QSignalSpy spy(&ev, &KDeclarativeMouseEvent::acceptedChanged);

    ev.setAccepted(true);
    EXPECT_TRUE(ev.isAccepted());
    EXPECT_EQ(spy.count(), 1);

    // Setting same value again — no signal
    ev.setAccepted(true);
    EXPECT_EQ(spy.count(), 1);
}

// setAccepted(false) after true emits signal
TEST_F(KDeclarativeMouseEventTest, SetAccepted_FalseAfterTrueEmitsSignal)
{
    KDeclarativeMouseEvent ev(0, 0, 0, 0, Qt::NoButton, Qt::NoButton, Qt::NoModifier, nullptr,
                              Qt::MouseEventNotSynthesized);
    ev.setAccepted(true);

    QSignalSpy spy(&ev, &KDeclarativeMouseEvent::acceptedChanged);
    ev.setAccepted(false);
    EXPECT_FALSE(ev.isAccepted());
    EXPECT_EQ(spy.count(), 1);
}

// setX / setY update coordinate getters
TEST_F(KDeclarativeMouseEventTest, SetX_SetY_UpdateValues)
{
    KDeclarativeMouseEvent ev(1, 2, 3, 4, Qt::NoButton, Qt::NoButton, Qt::NoModifier, nullptr,
                              Qt::MouseEventNotSynthesized);

    ev.setX(100);
    EXPECT_EQ(ev.x(), 100);

    ev.setY(200);
    EXPECT_EQ(ev.y(), 200);
}
