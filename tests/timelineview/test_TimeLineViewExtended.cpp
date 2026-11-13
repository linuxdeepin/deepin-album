// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QApplication>
#include <QString>
#include <QList>
#include <QUrl>
#include <QMimeData>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QPaintEvent>
#include <QPaintEngine>
#include <QSignalSpy>
#include <DLabel>
#include <DCheckBox>
#include <DWidget>
#include <DPaletteHelper>
#include <DGuiApplicationHelper>
#include <DHiDPIHelper>

#include "stubext.h"
#include "addr_pri.h"
#include "widgets/timelineview/timelineview.h"
#include "dbmanager/dbmanager.h"
#include "widgets/thumbnail/thumbnaillistview.h"
#include "imageengine/imagedataservice.h"
#include "globalstatus.h"
#include "widgets/widgtes/expansionmenu.h"
#include "widgets/widgtes/noresultwidget.h"
#include "qmlWidget.h"
#include "unionimage/baseutils.h"

using namespace stub_ext;

// ===== Private field accessors =====
// Re-declare fields also used by the original file (each .cpp has its own
// anonymous namespace, so no ODR conflict)
ACCESS_PRIVATE_FIELD(TimeLineView, QList<QString>, m_timelines)
ACCESS_PRIVATE_FIELD(TimeLineView, ThumbnailListView *, m_timeLineThumbnailListView)
ACCESS_PRIVATE_FIELD(TimeLineView, int, m_suspensionHeight)
ACCESS_PRIVATE_FIELD(TimeLineView, int, m_timelineTitleHeight)
ACCESS_PRIVATE_FIELD(TimeLineView, DLabel *, m_dateLabel)
ACCESS_PRIVATE_FIELD(TimeLineView, DLabel *, m_numLabel)
ACCESS_PRIVATE_FIELD(TimeLineView, DCheckBox *, m_numCheckBox)
// Additional fields
ACCESS_PRIVATE_FIELD(TimeLineView, DWidget *, m_dateNumItemWidget)
ACCESS_PRIVATE_FIELD(TimeLineView, NoResultWidget *, m_noResultWidget)
ACCESS_PRIVATE_FIELD(TimeLineView, FilterWidget *, m_ToolButton)
ACCESS_PRIVATE_FIELD(TimeLineView, bool, m_hideBuiltinFilter)
ACCESS_PRIVATE_FIELD(TimeLineView, QMetaObject::Connection, m_filterTypeConn)
ACCESS_PRIVATE_FIELD(TimeLineView, QmlWidget *, m_qquickContainer)
ACCESS_PRIVATE_FIELD(TimeLineView, DWidget *, m_timeLineViewWidget)
ACCESS_PRIVATE_FIELD(TimeLineView, ExpansionMenu *, m_expansionMenu)
ACCESS_PRIVATE_FIELD(TimeLineView, QLayout *, m_mainLayout)
ACCESS_PRIVATE_FIELD(TimeLineView, QGraphicsOpacityEffect *, m_oe)
ACCESS_PRIVATE_FIELD(TimeLineView, QGraphicsOpacityEffect *, m_oet)

// ===== Private function accessors =====
// Re-declare functions also used by the original file
ACCESS_PRIVATE_FUN(TimeLineView, void(), initTimeLineViewWidget)
ACCESS_PRIVATE_FUN(TimeLineView, void(), initConnections)
ACCESS_PRIVATE_FUN(TimeLineView, void(DGuiApplicationHelper::ColorType), themeChangeSlot)
ACCESS_PRIVATE_FUN(TimeLineView, void(), addTimelineLayout)
// Additional functions
ACCESS_PRIVATE_FUN(TimeLineView, void(), clearAndStartLayout)
ACCESS_PRIVATE_FUN(TimeLineView, void(QString, QString, QString), slotTimeLineDataAndNum)
ACCESS_PRIVATE_FUN(TimeLineView, void(ExpansionPanel::FilteData &), sltCurrentFilterChanged)
ACCESS_PRIVATE_FUN(TimeLineView, void(bool), onShowCheckBox)
ACCESS_PRIVATE_FUN(TimeLineView, void(), initDropDown)
ACCESS_PRIVATE_FUN(TimeLineView, void(), updateSize)
ACCESS_PRIVATE_FUN(TimeLineView, void(), applyHeights)
ACCESS_PRIVATE_FUN(TimeLineView, void(QDragEnterEvent *), dragEnterEvent)
ACCESS_PRIVATE_FUN(TimeLineView, void(QDropEvent *), dropEvent)
ACCESS_PRIVATE_FUN(TimeLineView, void(QDragMoveEvent *), dragMoveEvent)
ACCESS_PRIVATE_FUN(TimeLineView, void(QDragLeaveEvent *), dragLeaveEvent)
ACCESS_PRIVATE_FUN(TimeLineView, void(QMouseEvent *), mousePressEvent)
ACCESS_PRIVATE_FUN(TimeLineView, void(QResizeEvent *), resizeEvent)

// ThumbnailListView private function accessors (needed to stub its init)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), initMenuAction)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), initConnections)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(int), flushTopTimeLine)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), onSelectionChanged)

// ===== Capture variables =====
static int g_clearSelectionCount = 0;
static int g_clearAllCount = 0;
static int g_insertCallCount = 0;
static int g_showAppointCallCount = 0;
static ItemType g_capturedShowType = ItemType::ItemTypeNull;
static int g_getAppointTypeItemCount_ret = 1;
static int g_floatBtnClickCount = 0;
static QString g_capturedFloatBtnDate;
static bool g_capturedFloatBtnSelect = false;
static int g_setFilterTypeCount = 0;
static int g_setFilterType_lastVal = -1;
static int g_focusCount = 0;
static DBImgInfoList g_capturedList;

class TimeLineViewExtendedTest : public ::testing::Test
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
        resetCounters();
        setupConstructorStubs();
        setupDBStubs();

        view = new TimeLineView(nullptr);

        setupListViewInitStubs();
        listView = new ThumbnailListView();
        access_private_field::TimeLineViewm_timeLineThumbnailListView(*view) = listView;

        access_private_field::TimeLineViewm_suspensionHeight(*view) = 200;
        access_private_field::TimeLineViewm_timelineTitleHeight(*view) = 40;

        access_private_field::TimeLineViewm_dateLabel(*view) = new DLabel(view);
        access_private_field::TimeLineViewm_numLabel(*view) = new DLabel(view);
        access_private_field::TimeLineViewm_numCheckBox(*view) = new DCheckBox(view);

        // Create m_dateNumItemWidget with child labels for on_AddLabel
        DWidget *dateNumWidget = new DWidget(view);
        new DLabel(dateNumWidget);
        new DLabel(dateNumWidget);
        access_private_field::TimeLineViewm_dateNumItemWidget(*view) = dateNumWidget;

        // Create m_noResultWidget
        access_private_field::TimeLineViewm_noResultWidget(*view) =
            new NoResultWidget(view);

        // Create m_ToolButton
        access_private_field::TimeLineViewm_ToolButton(*view) =
            new FilterWidget(view);

        // Create m_timeLineViewWidget (already created in constructor, but ensure it exists)
        if (!access_private_field::TimeLineViewm_timeLineViewWidget(*view)) {
            access_private_field::TimeLineViewm_timeLineViewWidget(*view) = new DWidget(view);
        }

        setupListViewStubs();
        setupAdditionalStubs();
    }

    void TearDown() override
    {
        stub.clear();
        delete view;
        view = nullptr;
    }

    void resetCounters()
    {
        g_clearSelectionCount = 0;
        g_clearAllCount = 0;
        g_insertCallCount = 0;
        g_showAppointCallCount = 0;
        g_capturedShowType = ItemType::ItemTypeNull;
        g_getAppointTypeItemCount_ret = 1;
        g_floatBtnClickCount = 0;
        g_capturedFloatBtnDate.clear();
        g_capturedFloatBtnSelect = false;
        g_setFilterTypeCount = 0;
        g_setFilterType_lastVal = -1;
        g_focusCount = 0;
        g_capturedList.clear();
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
            [](QAbstractItemView *) -> void { g_clearSelectionCount++; });
        stub.set_lamda(ADDR(ThumbnailListView, clearAll),
            [](ThumbnailListView *) -> void { g_clearAllCount++; });
        stub.set_lamda(ADDR(ThumbnailListView, insertThumbnails),
            [](ThumbnailListView *, const DBImgInfoList &infos) -> void {
                g_capturedList = infos;
                g_insertCallCount++;
            });
    }

    void setupAdditionalStubs()
    {
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
        stub.set_lamda(static_cast<void(QWidget::*)()>(&QWidget::setFocus),
            [](QWidget *) -> void { g_focusCount++; });
    }

    TimeLineView *view = nullptr;
    ThumbnailListView *listView = nullptr;
};

// ======================== getThumbnailListView ========================

TEST_F(TimeLineViewExtendedTest, GetThumbnailListView_ReturnsInjectedListView)
{
    ThumbnailListView *result = view->getThumbnailListView();
    EXPECT_EQ(result, listView);
}

TEST_F(TimeLineViewExtendedTest, GetThumbnailListView_ReturnsNullptrWhenNotSet)
{
    access_private_field::TimeLineViewm_timeLineThumbnailListView(*view) = nullptr;
    EXPECT_EQ(view->getThumbnailListView(), nullptr);
}

// ======================== clearAllSelection ========================

TEST_F(TimeLineViewExtendedTest, ClearAllSelection_CallsClearSelection)
{
    g_clearSelectionCount = 0;
    view->clearAllSelection();
    EXPECT_EQ(g_clearSelectionCount, 1);
}

// ======================== on_AddLabel ========================

TEST_F(TimeLineViewExtendedTest, OnAddLabel_SetsDateAndNumText)
{
    DWidget *dateNumWidget = access_private_field::TimeLineViewm_dateNumItemWidget(*view);
    ASSERT_NE(dateNumWidget, nullptr);
    QList<QLabel *> labelList = dateNumWidget->findChildren<QLabel *>();
    ASSERT_GE(labelList.size(), 2);

    view->on_AddLabel("2024-03-15", "10 photos");
    EXPECT_EQ(labelList[0]->text(), "2024-03-15");
    EXPECT_EQ(labelList[1]->text(), "10 photos");
    EXPECT_TRUE(dateNumWidget->isVisible());
}

TEST_F(TimeLineViewExtendedTest, OnAddLabel_NullDateNumWidget_NoCrash)
{
    access_private_field::TimeLineViewm_dateNumItemWidget(*view) = nullptr;
    EXPECT_NO_FATAL_FAILURE(view->on_AddLabel("2024-01-01", "5 photos"));
}

// ======================== onCheckBoxClicked ========================

TEST_F(TimeLineViewExtendedTest, OnCheckBoxClicked_Checked_PassesTrue)
{
    DLabel *dateLabel = access_private_field::TimeLineViewm_dateLabel(*view);
    dateLabel->setText("2024/01/15");
    DCheckBox *checkBox = access_private_field::TimeLineViewm_numCheckBox(*view);
    checkBox->setChecked(true);

    g_floatBtnClickCount = 0;
    view->onCheckBoxClicked();

    EXPECT_EQ(g_floatBtnClickCount, 1);
    EXPECT_EQ(g_capturedFloatBtnDate, "2024/01/15");
    EXPECT_TRUE(g_capturedFloatBtnSelect);
}

TEST_F(TimeLineViewExtendedTest, OnCheckBoxClicked_Unchecked_PassesFalse)
{
    DLabel *dateLabel = access_private_field::TimeLineViewm_dateLabel(*view);
    dateLabel->setText("2024/06/01");
    DCheckBox *checkBox = access_private_field::TimeLineViewm_numCheckBox(*view);
    checkBox->setChecked(false);

    g_floatBtnClickCount = 0;
    view->onCheckBoxClicked();

    EXPECT_EQ(g_floatBtnClickCount, 1);
    EXPECT_FALSE(g_capturedFloatBtnSelect);
}

// ======================== clearAndStartLayout ========================

TEST_F(TimeLineViewExtendedTest, ClearAndStartLayout_GetsDaysAndCallsAddTimelineLayout)
{
    stub.set_lamda(ADDR(DBManager, getDays),
        []() -> QStringList { return QStringList{"2024-01-15"}; });

    g_clearAllCount = 0;
    g_insertCallCount = 0;
    call_private_fun::TimeLineViewclearAndStartLayout(*view);

    EXPECT_EQ(g_clearAllCount, 1);
    EXPECT_EQ(g_insertCallCount, 1);

    QList<QString> timelines = access_private_field::TimeLineViewm_timelines(*view);
    EXPECT_EQ(timelines.size(), 1);
    EXPECT_EQ(timelines[0], "2024-01-15");
}

TEST_F(TimeLineViewExtendedTest, ClearAndStartLayout_WithContainerAppliesFilter)
{
    stub.set_lamda(ADDR(DBManager, getDays),
        []() -> QStringList { return QStringList{"2024-01-15"}; });
    stub.set_lamda(ADDR(QmlWidget, filterType),
        []() -> int { return 0; });
    stub.set_lamda(ADDR(QmlWidget, setFilterType),
        [](QmlWidget *, int ft) -> void {
            g_setFilterTypeCount++;
            g_setFilterType_lastVal = ft;
        });

    access_private_field::TimeLineViewm_qquickContainer(*view) =
        reinterpret_cast<QmlWidget *>(0x1234);

    g_clearSelectionCount = 0;
    g_showAppointCallCount = 0;
    call_private_fun::TimeLineViewclearAndStartLayout(*view);

    EXPECT_EQ(g_showAppointCallCount, 1);
    EXPECT_EQ(g_capturedShowType, ItemType::ItemTypeNull);
    EXPECT_EQ(g_clearSelectionCount, 1);
}

// ======================== slotTimeLineDataAndNum ========================

TEST_F(TimeLineViewExtendedTest, SlotTimeLineDataAndNum_WithNonEmptyData_SetsLabels)
{
    DLabel *dateLabel = access_private_field::TimeLineViewm_dateLabel(*view);
    DCheckBox *checkBox = access_private_field::TimeLineViewm_numCheckBox(*view);
    DLabel *numLabel = access_private_field::TimeLineViewm_numLabel(*view);

    view->slotTimeLineDataAndNum("2024/05/20", "3 photos", "Select");

    EXPECT_EQ(dateLabel->text(), "2024/05/20");
    EXPECT_EQ(checkBox->text(), "3 photos");
    EXPECT_EQ(numLabel->text(), "3 photos");
    EXPECT_FALSE(checkBox->isChecked());
}

TEST_F(TimeLineViewExtendedTest, SlotTimeLineDataAndNum_WithNonSelectText_ChecksBox)
{
    DCheckBox *checkBox = access_private_field::TimeLineViewm_numCheckBox(*view);

    view->slotTimeLineDataAndNum("2024/05/20", "3 photos", "Deselect");
    EXPECT_TRUE(checkBox->isChecked());
}

TEST_F(TimeLineViewExtendedTest, SlotTimeLineDataAndNum_WithEmptyData_DoesNotOverwrite)
{
    DLabel *dateLabel = access_private_field::TimeLineViewm_dateLabel(*view);
    dateLabel->setText("existing date");
    DCheckBox *checkBox = access_private_field::TimeLineViewm_numCheckBox(*view);
    checkBox->setText("existing num");

    view->slotTimeLineDataAndNum("", "", "Select");

    EXPECT_EQ(dateLabel->text(), "existing date");
    EXPECT_EQ(checkBox->text(), "existing num");
}

// ======================== sltCurrentFilterChanged ========================

TEST_F(TimeLineViewExtendedTest, SltCurrentFilterChanged_AllType_ShowAllItems)
{
    access_private_field::TimeLineViewm_hideBuiltinFilter(*view) = false;
    g_showAppointCallCount = 0;
    g_clearSelectionCount = 0;
    g_getAppointTypeItemCount_ret = 1;

    ExpansionPanel::FilteData data;
    data.type = ItemType::ItemTypeNull;

    call_private_fun::TimeLineViewsltCurrentFilterChanged(*view, data);

    EXPECT_EQ(g_showAppointCallCount, 1);
    EXPECT_EQ(g_capturedShowType, ItemType::ItemTypeNull);
    EXPECT_EQ(g_clearSelectionCount, 1);
    EXPECT_EQ(g_focusCount, 1);
}

TEST_F(TimeLineViewExtendedTest, SltCurrentFilterChanged_PicType_ShowPicItems)
{
    access_private_field::TimeLineViewm_hideBuiltinFilter(*view) = false;
    ExpansionPanel::FilteData data;
    data.type = ItemType::ItemTypePic;

    call_private_fun::TimeLineViewsltCurrentFilterChanged(*view, data);

    EXPECT_EQ(g_capturedShowType, ItemType::ItemTypePic);
}

TEST_F(TimeLineViewExtendedTest, SltCurrentFilterChanged_VideoType_ShowVideoItems)
{
    access_private_field::TimeLineViewm_hideBuiltinFilter(*view) = false;
    ExpansionPanel::FilteData data;
    data.type = ItemType::ItemTypeVideo;

    call_private_fun::TimeLineViewsltCurrentFilterChanged(*view, data);

    EXPECT_EQ(g_capturedShowType, ItemType::ItemTypeVideo);
}

TEST_F(TimeLineViewExtendedTest, SltCurrentFilterChanged_WithContainer_SetsFilterType)
{
    access_private_field::TimeLineViewm_qquickContainer(*view) =
        reinterpret_cast<QmlWidget *>(0x1234);
    stub.set_lamda(ADDR(QmlWidget, setFilterType),
        [](QmlWidget *, int ft) -> void {
            g_setFilterTypeCount++;
            g_setFilterType_lastVal = ft;
        });

    ExpansionPanel::FilteData data;
    data.type = ItemType::ItemTypePic;

    call_private_fun::TimeLineViewsltCurrentFilterChanged(*view, data);

    EXPECT_EQ(g_setFilterTypeCount, 1);
    EXPECT_EQ(g_setFilterType_lastVal, Types::Picture);
}

TEST_F(TimeLineViewExtendedTest, SltCurrentFilterChanged_HasItems_ToolButtonEnabled)
{
    access_private_field::TimeLineViewm_hideBuiltinFilter(*view) = false;
    FilterWidget *toolButton = access_private_field::TimeLineViewm_ToolButton(*view);
    toolButton->setEnabled(false);
    toolButton->setVisible(false);
    g_getAppointTypeItemCount_ret = 5;

    ExpansionPanel::FilteData data;
    data.type = ItemType::ItemTypePic;

    call_private_fun::TimeLineViewsltCurrentFilterChanged(*view, data);

    EXPECT_TRUE(toolButton->isEnabled());
    EXPECT_TRUE(toolButton->isVisible());
}

TEST_F(TimeLineViewExtendedTest, SltCurrentFilterChanged_NoItems_ToolButtonDisabled)
{
    access_private_field::TimeLineViewm_hideBuiltinFilter(*view) = false;
    FilterWidget *toolButton = access_private_field::TimeLineViewm_ToolButton(*view);
    toolButton->setEnabled(true);
    toolButton->setVisible(true);
    g_getAppointTypeItemCount_ret = 0;

    ExpansionPanel::FilteData data;
    data.type = ItemType::ItemTypePic;

    call_private_fun::TimeLineViewsltCurrentFilterChanged(*view, data);

    EXPECT_FALSE(toolButton->isEnabled());
    EXPECT_FALSE(toolButton->isVisible());
}

TEST_F(TimeLineViewExtendedTest, SltCurrentFilterChanged_HideBuiltinFilter_ToolButtonUntouched)
{
    access_private_field::TimeLineViewm_hideBuiltinFilter(*view) = true;
    FilterWidget *toolButton = access_private_field::TimeLineViewm_ToolButton(*view);
    toolButton->setEnabled(true);
    toolButton->setVisible(true);
    g_getAppointTypeItemCount_ret = 0;

    ExpansionPanel::FilteData data;
    data.type = ItemType::ItemTypePic;

    call_private_fun::TimeLineViewsltCurrentFilterChanged(*view, data);

    EXPECT_TRUE(toolButton->isEnabled());
    EXPECT_TRUE(toolButton->isVisible());
}

// ======================== slotNoPicOrNoVideo ========================

TEST_F(TimeLineViewExtendedTest, SlotNoPicOrNoVideo_True_HidesListViewShowsNoResult)
{
    NoResultWidget *noResultWidget = access_private_field::TimeLineViewm_noResultWidget(*view);
    noResultWidget->setVisible(false);
    listView->setVisible(true);

    DLabel *dateLabel = access_private_field::TimeLineViewm_dateLabel(*view);
    dateLabel->setText("2024/01/01");
    DCheckBox *checkBox = access_private_field::TimeLineViewm_numCheckBox(*view);
    checkBox->setText("5 photos");
    DLabel *numLabel = access_private_field::TimeLineViewm_numLabel(*view);
    numLabel->setText("5 photos");

    view->slotNoPicOrNoVideo(true);

    EXPECT_TRUE(noResultWidget->isVisible());
    EXPECT_FALSE(listView->isVisible());
    EXPECT_TRUE(dateLabel->text().isEmpty());
    EXPECT_TRUE(checkBox->text().isEmpty());
    EXPECT_TRUE(numLabel->text().isEmpty());
}

TEST_F(TimeLineViewExtendedTest, SlotNoPicOrNoVideo_False_ShowsListViewHidesNoResult)
{
    NoResultWidget *noResultWidget = access_private_field::TimeLineViewm_noResultWidget(*view);
    noResultWidget->setVisible(true);
    listView->setVisible(false);

    view->slotNoPicOrNoVideo(false);

    EXPECT_FALSE(noResultWidget->isVisible());
    EXPECT_TRUE(listView->isVisible());
}

// ======================== onShowCheckBox ========================

TEST_F(TimeLineViewExtendedTest, OnShowCheckBox_True_ShowsCheckBoxHidesNumLabel)
{
    DCheckBox *checkBox = access_private_field::TimeLineViewm_numCheckBox(*view);
    DLabel *numLabel = access_private_field::TimeLineViewm_numLabel(*view);

    view->onShowCheckBox(true);

    EXPECT_TRUE(checkBox->isVisible());
    EXPECT_FALSE(numLabel->isVisible());
}

TEST_F(TimeLineViewExtendedTest, OnShowCheckBox_False_HidesCheckBoxShowsNumLabel)
{
    DCheckBox *checkBox = access_private_field::TimeLineViewm_numCheckBox(*view);
    DLabel *numLabel = access_private_field::TimeLineViewm_numLabel(*view);

    view->onShowCheckBox(false);

    EXPECT_FALSE(checkBox->isVisible());
    EXPECT_TRUE(numLabel->isVisible());
}

// ======================== setHideBuiltinFilter ========================

TEST_F(TimeLineViewExtendedTest, SetHideBuiltinFilter_True_HidesToolButton)
{
    access_private_field::TimeLineViewm_hideBuiltinFilter(*view) = false;
    FilterWidget *toolButton = access_private_field::TimeLineViewm_ToolButton(*view);
    toolButton->setVisible(true);

    view->setHideBuiltinFilter(true);

    EXPECT_TRUE(access_private_field::TimeLineViewm_hideBuiltinFilter(*view));
    EXPECT_FALSE(toolButton->isVisible());
}

TEST_F(TimeLineViewExtendedTest, SetHideBuiltinFilter_False_ShowsToolButton)
{
    access_private_field::TimeLineViewm_hideBuiltinFilter(*view) = true;
    FilterWidget *toolButton = access_private_field::TimeLineViewm_ToolButton(*view);
    toolButton->setVisible(false);

    view->setHideBuiltinFilter(false);

    EXPECT_FALSE(access_private_field::TimeLineViewm_hideBuiltinFilter(*view));
    EXPECT_TRUE(toolButton->isVisible());
}

TEST_F(TimeLineViewExtendedTest, SetHideBuiltinFilter_SameValue_NoChange)
{
    access_private_field::TimeLineViewm_hideBuiltinFilter(*view) = true;
    FilterWidget *toolButton = access_private_field::TimeLineViewm_ToolButton(*view);
    toolButton->setVisible(false);

    view->setHideBuiltinFilter(true);

    EXPECT_FALSE(toolButton->isVisible());
}

TEST_F(TimeLineViewExtendedTest, SetHideBuiltinFilter_TrueWithContainer_EstablishesConnection)
{
    access_private_field::TimeLineViewm_hideBuiltinFilter(*view) = false;
    access_private_field::TimeLineViewm_qquickContainer(*view) =
        reinterpret_cast<QmlWidget *>(0x1234);

    QMetaObject::Connection conn = access_private_field::TimeLineViewm_filterTypeConn(*view);
    EXPECT_FALSE(conn);

    view->setHideBuiltinFilter(true);

    conn = access_private_field::TimeLineViewm_filterTypeConn(*view);
    EXPECT_TRUE(conn);
}

TEST_F(TimeLineViewExtendedTest, SetHideBuiltinFilter_TrueThenFalse_DisconnectsConnection)
{
    access_private_field::TimeLineViewm_qquickContainer(*view) =
        reinterpret_cast<QmlWidget *>(0x1234);

    view->setHideBuiltinFilter(true);
    QMetaObject::Connection conn = access_private_field::TimeLineViewm_filterTypeConn(*view);
    EXPECT_TRUE(conn);

    view->setHideBuiltinFilter(false);
    conn = access_private_field::TimeLineViewm_filterTypeConn(*view);
    EXPECT_FALSE(conn);
}

TEST_F(TimeLineViewExtendedTest, SetHideBuiltinFilter_TrueWithoutContainer_NoConnection)
{
    access_private_field::TimeLineViewm_qquickContainer(*view) = nullptr;
    access_private_field::TimeLineViewm_hideBuiltinFilter(*view) = false;

    view->setHideBuiltinFilter(true);

    QMetaObject::Connection conn = access_private_field::TimeLineViewm_filterTypeConn(*view);
    EXPECT_FALSE(conn);
}

// ======================== dragEnterEvent ========================

TEST_F(TimeLineViewExtendedTest, DragEnterEvent_ValidMime_AcceptsEvent)
{
    stub.set_lamda(ADDR(Libutils::base, checkMimeUrls),
        [](const QList<QUrl> &) -> bool { return true; });
    QMimeData mimeData;
    mimeData.setUrls({QUrl("file:///test.jpg")});
    QDragEnterEvent event(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);

    call_private_fun::TimeLineViewdragEnterEvent(*view, &event);

    EXPECT_TRUE(event.isAccepted());
    EXPECT_EQ(event.dropAction(), Qt::CopyAction);
}

TEST_F(TimeLineViewExtendedTest, DragEnterEvent_InvalidMime_RejectsEvent)
{
    stub.set_lamda(ADDR(Libutils::base, checkMimeUrls),
        [](const QList<QUrl> &) -> bool { return false; });
    QMimeData mimeData;
    mimeData.setUrls({QUrl("file:///test.jpg")});
    QDragEnterEvent event(QPoint(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);

    call_private_fun::TimeLineViewdragEnterEvent(*view, &event);

    EXPECT_FALSE(event.isAccepted());
}

// ======================== dragMoveEvent ========================

TEST_F(TimeLineViewExtendedTest, DragMoveEvent_AlwaysAccepts)
{
    QMimeData mimeData;
    QDragMoveEvent event(QPoint(10, 10), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);

    call_private_fun::TimeLineViewdragMoveEvent(*view, &event);

    EXPECT_TRUE(event.isAccepted());
}

// ======================== dragLeaveEvent ========================

TEST_F(TimeLineViewExtendedTest, DragLeaveEvent_NoCrash)
{
    QDragLeaveEvent event;
    EXPECT_NO_FATAL_FAILURE(call_private_fun::TimeLineViewdragLeaveEvent(*view, &event));
}

// ======================== dropEvent ========================

TEST_F(TimeLineViewExtendedTest, DropEvent_WithUrls_AcceptsEvent)
{
    QMimeData mimeData;
    mimeData.setUrls({QUrl("file:///photo1.jpg"), QUrl("file:///photo2.jpg")});
    QDropEvent event(QPointF(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);

    call_private_fun::TimeLineViewdropEvent(*view, &event);

    EXPECT_TRUE(event.isAccepted());
}

TEST_F(TimeLineViewExtendedTest, DropEvent_EmptyUrls_RejectsEvent)
{
    QMimeData mimeData;
    QDropEvent event(QPointF(0, 0), Qt::CopyAction, &mimeData, Qt::LeftButton, Qt::NoModifier);

    call_private_fun::TimeLineViewdropEvent(*view, &event);

    EXPECT_FALSE(event.isAccepted());
}

// ======================== mousePressEvent ========================

TEST_F(TimeLineViewExtendedTest, MousePressEvent_LeftButtonNoCtrl_ClearsSelection)
{
    g_clearSelectionCount = 0;
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(0, 0), QPointF(0, 0),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    call_private_fun::TimeLineViewmousePressEvent(*view, &event);

    EXPECT_EQ(g_clearSelectionCount, 1);
}

TEST_F(TimeLineViewExtendedTest, MousePressEvent_LeftButtonWithCtrl_DoesNotClearSelection)
{
    g_clearSelectionCount = 0;
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(0, 0), QPointF(0, 0),
                      Qt::LeftButton, Qt::LeftButton, Qt::ControlModifier);

    call_private_fun::TimeLineViewmousePressEvent(*view, &event);

    EXPECT_EQ(g_clearSelectionCount, 0);
}

TEST_F(TimeLineViewExtendedTest, MousePressEvent_RightButton_DoesNotClearSelection)
{
    g_clearSelectionCount = 0;
    QMouseEvent event(QEvent::MouseButtonPress, QPointF(0, 0), QPointF(0, 0),
                      Qt::RightButton, Qt::RightButton, Qt::NoModifier);

    call_private_fun::TimeLineViewmousePressEvent(*view, &event);

    EXPECT_EQ(g_clearSelectionCount, 0);
}

// ======================== resizeEvent ========================

TEST_F(TimeLineViewExtendedTest, ResizeEvent_UpdatesDateNumWidgetGeometry)
{
    view->resize(800, 600);
    access_private_field::TimeLineViewm_suspensionHeight(*view) = 200;
    DWidget *dateNumWidget = access_private_field::TimeLineViewm_dateNumItemWidget(*view);

    QResizeEvent event(QSize(800, 600), QSize(400, 300));
    call_private_fun::TimeLineViewresizeEvent(*view, &event);

    QRect geo = dateNumWidget->geometry();
    EXPECT_EQ(geo.x(), 0);
    EXPECT_EQ(geo.y(), 0);
    EXPECT_EQ(geo.width(), 800 - 15);
    EXPECT_EQ(geo.height(), 200);
}

// ======================== paintEngine ========================

TEST_F(TimeLineViewExtendedTest, PaintEngine_ReturnsNullptr)
{
    // paintEngine is public virtual in QWidget, overridden in TimeLineView.
    // Call through a base pointer to invoke the override.
    QWidget *w = view;
    QPaintEngine *engine = w->paintEngine();
    EXPECT_EQ(engine, nullptr);
}

// ======================== paintEvent ========================

TEST_F(TimeLineViewExtendedTest, PaintEvent_WithNullContainer_NoCrash)
{
    access_private_field::TimeLineViewm_qquickContainer(*view) = nullptr;

    QPaintEvent event(view->rect());
    EXPECT_NO_FATAL_FAILURE(view->repaint());
}

// ======================== themeChangeSlot ========================

TEST_F(TimeLineViewExtendedTest, ThemeChangeSlot_LightTheme_SetsForegroundRoles)
{
    // Clear all stubs to test real themeChangeSlot, then re-stub only what
    // the constructor chain needs (but not themeChangeSlot itself).
    stub.clear();
    setupDBStubs();
    setupListViewInitStubs();
    setupListViewStubs();
    setupAdditionalStubs();
    stub.set_lamda(get_private_fun::TimeLineViewinitTimeLineViewWidget(),
        [](TimeLineView *) -> void {});
    stub.set_lamda(get_private_fun::TimeLineViewinitConnections(),
        [](TimeLineView *) -> void {});

    DLabel *dateLabel = access_private_field::TimeLineViewm_dateLabel(*view);
    DCheckBox *checkBox = access_private_field::TimeLineViewm_numCheckBox(*view);
    DLabel *numLabel = access_private_field::TimeLineViewm_numLabel(*view);

    view->themeChangeSlot(DGuiApplicationHelper::LightType);

    EXPECT_EQ(dateLabel->foregroundRole(), QPalette::Text);
    EXPECT_EQ(checkBox->foregroundRole(), QPalette::Text);
    EXPECT_EQ(numLabel->foregroundRole(), QPalette::Text);
}

TEST_F(TimeLineViewExtendedTest, ThemeChangeSlot_DarkTheme_SetsForegroundRoles)
{
    stub.clear();
    setupDBStubs();
    setupListViewInitStubs();
    setupListViewStubs();
    setupAdditionalStubs();
    stub.set_lamda(get_private_fun::TimeLineViewinitTimeLineViewWidget(),
        [](TimeLineView *) -> void {});
    stub.set_lamda(get_private_fun::TimeLineViewinitConnections(),
        [](TimeLineView *) -> void {});

    DLabel *dateLabel = access_private_field::TimeLineViewm_dateLabel(*view);
    DCheckBox *checkBox = access_private_field::TimeLineViewm_numCheckBox(*view);
    DLabel *numLabel = access_private_field::TimeLineViewm_numLabel(*view);

    view->themeChangeSlot(DGuiApplicationHelper::DarkType);

    EXPECT_EQ(dateLabel->foregroundRole(), QPalette::Text);
    EXPECT_EQ(checkBox->foregroundRole(), QPalette::Text);
    EXPECT_EQ(numLabel->foregroundRole(), QPalette::Text);
}

// ======================== initConnections ========================

TEST_F(TimeLineViewExtendedTest, InitConnections_DoesNotCrash)
{
    stub.clear();
    setupDBStubs();
    setupListViewInitStubs();
    setupListViewStubs();
    setupAdditionalStubs();
    stub.set_lamda(get_private_fun::TimeLineViewinitTimeLineViewWidget(),
        [](TimeLineView *) -> void {});
    stub.set_lamda(get_private_fun::TimeLineViewthemeChangeSlot(),
        [](TimeLineView *, DGuiApplicationHelper::ColorType) -> void {});

    EXPECT_NO_FATAL_FAILURE(call_private_fun::TimeLineViewinitConnections(*view));
}

// ======================== updateSize ========================

TEST_F(TimeLineViewExtendedTest, UpdateSize_DelegatesToApplyHeights)
{
    access_private_field::TimeLineViewm_suspensionHeight(*view) = 0;
    access_private_field::TimeLineViewm_timelineTitleHeight(*view) = 0;

    call_private_fun::TimeLineViewupdateSize(*view);

    EXPECT_GT(access_private_field::TimeLineViewm_suspensionHeight(*view), 0);
    EXPECT_GT(access_private_field::TimeLineViewm_timelineTitleHeight(*view), 0);
}

// ======================== applyHeights ========================

TEST_F(TimeLineViewExtendedTest, ApplyHeights_SetsHeightFieldsAndItemGeometry)
{
    access_private_field::TimeLineViewm_suspensionHeight(*view) = 0;
    access_private_field::TimeLineViewm_timelineTitleHeight(*view) = 0;
    view->resize(800, 600);

    call_private_fun::TimeLineViewapplyHeights(*view);

    int suspensionHeight = access_private_field::TimeLineViewm_suspensionHeight(*view);
    int timelineTitleHeight = access_private_field::TimeLineViewm_timelineTitleHeight(*view);
    EXPECT_GT(suspensionHeight, 0);
    EXPECT_GT(timelineTitleHeight, 0);
    // timelineTitleHeight = suspensionHeight + ITEM_SPACING_COMPENSATION(4)
    EXPECT_EQ(timelineTitleHeight, suspensionHeight + 4);

    EXPECT_GT(listView->m_blankItemHeight, 0);

    DWidget *dateNumWidget = access_private_field::TimeLineViewm_dateNumItemWidget(*view);
    QRect geo = dateNumWidget->geometry();
    EXPECT_EQ(geo.width(), 800 - 15);
    EXPECT_EQ(geo.height(), suspensionHeight);
}

TEST_F(TimeLineViewExtendedTest, ApplyHeights_SetsLabelHeights)
{
    DLabel *dateLabel = access_private_field::TimeLineViewm_dateLabel(*view);
    DCheckBox *checkBox = access_private_field::TimeLineViewm_numCheckBox(*view);
    DLabel *numLabel = access_private_field::TimeLineViewm_numLabel(*view);

    call_private_fun::TimeLineViewapplyHeights(*view);

    EXPECT_GT(dateLabel->height(), 0);
    EXPECT_GT(checkBox->height(), 0);
    EXPECT_GT(numLabel->height(), 0);
}

// ======================== initDropDown ========================

TEST_F(TimeLineViewExtendedTest, InitDropDown_CreatesExpansionMenuAndToolButton)
{
    access_private_field::TimeLineViewm_ToolButton(*view) = nullptr;

    // Stub DHiDPIHelper::loadNxPixmap to avoid file loading issues
    stub.set_lamda(ADDR(DHiDPIHelper, loadNxPixmap),
        [](const QString &) -> QPixmap { return QPixmap(); });

    EXPECT_NO_FATAL_FAILURE(call_private_fun::TimeLineViewinitDropDown(*view));

    EXPECT_NE(access_private_field::TimeLineViewm_expansionMenu(*view), nullptr);
    EXPECT_NE(access_private_field::TimeLineViewm_ToolButton(*view), nullptr);
}

// ======================== initTimeLineViewWidget ========================

TEST_F(TimeLineViewExtendedTest, InitTimeLineViewWidget_CreatesWidgetsAndLayouts)
{
    stub.clear();
    setupDBStubs();
    setupListViewInitStubs();
    setupListViewStubs();
    setupAdditionalStubs();
    stub.set_lamda(get_private_fun::TimeLineViewinitConnections(),
        [](TimeLineView *) -> void {});
    stub.set_lamda(get_private_fun::TimeLineViewthemeChangeSlot(),
        [](TimeLineView *, DGuiApplicationHelper::ColorType) -> void {});

    if (!access_private_field::TimeLineViewm_timeLineViewWidget(*view)) {
        access_private_field::TimeLineViewm_timeLineViewWidget(*view) = new DWidget(view);
    }

    EXPECT_NO_FATAL_FAILURE(call_private_fun::TimeLineViewinitTimeLineViewWidget(*view));

    EXPECT_NE(access_private_field::TimeLineViewm_dateNumItemWidget(*view), nullptr);
    EXPECT_NE(access_private_field::TimeLineViewm_dateLabel(*view), nullptr);
    EXPECT_NE(access_private_field::TimeLineViewm_numCheckBox(*view), nullptr);
    EXPECT_NE(access_private_field::TimeLineViewm_noResultWidget(*view), nullptr);
}

// ======================== Opacity effects ========================

TEST_F(TimeLineViewExtendedTest, Constructor_CreatesOpacityEffectsWithCorrectValues)
{
    QGraphicsOpacityEffect *oe = access_private_field::TimeLineViewm_oe(*view);
    QGraphicsOpacityEffect *oet = access_private_field::TimeLineViewm_oet(*view);

    ASSERT_NE(oe, nullptr);
    ASSERT_NE(oet, nullptr);
    EXPECT_FLOAT_EQ(oe->opacity(), 0.5f);
    EXPECT_FLOAT_EQ(oet->opacity(), 0.75f);
}
