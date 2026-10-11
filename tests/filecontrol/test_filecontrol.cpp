// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | FileControl::parseCommandlineGetPaths | high | 4 | 3 | 4 |
 * | FileControl::setWallpaper | high | 3 | 3 | 3 |
 *
 * 分支列表 (parseCommandlineGetPaths):
 *   B1: arguments.size() <= 1 (no path args) → return empty QStringList
 *   B2: argument is existing file, isImage||isVideo → add to validPaths, return validPaths
 *   B3: argument is existing file, !isImage && !isVideo → add to paths only; if
 *       validPaths empty && paths not empty → emit invalidFormat()
 *   B4: argument is non-existent file (QFileInfo::isFile()==false) → skip, return empty
 *
 * 分支列表 (setWallpaper):
 *   B1: imgPath.isNull() → thread lambda does nothing
 *   B2: imgPath non-null → thread creates QDBusInterface (invalid in test env),
 *       logs warning, thread finishes
 *   B3: method returns immediately after starting thread (non-blocking)
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QThread>
#include <QSignalSpy>
#include <QTest>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QFile>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QUrl>
#include <QStringList>
#include <memory>

#include "filecontrol.h"
#include "unionimage/unionimage.h"
#include "stubext.h"

using namespace stub_ext;

class FileControlTest : public ::testing::Test
{
protected:
    StubExt stub;

    void SetUp() override
    {
        // Stub LibUnionImage_NameSpace::localPath to pass through URL as local file path
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, localPath),
            [](const QUrl &url) -> QString { return url.toLocalFile(); });
    }

    void TearDown() override
    {
        stub.clear();
    }
};

// ============ parseCommandlineGetPaths ============

// B1: No path arguments (only app name) → return empty QStringList
TEST_F(FileControlTest, ParseCommandlineGetPaths_NoArguments_ReturnsEmpty)
{
    // Arrange
    FileControl fc;
    stub.set_lamda(ADDR(QCoreApplication, arguments),
        []() -> QStringList { return {"test_app"}; });

    // Act
    QStringList result = fc.parseCommandlineGetPaths();

    // Assert
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(result.size(), 0);
}

// B2: Valid image file argument → validPaths contains the file
TEST_F(FileControlTest, ParseCommandlineGetPaths_ValidImageFile_ReturnsInValidPaths)
{
    // Arrange
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("test.jpg");
    QFile testFile(filePath);
    ASSERT_TRUE(testFile.open(QIODevice::WriteOnly));
    testFile.write("fake image content");
    testFile.close();

    stub.set_lamda(ADDR(QCoreApplication, arguments),
        [filePath]() -> QStringList { return {"test_app", filePath}; });
    stub.set_lamda(ADDR(FileControl, isImage),
        [](FileControl *, const QString &) -> bool { return true; });
    stub.set_lamda(ADDR(FileControl, isVideo),
        [](FileControl *, const QString &) -> bool { return false; });

    // Act
    QStringList result = fc.parseCommandlineGetPaths();

    // Assert
    EXPECT_EQ(result.size(), 1);
    EXPECT_FALSE(result[0].isEmpty());
}

// B3: Existing file but not image/video → emit invalidFormat signal
TEST_F(FileControlTest, ParseCommandlineGetPaths_NonImageVideoFile_EmitsInvalidFormat)
{
    // Arrange
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("test.txt");
    QFile testFile(filePath);
    ASSERT_TRUE(testFile.open(QIODevice::WriteOnly));
    testFile.write("non-image content");
    testFile.close();

    stub.set_lamda(ADDR(QCoreApplication, arguments),
        [filePath]() -> QStringList { return {"test_app", filePath}; });
    stub.set_lamda(ADDR(FileControl, isImage),
        [](FileControl *, const QString &) -> bool { return false; });
    stub.set_lamda(ADDR(FileControl, isVideo),
        [](FileControl *, const QString &) -> bool { return false; });

    QSignalSpy spy(&fc, &FileControl::invalidFormat);

    // Act
    QStringList result = fc.parseCommandlineGetPaths();

    // Assert
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(spy.count(), 1);
}

// B4: Non-existent file argument → return empty (no signal)
TEST_F(FileControlTest, ParseCommandlineGetPaths_NonExistentFile_ReturnsEmpty)
{
    // Arrange
    FileControl fc;
    QTemporaryDir tmpDir;
    QString fakePath = tmpDir.filePath("nonexistent_99999.txt");

    stub.set_lamda(ADDR(QCoreApplication, arguments),
        [fakePath]() -> QStringList { return {"test_app", fakePath}; });
    stub.set_lamda(ADDR(FileControl, isImage),
        [](FileControl *, const QString &) -> bool { return true; });

    QSignalSpy spy(&fc, &FileControl::invalidFormat);

    // Act
    QStringList result = fc.parseCommandlineGetPaths();

    // Assert
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(spy.count(), 0);  // No signal because QFileInfo::isFile() is false
}

// ============ setWallpaper ============

// B1: Null imgPath → method returns without crash, no thread work
TEST_F(FileControlTest, SetWallpaper_NullImgPath_NoCrash)
{
    // Arrange
    FileControl fc;

    // Act
    fc.setWallpaper(QString());

    // Assert — no crash, method returns; verify object is still valid
    QTest::qWait(100);
    EXPECT_EQ(fc.metaObject()->className(), std::string("FileControl"));
    EXPECT_NE(fc.metaObject(), nullptr);
}

// B2: Non-null imgPath → thread runs, DBus invalid, method returns without crash
TEST_F(FileControlTest, SetWallpaper_NonNullImgPath_NoCrash)
{
    // Arrange
    FileControl fc;
    QTemporaryDir tmpDir;
    QString wallpaperPath = tmpDir.filePath("wallpaper.jpg");

    // Act
    fc.setWallpaper(wallpaperPath);

    // Assert — wait for async thread to finish, verify object survives
    QTest::qWait(500);
    EXPECT_EQ(fc.metaObject()->className(), std::string("FileControl"));
    EXPECT_NE(fc.metaObject(), nullptr);
}

// B3: Non-null imgPath → method is non-blocking (returns before thread finishes)
TEST_F(FileControlTest, SetWallpaper_IsNonBlocking_ReturnsImmediately)
{
    // Arrange
    FileControl fc;
    QTemporaryDir tmpDir;
    QString wallpaperPath = tmpDir.filePath("wallpaper2.jpg");
    QElapsedTimer timer;

    // Act — measure the wall time of the call itself
    timer.start();
    fc.setWallpaper(wallpaperPath);
    qint64 elapsedMs = timer.elapsed();

    // Assert — the DBus work runs in a detached thread; if setWallpaper blocked
    // on it, elapsed would include the full DBus round-trip
    EXPECT_LT(elapsedMs, 200);

    // Cleanup — let the async thread finish before fc goes out of scope
    QTest::qWait(500);
}

// ============ isImage ============

// Test: isImage delegates to LibUnionImage_NameSpace::isImage, returns true
TEST_F(FileControlTest, IsImage_StubbedTrue_ReturnsTrue)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isImage),
        [](const QString &) -> bool { return true; });

    EXPECT_TRUE(fc.isImage("file:///tmp/test.jpg"));
}

// Test: isImage returns false when underlying function returns false
TEST_F(FileControlTest, IsImage_StubbedFalse_ReturnsFalse)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isImage),
        [](const QString &) -> bool { return false; });

    EXPECT_FALSE(fc.isImage("file:///tmp/test.txt"));
}

// ============ isVideo ============

// Test: isVideo delegates to LibUnionImage_NameSpace::isVideo, returns true
TEST_F(FileControlTest, IsVideo_StubbedTrue_ReturnsTrue)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isVideo),
        [](QString) -> bool { return true; });

    EXPECT_TRUE(fc.isVideo("file:///tmp/test.mp4"));
}

// Test: isVideo returns false when underlying function returns false
TEST_F(FileControlTest, IsVideo_StubbedFalse_ReturnsFalse)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isVideo),
        [](QString) -> bool { return false; });

    EXPECT_FALSE(fc.isVideo("file:///tmp/test.txt"));
}

// ============ isFile ============

// Test: isFile returns true for an existing real file
TEST_F(FileControlTest, IsFile_RealFile_ReturnsTrue)
{
    FileControl fc;
    QTemporaryFile tmpFile;
    ASSERT_TRUE(tmpFile.open());
    tmpFile.write("data");
    tmpFile.close();

    EXPECT_TRUE(fc.isFile(QUrl::fromLocalFile(tmpFile.fileName()).toString()));
}

// Test: isFile returns false for a non-existent path
TEST_F(FileControlTest, IsFile_NonExistent_ReturnsFalse)
{
    FileControl fc;
    EXPECT_FALSE(fc.isFile(QUrl::fromLocalFile("/nonexistent_99999.txt").toString()));
}

// ============ pathExists ============

// Test: pathExists returns true for existing file
TEST_F(FileControlTest, PathExists_ExistingFile_ReturnsTrue)
{
    FileControl fc;
    QTemporaryFile tmpFile;
    ASSERT_TRUE(tmpFile.open());
    tmpFile.close();

    EXPECT_TRUE(fc.pathExists(QUrl::fromLocalFile(tmpFile.fileName()).toString()));
}

// Test: pathExists returns false for non-existent path
TEST_F(FileControlTest, PathExists_NonExistent_ReturnsFalse)
{
    FileControl fc;
    EXPECT_FALSE(fc.pathExists(QUrl::fromLocalFile("/nonexistent_99999.txt").toString()));
}

// ============ haveImage ============

// Test: haveImage returns true when list contains a valid image (isImage stubbed true)
TEST_F(FileControlTest, HaveImage_WithImageUrl_ReturnsTrue)
{
    FileControl fc;
    stub.set_lamda(ADDR(FileControl, isImage),
        [](FileControl *, const QString &) -> bool { return true; });

    QVariantList urls;
    urls << QVariant(QUrl::fromLocalFile("/tmp/test.jpg").toString());

    EXPECT_TRUE(fc.haveImage(urls));
}

// Test: haveImage returns false for empty list
TEST_F(FileControlTest, HaveImage_EmptyList_ReturnsFalse)
{
    FileControl fc;
    QVariantList urls;
    EXPECT_FALSE(fc.haveImage(urls));
}

// Test: haveImage returns false when all entries are null
TEST_F(FileControlTest, HaveImage_NullEntries_ReturnsFalse)
{
    FileControl fc;
    stub.set_lamda(ADDR(FileControl, isImage),
        [](FileControl *, const QString &) -> bool { return true; });

    QVariantList urls;
    urls << QVariant();

    EXPECT_FALSE(fc.haveImage(urls));
}

// ============ haveVideo ============

// Test: haveVideo returns true when list contains a valid video (isVideo stubbed true)
TEST_F(FileControlTest, HaveVideo_WithVideoUrl_ReturnsTrue)
{
    FileControl fc;
    stub.set_lamda(ADDR(FileControl, isVideo),
        [](FileControl *, const QString &) -> bool { return true; });

    QVariantList urls;
    urls << QVariant(QString("/tmp/test.mp4"));

    EXPECT_TRUE(fc.haveVideo(urls));
}

// Test: haveVideo returns false for empty list
TEST_F(FileControlTest, HaveVideo_EmptyList_ReturnsFalse)
{
    FileControl fc;
    QVariantList urls;
    EXPECT_FALSE(fc.haveVideo(urls));
}

// Test: haveVideo returns false when all entries are null
TEST_F(FileControlTest, HaveVideo_NullEntries_ReturnsFalse)
{
    FileControl fc;
    stub.set_lamda(ADDR(FileControl, isVideo),
        [](FileControl *, const QString &) -> bool { return true; });

    QVariantList urls;
    urls << QVariant();

    EXPECT_FALSE(fc.haveVideo(urls));
}

// ============ isTrashPath ============

// Test: isTrashPath returns true for a path under the trash directory
TEST_F(FileControlTest, IsTrashPath_UnderTrashDir_ReturnsTrue)
{
    FileControl fc;
    QString trashDir = QDir::cleanPath(
        QDir::homePath() + "/.local/share/deepin/deepin-album-delete");
    QString pathUnderTrash = trashDir + "/somefile.jpg";

    EXPECT_TRUE(fc.isTrashPath(QUrl::fromLocalFile(pathUnderTrash).toString()));
}

// Test: isTrashPath returns false for a normal path
TEST_F(FileControlTest, IsTrashPath_NormalPath_ReturnsFalse)
{
    FileControl fc;
    EXPECT_FALSE(fc.isTrashPath(QUrl::fromLocalFile("/tmp/test.jpg").toString()));
}

// ============ isCanWrite ============

// Test: isCanWrite returns true for a file in a writable temp directory
TEST_F(FileControlTest, IsCanWrite_WritableFile_ReturnsTrue)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("writable.txt");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    EXPECT_TRUE(fc.isCanWrite(QUrl::fromLocalFile(filePath).toString()));
}

// Test: isCanWrite returns false for a non-existent path
TEST_F(FileControlTest, IsCanWrite_NonExistent_ReturnsFalse)
{
    FileControl fc;
    EXPECT_FALSE(fc.isCanWrite(QUrl::fromLocalFile("/nonexistent_99999.txt").toString()));
}

// ============ isCanDelete ============

// Test: isCanDelete returns true for a writable, readable, local file
TEST_F(FileControlTest, IsCanDelete_LocalWritableReadable_ReturnsTrue)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("deletable.txt");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType {
            return imageViewerSpace::PathTypeLOCAL;
        });

    EXPECT_TRUE(fc.isCanDelete(QUrl::fromLocalFile(filePath).toString()));
}

// Test: isCanDelete returns false when pathType is PathTypeAPPLE
TEST_F(FileControlTest, IsCanDelete_ApplePath_ReturnsFalse)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("apple_file.txt");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType {
            return imageViewerSpace::PathTypeAPPLE;
        });

    EXPECT_FALSE(fc.isCanDelete(QUrl::fromLocalFile(filePath).toString()));
}

// ============ isCanPrint ============

// Test: isCanPrint returns true when isImage is true and file is readable
TEST_F(FileControlTest, IsCanPrint_ImageAndReadable_ReturnsTrue)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("printable.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isImage),
        [](const QString &) -> bool { return true; });

    EXPECT_TRUE(fc.isCanPrint(QUrl::fromLocalFile(filePath).toString()));
}

// Test: isCanPrint returns false when isImage is false
TEST_F(FileControlTest, IsCanPrint_NotImage_ReturnsFalse)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("not_image.txt");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isImage),
        [](const QString &) -> bool { return false; });

    EXPECT_FALSE(fc.isCanPrint(QUrl::fromLocalFile(filePath).toString()));
}

// ============ isCanSupportOcr ============

// Test: isCanSupportOcr returns true for readable, non-dynamic image
TEST_F(FileControlTest, IsCanSupportOcr_StaticReadable_ReturnsTrue)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("ocr.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getImageType),
        [](const QString &) -> imageViewerSpace::ImageType {
            return imageViewerSpace::ImageTypeStatic;
        });

    EXPECT_TRUE(fc.isCanSupportOcr(QUrl::fromLocalFile(filePath).toString()));
}

// Test: isCanSupportOcr returns false for dynamic image type
TEST_F(FileControlTest, IsCanSupportOcr_DynamicType_ReturnsFalse)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("dynamic.gif");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getImageType),
        [](const QString &) -> imageViewerSpace::ImageType {
            return imageViewerSpace::ImageTypeDynamic;
        });

    EXPECT_FALSE(fc.isCanSupportOcr(QUrl::fromLocalFile(filePath).toString()));
}

// ============ isRotatable (QStringList overload) ============

// Test: Empty list returns true
TEST_F(FileControlTest, IsRotatable_EmptyList_ReturnsTrue)
{
    FileControl fc;
    QStringList paths;
    EXPECT_TRUE(fc.isRotatable(paths));
}

// Test: List with empty strings returns true
TEST_F(FileControlTest, IsRotatable_EmptyStrings_ReturnsTrue)
{
    FileControl fc;
    QStringList paths;
    paths << "" << "";
    EXPECT_TRUE(fc.isRotatable(paths));
}

// Test: All elements rotatable (QString overload stubbed true) → true
TEST_F(FileControlTest, IsRotatable_AllRotatable_ReturnsTrue)
{
    FileControl fc;
    stub.set_lamda(static_cast<bool(FileControl::*)(const QString &)>(&FileControl::isRotatable),
        [](FileControl *, const QString &) -> bool { return true; });

    QStringList paths;
    paths << "/tmp/a.jpg" << "/tmp/b.jpg";
    EXPECT_TRUE(fc.isRotatable(paths));
}

// Test: One element not rotatable → false
TEST_F(FileControlTest, IsRotatable_OneNotRotatable_ReturnsFalse)
{
    FileControl fc;
    stub.set_lamda(static_cast<bool(FileControl::*)(const QString &)>(&FileControl::isRotatable),
        [](FileControl *, const QString &path) -> bool {
            return path != "/tmp/bad.jpg";
        });

    QStringList paths;
    paths << "/tmp/good.jpg" << "/tmp/bad.jpg";
    EXPECT_FALSE(fc.isRotatable(paths));
}

// ============ isSupportSetWallpaper ============

// Test: Supported format (jpg) and readable → true
TEST_F(FileControlTest, IsSupportSetWallpaper_JpgReadable_ReturnsTrue)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("wallpaper.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    EXPECT_TRUE(fc.isSupportSetWallpaper(QUrl::fromLocalFile(filePath).toString()));
}

// Test: Unsupported format (txt) → false
TEST_F(FileControlTest, IsSupportSetWallpaper_TxtFormat_ReturnsFalse)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("wallpaper.txt");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    EXPECT_FALSE(fc.isSupportSetWallpaper(QUrl::fromLocalFile(filePath).toString()));
}

// Test: Non-existent file → false (not readable)
TEST_F(FileControlTest, IsSupportSetWallpaper_NonExistent_ReturnsFalse)
{
    FileControl fc;
    EXPECT_FALSE(fc.isSupportSetWallpaper(
        QUrl::fromLocalFile("/nonexistent_99999.jpg").toString()));
}

// ============ getDirImagePath ============

// Test: Empty path returns empty list
TEST_F(FileControlTest, GetDirImagePath_EmptyPath_ReturnsEmpty)
{
    FileControl fc;
    QStringList result = fc.getDirImagePath(QString());
    EXPECT_TRUE(result.isEmpty());
}

// Test: Directory with image files (isImage stubbed true) returns URLs
TEST_F(FileControlTest, GetDirImagePath_WithImages_ReturnsUrls)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isImage),
        [](const QString &) -> bool { return true; });

    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    // Create two image files in the temp dir
    QString f1 = tmpDir.filePath("img1.jpg");
    QString f2 = tmpDir.filePath("img2.png");
    for (const auto &p : {f1, f2}) {
        QFile f(p);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("data");
        f.close();
    }

    // Use one of the files as the path argument; getDirImagePath finds sibling files
    QStringList result = fc.getDirImagePath(QUrl::fromLocalFile(f1).toString());
    EXPECT_EQ(result.size(), 2);
    for (const auto &url : result) {
        EXPECT_TRUE(url.startsWith("file://"));
    }
}

// Test: Mix of image and non-image (isImage returns false for .txt) filters correctly
TEST_F(FileControlTest, GetDirImagePath_MixedFiles_ReturnsOnlyImages)
{
    FileControl fc;
    // Stub FileControl::isImage to return true only for .jpg files
    stub.set_lamda(static_cast<bool(FileControl::*)(const QString &)>(&FileControl::isImage),
        [](FileControl *, const QString &path) -> bool {
            return path.endsWith(".jpg", Qt::CaseInsensitive);
        });

    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString imgFile = tmpDir.filePath("photo.jpg");
    QString txtFile = tmpDir.filePath("notes.txt");
    for (const auto &p : {imgFile, txtFile}) {
        QFile f(p);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("data");
        f.close();
    }

    QStringList result = fc.getDirImagePath(QUrl::fromLocalFile(imgFile).toString());
    EXPECT_EQ(result.size(), 1);
    EXPECT_TRUE(result[0].contains("photo.jpg"));
}
