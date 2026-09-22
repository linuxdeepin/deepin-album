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
#include <QSvgGenerator>
#include <QPainter>
#include <QStandardPaths>
#include <QDebug>
#include <QByteArray>

#include "unionimage/unionimage.h"
#include "unionimage/unionimage_global.h"
#include "unionimage/baseutils.h"
#include "unionimage/imageutils.h"

// =========================================================================
//  Test fixture
// =========================================================================
class UnionImageTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char *argv[] = {(char *)"test_unionimage", nullptr};
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
//  1. detectImageFormat
// =========================================================================

TEST_F(UnionImageTest, detectImageFormat_BMP)
{
    QByteArray data;
    data.append("BM");
    data.append(QByteArray(20, '\0'));
    QString path = createRawFile("test.bmp", data);
    EXPECT_EQ(LibUnionImage_NameSpace::detectImageFormat(path), "BMP");
}

TEST_F(UnionImageTest, detectImageFormat_PNG)
{
    QByteArray data;
    data.append('\x89');
    data.append("PNG\r\n\x1a\n");
    data.append(QByteArray(20, '\0'));
    QString path = createRawFile("test.png", data);
    EXPECT_EQ(LibUnionImage_NameSpace::detectImageFormat(path), "PNG");
}

TEST_F(UnionImageTest, detectImageFormat_JPG)
{
    QByteArray data;
    data.append('\xff');
    data.append('\xd8');
    data.append(QByteArray(20, '\0'));
    QString path = createRawFile("test.jpg", data);
    EXPECT_EQ(LibUnionImage_NameSpace::detectImageFormat(path), "JPG");
}

TEST_F(UnionImageTest, detectImageFormat_SVG)
{
    QByteArray data =
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"10\" height=\"10\">"
        "<rect width=\"10\" height=\"10\" fill=\"red\"/></svg>";
    QString path = createRawFile("test.svg", data);
    EXPECT_EQ(LibUnionImage_NameSpace::detectImageFormat(path), "SVG");
}

TEST_F(UnionImageTest, detectImageFormat_NonexistentFile)
{
    QString path = tempDir.filePath("does_not_exist.png");
    EXPECT_EQ(LibUnionImage_NameSpace::detectImageFormat(path), "");
}

TEST_F(UnionImageTest, detectImageFormat_UnknownFormat)
{
    QString path = createRawFile("test.unknown", "UNKNOWNFORMATDATA1234567890");
    EXPECT_EQ(LibUnionImage_NameSpace::detectImageFormat(path), "");
}

// =========================================================================
//  2. loadStaticImageFromFile
// =========================================================================

TEST_F(UnionImageTest, loadStaticImageFromFile_ValidPng)
{
    QString path = createPngImage("valid.png");
    QImage res;
    QString errorMsg;
    bool ok = LibUnionImage_NameSpace::loadStaticImageFromFile(path, res, errorMsg);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(res.isNull());
}

TEST_F(UnionImageTest, loadStaticImageFromFile_NonexistentFile)
{
    QString path = tempDir.filePath("nonexistent.png");
    QImage res;
    QString errorMsg;
    bool ok = LibUnionImage_NameSpace::loadStaticImageFromFile(path, res, errorMsg);
    EXPECT_FALSE(ok);
}

TEST_F(UnionImageTest, loadStaticImageFromFile_EmptyFile)
{
    QString path = createRawFile("empty.png", QByteArray());
    QImage res;
    QString errorMsg;
    bool ok = LibUnionImage_NameSpace::loadStaticImageFromFile(path, res, errorMsg);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(errorMsg.contains("error file") || errorMsg.contains("device"));
}

// =========================================================================
//  3. getPathType
// =========================================================================

TEST_F(UnionImageTest, getPathType_Local)
{
    EXPECT_EQ(LibUnionImage_NameSpace::getPathType("/tmp/test/image.png"),
              imageViewerSpace::PathTypeLOCAL);
}

TEST_F(UnionImageTest, getPathType_SMB)
{
    EXPECT_EQ(LibUnionImage_NameSpace::getPathType(
                  "smb-share:server=host,share=share/path"),
              imageViewerSpace::PathTypeSMB);
}

TEST_F(UnionImageTest, getPathType_MTP)
{
    EXPECT_EQ(LibUnionImage_NameSpace::getPathType(
                  "mtp:host=device,path=/path"),
              imageViewerSpace::PathTypeMTP);
}

TEST_F(UnionImageTest, getPathType_PTP)
{
    EXPECT_EQ(LibUnionImage_NameSpace::getPathType(
                  "gphoto2:host=device,path=/path"),
              imageViewerSpace::PathTypePTP);
}

TEST_F(UnionImageTest, getPathType_Trash)
{
    QString trashPath = QDir::homePath() + "/.local/share/Trash/files/image.png";
    EXPECT_EQ(LibUnionImage_NameSpace::getPathType(trashPath),
              imageViewerSpace::PathTypeRECYCLEBIN);
}

// =========================================================================
//  4. rotateImageFIle
// =========================================================================

TEST_F(UnionImageTest, rotateImageFIle_InvalidAngle)
{
    QString path = createPngImage("rotate_invalid.png");
    QString erroMsg;
    bool ok = LibUnionImage_NameSpace::rotateImageFIle(45, path, erroMsg);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(erroMsg.contains("unsupported angel"));
}

TEST_F(UnionImageTest, rotateImageFIle_SVG)
{
    QString path = createSvgImage("rotate.svg");
    QString erroMsg;
    bool ok = LibUnionImage_NameSpace::rotateImageFIle(90, path, erroMsg);
    EXPECT_TRUE(ok);
}

TEST_F(UnionImageTest, rotateImageFIle_NonexistentFile)
{
    QString path = tempDir.filePath("nonexistent.png");
    QString erroMsg;
    bool ok = LibUnionImage_NameSpace::rotateImageFIle(90, path, erroMsg);
    EXPECT_FALSE(ok);
}

// =========================================================================
//  5. rotateImageFIleWithImage
// =========================================================================

TEST_F(UnionImageTest, rotateImageFIleWithImage_InvalidAngle)
{
    QImage img(10, 10, QImage::Format_ARGB32);
    img.fill(Qt::red);
    QString path = createPngImage("rotate_img_invalid.png");
    QString erroMsg;
    bool ok = LibUnionImage_NameSpace::rotateImageFIleWithImage(45, img, path, erroMsg);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(erroMsg.contains("unsupported angel"));
}

TEST_F(UnionImageTest, rotateImageFIleWithImage_NullImage)
{
    QImage img;
    QString path = createPngImage("rotate_img_null.png");
    QString erroMsg;
    bool ok = LibUnionImage_NameSpace::rotateImageFIleWithImage(90, img, path, erroMsg);
    EXPECT_FALSE(ok);
}

TEST_F(UnionImageTest, rotateImageFIleWithImage_SVG)
{
    QImage img(10, 10, QImage::Format_ARGB32);
    img.fill(Qt::red);
    QString path = createSvgImage("rotate_img.svg");
    QString erroMsg;
    bool ok = LibUnionImage_NameSpace::rotateImageFIleWithImage(90, img, path, erroMsg);
    EXPECT_TRUE(ok);
}

// =========================================================================
//  6. getAllDirInDir
// =========================================================================

TEST_F(UnionImageTest, getAllDirInDir_NestedDirs)
{
    QDir base(tempDir.path());
    base.mkdir("dir1");
    base.mkdir("dir1/subdir1");
    base.mkdir("dir1/subdir2");
    base.mkdir("dir2");
    base.mkdir("dir2/subdir3");

    QDir root(tempDir.path());
    QFileInfoList result;
    LibUnionImage_NameSpace::getAllDirInDir(root, result);
    EXPECT_EQ(result.size(), 5);
}

TEST_F(UnionImageTest, getAllDirInDir_EmptyDir)
{
    QDir root(tempDir.path());
    QFileInfoList result;
    LibUnionImage_NameSpace::getAllDirInDir(root, result);
    EXPECT_EQ(result.size(), 0);
}

// =========================================================================
//  7. SpliteText  (Libutils::base)
// =========================================================================

TEST_F(UnionImageTest, SpliteText_ShortText)
{
    QFont font("Arial", 10);
    QString text = "Hi";
    QString result = Libutils::base::SpliteText(text, font, 1000, false);
    EXPECT_EQ(result, text);
}

TEST_F(UnionImageTest, SpliteText_LongText_bReturnFalse)
{
    QFont font("Arial", 10);
    QString text = "This is a very long text that should be split into multiple lines";
    QString result = Libutils::base::SpliteText(text, font, 30, false);
    EXPECT_NE(result, text);
    EXPECT_TRUE(result.contains("\n"));
}

TEST_F(UnionImageTest, SpliteText_LongText_bReturnTrue)
{
    QFont font("Arial", 10);
    QString text = "This is a very long text that should be split into multiple lines";
    QString result = Libutils::base::SpliteText(text, font, 30, true);
    EXPECT_NE(result, text);
    EXPECT_TRUE(result.contains("\n"));
}

TEST_F(UnionImageTest, SpliteText_EmptyText)
{
    QFont font("Arial", 10);
    QString text = "";
    QString result = Libutils::base::SpliteText(text, font, 100, false);
    EXPECT_EQ(result, text);
}

// =========================================================================
//  8. isSupportClassify  (Libutils::base)
// =========================================================================

TEST_F(UnionImageTest, isSupportClassify_PNG)
{
    QString path = createPngImage("classify.png");
    EXPECT_TRUE(Libutils::base::isSupportClassify(path));
}

TEST_F(UnionImageTest, isSupportClassify_GIF)
{
    QByteArray data;
    data.append("GIF89a");
    data.append(QByteArray(20, '\0'));
    QString path = createRawFile("classify.gif", data);
    EXPECT_FALSE(Libutils::base::isSupportClassify(path));
}

TEST_F(UnionImageTest, isSupportClassify_EmptyPath)
{
    EXPECT_FALSE(Libutils::base::isSupportClassify(""));
}

TEST_F(UnionImageTest, isSupportClassify_NonImageFile)
{
    QString path = createRawFile("classify.txt", "This is a text file");
    EXPECT_FALSE(Libutils::base::isSupportClassify(path));
}

// =========================================================================
//  9. getAllFileInDir  (Libutils::image)
// =========================================================================

TEST_F(UnionImageTest, getAllFileInDir_NestedFiles)
{
    QDir base(tempDir.path());
    base.mkdir("dir1");
    base.mkdir("dir1/subdir1");

    createRawFile("file1.txt", "content1");
    createRawFile("dir1/file2.txt", "content2");
    createRawFile("dir1/subdir1/file3.txt", "content3");

    QDir root(tempDir.path());
    QFileInfoList result;
    Libutils::image::getAllFileInDir(root, result);
    EXPECT_EQ(result.size(), 3);
}

TEST_F(UnionImageTest, getAllFileInDir_EmptyDir)
{
    QDir root(tempDir.path());
    QFileInfoList result;
    Libutils::image::getAllFileInDir(root, result);
    EXPECT_EQ(result.size(), 0);
}
