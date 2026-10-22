// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QImage>
#include <QTemporaryDir>
#include <QFileInfo>
#include <QFile>
#include <memory>

#include "imageengine/imagedataservice.h"
#include "configsetter.h"
#include "unionimage/baseutils.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// Typedefs to avoid commas inside ACCESS_PRIVATE_* macro arguments
using ImageMapType = std::deque<std::pair<QString, QImage>>;
using MovieDurationMapType = QMap<QString, QString>;
using GetImageReturnType = std::pair<QImage, bool>;

// Access private methods of ImageDataService
ACCESS_PRIVATE_FUN(ImageDataService, bool(const QString &), pathInMap)
ACCESS_PRIVATE_FUN(ImageDataService, GetImageReturnType(const QString &), getImageFromMap)
ACCESS_PRIVATE_FUN(ImageDataService, void(const QString &), removePathFromMap)
ACCESS_PRIVATE_FUN(ImageDataService, void(const QString &), removeThumbnailFile)

// Access private fields for test cleanup
ACCESS_PRIVATE_FIELD(ImageDataService, ImageMapType, m_AllImageMap)
ACCESS_PRIVATE_FIELD(ImageDataService, MovieDurationMapType, m_movieDurationStrMap)
ACCESS_PRIVATE_FIELD(ImageDataService, std::atomic_int, m_loadMode)

class ImageDataServiceTest : public ::testing::Test
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

    StubExt stub;
    ImageDataService *svc = nullptr;

    void SetUp() override
    {
        stub.clear();
        // Stub LibConfigSetter::value to return defaultValue
        stub.set_lamda(ADDR(LibConfigSetter, value),
            [](LibConfigSetter *, const QString &, const QString &,
               const QVariant &defaultValue) -> QVariant {
                return defaultValue;
            });
        // Stub LibConfigSetter::setValue to no-op
        stub.set_lamda(ADDR(LibConfigSetter, setValue),
            [](LibConfigSetter *, const QString &, const QString &,
               const QVariant &) {});
        // Stub ReadThumbnailManager::addLoadPath to prevent async thumbnail loading
        stub.set_lamda(ADDR(ReadThumbnailManager, addLoadPath),
            [](ReadThumbnailManager *, const QString &) {});

        svc = ImageDataService::instance();

        // Clean up singleton state between tests
        access_private_field::ImageDataServicem_AllImageMap(*svc).clear();
        access_private_field::ImageDataServicem_movieDurationStrMap(*svc).clear();
        access_private_field::ImageDataServicem_loadMode(*svc) = 0;
    }

    void TearDown() override
    {
        stub.clear();
    }
};

// --- instance() ---

TEST_F(ImageDataServiceTest, Instance_ReturnsSameNonNullPointer)
{
    ImageDataService *p1 = ImageDataService::instance();
    ImageDataService *p2 = ImageDataService::instance();
    EXPECT_NE(p1, nullptr);
    EXPECT_EQ(p1, p2);
}

// --- addImage() + imageIsLoaded() ---

TEST_F(ImageDataServiceTest, AddImageThenImageIsLoaded_ReturnsTrue)
{
    QImage img(10, 10, QImage::Format_RGB32);
    img.fill(Qt::red);
    QString path = "/tmp/test_addimage.jpg";
    svc->addImage(path, img);
    EXPECT_TRUE(svc->imageIsLoaded(path, false));
}

TEST_F(ImageDataServiceTest, ImageIsLoaded_UnaddedPath_ReturnsFalse)
{
    QString path = "/tmp/test_notloaded.jpg";
    EXPECT_FALSE(svc->imageIsLoaded(path, false));
}

TEST_F(ImageDataServiceTest, ImageIsLoaded_TrashFile_PathInMap_ReturnsTrue)
{
    QImage img(10, 10, QImage::Format_RGB32);
    img.fill(Qt::blue);
    QString path = "/tmp/test_trash_loaded.jpg";
    svc->addImage(path, img);
    EXPECT_TRUE(svc->imageIsLoaded(path, true));
}

TEST_F(ImageDataServiceTest, ImageIsLoaded_TrashFile_NotInMap_ReturnsFalse)
{
    QString path = "/tmp/test_trash_notloaded.jpg";
    EXPECT_FALSE(svc->imageIsLoaded(path, true));
}

// --- getThumnailImageByPathRealTime() ---

TEST_F(ImageDataServiceTest, GetThumbnailRealTime_LoadedPath_ReturnsCachedImage)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test.jpg";
    QImage img(10, 10, QImage::Format_RGB32);
    img.fill(Qt::green);
    ASSERT_TRUE(img.save(filePath, "JPG"));

    svc->addImage(filePath, img);
    QImage result = svc->getThumnailImageByPathRealTime(filePath, false, false);
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.width(), 10);
    EXPECT_EQ(result.height(), 10);
}

TEST_F(ImageDataServiceTest, GetThumbnailRealTime_UnloadedPath_ReturnsNullImage)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test.jpg";
    QImage img(10, 10, QImage::Format_RGB32);
    img.fill(Qt::green);
    ASSERT_TRUE(img.save(filePath, "JPG"));

    QImage result = svc->getThumnailImageByPathRealTime(filePath, false, false);
    EXPECT_TRUE(result.isNull());
}

TEST_F(ImageDataServiceTest, GetThumbnailRealTime_NonExistentFile_ReturnsNull)
{
    QString filePath = "/tmp/this_file_does_not_exist_12345.jpg";
    QImage result = svc->getThumnailImageByPathRealTime(filePath, false, false);
    EXPECT_TRUE(result.isNull());
}

TEST_F(ImageDataServiceTest, GetThumbnailRealTime_ReloadFlag_ClearsCacheAndReturnsNull)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.path() + "/test.jpg";
    QImage img(10, 10, QImage::Format_RGB32);
    img.fill(Qt::green);
    ASSERT_TRUE(img.save(filePath, "JPG"));

    svc->addImage(filePath, img);
    EXPECT_TRUE(call_private_fun::ImageDataServicepathInMap(*svc, filePath));

    // bReload=true removes cached image and triggers async reload, so returns null
    QImage result = svc->getThumnailImageByPathRealTime(filePath, false, true);
    EXPECT_TRUE(result.isNull());
    // Cache entry was removed by reload
    EXPECT_FALSE(call_private_fun::ImageDataServicepathInMap(*svc, filePath));
}

// --- addMovieDurationStr() / getMovieDurationStrByPath() ---

TEST_F(ImageDataServiceTest, AddMovieDurationStrThenGet_ReturnsSameValue)
{
    QString path = "/tmp/test_movie.mp4";
    QString duration = "00:01:30";
    svc->addMovieDurationStr(path, duration);
    EXPECT_EQ(svc->getMovieDurationStrByPath(path), duration);
}

TEST_F(ImageDataServiceTest, GetMovieDurationStr_UnknownPath_ReturnsEmpty)
{
    QString path = "/tmp/test_unknown_movie.mp4";
    EXPECT_TRUE(svc->getMovieDurationStrByPath(path).isEmpty());
}

TEST_F(ImageDataServiceTest, AddMovieDurationStr_DuplicatePath_ReturnsLatestValue)
{
    QString path = "/tmp/test_dup_movie.mp4";
    svc->addMovieDurationStr(path, "00:01:00");
    svc->addMovieDurationStr(path, "00:02:00");
    EXPECT_EQ(svc->getMovieDurationStrByPath(path), "00:02:00");
}

// --- switchLoadMode() / getLoadMode() ---

TEST_F(ImageDataServiceTest, SwitchLoadMode_ChangesModeValue)
{
    int originalMode = svc->getLoadMode();
    svc->switchLoadMode();
    EXPECT_NE(svc->getLoadMode(), originalMode);
}

TEST_F(ImageDataServiceTest, SwitchLoadMode_DoubleToggle_RestoresValue)
{
    int originalMode = svc->getLoadMode();
    svc->switchLoadMode();
    svc->switchLoadMode();
    EXPECT_EQ(svc->getLoadMode(), originalMode);
}

// --- getLoadModePath() ---

TEST_F(ImageDataServiceTest, GetLoadModePath_Mode0_ReturnsOriginalPath)
{
    access_private_field::ImageDataServicem_loadMode(*svc) = 0;
    QString path = "/tmp/test_mode0.jpg";
    EXPECT_EQ(svc->getLoadModePath(path), path);
}

TEST_F(ImageDataServiceTest, GetLoadModePath_Mode1_ReturnsScaledPath)
{
    access_private_field::ImageDataServicem_loadMode(*svc) = 1;
    QString path = "/tmp/test_mode1.jpg";
    QString result = svc->getLoadModePath(path);
    EXPECT_NE(result, path);
    EXPECT_TRUE(result.contains("_scale"));
}

// --- getScaledPath() ---

TEST_F(ImageDataServiceTest, GetScaledPath_ContainsScaleSuffix)
{
    QString path = "/tmp/test_dir/image.jpg";
    QString scaledPath = svc->getScaledPath(path);
    QFileInfo fi(path);
    QFileInfo fiScaled(scaledPath);
    EXPECT_EQ(fiScaled.absolutePath(), fi.absolutePath());
    EXPECT_TRUE(scaledPath.contains("_scale"));
    EXPECT_TRUE(scaledPath.endsWith(".jpg"));
}

// --- stopFlushThumbnail() / waitFlushThumbnailFinish() / readerIsRunning() ---

TEST_F(ImageDataServiceTest, ReaderIsRunning_NoTask_ReturnsFalse)
{
    EXPECT_FALSE(svc->readerIsRunning());
}

TEST_F(ImageDataServiceTest, StopFlushThumbnail_NoTask_NoCrash)
{
    svc->stopFlushThumbnail();
    SUCCEED();
}

TEST_F(ImageDataServiceTest, WaitFlushThumbnailFinish_NoTask_NoCrash)
{
    svc->waitFlushThumbnailFinish();
    EXPECT_FALSE(svc->readerIsRunning());
}

// --- Private: pathInMap() ---

TEST_F(ImageDataServiceTest, PathInMap_PathExists_ReturnsTrue)
{
    QImage img(5, 5, QImage::Format_RGB32);
    QString path = "/tmp/test_pathinmap_found.jpg";
    svc->addImage(path, img);
    EXPECT_TRUE(call_private_fun::ImageDataServicepathInMap(*svc, path));
}

TEST_F(ImageDataServiceTest, PathInMap_PathNotExists_ReturnsFalse)
{
    QString path = "/tmp/test_pathinmap_notfound.jpg";
    EXPECT_FALSE(call_private_fun::ImageDataServicepathInMap(*svc, path));
}

// --- Private: getImageFromMap() ---

TEST_F(ImageDataServiceTest, GetImageFromMap_Found_ReturnsImageAndTrue)
{
    QImage img(8, 8, QImage::Format_RGB32);
    img.fill(Qt::cyan);
    QString path = "/tmp/test_getimage_found.jpg";
    svc->addImage(path, img);
    auto result = call_private_fun::ImageDataServicegetImageFromMap(*svc, path);
    EXPECT_TRUE(result.second);
    EXPECT_FALSE(result.first.isNull());
    EXPECT_EQ(result.first.width(), 8);
}

TEST_F(ImageDataServiceTest, GetImageFromMap_NotFound_ReturnsNullAndFalse)
{
    QString path = "/tmp/test_getimage_notfound.jpg";
    auto result = call_private_fun::ImageDataServicegetImageFromMap(*svc, path);
    EXPECT_FALSE(result.second);
    EXPECT_TRUE(result.first.isNull());
}

// --- Private: removePathFromMap() ---

TEST_F(ImageDataServiceTest, RemovePathFromMap_RemovesEntry)
{
    QImage img(5, 5, QImage::Format_RGB32);
    QString path = "/tmp/test_remove_path.jpg";
    svc->addImage(path, img);
    EXPECT_TRUE(call_private_fun::ImageDataServicepathInMap(*svc, path));

    call_private_fun::ImageDataServiceremovePathFromMap(*svc, path);
    EXPECT_FALSE(call_private_fun::ImageDataServicepathInMap(*svc, path));
}

TEST_F(ImageDataServiceTest, RemovePathFromMap_AlsoRemovesScaledPath)
{
    QImage img(5, 5, QImage::Format_RGB32);
    QString path = "/tmp/test_remove_scaled.jpg";
    // Add with scaled path key (mode 1)
    access_private_field::ImageDataServicem_loadMode(*svc) = 1;
    svc->addImage(path, img);
    EXPECT_TRUE(call_private_fun::ImageDataServicepathInMap(*svc, path));

    // removePathFromMap removes by exact path AND by getScaledPath(path)
    call_private_fun::ImageDataServiceremovePathFromMap(*svc, path);
    EXPECT_FALSE(call_private_fun::ImageDataServicepathInMap(*svc, path));
}

// --- Private: removeThumbnailFile() ---

TEST_F(ImageDataServiceTest, RemoveThumbnailFile_DeletesFile)
{
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString thumbFile = tmpDir.path() + "/thumb.png";
    QImage img(5, 5, QImage::Format_RGB32);
    img.fill(Qt::red);
    ASSERT_TRUE(img.save(thumbFile, "PNG"));
    ASSERT_TRUE(QFile::exists(thumbFile));

    // Stub filePathToThumbnailPath to return our temp file
    stub.set_lamda(ADDR(Libutils::base, filePathToThumbnailPath),
        [&thumbFile](const QString &, QString) -> QString { return thumbFile; });

    // Mode 0: getLoadModePath returns same path
    access_private_field::ImageDataServicem_loadMode(*svc) = 0;

    call_private_fun::ImageDataServiceremoveThumbnailFile(*svc, "/dummy/path.jpg");
    EXPECT_FALSE(QFile::exists(thumbFile));
}

TEST_F(ImageDataServiceTest, RemoveThumbnailFile_EmptyPath_NoCrash)
{
    call_private_fun::ImageDataServiceremoveThumbnailFile(*svc, "");
    SUCCEED();
}
