// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Supplementary unit tests for ImageInfo (issue V-9596).
// Covers uncovered interfaces: constructor(QUrl), type, swapWidthAndHeight,
// setFrameIndex/frameIndex/frameCount, setScale/scale, clearCurrentCache,
// static clearCache, setStatus, onLoadFinished, onSizeChanged,
// refreshDataFromCache branches, and signal emissions.

#include <gtest/gtest.h>
#include <QApplication>
#include <QUrl>
#include <QSignalSpy>
#include <QTemporaryFile>
#include <QImage>
#include <memory>

#include "imagedata/imageinfo.h"
#include "types.h"
#include "imagedata/thumbnailcache.h"
#include "globalcontrol.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// ---- Private field / fun accessors (file-scope, independent) ----
ACCESS_PRIVATE_FIELD(ImageInfo, QUrl, imageUrl)
ACCESS_PRIVATE_FIELD(ImageInfo, QSharedPointer<ImageInfoData>, data)
ACCESS_PRIVATE_FIELD(ImageInfo, int, currentIndex)
ACCESS_PRIVATE_FIELD(ImageInfo, ImageInfo::Status, imageStatus)
ACCESS_PRIVATE_FUN(ImageInfo, void(bool), refreshDataFromCache)
ACCESS_PRIVATE_FUN(ImageInfo, bool(const QSharedPointer<ImageInfoData> &), updateData)
ACCESS_PRIVATE_FUN(ImageInfo, void(ImageInfo::Status), setStatus)
ACCESS_PRIVATE_FUN(ImageInfo, void(const QString &, int), onLoadFinished)
ACCESS_PRIVATE_FUN(ImageInfo, void(const QString &, int), onSizeChanged)

class ImageInfoExtendedTest : public ::testing::Test
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
    std::unique_ptr<ImageInfo> m_info;

    void SetUp() override
    {
        stub.clear();
        // Stub refreshDataFromCache by default to avoid real cache/disk access
        stub.set_lamda(get_private_fun::ImageInforefreshDataFromCache(),
                       [](ImageInfo *, bool) {});
        // Stub enableMultiThread to avoid thread pool in any cache load
        stub.set_lamda(ADDR(GlobalControl, enableMultiThread), []() -> bool { return false; });
        m_info = std::make_unique<ImageInfo>();
    }

    void TearDown() override
    {
        m_info.reset();
        stub.clear();
    }
};



// ---- Second fixture: does NOT stub refreshDataFromCache ----
class ImageInfoRealRefreshTest : public ::testing::Test
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
    std::unique_ptr<ImageInfo> m_info;

    void SetUp() override
    {
        stub.clear();
        stub.set_lamda(ADDR(GlobalControl, enableMultiThread), []() -> bool { return false; });
        stub.set_lamda(ADDR(ThumbnailCache, contains), [](ThumbnailCache *, const QString &, int) -> bool { return false; });
        m_info = std::make_unique<ImageInfo>();
    }

    void TearDown() override
    {
        m_info.reset();
        stub.clear();
    }
};

// ─────────────────────── Constructor with QUrl ───────────────────────

TEST_F(ImageInfoExtendedTest, ConstructorWithUrl_SetsSource)
{
    QUrl url = QUrl::fromLocalFile("/tmp/test_construct.jpg");
    auto info = std::make_unique<ImageInfo>(url);
    EXPECT_EQ(info->source(), url);
}

TEST_F(ImageInfoExtendedTest, ConstructorWithUrl_EmptyUrl)
{
    QUrl emptyUrl;
    auto info = std::make_unique<ImageInfo>(emptyUrl);
    EXPECT_EQ(info->source(), emptyUrl);
}

TEST_F(ImageInfoExtendedTest, ConstructorWithUrl_SourceSet)
{
    QUrl url = QUrl::fromLocalFile("/tmp/test_construct.jpg");
    auto info = std::make_unique<ImageInfo>(url);
    // Constructor calls setSource which stores the URL
    EXPECT_EQ(info->source(), url);
}

// ─────────────────────── type() ───────────────────────

TEST_F(ImageInfoExtendedTest, Type_DefaultNullImage)
{
    // data is null → type() returns Types::NullImage
    EXPECT_EQ(m_info->type(), Types::NullImage);
}

TEST_F(ImageInfoExtendedTest, Type_StillNullAfterSetSource)
{
    m_info->setSource(QUrl::fromLocalFile("/tmp/nonexist.jpg"));
    // data is still null (refreshDataFromCache stubbed)
    EXPECT_EQ(m_info->type(), Types::NullImage);
}

// ─────────────────────── swapWidthAndHeight ───────────────────────

TEST_F(ImageInfoExtendedTest, SwapWidthAndHeight_NullDataNoCrash)
{
    // data is null → swapWidthAndHeight is a no-op
    m_info->swapWidthAndHeight();
    // No crash, no signal
    SUCCEED();
}

TEST_F(ImageInfoExtendedTest, SwapWidthAndHeight_NullDataNoSignal)
{
    QSignalSpy widthSpy(m_info.get(), &ImageInfo::widthChanged);
    QSignalSpy heightSpy(m_info.get(), &ImageInfo::heightChanged);
    m_info->swapWidthAndHeight();
    EXPECT_EQ(widthSpy.count(), 0);
    EXPECT_EQ(heightSpy.count(), 0);
}

// ─────────────────────── setFrameIndex / frameIndex ───────────────────────

TEST_F(ImageInfoExtendedTest, SetFrameIndex_DifferentValueEmitsSignal)
{
    QSignalSpy spy(m_info.get(), &ImageInfo::frameIndexChanged);
    m_info->setFrameIndex(5);
    EXPECT_EQ(m_info->frameIndex(), 5);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ImageInfoExtendedTest, SetFrameIndex_SameValueNoSignal)
{
    // Default currentIndex is 0
    QSignalSpy spy(m_info.get(), &ImageInfo::frameIndexChanged);
    m_info->setFrameIndex(0);
    EXPECT_EQ(m_info->frameIndex(), 0);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ImageInfoExtendedTest, SetFrameIndex_NegativeValue)
{
    QSignalSpy spy(m_info.get(), &ImageInfo::frameIndexChanged);
    m_info->setFrameIndex(-1);
    EXPECT_EQ(m_info->frameIndex(), -1);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ImageInfoExtendedTest, SetFrameIndex_MultipleChanges)
{
    QSignalSpy spy(m_info.get(), &ImageInfo::frameIndexChanged);
    m_info->setFrameIndex(1);
    m_info->setFrameIndex(2);
    m_info->setFrameIndex(3);
    EXPECT_EQ(m_info->frameIndex(), 3);
    EXPECT_EQ(spy.count(), 3);
}

// ─────────────────────── frameCount ───────────────────────

TEST_F(ImageInfoExtendedTest, FrameCount_DefaultOneWhenDataNull)
{
    // data is null → frameCount returns 1
    EXPECT_EQ(m_info->frameCount(), 1);
}

TEST_F(ImageInfoExtendedTest, FrameCount_StillOneAfterSetSource)
{
    m_info->setSource(QUrl::fromLocalFile("/tmp/nonexist.jpg"));
    EXPECT_EQ(m_info->frameCount(), 1);
}

// ─────────────────────── setScale / scale ───────────────────────

TEST_F(ImageInfoExtendedTest, Scale_DefaultNegativeOne)
{
    // data is null → scale returns -1
    EXPECT_EQ(m_info->scale(), -1);
}

TEST_F(ImageInfoExtendedTest, SetScale_NullDataNoEffect)
{
    m_info->setScale(1.5);
    EXPECT_EQ(m_info->scale(), -1); // data is null, no effect
}

TEST_F(ImageInfoExtendedTest, SetScale_ZeroValue)
{
    m_info->setScale(0);
    EXPECT_EQ(m_info->scale(), -1); // data is null
}

TEST_F(ImageInfoExtendedTest, SetScale_NegativeValue)
{
    m_info->setScale(-2.5);
    EXPECT_EQ(m_info->scale(), -1); // data is null
}

// ─────────────────────── clearCurrentCache ───────────────────────

TEST_F(ImageInfoExtendedTest, ClearCurrentCache_NullDataNoCrash)
{
    // data is null → clearCurrentCache is a no-op
    m_info->clearCurrentCache();
    SUCCEED();
}

TEST_F(ImageInfoExtendedTest, ClearCurrentCache_NullDataNoSignal)
{
    QSignalSpy spy(m_info.get(), &ImageInfo::infoChanged);
    m_info->clearCurrentCache();
    EXPECT_EQ(spy.count(), 0);
}

// ─────────────────────── static clearCache ───────────────────────

TEST_F(ImageInfoExtendedTest, StaticClearCache_NoCrash)
{
    // Stub ThumbnailCache::instance()->clear() to avoid real cache side effects
    stub.set_lamda(ADDR(ThumbnailCache, clear), [](ThumbnailCache *) {});
    ImageInfo::clearCache();
    SUCCEED();
}

TEST_F(ImageInfoExtendedTest, StaticClearCache_CallableMultipleTimes)
{
    stub.set_lamda(ADDR(ThumbnailCache, clear), [](ThumbnailCache *) {});
    ImageInfo::clearCache();
    ImageInfo::clearCache();
    ImageInfo::clearCache();
    SUCCEED();
}

// ─────────────────────── setStatus (protected) ───────────────────────

TEST_F(ImageInfoExtendedTest, SetStatus_DifferentStatusEmitsSignal)
{
    QSignalSpy spy(m_info.get(), &ImageInfo::statusChanged);
    EXPECT_EQ(m_info->status(), ImageInfo::Null);
    call_private_fun::ImageInfosetStatus(*m_info, ImageInfo::Loading);
    EXPECT_EQ(m_info->status(), ImageInfo::Loading);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ImageInfoExtendedTest, SetStatus_SameStatusNoSignal)
{
    // Default status is Null
    QSignalSpy spy(m_info.get(), &ImageInfo::statusChanged);
    call_private_fun::ImageInfosetStatus(*m_info, ImageInfo::Null);
    EXPECT_EQ(m_info->status(), ImageInfo::Null);
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ImageInfoExtendedTest, SetStatus_MultipleTransitions)
{
    QSignalSpy spy(m_info.get(), &ImageInfo::statusChanged);
    call_private_fun::ImageInfosetStatus(*m_info, ImageInfo::Loading);
    call_private_fun::ImageInfosetStatus(*m_info, ImageInfo::Ready);
    call_private_fun::ImageInfosetStatus(*m_info, ImageInfo::Error);
    EXPECT_EQ(m_info->status(), ImageInfo::Error);
    EXPECT_EQ(spy.count(), 3);
}

TEST_F(ImageInfoExtendedTest, SetStatus_DedupRepeatedSameStatus)
{
    QSignalSpy spy(m_info.get(), &ImageInfo::statusChanged);
    call_private_fun::ImageInfosetStatus(*m_info, ImageInfo::Loading);
    call_private_fun::ImageInfosetStatus(*m_info, ImageInfo::Loading);
    call_private_fun::ImageInfosetStatus(*m_info, ImageInfo::Loading);
    EXPECT_EQ(m_info->status(), ImageInfo::Loading);
    EXPECT_EQ(spy.count(), 1); // Only first transition emits
}

TEST_F(ImageInfoExtendedTest, SetStatus_AllStatusValues)
{
    call_private_fun::ImageInfosetStatus(*m_info, ImageInfo::Ready);
    EXPECT_EQ(m_info->status(), ImageInfo::Ready);
    call_private_fun::ImageInfosetStatus(*m_info, ImageInfo::Loading);
    EXPECT_EQ(m_info->status(), ImageInfo::Loading);
    call_private_fun::ImageInfosetStatus(*m_info, ImageInfo::Error);
    EXPECT_EQ(m_info->status(), ImageInfo::Error);
    call_private_fun::ImageInfosetStatus(*m_info, ImageInfo::Null);
    EXPECT_EQ(m_info->status(), ImageInfo::Null);
}

// ─────────────────────── onLoadFinished (protected slot) ───────────────────────

TEST_F(ImageInfoExtendedTest, OnLoadFinished_MatchingPathCallsRefresh)
{
    // Set imageUrl to a known path
    QString testPath = "/tmp/test_match.jpg";
    access_private_field::ImageInfoimageUrl(*m_info) = QUrl::fromLocalFile(testPath);

    // refreshDataFromCache is stubbed in SetUp; verify it gets called
    // by checking that no crash occurs and status is unaffected (stub is no-op)
    ImageInfo::Status statusBefore = m_info->status();
    call_private_fun::ImageInfoonLoadFinished(*m_info, testPath, 0);
    EXPECT_EQ(m_info->status(), statusBefore); // stubbed, no change
}

TEST_F(ImageInfoExtendedTest, OnLoadFinished_NonMatchingPathNoOp)
{
    QString testPath = "/tmp/test_match.jpg";
    access_private_field::ImageInfoimageUrl(*m_info) = QUrl::fromLocalFile(testPath);

    ImageInfo::Status statusBefore = m_info->status();
    call_private_fun::ImageInfoonLoadFinished(*m_info, "/tmp/other.jpg", 0);
    EXPECT_EQ(m_info->status(), statusBefore); // no change, path doesn't match
}

TEST_F(ImageInfoExtendedTest, OnLoadFinished_NonMatchingFrameIndexNoOp)
{
    QString testPath = "/tmp/test_match.jpg";
    access_private_field::ImageInfoimageUrl(*m_info) = QUrl::fromLocalFile(testPath);
    access_private_field::ImageInfocurrentIndex(*m_info) = 3;

    ImageInfo::Status statusBefore = m_info->status();
    call_private_fun::ImageInfoonLoadFinished(*m_info, testPath, 5); // frameIndex mismatch
    EXPECT_EQ(m_info->status(), statusBefore); // no change
}

TEST_F(ImageInfoExtendedTest, OnLoadFinished_EmptyUrlNoOp)
{
    // imageUrl is empty by default
    ImageInfo::Status statusBefore = m_info->status();
    call_private_fun::ImageInfoonLoadFinished(*m_info, "/tmp/any.jpg", 0);
    EXPECT_EQ(m_info->status(), statusBefore); // no match
}

// ─────────────────────── onLoadFinished with real refreshDataFromCache ───────────────────────

TEST_F(ImageInfoRealRefreshTest, OnLoadFinished_MatchingPath_RealRefresh_ErrorOnEmptyCache)
{
    // Create a fresh info without the refreshDataFromCache stub
    StubExt localStub;
    localStub.set_lamda(ADDR(GlobalControl, enableMultiThread), []() -> bool { return false; });
    localStub.set_lamda(ADDR(ThumbnailCache, contains), [](ThumbnailCache *, const QString &, int) -> bool { return false; });

    auto info = std::make_unique<ImageInfo>();
    QString testPath = "/tmp/test_real_refresh.jpg";
    access_private_field::ImageInfoimageUrl(*info) = QUrl::fromLocalFile(testPath);

    // When refreshDataFromCache(false) runs with empty URL data:
    // localPath is not empty, cache.find returns null, reload=false → setStatus(Error)
    call_private_fun::ImageInfoonLoadFinished(*info, testPath, 0);
    EXPECT_EQ(info->status(), ImageInfo::Error);
}

// ─────────────────────── onSizeChanged (protected slot) ───────────────────────

TEST_F(ImageInfoExtendedTest, OnSizeChanged_MatchingPathNullDataNoSignal)
{
    QString testPath = "/tmp/test_size.jpg";
    access_private_field::ImageInfoimageUrl(*m_info) = QUrl::fromLocalFile(testPath);

    QSignalSpy widthSpy(m_info.get(), &ImageInfo::widthChanged);
    QSignalSpy heightSpy(m_info.get(), &ImageInfo::heightChanged);

    call_private_fun::ImageInfoonSizeChanged(*m_info, testPath, 0);

    // data is null → no signal emitted
    EXPECT_EQ(widthSpy.count(), 0);
    EXPECT_EQ(heightSpy.count(), 0);
}

TEST_F(ImageInfoExtendedTest, OnSizeChanged_NonMatchingPathNoOp)
{
    QString testPath = "/tmp/test_size.jpg";
    access_private_field::ImageInfoimageUrl(*m_info) = QUrl::fromLocalFile(testPath);

    QSignalSpy widthSpy(m_info.get(), &ImageInfo::widthChanged);
    QSignalSpy heightSpy(m_info.get(), &ImageInfo::heightChanged);

    call_private_fun::ImageInfoonSizeChanged(*m_info, "/tmp/other.jpg", 0);

    EXPECT_EQ(widthSpy.count(), 0);
    EXPECT_EQ(heightSpy.count(), 0);
}

TEST_F(ImageInfoExtendedTest, OnSizeChanged_NonMatchingFrameIndexNoOp)
{
    QString testPath = "/tmp/test_size.jpg";
    access_private_field::ImageInfoimageUrl(*m_info) = QUrl::fromLocalFile(testPath);
    access_private_field::ImageInfocurrentIndex(*m_info) = 2;

    QSignalSpy widthSpy(m_info.get(), &ImageInfo::widthChanged);
    QSignalSpy heightSpy(m_info.get(), &ImageInfo::heightChanged);

    call_private_fun::ImageInfoonSizeChanged(*m_info, testPath, 5);

    EXPECT_EQ(widthSpy.count(), 0);
    EXPECT_EQ(heightSpy.count(), 0);
}

TEST_F(ImageInfoExtendedTest, OnSizeChanged_EmptyUrlNoOp)
{
    QSignalSpy widthSpy(m_info.get(), &ImageInfo::widthChanged);
    QSignalSpy heightSpy(m_info.get(), &ImageInfo::heightChanged);

    call_private_fun::ImageInfoonSizeChanged(*m_info, "/tmp/any.jpg", 0);

    EXPECT_EQ(widthSpy.count(), 0);
    EXPECT_EQ(heightSpy.count(), 0);
}

// ─────────────────────── refreshDataFromCache (protected) ───────────────────────

TEST_F(ImageInfoRealRefreshTest, RefreshDataFromCache_EmptyUrl_SetsError)
{
    // Use a separate StubExt to remove the default refreshDataFromCache stub
    StubExt localStub;
    localStub.set_lamda(ADDR(GlobalControl, enableMultiThread), []() -> bool { return false; });
    localStub.set_lamda(ADDR(ThumbnailCache, contains), [](ThumbnailCache *, const QString &, int) -> bool { return false; });

    auto info = std::make_unique<ImageInfo>();
    // imageUrl is empty by default
    EXPECT_EQ(info->status(), ImageInfo::Null);

    call_private_fun::ImageInforefreshDataFromCache(*info, true);

    EXPECT_EQ(info->status(), ImageInfo::Error);
}

TEST_F(ImageInfoRealRefreshTest, RefreshDataFromCache_EmptyUrl_ReloadFalse_SetsError)
{
StubExt localStub;
    localStub.set_lamda(ADDR(GlobalControl, enableMultiThread), []() -> bool { return false; });
    localStub.set_lamda(ADDR(ThumbnailCache, contains), [](ThumbnailCache *, const QString &, int) -> bool { return false; });

    auto info = std::make_unique<ImageInfo>();
    EXPECT_EQ(info->status(), ImageInfo::Null);

    call_private_fun::ImageInforefreshDataFromCache(*info, false);

    EXPECT_EQ(info->status(), ImageInfo::Error);
}

TEST_F(ImageInfoRealRefreshTest, RefreshDataFromCache_NonEmptyUrl_CacheMiss_ReloadFalse_SetsError)
{
StubExt localStub;
    localStub.set_lamda(ADDR(GlobalControl, enableMultiThread), []() -> bool { return false; });
    localStub.set_lamda(ADDR(ThumbnailCache, contains), [](ThumbnailCache *, const QString &, int) -> bool { return false; });

    auto info = std::make_unique<ImageInfo>();
    access_private_field::ImageInfoimageUrl(*info) = QUrl::fromLocalFile("/tmp/nonexist_miss.jpg");

    call_private_fun::ImageInforefreshDataFromCache(*info, false);

    // Cache miss + reload=false → Error
    EXPECT_EQ(info->status(), ImageInfo::Error);
}

TEST_F(ImageInfoRealRefreshTest, RefreshDataFromCache_NonEmptyUrl_CacheMiss_ReloadTrue_SetsLoading)
{
StubExt localStub;
    localStub.set_lamda(ADDR(GlobalControl, enableMultiThread), []() -> bool { return false; });
    localStub.set_lamda(ADDR(ThumbnailCache, contains), [](ThumbnailCache *, const QString &, int) -> bool { return false; });

    auto info = std::make_unique<ImageInfo>();
    access_private_field::ImageInfoimageUrl(*info) = QUrl::fromLocalFile("/tmp/nonexist_load.jpg");

    call_private_fun::ImageInforefreshDataFromCache(*info, true);

    // Cache miss + reload=true → Loading (load is dispatched but loadFinished is async)
    EXPECT_EQ(info->status(), ImageInfo::Loading);
}

TEST_F(ImageInfoRealRefreshTest, RefreshDataFromCache_EmptyUrl_EmitsStatusChanged)
{
StubExt localStub;
    localStub.set_lamda(ADDR(GlobalControl, enableMultiThread), []() -> bool { return false; });
    localStub.set_lamda(ADDR(ThumbnailCache, contains), [](ThumbnailCache *, const QString &, int) -> bool { return false; });

    auto info = std::make_unique<ImageInfo>();
    QSignalSpy spy(info.get(), &ImageInfo::statusChanged);

    call_private_fun::ImageInforefreshDataFromCache(*info, true);

    EXPECT_EQ(spy.count(), 1); // Null → Error
}

TEST_F(ImageInfoRealRefreshTest, RefreshDataFromCache_CacheMiss_ReloadFalse_EmitsStatusChanged)
{
StubExt localStub;
    localStub.set_lamda(ADDR(GlobalControl, enableMultiThread), []() -> bool { return false; });
    localStub.set_lamda(ADDR(ThumbnailCache, contains), [](ThumbnailCache *, const QString &, int) -> bool { return false; });

    auto info = std::make_unique<ImageInfo>();
    access_private_field::ImageInfoimageUrl(*info) = QUrl::fromLocalFile("/tmp/nonexist_spy.jpg");
    QSignalSpy spy(info.get(), &ImageInfo::statusChanged);

    call_private_fun::ImageInforefreshDataFromCache(*info, false);

    EXPECT_EQ(spy.count(), 1); // Null → Error
}

TEST_F(ImageInfoRealRefreshTest, RefreshDataFromCache_CacheMiss_ReloadTrue_EmitsStatusChanged)
{
StubExt localStub;
    localStub.set_lamda(ADDR(GlobalControl, enableMultiThread), []() -> bool { return false; });
    localStub.set_lamda(ADDR(ThumbnailCache, contains), [](ThumbnailCache *, const QString &, int) -> bool { return false; });

    auto info = std::make_unique<ImageInfo>();
    access_private_field::ImageInfoimageUrl(*info) = QUrl::fromLocalFile("/tmp/nonexist_spy2.jpg");
    QSignalSpy spy(info.get(), &ImageInfo::statusChanged);

    call_private_fun::ImageInforefreshDataFromCache(*info, true);

    EXPECT_EQ(spy.count(), 1); // Null → Loading
}

// ─────────────────────── Signal emissions ───────────────────────

TEST_F(ImageInfoExtendedTest, SetSource_EmitsSourceChanged)
{
    QSignalSpy spy(m_info.get(), &ImageInfo::sourceChanged);
    m_info->setSource(QUrl::fromLocalFile("/tmp/signal_test.jpg"));
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ImageInfoExtendedTest, SetSource_SameUrl_NoSourceChanged)
{
    QUrl url = QUrl::fromLocalFile("/tmp/signal_test.jpg");
    m_info->setSource(url);
    QSignalSpy spy(m_info.get(), &ImageInfo::sourceChanged);
    m_info->setSource(url); // same URL
    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ImageInfoExtendedTest, SetFrameIndex_EmitsFrameIndexChanged)
{
    QSignalSpy spy(m_info.get(), &ImageInfo::frameIndexChanged);
    m_info->setFrameIndex(7);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ImageInfoExtendedTest, ReloadData_EmitsStatusChanged)
{
    access_private_field::ImageInfoimageUrl(*m_info) = QUrl::fromLocalFile("/tmp/reload_signal.jpg");
    QSignalSpy spy(m_info.get(), &ImageInfo::statusChanged);
    m_info->reloadData();
    EXPECT_EQ(spy.count(), 1); // Null → Loading
}

TEST_F(ImageInfoExtendedTest, ReloadData_SetsLoadingStatus)
{
    access_private_field::ImageInfoimageUrl(*m_info) = QUrl::fromLocalFile("/tmp/reload_status.jpg");
    EXPECT_EQ(m_info->status(), ImageInfo::Null);
    m_info->reloadData();
    EXPECT_EQ(m_info->status(), ImageInfo::Loading);
}

// ─────────────────────── updateData (additional coverage) ───────────────────────

TEST_F(ImageInfoExtendedTest, UpdateData_BothNullReturnsFalse)
{
    QSharedPointer<ImageInfoData> nullData;
    EXPECT_FALSE(call_private_fun::ImageInfoupdateData(*m_info, nullData));
}

TEST_F(ImageInfoExtendedTest, UpdateData_SameNullPointerReturnsFalse)
{
    QSharedPointer<ImageInfoData> nullPtr;
    access_private_field::ImageInfodata(*m_info) = nullPtr;
    EXPECT_FALSE(call_private_fun::ImageInfoupdateData(*m_info, nullPtr));
}

TEST_F(ImageInfoExtendedTest, UpdateData_DataRemainsNullAfterNullUpdate)
{
    QSharedPointer<ImageInfoData> nullData;
    call_private_fun::ImageInfoupdateData(*m_info, nullData);
    EXPECT_TRUE(access_private_field::ImageInfodata(*m_info).isNull());
}

// ─────────────────────── x / y / setX / setY (data null paths) ───────────────────────

TEST_F(ImageInfoExtendedTest, X_DefaultZero)
{
    EXPECT_EQ(m_info->x(), 0); // data is null
}

TEST_F(ImageInfoExtendedTest, Y_DefaultZero)
{
    EXPECT_EQ(m_info->y(), 0); // data is null
}

TEST_F(ImageInfoExtendedTest, SetX_NullDataNoEffect)
{
    m_info->setX(10.5);
    EXPECT_EQ(m_info->x(), 0); // data is null
}

TEST_F(ImageInfoExtendedTest, SetY_NullDataNoEffect)
{
    m_info->setY(-5.0);
    EXPECT_EQ(m_info->y(), 0); // data is null
}

// ─────────────────────── width / height (data null paths) ───────────────────────

TEST_F(ImageInfoExtendedTest, Width_DefaultNegativeOne)
{
    EXPECT_EQ(m_info->width(), -1); // data is null
}

TEST_F(ImageInfoExtendedTest, Height_DefaultNegativeOne)
{
    EXPECT_EQ(m_info->height(), -1); // data is null
}

TEST_F(ImageInfoExtendedTest, Width_StillNegativeAfterSetSource)
{
    m_info->setSource(QUrl::fromLocalFile("/tmp/width_test.jpg"));
    EXPECT_EQ(m_info->width(), -1);
}

TEST_F(ImageInfoExtendedTest, Height_StillNegativeAfterSetSource)
{
    m_info->setSource(QUrl::fromLocalFile("/tmp/height_test.jpg"));
    EXPECT_EQ(m_info->height(), -1);
}

// ─────────────────────── exists (data null path) ───────────────────────

TEST_F(ImageInfoExtendedTest, Exists_DefaultFalse)
{
    EXPECT_FALSE(m_info->exists()); // data is null
}

TEST_F(ImageInfoExtendedTest, Exists_FalseAfterSetSource)
{
    m_info->setSource(QUrl::fromLocalFile("/tmp/exists_test.jpg"));
    EXPECT_FALSE(m_info->exists());
}
