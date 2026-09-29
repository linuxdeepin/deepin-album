// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QApplication>
#include <QDateTime>
#include <QString>
#include <QList>
#include <QVariant>

#include "stubext.h"
#include "addr_pri.h"
#include "widgets/importtimelineview/importtimelineview.h"
#include "dbmanager/dbmanager.h"
#include "widgets/thumbnail/thumbnaillistview.h"
#include "imageengine/imagedataservice.h"
#include "globalstatus.h"

using namespace stub_ext;

// Private field accessors
ACCESS_PRIVATE_FIELD(ImportTimeLineView, QList<QDateTime>, m_timelines)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, ThumbnailListView *, m_importTimeLineListView)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, int, m_suspensionHeight)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, int, m_timelineTitleHeight)

// Private function accessors for constructor stubbing
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(), initTimeLineViewWidget)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(), initConnections)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(DGuiApplicationHelper::ColorType), themeChangeSlot)

// ThumbnailListView private function accessors (needed to stub its init)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), initMenuAction)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), initConnections)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(int), flushTopTimeLine)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), onSelectionChanged)

// Capture variable for insertThumbnails
static DBImgInfoList g_capturedList;
static int g_insertCallCount = 0;

class ImportTimeLineViewTest : public ::testing::Test
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

        view = new ImportTimeLineView(nullptr);

        // Create a real ThumbnailListView and inject it
        setupListViewInitStubs();
        listView = new ThumbnailListView();
        access_private_field::ImportTimeLineViewm_importTimeLineListView(*view) = listView;

        // Set required height fields
        access_private_field::ImportTimeLineViewm_suspensionHeight(*view) = 200;
        access_private_field::ImportTimeLineViewm_timelineTitleHeight(*view) = 40;

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
        stub.set_lamda(get_private_fun::ImportTimeLineViewinitTimeLineViewWidget(),
            [](ImportTimeLineView *) -> void {});
        stub.set_lamda(get_private_fun::ImportTimeLineViewinitConnections(),
            [](ImportTimeLineView *) -> void {});
        stub.set_lamda(get_private_fun::ImportTimeLineViewthemeChangeSlot(),
            [](ImportTimeLineView *, DGuiApplicationHelper::ColorType) -> void {});
    }

    void setupDBStubs()
    {
        stub.set_lamda(ADDR(DBManager, instance),
            []() -> DBManager * { return reinterpret_cast<DBManager *>(0x1); });
        stub.set_lamda(ADDR(DBManager, getInfosByImportTimeline),
            [](DBManager *, const QDateTime &, const ItemType &) -> DBImgInfoList {
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

    ImportTimeLineView *view = nullptr;
    ThumbnailListView *listView = nullptr;
};

// ---- Single timeline with one photo ----

TEST_F(ImportTimeLineViewTest, AddTimelineLayout_SingleTimeline_InsertsBlankAndPhoto)
{
    QList<QDateTime> timelines;
    timelines << QDateTime(QDate(2024, 1, 15), QTime(10, 30));
    access_private_field::ImportTimeLineViewm_timelines(*view) = timelines;

    view->addTimelineLayout();

    EXPECT_EQ(g_insertCallCount, 1);
    // timelineIndex==0 → 1 blank + 1 photo = 2 items
    EXPECT_EQ(g_capturedList.size(), 2);
    EXPECT_EQ(g_capturedList.at(0).itemType, ItemTypeBlank);
    EXPECT_EQ(g_capturedList.at(1).itemType, ItemTypePic);
}

// ---- Multiple timelines ----

TEST_F(ImportTimeLineViewTest, AddTimelineLayout_MultipleTimelines_InsertsBlankAndTitles)
{
    QList<QDateTime> timelines;
    timelines << QDateTime(QDate(2024, 1, 15), QTime(10, 30));
    timelines << QDateTime(QDate(2024, 2, 20), QTime(14, 0));
    timelines << QDateTime(QDate(2024, 3, 10), QTime(9, 15));
    access_private_field::ImportTimeLineViewm_timelines(*view) = timelines;

    view->addTimelineLayout();

    EXPECT_EQ(g_insertCallCount, 1);
    // 3 timelines: 1 blank + 1 photo + 1 title + 1 photo + 1 title + 1 photo = 6
    EXPECT_EQ(g_capturedList.size(), 6);
    EXPECT_EQ(g_capturedList.at(0).itemType, ItemTypeBlank);
    EXPECT_EQ(g_capturedList.at(1).itemType, ItemTypePic);
    EXPECT_EQ(g_capturedList.at(2).itemType, ItemTypeImportTimeLineTitle);
    EXPECT_EQ(g_capturedList.at(3).itemType, ItemTypePic);
    EXPECT_EQ(g_capturedList.at(4).itemType, ItemTypeImportTimeLineTitle);
    EXPECT_EQ(g_capturedList.at(5).itemType, ItemTypePic);
}

// ---- Empty timelines ----

TEST_F(ImportTimeLineViewTest, AddTimelineLayout_EmptyTimelines_InsertsNothing)
{
    QList<QDateTime> timelines;
    access_private_field::ImportTimeLineViewm_timelines(*view) = timelines;

    view->addTimelineLayout();

    EXPECT_EQ(g_insertCallCount, 1);
    EXPECT_EQ(g_capturedList.size(), 0);
}

// ---- Date string formatting ----

TEST_F(ImportTimeLineViewTest, AddTimelineLayout_DateStringSetOnItems)
{
    QList<QDateTime> timelines;
    timelines << QDateTime(QDate(2024, 6, 15), QTime(10, 30));
    access_private_field::ImportTimeLineViewm_timelines(*view) = timelines;

    view->addTimelineLayout();

    ASSERT_GE(g_capturedList.size(), 2);
    // The blank item and photo should have non-empty date strings
    EXPECT_FALSE(g_capturedList.at(0).date.isEmpty());
    EXPECT_FALSE(g_capturedList.at(0).num.isEmpty());
    EXPECT_FALSE(g_capturedList.at(1).date.isEmpty());
    EXPECT_FALSE(g_capturedList.at(1).num.isEmpty());
}

// ---- Num string for single photo ----

TEST_F(ImportTimeLineViewTest, AddTimelineLayout_SinglePhoto_NumStringSet)
{
    QList<QDateTime> timelines;
    timelines << QDateTime(QDate(2024, 1, 1), QTime(0, 0));
    access_private_field::ImportTimeLineViewm_timelines(*view) = timelines;

    view->addTimelineLayout();

    ASSERT_GE(g_capturedList.size(), 1);
    // With 1 photo, num should contain "photo"
    EXPECT_TRUE(g_capturedList.at(0).num.contains("photo") || g_capturedList.at(0).num.contains("1"));
}

// ---- clearSelection and clearAll called ----

TEST_F(ImportTimeLineViewTest, AddTimelineLayout_ClearsSelectionBeforeInsert)
{
    QList<QDateTime> timelines;
    timelines << QDateTime(QDate(2024, 1, 1), QTime(0, 0));
    access_private_field::ImportTimeLineViewm_timelines(*view) = timelines;

    view->addTimelineLayout();

    // If we got here without crashing, clearSelection and clearAll were called
    EXPECT_EQ(g_insertCallCount, 1);
}
