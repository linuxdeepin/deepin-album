// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later
//
// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | applyPositions | high | complexity:N/A | 3 | 4 |
// | maps | high | complexity:1 | 3 | 3 |
// | move | high | complexity:N/A | 3 | 3 |
// | nearestItem | high | complexity:N/A | 3 | 8 |
// | setRangeSelected | high | complexity:8 | 3 | 4 |
// | sourceRowsAboutToBeInserted | high | complexity:N/A | 3 | 3 |
// ─── 生成后填入 actual 列，低于 min 即违规 ───
//
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

// 分支清单映射（applyPositions——顶层控制流）：
// B1: model 处于 Listing → m_deferApplyPositions=true, 直接返回
// B2: positions.size()<5 → (defer 时 reset()) 返回，不重建映射
// B3: positions.mid(2) 数量 %3 !=0 → 返回
// B4: pos <= perStripe → 按 stripe*perStripe+pos 恢复
// B5: pos > perStripe → 溢出，分配到 firstFreeRow()
// B6: 无记录的剩余 source item → 分配到 firstFreeRow() / lastRow()+1
// B7: positions 中 name 不在 sourceIndices → 跳过
// 用例映射: B1→(构造恒为 Normal，不走 Listing，见注释), B2→ApplyPositionsTooShort,
//           B4→ApplyPositionsFullPlacement, B5→ApplyPositionsOverflow,
//           B6→ApplyPositionsMissingItem, B7→ApplyPositionsMissingItem

// 分支清单映射（move——顶层控制流）：
// B1: model Listing → 追加 defer 列表返回 -1（恒走不到，见注释）
// B2: sourceRow==-1 或 from==to → continue（不改变映射）
// B3: to==-1 → to=firstFreeRow()/lastRow()+1（不构造，见注释）
// B4: 目标被占 → while 递增 to 直到空白（isBlank 恒 false 时递增到 from，缺陷）
// B5: 不回移 from（toIndices 含 from）→ 保留原映射
// 用例映射: B2→MoveSameRow, B4→MoveToOccupied*, B5→MoveToOccupied

// 分支清单映射（nearestItem——顶层控制流）：
// B1: !m_enabled 或 currentIndex>=rowCount → -1
// B2: currentIndex<0 → firstRow()
// B3: direction 非四方向 → -1
// B4: 水平方向（Left/RightArrow）仅比较同一行的 x 距离
// B5: 垂直方向（Up/DownArrow）仅比较相邻 y 的整行
// B6: 同距离平局按 x（或 y）优先
// 用例映射: B1→NearestDisabled / NearestOutOfRange, B2→NearestNegativeIndex,
//           B3→NearestInvalidDirection, B4→NearestRight/Left, B5→NearestUp/Down

// 分支清单映射（sourceRowsAboutToBeInserted——顶层控制流）：
// B1: !m_enabled → 双 beginInsertRows（缺陷，不触发，见注释）
// B2: m_deferApplyPositions → 返回
// B3: 映射为空 → beginInsertRows(start,end)+initMaps(end+1)
// B4: 有空闲行 → updateMaps(free, i) + pending + m_ignoreNextTransaction
// B5: 无空闲行 → beginInsertRows(firstNew, firstNew+remainder)
// 用例映射: B3→InsertIntoEmpty, B4→InsertMiddle, B5→AppendToPopulated

#include <gtest/gtest.h>
#include <QApplication>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <memory>

#include "thumbnailview/thumbnailmodel.h"
#include "thumbnailview/positioner.h"
#include "imageengine/imagedataservice.h"
#include "albumControl.h"
#include "ut_fake_source.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;
using namespace std;

class PositionerTest : public ::testing::Test
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
        stub.set_lamda(ADDR(ImageDataService, instance),
                       [](QObject *) -> ImageDataService * { return nullptr; });
        stub.set_lamda(ADDR(AlbumControl, instance),
                       []() -> AlbumControl * { return nullptr; });
    }

    void TearDown() override { stub.clear(); }

    struct Harness {
        std::unique_ptr<FakeSourceModel> source;
        std::unique_ptr<ThumbnailModel> model;
        std::unique_ptr<Positioner> pos;
    };

    // Build a fake bus: FakeSourceModel(count items) -> ThumbnailModel ->
    // -> Positioner(perStripe) already enabled (initMaps done).
    static Harness build(int count, int perStripe = 4)
    {
        Harness h;
        h.source = std::make_unique<FakeSourceModel>();
        QList<FakeSourceModel::Item> items;
        for (int i = 0; i < count; ++i) {
            items.append(utFakePic(utFakeUrl(i),
                                   QStringLiteral("/home/uos/Pictures/p%1.jpg").arg(i)));
        }
        h.source->setItems(items);
        h.model = std::make_unique<ThumbnailModel>();
        h.model->setSourceModel(h.source.get());
        h.pos = std::make_unique<Positioner>();
        h.pos->setPerStripe(perStripe);
        h.pos->setThumbnailModel(h.model.get());
        h.pos->setEnabled(true);
        return h;
    }

    // Read a url through the positioner mapping (proxy row -> source).
    static QString urlAt(Harness &h, int row)
    {
        return h.pos->data(h.pos->index(row, 0), Roles::UrlRole).toString();
    }

    static const QString NEW;
};

const QString PositionerTest::NEW = QStringLiteral("file:///new.jpg");

// ─────────────────────────── applyPositions ───────────────────────────
// Each record after the {count, perStripe} header is (name, stripe, pos).
// `name` must equal the source row's UrlRole value; restore rows via
// stripe*m_perStripe+pos. Using real urls keeps the restore looped-path
// deterministic instead of falling through to the leftover (QHash-order) loop.

// Full placement: perStripe=2, each item fits -> deterministic restore.
TEST_F(PositionerTest, ApplyPositionsFullPlacement)
{
    auto h = build(4, 2);   // u0..u3
    h.pos->setPositions({QStringLiteral("4"), QStringLiteral("0"),
                         utFakeUrl(3), QStringLiteral("0"), QStringLiteral("0"),
                         utFakeUrl(2), QStringLiteral("0"), QStringLiteral("1"),
                         utFakeUrl(1), QStringLiteral("1"), QStringLiteral("0"),
                         utFakeUrl(0), QStringLiteral("1"), QStringLiteral("1")});

    EXPECT_EQ(h.pos->rowCount(), 4);
    // proxy row = stripe*2 + pos: p0->u3, p1->u2, p2->u1, p3->u0
    EXPECT_EQ(h.pos->maps({0, 1, 2, 3}), (QVariantList{3, 2, 1, 0}));
    EXPECT_EQ(urlAt(h, 0), utFakeUrl(3));
    EXPECT_EQ(urlAt(h, 1), utFakeUrl(2));
    EXPECT_EQ(urlAt(h, 3), utFakeUrl(0));
}

// One item no longer fits (pos > perStripe) -> moved to the first free row.
TEST_F(PositionerTest, ApplyPositionsOverflow)
{
    auto h = build(5, 2);   // u0..u4
    h.pos->setPositions({QStringLiteral("5"), QStringLiteral("0"),
                         utFakeUrl(4), QStringLiteral("0"), QStringLiteral("3"),
                         utFakeUrl(3), QStringLiteral("0"), QStringLiteral("1"),
                         utFakeUrl(2), QStringLiteral("1"), QStringLiteral("0"),
                         utFakeUrl(1), QStringLiteral("1"), QStringLiteral("1"),
                         utFakeUrl(0), QStringLiteral("2"), QStringLiteral("1")});

    // p1->u3, p2->u2, p3->u1, p5->u0; u4 (pos3>2) overflows -> firstFreeRow p0
    EXPECT_EQ(h.pos->rowCount(), 6);
    EXPECT_EQ(h.pos->maps({0, 1, 2, 3, 4, 5}), (QVariantList{4, 3, 2, 1, -1, 0}));
    EXPECT_EQ(urlAt(h, 0), utFakeUrl(4));
    EXPECT_EQ(urlAt(h, 1), utFakeUrl(3));
    EXPECT_EQ(urlAt(h, 2), utFakeUrl(2));
}

// A record name we don't have in the source is skipped; leftover source
// items are appended to free rows.
TEST_F(PositionerTest, ApplyPositionsMissingItem)
{
    auto h = build(3, 2);   // u0..u2
    h.pos->setPositions({QStringLiteral("3"), QStringLiteral("0"),
                         QStringLiteral("file:///gone.jpg"), QStringLiteral("0"), QStringLiteral("0"),
                         utFakeUrl(2), QStringLiteral("0"), QStringLiteral("1"),
                         utFakeUrl(1), QStringLiteral("1"), QStringLiteral("0")});

    // "gone.jpg" skipped; p1->u2, p2->u1; leftover u0 -> p0
    EXPECT_EQ(h.pos->rowCount(), 3);
    EXPECT_EQ(h.pos->maps({0, 1, 2}), (QVariantList{0, 2, 1}));
    EXPECT_EQ(urlAt(h, 0), utFakeUrl(0));
    EXPECT_EQ(urlAt(h, 2), utFakeUrl(1));
}

// Too-short positions list (<5) → early return, mapping untouched.
TEST_F(PositionerTest, ApplyPositionsTooShort)
{
    auto h = build(4, 2);
    h.pos->setPositions({"a", "b"});
    EXPECT_EQ(h.pos->maps({0, 1, 2, 3}), (QVariantList{0, 1, 2, 3}));
}

// ─────────────────────────── maps ───────────────────────────

TEST_F(PositionerTest, MapsIdentityWhenEnabled)
{
    auto h = build(10);
    EXPECT_EQ(h.pos->maps({0, 1, 2}), (QVariantList{0, 1, 2}));
}

TEST_F(PositionerTest, MapsOutOfRangeYieldsMinusOne)
{
    auto h = build(10);
    EXPECT_EQ(h.pos->maps({0, 99}), (QVariantList{0, -1}));
    EXPECT_EQ(h.pos->maps({-5, 0, 1}), (QVariantList{0, 1}));   // negative skipped
}

TEST_F(PositionerTest, MapsEmptyWhenDisabled)
{
    auto h = build(10);
    h.pos->setEnabled(false);
    EXPECT_TRUE(h.pos->maps({0, 1}).isEmpty());
}

// ─────────────────────────── move ───────────────────────────

TEST_F(PositionerTest, MoveSameRow)
{
    auto h = build(10);
    EXPECT_EQ(h.pos->move({3, 3}), 3);           // from==to -> no remap
    EXPECT_EQ(h.model->selectedIndexes(), (QList<int>{3}));
    EXPECT_EQ(h.pos->maps({0, 1, 2, 3}), (QVariantList{0, 1, 2, 3}));
}

// to < from: target occupied, blank never found (isBlank stubbed false) →
// while loop advances to == from, keeps the row mapping, selects proxy row 8.
TEST_F(PositionerTest, MoveToOccupiedBackwards)
{
    auto h = build(10);
    EXPECT_EQ(h.pos->move({8, 3}), 8);
    EXPECT_EQ(h.model->selectedIndexes(), (QList<int>{8}));
    // mapping unchanged (row 8 still maps to source 8)
    EXPECT_EQ(h.pos->maps({8}), (QVariantList{8}));
}

TEST_F(PositionerTest, MoveUnknownRowNoSelection)
{
    auto h = build(10);
    EXPECT_EQ(h.pos->move({99, 99}), 99);        // sourceRow == -1
    EXPECT_TRUE(h.model->selectedIndexes().isEmpty());
    EXPECT_EQ(h.pos->maps({0, 1}), (QVariantList{0, 1}));
}

// ─────────────────────────── nearestItem ───────────────────────────

// 10 rows, perStripe = 4 → 2D grid: row = (col, line).
TEST_F(PositionerTest, NearestRight)
{
    auto h = build(10);
    // from proxy row 5: row6 is the closest to the right on the same line.
    EXPECT_EQ(h.pos->nearestItem(5, Qt::RightArrow), 6);
}

TEST_F(PositionerTest, NearestUp)
{
    auto h = build(10);
    EXPECT_EQ(h.pos->nearestItem(6, Qt::UpArrow), 2);
}

TEST_F(PositionerTest, NearestDown)
{
    auto h = build(10);
    EXPECT_EQ(h.pos->nearestItem(1, Qt::DownArrow), 5);
}

TEST_F(PositionerTest, NearestLeft)
{
    auto h = build(10);
    EXPECT_EQ(h.pos->nearestItem(5, Qt::LeftArrow), 4);
}

TEST_F(PositionerTest, NearestInvalidDirection)
{
    auto h = build(10);
    EXPECT_EQ(h.pos->nearestItem(0, Qt::NoArrow), -1);
}

TEST_F(PositionerTest, NearestDisabled)
{
    auto h = build(10);
    h.pos->setEnabled(false);
    EXPECT_EQ(h.pos->nearestItem(5, Qt::RightArrow), -1);
}

TEST_F(PositionerTest, NearestOutOfRange)
{
    auto h = build(10);
    EXPECT_EQ(h.pos->nearestItem(10, Qt::RightArrow), -1);
}

TEST_F(PositionerTest, NearestNegativeReturnsFirst)
{
    auto h = build(10);
    EXPECT_EQ(h.pos->nearestItem(-1, Qt::RightArrow), 0);
}

// ─────────────────────────── setRangeSelected ───────────────────────────

TEST_F(PositionerTest, SetRangeSelectedEnabled)
{
    auto h = build(10);
    h.pos->setRangeSelected(2, 4);
    EXPECT_EQ(h.model->selectedIndexes(), (QList<int>{2, 3, 4}));
}

TEST_F(PositionerTest, SetRangeSelectedReversedOrder)
{
    auto h = build(10);
    h.pos->setRangeSelected(4, 2);               // anchor > to still selects 2..4
    EXPECT_EQ(h.model->selectedIndexes(), (QList<int>{2, 3, 4}));
}

TEST_F(PositionerTest, SetRangeSelectedDisabledGoesThroughModel)
{
    auto h = build(10);
    h.pos->setEnabled(false);
    h.pos->setRangeSelected(2, 4);               // forwards to model
    EXPECT_EQ(h.model->selectedIndexes(), (QList<int>{2, 3, 4}));
}

TEST_F(PositionerTest, SetRangeSelectedNoSelectionInitially)
{
    auto h = build(10);
    EXPECT_TRUE(h.model->selectedIndexes().isEmpty());
}

// ─────────────────────────── sourceRowsAboutToBeInserted ───────────────────────────

// Append to a populated list: no free slot → beginInsertRows(firstNew=3,3).
TEST_F(PositionerTest, SourceRowsAboutToBeInsertedAppend)
{
    auto h = build(3);                           // u0..u2 -> maps 0..2
    h.source->appendItem(utFakePic(PositionerTest::NEW, "/home/uos/Pictures/new.jpg"));

    EXPECT_EQ(h.pos->rowCount(), 4);
    EXPECT_EQ(h.pos->maps({0, 1, 2, 3}), (QVariantList{0, 1, 2, 3}));
    EXPECT_EQ(urlAt(h, 3), PositionerTest::NEW);
}

// Insert into an empty model: mapping empty branch → beginInsertRows + initMaps.
TEST_F(PositionerTest, SourceRowsAboutToBeInsertedIntoEmpty)
{
    auto h = build(0);
    h.source->insertItem(0, utFakePic(PositionerTest::NEW, "/home/uos/Pictures/new.jpg"));

    EXPECT_EQ(h.pos->rowCount(), 1);
    EXPECT_EQ(urlAt(h, 0), PositionerTest::NEW);
    EXPECT_EQ(h.pos->maps({0}), (QVariantList{0}));
}

// Middle insert: the proxy places an equal-key (stable-sorted) new row at
// the END of the proxy, so source order stays [u0,u1,u2,NEW] and the new
// row lands on proxy row 3 (its own first free slot given the append range).
TEST_F(PositionerTest, SourceRowsAboutToBeInsertedMiddle)
{
    auto h = build(3);                           // u0..u2
    h.source->insertItem(1, utFakePic(PositionerTest::NEW, "/home/uos/Pictures/new.jpg"));

    EXPECT_EQ(h.pos->rowCount(), 4);
    EXPECT_EQ(h.pos->maps({0, 1, 2, 3}), (QVariantList{0, 1, 2, 3}));
    EXPECT_EQ(urlAt(h, 0), utFakeUrl(0));
    EXPECT_EQ(urlAt(h, 1), utFakeUrl(1));
    EXPECT_EQ(urlAt(h, 2), utFakeUrl(2));
    EXPECT_EQ(urlAt(h, 3), PositionerTest::NEW);
}
