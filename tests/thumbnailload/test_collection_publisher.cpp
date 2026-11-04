// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | CollectionPublisher::requestImage | high | complexity:7 | 3 | 8 |
// | CollectionPublisher::createYearImage | high | complexity:6 | 3 | 5 |
// | CollectionPublisher::createMonthCellImage | high | complexity:8 | 3 | 7 |
// | CollectionPublisher::clipHelper | high | complexity:6 | 3 | 5 |
// | ImagePublisher::addPadAndScaled | mid | complexity:3 | 2 | 4 |
// | ImagePublisher::requestImage | high | complexity:10 | 3 | 7 |
// | AsyncImageResponseAlbum::run | high | complexity:9 | 3 | 6 |
// | LoadImage::getFitWindowScale | mid | complexity:2 | 2 | 2 |
// | LoadImage::getImageWidth/Height | mid | complexity:3 | 2 | 3 |
// | LoadImage::getrealWidthHeightRatio | mid | complexity:3 | 2 | 3 |
// | LoadImage::imageIsNull | low | complexity:1 | 1 | 1 |
// | LoadImage::setMultiFrameIndex | low | complexity:1 | 1 | 2 |
// | LoadImage::setReverseHeightWidth | low | complexity:1 | 1 | 2 |
// | LoadImage::loadThumbnail | high | complexity:4 | 2 | 3 |
// | LoadImage::loadThumbnails | mid | complexity:2 | 1 | 2 |
// | LoadImage::onImageFileChanged | high | complexity:6 | 3 | 4 |
// ─── 生成后填入 actual 列，低于 min 即违规 ───

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QImage>
#include <QSignalSpy>
#include <QUrl>
#include <QVariant>

#include "thumbnailload.h"
#include "configsetter.h"
#include "unionimage/unionimage.h"
#include "utils/devicehelper.h"
#include "dbmanager/dbmanager.h"
#include "imageengine/movieservice.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// ---------------------------------------------------------------------------
// Private-method access declarations
// ---------------------------------------------------------------------------
ACCESS_PRIVATE_FUN(ImagePublisher, QImage(const QImage &), addPadAndScaled)
ACCESS_PRIVATE_FUN(CollectionPublisher, QImage(const QString &), createYearImage)
ACCESS_PRIVATE_FUN(CollectionPublisher, QImage(const QImage &, int, int), clipHelper)

// Private fields
ACCESS_PRIVATE_FIELD(ImagePublisher, std::atomic_int, m_loadMode)
ACCESS_PRIVATE_FIELD(LoadImage, int, m_FrameIndex)
ACCESS_PRIVATE_FIELD(LoadImage, bool, m_bReverseHeightWidth)

// ---------------------------------------------------------------------------
// Testable subclasses to expose protected requestImage
// ---------------------------------------------------------------------------
class TestableImagePublisher : public ImagePublisher
{
public:
    using ImagePublisher::ImagePublisher;
    using ImagePublisher::requestImage;
};

class TestableCollectionPublisher : public CollectionPublisher
{
public:
    using CollectionPublisher::CollectionPublisher;
    using CollectionPublisher::requestImage;
};

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------
class CollectionPublisherTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_collection_publisher";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    StubExt stub;

    int loadCallCount = 0;
    bool deviceReadable = true;
    bool isVideoResult = false;
    bool loadSuccess = true;

    void SetUp() override
    {
        stub.clear();
        loadCallCount = 0;
        deviceReadable = true;
        isVideoResult = false;
        loadSuccess = true;

        // Stub DeviceHelper::tryBeginRead
        stub.set_lamda(ADDR(DeviceHelper, tryBeginRead),
            [this](const QString &) -> bool { return deviceReadable; });

        // Stub loadStaticImageFromFile
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, loadStaticImageFromFile),
            [this](const QString &, QImage &res, QString &, const QString &) -> bool {
                if (loadSuccess) {
                    res = QImage(200, 100, QImage::Format_RGB888);
                    res.fill(Qt::red);
                    loadCallCount++;
                    return true;
                }
                return false;
            });

        // Stub localPath
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, localPath),
            [](const QUrl &url) -> QString {
                QString path = url.toLocalFile();
                return path.isEmpty() ? url.toString() : path;
            });

        // Stub isVideo
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, isVideo),
            [this](const QString &) -> bool { return isVideoResult; });

        // Stub LibConfigSetter
        stub.set_lamda(ADDR(LibConfigSetter, value),
            [](LibConfigSetter *, const QString &, const QString &,
               const QVariant &defaultValue) -> QVariant { return defaultValue; });
        stub.set_lamda(ADDR(LibConfigSetter, setValue),
            [](LibConfigSetter *, const QString &, const QString &, const QVariant &) -> void {});

        // Stub MovieService::getMovieCover
        stub.set_lamda(ADDR(MovieService, getMovieCover),
            [](MovieService *, const QUrl &) -> QImage {
                QImage img(80, 60, QImage::Format_RGB888);
                img.fill(Qt::blue);
                return img;
            });

        // Stub DBManager::getYearPaths
        stub.set_lamda(ADDR(DBManager, getYearPaths),
            [](DBManager *, const QString &, int) -> QStringList {
                return QStringList{"/fake/year/path.png"};
            });
    }

    static QImage makeImage(int w, int h, Qt::GlobalColor color)
    {
        QImage img(w, h, QImage::Format_RGB888);
        img.fill(color);
        return img;
    }
};

// ===========================================================================
// Section 1: CollectionPublisher::requestImage
// ===========================================================================

// B1: id length < 4 → return empty image, size not written (nullptr)
TEST_F(CollectionPublisherTest, RequestImage_ShortId_ReturnsEmpty_NullSize)
{
    TestableCollectionPublisher publisher;
    QImage result = publisher.requestImage("ab", nullptr, QSize());
    EXPECT_TRUE(result.isNull());
}

// B2: id length < 4 → return empty image, size written via valid pointer
TEST_F(CollectionPublisherTest, RequestImage_ShortId_ReturnsEmpty_ValidSize)
{
    TestableCollectionPublisher publisher;
    QSize sz(100, 100);
    QImage result = publisher.requestImage("ab", &sz, QSize());
    EXPECT_TRUE(result.isNull());
}

// B3: Year view branch: type == "Y"
TEST_F(CollectionPublisherTest, RequestImage_YearBranch_ReturnsNonEmpty)
{
    TestableCollectionPublisher publisher;
    QImage result = publisher.requestImage("random_Y_2022", nullptr, QSize());
    EXPECT_FALSE(result.isNull());
}

// B4: Month view branch: type == "M" with ImageSize 0 (Full)
TEST_F(CollectionPublisherTest, RequestImage_MonthBranch_FullSize)
{
    TestableCollectionPublisher publisher;
    QImage result = publisher.requestImage("random_M_2022_0", nullptr, QSize());
    EXPECT_FALSE(result.isNull());
}

// B5: Month view branch: ImageSize 3 (SplitBig)
TEST_F(CollectionPublisherTest, RequestImage_MonthBranch_SplitBigSize)
{
    TestableCollectionPublisher publisher;
    QImage result = publisher.requestImage("random_M_2022_3", nullptr, QSize());
    EXPECT_FALSE(result.isNull());
}

// B6: requestedSize both > 0 → scaled result
TEST_F(CollectionPublisherTest, RequestImage_ValidRequestedSize_Scaled)
{
    TestableCollectionPublisher publisher;
    QSize outSize;
    QImage result = publisher.requestImage("random_Y_2022", &outSize, QSize(100, 100));
    EXPECT_FALSE(result.isNull());
    EXPECT_LE(result.width(), 100);
    EXPECT_LE(result.height(), 100);
}

// B7: requestedSize with 0 width → return original unscaled
TEST_F(CollectionPublisherTest, RequestImage_ZeroRequestedSize_NoScaling)
{
    TestableCollectionPublisher publisher;
    QImage result = publisher.requestImage("random_Y_2022", nullptr, QSize(0, 100));
    EXPECT_FALSE(result.isNull());
}

// B8: size pointer receives correct size
TEST_F(CollectionPublisherTest, RequestImage_ValidSizePointer_Updated)
{
    TestableCollectionPublisher publisher;
    QSize outSize;
    QImage result = publisher.requestImage("random_Y_2022", &outSize, QSize());
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(outSize.width(), result.width());
    EXPECT_EQ(outSize.height(), result.height());
}

// ===========================================================================
// Section 1: CollectionPublisher::createYearImage (private)
// ===========================================================================

// B1: DBManager returns empty path list → return empty image
TEST_F(CollectionPublisherTest, CreateYearImage_EmptyPaths_ReturnsEmpty)
{
    stub.set_lamda(ADDR(DBManager, getYearPaths),
        [](DBManager *, const QString &, int) -> QStringList { return QStringList{}; });

    CollectionPublisher publisher;
    QImage result = call_private_fun::CollectionPublishercreateYearImage(publisher, "2022");
    EXPECT_TRUE(result.isNull());
}

// B2: DeviceReadGuard inactive → return empty image
TEST_F(CollectionPublisherTest, CreateYearImage_DeviceInactive_ReturnsEmpty)
{
    deviceReadable = false;

    CollectionPublisher publisher;
    QImage result = call_private_fun::CollectionPublishercreateYearImage(publisher, "2022");
    EXPECT_TRUE(result.isNull());
}

// B3: Video path → uses MovieService::getMovieCover
TEST_F(CollectionPublisherTest, CreateYearImage_VideoPath_UsesMovieCover)
{
    isVideoResult = true;

    CollectionPublisher publisher;
    QImage result = call_private_fun::CollectionPublishercreateYearImage(publisher, "2022");
    EXPECT_FALSE(result.isNull());
}

// B4: Load fails (loadStaticImageFromFile returns false) → return empty image
TEST_F(CollectionPublisherTest, CreateYearImage_LoadFails_ReturnsEmpty)
{
    loadSuccess = false;

    CollectionPublisher publisher;
    QImage result = call_private_fun::CollectionPublishercreateYearImage(publisher, "2022");
    EXPECT_TRUE(result.isNull());
}

// B5: Normal image load → scaled to outputWidth x outputHeight
TEST_F(CollectionPublisherTest, CreateYearImage_NormalLoad_ReturnsScaled)
{
    CollectionPublisher publisher;
    QImage result = call_private_fun::CollectionPublishercreateYearImage(publisher, "2022");
    EXPECT_FALSE(result.isNull());
    EXPECT_GE(result.width(), 1000);
    EXPECT_GE(result.height(), 618);
}

// ===========================================================================
// Section 1: CollectionPublisher::createMonthCellImage (private)
// Tested indirectly through requestImage with different ImageSize values (0-5)
// ===========================================================================

// B1: ImageSize_Full (0) → output through clipHelper
TEST_F(CollectionPublisherTest, CreateMonthCellImage_FullSize_CorrectOutput)
{
    TestableCollectionPublisher publisher;
    QImage result = publisher.requestImage("random_M_2022_0", nullptr, QSize());
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.width(), 1000);
    EXPECT_EQ(result.height(), 618);
}

// B2: ImageSize_Half (1) → half width
TEST_F(CollectionPublisherTest, CreateMonthCellImage_HalfSize_CorrectOutput)
{
    TestableCollectionPublisher publisher;
    QImage result = publisher.requestImage("random_M_2022_1", nullptr, QSize());
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.width(), 500);
    EXPECT_EQ(result.height(), 618);
}

// B3: ImageSize_Quarter (2) → quarter dimensions
TEST_F(CollectionPublisherTest, CreateMonthCellImage_QuarterSize_CorrectOutput)
{
    TestableCollectionPublisher publisher;
    QImage result = publisher.requestImage("random_M_2022_2", nullptr, QSize());
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.width(), 500);
    EXPECT_EQ(result.height(), 309);
}

// B4: ImageSize_SplitBig (3) → split big dimensions
TEST_F(CollectionPublisherTest, CreateMonthCellImage_SplitBigSize_CorrectOutput)
{
    TestableCollectionPublisher publisher;
    QImage result = publisher.requestImage("random_M_2022_3", nullptr, QSize());
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.width(), 1000);
    EXPECT_EQ(result.height(), static_cast<int>(618 * 0.618));
}

// B5: DeviceReadGuard inactive → return empty image
TEST_F(CollectionPublisherTest, CreateMonthCellImage_DeviceInactive_ReturnsEmpty)
{
    deviceReadable = false;

    TestableCollectionPublisher publisher;
    QImage result = publisher.requestImage("random_M_2022_0", nullptr, QSize());
    EXPECT_TRUE(result.isNull());
}

// B6: Video path → uses MovieService::getMovieCover
TEST_F(CollectionPublisherTest, CreateMonthCellImage_VideoPath_UsesMovieCover)
{
    isVideoResult = true;

    TestableCollectionPublisher publisher;
    QImage result = publisher.requestImage("random_M_2022_0", nullptr, QSize());
    EXPECT_FALSE(result.isNull());
}

// B7: Normal image → output through clipHelper with Format_RGB888
TEST_F(CollectionPublisherTest, CreateMonthCellImage_NormalImage_CorrectFormat)
{
    TestableCollectionPublisher publisher;
    QImage result = publisher.requestImage("random_M_2022_0", nullptr, QSize());
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.format(), QImage::Format_RGB888);
}

// ===========================================================================
// Section 1: CollectionPublisher::clipHelper (private)
// ===========================================================================

// B1: Wide image (width > height) → center crop
TEST_F(CollectionPublisherTest, ClipHelper_WideImage_CenterCrop)
{
    CollectionPublisher publisher;
    QImage wide = makeImage(400, 200, Qt::red);
    QImage result = call_private_fun::CollectionPublisherclipHelper(publisher, wide, 100, 100);
    EXPECT_EQ(result.width(), 100);
    EXPECT_EQ(result.height(), 100);
    EXPECT_EQ(result.format(), QImage::Format_RGB888);
}

// B2: Tall image (height > width) → center crop
TEST_F(CollectionPublisherTest, ClipHelper_TallImage_CenterCrop)
{
    CollectionPublisher publisher;
    QImage tall = makeImage(200, 400, Qt::green);
    QImage result = call_private_fun::CollectionPublisherclipHelper(publisher, tall, 100, 100);
    EXPECT_EQ(result.width(), 100);
    EXPECT_EQ(result.height(), 100);
    EXPECT_EQ(result.format(), QImage::Format_RGB888);
}

// B3: Square image → no offset needed for one dimension
TEST_F(CollectionPublisherTest, ClipHelper_SquareImage_CenterCrop)
{
    CollectionPublisher publisher;
    QImage square = makeImage(300, 300, Qt::blue);
    QImage result = call_private_fun::CollectionPublisherclipHelper(publisher, square, 100, 100);
    EXPECT_EQ(result.width(), 100);
    EXPECT_EQ(result.height(), 100);
    EXPECT_EQ(result.format(), QImage::Format_RGB888);
}

// B4: Target larger than source after scaling → secondary upscale branch
TEST_F(CollectionPublisherTest, ClipHelper_SmallImage_SecondaryUpscale)
{
    CollectionPublisher publisher;
    QImage narrow = makeImage(50, 500, Qt::yellow);
    QImage result = call_private_fun::CollectionPublisherclipHelper(publisher, narrow, 200, 100);
    EXPECT_EQ(result.width(), 200);
    EXPECT_EQ(result.height(), 100);
    EXPECT_EQ(result.format(), QImage::Format_RGB888);
}

// B5: Output always matches (width, height) and Format_RGB888
TEST_F(CollectionPublisherTest, ClipHelper_OutputSizeAndFormat)
{
    CollectionPublisher publisher;
    QImage img = makeImage(500, 300, Qt::magenta);
    QImage result = call_private_fun::CollectionPublisherclipHelper(publisher, img, 250, 150);
    EXPECT_EQ(result.size(), QSize(250, 150));
    EXPECT_EQ(result.format(), QImage::Format_RGB888);
}

// ===========================================================================
// Section 2: ImagePublisher::addPadAndScaled (private)
// ===========================================================================

// B1: Portrait image → scaledToHeight
TEST_F(CollectionPublisherTest, AddPadAndScaled_Portrait_ScaledToHeight)
{
    ImagePublisher publisher;
    QImage portrait = makeImage(50, 100, Qt::red);
    QImage result = call_private_fun::ImagePublisheraddPadAndScaled(publisher, portrait);
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.height(), 180);
    EXPECT_EQ(result.format(), QImage::Format_RGBA8888);
}

// B2: Landscape image → scaledToWidth
TEST_F(CollectionPublisherTest, AddPadAndScaled_Landscape_ScaledToWidth)
{
    ImagePublisher publisher;
    QImage landscape = makeImage(100, 50, Qt::green);
    QImage result = call_private_fun::ImagePublisheraddPadAndScaled(publisher, landscape);
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.width(), 180);
    EXPECT_EQ(result.format(), QImage::Format_RGBA8888);
}

// B3: Square image → scaledToWidth (width >= height falls to else branch)
TEST_F(CollectionPublisherTest, AddPadAndScaled_Square_ScaledToWidth)
{
    ImagePublisher publisher;
    QImage square = makeImage(80, 80, Qt::blue);
    QImage result = call_private_fun::ImagePublisheraddPadAndScaled(publisher, square);
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.width(), 180);
    EXPECT_EQ(result.height(), 180);
    EXPECT_EQ(result.format(), QImage::Format_RGBA8888);
}

// B4: Null image input → should not crash
TEST_F(CollectionPublisherTest, AddPadAndScaled_NullImage_NoCrash)
{
    ImagePublisher publisher;
    QImage nullImg;
    QImage result = call_private_fun::ImagePublisheraddPadAndScaled(publisher, nullImg);
    EXPECT_TRUE(result.isNull());
}

// ===========================================================================
// Section 2: ImagePublisher::requestImage (protected)
// ===========================================================================

// B1: DeviceReadGuard inactive → return empty image
TEST_F(CollectionPublisherTest, ImagePublisher_RequestImage_DeviceInactive_ReturnsEmpty)
{
    deviceReadable = false;
    TestableImagePublisher publisher;
    QImage result = publisher.requestImage("prefix_/test/path.png", nullptr, QSize());
    EXPECT_TRUE(result.isNull());
}

// B2: Normal load with loadMode=0 (clipToRect)
TEST_F(CollectionPublisherTest, ImagePublisher_RequestImage_LoadMode0_ClipToRect)
{
    TestableImagePublisher publisher;
    QImage result = publisher.requestImage("prefix_/test/path.png", nullptr, QSize());
    EXPECT_FALSE(result.isNull());
    EXPECT_LE(result.width(), 180);
    EXPECT_LE(result.height(), 180);
}

// B3: Load with loadMode=1 (addPadAndScaled)
TEST_F(CollectionPublisherTest, ImagePublisher_RequestImage_LoadMode1_AddPadAndScaled)
{
    TestableImagePublisher publisher;
    auto &mode = access_private_field::ImagePublisherm_loadMode(publisher);
    mode = 1;

    QImage result = publisher.requestImage("prefix_/test/path.png", nullptr, QSize());
    EXPECT_FALSE(result.isNull());
    EXPECT_TRUE(result.width() == 180 || result.height() == 180);
}

// B4: Video path → uses MovieService::getMovieCover
TEST_F(CollectionPublisherTest, ImagePublisher_RequestImage_VideoPath_UsesMovieCover)
{
    isVideoResult = true;
    TestableImagePublisher publisher;
    QImage result = publisher.requestImage("prefix_/test/video.mp4", nullptr, QSize());
    EXPECT_FALSE(result.isNull());
}

// B5: requestedSize valid → scaled result
TEST_F(CollectionPublisherTest, ImagePublisher_RequestImage_ValidRequestedSize_Scaled)
{
    TestableImagePublisher publisher;
    QImage result = publisher.requestImage("prefix_/test/path.png", nullptr, QSize(50, 50));
    EXPECT_FALSE(result.isNull());
    EXPECT_LE(result.width(), 50);
    EXPECT_LE(result.height(), 50);
}

// B6: size pointer receives correct size
TEST_F(CollectionPublisherTest, ImagePublisher_RequestImage_SizePointer_Updated)
{
    TestableImagePublisher publisher;
    QSize outSize;
    QImage result = publisher.requestImage("prefix_/test/path.png", &outSize, QSize());
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(outSize.width(), result.width());
    EXPECT_EQ(outSize.height(), result.height());
}

// B7: requestedSize with 0 dimensions → no scaling
TEST_F(CollectionPublisherTest, ImagePublisher_RequestImage_ZeroRequestedSize_NoScaling)
{
    TestableImagePublisher publisher;
    QImage result = publisher.requestImage("prefix_/test/path.png", nullptr, QSize(0, 0));
    EXPECT_FALSE(result.isNull());
    EXPECT_LE(result.width(), 180);
    EXPECT_LE(result.height(), 180);
}

// ===========================================================================
// Section 2: AsyncImageResponseAlbum::run()
// ===========================================================================

// B1: DeviceReadGuard inactive → emit finished(), m_image stays empty
TEST_F(CollectionPublisherTest, AsyncResponseRun_DeviceInactive_EmitsFinished_EmptyImage)
{
    deviceReadable = false;
    AsyncImageResponseAlbum response("prefix_/test/path.png", QSize());
    QSignalSpy spy(&response, &AsyncImageResponseAlbum::finished);
    response.run();
    EXPECT_EQ(spy.count(), 1);
    auto tex = response.textureFactory();
    QImage img;
    if (tex) {
        img = tex->image();
        delete tex;
    }
    EXPECT_TRUE(img.isNull());
}

// B2: Normal load with loadMode=0 → emit finished once, non-empty image
TEST_F(CollectionPublisherTest, AsyncResponseRun_NormalLoad_LoadMode0_EmitsFinished)
{
    AsyncImageResponseAlbum response("prefix_/test/path.png", QSize());
    response.setLoadMode(0);
    QSignalSpy spy(&response, &AsyncImageResponseAlbum::finished);
    response.run();
    EXPECT_EQ(spy.count(), 1);
    auto tex = response.textureFactory();
    QImage img;
    if (tex) {
        img = tex->image();
        delete tex;
    }
    EXPECT_FALSE(img.isNull());
}

// B3: Normal load with loadMode=1 → emit finished once
TEST_F(CollectionPublisherTest, AsyncResponseRun_NormalLoad_LoadMode1_EmitsFinished)
{
    AsyncImageResponseAlbum response("prefix_/test/path.png", QSize());
    response.setLoadMode(1);
    QSignalSpy spy(&response, &AsyncImageResponseAlbum::finished);
    response.run();
    EXPECT_EQ(spy.count(), 1);
    auto tex = response.textureFactory();
    QImage img;
    if (tex) {
        img = tex->image();
        delete tex;
    }
    EXPECT_FALSE(img.isNull());
}

// B4: Video path → uses MovieService::getMovieCover
TEST_F(CollectionPublisherTest, AsyncResponseRun_VideoPath_EmitsFinished)
{
    isVideoResult = true;
    AsyncImageResponseAlbum response("prefix_/test/video.mp4", QSize());
    response.setLoadMode(0);
    QSignalSpy spy(&response, &AsyncImageResponseAlbum::finished);
    response.run();
    EXPECT_EQ(spy.count(), 1);
    auto tex = response.textureFactory();
    QImage img;
    if (tex) {
        img = tex->image();
        delete tex;
    }
    EXPECT_FALSE(img.isNull());
}

// B5: Valid requestedSize → scaled image
TEST_F(CollectionPublisherTest, AsyncResponseRun_ValidRequestedSize_Scaled)
{
    AsyncImageResponseAlbum response("prefix_/test/path.png", QSize(50, 50));
    response.setLoadMode(0);
    QSignalSpy spy(&response, &AsyncImageResponseAlbum::finished);
    response.run();
    EXPECT_EQ(spy.count(), 1);
    auto tex = response.textureFactory();
    QImage img;
    if (tex) {
        img = tex->image();
        delete tex;
    }
    EXPECT_FALSE(img.isNull());
    if (!img.isNull()) {
        EXPECT_LE(img.width(), 50);
        EXPECT_LE(img.height(), 50);
    }
}

// B6: Load fails but still emits finished
TEST_F(CollectionPublisherTest, AsyncResponseRun_LoadFails_StillEmitsFinished)
{
    loadSuccess = false;
    AsyncImageResponseAlbum response("prefix_/test/path.png", QSize());
    response.setLoadMode(0);
    QSignalSpy spy(&response, &AsyncImageResponseAlbum::finished);
    response.run();
    EXPECT_EQ(spy.count(), 1);
}

// ===========================================================================
// Section 3: LoadImage dispatch logic
// ===========================================================================

// --- getFitWindowScale ---

// B1: m_FrameIndex == Invalid → delegates to ViewLoad
TEST_F(CollectionPublisherTest, LoadImage_GetFitWindowScale_DelegatesToViewLoad)
{
    LoadImage loader;
    stub.set_lamda(ADDR(ViewLoad, getFitWindowScale),
        [](ViewLoad *, const QString &, double, double, bool) -> double { return 3.14; });

    double scale = loader.getFitWindowScale("/test.png", 100, 100);
    EXPECT_DOUBLE_EQ(scale, 3.14);
}

// B2: m_FrameIndex != Invalid → delegates to MultiImageLoad
TEST_F(CollectionPublisherTest, LoadImage_GetFitWindowScale_DelegatesToMultiImageLoad)
{
    LoadImage loader;
    loader.setMultiFrameIndex(0);

    stub.set_lamda(ADDR(MultiImageLoad, getFitWindowScale),
        [](MultiImageLoad *, const QString &, double, double, int) -> double { return 2.71; });

    double scale = loader.getFitWindowScale("/test.png", 100, 100);
    EXPECT_DOUBLE_EQ(scale, 2.71);
}

// --- getImageWidth / getImageHeight ---

// B1: Default (Invalid frame) → delegates to ViewLoad::getImageWidth
TEST_F(CollectionPublisherTest, LoadImage_GetImageWidth_DelegatesToViewLoad)
{
    LoadImage loader;
    stub.set_lamda(ADDR(ViewLoad, getImageWidth),
        [](ViewLoad *, const QString &) -> int { return 42; });
    stub.set_lamda(ADDR(ViewLoad, getImageHeight),
        [](ViewLoad *, const QString &) -> int { return 84; });

    EXPECT_EQ(loader.getImageWidth("/test.png"), 42);
    EXPECT_EQ(loader.getImageHeight("/test.png"), 84);
}

// B2: m_bReverseHeightWidth=true → width returns viewLoad's height and vice versa
TEST_F(CollectionPublisherTest, LoadImage_GetImageWidth_ReverseSwaps)
{
    LoadImage loader;
    loader.setReverseHeightWidth(true);

    stub.set_lamda(ADDR(ViewLoad, getImageWidth),
        [](ViewLoad *, const QString &) -> int { return 100; });
    stub.set_lamda(ADDR(ViewLoad, getImageHeight),
        [](ViewLoad *, const QString &) -> int { return 200; });

    EXPECT_EQ(loader.getImageWidth("/test.png"), 200);
    EXPECT_EQ(loader.getImageHeight("/test.png"), 100);
}

// B3: m_FrameIndex valid → delegates to MultiImageLoad
TEST_F(CollectionPublisherTest, LoadImage_GetImageWidth_MultiFrame_DelegatesToMultiImageLoad)
{
    LoadImage loader;
    loader.setMultiFrameIndex(0);

    stub.set_lamda(ADDR(MultiImageLoad, getImageWidth),
        [](MultiImageLoad *, const QString &, int) -> int { return 333; });
    stub.set_lamda(ADDR(MultiImageLoad, getImageHeight),
        [](MultiImageLoad *, const QString &, int) -> int { return 444; });

    EXPECT_EQ(loader.getImageWidth("/test.png"), 333);
    EXPECT_EQ(loader.getImageHeight("/test.png"), 444);
}

// --- getrealWidthHeightRatio ---

// B1: Normal mode → width/height
TEST_F(CollectionPublisherTest, LoadImage_GetRatio_NormalMode)
{
    LoadImage loader;
    stub.set_lamda(ADDR(ViewLoad, getImageWidth),
        [](ViewLoad *, const QString &) -> int { return 200; });
    stub.set_lamda(ADDR(ViewLoad, getImageHeight),
        [](ViewLoad *, const QString &) -> int { return 100; });

    double ratio = loader.getrealWidthHeightRatio("/test.png");
    EXPECT_DOUBLE_EQ(ratio, 2.0);
}

// B2: Reverse mode → height/width
TEST_F(CollectionPublisherTest, LoadImage_GetRatio_ReverseMode)
{
    LoadImage loader;
    loader.setReverseHeightWidth(true);

    stub.set_lamda(ADDR(ViewLoad, getImageWidth),
        [](ViewLoad *, const QString &) -> int { return 200; });
    stub.set_lamda(ADDR(ViewLoad, getImageHeight),
        [](ViewLoad *, const QString &) -> int { return 100; });

    double ratio = loader.getrealWidthHeightRatio("/test.png");
    EXPECT_DOUBLE_EQ(ratio, 0.5);
}

// B3: Multi-image frame → uses MultiImageLoad
TEST_F(CollectionPublisherTest, LoadImage_GetRatio_MultiFrame)
{
    LoadImage loader;
    loader.setMultiFrameIndex(0);

    stub.set_lamda(ADDR(MultiImageLoad, getImageWidth),
        [](MultiImageLoad *, const QString &, int) -> int { return 300; });
    stub.set_lamda(ADDR(MultiImageLoad, getImageHeight),
        [](MultiImageLoad *, const QString &, int) -> int { return 150; });

    double ratio = loader.getrealWidthHeightRatio("/test.png");
    EXPECT_DOUBLE_EQ(ratio, 2.0);
}

// --- imageIsNull ---

TEST_F(CollectionPublisherTest, LoadImage_ImageIsNull_DelegatesToThumbnailLoad)
{
    LoadImage loader;
    stub.set_lamda(ADDR(ThumbnailLoad, imageIsNull),
        [](ThumbnailLoad *, const QString &) -> bool { return true; });

    EXPECT_TRUE(loader.imageIsNull("/test.png"));
}

// --- setMultiFrameIndex / setReverseHeightWidth ---

// B1: setMultiFrameIndex sets value
TEST_F(CollectionPublisherTest, LoadImage_SetMultiFrameIndex_ReadsBack)
{
    LoadImage loader;
    loader.setMultiFrameIndex(5);
    auto &idx = access_private_field::LoadImagem_FrameIndex(loader);
    EXPECT_EQ(idx, 5);
}

// B2: setMultiFrameIndex default is Invalid (-1)
TEST_F(CollectionPublisherTest, LoadImage_SetMultiFrameIndex_DefaultIsInvalid)
{
    LoadImage loader;
    loader.setMultiFrameIndex();
    auto &idx = access_private_field::LoadImagem_FrameIndex(loader);
    EXPECT_EQ(idx, -1);
}

// B3: setReverseHeightWidth sets value
TEST_F(CollectionPublisherTest, LoadImage_SetReverseHeightWidth_True)
{
    LoadImage loader;
    loader.setReverseHeightWidth(true);
    auto &rev = access_private_field::LoadImagem_bReverseHeightWidth(loader);
    EXPECT_TRUE(rev);
}

// B4: setReverseHeightWidth false
TEST_F(CollectionPublisherTest, LoadImage_SetReverseHeightWidth_False)
{
    LoadImage loader;
    loader.setReverseHeightWidth(true);
    loader.setReverseHeightWidth(false);
    auto &rev = access_private_field::LoadImagem_bReverseHeightWidth(loader);
    EXPECT_FALSE(rev);
}

// --- loadThumbnail ---

// B1: Load success → emit callQmlRefeshImg signal
TEST_F(CollectionPublisherTest, LoadImage_LoadThumbnail_Success_EmitsSignal)
{
    LoadImage loader;
    QSignalSpy spy(&loader, &LoadImage::callQmlRefeshImg);

    loader.loadThumbnail("/test/thumb.png");

    EXPECT_EQ(spy.count(), 1);
}

// B2: Load fails → no signal
TEST_F(CollectionPublisherTest, LoadImage_LoadThumbnail_Fail_NoSignal)
{
    loadSuccess = false;
    LoadImage loader;
    QSignalSpy spy(&loader, &LoadImage::callQmlRefeshImg);

    loader.loadThumbnail("/test/thumb.png");

    EXPECT_EQ(spy.count(), 0);
}

// B3: file:// prefix path → localPath conversion, still loads
TEST_F(CollectionPublisherTest, LoadImage_LoadThumbnail_FilePrefix_StillLoads)
{
    LoadImage loader;
    QSignalSpy spy(&loader, &LoadImage::callQmlRefeshImg);

    loader.loadThumbnail("file:///test/thumb.png");

    EXPECT_EQ(spy.count(), 1);
}

// --- loadThumbnails ---

// B1: Empty list → no signals
TEST_F(CollectionPublisherTest, LoadImage_LoadThumbnails_EmptyList_NoSignals)
{
    LoadImage loader;
    QSignalSpy spy(&loader, &LoadImage::callQmlRefeshImg);

    loader.loadThumbnails(QStringList{});

    EXPECT_EQ(spy.count(), 0);
}

// B2: Multiple items → one signal per item
TEST_F(CollectionPublisherTest, LoadImage_LoadThumbnails_MultipleItems_MultipleSignals)
{
    LoadImage loader;
    QSignalSpy spy(&loader, &LoadImage::callQmlRefeshImg);

    loader.loadThumbnails({"/test/1.png", "/test/2.png", "/test/3.png"});

    EXPECT_EQ(spy.count(), 3);
}

// --- onImageFileChanged ---

// B1: isMultiImage=true → calls m_multiLoad->removeImageCache
TEST_F(CollectionPublisherTest, LoadImage_OnImageFileChanged_MultiImage_RemovesMultiCache)
{
    LoadImage loader;

    int multiRemoveCount = 0;
    stub.set_lamda(ADDR(MultiImageLoad, removeImageCache),
        [&multiRemoveCount](MultiImageLoad *, const QString &) -> void { multiRemoveCount++; });

    loader.onImageFileChanged("/test/path.png", true, false);

    EXPECT_EQ(multiRemoveCount, 1);
}

// B2: isExist=true → calls m_pThumbnail->removeImageCache + requestImage + m_viewLoad->reloadImageCache
TEST_F(CollectionPublisherTest, LoadImage_OnImageFileChanged_IsExist_ReloadsCache)
{
    LoadImage loader;

    int thumbRemoveCount = 0;
    int viewReloadCount = 0;
    stub.set_lamda(ADDR(ThumbnailLoad, removeImageCache),
        [&thumbRemoveCount](ThumbnailLoad *, const QString &) -> void { thumbRemoveCount++; });
    stub.set_lamda(ADDR(ViewLoad, reloadImageCache),
        [&viewReloadCount](ViewLoad *, const QString &) -> void { viewReloadCount++; });

    loader.onImageFileChanged("/test/path.png", false, true);

    EXPECT_EQ(thumbRemoveCount, 1);
    EXPECT_EQ(viewReloadCount, 1);
    // requestImage is called internally; verify via loadCallCount
    EXPECT_GE(loadCallCount, 1);
}

// B3: isExist=false → calls m_viewLoad->removeImageCache (not reloadImageCache)
TEST_F(CollectionPublisherTest, LoadImage_OnImageFileChanged_NotExist_RemovesViewCache)
{
    LoadImage loader;

    int viewRemoveCount = 0;
    int viewReloadCount = 0;
    stub.set_lamda(ADDR(ViewLoad, removeImageCache),
        [&viewRemoveCount](ViewLoad *, const QString &) -> void { viewRemoveCount++; });
    stub.set_lamda(ADDR(ViewLoad, reloadImageCache),
        [&viewReloadCount](ViewLoad *, const QString &) -> void { viewReloadCount++; });

    loader.onImageFileChanged("/test/path.png", false, false);

    EXPECT_EQ(viewRemoveCount, 1);
    EXPECT_EQ(viewReloadCount, 0);
}

// B4: isMultiImage=true AND isExist=true → both paths taken
TEST_F(CollectionPublisherTest, LoadImage_OnImageFileChanged_MultiAndExist_BothPaths)
{
    LoadImage loader;

    int multiRemoveCount = 0;
    int thumbRemoveCount = 0;
    int viewReloadCount = 0;
    stub.set_lamda(ADDR(MultiImageLoad, removeImageCache),
        [&multiRemoveCount](MultiImageLoad *, const QString &) -> void { multiRemoveCount++; });
    stub.set_lamda(ADDR(ThumbnailLoad, removeImageCache),
        [&thumbRemoveCount](ThumbnailLoad *, const QString &) -> void { thumbRemoveCount++; });
    stub.set_lamda(ADDR(ViewLoad, reloadImageCache),
        [&viewReloadCount](ViewLoad *, const QString &) -> void { viewReloadCount++; });

    loader.onImageFileChanged("/test/path.png", true, true);

    EXPECT_EQ(multiRemoveCount, 1);
    EXPECT_EQ(thumbRemoveCount, 1);
    EXPECT_EQ(viewReloadCount, 1);
    // requestImage is called internally; verify via loadCallCount
    EXPECT_GE(loadCallCount, 1);
}
