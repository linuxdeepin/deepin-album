// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | QmlCustomInternalWidget::QmlCustomInternalWidget | high | 4 | 4 | 4 |
 * | QmlCustomInternalWidget::paintEngine              | high | 1 | 1 | 1 |
 * | QmlCustomInternalWidget::paintEvent               | high | 2 | 2 | 3 |
 *
 * 分支列表 (Constructor):
 *   B1: focusPolicy set to Qt::WheelFocus
 *   B2: WA_WState_Created attribute set to true
 *   B3: visible set to true
 *   B4: m_qquickContainer set to parent
 *
 * 分支列表 (paintEngine):
 *   B1: returns nullptr
 *
 * 分支列表 (paintEvent):
 *   B1: delegates event->rect() to m_qquickContainer->update()
 *   B2: QWidget::paintEvent called first (no crash)
 *   B3: update called exactly once with correct rect
 */

// Include all system/Qt headers first (before #define private public)
#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QApplication>
#include <QQuickPaintedItem>
#include <QQuickItem>
#include <QWidget>
#include <QPaintEvent>
#include <QRect>
#include <DWidget>
DWIDGET_USE_NAMESPACE

#include "types.h"
#include "stubext.h"
using namespace stub_ext;

// Make private/protected members accessible for testing the private constructor
#define private public
#define protected public
#include "qmlWidget.h"
#undef private
#undef protected

class QmlCustomInternalWidgetTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test";
            static char *argv[] = {arg0, nullptr};
            new QApplication(argc, argv);
        }
    }

    static void TearDownTestSuite() {}

    StubExt stub;
    QmlWidget *parentItem = nullptr;
    QmlCustomInternalWidget *widget = nullptr;
    int updateCallCount = 0;
    QRect lastUpdateRect;

    void SetUp() override
    {
        updateCallCount = 0;
        lastUpdateRect = QRect();

        // Create parent QQuickPaintedItem (QmlWidget extends QQuickPaintedItem)
        // QmlWidget constructor calls initWidget() but viewType defaults to
        // WidgetViewUnknown (-1) so initWidget returns early without creating views
        parentItem = new QmlWidget();

        // Create QmlCustomInternalWidget — constructor is private but
        // #define private public made it accessible
        widget = new QmlCustomInternalWidget(parentItem);
    }

    void TearDown() override
    {
        stub.clear();
        delete widget;
        widget = nullptr;
        delete parentItem;
        parentItem = nullptr;
    }
};

// ============ Constructor tests ============

// B1: focusPolicy set to Qt::WheelFocus
TEST_F(QmlCustomInternalWidgetTest, Constructor_FocusPolicy_IsWheelFocus)
{
    EXPECT_EQ(widget->focusPolicy(), Qt::WheelFocus);
}

// B2: WA_WState_Created attribute set to true
TEST_F(QmlCustomInternalWidgetTest, Constructor_WStateCreated_IsSet)
{
    EXPECT_TRUE(widget->testAttribute(Qt::WA_WState_Created));
}

// B3: visible set to true
TEST_F(QmlCustomInternalWidgetTest, Constructor_Visible_IsTrue)
{
    EXPECT_TRUE(widget->isVisible());
}

// B4: m_qquickContainer set to parent
TEST_F(QmlCustomInternalWidgetTest, Constructor_ContainerSetToParent)
{
    EXPECT_EQ(widget->m_qquickContainer, parentItem);
}

// ============ paintEngine tests ============

// B1: returns nullptr
TEST_F(QmlCustomInternalWidgetTest, PaintEngine_ReturnsNullptr)
{
    QPaintEngine *engine = widget->paintEngine();
    EXPECT_EQ(engine, nullptr);
}

// ============ paintEvent tests ============

// B1+B3: delegates event->rect() to m_qquickContainer->update() exactly once
TEST_F(QmlCustomInternalWidgetTest, PaintEvent_ForwardsRectToUpdate)
{
    // Stub QQuickPaintedItem::update(const QRect&) to record calls
    stub.set_lamda(
        static_cast<void (QQuickPaintedItem::*)(const QRect &)>(&QQuickPaintedItem::update),
        [this](QQuickPaintedItem *self, const QRect &rect) -> void {
            updateCallCount++;
            lastUpdateRect = rect;
        });

    QRect eventRect(10, 20, 100, 200);
    QPaintEvent event(eventRect);
    widget->paintEvent(&event);

    EXPECT_EQ(updateCallCount, 1);
    EXPECT_EQ(lastUpdateRect, eventRect);
}

// B2: QWidget::paintEvent called first (no crash)
TEST_F(QmlCustomInternalWidgetTest, PaintEvent_QWidgetPaintEvent_NoCrash)
{
    // Stub update to avoid QML rendering side effects
    stub.set_lamda(
        static_cast<void (QQuickPaintedItem::*)(const QRect &)>(&QQuickPaintedItem::update),
        [](QQuickPaintedItem *, const QRect &) -> void {});

    QRect eventRect(0, 0, 50, 50);
    QPaintEvent event(eventRect);
    widget->paintEvent(&event);

    // If we got here without crashing, the test passed
    SUCCEED();
}

// B1 variant: different rect values forwarded correctly
TEST_F(QmlCustomInternalWidgetTest, PaintEvent_DifferentRect_ForwardedCorrectly)
{
    stub.set_lamda(
        static_cast<void (QQuickPaintedItem::*)(const QRect &)>(&QQuickPaintedItem::update),
        [this](QQuickPaintedItem *self, const QRect &rect) -> void {
            updateCallCount++;
            lastUpdateRect = rect;
        });

    QRect eventRect(5, 5, 300, 400);
    QPaintEvent event(eventRect);
    widget->paintEvent(&event);

    EXPECT_EQ(updateCallCount, 1);
    EXPECT_EQ(lastUpdateRect.width(), 300);
    EXPECT_EQ(lastUpdateRect.height(), 400);
    EXPECT_EQ(lastUpdateRect.x(), 5);
    EXPECT_EQ(lastUpdateRect.y(), 5);
}
