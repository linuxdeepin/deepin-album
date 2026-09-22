// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-or-later

#include <gtest/gtest.h>

#include <QGuiApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QTemporaryDir>
#include <QDebug>

#include "unionimage/unionimage.h"
#include "unionimage/unionimage_global.h"
#include "unionimage/baseutils.h"

// 测试数据目录（由 CMake 注入），存放已提交的测试图片（tests/files/album_test）
#ifndef ALBUM_TEST_DATA_DIR
#define ALBUM_TEST_DATA_DIR "."
#endif

namespace {
// 写入一个临时文件并返回其绝对路径
QString writeTempFile(const QString &dir, const QString &name, const QByteArray &content)
{
    const QString path = dir + QDir::separator() + name;
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(content);
        f.close();
    }
    return path;
}
}  // namespace

// 复用现有测试模式：全量 glob 生产源码 + stub-shadow + gtest_discover_tests + offscreen。
// QGuiApplication 供 QFont/QFontMetrics/QImage 在无头环境下使用。
class UnionImageTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_unionimage";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    void SetUp() override
    {
        ASSERT_TRUE(tmpDir.isValid());
        tmpPath = tmpDir.path();
    }

    QTemporaryDir tmpDir;
    QString tmpPath;
    // 已提交的真实 PNG 图片，用于加载/旋转/分类的成功用例
    const QString pngFixture = QStringLiteral(ALBUM_TEST_DATA_DIR "/test1.png");
};

// ============================ detectImageFormat ============================
// detectImageFormat 打开文件(只读,失败返回""),读取 1024 字节,按 magic 判定格式,未知返回 ""。

TEST_F(UnionImageTest, DetectImageFormat_NonExistentFile_ReturnsEmpty)
{
    EXPECT_EQ(LibUnionImage_NameSpace::detectImageFormat(tmpPath + "/no_such_file.png"),
              QString());
}

TEST_F(UnionImageTest, DetectImageFormat_BmpMagic_ReturnsBMP)
{
    const QString p = writeTempFile(tmpPath, "a.bmp", "BM fake bitmap data padding");
    EXPECT_EQ(LibUnionImage_NameSpace::detectImageFormat(p), QString("BMP"));
}

TEST_F(UnionImageTest, DetectImageFormat_PngMagic_ReturnsPNG)
{
    QByteArray magic;
    magic.append("\x89PNG\r\n\x1a\n");
    const QString p = writeTempFile(tmpPath, "a.png", magic);
    EXPECT_EQ(LibUnionImage_NameSpace::detectImageFormat(p), QString("PNG"));
}

TEST_F(UnionImageTest, DetectImageFormat_JpgMagic_ReturnsJPG)
{
    QByteArray magic;
    magic.append("\xff\xd8\xff\xe0");
    const QString p = writeTempFile(tmpPath, "a.jpg", magic);
    EXPECT_EQ(LibUnionImage_NameSpace::detectImageFormat(p), QString("JPG"));
}

TEST_F(UnionImageTest, DetectImageFormat_SvgContent_ReturnsSVG)
{
    const QString p = writeTempFile(tmpPath, "a.svg",
        "<?xml version=\"1.0\"?><svg xmlns=\"http://www.w3.org/2000/svg\""
        " width=\"10\" height=\"10\"><rect width=\"10\" height=\"10\"/></svg>");
    EXPECT_EQ(LibUnionImage_NameSpace::detectImageFormat(p), QString("SVG"));
}

TEST_F(UnionImageTest, DetectImageFormat_UnknownContent_ReturnsEmpty)
{
    const QString p = writeTempFile(tmpPath, "a.unknown", "HELLO WORLD this is not an image");
    EXPECT_EQ(LibUnionImage_NameSpace::detectImageFormat(p), QString());
}

// ============================ loadStaticImageFromFile ============================
// loadStaticImageFromFile: DeviceReadGuard 非激活->false; 文件大小为0->false "error file!";
// 否则读取图片,成功返回 true 并填充 res。

TEST_F(UnionImageTest, LoadStaticImageFromFile_ValidPng_ReturnsTrue)
{
    QImage res;
    QString err;
    EXPECT_TRUE(LibUnionImage_NameSpace::loadStaticImageFromFile(pngFixture, res, err));
    EXPECT_FALSE(res.isNull());
}

TEST_F(UnionImageTest, LoadStaticImageFromFile_EmptyFile_ReturnsFalse)
{
    const QString p = writeTempFile(tmpPath, "empty.png", "");
    QImage res;
    QString err;
    EXPECT_FALSE(LibUnionImage_NameSpace::loadStaticImageFromFile(p, res, err));
    EXPECT_TRUE(err.contains("error file", Qt::CaseInsensitive));
}

TEST_F(UnionImageTest, LoadStaticImageFromFile_NonExistent_ReturnsFalse)
{
    QImage res;
    QString err;
    EXPECT_FALSE(LibUnionImage_NameSpace::loadStaticImageFromFile(tmpPath + "/nope.png", res, err));
    EXPECT_FALSE(err.isEmpty());
}

// ============================ getPathType ============================
// getPathType 默认 LOCAL;smb-share:server= ->SMB; mtp:host= ->MTP;
// gphoto2:host= ->PTP; isVaultFile ->SAFEBOX; 含 ~/.local/share/Trash ->RECYCLEBIN。

TEST_F(UnionImageTest, GetPathType_LocalPath_ReturnsLOCAL)
{
    EXPECT_EQ(LibUnionImage_NameSpace::getPathType("/tmp/test_local_image.png"),
              imageViewerSpace::PathTypeLOCAL);
}

TEST_F(UnionImageTest, GetPathType_SmbShare_ReturnsSMB)
{
    EXPECT_EQ(LibUnionImage_NameSpace::getPathType("smb-share:server=host,share=data/x.png"),
              imageViewerSpace::PathTypeSMB);
}

TEST_F(UnionImageTest, GetPathType_MtpHost_ReturnsMTP)
{
    EXPECT_EQ(LibUnionImage_NameSpace::getPathType("mtp:host=123/x.png"),
              imageViewerSpace::PathTypeMTP);
}

TEST_F(UnionImageTest, GetPathType_Gphoto2Host_ReturnsPTP)
{
    EXPECT_EQ(LibUnionImage_NameSpace::getPathType("gphoto2:host=Cam/x.png"),
              imageViewerSpace::PathTypePTP);
}

TEST_F(UnionImageTest, GetPathType_TrashPath_ReturnsRECYCLEBIN)
{
    const QString trash = QDir::homePath() + "/.local/share/Trash/files/x.png";
    EXPECT_EQ(LibUnionImage_NameSpace::getPathType(trash),
              imageViewerSpace::PathTypeRECYCLEBIN);
}

// ============================ rotateImageFIle ============================
// rotateImageFIle: angel%90!=0 ->false "unsupported angel"; 否则按格式旋转并另存。
// PNG 在 m_qtrotate 中,可被 QImage 加载并旋转保存;不存在文件 detectImageFormat 返回"" ->false。

TEST_F(UnionImageTest, RotateImageFIle_InvalidAngle_ReturnsFalse)
{
    QString err;
    EXPECT_FALSE(LibUnionImage_NameSpace::rotateImageFIle(45, pngFixture, err));
    EXPECT_TRUE(err.contains("unsupported angel", Qt::CaseInsensitive));
}

TEST_F(UnionImageTest, RotateImageFIle_NonExistentPath_ReturnsFalse)
{
    QString err;
    EXPECT_FALSE(LibUnionImage_NameSpace::rotateImageFIle(90, tmpPath + "/nope.png", err));
}

TEST_F(UnionImageTest, RotateImageFIle_ValidPng_ReturnsTrue)
{
    const QString out = tmpPath + "/rotated.png";
    QString err;
    EXPECT_TRUE(LibUnionImage_NameSpace::rotateImageFIle(90, pngFixture, err, out));
    EXPECT_TRUE(QFile::exists(out));
}

// ============================ rotateImageFIleWithImage ============================
// rotateImageFIleWithImage: angel%90!=0 ->false; img 为空 ->false;
// SVG 分支(目标文件含 "<svg")用 QSvgGenerator 写入,返回 true。

TEST_F(UnionImageTest, RotateImageFIleWithImage_InvalidAngle_ReturnsFalse)
{
    QImage img(10, 10, QImage::Format_ARGB32);
    QString err;
    EXPECT_FALSE(LibUnionImage_NameSpace::rotateImageFIleWithImage(45, img, tmpPath + "/a.svg", err));
    EXPECT_TRUE(err.contains("unsupported angel", Qt::CaseInsensitive));
}

TEST_F(UnionImageTest, RotateImageFIleWithImage_NullImage_ReturnsFalse)
{
    QImage img;  // null
    QString err;
    EXPECT_FALSE(LibUnionImage_NameSpace::rotateImageFIleWithImage(90, img, tmpPath + "/a.svg", err));
}

TEST_F(UnionImageTest, RotateImageFIleWithImage_SvgPath_ReturnsTrue)
{
    // 目标路径需是已存在的 SVG 文件,使 detectImageFormat(path) == "SVG"
    const QString svg = writeTempFile(tmpPath, "r.svg",
        "<?xml version=\"1.0\"?><svg xmlns=\"http://www.w3.org/2000/svg\""
        " width=\"10\" height=\"10\"><rect width=\"10\" height=\"10\"/></svg>");
    QImage img(10, 10, QImage::Format_ARGB32);
    img.fill(Qt::red);
    QString err;
    EXPECT_TRUE(LibUnionImage_NameSpace::rotateImageFIleWithImage(90, img, svg, err));
}

// ============================ getAllDirInDir ============================
// getAllDirInDir: entryInfoList(AllDirs|NoDotAndDotDot), 递归收集所有子目录(不含文件)。

TEST_F(UnionImageTest, GetAllDirInDir_NestedDirs_ReturnsAllSubdirs)
{
    QDir d(tmpPath);
    d.mkpath("a/b/c");
    d.mkpath("a/d");
    QFileInfoList result;
    LibUnionImage_NameSpace::getAllDirInDir(QDir(tmpPath + "/a"), result);

    QStringList names;
    for (const auto &fi : result)
        names << fi.absoluteFilePath();
    EXPECT_GE(result.size(), 3);
    EXPECT_TRUE(names.contains(QDir(tmpPath + "/a/b").absolutePath()));
    EXPECT_TRUE(names.contains(QDir(tmpPath + "/a/b/c").absolutePath()));
    EXPECT_TRUE(names.contains(QDir(tmpPath + "/a/d").absolutePath()));
}

TEST_F(UnionImageTest, GetAllDirInDir_EmptyDir_ReturnsEmpty)
{
    QFileInfoList result;
    LibUnionImage_NameSpace::getAllDirInDir(QDir(tmpPath), result);
    EXPECT_TRUE(result.isEmpty());
}

// ============================ SpliteText ============================
// SpliteText: 文本宽度>nLabelSize 时按宽度切分;bReturn 控制空格是否替换为换行;否则原样返回。

TEST_F(UnionImageTest, SpliteText_ShortText_ReturnsUnchanged)
{
    const QFont font("Sans Serif", 10);
    const QString text = "Hi";
    EXPECT_EQ(Libutils::base::SpliteText(text, font, 1000, false), text);
}

TEST_F(UnionImageTest, SpliteText_LongText_BReturnFalse_ContainsNewline)
{
    const QFont font("Sans Serif", 10);
    const QString text = "AAAA BBBB CCCC DDDD";
    const QString result = Libutils::base::SpliteText(text, font, 20, false);
    EXPECT_TRUE(result.contains("\n"));
}

TEST_F(UnionImageTest, SpliteText_LongText_BReturnTrue_ContainsNewline)
{
    const QFont font("Sans Serif", 10);
    const QString text = "AAAA BBBB CCCC DDDD";
    const QString result = Libutils::base::SpliteText(text, font, 20, true);
    EXPECT_TRUE(result.contains("\n"));
}

TEST_F(UnionImageTest, SpliteText_EmptyText_ReturnsEmpty)
{
    const QFont font("Sans Serif", 10);
    EXPECT_TRUE(Libutils::base::SpliteText("", font, 100, false).isEmpty());
}

// ============================ isSupportClassify ============================
// isSupportClassify: 空路径->false; MIME 为 image/* 或 video/x-mng 且非 gif/tiff ->true,否则 false。

TEST_F(UnionImageTest, IsSupportClassify_PngFile_ReturnsTrue)
{
    EXPECT_TRUE(Libutils::base::isSupportClassify(pngFixture));
}

TEST_F(UnionImageTest, IsSupportClassify_GifFile_ReturnsFalse)
{
    QByteArray gif;
    gif.append("GIF89a");
    const QString p = writeTempFile(tmpPath, "a.gif", gif);
    EXPECT_FALSE(Libutils::base::isSupportClassify(p));
}

TEST_F(UnionImageTest, IsSupportClassify_EmptyPath_ReturnsFalse)
{
    EXPECT_FALSE(Libutils::base::isSupportClassify(QString()));
}

TEST_F(UnionImageTest, IsSupportClassify_TxtFile_ReturnsFalse)
{
    const QString p = writeTempFile(tmpPath, "a.txt", "hello world text file");
    EXPECT_FALSE(Libutils::base::isSupportClassify(p));
}

// ============================ getAllFileInDir ============================
// getAllFileInDir: entryInfoList(Dirs|Files|NoDotAndDotDot), 目录递归,文件收集。

TEST_F(UnionImageTest, GetAllFileInDir_NestedFiles_ReturnsAllFiles)
{
    QDir d(tmpPath);
    d.mkpath("a/b");
    writeTempFile(tmpPath, "a/f1.txt", "1");
    writeTempFile(tmpPath, "a/b/f2.txt", "2");
    writeTempFile(tmpPath, "a/b/f3.txt", "3");
    QFileInfoList result;
    LibUnionImage_NameSpace::getAllFileInDir(QDir(tmpPath + "/a"), result);

    QStringList names;
    for (const auto &fi : result)
        names << fi.fileName();
    EXPECT_EQ(result.size(), 3);
    EXPECT_TRUE(names.contains("f1.txt"));
    EXPECT_TRUE(names.contains("f2.txt"));
    EXPECT_TRUE(names.contains("f3.txt"));
}

TEST_F(UnionImageTest, GetAllFileInDir_EmptyDir_ReturnsEmpty)
{
    QFileInfoList result;
    LibUnionImage_NameSpace::getAllFileInDir(QDir(tmpPath), result);
    EXPECT_TRUE(result.isEmpty());
}
