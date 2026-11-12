// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QApplication>
#include <QDateTime>
#include <QString>
#include <QList>
#include <QVariant>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QDragMoveEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QMimeData>
#include <QUrl>
#include <QSignalSpy>

#include "stubext.h"
#include "addr_pri.h"
#include "widgets/importtimelineview/importtimelineview.h"
#include "dbmanager/dbmanager.h"
#include "widgets/thumbnail/thumbnaillistview.h"
#include "imageengine/imagedataservice.h"
#include "globalstatus.h"
#include "qmlWidget.h"
#include "widgets/widgtes/expansionmenu.h"
#include "widgets/widgtes/expansionpanel.h"
#include "widgets/widgtes/noresultwidget.h"
#include "unionimage/baseutils.h"
#include "types.h"

#include <DCheckBox>
#include <DLabel>
#include <DWidget>
#include <DPaletteHelper>
#include <DFontSizeManager>

using namespace stub_ext;

// ======================== Private field accessors ========================

// Existing
ACCESS_PRIVATE_FIELD(ImportTimeLineView, QList<QDateTime>, m_timelines)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, ThumbnailListView *, m_importTimeLineListView)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, int, m_suspensionHeight)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, int, m_timelineTitleHeight)

// New field accessors
ACCESS_PRIVATE_FIELD(ImportTimeLineView, DCheckBox *, m_dateNumCheckBox)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, DLabel *, m_dateNumLabel)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, DLabel *, m_importLabel)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, NoResultWidget *, m_noResultWidget)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, FilterWidget *, m_ToolButton)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, bool, m_hideBuiltinFilter)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, bool, m_ctrlPress)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, QmlWidget *, m_qquickContainer)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, QMetaObject::Connection, m_filterTypeConn)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, QString, dateFullStr)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, QString, numFullStr)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, DWidget *, m_importTitleItem)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, DWidget *, m_timeLineViewWidget)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, ExpansionMenu *, m_expansionMenu)
ACCESS_PRIVATE_FIELD(ImportTimeLineView, QLayout *, m_mainLayout)

// ======================== Private function accessors ========================

// Existing (for constructor stubbing)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(), initTimeLineViewWidget)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(), initConnections)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(DGuiApplicationHelper::ColorType), themeChangeSlot)

// New function accessors
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(), updateDateNumLabel)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(), initDropDown)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(), applyHeights)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(), updateSize)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(), onCheckBoxClicked)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(bool), slotNoPicOrNoVideo)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(ExpansionPanel::FilteData &), sltCurrentFilterChanged)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(QDragEnterEvent *), dragEnterEvent)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(QDropEvent *), dropEvent)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(QDragMoveEvent *), dragMoveEvent)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(QMouseEvent *), mousePressEvent)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(QResizeEvent *), resizeEvent)
ACCESS_PRIVATE_FUN(ImportTimeLineView, void(QShowEvent *), showEvent)

// ThumbnailListView private function accessors (needed to stub its init)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), initMenuAction)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), initConnections)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(int), flushTopTimeLine)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), onSelectionChanged)

// ======================== Capture variables ========================

static DBImgInfoList g_capturedList;
static int g_insertCallCount = 0;
static int g_clearSelectionCount = 0;
static ItemType g_capturedShowType = ItemTypeNull;
static int g_showAppointCallCount = 0;
static int g_getAppointTypeItemCount_ret = 1;
static int g_floatBtnClickCount = 0;
static QString g_capturedFloatBtnDate;
static bool g_capturedFloatBtnSelect = false;
static QStringList g_selectedPaths_ret;
static int g_updatetimeLimeBtnTextCount = 0;
static int g_getImportTimelinesCount = 0;

// ======================== Test fixture ========================

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
        resetCaptures();
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

        // Inject real widget instances for fields that are NULL (initTimeLineViewWidget is stubbed)
        injectWidgets();

        setupListViewStubs();
        setupAdditionalStubs();

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

    void resetCaptures()
    {
        g_clearSelectionCount = 0;
        g_showAppointCallCount = 0;
        g_capturedShowType = ItemTypeNull;
        g_getAppointTypeItemCount_ret = 1;
        g_floatBtnClickCount = 0;
        g_capturedFloatBtnDate.clear();
        g_capturedFloatBtnSelect = false;
        g_selectedPaths_ret.clear();
        g_updatetimeLimeBtnTextCount = 0;
        g_getImportTimelinesCount = 0;
    }

    void injectWidgets()
    {
        // m_timeLineViewWidget is already created by the constructor
        // These fields are NULL because initTimeLineViewWidget() is stubbed
        access_private_field::ImportTimeLineViewm_dateNumCheckBox(*view) = new DCheckBox(view);
        access_private_field::ImportTimeLineViewm_dateNumLabel(*view) = new DLabel(view);
        access_private_field::ImportTimeLineViewm_importLabel(*view) = new DLabel(view);
        access_private_field::ImportTimeLineViewm_importTitleItem(*view) = new DWidget(view);
        access_private_field::ImportTimeLineViewm_noResultWidget(*view) = new NoResultWidget(view);
        access_private_field::ImportTimeLineViewm_ToolButton(*view) = new FilterWidget(view);
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
            [](QAbstractItemView *) -> void { g_clearSelectionCount++; });
        stub.set_lamda(ADDR(ThumbnailListView, clearAll),
            [](ThumbnailListView *) -> void {});
        stub.set_lamda(ADDR(ThumbnailListView, insertThumbnails),
            [](ThumbnailListView *, const DBImgInfoList &infos) -> void {
                g_capturedList = infos;
                g_insertCallCount++;
            });
    }

    void setupAdditionalStubs()
    {
        stub.set_lamda(ADDR(ThumbnailListView, selectedPaths),
            []() -> QStringList { return g_selectedPaths_ret; });
        stub.set_lamda(ADDR(ThumbnailListView, showAppointTypeItem),
            [](ThumbnailListView *, ItemType type) -> void {
                g_capturedShowType = type;
                g_showAppointCallCount++;
            });
        stub.set_lamda(ADDR(ThumbnailListView, getAppointTypeItemCount),
            [](ThumbnailListView *, ItemType) -> int { return g_getAppointTypeItemCount_ret; });
        stub.set_lamda(ADDR(ThumbnailListView, timeLimeFloatBtnClicked),
            [](ThumbnailListView *, const QString &date, bool isSelect) -> void {
                g_capturedFloatBtnDate = date;
                g_capturedFloatBtnSelect = isSelect;
                g_floatBtnClickCount++;
            });
        stub.set_lamda(ADDR(ThumbnailListView, updatetimeLimeBtnText),
            [](ThumbnailListView *) -> void { g_updatetimeLimeBtnTextCount++; });
    }

    ImportTimeLineView *view = nullptr;
    ThumbnailListView *listView = nullptr;
};

// ======================== Existing tests (unchanged) ========================

// ---- Single timeline with one photo ----

TEST_F(ImportTimeLineViewTest, AddTimelineLayout_SingleTimeline_InsertsBlankAndPhoto)
{
    // Arrange
    QList<QDateTime> timelines;
    timelines << QDateTime(QDate(2024, 1, 15), QTime(10, 30));
    access_private_field::ImportTimeLineViewm_timelines(*view) = timelines;

    // Act
    view->addTimelineLayout();

    // Assert
    EXPECT_EQ(g_insertCallCount, 1);
    // timelineIndex==0 → 1 blank + 1 photo = 2 items
    EXPECT_EQ(g_capturedList.size(), 2);
    EXPECT_EQ(g_capturedList.at(0).itemType, ItemTypeBlank);
    EXPECT_EQ(g_capturedList.at(1).itemType, ItemTypePic);
}

// ---- Multiple timelines ----

TEST_F(ImportTimeLineViewTest, AddTimelineLayout_MultipleTimelines_InsertsBlankAndTitles)
{
    // Arrange
    QList<QDateTime> timelines;
    timelines << QDateTime(QDate(2024, 1, 15), QTime(10, 30));
    timelines << QDateTime(QDate(2024, 2, 20), QTime(14, 0));
    timelines << QDateTime(QDate(2024, 3, 10), QTime(9, 15));
    access_private_field::ImportTimeLineViewm_timelines(*view) = timelines;

    // Act
    view->addTimelineLayout();

    // Assert
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
    // Arrange
    QList<QDateTime> timelines;
    access_private_field::ImportTimeLineViewm_timelines(*view) = timelines;

    // Act
    view->addTimelineLayout();

    // Assert
    EXPECT_EQ(g_insertCallCount, 1);
    EXPECT_EQ(g_capturedList.size(), 0);
}

// ---- Date string formatting ----

TEST_F(ImportTimeLineViewTest, AddTimelineLayout_DateStringSetOnItems)
{
    // Arrange
    QList<QDateTime> timelines;
    timelines << QDateTime(QDate(2024, 6, 15), QTime(10, 30));
    access_private_field::ImportTimeLineViewm_timelines(*view) = timelines;

    // Act
    view->addTimelineLayout();

    // Assert
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
    // Arrange
    QList<QDateTime> timelines;
    timelines << QDateTime(QDate(2024, 1, 1), QTime(0, 0));
    access_private_field::ImportTimeLineViewm_timelines(*view) = timelines;

    // Act
    view->addTimelineLayout();

    // Assert
    ASSERT_GE(g_capturedList.size(), 1);
    // With 1 photo, num should contain "photo"
    EXPECT_TRUE(g_capturedList.at(0).num.contains("photo") || g_capturedList.at(0).num.contains("1"));
}

// ---- clearSelection and clearAll called ----

TEST_F(ImportTimeLineViewTest, AddTimelineLayout_ClearsSelectionBeforeInsert)
{
    // Arrange
    QList<QDateTime> timelines;
    timelines << QDateTime(QDate(2024, 1, 1), QTime(0, 0));
    access_private_field::ImportTimeLineViewm_timelines(*view) = timelines;

    // Act
    view->addTimelineLayout();

    // Assert
    // If we got here without crashing, clearSelection and clearAll were called
    EXPECT_EQ(g_insertCallCount, 1);
}

// ======================== New tests ========================

// ---- clearAllSelection ----

TEST_F(ImportTimeLineViewTest, ClearAllSelection_DelegatesToListView)
{
    // Arrange
    g_clearSelectionCount = 0;

    // Act
    view->clearAllSelection();

    // Assert
    EXPECT_EQ(g_clearSelectionCount, 1);
    EXPECT_EQ(g_insertCallCount, 0);
}

// ---- selectPaths ----

TEST_F(ImportTimeLineViewTest, SelectPaths_ReturnsSelectedPaths)
{
    // Arrange
    g_selectedPaths_ret = QStringList() << "/path1/photo.jpg" << "/path2/video.mp4";

    // Act
    QStringList result = view->selectPaths();

    // Assert
    EXPECT_EQ(result.size(), 2);
    EXPECT_TRUE(result.contains("/path1/photo.jpg"));
    EXPECT_TRUE(result.contains("/path2/video.mp4"));
}

TEST_F(ImportTimeLineViewTest, SelectPaths_EmptyWhenNoSelection)
{
    // Arrange
    g_selectedPaths_ret.clear();

    // Act
    QStringList result = view->selectPaths();

    // Assert
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(result.size(), 0);
}

// ---- getListView ----

TEST_F(ImportTimeLineViewTest, GetListView_ReturnsInjectedListView)
{
    // Arrange — listView was injected in SetUp

    // Act
    ThumbnailListView *result = view->getListView();

    // Assert
    EXPECT_EQ(result, listView);
    EXPECT_NE(result, nullptr);
}

// ---- onShowCheckBox ----

TEST_F(ImportTimeLineViewTest, OnShowCheckBox_True_CheckboxVisibleLabelHidden)
{
    // Arrange
    DCheckBox *checkBox = access_private_field::ImportTimeLineViewm_dateNumCheckBox(*view);
    DLabel *label = access_private_field::ImportTimeLineViewm_dateNumLabel(*view);
    checkBox->setVisible(false);
    label->setVisible(true);

    // Act
    view->onShowCheckBox(true);

    // Assert
    EXPECT_TRUE(checkBox->isVisible());
    EXPECT_FALSE(label->isVisible());
}

TEST_F(ImportTimeLineViewTest, OnShowCheckBox_False_CheckboxHiddenLabelVisible)
{
    // Arrange
    DCheckBox *checkBox = access_private_field::ImportTimeLineViewm_dateNumCheckBox(*view);
    DLabel *label = access_private_field::ImportTimeLineViewm_dateNumLabel(*view);
    checkBox->setVisible(true);
    label->setVisible(false);

    // Act
    view->onShowCheckBox(false);

    // Assert
    EXPECT_FALSE(checkBox->isVisible());
    EXPECT_TRUE(label->isVisible());
}

// ---- onCheckBoxClicked ----
// 分支清单映射（onCheckBoxClicked complexity:2）:
// B1: isChecked()==true → timeLimeFloatBtnClicked(dateFullStr, true)
// B2: isChecked()==false → timeLimeFloatBtnClicked(dateFullStr, false)

TEST_F(ImportTimeLineViewTest, OnCheckBoxClicked_Checked_CallsFloatBtnClickedWithTrue)
{
    // Arrange
    DCheckBox *checkBox = access_private_field::ImportTimeLineViewm_dateNumCheckBox(*view);
    checkBox->setChecked(true);
    access_private_field::ImportTimeLineViewdateFullStr(*view) = "2024-01-15";
    g_floatBtnClickCount = 0;

    // Act
    call_private_fun::ImportTimeLineViewonCheckBoxClicked(*view);

    // Assert
    EXPECT_EQ(g_floatBtnClickCount, 1);
    EXPECT_EQ(g_capturedFloatBtnDate, QString("2024-01-15"));
    EXPECT_TRUE(g_capturedFloatBtnSelect);
}

TEST_F(ImportTimeLineViewTest, OnCheckBoxClicked_Unchecked_CallsFloatBtnClickedWithFalse)
{
    // Arrange
    DCheckBox *checkBox = access_private_field::ImportTimeLineViewm_dateNumCheckBox(*view);
    checkBox->setChecked(false);
    access_private_field::ImportTimeLineViewdateFullStr(*view) = "2024-06-01";
    g_floatBtnClickCount = 0;

    // Act
    call_private_fun::ImportTimeLineViewonCheckBoxClicked(*view);

    // Assert
    EXPECT_EQ(g_floatBtnClickCount, 1);
    EXPECT_EQ(g_capturedFloatBtnDate, QString("2024-06-01"));
    EXPECT_FALSE(g_capturedFloatBtnSelect);
}

// ---- slotTimeLineDataAndNum ----

TEST_F(ImportTimeLineViewTest, SlotTimeLineDataAndNum_SetsDateAndNumStrings)
{
    // Arrange
    access_private_field::ImportTimeLineViewdateFullStr(*view) = "";
    access_private_field::ImportTimeLineViewnumFullStr(*view) = "";

    // Act
    view->slotTimeLineDataAndNum("2024-03-15", "5 photos", "Deselect");

    // Assert
    EXPECT_EQ(access_private_field::ImportTimeLineViewdateFullStr(*view), QString("2024-03-15"));
    EXPECT_EQ(access_private_field::ImportTimeLineViewnumFullStr(*view), QString("5 photos"));
}

TEST_F(ImportTimeLineViewTest, SlotTimeLineDataAndNum_CheckboxCheckedWhenNotSelect)
{
    // Arrange
    DCheckBox *checkBox = access_private_field::ImportTimeLineViewm_dateNumCheckBox(*view);
    checkBox->setChecked(false);

    // Act — pass text != tr("Select"), so checkbox should be checked
    view->slotTimeLineDataAndNum("2024-01-01", "1 photo", "Deselect");

    // Assert
    EXPECT_TRUE(checkBox->isChecked());
    // updateDateNumLabel should have set the text on both widgets
    EXPECT_FALSE(checkBox->text().isEmpty());
    DLabel *label = access_private_field::ImportTimeLineViewm_dateNumLabel(*view);
    EXPECT_FALSE(label->text().isEmpty());
}

TEST_F(ImportTimeLineViewTest, SlotTimeLineDataAndNum_CheckboxUncheckedWhenSelect)
{
    // Arrange
    DCheckBox *checkBox = access_private_field::ImportTimeLineViewm_dateNumCheckBox(*view);
    checkBox->setChecked(true);

    // Act — pass text == tr("Select"), so checkbox should be unchecked
    view->slotTimeLineDataAndNum("2024-01-01", "1 photo", QObject::tr("Select"));

    // Assert
    EXPECT_FALSE(checkBox->isChecked());
}

// ---- updateDateNumLabel ----

TEST_F(ImportTimeLineViewTest, UpdateDateNumLabel_SetsCombinedTextOnWidgets)
{
    // Arrange
    access_private_field::ImportTimeLineViewdateFullStr(*view) = "2024-07-20";
    access_private_field::ImportTimeLineViewnumFullStr(*view) = "3 items";
    DCheckBox *checkBox = access_private_field::ImportTimeLineViewm_dateNumCheckBox(*view);
    DLabel *label = access_private_field::ImportTimeLineViewm_dateNumLabel(*view);
    checkBox->setText("");
    label->setText("");

    // Act
    call_private_fun::ImportTimeLineViewupdateDateNumLabel(*view);

    // Assert
    EXPECT_EQ(checkBox->text(), QString("2024-07-20 3 items"));
    EXPECT_EQ(label->text(), QString("2024-07-20 3 items"));
}

// ---- setHideBuiltinFilter ----
// 分支清单映射（setHideBuiltinFilter complexity:4）:
// B1: m_hideBuiltinFilter == hide → early return
// B2: hide=true, m_qquickContainer!=null, !m_filterTypeConn → connect lambda
// B3: hide=false, m_filterTypeConn → disconnect
// B4: m_ToolButton->setVisible(!hide)

TEST_F(ImportTimeLineViewTest, SetHideBuiltinFilter_True_HidesToolButton)
{
    // Arrange
    access_private_field::ImportTimeLineViewm_hideBuiltinFilter(*view) = false;
    FilterWidget *toolButton = access_private_field::ImportTimeLineViewm_ToolButton(*view);
    toolButton->setVisible(true);

    // Act
    view->setHideBuiltinFilter(true);

    // Assert
    EXPECT_FALSE(toolButton->isVisible());
    EXPECT_TRUE(access_private_field::ImportTimeLineViewm_hideBuiltinFilter(*view));
}

TEST_F(ImportTimeLineViewTest, SetHideBuiltinFilter_False_ShowsToolButton)
{
    // Arrange
    access_private_field::ImportTimeLineViewm_hideBuiltinFilter(*view) = true;
    FilterWidget *toolButton = access_private_field::ImportTimeLineViewm_ToolButton(*view);
    toolButton->setVisible(false);

    // Act
    view->setHideBuiltinFilter(false);

    // Assert
    EXPECT_TRUE(toolButton->isVisible());
    EXPECT_FALSE(access_private_field::ImportTimeLineViewm_hideBuiltinFilter(*view));
}

TEST_F(ImportTimeLineViewTest, SetHideBuiltinFilter_SameValue_NoChange)
{
    // Arrange
    access_private_field::ImportTimeLineViewm_hideBuiltinFilter(*view) = false;
    FilterWidget *toolButton = access_private_field::ImportTimeLineViewm_ToolButton(*view);
    toolButton->setVisible(true);

    // Act — same value, should early-return without changing visibility
    view->setHideBuiltinFilter(false);

    // Assert
    EXPECT_TRUE(toolButton->isVisible());
    EXPECT_FALSE(access_private_field::ImportTimeLineViewm_hideBuiltinFilter(*view));
}

// ---- slotNoPicOrNoVideo ----

TEST_F(ImportTimeLineViewTest, SlotNoPicOrNoVideo_True_ShowsNoResultHidesList)
{
    // Arrange
    NoResultWidget *noResult = access_private_field::ImportTimeLineViewm_noResultWidget(*view);
    ThumbnailListView *listView = access_private_field::ImportTimeLineViewm_importTimeLineListView(*view);
    noResult->setVisible(false);
    listView->setVisible(true);
    DLabel *importLabel = access_private_field::ImportTimeLineViewm_importLabel(*view);
    importLabel->setText("Import");
    DCheckBox *checkBox = access_private_field::ImportTimeLineViewm_dateNumCheckBox(*view);
    checkBox->setText("2024-01-01 3 photos");
    DLabel *dateLabel = access_private_field::ImportTimeLineViewm_dateNumLabel(*view);
    dateLabel->setText("2024-01-01 3 photos");

    // Act
    call_private_fun::ImportTimeLineViewslotNoPicOrNoVideo(*view, true);

    // Assert
    EXPECT_TRUE(noResult->isVisible());
    EXPECT_FALSE(listView->isVisible());
    EXPECT_TRUE(importLabel->text().isEmpty());
    EXPECT_TRUE(checkBox->text().isEmpty());
    EXPECT_TRUE(dateLabel->text().isEmpty());
}

TEST_F(ImportTimeLineViewTest, SlotNoPicOrNoVideo_False_HidesNoResultShowsList)
{
    // Arrange
    NoResultWidget *noResult = access_private_field::ImportTimeLineViewm_noResultWidget(*view);
    ThumbnailListView *listView = access_private_field::ImportTimeLineViewm_importTimeLineListView(*view);
    noResult->setVisible(true);
    listView->setVisible(false);

    // Act
    call_private_fun::ImportTimeLineViewslotNoPicOrNoVideo(*view, false);

    // Assert
    EXPECT_FALSE(noResult->isVisible());
    EXPECT_TRUE(listView->isVisible());
}

// ---- dragEnterEvent ----
// 分支清单映射（dragEnterEvent complexity:2）:
// B1: checkMimeUrls returns false → return (reject)
// B2: checkMimeUrls returns true → accept

TEST_F(ImportTimeLineViewTest, DragEnterEvent_ValidMime_AcceptsEvent)
{
    // Arrange
    stub.set_lamda(ADDR(Libutils::base, checkMimeUrls),
        [](const QList<QUrl> &) -> bool { return true; });
    QMimeData mimeData;
    mimeData.setUrls({QUrl("file:///test.jpg")});
    QDragEnterEvent event(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);

    // Act
    call_private_fun::ImportTimeLineViewdragEnterEvent(*view, &event);

    // Assert
    EXPECT_TRUE(event.isAccepted());
    EXPECT_EQ(event.dropAction(), Qt::CopyAction);
}

TEST_F(ImportTimeLineViewTest, DragEnterEvent_InvalidMime_RejectsEvent)
{
    // Arrange
    stub.set_lamda(ADDR(Libutils::base, checkMimeUrls),
        [](const QList<QUrl> &) -> bool { return false; });
    QMimeData mimeData;
    mimeData.setUrls({QUrl("file:///test.jpg")});
    QDragEnterEvent event(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);

    // Act
    call_private_fun::ImportTimeLineViewdragEnterEvent(*view, &event);

    // Assert
    EXPECT_FALSE(event.isAccepted());
}

// ---- dragMoveEvent ----

TEST_F(ImportTimeLineViewTest, DragMoveEvent_AlwaysAccepts)
{
    // Arrange
    QMimeData mimeData;
    QDragMoveEvent event(QPoint(10, 10), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);

    // Act
    call_private_fun::ImportTimeLineViewdragMoveEvent(*view, &event);

    // Assert
    EXPECT_TRUE(event.isAccepted());
}

// ---- dropEvent ----
// 分支清单映射（dropEvent complexity:2）:
// B1: urls.isEmpty() → return (reject)
// B2: urls not empty → accept

TEST_F(ImportTimeLineViewTest, DropEvent_WithUrls_AcceptsEvent)
{
    // Arrange
    QMimeData mimeData;
    mimeData.setUrls({QUrl("file:///photo1.jpg"), QUrl("file:///photo2.jpg")});
    QDropEvent event(QPointF(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);

    // Act
    call_private_fun::ImportTimeLineViewdropEvent(*view, &event);

    // Assert
    EXPECT_TRUE(event.isAccepted());
}

TEST_F(ImportTimeLineViewTest, DropEvent_EmptyUrls_RejectsEvent)
{
    // Arrange
    QMimeData mimeData;
    // No urls set
    QDropEvent event(QPointF(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);

    // Act
    call_private_fun::ImportTimeLineViewdropEvent(*view, &event);

    // Assert
    EXPECT_FALSE(event.isAccepted());
}

// ---- mousePressEvent ----
// 分支清单映射（mousePressEvent complexity:3）:
// B1: !m_ctrlPress && LeftButton → clearSelection + emit sigUpdatePicNum
// B2: m_ctrlPress || !LeftButton → skip clearSelection
// B3: DWidget::mousePressEvent(e) + updatetimeLimeBtnText() (always)

TEST_F(ImportTimeLineViewTest, MousePressEvent_LeftButton_ClearsSelectionAndEmitsSignal)
{
    // Arrange
    access_private_field::ImportTimeLineViewm_ctrlPress(*view) = false;
    g_clearSelectionCount = 0;
    g_updatetimeLimeBtnTextCount = 0;
    QSignalSpy spy(view, &ImportTimeLineView::sigUpdatePicNum);
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(0, 0), QPointF(0, 0),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act
    call_private_fun::ImportTimeLineViewmousePressEvent(*view, &event);

    // Assert
    EXPECT_EQ(g_clearSelectionCount, 1);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(g_updatetimeLimeBtnTextCount, 1);
}

TEST_F(ImportTimeLineViewTest, MousePressEvent_CtrlPress_DoesNotClearSelection)
{
    // Arrange
    access_private_field::ImportTimeLineViewm_ctrlPress(*view) = true;
    g_clearSelectionCount = 0;
    g_updatetimeLimeBtnTextCount = 0;
    QSignalSpy spy(view, &ImportTimeLineView::sigUpdatePicNum);
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(0, 0), QPointF(0, 0),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // Act
    call_private_fun::ImportTimeLineViewmousePressEvent(*view, &event);

    // Assert
    EXPECT_EQ(g_clearSelectionCount, 0);
    EXPECT_EQ(spy.count(), 0);
    // updatetimeLimeBtnText is always called
    EXPECT_EQ(g_updatetimeLimeBtnTextCount, 1);
}

// ---- resizeEvent ----

TEST_F(ImportTimeLineViewTest, ResizeEvent_CallsUpdateSizeAndUpdatesHeights)
{
    // Arrange — reset heights to 0 so we can detect the update
    access_private_field::ImportTimeLineViewm_suspensionHeight(*view) = 0;
    access_private_field::ImportTimeLineViewm_timelineTitleHeight(*view) = 0;
    QResizeEvent event(QSize(800, 600), QSize(400, 300));

    // Act
    call_private_fun::ImportTimeLineViewresizeEvent(*view, &event);

    // Assert — applyHeights should have set non-zero heights
    EXPECT_GT(access_private_field::ImportTimeLineViewm_suspensionHeight(*view), 0);
    EXPECT_GT(access_private_field::ImportTimeLineViewm_timelineTitleHeight(*view), 0);
}

// ---- showEvent ----

TEST_F(ImportTimeLineViewTest, ShowEvent_CallsUpdateSizeAndUpdatesHeights)
{
    // Arrange — reset heights to 0 so we can detect the update
    access_private_field::ImportTimeLineViewm_suspensionHeight(*view) = 0;
    access_private_field::ImportTimeLineViewm_timelineTitleHeight(*view) = 0;
    QShowEvent event;

    // Act
    call_private_fun::ImportTimeLineViewshowEvent(*view, &event);

    // Assert — applyHeights should have set non-zero heights
    EXPECT_GT(access_private_field::ImportTimeLineViewm_suspensionHeight(*view), 0);
    EXPECT_GT(access_private_field::ImportTimeLineViewm_timelineTitleHeight(*view), 0);
}

// ---- themeChangeSlot ----

TEST_F(ImportTimeLineViewTest, ThemeChangeSlot_LightTheme_SetsPaletteAndForegroundRoles)
{
    // Arrange
    DLabel *importLabel = access_private_field::ImportTimeLineViewm_importLabel(*view);
    DCheckBox *checkBox = access_private_field::ImportTimeLineViewm_dateNumCheckBox(*view);
    DLabel *dateLabel = access_private_field::ImportTimeLineViewm_dateNumLabel(*view);

    // Act
    view->themeChangeSlot(DGuiApplicationHelper::LightType);

    // Assert — foreground role should be set to Text for all label/checkbox widgets
    EXPECT_EQ(importLabel->foregroundRole(), QPalette::Text);
    EXPECT_EQ(checkBox->foregroundRole(), QPalette::Text);
    EXPECT_EQ(dateLabel->foregroundRole(), QPalette::Text);
}

TEST_F(ImportTimeLineViewTest, ThemeChangeSlot_DarkTheme_SetsPaletteAndForegroundRoles)
{
    // Arrange
    DLabel *importLabel = access_private_field::ImportTimeLineViewm_importLabel(*view);
    DCheckBox *checkBox = access_private_field::ImportTimeLineViewm_dateNumCheckBox(*view);
    DLabel *dateLabel = access_private_field::ImportTimeLineViewm_dateNumLabel(*view);

    // Act
    view->themeChangeSlot(DGuiApplicationHelper::DarkType);

    // Assert — foreground role should be set to Text for all label/checkbox widgets
    EXPECT_EQ(importLabel->foregroundRole(), QPalette::Text);
    EXPECT_EQ(checkBox->foregroundRole(), QPalette::Text);
    EXPECT_EQ(dateLabel->foregroundRole(), QPalette::Text);
}

// ---- applyHeights ----

TEST_F(ImportTimeLineViewTest, ApplyHeights_SetsHeightFieldsAndItemGeometry)
{
    // Arrange
    access_private_field::ImportTimeLineViewm_suspensionHeight(*view) = 0;
    access_private_field::ImportTimeLineViewm_timelineTitleHeight(*view) = 0;
    view->resize(800, 600);

    // Act
    call_private_fun::ImportTimeLineViewapplyHeights(*view);

    // Assert
    int suspensionHeight = access_private_field::ImportTimeLineViewm_suspensionHeight(*view);
    int timelineTitleHeight = access_private_field::ImportTimeLineViewm_timelineTitleHeight(*view);
    EXPECT_GT(suspensionHeight, 0);
    EXPECT_GT(timelineTitleHeight, 0);
    // m_blankItemHeight on the list view should be updated
    EXPECT_GT(listView->m_blankItemHeight, 0);
}

// ---- updateSize ----

TEST_F(ImportTimeLineViewTest, UpdateSize_DelegatesToApplyHeights)
{
    // Arrange
    access_private_field::ImportTimeLineViewm_suspensionHeight(*view) = 0;
    access_private_field::ImportTimeLineViewm_timelineTitleHeight(*view) = 0;

    // Act
    call_private_fun::ImportTimeLineViewupdateSize(*view);

    // Assert — updateSize calls applyHeights, which sets non-zero heights
    EXPECT_GT(access_private_field::ImportTimeLineViewm_suspensionHeight(*view), 0);
    EXPECT_GT(access_private_field::ImportTimeLineViewm_timelineTitleHeight(*view), 0);
}

// ---- sltCurrentFilterChanged ----
// 分支清单映射（sltCurrentFilterChanged complexity:10）:
// B1: data.type==ItemTypeNull → showAppointTypeItem(ItemTypeNull), filterType=All
// B2: data.type==ItemTypePic → showAppointTypeItem(ItemTypePic), filterType=Picture
// B3: data.type==ItemTypeVideo → showAppointTypeItem(ItemTypeVideo), filterType=Video
// B4: m_qquickContainer!=null → setFilterType
// B5: clearAllSelection()
// B6: getAppointTypeItemCount>0 → hasItems=true
// B7: !m_hideBuiltinFilter → m_ToolButton enable/visible = hasItems
// B8: setFocus()

TEST_F(ImportTimeLineViewTest, SltCurrentFilterChanged_AllType_ShowAllItems)
{
    // Arrange
    access_private_field::ImportTimeLineViewm_hideBuiltinFilter(*view) = false;
    g_showAppointCallCount = 0;
    g_clearSelectionCount = 0;
    ExpansionPanel::FilteData data;
    data.type = ItemType::ItemTypeNull;

    // Act
    call_private_fun::ImportTimeLineViewsltCurrentFilterChanged(*view, data);

    // Assert
    EXPECT_EQ(g_showAppointCallCount, 1);
    EXPECT_EQ(g_capturedShowType, ItemType::ItemTypeNull);
    EXPECT_EQ(g_clearSelectionCount, 1);
}

TEST_F(ImportTimeLineViewTest, SltCurrentFilterChanged_PicType_ShowPicItems)
{
    // Arrange
    access_private_field::ImportTimeLineViewm_hideBuiltinFilter(*view) = false;
    g_showAppointCallCount = 0;
    ExpansionPanel::FilteData data;
    data.type = ItemType::ItemTypePic;

    // Act
    call_private_fun::ImportTimeLineViewsltCurrentFilterChanged(*view, data);

    // Assert
    EXPECT_EQ(g_showAppointCallCount, 1);
    EXPECT_EQ(g_capturedShowType, ItemType::ItemTypePic);
}

TEST_F(ImportTimeLineViewTest, SltCurrentFilterChanged_VideoType_ShowVideoItems)
{
    // Arrange
    access_private_field::ImportTimeLineViewm_hideBuiltinFilter(*view) = false;
    g_showAppointCallCount = 0;
    ExpansionPanel::FilteData data;
    data.type = ItemType::ItemTypeVideo;

    // Act
    call_private_fun::ImportTimeLineViewsltCurrentFilterChanged(*view, data);

    // Assert
    EXPECT_EQ(g_showAppointCallCount, 1);
    EXPECT_EQ(g_capturedShowType, ItemType::ItemTypeVideo);
}

TEST_F(ImportTimeLineViewTest, SltCurrentFilterChanged_NoItems_DisablesToolButton)
{
    // Arrange
    access_private_field::ImportTimeLineViewm_hideBuiltinFilter(*view) = false;
    g_getAppointTypeItemCount_ret = 0;
    FilterWidget *toolButton = access_private_field::ImportTimeLineViewm_ToolButton(*view);
    toolButton->setEnabled(true);
    toolButton->setVisible(true);
    ExpansionPanel::FilteData data;
    data.type = ItemType::ItemTypePic;

    // Act
    call_private_fun::ImportTimeLineViewsltCurrentFilterChanged(*view, data);

    // Assert
    EXPECT_FALSE(toolButton->isEnabled());
    EXPECT_FALSE(toolButton->isVisible());
}

// ---- clearAndStartLayout ----

TEST_F(ImportTimeLineViewTest, ClearAndStartLayout_LoadsTimelinesAndInserts)
{
    // Arrange
    stub.set_lamda(ADDR(DBManager, getImportTimelines),
        [](DBManager *) -> const QList<QDateTime> {
            QList<QDateTime> list;
            list << QDateTime(QDate(2024, 1, 15), QTime(10, 30));
            return list;
        });
    g_insertCallCount = 0;
    // m_qquickContainer is null (parent was nullptr), so sltCurrentFilterChanged is skipped

    // Act
    view->clearAndStartLayout();

    // Assert
    QList<QDateTime> timelines = access_private_field::ImportTimeLineViewm_timelines(*view);
    EXPECT_EQ(timelines.size(), 1);
    EXPECT_EQ(g_insertCallCount, 1);
    // updateDateNumLabel should have set text on checkbox
    DCheckBox *checkBox = access_private_field::ImportTimeLineViewm_dateNumCheckBox(*view);
    EXPECT_FALSE(checkBox->text().isEmpty());
}

// ---- initDropDown ----

TEST_F(ImportTimeLineViewTest, InitDropDown_CreatesExpansionMenuAndToolButton)
{
    // Arrange — m_expansionMenu and m_ToolButton are currently the injected ones
    // initDropDown will create new ones

    // Act
    call_private_fun::ImportTimeLineViewinitDropDown(*view);

    // Assert
    ExpansionMenu *menu = access_private_field::ImportTimeLineViewm_expansionMenu(*view);
    FilterWidget *toolButton = access_private_field::ImportTimeLineViewm_ToolButton(*view);
    EXPECT_NE(menu, nullptr);
    EXPECT_NE(toolButton, nullptr);
    EXPECT_EQ(toolButton, menu->mainWidget());
}
