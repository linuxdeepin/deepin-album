// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | MovieService::parseFromFile | high | 4 | 3 | 4 |
 *
 * 分支列表 (parseFromFile):
 *   B1: fi.exists() == false → return invalid MovieInfo (mi.valid=false)
 *   B2: fi exists but empty → avformat_open_input fails → return invalid
 *   B3: fi exists with garbage content → avformat_open_input fails → return invalid
 *   B4: fi exists, avformat_open_input succeeds but find_stream_info fails → return invalid
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QFileInfo>
#include <QFile>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QString>
#include <memory>

#include "stubext.h"
#include "addr_pri.h"
#include "imageengine/movieservice.h"

using namespace stub_ext;

// parseFromFile is private — access without "#define private public" (ODR-safe).
// The private constructor is reached via the public instance() singleton.
ACCESS_PRIVATE_FUN(MovieService, MovieInfo(const QFileInfo &), parseFromFile)

class MovieServiceTest : public ::testing::Test
{
protected:
    StubExt stub;

    void SetUp() override
    {
        // No stubs needed — avformat functions handle invalid input gracefully
    }

    void TearDown() override
    {
        stub.clear();
    }
};

// B1: Non-existent file → return invalid MovieInfo
TEST_F(MovieServiceTest, ParseFromFile_NonExistentFile_ReturnsInvalid)
{
    // Arrange
    MovieService *service = MovieService::instance();
    QTemporaryDir tmpDir;
    QFileInfo fi(tmpDir.filePath("nonexistent_video.mp4"));

    // Act
    MovieInfo result = call_private_fun::MovieServiceparseFromFile(*service, fi);

    // Assert
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.filePath.size(), 0);
}

// B2: Empty file (0 bytes) → avformat_open_input fails → return invalid
TEST_F(MovieServiceTest, ParseFromFile_EmptyFile_ReturnsInvalid)
{
    // Arrange
    MovieService *service = MovieService::instance();
    QTemporaryFile tmpFile;
    tmpFile.open();  // Creates empty file
    QFileInfo fi(tmpFile.fileName());

    // Act
    MovieInfo result = call_private_fun::MovieServiceparseFromFile(*service, fi);

    // Assert
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.filePath.size(), 0);
}

// B3: File with garbage content → avformat_open_input fails → return invalid
TEST_F(MovieServiceTest, ParseFromFile_GarbageContent_ReturnsInvalid)
{
    // Arrange
    MovieService *service = MovieService::instance();
    QTemporaryFile tmpFile;
    tmpFile.open();
    tmpFile.write("This is not a valid video file content!");
    tmpFile.flush();
    QFileInfo fi(tmpFile.fileName());

    // Act
    MovieInfo result = call_private_fun::MovieServiceparseFromFile(*service, fi);

    // Assert
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.filePath.size(), 0);
}

// B4: File exists but not a valid video → no crash, returns invalid MovieInfo
TEST_F(MovieServiceTest, ParseFromFile_ExistingFile_DoesNotCrash)
{
    // Arrange
    MovieService *service = MovieService::instance();
    QTemporaryDir tmpDir;
    QString filePath = tmpDir.filePath("test_not_video.txt");
    QFile testFile(filePath);
    testFile.open(QIODevice::WriteOnly);
    testFile.write("dummy content for testing");
    testFile.close();
    QFileInfo fi(filePath);

    // Act
    MovieInfo result = call_private_fun::MovieServiceparseFromFile(*service, fi);

    // Assert — should not crash; invalid file → invalid MovieInfo
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(result.filePath.size(), 0);
}
