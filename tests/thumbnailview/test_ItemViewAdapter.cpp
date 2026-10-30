// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QObject>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QPalette>
#include <QSize>
#include <QRect>
#include <QModelIndex>
#include <memory>

#include "thumbnailview/itemviewadapter.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

class ReceiverObject : public QObject
{
    Q_OBJECT
public:
    int scrollCallCount = 0;
    int iconSizeCallCount = 0;

public slots:
    void onScrolled() { ++scrollCallCount; }
    void onIconSizeChanged() { ++iconSizeCallCount; }
};

class ItemViewAdapterTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test";
            static char *argv[] = {arg0, nullptr};
            new QCoreApplication(argc, argv);
        }
    }

    static void TearDownTestSuite() {}

    StubExt stub;
    std::unique_ptr<ItemViewAdapter> m_adapter;
    std::unique_ptr<ReceiverObject> m_receiver;

    void SetUp() override
    {
        stub.clear();
        m_adapter = std::make_unique<ItemViewAdapter>();
        m_receiver = std::make_unique<ReceiverObject>();
    }

    void TearDown() override
    {
        m_receiver.reset();
        m_adapter.reset();
        stub.clear();
    }
};

// --- Constructor defaults ---

TEST_F(ItemViewAdapterTest, Constructor_Defaults)
{
    EXPECT_EQ(m_adapter->adapterView(), nullptr);
    EXPECT_EQ(m_adapter->adapterModel(), nullptr);
    EXPECT_EQ(m_adapter->adapterIconSize(), -1);
    EXPECT_EQ(m_adapter->model(), nullptr);
    EXPECT_EQ(m_adapter->iconSize(), QSize(-1, -1));
    EXPECT_EQ(m_adapter->adapterVisibleArea(), QRect());
    EXPECT_EQ(m_adapter->visibleArea(), QRect());
}

// --- setAdapterView / signal dedup ---

TEST_F(ItemViewAdapterTest, SetAdapterView_EmitsSignal)
{
    QSignalSpy spy(m_adapter.get(), &ItemViewAdapter::adapterViewChanged);
    QObject dummyView;
    m_adapter->setAdapterView(&dummyView);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(m_adapter->adapterView(), &dummyView);
}

TEST_F(ItemViewAdapterTest, SetAdapterView_SameValue_NoSignal)
{
    QObject dummyView;
    m_adapter->setAdapterView(&dummyView);
    QSignalSpy spy(m_adapter.get(), &ItemViewAdapter::adapterViewChanged);
    m_adapter->setAdapterView(&dummyView);
    EXPECT_EQ(spy.count(), 0);
}

// --- setAdapterModel / signal dedup ---

TEST_F(ItemViewAdapterTest, SetAdapterModel_EmitsSignal)
{
    QSignalSpy spy(m_adapter.get(), &ItemViewAdapter::adapterModelChanged);
    QStandardItemModel model;
    m_adapter->setAdapterModel(&model);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(m_adapter->adapterModel(), &model);
    EXPECT_EQ(m_adapter->model(), &model);
}

TEST_F(ItemViewAdapterTest, SetAdapterModel_SameValue_NoSignal)
{
    QStandardItemModel model;
    m_adapter->setAdapterModel(&model);
    QSignalSpy spy(m_adapter.get(), &ItemViewAdapter::adapterModelChanged);
    m_adapter->setAdapterModel(&model);
    EXPECT_EQ(spy.count(), 0);
}

// --- setAdapterIconSize / iconSize ---

TEST_F(ItemViewAdapterTest, SetAdapterIconSize_EmitsSignal)
{
    QSignalSpy spy(m_adapter.get(), &ItemViewAdapter::adapterIconSizeChanged);
    m_adapter->setAdapterIconSize(48);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(m_adapter->adapterIconSize(), 48);
    EXPECT_EQ(m_adapter->iconSize(), QSize(48, 48));
}

TEST_F(ItemViewAdapterTest, SetAdapterIconSize_SameValue_NoSignal)
{
    m_adapter->setAdapterIconSize(48);
    QSignalSpy spy(m_adapter.get(), &ItemViewAdapter::adapterIconSizeChanged);
    m_adapter->setAdapterIconSize(48);
    EXPECT_EQ(spy.count(), 0);
}

// --- setAdapterVisibleArea / visibleArea ---

TEST_F(ItemViewAdapterTest, SetAdapterVisibleArea_EmitsSignal)
{
    QSignalSpy spy(m_adapter.get(), &ItemViewAdapter::adapterVisibleAreaChanged);
    QRect rect(0, 0, 100, 200);
    m_adapter->setAdapterVisibleArea(rect);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(m_adapter->adapterVisibleArea(), rect);
    EXPECT_EQ(m_adapter->visibleArea(), rect);
}

TEST_F(ItemViewAdapterTest, SetAdapterVisibleArea_SameValue_NoSignal)
{
    QRect rect(0, 0, 100, 200);
    m_adapter->setAdapterVisibleArea(rect);
    QSignalSpy spy(m_adapter.get(), &ItemViewAdapter::adapterVisibleAreaChanged);
    m_adapter->setAdapterVisibleArea(rect);
    EXPECT_EQ(spy.count(), 0);
}

// --- connect() ---

TEST_F(ItemViewAdapterTest, Connect_ScrollBarValueChanged)
{
    m_adapter->connect(ItemViewAdapter::ScrollBarValueChanged, m_receiver.get(), SLOT(onScrolled()));
    emit m_adapter->viewScrolled();
    EXPECT_EQ(m_receiver->scrollCallCount, 1);
}

TEST_F(ItemViewAdapterTest, Connect_IconSizeChanged)
{
    m_adapter->connect(ItemViewAdapter::IconSizeChanged, m_receiver.get(), SLOT(onIconSizeChanged()));
    emit m_adapter->adapterIconSizeChanged();
    EXPECT_EQ(m_receiver->iconSizeCallCount, 1);
}

// --- palette ---

TEST_F(ItemViewAdapterTest, Palette_ReturnsDefaultPalette)
{
    QPalette defaultPalette;
    EXPECT_EQ(m_adapter->palette(), defaultPalette);
}

// --- visualRect ---

TEST_F(ItemViewAdapterTest, VisualRect_ReturnsEmptyRect)
{
    QStandardItemModel model;
    QModelIndex idx = model.index(0, 0);
    EXPECT_EQ(m_adapter->visualRect(idx), QRect());
}

#include "test_ItemViewAdapter.moc"
