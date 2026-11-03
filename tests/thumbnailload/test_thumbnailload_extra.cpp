// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | ThumbnailLoad::requestImage | high | complexity:5 | 3 | 2 |
// | ThumbnailLoad::requestPixmap | mid | complexity:3 | 1 | 1 |
// | ThumbnailLoad::imageIsNull | mid | complexity:3 | 2 | 2 |
// | ThumbnailLoad::removeImageCache | mid | complexity:2 | 1 | 1 |
// | ViewLoad::requestImage | high | complexity:7 | 3 | 3 |
// | ViewLoad::requestPixmap | mid | complexity:4 | 1 | 2 |
// | ViewLoad::getImageWidth/Height | mid | complexity:2 | 1 | 1 |
// | ViewLoad::getFitWindowScale | mid | complexity:4 | 2 | 2 |
// | ViewLoad::removeImageCache | mid | complexity:3 | 1 | 1 |
// | ViewLoad::reloadImageCache | mid | complexity:4 | 1 | 1 |
// | MultiImageLoad::requestImage | high | complexity:9 | 3 | 4 |
// | MultiImageLoad::requestPixmap | mid | complexity:2 | 1 | 1 |
// | MultiImageLoad::getImageWidth/Height | mid | complexity:3 | 1 | 1 |
// | MultiImageLoad::getFitWindowScale | mid | complexity:4 | 1 | 1 |
// | MultiImageLoad::removeImageCache | mid | complexity:4 | 1 | 1 |
// | ImagePublisher::getLoadMode | mid | complexity:2 | 1 | 1 |
// | ImagePublisher::switchLoadMode | high | complexity:5 | 3 | 3 |
// | AsyncImageProviderAlbum::getLoadMode | mid | complexity:2 | 1 | 1 |
// | AsyncImageProviderAlbum::switchLoadMode | high | complexity:5 | 3 | 3 |
// ─── 生成后填入 actual 列，低于 min 即违规 ───

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QUrl>
#include <memory>

#include "thumbnailload.h"
#include "configsetter.h"
#include "unionimage/unionimage.h"
#include "utils/devicehelper.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// Access private fields for ImagePublisher
ACCESS_PRIVATE_FIELD(ImagePublisher, std::atomic_int, m_loadMode)

// Access private fields for AsyncImageProviderAlbum
ACCESS_PRIVATE_FIELD(AsyncImageProviderAlbum, std::atomic_int, m_loadMode)

class ThumbnailLoadExtraTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_thumb_extra";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    StubExt stub;
    int loadCallCount = 0;

    void SetUp() override
    {
        stub.clear();
        loadCallCount = 0;

        // Stub loadStaticImageFromFile to avoid real file deps
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, loadStaticImageFromFile),
            [this](const QString &, QImage &res, QString &, const QString &) -> bool {
                res = QImage(200, 100, QImage::Format_RGB32);
                res.fill(Qt::red);
                loadCallCount++;
                return true;
            });

        // Stub localPath to normalize URLs
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, localPath),
            [](const QUrl &url) -> QString {
                QString path = url.toLocalFile();
                return path.isEmpty() ? url.toString() : path;
            });

        // Stub LibConfigSetter to avoid real settings access
        stub.set_lamda(ADDR(LibConfigSetter, value),
            [](LibConfigSetter *, const QString &, const QString &,
               const QVariant &defaultValue) -> QVariant { return defaultValue; });
        stub.set_lamda(ADDR(LibConfigSetter, setValue),
            [](LibConfigSetter *, const QString &, const QString &, const QVariant &) -> void {});
    }

    static QImage makeImage(int w, int h, Qt::GlobalColor color)
    {
        QImage img(w, h, QImage::Format_RGB32);
        img.fill(color);
        return img;
    }
};

// =============================================================================
// ThumbnailLoad tests
// =============================================================================

// B1: First request loads and caches; second request hits cache
TEST_F(ThumbnailLoadExtraTest, ThumbnailLoad_RequestImage_FirstLoadThenCacheHit)
{
    ThumbnailLoad loader;
    QString path = "/test/thumb_load.png";

    QImage img1 = loader.requestImage(path, nullptr, QSize());
    ASSERT_FALSE(img1.isNull());
    EXPECT_EQ(loadCallCount, 1);

    // Second request should hit cache (no new load)
    QImage img2 = loader.requestImage(path, nullptr, QSize());
    ASSERT_FALSE(img2.isNull());
    EXPECT_EQ(loadCallCount, 1); // still 1
}

// B2: requestPixmap always loads (no cache)
TEST_F(ThumbnailLoadExtraTest, ThumbnailLoad_RequestPixmap_ReturnsPixmap)
{
    ThumbnailLoad loader;
    QString path = "/test/thumb_pixmap.png";

    QPixmap pix = loader.requestPixmap(path, nullptr, QSize());
    EXPECT_FALSE(pix.isNull());
}

// B3: imageIsNull — cached non-null image → false
TEST_F(ThumbnailLoadExtraTest, ThumbnailLoad_ImageIsNull_CachedNonNull_ReturnsFalse)
{
    ThumbnailLoad loader;
    QString path = "/test/thumb_isnull.png";
    loader.requestImage(path, nullptr, QSize());
    EXPECT_FALSE(loader.imageIsNull(path));
}

// B4: imageIsNull — not cached path → false
TEST_F(ThumbnailLoadExtraTest, ThumbnailLoad_ImageIsNull_NotCached_ReturnsFalse)
{
    ThumbnailLoad loader;
    EXPECT_FALSE(loader.imageIsNull("/test/not_cached.png"));
}

// B5: removeImageCache removes from m_imgMap
TEST_F(ThumbnailLoadExtraTest, ThumbnailLoad_RemoveImageCache_RemovesEntry)
{
    ThumbnailLoad loader;
    QString path = "/test/thumb_remove.png";
    loader.requestImage(path, nullptr, QSize());
    EXPECT_TRUE(loader.m_imgMap.keys().contains(path));

    loader.removeImageCache(path);
    EXPECT_FALSE(loader.m_imgMap.keys().contains(path));
}

// =============================================================================
// ViewLoad tests
// =============================================================================

// B1: New path → loads, records m_currentPath and m_imgSizes
TEST_F(ThumbnailLoadExtraTest, ViewLoad_RequestImage_NewPath_LoadsAndRecords)
{
    ViewLoad loader;
    QString path = "/test/view_new.png";
    QSize size;
    QImage result = loader.requestImage(path, &size, QSize());
    ASSERT_FALSE(result.isNull());
    EXPECT_EQ(loader.m_currentPath, path);
    EXPECT_EQ(loader.m_imgSizes[path], QSize(200, 100));
}

// B2: Same path with valid requestedSize → scales cached image
TEST_F(ThumbnailLoadExtraTest, ViewLoad_RequestImage_SamePath_ScalesCached)
{
    ViewLoad loader;
    QString path = "/test/view_same.png";
    loader.requestImage(path, nullptr, QSize());

    // Second request with same path but different requestedSize
    QImage result = loader.requestImage(path, nullptr, QSize(50, 50));
    ASSERT_FALSE(result.isNull());
    EXPECT_EQ(result.size(), QSize(50, 50));
}

// B3: Same path with zero requestedSize → returns cached as-is
TEST_F(ThumbnailLoadExtraTest, ViewLoad_RequestImage_SamePath_ZeroRequestedSize)
{
    ViewLoad loader;
    QString path = "/test/view_zero.png";
    loader.requestImage(path, nullptr, QSize());

    QImage result = loader.requestImage(path, nullptr, QSize());
    ASSERT_FALSE(result.isNull());
    // Should be the original 200x100 image
    EXPECT_EQ(result.size(), QSize(200, 100));
}

// B4: requestPixmap — same path returns cached
TEST_F(ThumbnailLoadExtraTest, ViewLoad_RequestPixmap_SamePath_ReturnsCached)
{
    ViewLoad loader;
    QString path = "/test/view_pixmap_same.png";
    loader.requestPixmap(path, nullptr, QSize());

    // Second call with same path should return cached
    QPixmap pix2 = loader.requestPixmap(path, nullptr, QSize());
    EXPECT_FALSE(pix2.isNull());
}

// B5: requestPixmap — different path loads new
TEST_F(ThumbnailLoadExtraTest, ViewLoad_RequestPixmap_DifferentPath_LoadsNew)
{
    ViewLoad loader;
    loader.requestPixmap("/test/view_pixmap_a.png", nullptr, QSize());

    QPixmap pix2 = loader.requestPixmap("/test/view_pixmap_b.png", nullptr, QSize());
    EXPECT_FALSE(pix2.isNull());
    EXPECT_EQ(loader.m_currentPath, QString("/test/view_pixmap_b.png"));
}

// B6: getImageWidth/Height returns cached size, uncached returns -1 (default QSize)
TEST_F(ThumbnailLoadExtraTest, ViewLoad_GetImageWidthHeight_ReturnsCachedSize)
{
    ViewLoad loader;
    QString path = "/test/view_wh.png";
    loader.requestImage(path, nullptr, QSize());

    EXPECT_EQ(loader.getImageWidth(path), 200);
    EXPECT_EQ(loader.getImageHeight(path), 100);

    // Uncached path returns 0
    EXPECT_EQ(loader.getImageWidth("/test/uncached.png"), -1); // default QSize(-1,-1)
    EXPECT_EQ(loader.getImageHeight("/test/uncached.png"), -1); // default QSize(-1,-1)
}

// B7: getFitWindowScale — width-dominated
TEST_F(ThumbnailLoadExtraTest, ViewLoad_GetFitWindowScale_WidthDominated)
{
    ViewLoad loader;
    QString path = "/test/view_scale_w.png";
    loader.requestImage(path, nullptr, QSize());

    // image 200x100, window 100x100 → scaleW=2.0, scaleH=1.0 → max=2.0
    double scale = loader.getFitWindowScale(path, 100, 100);
    EXPECT_DOUBLE_EQ(scale, 2.0);
}

// B8: getFitWindowScale — height-dominated
TEST_F(ThumbnailLoadExtraTest, ViewLoad_GetFitWindowScale_HeightDominated)
{
    ViewLoad loader;
    QString path = "/test/view_scale_h.png";

    // Use a tall image stub
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, loadStaticImageFromFile),
        [](const QString &, QImage &res, QString &, const QString &) -> bool {
            res = QImage(100, 200, QImage::Format_RGB32);
            res.fill(Qt::blue);
            return true;
        });

    loader.requestImage(path, nullptr, QSize());

    // image 100x200, window 100x100 → scaleW=1.0, scaleH=2.0 → max=2.0
    double scale = loader.getFitWindowScale(path, 100, 100);
    EXPECT_DOUBLE_EQ(scale, 2.0);
}

// B9: removeImageCache clears m_currentPath when matches
TEST_F(ThumbnailLoadExtraTest, ViewLoad_RemoveImageCache_ClearsCurrentPath)
{
    ViewLoad loader;
    QString path = "/test/view_remove.png";
    loader.requestImage(path, nullptr, QSize());
    EXPECT_EQ(loader.m_currentPath, path);

    loader.removeImageCache(path);
    EXPECT_TRUE(loader.m_currentPath.isEmpty());
    EXPECT_FALSE(loader.m_imgSizes.contains(path));
}

// B10: reloadImageCache updates size and syncs m_Img when current
TEST_F(ThumbnailLoadExtraTest, ViewLoad_ReloadImageCache_UpdatesSizeAndImg)
{
    ViewLoad loader;
    QString path = "/test/view_reload.png";
    loader.requestImage(path, nullptr, QSize());
    EXPECT_EQ(loader.m_imgSizes[path], QSize(200, 100));

    // Stub to return different size
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, loadStaticImageFromFile),
        [](const QString &, QImage &res, QString &, const QString &) -> bool {
            res = QImage(400, 200, QImage::Format_RGB32);
            res.fill(Qt::green);
            return true;
        });

    loader.reloadImageCache(path);
    EXPECT_EQ(loader.m_imgSizes[path], QSize(400, 200));
    EXPECT_EQ(loader.m_Img.size(), QSize(400, 200));
}

// =============================================================================
// MultiImageLoad tests
// =============================================================================

// B1: Invalid id (no #frame_ tag) → returns empty QImage
TEST_F(ThumbnailLoadExtraTest, MultiImageLoad_RequestImage_InvalidId_ReturnsEmpty)
{
    MultiImageLoad loader;
    QString id = "/test/multi_invalid.png";
    QImage result = loader.requestImage(id, nullptr, QSize());
    EXPECT_TRUE(result.isNull());
}

// B2: Valid frame id with successful jumpToImage and read
TEST_F(ThumbnailLoadExtraTest, MultiImageLoad_RequestImage_ValidFrame_ReturnsImage)
{
    StubExt readerStub;
    readerStub.set_lamda(ADDR(QImageReader, jumpToImage),
        [](QImageReader *, int) -> bool { return true; });
    readerStub.set_lamda(static_cast<QImage (QImageReader::*)()>(&QImageReader::read),
        [](QImageReader *) -> QImage {
            QImage img(100, 100, QImage::Format_RGB32);
            img.fill(Qt::red);
            return img;
        });
    readerStub.set_lamda(ADDR(QImageReader, canRead),
        [](QImageReader *) -> bool { return true; });

    MultiImageLoad loader;
    QString id = "/test/multi_valid.tif#frame_0";
    QImage result = loader.requestImage(id, nullptr, QSize());
    EXPECT_FALSE(result.isNull());
}

// B3: _thumbnail suffix → uses cached thumbnail
TEST_F(ThumbnailLoadExtraTest, MultiImageLoad_RequestImage_ThumbnailSuffix_UsesCache)
{
    StubExt readerStub;
    readerStub.set_lamda(ADDR(QImageReader, jumpToImage),
        [](QImageReader *, int) -> bool { return true; });
    readerStub.set_lamda(static_cast<QImage (QImageReader::*)()>(&QImageReader::read),
        [](QImageReader *) -> QImage {
            QImage img(200, 200, QImage::Format_RGB32);
            img.fill(Qt::green);
            return img;
        });
    readerStub.set_lamda(ADDR(QImageReader, canRead),
        [](QImageReader *) -> bool { return true; });

    MultiImageLoad loader;
    QString path = "/test/multi_thumb.tif";

    // First request loads and caches
    QString id1 = path + "#frame_0";
    QImage img1 = loader.requestImage(id1, nullptr, QSize());
    ASSERT_FALSE(img1.isNull());

    // Second request with _thumbnail suffix should return cached
    QString id2 = path + "#frame_0_thumbnail";
    QImage img2 = loader.requestImage(id2, nullptr, QSize());
    EXPECT_FALSE(img2.isNull());
}

// B4: jumpToImage failure → returns empty, no crash
TEST_F(ThumbnailLoadExtraTest, MultiImageLoad_RequestImage_JumpFailure_ReturnsEmpty)
{
    StubExt readerStub;
    readerStub.set_lamda(ADDR(QImageReader, jumpToImage),
        [](QImageReader *, int) -> bool { return false; });
    readerStub.set_lamda(ADDR(QImageReader, canRead),
        [](QImageReader *) -> bool { return true; });

    MultiImageLoad loader;
    QString id = "/test/multi_jumpfail.tif#frame_5";
    QImage result = loader.requestImage(id, nullptr, QSize());
    EXPECT_TRUE(result.isNull());
}

// B5: requestPixmap returns pixmap consistent with requestImage
TEST_F(ThumbnailLoadExtraTest, MultiImageLoad_RequestPixmap_ReturnsPixmap)
{
    StubExt readerStub;
    readerStub.set_lamda(ADDR(QImageReader, jumpToImage),
        [](QImageReader *, int) -> bool { return true; });
    readerStub.set_lamda(static_cast<QImage (QImageReader::*)()>(&QImageReader::read),
        [](QImageReader *) -> QImage {
            QImage img(100, 100, QImage::Format_RGB32);
            img.fill(Qt::red);
            return img;
        });
    readerStub.set_lamda(ADDR(QImageReader, canRead),
        [](QImageReader *) -> bool { return true; });

    MultiImageLoad loader;
    QString id = "/test/multi_pixmap.tif#frame_0";
    QPixmap pix = loader.requestPixmap(id, nullptr, QSize());
    EXPECT_FALSE(pix.isNull());
}

// B6: getImageWidth/Height returns cached origin size, uncached returns 0
TEST_F(ThumbnailLoadExtraTest, MultiImageLoad_GetImageWidthHeight_ReturnsCachedSize)
{
    StubExt readerStub;
    readerStub.set_lamda(ADDR(QImageReader, jumpToImage),
        [](QImageReader *, int) -> bool { return true; });
    readerStub.set_lamda(static_cast<QImage (QImageReader::*)()>(&QImageReader::read),
        [](QImageReader *) -> QImage {
            QImage img(200, 100, QImage::Format_RGB32);
            img.fill(Qt::red);
            return img;
        });
    readerStub.set_lamda(ADDR(QImageReader, canRead),
        [](QImageReader *) -> bool { return true; });

    MultiImageLoad loader;
    QString path = "/test/multi_wh.tif";

    // Load to populate cache
    loader.requestImage(path + "#frame_0", nullptr, QSize());

    EXPECT_EQ(loader.getImageWidth(path, 0), 200);
    EXPECT_EQ(loader.getImageHeight(path, 0), 100);

    // Uncached frame returns 0
    EXPECT_EQ(loader.getImageWidth(path, 99), 0);
    EXPECT_EQ(loader.getImageHeight(path, 99), 0);
}

// B7: getFitWindowScale returns max(scaleW, scaleH)
TEST_F(ThumbnailLoadExtraTest, MultiImageLoad_GetFitWindowScale_ReturnsMaxScale)
{
    StubExt readerStub;
    readerStub.set_lamda(ADDR(QImageReader, jumpToImage),
        [](QImageReader *, int) -> bool { return true; });
    readerStub.set_lamda(static_cast<QImage (QImageReader::*)()>(&QImageReader::read),
        [](QImageReader *) -> QImage {
            QImage img(200, 100, QImage::Format_RGB32);
            img.fill(Qt::red);
            return img;
        });
    readerStub.set_lamda(ADDR(QImageReader, canRead),
        [](QImageReader *) -> bool { return true; });

    MultiImageLoad loader;
    QString path = "/test/multi_scale.tif";
    loader.requestImage(path + "#frame_0", nullptr, QSize());

    // image 200x100, window 100x100 → scaleW=2.0, scaleH=1.0 → max=2.0
    double scale = loader.getFitWindowScale(path, 100, 100, 0);
    EXPECT_DOUBLE_EQ(scale, 2.0);
}

// B8: removeImageCache removes all frames for path
TEST_F(ThumbnailLoadExtraTest, MultiImageLoad_RemoveImageCache_RemovesAllFrames)
{
    StubExt readerStub;
    readerStub.set_lamda(ADDR(QImageReader, jumpToImage),
        [](QImageReader *, int) -> bool { return true; });
    readerStub.set_lamda(static_cast<QImage (QImageReader::*)()>(&QImageReader::read),
        [](QImageReader *) -> QImage {
            QImage img(100, 100, QImage::Format_RGB32);
            img.fill(Qt::red);
            return img;
        });
    readerStub.set_lamda(ADDR(QImageReader, canRead),
        [](QImageReader *) -> bool { return true; });

    MultiImageLoad loader;
    QString path = "/test/multi_remove.tif";

    loader.requestImage(path + "#frame_0", nullptr, QSize());
    loader.requestImage(path + "#frame_1", nullptr, QSize());

    EXPECT_EQ(loader.getImageWidth(path, 0), 100);
    EXPECT_EQ(loader.getImageWidth(path, 1), 100);

    loader.removeImageCache(path);

    EXPECT_EQ(loader.getImageWidth(path, 0), 0);
    EXPECT_EQ(loader.getImageWidth(path, 1), 0);
}

// =============================================================================
// ImagePublisher loadMode tests
// =============================================================================

// B1: Default loadMode is 0
TEST_F(ThumbnailLoadExtraTest, ImagePublisher_GetLoadMode_DefaultIsZero)
{
    ImagePublisher publisher;
    EXPECT_EQ(publisher.getLoadMode(), 0);
}

// B2: switchLoadMode 0→1
TEST_F(ThumbnailLoadExtraTest, ImagePublisher_SwitchLoadMode_ZeroToOne)
{
    ImagePublisher publisher;
    EXPECT_EQ(publisher.getLoadMode(), 0);
    publisher.switchLoadMode();
    EXPECT_EQ(publisher.getLoadMode(), 1);
}

// B3: switchLoadMode 1→0
TEST_F(ThumbnailLoadExtraTest, ImagePublisher_SwitchLoadMode_OneToZero)
{
    ImagePublisher publisher;
    // Force mode to 1 first
    auto &mode = access_private_field::ImagePublisherm_loadMode(publisher);
    mode = 1;
    EXPECT_EQ(publisher.getLoadMode(), 1);
    publisher.switchLoadMode();
    EXPECT_EQ(publisher.getLoadMode(), 0);
}

// B4: switchLoadMode from invalid value → 0
TEST_F(ThumbnailLoadExtraTest, ImagePublisher_SwitchLoadMode_InvalidToZero)
{
    ImagePublisher publisher;
    auto &mode = access_private_field::ImagePublisherm_loadMode(publisher);
    mode = 5;
    publisher.switchLoadMode();
    EXPECT_EQ(publisher.getLoadMode(), 0);
}

// =============================================================================
// AsyncImageProviderAlbum loadMode tests
// =============================================================================

// B5: Default loadMode is 0
TEST_F(ThumbnailLoadExtraTest, AsyncImageProviderAlbum_GetLoadMode_DefaultIsZero)
{
    AsyncImageProviderAlbum provider;
    EXPECT_EQ(provider.getLoadMode(), 0);
}

// B6: switchLoadMode 0→1
TEST_F(ThumbnailLoadExtraTest, AsyncImageProviderAlbum_SwitchLoadMode_ZeroToOne)
{
    AsyncImageProviderAlbum provider;
    EXPECT_EQ(provider.getLoadMode(), 0);
    provider.switchLoadMode();
    EXPECT_EQ(provider.getLoadMode(), 1);
}

// B7: switchLoadMode 1→0
TEST_F(ThumbnailLoadExtraTest, AsyncImageProviderAlbum_SwitchLoadMode_OneToZero)
{
    AsyncImageProviderAlbum provider;
    auto &mode = access_private_field::AsyncImageProviderAlbumm_loadMode(provider);
    mode = 1;
    EXPECT_EQ(provider.getLoadMode(), 1);
    provider.switchLoadMode();
    EXPECT_EQ(provider.getLoadMode(), 0);
}

// B8: switchLoadMode from invalid value → 0
TEST_F(ThumbnailLoadExtraTest, AsyncImageProviderAlbum_SwitchLoadMode_InvalidToZero)
{
    AsyncImageProviderAlbum provider;
    auto &mode = access_private_field::AsyncImageProviderAlbumm_loadMode(provider);
    mode = 5;
    provider.switchLoadMode();
    EXPECT_EQ(provider.getLoadMode(), 0);
}
