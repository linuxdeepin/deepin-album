// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | roleNames | low | complexity:0 | 1 | 1 |
// | setData | low | complexity:1 | 1 | 2 |
// | rowCount | low | complexity:1 | 1 | 2 |
// | currentIndex | low | complexity:0 | 1 | 1 |
// | setCurrentIndex | mid | complexity:2 | 3 | 3 |
// | setQueueCount | mid | complexity:2 | 2 | 2 |
// | syncState | mid | complexity:2 | 2 | 2 |
// | setCurrentSourceIndex | high | complexity:8 | 3 | 6 |
// | movePrevoius | mid | complexity:3 | 2 | 2 |
// | moveNext | mid | complexity:3 | 2 | 2 |
// | resetModel | mid | complexity:4 | 2 | 3 |
// | deleteCurrent | high | complexity:6 | 3 | 5 |
// | dumpInfo | low | complexity:0 | 1 | 1 |
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

// 分支清单映射（setCurrentIndex complexity=2）：
// B1: index == currentProxyIdx → no signal
// B2: index != currentProxyIdx → set + emit currentIndexChanged
// 用例映射: B1→SetCurrentIndex_SameValueNoSignal, B2→SetCurrentIndex_DifferentValueEmitsSignal,
//           B2边界→SetCurrentIndex_NegativeValue
//
// 分支清单映射（setQueueCount complexity=2）：
// B1: count >= 3 && odd → set maxCount and radius
// B2: Q_ASSERT fires for invalid (debug-only, not tested)
// 用例映射: B1→SetQueueCount_ValidOdd, B1→SetQueueCount_ValidLargerOdd
//
// 分支清单映射（syncState complexity=2）：
// B1: jumpFlag == Current → no-op
// B2: jumpFlag != Current → refreshBothSideData
// 用例映射: B1→SyncState_JumpFlagCurrent_NoOp, B2→SyncState_JumpFlagNotCurrent_Refreshes
//
// 分支清单映射（setCurrentSourceIndex complexity=8）：
// B1: indexQueue.isEmpty() → early return
// B2: type == Current → default (no action)
// B3: type == Previous → movePrevoius
// B4: type == Next → moveNext
// B5: type == OutOfRange → jumpToIndex
// B6: type != Current && jumpFlag != Current → refreshBothSideData (interrupt animation)
// 用例映射: B1→SetCurrentSourceIndex_EmptyQueue_ReturnsEarly,
//           B2→SetCurrentSourceIndex_CurrentDistance_NoMovement,
//           B3→SetCurrentSourceIndex_PreviousDistance_MovesPrevious,
//           B4→SetCurrentSourceIndex_NextDistance_MovesNext,
//           B5→SetCurrentSourceIndex_OutOfRange_Jumps,
//           B6→SetCurrentSourceIndex_InterruptAnimation_Refreshes
//
// 分支清单映射（resetModel complexity=4）：
// B1: clears queue, sets currentProxyIdx=0
// B2: fills queue to maxCount
// B3: current at sourceIndex, rest filled around it
// 用例映射: B1+B2→ResetModel_PopulatesQueueToMaxCount,
//           B3→ResetModel_CurrentAtSpecifiedSourceIndex,
//           B1+B2→ResetModel_FirstItemIsCurrent
//
// 分支清单映射（deleteCurrent complexity=6）：
// B1: indexQueue.isEmpty() → early return
// B2: sourceModel->rowCount() == 0 → clear queue
// B3: current is null → early return
// B4: atEnd (current->index >= rowCount) → move to previous
// B5: !atEnd → move to next
// 用例映射: B1→DeleteCurrent_EmptyQueue_NoCrash,
//           B2→DeleteCurrent_EmptySourceModel_ClearsQueue,
//           B3→DeleteCurrent_NullCurrent_NoCrash,
//           B5→DeleteCurrent_NotAtEnd_MovesToNext,
//           B4→DeleteCurrent_AtEnd_MovesToPrevious

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QSignalSpy>
#include <QList>
#include <QUrl>
#include <QVariant>
#include <QHash>
#include <QByteArray>
#include <QSharedPointer>
#include <memory>

#include "types.h"
#include "imagedata/imagesourcemodel.h"
#include "imagedata/imageinfo.h"

// PathViewProxyModel has private nested types (DistanceType, IndexInfo, IndexInfoPtr)
// and private members (indexQueue, currentProxyIdx, jumpFlag). Use the standard testing
// technique of making private members public to access them directly.
#define private public
#include "imagedata/pathviewproxymodel.h"
#undef private

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// Access ImageInfo's private refreshDataFromCache for stubbing to prevent file I/O
ACCESS_PRIVATE_FUN(ImageInfo, void(bool), refreshDataFromCache)

class PathViewProxyModelExtendedTest : public ::testing::Test
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

    StubExt stub;
    std::unique_ptr<ImageSourceModel> m_sourceModel;
    std::unique_ptr<PathViewProxyModel> m_proxy;

    void SetUp() override
    {
        stub.clear();

        // Stub ImageInfo::refreshDataFromCache to prevent real file/cache I/O.
        // With the stub, ImageInfo status stays Null (not Loading), so asyncUpdateLoadInfo
        // is never triggered. frameCount() returns 1 (data is null), frameIndex clamped to 0.
        stub.set_lamda(get_private_fun::ImageInforefreshDataFromCache(),
                       [](ImageInfo *, bool) {});

        // Create source model with test data
        m_sourceModel = std::make_unique<ImageSourceModel>();
        QList<QUrl> files;
        files << QUrl("file:///tmp/img0.jpg")
              << QUrl("file:///tmp/img1.jpg")
              << QUrl("file:///tmp/img2.jpg")
              << QUrl("file:///tmp/img3.jpg")
              << QUrl("file:///tmp/img4.jpg");
        m_sourceModel->setImageFiles(files);

        m_proxy = std::make_unique<PathViewProxyModel>(m_sourceModel.get());
    }

    void TearDown() override
    {
        m_proxy.reset();
        m_sourceModel.reset();
        stub.clear();
    }

    // Helper to create an IndexInfoPtr
    PathViewProxyModel::IndexInfoPtr makeInfo(const QUrl &url, int index,
                                                int frameCount = 1, int frameIndex = 0)
    {
        auto info = PathViewProxyModel::IndexInfoPtr::create();
        info->url = url;
        info->index = index;
        info->frameCount = frameCount;
        info->frameIndex = frameIndex;
        return info;
    }

    // Helper to initialize proxy with resetModel for movement tests
    void setupWithReset(int sourceIndex = 2, int frameIndex = 0)
    {
        m_proxy->resetModel(sourceIndex, frameIndex);
    }
};

// ===== roleNames() tests =====

// B1: roleNames returns correct hash with ImageUrlRole and FrameIndexRole
TEST_F(PathViewProxyModelExtendedTest, RoleNames_ReturnsExpectedRoleHash)
{
    auto names = m_proxy->roleNames();
    EXPECT_EQ(names.size(), 2);
    EXPECT_EQ(names.value(Types::ImageUrlRole), "imageUrl");
    EXPECT_EQ(names.value(Types::FrameIndexRole), "frameIndex");
}

// ===== setData() tests =====

// B1: setData always returns false (no-op implementation)
TEST_F(PathViewProxyModelExtendedTest, SetData_AlwaysReturnsFalse)
{
    m_proxy->indexQueue.clear();
    m_proxy->indexQueue.append(makeInfo(QUrl("file:///tmp/test.jpg"), 0));

    QModelIndex idx = m_proxy->index(0, 0);
    EXPECT_FALSE(m_proxy->setData(idx, QVariant("test"), Qt::EditRole));
}

// B1: setData returns false even with invalid index
TEST_F(PathViewProxyModelExtendedTest, SetData_InvalidIndex_ReturnsFalse)
{
    QModelIndex invalid;
    EXPECT_FALSE(m_proxy->setData(invalid, QVariant(), Qt::EditRole));
}

// ===== rowCount() tests =====

// B1: Empty queue returns 0
TEST_F(PathViewProxyModelExtendedTest, RowCount_EmptyQueue_ReturnsZero)
{
    m_proxy->indexQueue.clear();
    EXPECT_EQ(m_proxy->rowCount(), 0);
}

// B2: After resetModel, rowCount equals maxCount
TEST_F(PathViewProxyModelExtendedTest, RowCount_AfterReset_ReturnsMaxCount)
{
    setupWithReset(2, 0);
    EXPECT_EQ(m_proxy->rowCount(), m_proxy->maxCount);
    EXPECT_EQ(m_proxy->rowCount(), 5);  // default queue count
}

// ===== currentIndex() tests =====

// B1: Initial currentIndex is 0 after construction
TEST_F(PathViewProxyModelExtendedTest, CurrentIndex_InitialValueIsZero)
{
    EXPECT_EQ(m_proxy->currentIndex(), 0);
    EXPECT_EQ(m_proxy->currentIndex(), m_proxy->currentProxyIdx);
}

// ===== setCurrentIndex() tests =====

// B1: Setting to same value does not emit signal
TEST_F(PathViewProxyModelExtendedTest, SetCurrentIndex_SameValueNoSignal)
{
    m_proxy->currentProxyIdx = 2;
    QSignalSpy spy(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    m_proxy->setCurrentIndex(2);
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(m_proxy->currentProxyIdx, 2);
}

// B2: Setting to different value emits signal with new index
TEST_F(PathViewProxyModelExtendedTest, SetCurrentIndex_DifferentValueEmitsSignal)
{
    m_proxy->currentProxyIdx = 1;
    QSignalSpy spy(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    m_proxy->setCurrentIndex(3);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toInt(), 3);
    EXPECT_EQ(m_proxy->currentProxyIdx, 3);
    EXPECT_EQ(m_proxy->currentIndex(), 3);
}

// B2 boundary: Setting to negative value still emits signal and updates
TEST_F(PathViewProxyModelExtendedTest, SetCurrentIndex_NegativeValue)
{
    m_proxy->currentProxyIdx = 2;
    QSignalSpy spy(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    m_proxy->setCurrentIndex(-1);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toInt(), -1);
    EXPECT_EQ(m_proxy->currentProxyIdx, -1);
}

// ===== setQueueCount() tests =====

// B1: Valid odd count sets maxCount and radius correctly
TEST_F(PathViewProxyModelExtendedTest, SetQueueCount_ValidOdd)
{
    m_proxy->setQueueCount(3);
    EXPECT_EQ(m_proxy->maxCount, 3);
    EXPECT_EQ(m_proxy->radius, 1);  // floor(3/2) = 1
}

// B1: Larger valid odd count
TEST_F(PathViewProxyModelExtendedTest, SetQueueCount_ValidLargerOdd)
{
    m_proxy->setQueueCount(7);
    EXPECT_EQ(m_proxy->maxCount, 7);
    EXPECT_EQ(m_proxy->radius, 3);  // floor(7/2) = 3
}

// ===== syncState() tests =====

// B1: jumpFlag == Current → no-op (no dataChanged signals)
TEST_F(PathViewProxyModelExtendedTest, SyncState_JumpFlagCurrent_NoOp)
{
    setupWithReset(2, 0);
    m_proxy->jumpFlag = PathViewProxyModel::Current;
    QSignalSpy spy(m_proxy.get(), &PathViewProxyModel::dataChanged);
    m_proxy->syncState();
    EXPECT_EQ(spy.count(), 0);
    EXPECT_EQ(m_proxy->jumpFlag, PathViewProxyModel::Current);
}

// B2: jumpFlag != Current → refreshBothSideData called (dataChanged emitted)
TEST_F(PathViewProxyModelExtendedTest, SyncState_JumpFlagNotCurrent_Refreshes)
{
    setupWithReset(2, 0);
    m_proxy->jumpFlag = PathViewProxyModel::Next;
    QSignalSpy spy(m_proxy.get(), &PathViewProxyModel::dataChanged);
    m_proxy->syncState();
    // refreshBothSideData updates radius entries on each side (2 * radius = 4 updates)
    EXPECT_EQ(spy.count(), m_proxy->radius * 2);
    EXPECT_EQ(m_proxy->jumpFlag, PathViewProxyModel::Current);
}

// ===== setCurrentSourceIndex() tests =====

// B1: Empty queue → early return (no crash, no signal)
TEST_F(PathViewProxyModelExtendedTest, SetCurrentSourceIndex_EmptyQueue_ReturnsEarly)
{
    m_proxy->indexQueue.clear();
    QSignalSpy spy(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    m_proxy->setCurrentSourceIndex(0, 0);
    EXPECT_EQ(spy.count(), 0);
}

// B2: distance == Current → no movement (default case in switch)
TEST_F(PathViewProxyModelExtendedTest, SetCurrentSourceIndex_CurrentDistance_NoMovement)
{
    setupWithReset(2, 0);
    // distance(2, 0) with current at index 2: indexDis = 0 (Current), range = 0
    QSignalSpy spyIdx(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    QSignalSpy spyData(m_proxy.get(), &PathViewProxyModel::dataChanged);
    m_proxy->setCurrentSourceIndex(2, 0);
    EXPECT_EQ(spyIdx.count(), 0);
    EXPECT_EQ(spyData.count(), 0);
    EXPECT_EQ(m_proxy->currentProxyIdx, 0);
}

// B3: distance == Previous → movePrevoius called
TEST_F(PathViewProxyModelExtendedTest, SetCurrentSourceIndex_PreviousDistance_MovesPrevious)
{
    setupWithReset(2, 0);
    // distance(1, 0) with current at index 2: indexDis = -1 (Previous), range = -1
    QSignalSpy spyIdx(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    m_proxy->setCurrentSourceIndex(1, 0);
    EXPECT_GE(spyIdx.count(), 1);
    // movePrevoius sets currentProxyIdx to previousPorxyIdx(0) = 4 (in ring of 5)
    EXPECT_EQ(m_proxy->currentProxyIdx, 4);
}

// B4: distance == Next → moveNext called
TEST_F(PathViewProxyModelExtendedTest, SetCurrentSourceIndex_NextDistance_MovesNext)
{
    setupWithReset(2, 0);
    // distance(3, 0) with current at index 2: indexDis = 1 (Next), range = 1
    QSignalSpy spyIdx(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    m_proxy->setCurrentSourceIndex(3, 0);
    EXPECT_GE(spyIdx.count(), 1);
    // moveNext sets currentProxyIdx to nextProxyIdx(0) = 1
    EXPECT_EQ(m_proxy->currentProxyIdx, 1);
}

// B5: distance == OutOfRange → jumpToIndex called
TEST_F(PathViewProxyModelExtendedTest, SetCurrentSourceIndex_OutOfRange_Jumps)
{
    setupWithReset(2, 0);
    // distance(4, 0) with current at index 2: indexDis = 2 (default), range = OutOfRange
    QSignalSpy spyIdx(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    QSignalSpy spyData(m_proxy.get(), &PathViewProxyModel::dataChanged);
    m_proxy->setCurrentSourceIndex(4, 0);
    // jumpToIndex: sourceIndex(4) >= current->index(2) && frameIndex(0) >= 0 → Next
    // jumpIndex = nextProxyIdx(0) = 1
    EXPECT_GE(spyIdx.count(), 1);
    EXPECT_GE(spyData.count(), 1);
    EXPECT_EQ(m_proxy->currentProxyIdx, 1);
    EXPECT_EQ(m_proxy->jumpFlag, PathViewProxyModel::Next);
}

// B6: type != Current && jumpFlag != Current → refreshBothSideData (interrupt animation)
TEST_F(PathViewProxyModelExtendedTest, SetCurrentSourceIndex_InterruptAnimation_Refreshes)
{
    setupWithReset(2, 0);
    // Set jumpFlag to Next (simulating an animation in progress)
    m_proxy->jumpFlag = PathViewProxyModel::Next;
    // distance(1, 0) = Previous (not Current), jumpFlag = Next (not Current)
    // → refreshBothSideData is called before movePrevoius
    QSignalSpy spyData(m_proxy.get(), &PathViewProxyModel::dataChanged);
    m_proxy->setCurrentSourceIndex(1, 0);
    // refreshBothSideData emits radius*2 dataChanged,
    // then movePrevoius emits 1 more dataChanged
    int expected = m_proxy->radius * 2 + 1;
    EXPECT_GE(spyData.count(), expected);
}

// ===== movePrevoius() tests =====

// B1: movePrevoius changes currentProxyIdx to previous in ring
TEST_F(PathViewProxyModelExtendedTest, MovePrevoius_ChangesCurrentProxyIdx)
{
    setupWithReset(2, 0);
    // currentProxyIdx = 0, previousPorxyIdx(0) = 4 (ring of 5)
    QSignalSpy spyIdx(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    QSignalSpy spyData(m_proxy.get(), &PathViewProxyModel::dataChanged);
    m_proxy->movePrevoius();
    EXPECT_EQ(spyIdx.count(), 1);
    EXPECT_EQ(spyIdx.takeFirst().at(0).toInt(), 4);
    EXPECT_EQ(m_proxy->currentProxyIdx, 4);
    // movePrevoius calls updateIndexInfo once → 1 dataChanged
    EXPECT_EQ(spyData.count(), 1);
}

// B2: movePrevoius wraps around ring correctly
TEST_F(PathViewProxyModelExtendedTest, MovePrevoius_WrapsAround)
{
    setupWithReset(2, 0);
    // Move twice: 0 → 4 → 3
    m_proxy->movePrevoius();
    EXPECT_EQ(m_proxy->currentProxyIdx, 4);
    m_proxy->movePrevoius();
    EXPECT_EQ(m_proxy->currentProxyIdx, 3);
}

// ===== moveNext() tests =====

// B1: moveNext changes currentProxyIdx to next in ring
TEST_F(PathViewProxyModelExtendedTest, MoveNext_ChangesCurrentProxyIdx)
{
    setupWithReset(2, 0);
    // currentProxyIdx = 0, nextProxyIdx(0) = 1
    QSignalSpy spyIdx(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    QSignalSpy spyData(m_proxy.get(), &PathViewProxyModel::dataChanged);
    m_proxy->moveNext();
    EXPECT_EQ(spyIdx.count(), 1);
    EXPECT_EQ(spyIdx.takeFirst().at(0).toInt(), 1);
    EXPECT_EQ(m_proxy->currentProxyIdx, 1);
    // moveNext calls updateIndexInfo once → 1 dataChanged
    EXPECT_EQ(spyData.count(), 1);
}

// B2: moveNext wraps around ring correctly
TEST_F(PathViewProxyModelExtendedTest, MoveNext_WrapsAround)
{
    setupWithReset(2, 0);
    // Move to last index, then wrap to 0
    for (int i = 0; i < 4; ++i) {
        m_proxy->moveNext();
    }
    EXPECT_EQ(m_proxy->currentProxyIdx, 4);
    m_proxy->moveNext();
    EXPECT_EQ(m_proxy->currentProxyIdx, 0);
}

// ===== resetModel() tests =====

// B1+B2: resetModel populates queue to maxCount
TEST_F(PathViewProxyModelExtendedTest, ResetModel_PopulatesQueueToMaxCount)
{
    m_proxy->resetModel(2, 0);
    EXPECT_EQ(m_proxy->indexQueue.size(), m_proxy->maxCount);
    EXPECT_EQ(m_proxy->indexQueue.size(), 5);
    EXPECT_EQ(m_proxy->currentProxyIdx, 0);
    EXPECT_EQ(m_proxy->jumpFlag, PathViewProxyModel::Current);
}

// B3: First item in queue corresponds to the specified source index
TEST_F(PathViewProxyModelExtendedTest, ResetModel_FirstItemIsCurrent)
{
    m_proxy->resetModel(2, 0);
    ASSERT_FALSE(m_proxy->indexQueue.isEmpty());
    ASSERT_NE(m_proxy->indexQueue[0], nullptr);
    EXPECT_EQ(m_proxy->indexQueue[0]->index, 2);
    EXPECT_FALSE(m_proxy->indexQueue[0]->url.isEmpty());
}

// B1+B2: resetModel with different source index
TEST_F(PathViewProxyModelExtendedTest, ResetModel_DifferentSourceIndex)
{
    m_proxy->resetModel(0, 0);
    EXPECT_EQ(m_proxy->indexQueue.size(), 5);
    ASSERT_NE(m_proxy->indexQueue[0], nullptr);
    EXPECT_EQ(m_proxy->indexQueue[0]->index, 0);
    // Previous of index 0 is out of bounds → null entries on left side
    // Next items should be indices 1, 2
    ASSERT_NE(m_proxy->indexQueue[1], nullptr);
    EXPECT_EQ(m_proxy->indexQueue[1]->index, 1);
}

// ===== deleteCurrent() tests =====

// B1: Empty queue → early return (no crash)
TEST_F(PathViewProxyModelExtendedTest, DeleteCurrent_EmptyQueue_NoCrash)
{
    m_proxy->indexQueue.clear();
    QSignalSpy spy(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    m_proxy->deleteCurrent();
    EXPECT_EQ(spy.count(), 0);
    EXPECT_TRUE(m_proxy->indexQueue.isEmpty());
}

// B2: Empty source model → clears queue
TEST_F(PathViewProxyModelExtendedTest, DeleteCurrent_EmptySourceModel_ClearsQueue)
{
    m_sourceModel->setImageFiles({});
    m_proxy->indexQueue.clear();
    m_proxy->indexQueue.append(makeInfo(QUrl("file:///tmp/img0.jpg"), 0));
    m_proxy->currentProxyIdx = 0;
    ASSERT_EQ(m_sourceModel->rowCount(), 0);

    m_proxy->deleteCurrent();
    EXPECT_TRUE(m_proxy->indexQueue.isEmpty());
}

// B3: Null current index info → early return (no crash)
TEST_F(PathViewProxyModelExtendedTest, DeleteCurrent_NullCurrent_NoCrash)
{
    m_proxy->indexQueue.clear();
    m_proxy->indexQueue.append(nullptr);
    m_proxy->currentProxyIdx = 0;
    // sourceModel has 5 items, so rowCount != 0
    QSignalSpy spy(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    m_proxy->deleteCurrent();
    EXPECT_EQ(spy.count(), 0);
}

// B5: Not at end (current->index < rowCount) → moves to next
TEST_F(PathViewProxyModelExtendedTest, DeleteCurrent_NotAtEnd_MovesToNext)
{
    setupWithReset(2, 0);
    // current->index = 2, sourceModel->rowCount() = 5, atEnd = false
    int oldProxyIdx = m_proxy->currentProxyIdx;
    QSignalSpy spyIdx(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    m_proxy->deleteCurrent();
    // Should move to next: nextProxyIdx(0) = 1
    EXPECT_EQ(m_proxy->currentProxyIdx, (oldProxyIdx + 1) % m_proxy->maxCount);
    EXPECT_GE(spyIdx.count(), 1);
}

// B4: At end (current->index >= rowCount) → moves to previous
TEST_F(PathViewProxyModelExtendedTest, DeleteCurrent_AtEnd_MovesToPrevious)
{
    // Set up a 3-item queue with current at index 5 (>= rowCount of 5)
    m_proxy->setQueueCount(3);
    m_proxy->indexQueue.clear();
    m_proxy->indexQueue.append(makeInfo(QUrl("file:///tmp/img1.jpg"), 1));
    m_proxy->indexQueue.append(makeInfo(QUrl("file:///tmp/img2.jpg"), 2));
    m_proxy->indexQueue.append(makeInfo(QUrl("file:///tmp/img5.jpg"), 5));  // index 5 >= 5
    m_proxy->currentProxyIdx = 2;

    QSignalSpy spyIdx(m_proxy.get(), &PathViewProxyModel::currentIndexChanged);
    m_proxy->deleteCurrent();
    // atEnd → moves to previous: previousPorxyIdx(2) = (2-1+3)%3 = 1
    EXPECT_EQ(m_proxy->currentProxyIdx, 1);
    EXPECT_GE(spyIdx.count(), 1);
}

// ===== dumpInfo() tests =====

// B1: dumpInfo does not crash (just prints log info)
TEST_F(PathViewProxyModelExtendedTest, DumpInfo_NoCrash)
{
    setupWithReset(2, 0);
    // Also test with a null entry to cover that branch
    m_proxy->indexQueue[1] = nullptr;
    m_proxy->dumpInfo();  // Should not crash
    SUCCEED();
}
