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
