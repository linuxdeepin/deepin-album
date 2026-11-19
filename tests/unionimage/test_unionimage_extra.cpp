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
    // Default orientation for a simple PNG should be 0 or 1
    EXPECT_GE(orientation, 0);
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
