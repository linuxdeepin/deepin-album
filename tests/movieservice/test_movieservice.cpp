// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | MovieService::parseFromFile | high | 4 | 3 | 4 |
 * | MovieService::instance | low | 2 | 1 | 1 |
 * | MovieService::getMovieInfo | low | 4 | 3 | 4 |
 * | MovieService::getMovieCover | low | 3 | 2 | 3 |
 * | MovieService::clearMovieInfoCache | mid | 1 | 1 | 1 |
 * | MovieService::libPath | low | 2 | 2 | 2 |
 * | MovieInfo::sizeStr | low | 4 | 3 | 3 |
 *
 * 分支列表 (parseFromFile):
 *   B1: fi.exists() == false → return invalid MovieInfo (mi.valid=false)
 *   B2: fi exists but empty → avformat_open_input fails → return invalid
 *   B3: fi exists with garbage content → avformat_open_input fails → return invalid
 *   B4: fi exists, avformat_open_input succeeds but find_stream_info fails → return invalid
 *
 * 分支列表 (instance):
 *   B1: first call initializes m_movieService under call_once
 *   B2: subsequent calls return the same singleton pointer
 *
 * 分支列表 (getMovieInfo):
 *   B1: URL is already cached → return cached MovieInfo without parsing
 *   B2: URL is not local → return and cache an invalid MovieInfo
 *   B3: local file does not exist → return and cache an invalid MovieInfo
 *   B4: local file exists → parseFromFile, cache the result, and trim after 30 entries
 *
 * 分支列表 (getMovieCover):
 *   B1: thumbnailer is not initialized → initialize it
 *   B2: any thumbnailer function pointer is null → return an empty QImage
 *   B3: thumbnailer is ready → generate the cover for the URL
 *
 * 分支列表 (clearMovieInfoCache):
 *   B1: remove entries whose URL matches the request
 *
 * 分支列表 (libPath):
 *   B1: exact library exists in Qt/LD_LIBRARY_PATH → return requested name
 *   B2: only a versioned candidate exists → return the sorted last candidate
 *
 * 分支列表 (MovieInfo::sizeStr):
 *   B1: fileSize <= 1K → return plain byte count
 *   B2: fileSize > 1K and <= 1M → return one-decimal K value
 *   B3: fileSize > 1M and <= 1G → return one-decimal M value
 *   B4: fileSize > 1G → return one-decimal G value
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QFileInfo>
#include <QFile>
#include <QImage>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QUrl>
#include <QString>
#include <QLibraryInfo>
#include <memory>

#include "stubext.h"
#include "addr_pri.h"
#include "imageengine/movieservice.h"

using namespace stub_ext;

// parseFromFile is private — access without "#define private public" (ODR-safe).
// The private constructor is reached via the public instance() singleton.
ACCESS_PRIVATE_FUN(MovieService, MovieInfo(const QFileInfo &), parseFromFile)
ACCESS_PRIVATE_FUN(MovieService, QString(const QString &), libPath)

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

// B1/B2: repeated instance() calls preserve the singleton, even when parent is supplied
TEST_F(MovieServiceTest, Instance_RepeatedCalls_ReturnsSamePointer)
{
    // Arrange
    QObject parent;

    // Act
    MovieService *first = MovieService::instance();
    MovieService *second = MovieService::instance(&parent);

    // Assert
    EXPECT_NE(first, nullptr);
    EXPECT_EQ(first, second);
    EXPECT_EQ(MovieService::instance(), first);
}

// B2: non-local URLs never reach parseFromFile but are cached
TEST_F(MovieServiceTest, GetMovieInfo_NonLocalUrl_ReturnsInvalid)
{
    // Arrange
    MovieService *service = MovieService::instance();
    int parseCount = 0;
    stub.set_lamda(get_private_fun::MovieServiceparseFromFile(),
                   [&parseCount](MovieService *, const QFileInfo &) -> MovieInfo {
                       ++parseCount;
                       return MovieInfo{};
                   });
    QUrl url("https://example.deepin.org/V-5796-nonlocal-video.mp4");

    // Act
    MovieInfo first = service->getMovieInfo(url);
    MovieInfo second = service->getMovieInfo(url);

    // Assert
    EXPECT_FALSE(first.valid);
    EXPECT_TRUE(first.filePath.isEmpty());
    EXPECT_EQ(first.title, second.title);
    EXPECT_EQ(parseCount, 0);
}

// B3: missing local files are not parsed, and the invalid result is cached
TEST_F(MovieServiceTest, GetMovieInfo_NonExistentFile_ReturnsInvalid)
{
    // Arrange
    MovieService *service = MovieService::instance();
    QTemporaryDir tmpDir;
    QUrl url = QUrl::fromLocalFile(tmpDir.filePath("V-5796-missing-video.mp4"));
    int parseCount = 0;
    stub.set_lamda(get_private_fun::MovieServiceparseFromFile(),
                   [&parseCount](MovieService *, const QFileInfo &) -> MovieInfo {
                       ++parseCount;
                       return MovieInfo{};
                   });

    // Act
    MovieInfo first = service->getMovieInfo(url);
    MovieInfo second = service->getMovieInfo(url);

    // Assert
    EXPECT_FALSE(first.valid);
    EXPECT_TRUE(first.filePath.isEmpty());
    EXPECT_EQ(first.title, second.title);
    EXPECT_EQ(parseCount, 0);
}

// B4: an existing but unsupported file is parsed once, then served from cache
TEST_F(MovieServiceTest, GetMovieInfo_NonVideoFile_ReturnsInvalidAndCaches)
{
    // Arrange
    MovieService *service = MovieService::instance();
    QTemporaryDir tmpDir;
    QString filePath = tmpDir.filePath("V-5796-not-video.txt");
    QFile testFile(filePath);
    ASSERT_TRUE(testFile.open(QIODevice::WriteOnly));
    ASSERT_TRUE(testFile.write("plain text, not a video") > 0);
    testFile.close();
    QUrl url = QUrl::fromLocalFile(filePath);
    int parseCount = 0;
    stub.set_lamda(get_private_fun::MovieServiceparseFromFile(),
                   [&parseCount](MovieService *, const QFileInfo &) -> MovieInfo {
                       ++parseCount;
                       return MovieInfo{};
                   });

    // Act
    MovieInfo first = service->getMovieInfo(url);
    MovieInfo second = service->getMovieInfo(url);

    // Assert
    EXPECT_FALSE(first.valid);
    EXPECT_TRUE(first.title.isEmpty());
    EXPECT_EQ(first.title, second.title);
    EXPECT_EQ(parseCount, 1);
}

// B1: a cached URL does not invoke parseFromFile a second time
TEST_F(MovieServiceTest, GetMovieInfo_CacheHit_SecondCallSkipsParse)
{
    // Arrange
    MovieService *service = MovieService::instance();
    QTemporaryDir tmpDir;
    QString filePath = tmpDir.filePath("V-5796-cache-hit.mp4");
    QFile localFile(filePath);
    ASSERT_TRUE(localFile.open(QIODevice::WriteOnly));
    localFile.close();
    QUrl url = QUrl::fromLocalFile(filePath);
    int parseCount = 0;
    stub.set_lamda(get_private_fun::MovieServiceparseFromFile(),
                   [&parseCount](MovieService *, const QFileInfo &) -> MovieInfo {
                       ++parseCount;
                       MovieInfo cached{};
                       cached.title = "V-5796-cached";
                       return cached;
                   });

    // Act
    MovieInfo first = service->getMovieInfo(url);
    MovieInfo second = service->getMovieInfo(url);

    // Assert
    EXPECT_EQ(first.title, QString("V-5796-cached"));
    EXPECT_EQ(second.title, first.title);
    EXPECT_EQ(parseCount, 1);
}

// B3: getMovieCover handles a missing file without generating a non-empty image
TEST_F(MovieServiceTest, GetMovieCover_NonExistentUrl_ReturnsEmptyImage)
{
    // Arrange
    MovieService *service = MovieService::instance();
    QTemporaryDir tmpDir;
    QUrl url = QUrl::fromLocalFile(tmpDir.filePath("V-5796-missing-cover.mp4"));

    // Act
    QImage cover = service->getMovieCover(url);

    // Assert
    EXPECT_TRUE(cover.isNull());
    EXPECT_EQ(cover.width(), 0);
    EXPECT_EQ(cover.height(), 0);
}

// B3: getMovieCover does not treat a text file as a valid video cover
TEST_F(MovieServiceTest, GetMovieCover_NonVideoFile_ReturnsEmptyImage)
{
    // Arrange
    MovieService *service = MovieService::instance();
    QTemporaryDir tmpDir;
    QString filePath = tmpDir.filePath("V-5796-not-video-cover.txt");
    QFile testFile(filePath);
    ASSERT_TRUE(testFile.open(QIODevice::WriteOnly));
    ASSERT_TRUE(testFile.write("plain text, no cover") > 0);
    testFile.close();
    QUrl url = QUrl::fromLocalFile(filePath);

    // Act
    QImage cover = service->getMovieCover(url);

    // Assert
    EXPECT_TRUE(cover.isNull());
    EXPECT_EQ(cover.width(), 0);
    EXPECT_EQ(cover.height(), 0);
}

// B1: clearing a cached URL forces a new parse on the next call
TEST_F(MovieServiceTest, ClearMovieInfoCache_RemovesEntry_AndReparses)
{
    // Arrange
    MovieService *service = MovieService::instance();
    QTemporaryDir tmpDir;
    QString filePath = tmpDir.filePath("V-5796-clear-cache.mp4");
    QFile localFile(filePath);
    ASSERT_TRUE(localFile.open(QIODevice::WriteOnly));
    localFile.close();
    QUrl url = QUrl::fromLocalFile(filePath);
    int parseCount = 0;
    stub.set_lamda(get_private_fun::MovieServiceparseFromFile(),
                   [&parseCount](MovieService *, const QFileInfo &) -> MovieInfo {
                       ++parseCount;
                       return MovieInfo{};
                   });
    service->getMovieInfo(url);
    service->getMovieInfo(url);

    // Act
    service->clearMovieInfoCache(url);
    MovieInfo reparsed = service->getMovieInfo(url);

    // Assert
    EXPECT_FALSE(reparsed.valid);
    EXPECT_EQ(parseCount, 2);
}

// B1: clearing an uncached URL is harmless and preserves later parsing
TEST_F(MovieServiceTest, ClearMovieInfoCache_UncachedUrl_DoesNotCrash)
{
    // Arrange
    MovieService *service = MovieService::instance();
    QTemporaryDir tmpDir;
    QUrl uncachedUrl = QUrl::fromLocalFile(tmpDir.filePath("V-5796-uncached.mp4"));
    QString cachedFilePath = tmpDir.filePath("V-5796-cached-before-clear.mp4");
    QFile cachedFile(cachedFilePath);
    ASSERT_TRUE(cachedFile.open(QIODevice::WriteOnly));
    cachedFile.close();
    QUrl cachedUrl = QUrl::fromLocalFile(cachedFilePath);
    int parseCount = 0;
    stub.set_lamda(get_private_fun::MovieServiceparseFromFile(),
                   [&parseCount](MovieService *, const QFileInfo &) -> MovieInfo {
                       ++parseCount;
                       return MovieInfo{};
                   });

    // Act
    service->clearMovieInfoCache(uncachedUrl);
    MovieInfo result = service->getMovieInfo(cachedUrl);

    // Assert
    EXPECT_FALSE(result.valid);
    EXPECT_EQ(parseCount, 1);
}

// B1: an exact LD_LIBRARY_PATH match returns the requested library name
TEST_F(MovieServiceTest, LibPath_ExactLdPathMatch_ReturnsRequestedName)
{
    // Arrange
    MovieService *service = MovieService::instance();
    QTemporaryDir tmpDir;
    QString libraryName = "libalbum_ut_marker.so";
    QFile libraryFile(tmpDir.filePath(libraryName));
    ASSERT_TRUE(libraryFile.open(QIODevice::WriteOnly));
    libraryFile.close();
    const QByteArray oldLdPath = qgetenv("LD_LIBRARY_PATH");
    ASSERT_TRUE(oldLdPath.isEmpty());
    qputenv("LD_LIBRARY_PATH", tmpDir.path().toLocal8Bit());

    // Act
    QString path = call_private_fun::MovieServicelibPath(*service, libraryName);
    qunsetenv("LD_LIBRARY_PATH");

    // Assert
    EXPECT_EQ(path, libraryName);
    EXPECT_FALSE(path.isEmpty());
}

// B2: an absent library falls back to the requested name
TEST_F(MovieServiceTest, LibPath_AbsentLibrary_ReturnsRequestedName)
{
    // Arrange
    MovieService *service = MovieService::instance();
    QString libraryName = "libalbum_ut_missing.so";

    // Act
    QString path = call_private_fun::MovieServicelibPath(*service, libraryName);

    // Assert
    EXPECT_EQ(path, libraryName);
    EXPECT_FALSE(path.contains('/'));
}

// B1: sizes at or below 1K omit the unit and retain exact bytes
TEST_F(MovieServiceTest, SizeStr_BytesRange_ReturnsPlainCount)
{
    // Arrange
    MovieInfo bytesInfo{};
    bytesInfo.fileSize = 512;

    // Act
    QString bytes = bytesInfo.sizeStr();

    // Assert
    EXPECT_EQ(bytes, QString("512"));
    EXPECT_FALSE(bytes.contains('K'));
}

// B2: sizes above 1K and at or below 1M format one decimal K value
TEST_F(MovieServiceTest, SizeStr_KilobyteRange_ReturnsKilobyteValue)
{
    // Arrange
    MovieInfo kilobyteInfo{};
    kilobyteInfo.fileSize = 1536;

    // Act
    QString kilobytes = kilobyteInfo.sizeStr();

    // Assert
    EXPECT_EQ(kilobytes, QString("1.5K"));
    EXPECT_TRUE(kilobytes.endsWith('K'));
}

// B3: sizes above 1M and at or below 1G format one decimal M value
TEST_F(MovieServiceTest, SizeStr_MegabyteRange_ReturnsMegabyteValue)
{
    // Arrange
    MovieInfo megabyteInfo{};
    megabyteInfo.fileSize = 2621440;

    // Act
    QString megabytes = megabyteInfo.sizeStr();

    // Assert
    EXPECT_EQ(megabytes, QString("2.5M"));
    EXPECT_TRUE(megabytes.endsWith('M'));
}

// B4: sizes above 1G format one decimal G value
TEST_F(MovieServiceTest, SizeStr_GigabyteRange_ReturnsGigabyteValue)
{
    // Arrange
    MovieInfo gigabyteInfo{};
    gigabyteInfo.fileSize = 3328599297;

    // Act
    QString gigabytes = gigabyteInfo.sizeStr();

    // Assert
    EXPECT_EQ(gigabytes, QString("3.1G"));
    EXPECT_TRUE(gigabytes.endsWith('G'));
}

// MovieInfo aggregate initialization keeps explicit defaults for unset metadata
TEST_F(MovieServiceTest, MovieInfo_DefaultFields_HaveStableInitialValues)
{
    // Arrange
    MovieInfo info{};

    // Act
    const MovieInfo initialized = std::move(info);

    // Assert
    EXPECT_FALSE(initialized.valid);
    EXPECT_EQ(initialized.fileSize, 0);
    EXPECT_EQ(initialized.duration, QString("-"));
    EXPECT_EQ(initialized.width, -1);
    EXPECT_EQ(initialized.height, -1);
    EXPECT_EQ(initialized.vCodecID, QString("-"));
}
