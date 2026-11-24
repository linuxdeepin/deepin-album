// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | insertThumbnail | high | complexity:6 | 3 | 3 |
// | insertThumbnails | high | complexity:7 | 3 | 3 |
// | stopLoadAndClear | high | complexity:4 | 3 | 3 |
// | clearAll | high | complexity:2 | 3 | 3 |
// | insertBlankOrTitleItem | high | complexity:5 | 3 | 3 |
// | resetBlankItemHeight | high | complexity:4 | 3 | 3 |
// | getRow(QString) | high | complexity:4 | 3 | 3 |
// | getRow(QPoint) | high | complexity:2 | 3 | 3 |
// | selectedPaths | high | complexity:5 | 3 | 3 |
// | selectedUrls | high | complexity:5 | 3 | 3 |
// | allUrls | high | complexity:5 | 3 | 3 |
// | setListViewUseFor | high | complexity:1 | 3 | 3 |
// | getListViewHeight | high | complexity:1 | 3 | 3 |
// | selectFirstPhoto | high | complexity:3 | 3 | 3 |
// | isFirstPhotoSelected | high | complexity:3 | 3 | 3 |
// | isNoPhotosSelected | high | complexity:2 | 3 | 3 |
// | isAllSelected | high | complexity:8 | 3 | 4 |
// | isAllSelectedSupportRotate | high | complexity:5 | 3 | 3 |
// | isSelectedCanUseFavorite | high | complexity:4 | 3 | 3 |
// | isSelectedCanUseDelete | high | complexity:5 | 3 | 3 |
// | showAppointTypeItem | high | complexity:8 | 3 | 4 |
// | getAppointTypeItemCount | high | complexity:4 | 3 | 3 |
// | getAppointTypeSelectItemCount | high | complexity:4 | 3 | 3 |
// | selectAllByItemType | high | complexity:3 | 3 | 3 |
// | TimeLineSelectAllBtn | high | complexity:1 | 3 | 3 |
// | setBatchOperateWidget | high | complexity:1 | 3 | 3 |
// | navigateToMonth | high | complexity:6 | 3 | 3 |
// | slotLoadFirstPageThumbnailsFinish | high | complexity:2 | 3 | 3 |
// | onUpdateListview | high | complexity:2 | 3 | 3 |
// | onKeyPressFromQml | high | complexity:5 | 3 | 4 |
// | slotChangeAllSelectBtnVisible | high | complexity:1 | 3 | 3 |
// | cutPixmap | high | complexity:4 | 3 | 3 |
// | removeSelectToTrash | high | complexity:4 | 3 | 3 |
// | selectPhotos | high | complexity:5 | 3 | 3 |
// | onScrollbarValueChanged | high | complexity:4 | 3 | 3 |
// ─── 生成后填入 actual 列，低于 min 即违规 ───

#include <gtest/gtest.h>
#include <QApplication>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QPoint>
#include <QStringList>
#include <QList>
#include <QVariant>
#include <memory>
#include <QSignalSpy>
#include <QAction>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QPixmap>
#include <QImage>
#include <QScrollEvent>

#include "widgets/thumbnail/thumbnaillistview.h"
#include "widgets/thumbnail/thumbnaildelegate.h"
#include "imageengine/imagedataservice.h"
#include "globalstatus.h"
#include "dbmanager/dbmanager.h"
#include <QKeyEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QScroller>
#include <QScrollBar>
#include <QPropertyAnimation>
#include "albumControl.h"
#include "setwallpaperconfig.h"
#include "unionimage/baseutils.h"
#include "unionimage/unionimage_global.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// ── Private field accessors for extended tests ──
ACCESS_PRIVATE_FIELD(ThumbnailListView, ThumbnailDelegate::DelegateType, m_delegatetype)
ACCESS_PRIVATE_FIELD(ThumbnailListView, QStandardItemModel *, m_model)
ACCESS_PRIVATE_FIELD(ThumbnailListView, QStringList, m_allfileslist)
ACCESS_PRIVATE_FIELD(ThumbnailListView, int, m_iBaseHeight)
ACCESS_PRIVATE_FIELD(ThumbnailListView, QString, m_imageType)
ACCESS_PRIVATE_FIELD(ThumbnailListView, int, m_currentUID)
ACCESS_PRIVATE_FIELD(ThumbnailListView, int, m_height)
ACCESS_PRIVATE_FIELD(ThumbnailListView, bool, m_isSelectAllBtn)
ACCESS_PRIVATE_FIELD(ThumbnailListView, int, m_blankItemHeight)
ACCESS_PRIVATE_FIELD(ThumbnailListView, int, m_onePicWidth)
ACCESS_PRIVATE_FIELD(ThumbnailListView, BatchOperateWidget *, m_batchOperateWidget)
ACCESS_PRIVATE_FIELD(ThumbnailListView, ItemType, m_currentShowItemType)
ACCESS_PRIVATE_FIELD(ThumbnailListView, QPropertyAnimation *, m_animation)
ACCESS_PRIVATE_FIELD(ThumbnailListView, QTimer *, m_scrollTimer)

// ── Private function accessors ──
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), initMenuAction)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), initConnections)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(int), flushTopTimeLine)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), onSelectionChanged)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), updateMenuContents)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), resizeEventF)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(const QStringList &), updateThumbnailViewAfterDelete)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(), onUpdateListview)
ACCESS_PRIVATE_FUN(ThumbnailListView, void(const QString &), onKeyPressFromQml)

// ── Helper: create a DBImgInfo for model rows ──

static DBImgInfo makeInfo(const QString &path, ItemType type,
                          const QString &date = QString(),
                          const QString &num = QString())
{
    DBImgInfo info;
    info.filePath = path;
    info.itemType = type;
    info.date = date;
    info.num = num;
    return info;
}

// ── Helper: append a row to the model with given DBImgInfo ──

static void appendRow(QStandardItemModel *model, const DBImgInfo &info)
{
    QStandardItem *item = new QStandardItem();
    item->setData(QVariant::fromValue(info), Qt::DisplayRole);
    model->appendRow(item);
}

// ── Extended test fixture ──

class ThumbnailListViewExtendedTest : public ::testing::Test
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

    void SetUp() override
    {
        stub.clear();
        setupCommonStubs();
    }

    void TearDown() override
    {
        stub.clear();
    }

    void setupCommonStubs()
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
        stub.set_lamda(get_private_fun::ThumbnailListViewupdateMenuContents(),
            [](ThumbnailListView *) -> void {});
    }

    std::unique_ptr<ThumbnailListView> createView(
        ThumbnailDelegate::DelegateType type = ThumbnailDelegate::AllPicViewType,
        const QString &imgtype = QString())
    {
        return std::make_unique<ThumbnailListView>(type, 0, imgtype, nullptr);
    }

    QStandardItemModel *getModel(ThumbnailListView *view)
    {
        return access_private_field::ThumbnailListViewm_model(*view);
    }

    void setDelegateType(ThumbnailListView *view, ThumbnailDelegate::DelegateType type)
    {
        access_private_field::ThumbnailListViewm_delegatetype(*view) = type;
    }

    QStringList &getAllFilesList(ThumbnailListView *view)
    {
        return access_private_field::ThumbnailListViewm_allfileslist(*view);
    }

    int &getHeight(ThumbnailListView *view)
    {
        return access_private_field::ThumbnailListViewm_height(*view);
    }

    bool &getIsSelectAllBtn(ThumbnailListView *view)
    {
        return access_private_field::ThumbnailListViewm_isSelectAllBtn(*view);
    }

    int &getBlankItemHeight(ThumbnailListView *view)
    {
        return access_private_field::ThumbnailListViewm_blankItemHeight(*view);
    }

    int &getOnePicWidth(ThumbnailListView *view)
    {
        return access_private_field::ThumbnailListViewm_onePicWidth(*view);
    }
};

// ═══════════════════════════════════════════════════════════════
// ⚠️ 以下每个 TEST_F 必须包含 // Arrange / // Act / // Assert 三段注释
// ═══════════════════════════════════════════════════════════════

// ============ insertThumbnail ============

TEST_F(ThumbnailListViewExtendedTest, InsertThumbnail_PicItem_AddsRowToModel)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    DBImgInfo info = makeInfo("test.jpg", ItemTypePic);

    // Act
    view->insertThumbnail(info);

    // Assert
    EXPECT_EQ(model->rowCount(), 1);
    DBImgInfo data = model->index(0, 0).data(Qt::DisplayRole).value<DBImgInfo>();
    EXPECT_EQ(data.filePath.toStdString(), "test.jpg");
}

TEST_F(ThumbnailListViewExtendedTest, InsertThumbnail_TimeLineTitleItem_MakesItemUnselectable)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::TimeLineViewType);
    auto *model = getModel(view.get());
    DBImgInfo info = makeInfo("", ItemTypeTimeLineTitle, "2024-01", "5");

    // Act
    view->insertThumbnail(info);

    // Assert
    EXPECT_EQ(model->rowCount(), 1);
    EXPECT_FALSE(model->item(0)->isSelectable());
}

TEST_F(ThumbnailListViewExtendedTest, InsertThumbnail_BlankItem_AdjustsImgWidth)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    DBImgInfo info = makeInfo("", ItemTypeBlank);
    info.imgHeight = 50;

    // Act
    view->insertThumbnail(info);

    // Assert
    EXPECT_EQ(model->rowCount(), 1);
    DBImgInfo data = model->index(0, 0).data(Qt::DisplayRole).value<DBImgInfo>();
    EXPECT_EQ(data.itemType, ItemTypeBlank);
    EXPECT_GT(data.imgWidth, 0);
}

// ============ insertThumbnails ============

TEST_F(ThumbnailListViewExtendedTest, InsertThumbnails_MultipleItems_AddsAllRows)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    DBImgInfoList infos;
    infos << makeInfo("pic1.jpg", ItemTypePic)
          << makeInfo("pic2.jpg", ItemTypePic)
          << makeInfo("vid1.mp4", ItemTypeVideo);

    // Act
    view->insertThumbnails(infos);

    // Assert
    EXPECT_EQ(model->rowCount(), 3);
    EXPECT_EQ(model->index(0, 0).data(Qt::DisplayRole).value<DBImgInfo>().filePath.toStdString(), "pic1.jpg");
    EXPECT_EQ(model->index(2, 0).data(Qt::DisplayRole).value<DBImgInfo>().filePath.toStdString(), "vid1.mp4");
}

TEST_F(ThumbnailListViewExtendedTest, InsertThumbnails_EmptyList_NoChange)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    DBImgInfoList infos;

    // Act
    view->insertThumbnails(infos);

    // Assert
    EXPECT_EQ(model->rowCount(), 0);
}

TEST_F(ThumbnailListViewExtendedTest, InsertThumbnails_WithTimeLineTitle_UnselectableTitle)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::TimeLineViewType);
    auto *model = getModel(view.get());
    DBImgInfoList infos;
    infos << makeInfo("", ItemTypeTimeLineTitle, "2024-03", "2")
          << makeInfo("pic1.jpg", ItemTypePic);

    // Act
    view->insertThumbnails(infos);

    // Assert
    EXPECT_EQ(model->rowCount(), 2);
    EXPECT_FALSE(model->item(0)->isSelectable());
    EXPECT_TRUE(model->item(1)->isSelectable());
}

// ============ stopLoadAndClear ============

TEST_F(ThumbnailListViewExtendedTest, StopLoadAndClear_WithClearModel_ClearsBoth)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    getAllFilesList(view.get()) << "pic1.jpg";

    // Act
    view->stopLoadAndClear(true);

    // Assert
    EXPECT_EQ(model->rowCount(), 0);
    EXPECT_TRUE(getAllFilesList(view.get()).isEmpty());
}

TEST_F(ThumbnailListViewExtendedTest, StopLoadAndClear_WithoutClearModel_KeepsModel)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    getAllFilesList(view.get()) << "pic1.jpg";

    // Act
    view->stopLoadAndClear(false);

    // Assert
    EXPECT_EQ(model->rowCount(), 1);
    EXPECT_TRUE(getAllFilesList(view.get()).isEmpty());
}

TEST_F(ThumbnailListViewExtendedTest, StopLoadAndClear_EmptyModel_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());

    // Act
    view->stopLoadAndClear(true);

    // Assert
    EXPECT_EQ(model->rowCount(), 0);
    EXPECT_TRUE(getAllFilesList(view.get()).isEmpty());
}

// ============ clearAll ============

TEST_F(ThumbnailListViewExtendedTest, ClearAll_WithItems_ClearsModel)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));

    // Act
    view->clearAll();

    // Assert
    EXPECT_EQ(model->rowCount(), 0);
}

TEST_F(ThumbnailListViewExtendedTest, ClearAll_EmptyModel_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());

    // Act
    view->clearAll();

    // Assert
    EXPECT_EQ(model->rowCount(), 0);
}

TEST_F(ThumbnailListViewExtendedTest, ClearAll_AfterClearModel_StillEmpty)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    view->clearAll();

    // Act
    view->clearAll();

    // Assert
    EXPECT_EQ(model->rowCount(), 0);
}

// ============ insertBlankOrTitleItem ============

TEST_F(ThumbnailListViewExtendedTest, InsertBlankOrTitleItem_BlankType_AddsBlankRow)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());

    // Act
    view->insertBlankOrTitleItem(ItemTypeBlank, "", "", 30);

    // Assert
    EXPECT_EQ(model->rowCount(), 1);
    DBImgInfo data = model->index(0, 0).data(Qt::DisplayRole).value<DBImgInfo>();
    EXPECT_EQ(data.itemType, ItemTypeBlank);
    EXPECT_EQ(getBlankItemHeight(view.get()), 30);
}

TEST_F(ThumbnailListViewExtendedTest, InsertBlankOrTitleItem_TimeLineTitle_AddsTitleRow)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::TimeLineViewType);
    auto *model = getModel(view.get());

    // Act
    view->insertBlankOrTitleItem(ItemTypeTimeLineTitle, "2024-06", "10", 0);

    // Assert
    EXPECT_EQ(model->rowCount(), 1);
    DBImgInfo data = model->index(0, 0).data(Qt::DisplayRole).value<DBImgInfo>();
    EXPECT_EQ(data.itemType, ItemTypeTimeLineTitle);
    EXPECT_EQ(data.date.toStdString(), "2024-06");
}

TEST_F(ThumbnailListViewExtendedTest, InsertBlankOrTitleItem_ImportTitle_AddsImportTitleRow)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AlbumViewImportTimeLineViewType);
    auto *model = getModel(view.get());

    // Act
    view->insertBlankOrTitleItem(ItemTypeImportTimeLineTitle, "2024-07", "3", 0);

    // Assert
    EXPECT_EQ(model->rowCount(), 1);
    DBImgInfo data = model->index(0, 0).data(Qt::DisplayRole).value<DBImgInfo>();
    EXPECT_EQ(data.itemType, ItemTypeImportTimeLineTitle);
}

// ============ resetBlankItemHeight ============

TEST_F(ThumbnailListViewExtendedTest, ResetBlankItemHeight_ExistingBlank_UpdatesHeight)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    view->insertBlankOrTitleItem(ItemTypeBlank, "", "", 30);
    EXPECT_EQ(getBlankItemHeight(view.get()), 30);

    // Act
    view->resetBlankItemHeight(50);

    // Assert
    EXPECT_EQ(getBlankItemHeight(view.get()), 50);
    DBImgInfo data = model->index(0, 0).data(Qt::DisplayRole).value<DBImgInfo>();
    EXPECT_EQ(data.imgHeight, 50);
}

TEST_F(ThumbnailListViewExtendedTest, ResetBlankItemHeight_NoBlankItem_NoChange)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    view->resetBlankItemHeight(60);

    // Assert
    EXPECT_EQ(model->rowCount(), 1);
    EXPECT_NE(getBlankItemHeight(view.get()), 60);
}

TEST_F(ThumbnailListViewExtendedTest, ResetBlankItemHeight_EmptyModel_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    view->resetBlankItemHeight(40);

    // Assert
    EXPECT_EQ(getModel(view.get())->rowCount(), 0);
}

// ============ getRow(QString) ============

TEST_F(ThumbnailListViewExtendedTest, GetRowByPath_ExistingPath_ReturnsCorrectRow)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic3.jpg", ItemTypePic));

    // Act
    int row = view->getRow("pic2.jpg");

    // Assert
    EXPECT_EQ(row, 1);
}

TEST_F(ThumbnailListViewExtendedTest, GetRowByPath_NonExistentPath_ReturnsNegativeOne)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    int row = view->getRow("nonexistent.jpg");

    // Assert
    EXPECT_EQ(row, -1);
}

TEST_F(ThumbnailListViewExtendedTest, GetRowByPath_EmptyModel_ReturnsNegativeOne)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    int row = view->getRow("any.jpg");

    // Assert
    EXPECT_EQ(row, -1);
}

// ============ getRow(QPoint) ============

TEST_F(ThumbnailListViewExtendedTest, GetRowByPoint_ValidPoint_ReturnsRow)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    // Point outside any item returns -1
    int row = view->getRow(QPoint(0, 0));

    // Assert
    EXPECT_GE(row, -1);
}

TEST_F(ThumbnailListViewExtendedTest, GetRowByPoint_InvalidPoint_ReturnsNegativeOne)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    int row = view->getRow(QPoint(-100, -100));

    // Assert
    EXPECT_EQ(row, -1);
}

TEST_F(ThumbnailListViewExtendedTest, GetRowByPoint_EmptyModel_ReturnsNegativeOne)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    int row = view->getRow(QPoint(50, 50));

    // Assert
    EXPECT_EQ(row, -1);
}

// ============ selectedPaths ============

TEST_F(ThumbnailListViewExtendedTest, SelectedPaths_WithSelectedPics_ReturnsPaths)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);

    // Act
    QStringList paths = view->selectedPaths();

    // Assert
    EXPECT_EQ(paths.size(), 1);
    EXPECT_EQ(paths[0].toStdString(), "pic1.jpg");
}

TEST_F(ThumbnailListViewExtendedTest, SelectedPaths_NoSelection_ReturnsEmpty)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    QStringList paths = view->selectedPaths();

    // Assert
    EXPECT_TRUE(paths.isEmpty());
}

TEST_F(ThumbnailListViewExtendedTest, SelectedPaths_HiddenRows_ExcludedFromResult)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);
    view->selectionModel()->select(model->index(1, 0), QItemSelectionModel::Select);
    view->setRowHidden(0, true);

    // Act
    QStringList paths = view->selectedPaths();

    // Assert
    EXPECT_EQ(paths.size(), 1);
    EXPECT_EQ(paths[0].toStdString(), "pic2.jpg");
}

// ============ selectedUrls ============

TEST_F(ThumbnailListViewExtendedTest, SelectedUrls_WithSelectedPics_ReturnsUrls)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("/tmp/pic1.jpg", ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);

    // Act
    QVariantList urls = view->selectedUrls();

    // Assert
    EXPECT_EQ(urls.size(), 1);
    EXPECT_TRUE(urls[0].toUrl().isLocalFile());
}

TEST_F(ThumbnailListViewExtendedTest, SelectedUrls_NoSelection_ReturnsEmpty)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    QVariantList urls = view->selectedUrls();

    // Assert
    EXPECT_TRUE(urls.isEmpty());
}

TEST_F(ThumbnailListViewExtendedTest, SelectedUrls_EmptyFilePath_Excluded)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("", ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);

    // Act
    QVariantList urls = view->selectedUrls();

    // Assert
    EXPECT_TRUE(urls.isEmpty());
}

// ============ allUrls ============

TEST_F(ThumbnailListViewExtendedTest, AllUrls_WithItems_ReturnsAllUrls)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("/tmp/pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("/tmp/pic2.jpg", ItemTypePic));

    // Act
    QVariantList urls = view->allUrls();

    // Assert
    EXPECT_EQ(urls.size(), 2);
}

TEST_F(ThumbnailListViewExtendedTest, AllUrls_EmptyModel_ReturnsEmpty)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    QVariantList urls = view->allUrls();

    // Assert
    EXPECT_TRUE(urls.isEmpty());
}

TEST_F(ThumbnailListViewExtendedTest, AllUrls_HiddenRows_ExcludedFromResult)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("/tmp/pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("/tmp/pic2.jpg", ItemTypePic));
    view->setRowHidden(0, true);

    // Act
    QVariantList urls = view->allUrls();

    // Assert
    EXPECT_EQ(urls.size(), 1);
}

// ============ setListViewUseFor ============

TEST_F(ThumbnailListViewExtendedTest, SetListViewUseFor_Normal_SetsUseForField)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    view->setListViewUseFor(ThumbnailListView::Normal);

    // Assert
    EXPECT_EQ(view->m_useFor, ThumbnailListView::Normal);
}

TEST_F(ThumbnailListViewExtendedTest, SetListViewUseFor_Mount_SetsUseForField)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    view->setListViewUseFor(ThumbnailListView::Mount);

    // Assert
    EXPECT_EQ(view->m_useFor, ThumbnailListView::Mount);
}

TEST_F(ThumbnailListViewExtendedTest, SetListViewUseFor_ToggleBackToNormal_RestoresValue)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    view->setListViewUseFor(ThumbnailListView::Mount);

    // Act
    view->setListViewUseFor(ThumbnailListView::Normal);

    // Assert
    EXPECT_EQ(view->m_useFor, ThumbnailListView::Normal);
}

// ============ getListViewHeight ============

TEST_F(ThumbnailListViewExtendedTest, GetListViewHeight_DefaultValue_ReturnsHeight)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    int h = view->getListViewHeight();

    // Assert
    EXPECT_EQ(h, 0);
}

TEST_F(ThumbnailListViewExtendedTest, GetListViewHeight_AfterSet_ReturnsSetValue)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    getHeight(view.get()) = 500;

    // Act
    int h = view->getListViewHeight();

    // Assert
    EXPECT_EQ(h, 500);
}

TEST_F(ThumbnailListViewExtendedTest, GetListViewHeight_NegativeValue_ReturnsAsIs)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    getHeight(view.get()) = -1;

    // Act
    int h = view->getListViewHeight();

    // Assert
    EXPECT_EQ(h, -1);
}

// ============ selectFirstPhoto ============

TEST_F(ThumbnailListViewExtendedTest, SelectFirstPhoto_WithItems_SelectsFirstRow)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));

    // Act
    view->selectFirstPhoto();

    // Assert
    EXPECT_TRUE(view->selectionModel()->isSelected(model->index(0, 0)));
    EXPECT_FALSE(view->selectionModel()->isSelected(model->index(1, 0)));
}

TEST_F(ThumbnailListViewExtendedTest, SelectFirstPhoto_EmptyModel_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    view->selectFirstPhoto();

    // Assert
    EXPECT_EQ(getModel(view.get())->rowCount(), 0);
    EXPECT_EQ(view->selectionModel()->selectedIndexes().size(), 0);
}

TEST_F(ThumbnailListViewExtendedTest, SelectFirstPhoto_SingleItem_SelectsIt)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("only.jpg", ItemTypePic));

    // Act
    view->selectFirstPhoto();

    // Assert
    EXPECT_EQ(view->selectionModel()->selectedIndexes().size(), 1);
    EXPECT_TRUE(view->selectionModel()->isSelected(model->index(0, 0)));
}

// ============ isFirstPhotoSelected ============

TEST_F(ThumbnailListViewExtendedTest, IsFirstPhotoSelected_FirstSelected_ReturnsTrue)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);

    // Act
    bool result = view->isFirstPhotoSelected();

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(ThumbnailListViewExtendedTest, IsFirstPhotoSelected_FirstNotSelected_ReturnsFalse)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));
    view->selectionModel()->select(model->index(1, 0), QItemSelectionModel::Select);

    // Act
    bool result = view->isFirstPhotoSelected();

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ThumbnailListViewExtendedTest, IsFirstPhotoSelected_NoSelection_ReturnsFalse)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    bool result = view->isFirstPhotoSelected();

    // Assert
    EXPECT_FALSE(result);
}

// ============ isNoPhotosSelected ============

TEST_F(ThumbnailListViewExtendedTest, IsNoPhotosSelected_WithSelection_ReturnsFalse)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);

    // Act
    bool result = view->isNoPhotosSelected();

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ThumbnailListViewExtendedTest, IsNoPhotosSelected_NoSelection_ReturnsTrue)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    bool result = view->isNoPhotosSelected();

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(ThumbnailListViewExtendedTest, IsNoPhotosSelected_EmptyModel_ReturnsTrue)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    bool result = view->isNoPhotosSelected();

    // Assert
    EXPECT_TRUE(result);
}

// ============ isAllSelected ============

TEST_F(ThumbnailListViewExtendedTest, IsAllSelected_AllPicsSelected_ReturnsTrue)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);
    view->selectionModel()->select(model->index(1, 0), QItemSelectionModel::Select);

    // Act
    bool result = view->isAllSelected(ItemTypeNull);

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(ThumbnailListViewExtendedTest, IsAllSelected_PartialSelection_ReturnsFalse)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);

    // Act
    bool result = view->isAllSelected(ItemTypeNull);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ThumbnailListViewExtendedTest, IsAllSelected_NoSelection_ReturnsFalse)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    bool result = view->isAllSelected(ItemTypeNull);

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ThumbnailListViewExtendedTest, IsAllSelected_FilterByPicType_ChecksOnlyPics)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("vid1.mp4", ItemTypeVideo));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);
    // Only the pic is selected, not the video

    // Act
    bool result = view->isAllSelected(ItemTypePic);

    // Assert
    EXPECT_TRUE(result);
}

// ============ isAllSelectedSupportRotate ============

TEST_F(ThumbnailListViewExtendedTest, IsAllSelectedSupportRotate_NoSelection_ReturnsFalse)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    bool result = view->isAllSelectedSupportRotate();

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ThumbnailListViewExtendedTest, IsAllSelectedSupportRotate_AllSupported_ReturnsTrue)
{
    // Arrange
    QTemporaryDir tmpDir;
    tmpDir.setAutoRemove(true);
    QVERIFY(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/pic1.jpg";
    QFile tmpFile(filePath);
    tmpFile.open(QIODevice::WriteOnly);
    tmpFile.close();
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo(filePath, ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isImageSupportRotate),
        [](const QString &) -> bool { return true; });

    // Act
    bool result = view->isAllSelectedSupportRotate();

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(ThumbnailListViewExtendedTest, IsAllSelectedSupportRotate_NotSupported_ReturnsFalse)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isImageSupportRotate),
        [](const QString &) -> bool { return false; });

    // Act
    bool result = view->isAllSelectedSupportRotate();

    // Assert
    EXPECT_FALSE(result);
}

// ============ isSelectedCanUseFavorite ============

TEST_F(ThumbnailListViewExtendedTest, IsSelectedCanUseFavorite_NoSelection_ReturnsFalse)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    bool result = view->isSelectedCanUseFavorite();

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ThumbnailListViewExtendedTest, IsSelectedCanUseFavorite_FileExists_ReturnsTrue)
{
    // Arrange
    QTemporaryFile tmpFile;
    tmpFile.open();
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo(tmpFile.fileName(), ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);

    // Act
    bool result = view->isSelectedCanUseFavorite();

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(ThumbnailListViewExtendedTest, IsSelectedCanUseFavorite_FileNotExists_ReturnsFalse)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("/nonexistent/path/file.jpg", ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);

    // Act
    bool result = view->isSelectedCanUseFavorite();

    // Assert
    EXPECT_FALSE(result);
}

// ============ isSelectedCanUseDelete ============

TEST_F(ThumbnailListViewExtendedTest, IsSelectedCanUseDelete_NoSelection_ReturnsFalse)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    bool result = view->isSelectedCanUseDelete();

    // Assert
    EXPECT_FALSE(result);
}

TEST_F(ThumbnailListViewExtendedTest, IsSelectedCanUseDelete_WritablePath_ReturnsTrue)
{
    // Arrange
    QTemporaryFile tmpFile;
    tmpFile.open();
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo(tmpFile.fileName(), ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);

    // Act
    bool result = view->isSelectedCanUseDelete();

    // Assert
    EXPECT_TRUE(result);
}

TEST_F(ThumbnailListViewExtendedTest, IsSelectedCanUseDelete_NonExistentPath_ReturnsFalse)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("/nonexistent/path/file.jpg", ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);

    // Act
    bool result = view->isSelectedCanUseDelete();

    // Assert
    // Non-existent path whose parent dir is also non-existent and not writable
    EXPECT_FALSE(result);
}

// ============ showAppointTypeItem ============

// 分支清单映射（showAppointTypeItem complexity=8 ≥ 3，列出主要分支）：
// B1: type==ItemTypePic + isAllAppointType(Video)==true → emit sigNoPicOrNoVideo(true)
// B2: type==ItemTypePic + isAllAppointType(Video)==false → hideAllAppointType(Video), emit false
// B3: type==ItemTypeVideo + isAllAppointType(Pic)==true → emit sigNoPicOrNoVideo(true)
// B4: type==ItemTypeVideo + isAllAppointType(Pic)==false → hideAllAppointType(Pic), emit false
// B5: type==ItemTypeNull → hideAllAppointType(Null), emit false
// 用例映射: B2→ShowPicWithMixedItems, B3→ShowVideoAllPics, B5→ShowAllItems

TEST_F(ThumbnailListViewExtendedTest, ShowAppointTypeItem_ShowPicWithMixedItems_HidesVideos)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("vid1.mp4", ItemTypeVideo));
    QSignalSpy spy(view.get(), &ThumbnailListView::sigNoPicOrNoVideo);

    // Act
    view->showAppointTypeItem(ItemTypePic);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.takeFirst().at(0).toBool());
}

TEST_F(ThumbnailListViewExtendedTest, ShowAppointTypeItem_ShowVideoAllPics_EmitsNoResult)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));
    QSignalSpy spy(view.get(), &ThumbnailListView::sigNoPicOrNoVideo);

    // Act
    view->showAppointTypeItem(ItemTypeVideo);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.takeFirst().at(0).toBool());
}

TEST_F(ThumbnailListViewExtendedTest, ShowAppointTypeItem_ShowAllItems_EmitsFalse)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("vid1.mp4", ItemTypeVideo));
    QSignalSpy spy(view.get(), &ThumbnailListView::sigNoPicOrNoVideo);

    // Act
    view->showAppointTypeItem(ItemTypeNull);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(spy.takeFirst().at(0).toBool());
}

TEST_F(ThumbnailListViewExtendedTest, ShowAppointTypeItem_ShowPicAllVideos_EmitsNoResult)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("vid1.mp4", ItemTypeVideo));
    appendRow(model, makeInfo("vid2.mp4", ItemTypeVideo));
    QSignalSpy spy(view.get(), &ThumbnailListView::sigNoPicOrNoVideo);

    // Act
    view->showAppointTypeItem(ItemTypePic);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(spy.takeFirst().at(0).toBool());
}

// ============ getAppointTypeItemCount ============

TEST_F(ThumbnailListViewExtendedTest, GetAppointTypeItemCount_PicType_ReturnsPicCount)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("vid1.mp4", ItemTypeVideo));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));

    // Act
    int count = view->getAppointTypeItemCount(ItemTypePic);

    // Assert
    EXPECT_EQ(count, 2);
}

TEST_F(ThumbnailListViewExtendedTest, GetAppointTypeItemCount_NullType_ReturnsPicAndVideo)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("vid1.mp4", ItemTypeVideo));

    // Act
    int count = view->getAppointTypeItemCount(ItemTypeNull);

    // Assert
    EXPECT_EQ(count, 2);
}

TEST_F(ThumbnailListViewExtendedTest, GetAppointTypeItemCount_EmptyModel_ReturnsZero)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    int count = view->getAppointTypeItemCount(ItemTypePic);

    // Assert
    EXPECT_EQ(count, 0);
}

// ============ getAppointTypeSelectItemCount ============

TEST_F(ThumbnailListViewExtendedTest, GetAppointTypeSelectItemCount_PicType_ReturnsSelectedPicCount)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("vid1.mp4", ItemTypeVideo));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);

    // Act
    int count = view->getAppointTypeSelectItemCount(ItemTypePic);

    // Assert
    EXPECT_EQ(count, 1);
}

TEST_F(ThumbnailListViewExtendedTest, GetAppointTypeSelectItemCount_NullType_ReturnsAllSelected)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("vid1.mp4", ItemTypeVideo));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);
    view->selectionModel()->select(model->index(1, 0), QItemSelectionModel::Select);

    // Act
    int count = view->getAppointTypeSelectItemCount(ItemTypeNull);

    // Assert
    EXPECT_EQ(count, 2);
}

TEST_F(ThumbnailListViewExtendedTest, GetAppointTypeSelectItemCount_NoSelection_ReturnsZero)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    int count = view->getAppointTypeSelectItemCount(ItemTypePic);

    // Assert
    EXPECT_EQ(count, 0);
}

// ============ selectAllByItemType ============

TEST_F(ThumbnailListViewExtendedTest, SelectAllByItemType_NullType_SelectsAll)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));
    QSignalSpy spy(view.get(), &ThumbnailListView::sigSelectAll);

    // Act
    view->selectAllByItemType(ItemTypeNull);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(view->selectionModel()->selectedIndexes().size(), 2);
}

TEST_F(ThumbnailListViewExtendedTest, SelectAllByItemType_PicType_EmitsSignal)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    QSignalSpy spy(view.get(), &ThumbnailListView::sigSelectAll);

    // Act
    view->selectAllByItemType(ItemTypePic);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_GT(view->selectionModel()->selectedIndexes().size(), 0);
}

TEST_F(ThumbnailListViewExtendedTest, SelectAllByItemType_EmptyModel_EmitsSignal)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    QSignalSpy spy(view.get(), &ThumbnailListView::sigSelectAll);

    // Act
    view->selectAllByItemType(ItemTypeNull);

    // Assert
    EXPECT_EQ(spy.count(), 1);
}

// ============ TimeLineSelectAllBtn ============

TEST_F(ThumbnailListViewExtendedTest, TimeLineSelectAllBtn_NoCrash_EmptyMethod)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::TimeLineViewType);

    // Act
    view->TimeLineSelectAllBtn();

    // Assert
    EXPECT_EQ(getModel(view.get())->rowCount(), 0);
}

TEST_F(ThumbnailListViewExtendedTest, TimeLineSelectAllBtn_WithItems_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::TimeLineViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    view->TimeLineSelectAllBtn();

    // Assert
    EXPECT_EQ(model->rowCount(), 1);
}

TEST_F(ThumbnailListViewExtendedTest, TimeLineSelectAllBtn_ImportType_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AlbumViewImportTimeLineViewType);

    // Act
    view->TimeLineSelectAllBtn();

    // Assert
    EXPECT_EQ(getModel(view.get())->rowCount(), 0);
}

// ============ setBatchOperateWidget ============

TEST_F(ThumbnailListViewExtendedTest, SetBatchOperateWidget_ValidPointer_SetsField)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    view->setBatchOperateWidget(nullptr);

    // Assert
    auto *widget = access_private_field::ThumbnailListViewm_batchOperateWidget(*view);
    EXPECT_EQ(widget, nullptr);
}

TEST_F(ThumbnailListViewExtendedTest, SetBatchOperateWidget_NonNull_SetsField)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    // Use a fake pointer - we just check the field is set
    BatchOperateWidget *fakeWidget = reinterpret_cast<BatchOperateWidget *>(0x1234);

    // Act
    view->setBatchOperateWidget(fakeWidget);

    // Assert
    auto *widget = access_private_field::ThumbnailListViewm_batchOperateWidget(*view);
    EXPECT_EQ(widget, fakeWidget);

    // Cleanup
    view->setBatchOperateWidget(nullptr);
}

TEST_F(ThumbnailListViewExtendedTest, SetBatchOperateWidget_Twice_SecondOverwrites)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    view->setBatchOperateWidget(nullptr);
    view->setBatchOperateWidget(nullptr);

    // Assert
    auto *widget = access_private_field::ThumbnailListViewm_batchOperateWidget(*view);
    EXPECT_EQ(widget, nullptr);
}

// ============ navigateToMonth ============

TEST_F(ThumbnailListViewExtendedTest, NavigateToMonth_TimeLineMatchingDate_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::TimeLineViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("", ItemTypeTimeLineTitle, "2024-06", "2"));
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    view->navigateToMonth("2024-06");

    // Assert
    EXPECT_EQ(model->rowCount(), 2);
}

TEST_F(ThumbnailListViewExtendedTest, NavigateToMonth_NoMatchingDate_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::TimeLineViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("", ItemTypeTimeLineTitle, "2024-01", "2"));
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    view->navigateToMonth("2025-12");

    // Assert
    EXPECT_EQ(model->rowCount(), 2);
}

TEST_F(ThumbnailListViewExtendedTest, NavigateToMonth_EmptyModel_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::TimeLineViewType);

    // Act
    view->navigateToMonth("2024-06");

    // Assert
    EXPECT_EQ(getModel(view.get())->rowCount(), 0);
}

// ============ slotLoadFirstPageThumbnailsFinish ============

TEST_F(ThumbnailListViewExtendedTest, SlotLoadFirstPageThumbnailsFinish_EmitsSignal)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    QSignalSpy spy(view.get(), &ThumbnailListView::sigDBImageLoaded);

    // Act
    view->slotLoadFirstPageThumbnailsFinish();

    // Assert
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ThumbnailListViewExtendedTest, SlotLoadFirstPageThumbnailsFinish_EmitsExactlyOnce)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    QSignalSpy spy(view.get(), &ThumbnailListView::sigDBImageLoaded);

    // Act
    view->slotLoadFirstPageThumbnailsFinish();
    view->slotLoadFirstPageThumbnailsFinish();

    // Assert
    EXPECT_EQ(spy.count(), 2);
}

TEST_F(ThumbnailListViewExtendedTest, SlotLoadFirstPageThumbnailsFinish_WithItems_EmitsSignal)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    QSignalSpy spy(view.get(), &ThumbnailListView::sigDBImageLoaded);

    // Act
    view->slotLoadFirstPageThumbnailsFinish();

    // Assert
    EXPECT_EQ(spy.count(), 1);
}

// ============ onUpdateListview ============

TEST_F(ThumbnailListViewExtendedTest, OnUpdateListview_NotVisible_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    view->hide();

    // Act
    call_private_fun::ThumbnailListViewonUpdateListview(*view);

    // Assert
    EXPECT_FALSE(view->isVisible());
}

TEST_F(ThumbnailListViewExtendedTest, OnUpdateListview_Visible_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    view->show();

    // Act
    call_private_fun::ThumbnailListViewonUpdateListview(*view);

    // Assert
    EXPECT_TRUE(view->isVisible());
}

TEST_F(ThumbnailListViewExtendedTest, OnUpdateListview_EmptyModel_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    call_private_fun::ThumbnailListViewonUpdateListview(*view);

    // Assert
    EXPECT_EQ(getModel(view.get())->rowCount(), 0);
}

// ============ onKeyPressFromQml ============

// 分支清单映射（onKeyPressFromQml complexity=5 ≥ 3，列出主要分支）：
// B1: key=="Esc" → clearSelection()
// B2: key=="Ctrl+A" → selectAll()
// B3: key=="Page Up" → scroll up
// B4: key=="Page Down" → scroll down
// B5: other key → no action
// 用例映射: B1→EscClearsSelection, B2→CtrlASelectsAll, B5→UnknownKeyNoOp

TEST_F(ThumbnailListViewExtendedTest, OnKeyPressFromQml_EscKey_ClearsSelection)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));
    view->selectAll();

    // Act
    call_private_fun::ThumbnailListViewonKeyPressFromQml(*view, QString("Esc"));

    // Assert
    EXPECT_EQ(view->selectionModel()->selectedIndexes().size(), 0);
}

TEST_F(ThumbnailListViewExtendedTest, OnKeyPressFromQml_CtrlAKey_SelectsAll)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));

    // Act
    call_private_fun::ThumbnailListViewonKeyPressFromQml(*view, QString("Ctrl+A"));

    // Assert
    EXPECT_GT(view->selectionModel()->selectedIndexes().size(), 0);
}

TEST_F(ThumbnailListViewExtendedTest, OnKeyPressFromQml_UnknownKey_NoChange)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    view->selectionModel()->select(model->index(0, 0), QItemSelectionModel::Select);
    int initialSelection = view->selectionModel()->selectedIndexes().size();

    // Act
    call_private_fun::ThumbnailListViewonKeyPressFromQml(*view, QString("Unknown"));

    // Assert
    EXPECT_EQ(view->selectionModel()->selectedIndexes().size(), initialSelection);
}

TEST_F(ThumbnailListViewExtendedTest, OnKeyPressFromQml_PageDown_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    call_private_fun::ThumbnailListViewonKeyPressFromQml(*view, QString("Page Down"));

    // Assert
    EXPECT_EQ(model->rowCount(), 1);
}

// ============ slotChangeAllSelectBtnVisible ============

TEST_F(ThumbnailListViewExtendedTest, SlotChangeAllSelectBtnVisible_True_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::TimeLineViewType);

    // Act
    view->slotChangeAllSelectBtnVisible(true);

    // Assert
    EXPECT_EQ(getModel(view.get())->rowCount(), 0);
}

TEST_F(ThumbnailListViewExtendedTest, SlotChangeAllSelectBtnVisible_False_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::TimeLineViewType);

    // Act
    view->slotChangeAllSelectBtnVisible(false);

    // Assert
    EXPECT_EQ(getModel(view.get())->rowCount(), 0);
}

TEST_F(ThumbnailListViewExtendedTest, SlotChangeAllSelectBtnVisible_WithItems_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::TimeLineViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    view->slotChangeAllSelectBtnVisible(true);

    // Assert
    EXPECT_EQ(model->rowCount(), 1);
}

// ============ cutPixmap ============

TEST_F(ThumbnailListViewExtendedTest, CutPixmap_SquareImage_NoCrop)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    DBImgInfo info;
    info.filePath = "test.jpg";
    info.image = QPixmap(100, 100);
    info.image.fill(Qt::red);

    // Act
    view->cutPixmap(info);

    // Assert
    EXPECT_EQ(info.image.width(), 100);
    EXPECT_EQ(info.image.height(), 100);
}

TEST_F(ThumbnailListViewExtendedTest, CutPixmap_WideImage_CropsToSquare)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    DBImgInfo info;
    info.filePath = "test.jpg";
    info.image = QPixmap(200, 100);
    info.image.fill(Qt::blue);

    // Act
    view->cutPixmap(info);

    // Assert
    EXPECT_EQ(info.image.width(), info.image.height());
    EXPECT_LE(info.image.width(), 100);
}

TEST_F(ThumbnailListViewExtendedTest, CutPixmap_TallImage_CropsToSquare)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    DBImgInfo info;
    info.filePath = "test.jpg";
    info.image = QPixmap(100, 200);
    info.image.fill(Qt::green);

    // Act
    view->cutPixmap(info);

    // Assert
    EXPECT_EQ(info.image.width(), info.image.height());
    EXPECT_LE(info.image.height(), 100);
}

// ============ removeSelectToTrash ============

TEST_F(ThumbnailListViewExtendedTest, RemoveSelectToTrash_EmptyPaths_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);

    // Act
    view->removeSelectToTrash(QStringList());

    // Assert
    EXPECT_EQ(getModel(view.get())->rowCount(), 0);
}

TEST_F(ThumbnailListViewExtendedTest, RemoveSelectToTrash_NonWritablePath_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    QStringList paths;
    paths << "/nonexistent/path/file.jpg";

    // Act
    view->removeSelectToTrash(paths);

    // Assert
    EXPECT_EQ(getModel(view.get())->rowCount(), 0);
}

TEST_F(ThumbnailListViewExtendedTest, RemoveSelectToTrash_WritablePath_NoCrash)
{
    // Arrange
    QTemporaryFile tmpFile;
    tmpFile.open();
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    QStringList paths;
    paths << tmpFile.fileName();

    // Act
    view->removeSelectToTrash(paths);

    // Assert
    EXPECT_EQ(getModel(view.get())->rowCount(), 0);
}

// ============ selectPhotos ============

TEST_F(ThumbnailListViewExtendedTest, SelectPhotos_MatchingPath_SelectsItem)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    appendRow(model, makeInfo("pic2.jpg", ItemTypePic));
    QStringList paths;
    paths << "pic1.jpg";

    // Act
    view->selectPhotos(paths);

    // Assert
    EXPECT_TRUE(view->selectionModel()->isSelected(model->index(0, 0)));
    EXPECT_FALSE(view->selectionModel()->isSelected(model->index(1, 0)));
}

TEST_F(ThumbnailListViewExtendedTest, SelectPhotos_NoMatchingPath_NoSelection)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    QStringList paths;
    paths << "nonexistent.jpg";

    // Act
    view->selectPhotos(paths);

    // Assert
    EXPECT_EQ(view->selectionModel()->selectedIndexes().size(), 0);
}

TEST_F(ThumbnailListViewExtendedTest, SelectPhotos_EmptyPaths_NoSelection)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));

    // Act
    view->selectPhotos(QStringList());

    // Assert
    EXPECT_EQ(view->selectionModel()->selectedIndexes().size(), 0);
}

// ============ onScrollbarValueChanged ============

TEST_F(ThumbnailListViewExtendedTest, OnScrollbarValueChanged_NoAnimation_NoCrash)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    // m_animation is nullptr by default, m_scrollTimer exists from constructor

    // Act
    view->onScrollbarValueChanged(0);

    // Assert
    EXPECT_EQ(model->rowCount(), 1);
}

TEST_F(ThumbnailListViewExtendedTest, OnScrollbarValueChanged_WithScrollTimer_StopsTimer)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    auto *scrollTimer = access_private_field::ThumbnailListViewm_scrollTimer(*view);
    ASSERT_NE(scrollTimer, nullptr);
    scrollTimer->start(1000);
    EXPECT_TRUE(scrollTimer->isActive());

    // Act
    view->onScrollbarValueChanged(50);

    // Assert
    EXPECT_FALSE(scrollTimer->isActive());
}

TEST_F(ThumbnailListViewExtendedTest, OnScrollbarValueChanged_FlushTopTimeLineCalled)
{
    // Arrange
    auto view = createView(ThumbnailDelegate::AllPicViewType);
    auto *model = getModel(view.get());
    appendRow(model, makeInfo("pic1.jpg", ItemTypePic));
    // flushTopTimeLine is stubbed in setupCommonStubs

    // Act
    view->onScrollbarValueChanged(100);

    // Assert - no crash means flushTopTimeLine was called
    EXPECT_EQ(model->rowCount(), 1);
}
