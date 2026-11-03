// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | ProviderCache::rotateImageCached | high | complexity:7 | 3 | 6 |
// | ProviderCache::removeImageCache | mid | complexity:4 | 2 | 1 |
// | ProviderCache::clearCache | mid | complexity:2 | 1 | 1 |
// | ThumbnailProvider::requestImage | high | complexity:8 | 3 | 5 |
// | ThumbnailProvider::requestPixmap | mid | complexity:3 | 1 | 1 |

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QImage>
#include <QTemporaryDir>
#include <QUrl>
#include <memory>

#include "imagedata/imageprovider.h"
#include "imagedata/thumbnailcache.h"
#include "unionimage/unionimage.h"
#include "utils/devicehelper.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// Subclass to access protected members of ProviderCache
class TestProviderCache : public ProviderCache
{
public:
    using ProviderCache::imageCache;
    using ProviderCache::lastRotatePath;
    using ProviderCache::lastRotateImage;
    using ProviderCache::lastRotation;
    using ProviderCache::mutex;
};

class ProviderCacheTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_provider_cache";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    StubExt stub;

    void SetUp() override
    {
        stub.clear();
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, rotateImage),
            [](int, QImage &) -> bool { return true; });
    }

    void TearDown() override
    {
        ThumbnailCache::instance()->clear();
    }

    static QImage makeImage(int w, int h, Qt::GlobalColor color)
    {
        QImage img(w, h, QImage::Format_RGB32);
        img.fill(color);
        return img;
    }
};

// === ProviderCache constructor/destructor ===
TEST_F(ProviderCacheTest, ConstructorDestructor_NoCrash)
{
    { TestProviderCache cache; }
    SUCCEED();
}

// B1: angle=0 → early return, no state change
TEST_F(ProviderCacheTest, RotateImageCached_ZeroAngle_NoStateChange)
{
    TestProviderCache cache;
    cache.imageCache.add("/test/zero.png", 0, makeImage(100, 100, Qt::red));
    cache.rotateImageCached(0, "/test/zero.png");
    EXPECT_TRUE(cache.lastRotatePath.isEmpty());
    EXPECT_EQ(cache.lastRotation, 0);
}

// B2: first rotation → records lastRotatePath/lastRotateImage/lastRotation
TEST_F(ProviderCacheTest, RotateImageCached_FirstRotation_RecordsState)
{
    TestProviderCache cache;
    cache.imageCache.add("/test/first.png", 0, makeImage(200, 100, Qt::blue));
    cache.rotateImageCached(90, "/test/first.png");
    EXPECT_EQ(cache.lastRotatePath, "/test/first.png");
    EXPECT_FALSE(cache.lastRotateImage.isNull());
    EXPECT_EQ(cache.lastRotation, 90);
    EXPECT_FALSE(cache.imageCache.get("/test/first.png", 0).isNull());
}

// B3: same path again → accumulates angle, no re-fetch
TEST_F(ProviderCacheTest, RotateImageCached_SamePath_AccumulatesAngle)
{
    TestProviderCache cache;
    cache.imageCache.add("/test/acc.png", 0, makeImage(200, 100, Qt::green));
    cache.rotateImageCached(90, "/test/acc.png");
    EXPECT_EQ(cache.lastRotation, 90);
    cache.rotateImageCached(90, "/test/acc.png");
    EXPECT_EQ(cache.lastRotation, 180);
}

// B4: accumulated 360 → skip rotation, still update cache
TEST_F(ProviderCacheTest, RotateImageCached_Accumulated360_SkipRotation_UpdateCache)
{
    TestProviderCache cache;
    cache.imageCache.add("/test/360.png", 0, makeImage(100, 100, Qt::yellow));
    cache.rotateImageCached(180, "/test/360.png");
    EXPECT_EQ(cache.lastRotation, 180);
    cache.rotateImageCached(180, "/test/360.png");
    EXPECT_EQ(cache.lastRotation, 360);
    EXPECT_FALSE(cache.imageCache.get("/test/360.png", 0).isNull());
}

// B5: different path → resets state
TEST_F(ProviderCacheTest, RotateImageCached_DifferentPath_ResetsState)
{
    TestProviderCache cache;
    cache.imageCache.add("/test/pathA.png", 0, makeImage(100, 100, Qt::red));
    cache.imageCache.add("/test/pathB.png", 0, makeImage(200, 200, Qt::blue));
    cache.rotateImageCached(90, "/test/pathA.png");
    EXPECT_EQ(cache.lastRotatePath, "/test/pathA.png");
    cache.rotateImageCached(45, "/test/pathB.png");
    EXPECT_EQ(cache.lastRotatePath, "/test/pathB.png");
    EXPECT_EQ(cache.lastRotation, 45);
}

// B6: null image in cache → no crash
TEST_F(ProviderCacheTest, RotateImageCached_NullImage_NoCrash)
{
    TestProviderCache cache;
    cache.rotateImageCached(90, "/test/nonexistent.png");
    SUCCEED();
}

// === removeImageCache: removes only specified path ===
TEST_F(ProviderCacheTest, RemoveImageCache_RemovesOnlySpecifiedPath)
{
    TestProviderCache cache;
    cache.imageCache.add("/test/keep.png", 0, makeImage(100, 100, Qt::red));
    cache.imageCache.add("/test/remove.png", 0, makeImage(200, 200, Qt::blue));
    cache.imageCache.add("/test/remove.png", 1, makeImage(300, 300, Qt::green));
    cache.removeImageCache("/test/remove.png");
    EXPECT_FALSE(cache.imageCache.get("/test/keep.png", 0).isNull());
    EXPECT_TRUE(cache.imageCache.get("/test/remove.png", 0).isNull());
    EXPECT_TRUE(cache.imageCache.get("/test/remove.png", 1).isNull());
}

// === clearCache: clears all state ===
TEST_F(ProviderCacheTest, ClearCache_ClearsAllState)
{
    TestProviderCache cache;
    cache.imageCache.add("/test/clear.png", 0, makeImage(100, 100, Qt::red));
    cache.lastRotatePath = "/test/clear.png";
    cache.lastRotateImage = makeImage(100, 100, Qt::blue);
    cache.lastRotation = 90;
    cache.clearCache();
    EXPECT_TRUE(cache.imageCache.keys().isEmpty());
    EXPECT_TRUE(cache.lastRotatePath.isEmpty());
    EXPECT_TRUE(cache.lastRotateImage.isNull());
}

// === ThumbnailProvider tests ===

class ThumbnailProviderTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_thumb_provider";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    StubExt stub;
    QTemporaryDir dir;
    QString filePath;
    QString fileUrl;

    void SetUp() override
    {
        stub.clear();
        ASSERT_TRUE(dir.isValid());
        filePath = dir.filePath("test.png");
        fileUrl = QUrl::fromLocalFile(filePath).toString();
        QImage img(200, 100, QImage::Format_RGB32);
        img.fill(Qt::red);
        ASSERT_TRUE(img.save(filePath, "PNG"));
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, loadStaticImageFromFile),
            [](const QString &, QImage &res, QString &, const QString &) -> bool {
                res = QImage(200, 100, QImage::Format_RGB32);
                res.fill(Qt::red);
                return true;
            });
        // Stub DeviceHelper::tryBeginRead to allow device reads in tests
        stub.set_lamda(ADDR(DeviceHelper, tryBeginRead),
            [](const QString &) -> bool { return true; });
    }

    void TearDown() override
    {
        ThumbnailCache::instance()->clear();
    }
};

// B1: single-frame image → decode, scale to 100x100, cache, size backfill
TEST_F(ThumbnailProviderTest, RequestImage_SingleFrame_LoadsAndCaches)
{
    ThumbnailProvider provider;
    QSize size;
    QImage result = provider.requestImage(fileUrl, &size, QSize());
    ASSERT_FALSE(result.isNull());
    EXPECT_EQ(size, QSize(200, 100));
    EXPECT_TRUE(ThumbnailCache::instance()->contains(filePath, 0));
}

// B2: cache hit → returns cached image without decoding
TEST_F(ThumbnailProviderTest, RequestImage_CacheHit_ReturnsCached)
{
    QImage cachedImg(50, 50, QImage::Format_RGB32);
    cachedImg.fill(Qt::green);
    ThumbnailCache::instance()->add(filePath, 0, cachedImg);
    ThumbnailProvider provider;
    QImage result = provider.requestImage(fileUrl, nullptr, QSize());
    ASSERT_FALSE(result.isNull());
    EXPECT_EQ(result.pixelColor(0, 0), QColor(Qt::green));
}

// B3: valid requestedSize → scales result
TEST_F(ThumbnailProviderTest, RequestImage_ValidRequestedSize_Scales)
{
    ThumbnailProvider provider;
    QImage result = provider.requestImage(fileUrl, nullptr, QSize(50, 50));
    ASSERT_FALSE(result.isNull());
    EXPECT_LE(result.width(), 50);
    EXPECT_LE(result.height(), 50);
}

// B4: DeviceReadGuard inactive → returns empty image
TEST_F(ThumbnailProviderTest, RequestImage_DeviceInactive_ReturnsEmpty)
{
    stub.set_lamda(ADDR(DeviceHelper, tryBeginRead),
        [](const QString &) -> bool { return false; });
    ThumbnailProvider provider;
    QImage result = provider.requestImage(fileUrl, nullptr, QSize());
    EXPECT_TRUE(result.isNull());
}

// B5: multi-frame id with frame > 0 on single-frame file → no crash, returns null
TEST_F(ThumbnailProviderTest, RequestImage_MultiFrameId_JumpFailure_NoCrash)
{
    ThumbnailProvider provider;
    QString id = fileUrl + "#frame_1";
    QSize size;
    QImage result = provider.requestImage(id, &size, QSize());
    EXPECT_TRUE(result.isNull());
}

// B6: requestPixmap → returns non-null pixmap consistent with requestImage
TEST_F(ThumbnailProviderTest, RequestPixmap_ReturnsNonNull)
{
    ThumbnailProvider provider;
    QPixmap result = provider.requestPixmap(fileUrl, nullptr, QSize());
    EXPECT_FALSE(result.isNull());
}
