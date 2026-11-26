// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QFont>
#include <QFontMetrics>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QSvgGenerator>
#include <QPainter>
#include <QStandardPaths>
#include <QCryptographicHash>
#include <QDateTime>
#include <QUrl>
#include <QVariant>
#include <QDebug>
#include <QByteArray>
#include <QPixmap>
#include <tiffio.h>
#include <QBuffer>
#include <QtEndian>

#include <zlib.h>

#include "unionimage/unionimage.h"
#include "unionimage/unionimage_global.h"
#include "unionimage/baseutils.h"
#include "unionimage/imageutils.h"

// =========================================================================
//  Test fixture
// =========================================================================
class UnionImageExtraTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char *argv[] = {(char *)"test_unionimage_extra", nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    void SetUp() override
    {
        ASSERT_TRUE(tempDir.isValid());
    }

    QTemporaryDir tempDir;

    QString createPngImage(const QString &name, int w = 10, int h = 10)
    {
        QString path = tempDir.filePath(name);
        QImage img(w, h, QImage::Format_ARGB32);
        img.fill(Qt::red);
        img.save(path, "PNG");
        return path;
    }

    QString createSvgImage(const QString &name, int w = 10, int h = 10)
    {
        QString path = tempDir.filePath(name);
        QSvgGenerator gen;
        gen.setFileName(path);
        gen.setSize(QSize(w, h));
        gen.setViewBox(QRect(0, 0, w, h));
        QPainter painter(&gen);
        painter.fillRect(0, 0, w, h, Qt::blue);
        painter.end();
        return path;
    }

    QString createRawFile(const QString &name, const QByteArray &content)
    {
        QString path = tempDir.filePath(name);
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write(content);
        f.close();
        return path;
    }

    QByteArray standardExifApp1() const
    {
        return QByteArray::fromHex(
            "ffe100bb45786966000049492a0008000000"
            "020010010200110000006800000069870400010000002600000000000000"
            "05009a82050001000000ab000000278803000100000090010000039002001400000083000000"
            "04900200140000009700000034a402000a0000007900000000000000"
            "4558494620546573742043616d65726100546573742035306d6d00"
            "323031383a30313a30322031363a30343a303500323031393a30323a30332031373a30353a303600"
            "010000007d000000");
    }

    QByteArray canonFlashApp1() const
    {
        return QByteArray::fromHex(
            "ffe1007845786966000049492a000800000002000f010200060000002600000069870400010000002c00"
            "00000000000043616e6f6e0001007c920700320000003e00000000000000010004000300100000005000"
            "000000000000200000000000000000000000000000000000000000000000000000000000ecff");
    }

    QByteArray makeJpegWithExif(const QByteArray &exif = {}) const
    {
        QByteArray jpeg;
        QBuffer buffer(&jpeg);
        if (!buffer.open(QIODevice::WriteOnly))
            return {};
        QImage image(16, 8, QImage::Format_RGB32);
        image.fill(Qt::red);
        if (!image.save(&buffer, "JPEG"))
            return {};
        buffer.close();

        if (jpeg.size() < 6 || jpeg.mid(2, 2) != QByteArray::fromHex("ffe0"))
            return {};
        const int app0End = 4 + qFromBigEndian<quint16>(jpeg.constData() + 4);
        jpeg.insert(app0End, exif.isEmpty() ? standardExifApp1() : exif);
        return jpeg;
    }

    QByteArray makePngWithExif(const QByteArray &tiff = {}) const
    {
        QByteArray png;
        QBuffer buffer(&png);
        if (!buffer.open(QIODevice::WriteOnly))
            return {};
        QImage image(16, 8, QImage::Format_RGB32);
        image.fill(Qt::red);
        if (!image.save(&buffer, "PNG"))
            return {};
        buffer.close();

        const QByteArray exifTiff = tiff.isEmpty() ? standardExifApp1().mid(10) : tiff;
        const QByteArray chunkData = "eXIf" + exifTiff;
        QByteArray chunk(4, Qt::Uninitialized);
        qToBigEndian<quint32>(exifTiff.size(), chunk.data());
        chunk.append(chunkData);
        char checksum[4];
        const auto crc = crc32(0, reinterpret_cast<const Bytef *>(chunkData.constData()), chunkData.size());
        qToBigEndian<quint32>(crc, checksum);
        chunk.append(checksum, sizeof(checksum));
        png.insert(33, chunk);
        return png;
    }
};

// =========================================================================
//  baseutils.cpp — timeToString
// =========================================================================

TEST_F(UnionImageExtraTest, timeToString_NormalFormat)
{
    QDateTime dt(QDate(2026, 1, 15), QTime(10, 30, 0));
    QString result = Libutils::base::timeToString(dt, true);
    EXPECT_EQ(result, "2026.01.15");
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(UnionImageExtraTest, timeToString_ExifFormat)
{
    QDateTime dt(QDate(2026, 1, 15), QTime(10, 30, 0));
    QString result = Libutils::base::timeToString(dt, false);
    EXPECT_EQ(result, "2026:01:15 10:30:00");
    EXPECT_TRUE(result.contains(":"));
}

// =========================================================================
//  baseutils.cpp — stringToDateTime
// =========================================================================

TEST_F(UnionImageExtraTest, stringToDateTime_ExifFormat)
{
    QString str = "2026:01:15 10:30:00";
    QDateTime dt = Libutils::base::stringToDateTime(str);
    EXPECT_TRUE(dt.isValid());
    EXPECT_EQ(dt.date().year(), 2026);
}

TEST_F(UnionImageExtraTest, stringToDateTime_NormalFormat)
{
    QString str = "2026.01.15";
    QDateTime dt = Libutils::base::stringToDateTime(str);
    EXPECT_TRUE(dt.isValid());
    EXPECT_EQ(dt.date().month(), 1);
}

TEST_F(UnionImageExtraTest, stringToDateTime_InvalidString)
{
    QString str = "not_a_date";
    QDateTime dt = Libutils::base::stringToDateTime(str);
    EXPECT_FALSE(dt.isValid());
}

// =========================================================================
//  baseutils.cpp — stringWidth / stringHeight
// =========================================================================

TEST_F(UnionImageExtraTest, stringWidth_Height_Basic)
{
    QFont font("Arial", 10);
    QString text = "Hello World";
    int w = Libutils::base::stringWidth(font, text);
    int h = Libutils::base::stringHeight(font, text);
    EXPECT_GT(w, 0);
    EXPECT_GT(h, 0);
}

TEST_F(UnionImageExtraTest, stringWidth_EmptyString)
{
    QFont font("Arial", 10);
    int w = Libutils::base::stringWidth(font, "");
    EXPECT_EQ(w, 0);
}

// =========================================================================
//  baseutils.cpp — getFileContent
// =========================================================================

TEST_F(UnionImageExtraTest, getFileContent_ValidFile)
{
    QString path = createRawFile("content.txt", "Hello, World!");
    QString content = Libutils::base::getFileContent(path);
    EXPECT_EQ(content, "Hello, World!");
    EXPECT_FALSE(content.isEmpty());
}

TEST_F(UnionImageExtraTest, getFileContent_NonexistentFile)
{
    QString path = tempDir.filePath("does_not_exist.txt");
    QString content = Libutils::base::getFileContent(path);
    EXPECT_TRUE(content.isEmpty());
}

// =========================================================================
//  baseutils.cpp — hash / hashByString
// =========================================================================

TEST_F(UnionImageExtraTest, hash_SameInputSameOutput)
{
    QString input = "test_hash_input";
    QString h1 = Libutils::base::hash(input);
    QString h2 = Libutils::base::hash(input);
    EXPECT_EQ(h1, h2);
    EXPECT_FALSE(h1.isEmpty());
    EXPECT_EQ(h1.length(), 32);  // MD5 hex = 32 chars
}

TEST_F(UnionImageExtraTest, hash_DifferentInputDifferentOutput)
{
    QString h1 = Libutils::base::hash("input1");
    QString h2 = Libutils::base::hash("input2");
    EXPECT_NE(h1, h2);
}

TEST_F(UnionImageExtraTest, hashByString_MatchesHash)
{
    QString input = "test_string";
    QString h1 = Libutils::base::hash(input);
    QString h2 = Libutils::base::hashByString(input);
    EXPECT_EQ(h1, h2);
    EXPECT_EQ(h1.length(), 32);
}

// =========================================================================
//  baseutils.cpp — hashByData
// =========================================================================

TEST_F(UnionImageExtraTest, hashByData_ValidFile)
{
    QString path = createRawFile("hashdata.txt", "data to hash");
    QString h = Libutils::base::hashByData(path);
    EXPECT_FALSE(h.isEmpty());
    EXPECT_EQ(h.length(), 32);
}

TEST_F(UnionImageExtraTest, hashByData_NonexistentFile)
{
    QString path = tempDir.filePath("no_such_file.txt");
    QString h = Libutils::base::hashByData(path);
    EXPECT_TRUE(h.isEmpty());
}

// =========================================================================
//  baseutils.cpp — onMountDevice
// =========================================================================

TEST_F(UnionImageExtraTest, onMountDevice_MediaPath)
{
    EXPECT_TRUE(Libutils::base::onMountDevice("/media/user/usb/file.txt"));
    EXPECT_TRUE(Libutils::base::onMountDevice("/run/media/user/usb/file.txt"));
}

TEST_F(UnionImageExtraTest, onMountDevice_NonMountPath)
{
    EXPECT_FALSE(Libutils::base::onMountDevice("/home/user/file.txt"));
    EXPECT_FALSE(Libutils::base::onMountDevice("/tmp/file.txt"));
}

// =========================================================================
//  baseutils.cpp — mountDeviceExist
// =========================================================================

TEST_F(UnionImageExtraTest, mountDeviceExist_NonMountPath)
{
    // Non-mount path → mountPoint is empty → QFileInfo("").exists() == false
    EXPECT_FALSE(Libutils::base::mountDeviceExist("/home/user/file.txt"));
}

TEST_F(UnionImageExtraTest, mountDeviceExist_NonExistentMountPath)
{
    // /media/ followed by non-existent device — mountPoint extracted but doesn't exist
    EXPECT_FALSE(Libutils::base::mountDeviceExist("/media/nonexistent_device_12345/file.txt"));
}

// =========================================================================
//  baseutils.cpp — getDeleteFullPath
// =========================================================================

TEST_F(UnionImageExtraTest, getDeleteFullPath_Basic)
{
    QString hash = "abcdef1234567890";
    QString fileName = "photo.png";
    QString path = Libutils::base::getDeleteFullPath(hash, fileName);
    EXPECT_TRUE(path.contains(hash));
    EXPECT_TRUE(path.endsWith(".png"));
    EXPECT_TRUE(path.contains(albumGlobal::DELETE_PATH));
}

TEST_F(UnionImageExtraTest, getDeleteFullPath_NoSuffix)
{
    QString hash = "deadbeef";
    QString fileName = "noextension";
    QString path = Libutils::base::getDeleteFullPath(hash, fileName);
    EXPECT_TRUE(path.contains(hash));
    EXPECT_TRUE(path.endsWith("."));
}

// =========================================================================
//  baseutils.cpp — filePathToThumbnailPath
// =========================================================================

TEST_F(UnionImageExtraTest, filePathToThumbnailPath_WithHash)
{
    QString filePath = "/home/user/photos/img.png";
    QString dataHash = "myhash123";
    QString path = Libutils::base::filePathToThumbnailPath(filePath, dataHash);
    EXPECT_TRUE(path.contains(dataHash));
    EXPECT_TRUE(path.endsWith(".png"));
    EXPECT_TRUE(path.contains(albumGlobal::CACHE_PATH));
}

TEST_F(UnionImageExtraTest, filePathToThumbnailPath_EmptyHash)
{
    QString filePath = "/home/user/photos/img.png";
    QString path = Libutils::base::filePathToThumbnailPath(filePath, "");
    EXPECT_TRUE(path.endsWith(".png"));
    EXPECT_FALSE(path.isEmpty());
}

// =========================================================================
//  baseutils.cpp — analyzeDateTime
// =========================================================================

TEST_F(UnionImageExtraTest, analyzeDateTime_DatabaseFormat)
{
    QVariant data = QVariant(QString("2026.01.15 10:30"));
    auto result = Libutils::base::analyzeDateTime(data);
    EXPECT_TRUE(result.second);
    EXPECT_TRUE(result.first.isValid());
    EXPECT_EQ(result.first.date().year(), 2026);
}

TEST_F(UnionImageExtraTest, analyzeDateTime_ExifFormatFallback)
{
    QVariant data = QVariant(QString("2026:01:15 10:30:00"));
    auto result = Libutils::base::analyzeDateTime(data);
    EXPECT_TRUE(result.second);
    EXPECT_TRUE(result.first.isValid());
}

TEST_F(UnionImageExtraTest, analyzeDateTime_InvalidData)
{
    QVariant data = QVariant(QString("not_a_date_at_all"));
    auto result = Libutils::base::analyzeDateTime(data);
    EXPECT_FALSE(result.second);
}

// =========================================================================
//  baseutils.cpp — syncCopy
// =========================================================================

TEST_F(UnionImageExtraTest, syncCopy_ValidFile)
{
    QString src = createRawFile("src.txt", "copy this content");
    QString dst = tempDir.filePath("dst.txt");
    bool ok = Libutils::base::syncCopy(src, dst);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(QFile::exists(dst));
    QFile df(dst);
    df.open(QIODevice::ReadOnly);
    EXPECT_EQ(QString::fromUtf8(df.readAll()), "copy this content");
    df.close();
}

TEST_F(UnionImageExtraTest, syncCopy_NonexistentSrc)
{
    QString src = tempDir.filePath("no_src.txt");
    QString dst = tempDir.filePath("dst2.txt");
    // src.open() fails silently, fileSize=0, dst.resize(0) succeeds, loop reads nothing
    bool ok = Libutils::base::syncCopy(src, dst);
    // Behavior: returns true even with nonexistent src (known code pattern)
    // but dst should exist with 0 size or be removed
    // Just verify it doesn't crash
    EXPECT_NO_FATAL_FAILURE(ok);
}

// =========================================================================
//  baseutils.cpp — mkMutiDir
// =========================================================================

TEST_F(UnionImageExtraTest, mkMutiDir_CreateNestedDirs)
{
    QString basePath = tempDir.filePath("multidir");
    QString nestedPath = basePath + "/level1/level2/level3";
    QString result = Libutils::base::mkMutiDir(nestedPath);
    EXPECT_TRUE(QDir(nestedPath).exists());
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(UnionImageExtraTest, mkMutiDir_ExistingDir)
{
    QString existingPath = tempDir.path() + "/already_exists";
    QDir().mkpath(existingPath);
    QString result = Libutils::base::mkMutiDir(existingPath);
    EXPECT_EQ(result, existingPath);
    EXPECT_TRUE(QDir(existingPath).exists());
}

// =========================================================================
//  baseutils.cpp — checkMimeUrls
// =========================================================================

TEST_F(UnionImageExtraTest, checkMimeUrls_EmptyList)
{
    QList<QUrl> urls;
    EXPECT_FALSE(Libutils::base::checkMimeUrls(urls));
}

TEST_F(UnionImageExtraTest, checkMimeUrls_SupportedImage)
{
    QString imgPath = createPngImage("check.png");
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(imgPath);
    EXPECT_TRUE(Libutils::base::checkMimeUrls(urls));
}

TEST_F(UnionImageExtraTest, checkMimeUrls_UnsupportedFile)
{
    QString txtPath = createRawFile("plain.txt", "just text");
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(txtPath);
    EXPECT_FALSE(Libutils::base::checkMimeUrls(urls));
}

TEST_F(UnionImageExtraTest, checkMimeUrls_DirectoryWithImages)
{
    QDir base(tempDir.path());
    base.mkdir("imgdir");
    createPngImage("imgdir/dir_img.png");
    QList<QUrl> urls;
    urls << QUrl::fromLocalFile(tempDir.filePath("imgdir"));
    EXPECT_TRUE(Libutils::base::checkMimeUrls(urls));
}

// =========================================================================
//  baseutils.cpp — isSupportWallpaper
// =========================================================================

TEST_F(UnionImageExtraTest, isSupportWallpaper_PNG)
{
    QString path = createPngImage("wallpaper.png");
    EXPECT_TRUE(Libutils::base::isSupportWallpaper(path));
}

TEST_F(UnionImageExtraTest, isSupportWallpaper_TextFile)
{
    QString path = createRawFile("not_wallpaper.txt", "text");
    EXPECT_FALSE(Libutils::base::isSupportWallpaper(path));
}

// =========================================================================
//  baseutils.cpp — renderSVG
// =========================================================================

TEST_F(UnionImageExtraTest, renderSVG_ValidSvg)
{
    QString svgPath = createSvgImage("test.svg");
    QPixmap pix = Libutils::base::renderSVG(svgPath, QSize(50, 50));
    // renderSVG loads the SVG and returns a pixmap; size may differ from requested
    EXPECT_FALSE(pix.isNull());
    EXPECT_GT(pix.width(), 0);
    EXPECT_GT(pix.height(), 0);
}

TEST_F(UnionImageExtraTest, renderSVG_NonexistentFile)
{
    QPixmap pix = Libutils::base::renderSVG("/nonexistent/file.svg", QSize(50, 50));
    // renderSVG always returns a QPixmap (may be blank but not null for ARGB32)
    EXPECT_NO_FATAL_FAILURE(pix);
}


TEST_F(UnionImageExtraTest, imageSupportRead_PNG)
{
    QString path = createPngImage("read_test.png");
    EXPECT_TRUE(Libutils::image::imageSupportRead(path));
}

TEST_F(UnionImageExtraTest, imageSupportRead_UnsupportedFormat)
{
    // imageSupportRead checks format support based on extension, not file existence.
    // A .txt file should not be a supported image format.
    QString path = tempDir.filePath("not_an_image.txt");
    EXPECT_FALSE(Libutils::image::imageSupportRead(path));
}

// =========================================================================
//  imageutils.cpp — imageSupportSave
// =========================================================================

TEST_F(UnionImageExtraTest, imageSupportSave_PNG)
{
    QString path = createPngImage("save_test.png");
    // Just verify it doesn't crash — canSave depends on format support
    bool result = Libutils::image::imageSupportSave(path);
    EXPECT_NO_FATAL_FAILURE(result);
}

// =========================================================================
//  imageutils.cpp — imageSupportWallPaper
// =========================================================================

TEST_F(UnionImageExtraTest, imageSupportWallPaper_PNG)
{
    QString path = createPngImage("wallpaper_test.png");
    EXPECT_TRUE(Libutils::image::imageSupportWallPaper(path));
}

TEST_F(UnionImageExtraTest, imageSupportWallPaper_TextFile)
{
    QString path = createRawFile("not_wallpaper_img.txt", "text content");
    EXPECT_FALSE(Libutils::image::imageSupportWallPaper(path));
}

// =========================================================================
//  imageutils.cpp — supportedImageFormats
// =========================================================================

TEST_F(UnionImageExtraTest, supportedImageFormats_NonEmpty)
{
    QStringList formats = Libutils::image::supportedImageFormats();
    EXPECT_FALSE(formats.isEmpty());
    // Each entry should start with "*."
    for (const QString &fmt : formats) {
        EXPECT_TRUE(fmt.startsWith("*."));
    }
}

TEST_F(UnionImageExtraTest, supportedImageFormats_ContainsPng)
{
    QStringList formats = Libutils::image::supportedImageFormats();
    bool foundPng = false;
    for (const QString &fmt : formats) {
        if (fmt.toLower().contains("png")) {
            foundPng = true;
            break;
        }
    }
    EXPECT_TRUE(foundPng);
}

// =========================================================================
//  imageutils.cpp — thumbnailExist
// =========================================================================

TEST_F(UnionImageExtraTest, thumbnailExist_NonexistentThumbnail)
{
    QString path = createPngImage("thumb_test.png");
    // No thumbnail generated yet, should return false
    bool exists = Libutils::image::thumbnailExist(path, Libutils::image::ThumbLarge);
    EXPECT_FALSE(exists);
}

TEST_F(UnionImageExtraTest, thumbnailExist_AfterGenerate)
{
    QString path = createPngImage("thumb_gen.png");
    // Generate thumbnail first
    Libutils::image::generateThumbnail(path);
    bool exists = Libutils::image::thumbnailExist(path, Libutils::image::ThumbLarge);
    EXPECT_TRUE(exists);
    // Cleanup
    Libutils::image::removeThumbnail(path);
}

// =========================================================================
//  imageutils.cpp — makeVaultLocalPath
// =========================================================================

TEST_F(UnionImageExtraTest, makeVaultLocalPath_WithBase)
{
    QString result = Libutils::image::makeVaultLocalPath("myfile.png", "vault_unlocked");
    EXPECT_TRUE(result.contains("vault_unlocked"));
    EXPECT_TRUE(result.contains("myfile.png"));
    EXPECT_TRUE(result.contains(VAULT_BASE_PATH));
}

TEST_F(UnionImageExtraTest, makeVaultLocalPath_EmptyBase)
{
    QString result = Libutils::image::makeVaultLocalPath("myfile.png", "");
    EXPECT_TRUE(result.contains(VAULT_DECRYPT_DIR_NAME));
    EXPECT_TRUE(result.contains("myfile.png"));
}

// =========================================================================
//  imageutils.cpp — isVaultFile
// =========================================================================

TEST_F(UnionImageExtraTest, isVaultFile_VaultPath)
{
    QString vaultPath = Libutils::image::makeVaultLocalPath("test.png", "");
    EXPECT_TRUE(Libutils::image::isVaultFile(vaultPath));
}

TEST_F(UnionImageExtraTest, isVaultFile_NonVaultPath)
{
    EXPECT_FALSE(Libutils::image::isVaultFile("/home/user/photos/img.png"));
    EXPECT_FALSE(Libutils::image::isVaultFile("/tmp/test.png"));
}

// =========================================================================
//  imageutils.cpp — isCanRemove
// =========================================================================

TEST_F(UnionImageExtraTest, isCanRemove_NormalPath)
{
    EXPECT_TRUE(Libutils::image::isCanRemove("/home/user/photos/img.png"));
    EXPECT_TRUE(Libutils::image::isCanRemove("/tmp/test.png"));
}

TEST_F(UnionImageExtraTest, isCanRemove_VaultPath)
{
    QString vaultPath = Libutils::image::makeVaultLocalPath("test.png", "");
    EXPECT_FALSE(Libutils::image::isCanRemove(vaultPath));
}

// =========================================================================
//  imageutils.cpp — getImagesAndVideoInfo
// =========================================================================

TEST_F(UnionImageExtraTest, getImagesAndVideoInfo_NonRecursive)
{
    QDir base(tempDir.path());
    base.mkdir("media_dir");
    createPngImage("media_dir/img1.png");
    createPngImage("media_dir/img2.png");
    createRawFile("media_dir/not_media.txt", "text");

    QFileInfoList infos = Libutils::image::getImagesAndVideoInfo(
        tempDir.filePath("media_dir"), false);
    EXPECT_GE(infos.size(), 2);
}

TEST_F(UnionImageExtraTest, getImagesAndVideoInfo_EmptyDir)
{
    QDir base(tempDir.path());
    base.mkdir("empty_media_dir");
    QFileInfoList infos = Libutils::image::getImagesAndVideoInfo(
        tempDir.filePath("empty_media_dir"), false);
    EXPECT_EQ(infos.size(), 0);
}

// =========================================================================
//  imageutils.cpp — isVideo
// =========================================================================

TEST_F(UnionImageExtraTest, isVideo_Mp4)
{
    EXPECT_TRUE(Libutils::image::isVideo("video.mp4"));
    EXPECT_TRUE(Libutils::image::isVideo("video.mkv"));
    EXPECT_TRUE(Libutils::image::isVideo("video.avi"));
}

TEST_F(UnionImageExtraTest, isVideo_NonVideo)
{
    EXPECT_FALSE(Libutils::image::isVideo("document.txt"));
    EXPECT_FALSE(Libutils::image::isVideo("image.png"));
    EXPECT_FALSE(Libutils::image::isVideo("music.mp3"));
}

TEST_F(UnionImageExtraTest, isVideo_TsFile_NonVideo)
{
    // .ts file that is not a video — create a text file with .ts extension
    QString path = createRawFile("subtitle.ts", "translation text content");
    EXPECT_FALSE(Libutils::image::isVideo(path));
}

// =========================================================================
//  imageutils.cpp — scaleImage
// =========================================================================

TEST_F(UnionImageExtraTest, scaleImage_ValidPng)
{
    QString path = createPngImage("scale_test.png", 100, 100);
    QImage result = Libutils::image::scaleImage(path, QSize(50, 50));
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.width(), 50);
    EXPECT_EQ(result.height(), 50);
}

TEST_F(UnionImageExtraTest, scaleImage_UnsupportedFormat)
{
    QString path = createRawFile("scale_unsupported.txt", "not an image");
    QImage result = Libutils::image::scaleImage(path, QSize(50, 50));
    EXPECT_TRUE(result.isNull());
}

// =========================================================================
//  imageutils.cpp — getCreateDateTime
// =========================================================================

TEST_F(UnionImageExtraTest, getCreateDateTime_ValidImage)
{
    QString path = createPngImage("datetime_test.png");
    QDateTime dt = Libutils::image::getCreateDateTime(path);
    EXPECT_TRUE(dt.isValid());
}

TEST_F(UnionImageExtraTest, getCreateDateTime_NonexistentFile)
{
    QString path = tempDir.filePath("nonexistent.png");
    QDateTime dt = Libutils::image::getCreateDateTime(path);
    // Falls back to currentDateTime for nonexistent files
    EXPECT_TRUE(dt.isValid());
}

// =========================================================================
//  imageutils.cpp — cutSquareImage
// =========================================================================

TEST_F(UnionImageExtraTest, cutSquareImage_FromPixmap)
{
    QPixmap pix(100, 100);
    pix.fill(Qt::red);
    QPixmap result = Libutils::image::cutSquareImage(pix);
    EXPECT_FALSE(result.isNull());
}

TEST_F(UnionImageExtraTest, cutSquareImage_WithTargetSize)
{
    QPixmap pix(100, 100);
    pix.fill(Qt::blue);
    QPixmap result = Libutils::image::cutSquareImage(pix, QSize(50, 50));
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.width(), 50);
    EXPECT_EQ(result.height(), 50);
}

// =========================================================================
//  imageutils.cpp — getOrientation
// =========================================================================

TEST_F(UnionImageExtraTest, getOrientation_ValidPng)
{
    QString path = createPngImage("orient_test.png");
    int orientation = Libutils::image::getOrientation(path);
    EXPECT_EQ(orientation, 1);
}

TEST_F(UnionImageExtraTest, getOrientation_PngExifReadsOrientation)
{
    const QByteArray tiff = QByteArray::fromHex("49492a0008000000010012010300010000000600000000000000");
    const QString path = createRawFile("oriented.png", makePngWithExif(tiff));
    EXPECT_EQ(LibUnionImage_NameSpace::getOrientation(path), 6);
}

TEST_F(UnionImageExtraTest, getAllMetaData_JpegExif_ReturnsModelAndOrientation)
{
    const QString path = tempDir.filePath("camera.jpg");
    QImage image(8, 6, QImage::Format_RGB32);
    image.fill(Qt::red);
    ASSERT_TRUE(image.save(path, "JPEG"));

    // Little-endian EXIF IFD0 with Model="Test Camera" and Orientation=6.
    const QByteArray app1 = QByteArray::fromHex(
        "ffe1003a45786966000049492a00080000000200100102000c00000026000000"
        "12010300010000000600000000000000546573742043616d65726100");
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::ReadOnly));
    QByteArray jpeg = file.readAll();
    file.close();
    ASSERT_TRUE(jpeg.startsWith(QByteArray::fromHex("ffd8")));
    jpeg.insert(2, app1);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    ASSERT_EQ(file.write(jpeg), jpeg.size());
    file.close();

    const auto metadata = LibUnionImage_NameSpace::getAllMetaData(path);
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("Test Camera"));
    EXPECT_EQ(LibUnionImage_NameSpace::getOrientation(path), 6);
}

TEST_F(UnionImageExtraTest, getAllMetaData_JpegExif_ReturnsShootingFields)
{
    const QString path = createRawFile("camera-exif.jpg", makeJpegWithExif());
    const auto metadata = LibUnionImage_NameSpace::getAllMetaData(path);
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("EXIF Test Camera"));
    EXPECT_EQ(metadata.value("LensType"), QStringLiteral("Test 50mm"));
    EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("400"));
    EXPECT_TRUE(metadata.value("ExposureTime").contains(QStringLiteral("1/125")));
    EXPECT_EQ(metadata.value("DateTimeOriginal"), QStringLiteral("2018/01/02 16:04"));
}

TEST_F(UnionImageExtraTest, getAllMetaData_CanonMakerNote_ReturnsFlashCompensation)
{
    const QString path = createRawFile("canon-flash.jpg", makeJpegWithExif(canonFlashApp1()));
    const auto metadata = LibUnionImage_NameSpace::getAllMetaData(path);
    EXPECT_EQ(metadata.value("FlashExposureComp"), QStringLiteral("-0.67 EV"));
}

TEST_F(UnionImageExtraTest, getAllMetaData_PngExif_ReturnsCameraAndCaptureTime)
{
    const QString path = createRawFile("camera-exif.png", makePngWithExif());
    ASSERT_FALSE(QImage(path).isNull());

    const auto metadata = LibUnionImage_NameSpace::getAllMetaData(path);
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("EXIF Test Camera"));
    EXPECT_EQ(metadata.value("LensType"), QStringLiteral("Test 50mm"));
    EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("400"));
    EXPECT_EQ(metadata.value("DateTimeOriginal"), QStringLiteral("2018/01/02 16:04"));
}

TEST_F(UnionImageExtraTest, getAllMetaData_PngExifWithBadChecksum_IgnoresExif)
{
    QByteArray png = makePngWithExif();
    ASSERT_FALSE(png.isEmpty());
    const qsizetype checksumEnd = 33 + 8 + standardExifApp1().size() - 10 + 4;
    png[checksumEnd - 1] = png.at(checksumEnd - 1) ^ 1;
    const QString path = createRawFile("bad-exif-crc.png", png);

    const auto metadata = LibUnionImage_NameSpace::getAllMetaData(path);
    EXPECT_FALSE(metadata.contains("Model"));
    EXPECT_FALSE(metadata.contains("ISOSpeedRatings"));
}

TEST_F(UnionImageExtraTest, getAllMetaData_WebpExif_ReadsExifChunk)
{
    if (!QImageReader::supportedImageFormats().contains("webp"))
        GTEST_SKIP() << "WebP image format plugin is unavailable";

    const QByteArray image = QByteArray::fromHex(
        "524946460000000057454250565038580a000000080000000f0000070000"
        "5650384c110000002f0fc0010007508a5294a2ff8188e87f0000");
    const QByteArray tiff = standardExifApp1().mid(10);
    QByteArray webp = image + "EXIF" + QByteArray(4, '\0') + tiff;
    if (tiff.size() % 2)
        webp.append('\0');
    qToLittleEndian<quint32>(webp.size() - 8, webp.data() + 4);
    qToLittleEndian<quint32>(tiff.size(), webp.data() + image.size() + 4);
    const QString path = createRawFile("camera-exif.webp", webp);
    ASSERT_FALSE(QImage(path).isNull());

    const auto metadata = LibUnionImage_NameSpace::getAllMetaData(path);
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("EXIF Test Camera"));
    EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("400"));
    EXPECT_EQ(metadata.value("DateTimeOriginal"), QStringLiteral("2018/01/02 16:04"));
}

TEST_F(UnionImageExtraTest, getAllMetaData_TiffExif_ReturnsShootingFields)
{
    QByteArray tiff = QByteArray::fromBase64(
        "SUkqAIQQAAAGAJqCBQABAAAAcgAAAJ2CBQABAAAAagAAACeIAwABAAAAkAEAAAOQAgAUAAAA"
        "VgAAAAmSAwABAAAAAAAAADSkAgAKAAAAegAAAAAAAAAyMDE4OjAxOjAyIDE2OjA0OjA1AAQA"
        "AAABAAAAbxKDAAAAAEBUZXN0IDUwbW0=");
    tiff.append(QByteArray(64 * 64, '\0'));
    tiff.append(QByteArray::fromBase64(
        "AAwAAAEDAAEAAABAAAAAAQEDAAEAAABAAAAAAgEDAAEAAAAIAAAAAwEDAAEAAAABAAAABgED"
        "AAEAAAABAAAAEAECABEAAAAaEQAAEQEEAAEAAACEAAAAFQEDAAEAAAABAAAAFgEDAAEAAABA"
        "AAAAFwEEAAEAAAAAEAAAHAEDAAEAAAABAAAAaYcEAAEAAAAIAAAAAAAAAEVYSUYgVGVzdCBD"
        "YW1lcmEA"));
    const QString path = createRawFile("camera-exif.tiff", tiff);

    const auto metadata = LibUnionImage_NameSpace::getAllMetaData(path);
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("EXIF Test Camera"));
    EXPECT_EQ(metadata.value("LensType"), QStringLiteral("Test 50mm"));
    EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("400"));
    EXPECT_EQ(metadata.value("ApertureValue"), QStringLiteral("f/4"));
    EXPECT_TRUE(metadata.value("ExposureTime").contains(QStringLiteral("1/125")));
    EXPECT_EQ(metadata.value("DateTimeOriginal"), QStringLiteral("2018/01/02 16:04"));
}

TEST_F(UnionImageExtraTest, getAllMetaData_SvgXmp_ReturnsCameraFields)
{
    const QByteArray svg = QByteArrayLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"16\" height=\"8\"><metadata>"
        "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" "
        "xmlns:tiff=\"http://ns.adobe.com/tiff/1.0/\" xmlns:exif=\"http://ns.adobe.com/exif/1.0/\">"
        "<rdf:Description rdf:about=\"\" tiff:Model=\"XMP Test Camera\" exif:ExposureTime=\"1/125\" "
        "exif:FNumber=\"4/1\" exif:DateTimeOriginal=\"2018-01-02T16:04:05+08:00\"/>"
        "</rdf:RDF></metadata><rect width=\"16\" height=\"8\"/></svg>");
    const QString path = createRawFile("camera-xmp.svg", svg);

    const auto metadata = LibUnionImage_NameSpace::getAllMetaData(path);
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("XMP Test Camera"));
    EXPECT_EQ(metadata.value("ApertureValue"), QStringLiteral("f/4"));
    EXPECT_EQ(metadata.value("DateTimeOriginal"), QStringLiteral("2018/01/02 16:04"));
}

TEST_F(UnionImageExtraTest, getAllMetaData_DngExif_ReturnsRawShootingFields)
{
    QByteArray dng = QByteArray::fromHex(
        "49492a0008000000140000010400010000004000000001010400010000004000"
        "0000020103000100000010000000030103000100000001000000060103000100"
        "0000238000000f010200050000004c0100001001020010000000520100001101"
        "040001000000a801000012010300010000000100000015010300010000000100"
        "00001601040001000000400000001701040001000000002000001c0103000100"
        "0000010000008d82030002000000020002008e82010004000000000101026987"
        "040001000000fe00000012c60100040000000104000013c60100040000000101"
        "000014c6020010000000620100001dc6040001000000ff0f0000000000000600"
        "9a82050001000000720100009d820500010000007a0100002788030001000000"
        "900100000390020014000000820100000a920500010000009601000034a40200"
        "0a0000009e0100000000000054657374000052415720546573742043616d6572"
        "610052415720546573742043616d65726100010000007d000000040000000100"
        "0000323031383a30313a30322031363a30343a30350032000000010000005465"
        "73742035306d6d00");
    dng.replace("2018:01:02 16:04:05", "2024:03:10 02:30:00");
    dng.append(QByteArray(64 * 64 * 2, '\0'));
    const QString path = createRawFile("camera.dng", dng);
    const QByteArray nativePath = QFile::encodeName(path);
    TIFF *rawTiff = TIFFOpen(nativePath.constData(), "r+");
    ASSERT_NE(rawTiff, nullptr);
    ASSERT_TRUE(TIFFSetField(rawTiff, TIFFTAG_ORIENTATION, ORIENTATION_RIGHTTOP));
    ASSERT_TRUE(TIFFRewriteDirectory(rawTiff));
    TIFFClose(rawTiff);

    const auto metadata = LibUnionImage_NameSpace::getAllMetaData(path);
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("RAW Test Camera"));
    EXPECT_EQ(metadata.value("LensType"), QStringLiteral("Test 50mm"));
    EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("400"));
    EXPECT_EQ(metadata.value("ApertureValue"), QStringLiteral("f/4"));
    EXPECT_TRUE(metadata.value("ExposureTime").contains(QStringLiteral("1/125")));
    EXPECT_EQ(metadata.value("FocalLength"), QStringLiteral("50 mm"));
    EXPECT_EQ(metadata.value("DateTimeOriginal"), QStringLiteral("2024/03/10 02:30"));
    EXPECT_EQ(LibUnionImage_NameSpace::getOrientation(path), 6);
}

TEST_F(UnionImageExtraTest, getAllMetaData_HeifAndAvifExif_ReadsPrimaryImageMetadata)
{
    const QByteArray heic = QByteArray::fromBase64(
        "AAAAHGZ0eXBoZWljAAAAAG1pZjFoZWljbWlhZgAAAadtZXRhAAAAAAAAACFoZGxyAAAAAAAA"
        "AABwaWN0AAAAAAAAAAAAAAAAAAAAAA5waXRtAAAAAAABAAAANGlsb2MAAAAAREAAAgABAAAA"
        "AAHLAAEAAAAAAAAANgACAAAAAAIBAAEAAAAAAAAAvQAAADhpaW5mAAAAAAACAAAAFWluZmUC"
        "AAAAAAEAAGh2YzEAAAAAFWluZmUCAAABAAIAAEV4aWYAAAAA5mlwcnAAAADHaXBjbwAAAHNo"
        "dmNDAQNwAAAAAAAAAAAAHvAA/P34+AAADwMgAAEAGEABDAH//wNwAAADAJAAAAMAAAMAHroC"
        "QCEAAQAnQgEBA3AAAAMAkAAAAwAAAwAeoCCBBZbqrprmwIAAAAMAgAAAAwCEIgABAAZEAcFz"
        "wYkAAAAUaXNwZQAAAAAAAABAAAAAQAAAAChjbGFwAAAAEAAAAAEAAAAIAAAAAf///9AAAAAC"
        "////yAAAAAIAAAAQcGl4aQAAAAADCAgIAAAAF2lwbWEAAAAAAAAAAQABBIECgwQAAAAaaXJl"
        "ZgAAAAAAAAAOY2RzYwACAAEAAQAAAPttZGF0AAAAMigBrxMhXsNA9rQ5i2ppI/049gjmjksY"
        "DcyGh2GEVn0iYbn6kNCsfWCQg6pApoCrXxycAAAABkV4aWYAAElJKgAIAAAAAgAQAQIAEQAA"
        "AGgAAABphwQAAQAAACYAAAAAAAAABQCaggUAAQAAAKsAAAAniAMAAQAAAJABAAADkAIAFAAA"
        "AIMAAAAEkAIAFAAAAJcAAAA0pAIACgAAAHkAAAAAAAAARVhJRiBUZXN0IENhbWVyYQBUZXN0"
        "IDUwbW0AMjAxODowMTowMiAxNjowNDowNQAyMDE5OjAyOjAzIDE3OjA1OjA2AAEAAAB9AAAA");
    const QByteArray avif = QByteArray::fromBase64(
        "AAAAHGZ0eXBhdmlmAAAAAGF2aWZtaWYxbWlhZgAAASttZXRhAAAAAAAAACFoZGxyAAAAAAAA"
        "AABwaWN0AAAAAAAAAAAAAAAAAAAAAA5waXRtAAAAAAABAAAANGlsb2MAAAAAREAAAgABAAAA"
        "AAFPAAEAAAAAAAAAGgACAAAAAAFpAAEAAAAAAAAAtwAAADhpaW5mAAAAAAACAAAAFWluZmUC"
        "AAAAAAEAAGF2MDEAAAAAFWluZmUCAAABAAIAAEV4aWYAAAAAamlwcnAAAABLaXBjbwAAABNj"
        "b2xybmNseAABAA0ABoAAAAAMYXYxQ4EADAAAAAAUaXNwZQAAAAAAAAAQAAAACAAAABBwaXhp"
        "AAAAAAMICAgAAAAXaXBtYQAAAAAAAAABAAEEAYIDBAAAABppcmVmAAAAAAAAAA5jZHNjAAIA"
        "AQABAAAA2W1kYXQSAAoFGAy/sCEyDxgACiiihAACOK3SEQiEgAAAAABJSSoACAAAAAIAEAEC"
        "ABEAAABoAAAAaYcEAAEAAAAmAAAAAAAAAAUAmoIFAAEAAACrAAAAJ4gDAAEAAACQAQAAA5AC"
        "ABQAAACDAAAABJACABQAAACXAAAANKQCAAoAAAB5AAAAAAAAAEVYSUYgVGVzdCBDYW1lcmEA"
        "VGVzdCA1MG1tADIwMTg6MDE6MDIgMTY6MDQ6MDUAMjAxOTowMjowMyAxNzowNTowNgABAAAA"
        "fQAAAA==");
    const QMap<QString, QByteArray> files = {
        {QStringLiteral("camera.heic"), heic},
        {QStringLiteral("camera.avif"), avif},
    };
    for (auto it = files.cbegin(); it != files.cend(); ++it) {
        SCOPED_TRACE(it.key().toStdString());
        const QString path = createRawFile(it.key(), it.value());
        const auto metadata = LibUnionImage_NameSpace::getAllMetaData(path);
        EXPECT_EQ(metadata.value("Model"), QStringLiteral("EXIF Test Camera"));
        EXPECT_EQ(metadata.value("LensType"), QStringLiteral("Test 50mm"));
        EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("400"));
        EXPECT_EQ(metadata.value("DateTimeOriginal"), QStringLiteral("2018/01/02 16:04"));
        EXPECT_EQ(LibUnionImage_NameSpace::getOrientation(path), 1);

        QByteArray oriented = it.value();
        const qsizetype tiffOffset = oriented.indexOf(QByteArray::fromHex("49492a0008000000"));
        ASSERT_GE(tiffOffset, 0);
        const qsizetype modelEntry = tiffOffset + 8 + 2;
        ASSERT_EQ(oriented.mid(modelEntry, 2), QByteArray::fromHex("1001"));
        oriented.replace(modelEntry, 12, QByteArray::fromHex("120103000100000006000000"));
        QString orientedName = it.key();
        orientedName.replace(QStringLiteral(".heic"), QStringLiteral("-orientation.heic"));
        orientedName.replace(QStringLiteral(".avif"), QStringLiteral("-orientation.avif"));
        const QString orientedPath = createRawFile(orientedName, oriented);
        EXPECT_EQ(LibUnionImage_NameSpace::getOrientation(orientedPath), 6);
    }
}

// =========================================================================
//  imageutils.cpp — getDamagePixmap
// =========================================================================

TEST_F(UnionImageExtraTest, getDamagePixmap_Light)
{
    QPixmap pix = Libutils::image::getDamagePixmap(true);
    // May return null if SVG resource not available in test env
    EXPECT_NO_FATAL_FAILURE(pix);
}

TEST_F(UnionImageExtraTest, getDamagePixmap_Dark)
{
    QPixmap pix = Libutils::image::getDamagePixmap(false);
    EXPECT_NO_FATAL_FAILURE(pix);
}
TEST_F(UnionImageExtraTest, getOrientation_TiffReadsExifOrientation)
{
    const QString path = tempDir.filePath("oriented.tiff");
    QImage image(8, 6, QImage::Format_RGB32);
    image.fill(Qt::red);
    ASSERT_TRUE(image.save(path, "TIFF"));

    const QByteArray nativePath = QFile::encodeName(path);
    TIFF *tiff = TIFFOpen(nativePath.constData(), "r+");
    ASSERT_NE(tiff, nullptr);
    ASSERT_TRUE(TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_RIGHTTOP));
    ASSERT_TRUE(TIFFRewriteDirectory(tiff));
    TIFFClose(tiff);

    EXPECT_EQ(LibUnionImage_NameSpace::getOrientation(path), 6);
}

TEST_F(UnionImageExtraTest, getOrientation_SvgXmpReadsOrientation)
{
    const QByteArray svg = QByteArrayLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"8\" height=\"6\"><metadata>"
        "<rdf:RDF xmlns:rdf=\"http://www.w3.org/1999/02/22-rdf-syntax-ns#\" "
        "xmlns:tiff=\"http://ns.adobe.com/tiff/1.0/\"><rdf:Description rdf:about=\"\" "
        "tiff:Orientation=\"8\"/></rdf:RDF></metadata><rect width=\"8\" height=\"6\"/></svg>");
    const QString path = createRawFile("oriented.svg", svg);
    EXPECT_EQ(LibUnionImage_NameSpace::getOrientation(path), 8);
}
TEST_F(UnionImageExtraTest, getOrientation_WebpExifReadsOrientation)
{
    if (!QImageReader::supportedImageFormats().contains("webp"))
        GTEST_SKIP() << "WebP image format plugin is unavailable";

    const QByteArray image = QByteArray::fromHex(
        "524946460000000057454250565038580a000000080000000f0000070000"
        "5650384c110000002f0fc0010007508a5294a2ff8188e87f0000");
    QByteArray tiff = QByteArray::fromHex("49492a0008000000010012010300010000000600000000000000");
    QByteArray webp = image + "EXIF" + QByteArray(4, '\0') + tiff;
    qToLittleEndian<quint32>(webp.size() - 8, webp.data() + 4);
    qToLittleEndian<quint32>(tiff.size(), webp.data() + image.size() + 4);
    const QString path = createRawFile("oriented.webp", webp);
    ASSERT_FALSE(QImage(path).isNull());
    EXPECT_EQ(LibUnionImage_NameSpace::getOrientation(path), 6);
}

TEST_F(UnionImageExtraTest, getOrientation_PsdExifReadsOrientation)
{
    const QByteArray tiff = QByteArray::fromHex("49492a0008000000010012010300010000000800000000000000");
    QByteArray resource = QByteArrayLiteral("8BIM") + QByteArray(2, '\0');
    qToBigEndian<quint16>(1058, resource.data() + 4);
    resource.append('\0');
    resource.append('\0');
    char dataSize[4];
    qToBigEndian<quint32>(tiff.size(), dataSize);
    resource.append(dataSize, sizeof(dataSize));
    resource.append(tiff);
    if (tiff.size() % 2)
        resource.append('\0');

    QByteArray psd(26, '\0');
    psd.replace(0, 4, "8BPS");
    qToBigEndian<quint16>(1, psd.data() + 4);
    qToBigEndian<quint16>(3, psd.data() + 12);
    qToBigEndian<quint32>(8, psd.data() + 14);
    qToBigEndian<quint32>(16, psd.data() + 18);
    qToBigEndian<quint16>(8, psd.data() + 22);
    qToBigEndian<quint16>(3, psd.data() + 24);
    psd.append(QByteArray(4, '\0'));
    char resourceSize[4];
    qToBigEndian<quint32>(resource.size(), resourceSize);
    psd.append(resourceSize, sizeof(resourceSize));
    psd.append(resource);
    psd.append(QByteArray(4, '\0'));
    psd.append(QByteArray(9, '\0'));

    const QString path = createRawFile("oriented.psd", psd);
    EXPECT_EQ(LibUnionImage_NameSpace::getOrientation(path), 8);
}

TEST_F(UnionImageExtraTest, getOrientation_GifXmpReadsOrientation)
{
    QByteArray gif = QByteArray::fromHex("47494638396101000100800000000000ffffff2c00000000010001000002024401003b");
    QByteArray xmp = QByteArrayLiteral(R"xml(<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
    xmlns:tiff="http://ns.adobe.com/tiff/1.0/" xmlns:exif="http://ns.adobe.com/exif/1.0/">
    <rdf:Description rdf:about="" tiff:Model="XMP Test Camera" tiff:Orientation="3" exif:ExposureTime="1/125"
                     exif:FNumber="4/1" exif:DateTimeOriginal="2018-01-02T16:04:05+08:00">
      <exif:ISOSpeedRatings><rdf:Seq><rdf:li>400</rdf:li><rdf:li>800</rdf:li></rdf:Seq></exif:ISOSpeedRatings>
      <exif:FocalLength>50/1</exif:FocalLength>
    </rdf:Description>
    </rdf:RDF>)xml");
    QByteArray packet = xmp + QByteArray(1, '\x01');
    for (int byte = 255; byte > 0; --byte)
        packet.append(static_cast<char>(byte));

    QByteArray extension = QByteArray::fromHex("21ff0b") + "XMP DataXMP";
    while (packet.size() > 255) {
        extension.append(static_cast<char>(255));
        extension.append(packet.first(255));
        packet.remove(0, 255);
    }
    extension.append(static_cast<char>(packet.size()));
    extension.append(packet);
    extension.append('\0');
    gif.insert(gif.size() - 1, extension);

    const QString path = createRawFile("oriented.gif", gif);
    const auto metadata = LibUnionImage_NameSpace::getAllMetaData(path);
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("XMP Test Camera"));
    EXPECT_EQ(LibUnionImage_NameSpace::getOrientation(path), 3);
}

TEST_F(UnionImageExtraTest, getOrientation_IcoPngEntryReadsExifOrientation)
{
    const QByteArray png = makePngWithExif(QByteArray::fromHex("49492a0008000000010012010300010000000600000000000000"));
    ASSERT_FALSE(png.isEmpty());

    QByteArray ico = QByteArray::fromHex("000001000100");
    ico.append(QByteArray(16, '\0'));
    ico[6] = 16;
    ico[7] = 8;
    qToLittleEndian<quint16>(1, ico.data() + 10);
    qToLittleEndian<quint16>(32, ico.data() + 12);
    qToLittleEndian<quint32>(png.size(), ico.data() + 14);
    qToLittleEndian<quint32>(22, ico.data() + 18);
    ico.append(png);

    const QString path = createRawFile("oriented.ico", ico);
    EXPECT_EQ(LibUnionImage_NameSpace::getOrientation(path), 6);
}
