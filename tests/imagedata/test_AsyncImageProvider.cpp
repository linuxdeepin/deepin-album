// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | AsyncImageProvider::requestImageResponse | high | complexity:6 | 3 | 6 |
// | AsyncImageProvider::preloadImage | mid | complexity:4 | 2 | 2 |
// | ProviderCache::preloadImage | low | complexity:1 | 1 | 1 |
// | ProviderCache::rotateImageCached (frameIndex) | high | complexity:7 | 1 | 3 |
// | ProviderCache::removeImageCache (edge) | mid | complexity:4 | 1 | 2 |
// | ThumbnailProvider::requestImage (edge) | high | complexity:8 | 1 | 2 |
// | ThumbnailProvider::requestPixmap (edge) | mid | complexity:3 | 1 | 1 |

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QUrl>
#include <memory>

#include "imagedata/imageprovider.h"
#include "imagedata/thumbnailcache.h"
#include "unionimage/unionimage.h"
#include "utils/devicehelper.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// Subclass to access protected members of ProviderCache via AsyncImageProvider
class TestableAsyncImageProvider : public AsyncImageProvider
{
public:
    using ProviderCache::imageCache;
    using ProviderCache::lastRotatePath;
    using ProviderCache::lastRotateImage;
    using ProviderCache::lastRotation;
};

// Subclass to access protected members of ProviderCache directly
class TestableProviderCache : public ProviderCache
{
public:
    using ProviderCache::imageCache;
    using ProviderCache::lastRotatePath;
    using ProviderCache::lastRotateImage;
    using ProviderCache::lastRotation;
};

/*
 * Branch list for AsyncImageResponse::run():
 * B1: tempPath.isEmpty() -> emit finished, return            | case: RequestImageResponse_EmptyPath_EmitsFinished
 * B2: !readGuard.isActive() -> emit finished, return         | case: RequestImageResponse_DeviceInactive_EmitsFinished
 * B3: cache miss, frameIndex==0 -> readNormalImageScaled     | case: RequestImageResponse_CacheMiss_LoadsAndCaches
 * B4: cache miss, frameIndex!=0 -> readMultiImage            | case: RequestImageResponse_FrameId_ParsesFrameIndex
 * B5: loaded non-null -> cache.add                           | case: RequestImageResponse_CacheMiss_LoadsAndCaches
 * B6: cache hit, !imageMeetsRequestedSize -> remove+reload   | (covered by test_ImageProvider.cpp TEST_P)
 * B7: cache hit, meets size, image > requestedSize -> scale  | (covered by test_ImageProvider.cpp TEST_P)
 * B8: emit finished (always)                                 | all cases
 *
 * Branch list for ProviderCache::rotateImageCached (supplement to test_ProviderCache.cpp):
 * B7: frameIndex != 0 -> operates on specific frame          | case: RotateImageCached_FrameIndex_RotatesCorrectFrame
 * B8: cache miss → null image, state recorded, rotation skipped  | case: RotateImageCached_CacheMissNullImage_RecordsStateNoCrash
 * B9: accumulation beyond 360 -> modulo non-zero, rotates    | case: RotateImageCached_AccumulateBeyond360_Rotates
 */

class AsyncImageProviderTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_async_provider";
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

        // Create a real image file for tests that read from disk via QImageReader
        QImage img(200, 100, QImage::Format_RGB32);
        img.fill(Qt::red);
        ASSERT_TRUE(img.save(filePath, "PNG"));

        // Stub rotateImage to avoid actual pixel rotation
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, rotateImage),
            [](int, QImage &) -> bool { return true; });
        // Default stub: DeviceHelper allows reads
        stub.set_lamda(ADDR(DeviceHelper, tryBeginRead),
            [](const QString &) -> bool { return true; });
    }

    void TearDown() override
    {
        stub.clear();
        ThumbnailCache::instance()->clear();
    }

    static QImage makeImage(int w, int h, Qt::GlobalColor color)
    {
        QImage img(w, h, QImage::Format_RGB32);
        img.fill(color);
        return img;
    }

    // Helper: wait for async response to complete and return its image
    QImage waitAndExtractImage(QQuickImageResponse *response)
    {
        QThreadPool::globalInstance()->waitForDone();
        QCoreApplication::processEvents();
        std::unique_ptr<QQuickTextureFactory> factory(response->textureFactory());
        if (factory) {
            return factory->image();
        }
        return QImage();
    }
};

// === AsyncImageProvider::requestImageResponse ===

// B8: response creation → returns non-null
TEST_F(AsyncImageProviderTest, RequestImageResponse_ValidUrl_ReturnsNonNullResponse)
{
    // Arrange
    TestableAsyncImageProvider provider;

    // Act
    QQuickImageResponse *response = provider.requestImageResponse(fileUrl, QSize(100, 100));

    // Assert
    EXPECT_NE(response, nullptr);
    QThreadPool::globalInstance()->waitForDone();
    QCoreApplication::processEvents();
    std::unique_ptr<QQuickTextureFactory> factory(response->textureFactory());
    EXPECT_NE(factory, nullptr);
    delete response;
}

// B1: empty path → response finishes with null image
TEST_F(AsyncImageProviderTest, RequestImageResponse_EmptyPath_EmitsFinished_NullImage)
{
    // Arrange
    TestableAsyncImageProvider provider;

    // Act
    std::unique_ptr<QQuickImageResponse> response(provider.requestImageResponse("", QSize(100, 100)));
    QSignalSpy spy(response.get(), &QQuickImageResponse::finished);
    QThreadPool::globalInstance()->waitForDone();
    QCoreApplication::processEvents();

    // Assert
    EXPECT_EQ(spy.count(), 1);
    QImage result = waitAndExtractImage(response.get());
    EXPECT_TRUE(result.isNull());
}

// B2: device inactive → response finishes with null image
TEST_F(AsyncImageProviderTest, RequestImageResponse_DeviceInactive_EmitsFinished_NullImage)
{
    // Arrange
    stub.set_lamda(ADDR(DeviceHelper, tryBeginRead),
        [](const QString &) -> bool { return false; });
    TestableAsyncImageProvider provider;

    // Act
    std::unique_ptr<QQuickImageResponse> response(provider.requestImageResponse(fileUrl, QSize(100, 100)));
    QSignalSpy spy(response.get(), &QQuickImageResponse::finished);
    QThreadPool::globalInstance()->waitForDone();
    QCoreApplication::processEvents();

    // Assert
    EXPECT_EQ(spy.count(), 1);
    QImage result = waitAndExtractImage(response.get());
    EXPECT_TRUE(result.isNull());
}

// B3+B5: cache miss → loads from file via QImageReader, caches result
TEST_F(AsyncImageProviderTest, RequestImageResponse_CacheMiss_LoadsAndCaches)
{
    // Arrange
    TestableAsyncImageProvider provider;

    // Act
    std::unique_ptr<QQuickImageResponse> response(provider.requestImageResponse(fileUrl, QSize()));
    QSignalSpy spy(response.get(), &QQuickImageResponse::finished);
    QThreadPool::globalInstance()->waitForDone();
    QCoreApplication::processEvents();

    // Assert
    EXPECT_EQ(spy.count(), 1);
    QImage result = waitAndExtractImage(response.get());
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.size(), QSize(200, 100));
    EXPECT_FALSE(provider.imageCache.get(filePath, 0).isNull());
}

// Cache hit → returns cached image without loading from disk
TEST_F(AsyncImageProviderTest, RequestImageResponse_CacheHit_ReturnsCached)
{
    // Arrange
    TestableAsyncImageProvider provider;
    QImage cached(108, 192, QImage::Format_RGB32);
    cached.fill(Qt::green);
    provider.imageCache.add(filePath, 0, cached);

    // Act
    std::unique_ptr<QQuickImageResponse> response(provider.requestImageResponse(fileUrl, QSize()));
    QSignalSpy spy(response.get(), &QQuickImageResponse::finished);
    QThreadPool::globalInstance()->waitForDone();
    QCoreApplication::processEvents();

    // Assert
    EXPECT_EQ(spy.count(), 1);
    QImage result = waitAndExtractImage(response.get());
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.pixelColor(0, 0), QColor(Qt::green));
}

// B4: frame id with #frame_1 → parses frame index, readMultiImage fails on single-frame PNG
TEST_F(AsyncImageProviderTest, RequestImageResponse_FrameId_ParsesFrameIndex)
{
    // Arrange
    TestableAsyncImageProvider provider;
    QString frameUrl = fileUrl + "#frame_1";

    // Act
    std::unique_ptr<QQuickImageResponse> response(provider.requestImageResponse(frameUrl, QSize()));
    QSignalSpy spy(response.get(), &QQuickImageResponse::finished);
    QThreadPool::globalInstance()->waitForDone();
    QCoreApplication::processEvents();

    // Assert
    EXPECT_EQ(spy.count(), 1);
    // Single-frame PNG cannot jump to frame 1 → null image, not cached
    QImage result = waitAndExtractImage(response.get());
    EXPECT_TRUE(result.isNull());
    EXPECT_TRUE(provider.imageCache.get(filePath, 1).isNull());
}

// === AsyncImageProvider::preloadImage ===

// Preload → caches image after thread pool completes
TEST_F(AsyncImageProviderTest, PreloadImage_ValidFile_CachesImageAfterLoad)
{
    // Arrange
    TestableAsyncImageProvider provider;

    // Act
    provider.preloadImage(fileUrl);
    QThreadPool::globalInstance()->waitForDone();
    QCoreApplication::processEvents();

    // Assert
    QImage cached = provider.imageCache.get(filePath, 0);
    EXPECT_FALSE(cached.isNull());
    EXPECT_EQ(cached.size(), QSize(200, 100));
}

// Preload with non-existent file → no crash, cache stays empty
TEST_F(AsyncImageProviderTest, PreloadImage_NonExistentFile_NoCrash_EmptyCache)
{
    // Arrange
    TestableAsyncImageProvider provider;
    QString badPath = dir.filePath("nonexistent.png");

    // Act
    provider.preloadImage(QUrl::fromLocalFile(badPath).toString());
    QThreadPool::globalInstance()->waitForDone();
    QCoreApplication::processEvents();

    // Assert
    QImage cached = provider.imageCache.get(badPath, 0);
    EXPECT_TRUE(cached.isNull());
    EXPECT_EQ(provider.imageCache.keys().size(), 0);
}

// === ProviderCache::preloadImage (base class no-op) ===

TEST_F(AsyncImageProviderTest, PreloadImage_BaseNoOp_NoCrash_NoStateChange)
{
    // Arrange
    TestableProviderCache cache;

    // Act
    cache.preloadImage("/test/preload.png");

    // Assert
    EXPECT_TRUE(cache.imageCache.get("/test/preload.png", 0).isNull());
    EXPECT_EQ(cache.imageCache.keys().size(), 0);
}

// === ProviderCache::rotateImageCached with frameIndex (supplement) ===

// B7: frameIndex != 0 → operates on specific frame, not frame 0
TEST_F(AsyncImageProviderTest, RotateImageCached_FrameIndex_RotatesCorrectFrame)
{
    // Arrange
    TestableProviderCache cache;
    cache.imageCache.add("/test/multi.tif", 1, makeImage(200, 100, Qt::blue));
    cache.imageCache.add("/test/multi.tif", 0, makeImage(100, 100, Qt::red));

    // Act
    cache.rotateImageCached(90, "/test/multi.tif", 1);

    // Assert
    EXPECT_EQ(cache.lastRotatePath, "/test/multi.tif");
    EXPECT_EQ(cache.lastRotation, 90);
    EXPECT_FALSE(cache.imageCache.get("/test/multi.tif", 1).isNull());
    // Frame 0 should be untouched
    QImage frame0 = cache.imageCache.get("/test/multi.tif", 0);
    EXPECT_EQ(frame0.size(), QSize(100, 100));
}

// B8: cache miss → null image, state recorded, rotation skipped, no crash
TEST_F(AsyncImageProviderTest, RotateImageCached_CacheMissNullImage_RecordsStateNoCrash)
{
    // Arrange
    TestableProviderCache cache;

    // Act
    cache.rotateImageCached(90, "/test/nonexistent.png");

    // Assert
    // imagePath != lastRotatePath → if branch: cache miss yields a null image,
    // state is still recorded, and rotateImage is skipped since image.isNull()
    EXPECT_EQ(cache.lastRotatePath, "/test/nonexistent.png");
    EXPECT_EQ(cache.lastRotation, 90);
    EXPECT_TRUE(cache.lastRotateImage.isNull());
    EXPECT_TRUE(cache.imageCache.get("/test/nonexistent.png", 0).isNull());
}

// B9: accumulation beyond 360 → modulo non-zero, rotation still executes
TEST_F(AsyncImageProviderTest, RotateImageCached_AccumulateBeyond360_RotatesByModulo)
{
    // Arrange
    TestableProviderCache cache;
    cache.imageCache.add("/test/acc360.png", 0, makeImage(200, 100, Qt::green));
    cache.rotateImageCached(270, "/test/acc360.png");
    EXPECT_EQ(cache.lastRotation, 270);

    // Act
    cache.rotateImageCached(180, "/test/acc360.png");

    // Assert
    // 270 + 180 = 450; 450 % 360 = 90 != 0, so rotation executes
    EXPECT_EQ(cache.lastRotation, 450);
    EXPECT_FALSE(cache.imageCache.get("/test/acc360.png", 0).isNull());
}

// === ProviderCache::removeImageCache edge cases (supplement) ===

// Repeat call → no crash, cache already empty
TEST_F(AsyncImageProviderTest, RemoveImageCache_RepeatCall_NoCrash)
{
    // Arrange
    TestableProviderCache cache;
    cache.imageCache.add("/test/remove.png", 0, makeImage(100, 100, Qt::red));
    cache.removeImageCache("/test/remove.png");

    // Act
    cache.removeImageCache("/test/remove.png");

    // Assert
    EXPECT_TRUE(cache.imageCache.get("/test/remove.png", 0).isNull());
    EXPECT_EQ(cache.imageCache.keys().size(), 0);
}

// Non-existent path → no crash, existing entries preserved
TEST_F(AsyncImageProviderTest, RemoveImageCache_NonExistentPath_PreservesExisting)
{
    // Arrange
    TestableProviderCache cache;
    cache.imageCache.add("/test/keep.png", 0, makeImage(100, 100, Qt::red));

    // Act
    cache.removeImageCache("/test/nonexistent.png");

    // Assert
    EXPECT_FALSE(cache.imageCache.get("/test/keep.png", 0).isNull());
    EXPECT_EQ(cache.imageCache.keys().size(), 1);
}

// === Additional ThumbnailProvider edge cases (supplement to test_ProviderCache.cpp) ===

class ThumbnailProviderEdgeTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_thumb_edge";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    StubExt stub;
    QTemporaryDir dir;

    void SetUp() override
    {
        stub.clear();
        ASSERT_TRUE(dir.isValid());
        stub.set_lamda(ADDR(DeviceHelper, tryBeginRead),
            [](const QString &) -> bool { return true; });
    }

    void TearDown() override
    {
        stub.clear();
        ThumbnailCache::instance()->clear();
    }
};

// Non-existent file → loadStaticImageFromFile returns false → null image
TEST_F(ThumbnailProviderEdgeTest, RequestImage_NonExistentFile_ReturnsNull)
{
    // Arrange
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, loadStaticImageFromFile),
        [](const QString &, QImage &, QString &, const QString &) -> bool {
            return false;
        });
    ThumbnailProvider provider;
    QString path = dir.filePath("nonexistent.png");
    QString url = QUrl::fromLocalFile(path).toString();

    // Act
    QImage result = provider.requestImage(url, nullptr, QSize());

    // Assert
    EXPECT_TRUE(result.isNull());
    // Source adds even null thumbnails to the cache
    EXPECT_TRUE(ThumbnailCache::instance()->contains(path, 0));
    EXPECT_EQ(ThumbnailCache::instance()->get(path, 0).width(), 0);
}

// Invalid id (empty string) → no crash, returns null
TEST_F(ThumbnailProviderEdgeTest, RequestImage_EmptyId_NoCrash_ReturnsNull)
{
    // Arrange
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, loadStaticImageFromFile),
        [](const QString &, QImage &, QString &, const QString &) -> bool {
            return false;
        });
    ThumbnailProvider provider;

    // Act
    QImage result = provider.requestImage("", nullptr, QSize(50, 50));

    // Assert
    EXPECT_TRUE(result.isNull());
    EXPECT_EQ(result.width(), 0);
}

// requestPixmap with non-existent file → returns null pixmap
TEST_F(ThumbnailProviderEdgeTest, RequestPixmap_NonExistentFile_ReturnsNull)
{
    // Arrange
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, loadStaticImageFromFile),
        [](const QString &, QImage &, QString &, const QString &) -> bool {
            return false;
        });
    ThumbnailProvider provider;
    QString path = dir.filePath("nonexistent.png");
    QString url = QUrl::fromLocalFile(path).toString();

    // Act
    QPixmap result = provider.requestPixmap(url, nullptr, QSize());

    // Assert
    EXPECT_TRUE(result.isNull());
    EXPECT_EQ(result.width(), 0);
}
