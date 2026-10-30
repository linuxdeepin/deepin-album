// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QApplication>
#include <QQuickItem>
#include <QQuickWindow>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QEvent>
#include <QList>
#include <memory>

#include "thumbnailview/eventgenerator.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

ACCESS_PRIVATE_STATIC_FUN(EventGenerator, QList<QQuickItem *>(QQuickItem *), allChildItemsRecursive)

// Custom QQuickItem that records received event types
class RecordingQuickItem : public QQuickItem
{
    Q_OBJECT
public:
    QList<QEvent::Type> receivedEvents;

    explicit RecordingQuickItem(QQuickItem *parent = nullptr)
        : QQuickItem(parent)
    {
    }

protected:
    bool event(QEvent *e) override
    {
        receivedEvents.append(e->type());
        return QQuickItem::event(e);
    }
};

class EventGeneratorTest : public ::testing::Test
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

    std::unique_ptr<EventGenerator> m_gen;
    std::unique_ptr<QQuickWindow> m_window;

    void SetUp() override
    {
        m_gen = std::make_unique<EventGenerator>();
        m_window = std::make_unique<QQuickWindow>();
    }

    void TearDown() override
    {
        m_window.reset();
        m_gen.reset();
    }
};

// --- sendMouseEvent ---

TEST_F(EventGeneratorTest, SendMouseEvent_NullItem_NoCrash)
{
    m_gen->sendMouseEvent(nullptr, EventGenerator::MouseButtonPress, 0, 0,
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    SUCCEED();
}

TEST_F(EventGeneratorTest, SendMouseEvent_Press)
{
    auto *item = new RecordingQuickItem(m_window->contentItem());
    m_gen->sendMouseEvent(item, EventGenerator::MouseButtonPress, 10, 20,
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    ASSERT_FALSE(item->receivedEvents.isEmpty());
    EXPECT_EQ(item->receivedEvents.last(), QEvent::MouseButtonPress);
    delete item;
}

TEST_F(EventGeneratorTest, SendMouseEvent_Release)
{
    auto *item = new RecordingQuickItem(m_window->contentItem());
    m_gen->sendMouseEvent(item, EventGenerator::MouseButtonRelease, 10, 20,
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    ASSERT_FALSE(item->receivedEvents.isEmpty());
    EXPECT_EQ(item->receivedEvents.last(), QEvent::MouseButtonRelease);
    delete item;
}

TEST_F(EventGeneratorTest, SendMouseEvent_Move)
{
    auto *item = new RecordingQuickItem(m_window->contentItem());
    m_gen->sendMouseEvent(item, EventGenerator::MouseMove, 30, 40,
                          Qt::NoButton, Qt::NoButton, Qt::NoModifier);
    ASSERT_FALSE(item->receivedEvents.isEmpty());
    EXPECT_EQ(item->receivedEvents.last(), QEvent::MouseMove);
    delete item;
}

// --- sendMouseEventRecursive ---

TEST_F(EventGeneratorTest, SendMouseEventRecursive_NullParent_NoCrash)
{
    m_gen->sendMouseEventRecursive(nullptr, EventGenerator::MouseButtonPress, 0, 0,
                                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    SUCCEED();
}

TEST_F(EventGeneratorTest, SendMouseEventRecursive_AllChildrenReceiveEvent)
{
    auto *parent = new RecordingQuickItem(m_window->contentItem());
    auto *child1 = new RecordingQuickItem(parent);
    auto *child2 = new RecordingQuickItem(parent);
    auto *grandchild = new RecordingQuickItem(child1);

    m_gen->sendMouseEventRecursive(parent, EventGenerator::MouseButtonPress, 5, 5,
                                   Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // parent itself is NOT included (only children), but child1, child2, grandchild should be
    EXPECT_TRUE(child1->receivedEvents.contains(QEvent::MouseButtonPress));
    EXPECT_TRUE(child2->receivedEvents.contains(QEvent::MouseButtonPress));
    EXPECT_TRUE(grandchild->receivedEvents.contains(QEvent::MouseButtonPress));

    delete parent;
}

// --- sendWheelEvent ---

TEST_F(EventGeneratorTest, SendWheelEvent_NullItem_NoCrash)
{
    m_gen->sendWheelEvent(nullptr, 0, 0, QPoint(), QPoint(0, 120),
                          Qt::NoButton, Qt::NoModifier);
    SUCCEED();
}

TEST_F(EventGeneratorTest, SendWheelEvent_ItemWithoutWindow_NoCrash)
{
    // Item without a window — should return early
    RecordingQuickItem item;
    m_gen->sendWheelEvent(&item, 0, 0, QPoint(), QPoint(0, 120),
                          Qt::NoButton, Qt::NoModifier);
    EXPECT_TRUE(item.receivedEvents.isEmpty());
}

TEST_F(EventGeneratorTest, SendWheelEvent_WithWindow_ReceivesEvent)
{
    auto *item = new RecordingQuickItem(m_window->contentItem());
    m_gen->sendWheelEvent(item, 10, 10, QPoint(), QPoint(0, 120),
                          Qt::NoButton, Qt::NoModifier);
    ASSERT_FALSE(item->receivedEvents.isEmpty());
    EXPECT_EQ(item->receivedEvents.last(), QEvent::Wheel);
    delete item;
}

// --- sendWheelEventRecursive ---

TEST_F(EventGeneratorTest, SendWheelEventRecursive_NullParent_NoCrash)
{
    m_gen->sendWheelEventRecursive(nullptr, 0, 0, QPoint(), QPoint(0, 120),
                                   Qt::NoButton, Qt::NoModifier);
    SUCCEED();
}

TEST_F(EventGeneratorTest, SendWheelEventRecursive_ChildrenReceiveEvent)
{
    auto *parent = new RecordingQuickItem(m_window->contentItem());
    auto *child = new RecordingQuickItem(parent);

    m_gen->sendWheelEventRecursive(parent, 10, 10, QPoint(), QPoint(0, 120),
                                   Qt::NoButton, Qt::NoModifier);

    EXPECT_TRUE(child->receivedEvents.contains(QEvent::Wheel));
    delete parent;
}

// --- sendGrabEvent ---

TEST_F(EventGeneratorTest, SendGrabEvent_NullItem_NoCrash)
{
    m_gen->sendGrabEvent(nullptr, EventGenerator::GrabMouse);
    m_gen->sendGrabEvent(nullptr, EventGenerator::UngrabMouse);
    SUCCEED();
}

TEST_F(EventGeneratorTest, SendGrabEvent_UngrabMouse_ReceivesEvent)
{
    auto *item = new RecordingQuickItem(m_window->contentItem());
    m_gen->sendGrabEvent(item, EventGenerator::UngrabMouse);
    EXPECT_TRUE(item->receivedEvents.contains(QEvent::UngrabMouse));
    delete item;
}

TEST_F(EventGeneratorTest, SendGrabEvent_GrabMouse_NoCrash)
{
    auto *item = new RecordingQuickItem(m_window->contentItem());
    m_gen->sendGrabEvent(item, EventGenerator::GrabMouse);
    SUCCEED();
    delete item;
}

// --- sendGrabEventRecursive ---

TEST_F(EventGeneratorTest, SendGrabEventRecursive_NullParent_NoCrash)
{
    m_gen->sendGrabEventRecursive(nullptr, EventGenerator::GrabMouse);
    m_gen->sendGrabEventRecursive(nullptr, EventGenerator::UngrabMouse);
    SUCCEED();
}

TEST_F(EventGeneratorTest, SendGrabEventRecursive_ChildrenReceiveEvent)
{
    auto *parent = new RecordingQuickItem(m_window->contentItem());
    auto *child = new RecordingQuickItem(parent);

    m_gen->sendGrabEventRecursive(parent, EventGenerator::UngrabMouse);
    EXPECT_TRUE(child->receivedEvents.contains(QEvent::UngrabMouse));
    delete parent;
}

// --- allChildItemsRecursive (private) ---

TEST_F(EventGeneratorTest, AllChildItemsRecursive_NoChildren_EmptyList)
{
    auto *item = new RecordingQuickItem(m_window->contentItem());
    auto result = call_private_static_fun::EventGenerator::EventGeneratorallChildItemsRecursive(item);
    EXPECT_TRUE(result.isEmpty());
    delete item;
}

TEST_F(EventGeneratorTest, AllChildItemsRecursive_NestedChildren_ReturnsAll)
{
    auto *parent = new RecordingQuickItem(m_window->contentItem());
    auto *child1 = new RecordingQuickItem(parent);
    auto *child2 = new RecordingQuickItem(parent);
    auto *grandchild = new RecordingQuickItem(child1);

    auto result = call_private_static_fun::EventGenerator::EventGeneratorallChildItemsRecursive(parent);
    // Should contain child1, child2, grandchild (3 items, parent not included)
    EXPECT_EQ(result.size(), 3);
    EXPECT_TRUE(result.contains(child1));
    EXPECT_TRUE(result.contains(child2));
    EXPECT_TRUE(result.contains(grandchild));

    delete parent;
}

#include "test_EventGenerator.moc"
