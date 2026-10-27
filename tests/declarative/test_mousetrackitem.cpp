// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | MouseTrackItem (ctor) | mid | complexity:2 | 2 | 2 |
// | setPressed | mid | complexity:2 | 2 | 3 |
// | pressed | low | complexity:0 | 1 | 2 |
// | mousePressEvent | mid | complexity:1 | 2 | 2 |
// | mouseReleaseEvent | mid | complexity:1 | 2 | 2 |
// | mouseDoubleClickEvent | mid | complexity:1 | 1 | 2 |
// ─── 生成后填入 actual 列，低于 min 即违规 ───

// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]
// 3. 每个等价类的边界值显式覆盖: [x]
// 4. 同质 ≥ 3 组用 TEST_P: [x]
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]
// 8. 负面场景有专门用例: [x]
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]

// 分支清单映射（MouseTrackItem ctor complexity=2）：
// B1: setFiltersChildMouseEvents(true)
// B2: setAcceptedMouseButtons(Qt::LeftButton)
// 用例映射: B1+B2→Constructor_DefaultState
//
// 分支清单映射（setPressed complexity=2）：
// B1: isPressed != press → set + emit pressedChanged
// B2: isPressed == press → no-op (no signal)
// 用例映射: B1→SetPressed_TrueEmitsSignal, B2→SetPressed_DuplicateNoSignal
//
// 分支清单映射（mousePressEvent complexity=1）：
// B1: setPressed(true) + event->accept()
// 用例映射: B1→MousePressEvent_SetsPressedTrue
//
// 分支清单映射（mouseReleaseEvent complexity=1）：
// B1: setPressed(false) + event->accept()
// 用例映射: B1→MouseReleaseEvent_SetsPressedFalse
//
// 分支清单映射（mouseDoubleClickEvent complexity=1）：
// B1: emit doubleClicked() + event->accept()
// 用例映射: B1→MouseDoubleClickEvent_EmitsSignal

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QPointF>
#include <memory>

#include "declarative/mousetrackitem.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// Access private field isPressed for direct verification
ACCESS_PRIVATE_FIELD(MouseTrackItem, bool, isPressed)

// mousePressEvent / mouseReleaseEvent / mouseDoubleClickEvent are protected;
// create a testable subclass that exposes them as public
class TestableMouseTrackItem : public MouseTrackItem
{
public:
    using MouseTrackItem::MouseTrackItem;
    using MouseTrackItem::mousePressEvent;
    using MouseTrackItem::mouseReleaseEvent;
    using MouseTrackItem::mouseDoubleClickEvent;
};

class MouseTrackItemTest : public ::testing::Test
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

    static void TearDownTestSuite() {}

    void SetUp() override
    {
        m_item = std::make_unique<TestableMouseTrackItem>();
    }

    void TearDown() override
    {
        m_item.reset();
    }

    std::unique_ptr<TestableMouseTrackItem> m_item;
};

// ─────────────────────────── Constructor ───────────────────────────

// B1+B2: Constructor sets filtersChildMouseEvents(true) and acceptedMouseButtons(LeftButton)
TEST_F(MouseTrackItemTest, Constructor_DefaultState)
{
    EXPECT_TRUE(m_item->filtersChildMouseEvents());
    EXPECT_EQ(m_item->acceptedMouseButtons(), Qt::LeftButton);
}

// Constructor: pressed initial value is false
TEST_F(MouseTrackItemTest, Constructor_PressedInitiallyFalse)
{
    EXPECT_FALSE(m_item->pressed());
    EXPECT_FALSE(access_private_field::MouseTrackItemisPressed(*m_item));
}

// ─────────────────────────── pressed ───────────────────────────

TEST_F(MouseTrackItemTest, Pressed_ReturnsFalseByDefault)
{
    EXPECT_FALSE(m_item->pressed());
}

TEST_F(MouseTrackItemTest, Pressed_ReturnsTrueAfterSet)
{
    m_item->setPressed(true);
    EXPECT_TRUE(m_item->pressed());
}

// ─────────────────────────── setPressed ───────────────────────────

// B1: Setting to true when current is false emits pressedChanged
TEST_F(MouseTrackItemTest, SetPressed_TrueEmitsSignal)
{
    QSignalSpy spy(m_item.get(), &MouseTrackItem::pressedChanged);
    ASSERT_TRUE(spy.isValid());

    m_item->setPressed(true);

    EXPECT_TRUE(m_item->pressed());
    EXPECT_EQ(spy.count(), 1);
}

// B1: Setting to false when current is true emits pressedChanged
TEST_F(MouseTrackItemTest, SetPressed_FalseEmitsSignal)
{
    m_item->setPressed(true);
    ASSERT_TRUE(m_item->pressed());

    QSignalSpy spy(m_item.get(), &MouseTrackItem::pressedChanged);
    ASSERT_TRUE(spy.isValid());

    m_item->setPressed(false);

    EXPECT_FALSE(m_item->pressed());
    EXPECT_EQ(spy.count(), 1);
}

// B2: Setting same value does not emit pressedChanged
TEST_F(MouseTrackItemTest, SetPressed_DuplicateNoSignal)
{
    m_item->setPressed(true);
    ASSERT_TRUE(m_item->pressed());

    QSignalSpy spy(m_item.get(), &MouseTrackItem::pressedChanged);
    ASSERT_TRUE(spy.isValid());

    m_item->setPressed(true);

    EXPECT_TRUE(m_item->pressed());
    EXPECT_EQ(spy.count(), 0);
}

// ─────────────────────────── mousePressEvent ───────────────────────────

// B1: mousePressEvent sets pressed=true and accepts the event
TEST_F(MouseTrackItemTest, MousePressEvent_SetsPressedTrue)
{
    ASSERT_FALSE(m_item->pressed());

    QMouseEvent event(QEvent::MouseButtonPress, QPointF(0, 0), QPointF(0, 0),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    m_item->mousePressEvent(&event);

    EXPECT_TRUE(m_item->pressed());
    EXPECT_TRUE(event.isAccepted());
}

// mousePressEvent emits pressedChanged when state transitions false→true
TEST_F(MouseTrackItemTest, MousePressEvent_EmitsPressedChanged)
{
    QSignalSpy spy(m_item.get(), &MouseTrackItem::pressedChanged);
    ASSERT_TRUE(spy.isValid());

    QMouseEvent event(QEvent::MouseButtonPress, QPointF(0, 0), QPointF(0, 0),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    m_item->mousePressEvent(&event);

    EXPECT_EQ(spy.count(), 1);
}

// ─────────────────────────── mouseReleaseEvent ───────────────────────────

// B1: mouseReleaseEvent sets pressed=false and accepts the event
TEST_F(MouseTrackItemTest, MouseReleaseEvent_SetsPressedFalse)
{
    m_item->setPressed(true);
    ASSERT_TRUE(m_item->pressed());

    QMouseEvent event(QEvent::MouseButtonRelease, QPointF(0, 0), QPointF(0, 0),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    m_item->mouseReleaseEvent(&event);

    EXPECT_FALSE(m_item->pressed());
    EXPECT_TRUE(event.isAccepted());
}

// mouseReleaseEvent emits pressedChanged when state transitions true→false
TEST_F(MouseTrackItemTest, MouseReleaseEvent_EmitsPressedChanged)
{
    m_item->setPressed(true);
    ASSERT_TRUE(m_item->pressed());

    QSignalSpy spy(m_item.get(), &MouseTrackItem::pressedChanged);
    ASSERT_TRUE(spy.isValid());

    QMouseEvent event(QEvent::MouseButtonRelease, QPointF(0, 0), QPointF(0, 0),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    m_item->mouseReleaseEvent(&event);

    EXPECT_EQ(spy.count(), 1);
}

// ─────────────────────────── mouseDoubleClickEvent ───────────────────────────

// B1: mouseDoubleClickEvent emits doubleClicked and accepts the event
TEST_F(MouseTrackItemTest, MouseDoubleClickEvent_EmitsSignal)
{
    QSignalSpy spy(m_item.get(), &MouseTrackItem::doubleClicked);
    ASSERT_TRUE(spy.isValid());

    QMouseEvent event(QEvent::MouseButtonDblClick, QPointF(0, 0), QPointF(0, 0),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    m_item->mouseDoubleClickEvent(&event);

    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(event.isAccepted());
}

// mouseDoubleClickEvent does not change pressed state
TEST_F(MouseTrackItemTest, MouseDoubleClickEvent_DoesNotChangePressed)
{
    m_item->setPressed(true);
    ASSERT_TRUE(m_item->pressed());

    QMouseEvent event(QEvent::MouseButtonDblClick, QPointF(0, 0), QPointF(0, 0),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    m_item->mouseDoubleClickEvent(&event);

    EXPECT_TRUE(m_item->pressed());
}
