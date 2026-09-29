// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QApplication>
#include <QString>
#include <QList>
#include <QVariant>
#include <DLabel>
#include <DCheckBox>

#include "stubext.h"
#include "addr_pri.h"
#include "widgets/timelineview/timelineview.h"
#include "dbmanager/dbmanager.h"
#include "widgets/thumbnail/thumbnaillistview.h"
#include "imageengine/imagedataservice.h"
#include "globalstatus.h"

using namespace stub_ext;

// Private field accessors
ACCESS_PRIVATE_FIELD(TimeLineView, QList<QString>, m_timelines)
ACCESS_PRIVATE_FIELD(TimeLineView, ThumbnailListView *, m_timeLineThumbnailListView)
ACCESS_PRIVATE_FIELD(TimeLineView, int, m_suspensionHeight)
ACCESS_PRIVATE_FIELD(TimeLineView, int, m_timelineTitleHeight)
ACCESS_PRIVATE_FIELD(TimeLineView, DLabel *, m_dateLabel)
ACCESS_PRIVATE_FIELD(TimeLineView, DLabel *, m_numLabel)
ACCESS_PRIVATE_FIELD(TimeLineView, DCheckBox *, m_numCheckBox)

// Private function accessors
ACCESS_PRIVATE_FUN(TimeLineView, void(), initTimeLineViewWidget)
ACCESS_PRIVATE_FUN(TimeLineView, void(), initConnections)
ACCESS_PRIVATE_FUN(TimeLineView, void(DGuiApplicationHelper::ColorType), themeChangeSlot)
ACCESS_PRIVATE_FUN(TimeLineView, void(), addTimelineLayout)

// ThumbnailListView private function accessors (needed to stub its init)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), initMenuAction)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), initConnections)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(int), flushTopTimeLine)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), onSelectionChanged)

// Capture variable for insertThumbnails
static DBImgInfoList g_capturedList;
static int g_insertCallCount = 0;

class TimeLineViewTest : public ::testing::Test
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

    StubExt stub;

    void SetUp() override
    {
        stub.clear();
        setupConstructorStubs();
        setupDBStubs();

        view = new TimeLineView(nullptr);

        // Create a real ThumbnailListView and inject it
        setupListViewInitStubs();
        listView = new ThumbnailListView();
        access_private_field::TimeLineViewm_timeLineThumbnailListView(*view) = listView;

        // Set required height fields
        access_private_field::TimeLineViewm_suspensionHeight(*view) = 200;
        access_private_field::TimeLineViewm_timelineTitleHeight(*view) = 40;

        // Create and inject label widgets (initTimeLineViewWidget is stubbed)
        access_private_field::TimeLineViewm_dateLabel(*view) = new DLabel(view);
        access_private_field::TimeLineViewm_numLabel(*view) = new DLabel(view);
        access_private_field::TimeLineViewm_numCheckBox(*view) = new DCheckBox(view);

        setupListViewStubs();

        g_capturedList.clear();
        g_insertCallCount = 0;
    }

    void TearDown() override
    {
        stub.clear();
        delete view;
        view = nullptr;
        // listView is a child of view, deleted by Qt
    }

    void setupConstructorStubs()
    {
        stub.set_lamda(get_private_fun::TimeLineViewinitTimeLineViewWidget(),
            [](TimeLineView *) -> void {});
        stub.set_lamda(get_private_fun::TimeLineViewinitConnections(),
            [](TimeLineView *) -> void {});
        stub.set_lamda(get_private_fun::TimeLineViewthemeChangeSlot(),
            [](TimeLineView *, DGuiApplicationHelper::ColorType) -> void {});
    }

    void setupDBStubs()
    {
        stub.set_lamda(ADDR(DBManager, instance),
            []() -> DBManager * { return reinterpret_cast<DBManager *>(0x1); });
        stub.set_lamda(ADDR(DBManager, getInfosByDay),
            [](DBManager *, const QString &) -> DBImgInfoList {
                DBImgInfoList list;
                DBImgInfo info;
                info.itemType = ItemTypePic;
                info.filePath = "/test/photo.jpg";
                list.append(info);
                return list;
            });
    }

    void setupListViewInitStubs()
    {
        stub.set_lamda(get_private_fun::ThumbnailListViewinitMenuAction(),
            [](ThumbnailListView *) -> void {});
        stub.set_lamda(get_private_fun::ThumbnailListViewinitConnections(),
            [](ThumbnailListView *) -> void {});
        stub.set_lamda(ADDR(ImageDataService, instance),
            [](QObject *) -> ImageDataService * { return nullptr; });
        stub.set_lamda(ADDR(GlobalStatus, instance),
            []() -> GlobalStatus * { return nullptr; });
        stub.set_lamda(get_private_fun::ThumbnailListViewflushTopTimeLine(),
            [](ThumbnailListView *, int) -> void {});
        stub.set_lamda(get_private_fun::ThumbnailListViewonSelectionChanged(),
            [](ThumbnailListView *) -> void {});
    }

    void setupListViewStubs()
    {
        stub.set_lamda(ADDR(ThumbnailListView, clearSelection),
            [](QAbstractItemView *) -> void {});
        stub.set_lamda(ADDR(ThumbnailListView, clearAll),
            [](ThumbnailListView *) -> void {});
        stub.set_lamda(ADDR(ThumbnailListView, insertThumbnails),
            [](ThumbnailListView *, const DBImgInfoList &infos) -> void {
                g_capturedList = infos;
                g_insertCallCount++;
            });
    }

    TimeLineView *view = nullptr;
    ThumbnailListView *listView = nullptr;
};

// ---- Single timeline with one photo ----

TEST_F(TimeLineViewTest, AddTimelineLayout_SingleTimeline_InsertsBlankAndPhoto)
{
    QList<QString> timelines;
    timelines << "2024-01-15";
    access_private_field::TimeLineViewm_timelines(*view) = timelines;

    call_private_fun::TimeLineViewaddTimelineLayout(*view);

    EXPECT_EQ(g_insertCallCount, 1);
    // timelineIndex==0 → 1 blank + 1 photo = 2 items
    EXPECT_EQ(g_capturedList.size(), 2);
    EXPECT_EQ(g_capturedList.at(0).itemType, ItemTypeBlank);
    EXPECT_EQ(g_capturedList.at(1).itemType, ItemTypePic);
}

// ---- Multiple timelines ----

TEST_F(TimeLineViewTest, AddTimelineLayout_MultipleTimelines_InsertsBlankAndTitles)
{
    QList<QString> timelines;
    timelines << "2024-01-15";
    timelines << "2024-02-20";
    timelines << "2024-03-10";
    access_private_field::TimeLineViewm_timelines(*view) = timelines;

    call_private_fun::TimeLineViewaddTimelineLayout(*view);

    EXPECT_EQ(g_insertCallCount, 1);
    // 3 timelines: 1 blank + 1 photo + 1 title + 1 photo + 1 title + 1 photo = 6
    EXPECT_EQ(g_capturedList.size(), 6);
    EXPECT_EQ(g_capturedList.at(0).itemType, ItemTypeBlank);
    EXPECT_EQ(g_capturedList.at(1).itemType, ItemTypePic);
    EXPECT_EQ(g_capturedList.at(2).itemType, ItemTypeTimeLineTitle);
    EXPECT_EQ(g_capturedList.at(3).itemType, ItemTypePic);
    EXPECT_EQ(g_capturedList.at(4).itemType, ItemTypeTimeLineTitle);
    EXPECT_EQ(g_capturedList.at(5).itemType, ItemTypePic);
}

// ---- Empty timelines ----

TEST_F(TimeLineViewTest, AddTimelineLayout_EmptyTimelines_InsertsNothing)
{
    QList<QString> timelines;
    access_private_field::TimeLineViewm_timelines(*view) = timelines;

    call_private_fun::TimeLineViewaddTimelineLayout(*view);

    EXPECT_EQ(g_insertCallCount, 1);
    EXPECT_EQ(g_capturedList.size(), 0);
}

// ---- Date string formatting ----

TEST_F(TimeLineViewTest, AddTimelineLayout_DateStringSetOnItems)
{
    QList<QString> timelines;
    timelines << "2024-06-15";
    access_private_field::TimeLineViewm_timelines(*view) = timelines;

    call_private_fun::TimeLineViewaddTimelineLayout(*view);

    ASSERT_GE(g_capturedList.size(), 2);
    // The blank item should have a formatted date string
    EXPECT_FALSE(g_capturedList.at(0).date.isEmpty());
    EXPECT_FALSE(g_capturedList.at(1).date.isEmpty());
}

// ---- Num string for single photo ----

TEST_F(TimeLineViewTest, AddTimelineLayout_SinglePhoto_NumStringSet)
{
    QList<QString> timelines;
    timelines << "2024-01-01";
    access_private_field::TimeLineViewm_timelines(*view) = timelines;

    call_private_fun::TimeLineViewaddTimelineLayout(*view);

    ASSERT_GE(g_capturedList.size(), 1);
    // With 1 photo, num should not be empty
    EXPECT_FALSE(g_capturedList.at(0).num.isEmpty());
}

// ---- Date with fewer than 3 parts (no valid date format) ----

TEST_F(TimeLineViewTest, AddTimelineLayout_InvalidDateFormat_DateEmpty)
{
    QList<QString> timelines;
    timelines << "invalid";
    access_private_field::TimeLineViewm_timelines(*view) = timelines;

    call_private_fun::TimeLineViewaddTimelineLayout(*view);

    // "invalid" has no "-" so datelist.count() <= 2, date stays empty
    ASSERT_GE(g_capturedList.size(), 1);
    EXPECT_TRUE(g_capturedList.at(0).date.isEmpty());
}
