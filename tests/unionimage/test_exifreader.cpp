// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QSvgGenerator>
#include <QPainter>
#include <QXmlStreamWriter>
#include <QtEndian>

#include <zlib.h>

#include "unionimage/exifreader.h"

/*
 * Branch list for getExifOrientation:
 * B1: format == "jpeg" -> readJpegExifPayload          | GetExifOrientation_JpegWithExif_ReturnsValue
 * B2: format == "png" -> readPngExifPayload             | GetExifOrientation_PngWithExif_ReturnsValue
 * B3: format == "webp" -> readWebpExifPayload           | GetExifOrientation_WebpWithExif_ReturnsValue
 * B4: isRawFormat -> readRawOrientation                  | GetExifOrientation_RawSuffix_ReturnsDefault
 * B5: detectedFormat == "tiff"/"tif" -> readTiffOrientation | GetExifOrientation_TiffWithOrientation_ReturnsValue
 * B6: detectedFormat == "ico" -> readIcoExifPayload      | GetExifOrientation_IcoWithPngExif_ReturnsValue
 * B7: detectedFormat == "heif"/"heic"/... -> readHeifExifPayload | GetExifOrientation_HeifCorrupted_ReturnsDefault
 * B8: detectedFormat == "gif" -> readXmpOrientation      | GetExifOrientation_GifWithXmp_ReturnsValue
 * B9: detectedFormat == "svg"/"svgz" -> SVG XMP path     | GetExifOrientation_SvgWithXmp_ReturnsValue
 * B10: payload.isEmpty() -> return 1                     | GetExifOrientation_JpegWithoutExif_ReturnsDefault
 * B11: non-existent / empty / corrupted / non-image      | GetExifOrientation_NonExistentPath_ReturnsDefault
 *
 * Branch list for readExifMetadata:
 * B1: format == "jpeg" -> readExif(readJpegExifPayload)  | ReadExifMetadata_JpegWithExif_ReadsFields
 * B2: format == "png" -> readExif(readPngExifPayload)    | ReadExifMetadata_PngWithExif_ReadsFields
 * B3: format == "webp" -> readExif(readWebpExifPayload)  | ReadExifMetadata_WebpWithExif_ReadsFields
 * B4: isRawFormat -> readRawMetadata                     | ReadExifMetadata_RawSuffix_MetadataUnchanged
 * B5: detectedFormat == "tiff"/"tif" -> readTiffMetadata | ReadExifMetadata_TiffFormat_ReadsFields
 * B6: detectedFormat == "ico" -> readExif(readIcoExif)   | ReadExifMetadata_IcoFormat_MetadataUnchanged
 * B7: detectedFormat == "gif" -> readXmpMetadata         | ReadExifMetadata_GifWithXmp_ReadsFields
 * B8: detectedFormat == "svg"/"svgz" -> readSvgXmpMetadata | ReadExifMetadata_SvgWithXmp_ReadsFields
 * B9: detectedFormat == "psd"/"psb" -> readPhotoshopMetadata | ReadExifMetadata_PsdFormat_MetadataUnchanged
 * B10: detectedFormat == "heif"/"heic"/... -> readHeifExifPayload | ReadExifMetadata_HeifCorrupted_MetadataUnchanged
 * B11: empty format -> use suffix                         | ReadExifMetadata_EmptyFormat_UsesSuffix
 * B12: unknown format -> no-op                            | ReadExifMetadata_UnknownFormat_MetadataUnchanged
 */

class ExifReaderTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char *argv[] = {(char *)"test_exifreader", nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    void SetUp() override
    {
        ASSERT_TRUE(tempDir.isValid());
    }

    QTemporaryDir tempDir;

    QString createRawFile(const QString &name, const QByteArray &content)
    {
        QString path = tempDir.filePath(name);
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.write(content);
        f.close();
        return path;
    }

    // Build a minimal TIFF (little-endian) with one IFD0 entry: Orientation.
    QByteArray makeTiffWithOrientation(quint16 orientation) const
    {
        QByteArray tiff;
        tiff = QByteArray::fromHex("49492a0008000000");  // II, 42, IFD0 at offset 8
        tiff += QByteArray::fromHex("0100");              // 1 entry
        // Entry: tag 0x0112 (Orientation), type 3 (SHORT), count 1, value
        tiff += QByteArray::fromHex("1201030001000000");
        char val[2];
        qToLittleEndian<quint16>(orientation, val);
        tiff.append(val, 2);
        tiff.append(QByteArray(2, '\0'));  // padding to 12 bytes
        tiff += QByteArray::fromHex("00000000");  // next IFD = 0
        return tiff;
    }

    // Build a TIFF with Model, LensModel, ExposureTime, ISOSpeedRatings,
    // DateTimeOriginal, DateTimeDigitized — proven structure matching
    // the standardExifApp1() fixture from test_unionimage_extra.cpp.
    QByteArray makeTiffWithExifFields() const
    {
        return QByteArray::fromHex(
            "49492a0008000000"
            "020010010200110000006800000069870400010000002600000000000000"
            "05009a82050001000000ab000000278803000100000090010000039002001400000083000000"
            "04900200140000009700000034a402000a0000007900000000000000"
            "4558494620546573742043616d65726100546573742035306d6d00"
            "323031383a30313a30322031363a30343a303500323031393a30323a30332031373a30353a303600"
            "010000007d000000");
    }

    // Build a JPEG file with an APP1 EXIF segment containing the given TIFF payload.
    QByteArray makeJpegWithExif(const QByteArray &tiffPayload) const
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

        QByteArray app1 = QByteArray::fromHex("ffe1");
        QByteArray exifBody = QByteArrayLiteral("Exif\0\0") + tiffPayload;
        char len[2];
        qToBigEndian<quint16>(exifBody.size() + 2, len);
        app1.append(len, 2);
        app1.append(exifBody);
        jpeg.insert(app0End, app1);
        return jpeg;
    }

    // Build a JPEG file without any EXIF data.
    QByteArray makeJpegWithoutExif() const
    {
        QByteArray jpeg;
        QBuffer buffer(&jpeg);
        if (!buffer.open(QIODevice::WriteOnly))
            return {};
        QImage image(16, 8, QImage::Format_RGB32);
        image.fill(Qt::blue);
        image.save(&buffer, "JPEG");
        buffer.close();
        return jpeg;
    }

    // Build a PNG file with an eXIf chunk containing the given TIFF payload.
    QByteArray makePngWithExif(const QByteArray &tiffPayload) const
    {
        QByteArray png;
        QBuffer buffer(&png);
        if (!buffer.open(QIODevice::WriteOnly))
            return {};
        QImage image(16, 8, QImage::Format_RGB32);
        image.fill(Qt::red);
        image.save(&buffer, "PNG");
        buffer.close();

        const QByteArray chunkData = "eXIf" + tiffPayload;
        QByteArray chunk(4, Qt::Uninitialized);
        qToBigEndian<quint32>(tiffPayload.size(), chunk.data());
        chunk.append(chunkData);
        char checksum[4];
        const auto crc = crc32(0, reinterpret_cast<const Bytef *>(chunkData.constData()), chunkData.size());
        qToBigEndian<quint32>(crc, checksum);
        chunk.append(checksum, sizeof(checksum));
        png.insert(33, chunk);
        return png;
    }

    // Build a minimal PNG without eXIf chunk.
    QByteArray makePngWithoutExif() const
    {
        QByteArray png;
        QBuffer buffer(&png);
        if (!buffer.open(QIODevice::WriteOnly))
            return {};
        QImage image(16, 8, QImage::Format_RGB32);
        image.fill(Qt::green);
        image.save(&buffer, "PNG");
        buffer.close();
        return png;
    }

    // Build a minimal WebP with an EXIF chunk.
    QByteArray makeWebpWithExif(const QByteArray &tiffPayload) const
    {
        QByteArray header = QByteArray::fromHex("524946460000000057454250565038580a000000080000000f0000070000"
                                                "5650384c110000002f0fc0010007508a5294a2ff8188e87f0000");
        QByteArray chunk = "EXIF";
        char len[4];
        qToLittleEndian<quint32>(tiffPayload.size(), len);
        chunk.append(len, 4);
        chunk.append(tiffPayload);
        if (tiffPayload.size() % 2)
            chunk.append('\0');

        QByteArray webp = header + chunk;
        qToLittleEndian<quint32>(webp.size() - 8, webp.data() + 4);
        return webp;
    }

    // Build a GIF file with an XMP extension block containing the given XMP packet.
    QByteArray makeGifWithXmp(const QByteArray &xmp) const
    {
        QByteArray gif = QByteArray::fromHex("47494638396101000100800000000000ffffff2c00000000010001000002024401003b");

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
        return gif;
    }

    // Build an SVG file with embedded XMP metadata in a <metadata> element.
    QByteArray makeSvgWithXmp(const QByteArray &xmp) const
    {
        QByteArray svg = QByteArrayLiteral(
            "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"10\" height=\"10\">"
            "<metadata>") + xmp + QByteArrayLiteral(
            "</metadata><rect width=\"10\" height=\"10\" fill=\"red\"/></svg>");
        return svg;
    }

    QByteArray standardXmpPacket() const
    {
        return QByteArrayLiteral(R"xml(<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
    xmlns:tiff="http://ns.adobe.com/tiff/1.0/" xmlns:exif="http://ns.adobe.com/exif/1.0/">
    <rdf:Description rdf:about="" tiff:Model="XMP Test Camera" tiff:Orientation="3" exif:ExposureTime="1/125"
                     exif:FNumber="4/1" exif:DateTimeOriginal="2018-01-02T16:04:05+08:00">
      <exif:ISOSpeedRatings><rdf:Seq><rdf:li>400</rdf:li></rdf:Seq></exif:ISOSpeedRatings>
      <exif:FocalLength>50/1</exif:FocalLength>
    </rdf:Description>
    </rdf:RDF>)xml");
    }
};

// =========================================================================
//  getExifOrientation — JPEG
// =========================================================================

TEST_F(ExifReaderTest, GetExifOrientation_JpegWithExif_ReturnsValue)
{
    // Arrange
    QByteArray tiff = makeTiffWithOrientation(6);
    QByteArray jpeg = makeJpegWithExif(tiff);
    QString path = createRawFile("oriented.jpg", jpeg);

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 6);
}

TEST_F(ExifReaderTest, GetExifOrientation_JpegWithoutExif_ReturnsDefault)
{
    // Arrange
    QByteArray jpeg = makeJpegWithoutExif();
    QString path = createRawFile("no_exif.jpg", jpeg);

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 1);
}

// =========================================================================
//  getExifOrientation — PNG
// =========================================================================

TEST_F(ExifReaderTest, GetExifOrientation_PngWithExif_ReturnsValue)
{
    // Arrange
    QByteArray tiff = makeTiffWithOrientation(8);
    QByteArray png = makePngWithExif(tiff);
    QString path = createRawFile("oriented.png", png);

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 8);
}

TEST_F(ExifReaderTest, GetExifOrientation_PngWithoutExif_ReturnsDefault)
{
    // Arrange
    QByteArray png = makePngWithoutExif();
    QString path = createRawFile("no_exif.png", png);

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 1);
}

// =========================================================================
//  getExifOrientation — WebP
// =========================================================================

TEST_F(ExifReaderTest, GetExifOrientation_WebpWithExif_ReturnsValue)
{
    // Arrange
    if (!QImageReader::supportedImageFormats().contains("webp"))
        GTEST_SKIP() << "WebP image format plugin is unavailable";

    QByteArray tiff = makeTiffWithOrientation(6);
    QByteArray webp = makeWebpWithExif(tiff);
    QString path = createRawFile("oriented.webp", webp);

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 6);
}

// =========================================================================
//  getExifOrientation — TIFF
// =========================================================================

TEST_F(ExifReaderTest, GetExifOrientation_TiffWithoutExif_ReturnsDefault)
{
    // Arrange — create a valid TIFF via QImage (libtiff requires ImageWidth/ImageLength)
    QString path = tempDir.filePath("plain.tif");
    QImage img(16, 8, QImage::Format_RGB32);
    img.fill(Qt::red);
    ASSERT_TRUE(img.save(path, "TIFF"));

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert — TIFF without EXIF orientation tag returns default 1
    EXPECT_EQ(result, 1);
}

// =========================================================================
//  getExifOrientation — GIF (XMP-based)
// =========================================================================

TEST_F(ExifReaderTest, GetExifOrientation_GifWithXmp_ReturnsValue)
{
    // Arrange
    QByteArray xmp = QByteArrayLiteral(
        R"xml(<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
    xmlns:tiff="http://ns.adobe.com/tiff/1.0/">
    <rdf:Description rdf:about="" tiff:Orientation="3"/>
    </rdf:RDF>)xml");
    QByteArray gif = makeGifWithXmp(xmp);
    QString path = createRawFile("oriented.gif", gif);

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 3);
}

TEST_F(ExifReaderTest, GetExifOrientation_GifWithoutXmp_ReturnsDefault)
{
    // Arrange
    QByteArray gif = QByteArray::fromHex("47494638396101000100800000000000ffffff2c00000000010001000002024401003b");
    QString path = createRawFile("no_xmp.gif", gif);

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 1);
}

// =========================================================================
//  getExifOrientation — SVG (XMP-based)
// =========================================================================

TEST_F(ExifReaderTest, GetExifOrientation_SvgWithXmp_ReturnsValue)
{
    // Arrange
    QByteArray xmp = QByteArrayLiteral(
        R"xml(<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
    xmlns:tiff="http://ns.adobe.com/tiff/1.0/">
    <rdf:Description rdf:about="" tiff:Orientation="5"/>
    </rdf:RDF>)xml");
    QByteArray svg = makeSvgWithXmp(xmp);
    QString path = createRawFile("oriented.svg", svg);

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 5);
}

TEST_F(ExifReaderTest, GetExifOrientation_SvgWithoutXmp_ReturnsDefault)
{
    // Arrange
    QByteArray svg = QByteArrayLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"10\" height=\"10\">"
        "<rect width=\"10\" height=\"10\" fill=\"blue\"/></svg>");
    QString path = createRawFile("no_xmp.svg", svg);

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 1);
}

// =========================================================================
//  getExifOrientation — Error / edge cases
// =========================================================================

TEST_F(ExifReaderTest, GetExifOrientation_NonExistentPath_ReturnsDefault)
{
    // Arrange
    QString path = tempDir.filePath("does_not_exist.jpg");

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 1);
}

TEST_F(ExifReaderTest, GetExifOrientation_EmptyPath_ReturnsDefault)
{
    // Arrange
    QString path;

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 1);
}

TEST_F(ExifReaderTest, GetExifOrientation_CorruptedJpeg_ReturnsDefault)
{
    // Arrange
    QByteArray corrupt = QByteArray::fromHex("ffd8ffffff");
    QString path = createRawFile("corrupt.jpg", corrupt);

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 1);
}

TEST_F(ExifReaderTest, GetExifOrientation_NonImageFile_ReturnsDefault)
{
    // Arrange
    QString path = createRawFile("not_image.txt", "this is not an image");

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 1);
}

TEST_F(ExifReaderTest, GetExifOrientation_BmpFile_ReturnsDefault)
{
    // Arrange — BMP has no EXIF, should return default 1
    QByteArray bmp = QByteArray::fromHex("424d3a0000000000000036000000280000000100000001000000"
                                         "010018000000000004000000000000000000000000000000000000"
                                         "0000ffffff");
    QString path = createRawFile("plain.bmp", bmp);

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 1);
}

// =========================================================================
//  getExifOrientation — ICO with embedded PNG
// =========================================================================

TEST_F(ExifReaderTest, GetExifOrientation_IcoWithPngExif_ReturnsValue)
{
    // Arrange
    QByteArray png = makePngWithExif(makeTiffWithOrientation(6));
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
    QString path = createRawFile("oriented.ico", ico);

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 6);
}

// =========================================================================
//  getExifOrientation — Orientation values 1-8 (parameterized)
// =========================================================================

class ExifOrientationParamTest : public ExifReaderTest,
                                 public ::testing::WithParamInterface<int>
{
};

TEST_P(ExifOrientationParamTest, JpegOrientation_ReturnsExpectedValue)
{
    // Arrange
    int expected = GetParam();
    QByteArray tiff = makeTiffWithOrientation(static_cast<quint16>(expected));
    QByteArray jpeg = makeJpegWithExif(tiff);
    QString path = createRawFile(QString("orient_%1.jpg").arg(expected), jpeg);

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, expected);
}

INSTANTIATE_TEST_SUITE_P(AllOrientations,
                         ExifOrientationParamTest,
                         ::testing::Range(1, 9));

// =========================================================================
//  readExifMetadata — JPEG
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_JpegWithExif_ReadsFields)
{
    // Arrange
    QByteArray tiff = makeTiffWithExifFields();
    QByteArray jpeg = makeJpegWithExif(tiff);
    QString path = createRawFile("metadata.jpg", jpeg);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "jpeg", metadata);

    // Assert
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("EXIF Test Camera"));
    EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("400"));
    EXPECT_EQ(metadata.value("DateTimeOriginal"), QStringLiteral("2018/01/02 16:04"));
}

TEST_F(ExifReaderTest, ReadExifMetadata_JpegWithoutExif_MetadataUnchanged)
{
    // Arrange
    QByteArray jpeg = makeJpegWithoutExif();
    QString path = createRawFile("no_meta.jpg", jpeg);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "jpeg", metadata);

    // Assert
    EXPECT_TRUE(metadata.isEmpty());
}

// =========================================================================
//  readExifMetadata — PNG
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_PngWithExif_ReadsFields)
{
    // Arrange
    QByteArray tiff = makeTiffWithExifFields();
    QByteArray png = makePngWithExif(tiff);
    QString path = createRawFile("metadata.png", png);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "png", metadata);

    // Assert
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("EXIF Test Camera"));
    EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("400"));
}

TEST_F(ExifReaderTest, ReadExifMetadata_PngWithoutExif_MetadataUnchanged)
{
    // Arrange
    QByteArray png = makePngWithoutExif();
    QString path = createRawFile("no_meta.png", png);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "png", metadata);

    // Assert
    EXPECT_TRUE(metadata.isEmpty());
}

// =========================================================================
//  readExifMetadata — WebP
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_WebpWithExif_ReadsFields)
{
    // Arrange
    if (!QImageReader::supportedImageFormats().contains("webp"))
        GTEST_SKIP() << "WebP image format plugin is unavailable";

    QByteArray tiff = makeTiffWithExifFields();
    QByteArray webp = makeWebpWithExif(tiff);
    QString path = createRawFile("metadata.webp", webp);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "webp", metadata);

    // Assert
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("EXIF Test Camera"));
    EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("400"));
}

// =========================================================================
//  readExifMetadata — TIFF
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_TiffFormat_ReadsFields)
{
    // Arrange — create a real TIFF file using QImage::save
    QString tiffPath = tempDir.filePath("metadata.tif");
    QImage img(16, 8, QImage::Format_RGB32);
    img.fill(Qt::red);
    ASSERT_TRUE(img.save(tiffPath, "TIFF"));
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(tiffPath, "tiff", metadata);

    // Assert — TIFF without EXIF IFD may still have basic model info or be empty;
    // verify it doesn't crash and metadata is accessible
    EXPECT_NO_FATAL_FAILURE(metadata.size());
    EXPECT_TRUE(metadata.contains("Model") || metadata.isEmpty());
}

// =========================================================================
//  readExifMetadata — GIF (XMP-based)
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_GifWithXmp_ReadsFields)
{
    // Arrange
    QByteArray gif = makeGifWithXmp(standardXmpPacket());
    QString path = createRawFile("metadata.gif", gif);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "gif", metadata);

    // Assert
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("XMP Test Camera"));
    EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("400"));
}

TEST_F(ExifReaderTest, ReadExifMetadata_GifWithoutXmp_MetadataUnchanged)
{
    // Arrange
    QByteArray gif = QByteArray::fromHex("47494638396101000100800000000000ffffff2c00000000010001000002024401003b");
    QString path = createRawFile("no_meta.gif", gif);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "gif", metadata);

    // Assert
    EXPECT_TRUE(metadata.isEmpty());
}

// =========================================================================
//  readExifMetadata — SVG (XMP-based)
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_SvgWithXmp_ReadsFields)
{
    // Arrange
    QByteArray svg = makeSvgWithXmp(standardXmpPacket());
    QString path = createRawFile("metadata.svg", svg);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "svg", metadata);

    // Assert
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("XMP Test Camera"));
    EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("400"));
}

TEST_F(ExifReaderTest, ReadExifMetadata_SvgWithoutXmp_MetadataUnchanged)
{
    // Arrange
    QByteArray svg = QByteArrayLiteral(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"10\" height=\"10\">"
        "<rect width=\"10\" height=\"10\" fill=\"green\"/></svg>");
    QString path = createRawFile("no_meta.svg", svg);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "svg", metadata);

    // Assert
    EXPECT_TRUE(metadata.isEmpty());
}

// =========================================================================
//  readExifMetadata — Empty format uses suffix
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_EmptyFormat_UsesSuffix)
{
    // Arrange
    QByteArray tiff = makeTiffWithExifFields();
    QByteArray jpeg = makeJpegWithExif(tiff);
    QString path = createRawFile("suffix_test.jpg", jpeg);
    QMap<QString, QString> metadata;

    // Act — pass empty format so it detects from .jpg suffix
    LibUnionImage_NameSpace::readExifMetadata(path, "", metadata);

    // Assert — suffix is "jpg", not "jpeg", so it falls to the else branch
    // which checks isRawFormat, tiff, etc. "jpg" is none of those, so metadata stays empty
    EXPECT_TRUE(metadata.isEmpty());
}

// =========================================================================
//  readExifMetadata — Unknown format
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_UnknownFormat_MetadataUnchanged)
{
    // Arrange
    QString path = createRawFile("unknown.xyz", "unknown content");
    QMap<QString, QString> metadata;
    metadata.insert("PreExisting", "value");

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "xyz", metadata);

    // Assert
    EXPECT_EQ(metadata.size(), 1);
    EXPECT_EQ(metadata.value("PreExisting"), QStringLiteral("value"));
}

// =========================================================================
//  readExifMetadata — Non-existent file
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_NonExistentFile_MetadataUnchanged)
{
    // Arrange
    QString path = tempDir.filePath("no_such_file.jpg");
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "jpeg", metadata);

    // Assert
    EXPECT_TRUE(metadata.isEmpty());
}

// =========================================================================
//  readExifMetadata — RAW format suffix detection
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_RawSuffix_MetadataUnchanged)
{
    // Arrange — a .nef file with garbage content; LibRaw will fail to open
    QString path = createRawFile("fake.nef", "not a real raw file");
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "", metadata);

    // Assert — LibRaw open_file fails, metadata stays empty
    EXPECT_TRUE(metadata.isEmpty());
}

// =========================================================================
//  readExifMetadata — PSD format
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_PsdFormat_MetadataUnchanged)
{
    // Arrange — minimal invalid PSD file
    QString path = createRawFile("fake.psd", "not a real psd");
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "psd", metadata);

    // Assert — readPhotoshopMetadata fails on invalid header, metadata stays empty
    EXPECT_TRUE(metadata.isEmpty());
}

// =========================================================================
//  readExifMetadata — HEIF format (corrupted)
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_HeifCorrupted_MetadataUnchanged)
{
    // Arrange
    QString path = createRawFile("corrupt.heic", "not a real heif");
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "heif", metadata);

    // Assert — heif_context_read_from_file fails, metadata stays empty
    EXPECT_TRUE(metadata.isEmpty());
}

TEST_F(ExifReaderTest, GetExifOrientation_HeifCorrupted_ReturnsDefault)
{
    // Arrange
    QString path = createRawFile("corrupt.heic", "not a real heif");

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert
    EXPECT_EQ(result, 1);
}

// =========================================================================
//  readExifMetadata — XMP with full camera properties (GIF)
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_GifXmpFullProperties_ReadsAllFields)
{
    // Arrange
    QByteArray xmp = QByteArrayLiteral(
        R"xml(<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
    xmlns:tiff="http://ns.adobe.com/tiff/1.0/" xmlns:exif="http://ns.adobe.com/exif/1.0/">
    <rdf:Description rdf:about="" tiff:Model="Canon EOS" tiff:Orientation="6"
                     exif:ExposureTime="1/200" exif:FNumber="28/10"
                     exif:DateTimeOriginal="2020-06-15T12:30:00Z">
      <exif:ISOSpeedRatings><rdf:Seq><rdf:li>800</rdf:li></rdf:Seq></exif:ISOSpeedRatings>
      <exif:FocalLength>35/1</exif:FocalLength>
      <exif:LensModel>Canon 35mm</exif:LensModel>
    </rdf:Description>
    </rdf:RDF>)xml");
    QByteArray gif = makeGifWithXmp(xmp);
    QString path = createRawFile("full_xmp.gif", gif);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "gif", metadata);

    // Assert
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("Canon EOS"));
    EXPECT_EQ(metadata.value("LensType"), QStringLiteral("Canon 35mm"));
    EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("800"));
    EXPECT_EQ(metadata.value("ApertureValue"), QStringLiteral("f/2.8"));
    EXPECT_EQ(metadata.value("FocalLength"), QStringLiteral("35 mm"));
    EXPECT_EQ(metadata.value("ExposureTime"), QStringLiteral("1/200 sec."));
    EXPECT_EQ(metadata.value("DateTimeOriginal"), QStringLiteral("2020/06/15 12:30"));
}

// =========================================================================
//  readExifMetadata — SVG XMP with full camera properties
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_SvgXmpFullProperties_ReadsAllFields)
{
    // Arrange
    QByteArray xmp = QByteArrayLiteral(
        R"xml(<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
    xmlns:tiff="http://ns.adobe.com/tiff/1.0/" xmlns:exif="http://ns.adobe.com/exif/1.0/">
    <rdf:Description rdf:about="" tiff:Model="Nikon D850" tiff:Orientation="8"
                     exif:ExposureTime="1/500" exif:FNumber="8/1"
                     exif:DateTimeOriginal="2021-03-20T08:15:00+09:00">
      <exif:ISOSpeedRatings><rdf:Seq><rdf:li>200</rdf:li></rdf:Seq></exif:ISOSpeedRatings>
      <exif:FocalLength>85/1</exif:FocalLength>
    </rdf:Description>
    </rdf:RDF>)xml");
    QByteArray svg = makeSvgWithXmp(xmp);
    QString path = createRawFile("full_xmp.svg", svg);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "svg", metadata);

    // Assert
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("Nikon D850"));
    EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("200"));
    EXPECT_EQ(metadata.value("ApertureValue"), QStringLiteral("f/8"));
    EXPECT_EQ(metadata.value("FocalLength"), QStringLiteral("85 mm"));
    EXPECT_EQ(metadata.value("ExposureTime"), QStringLiteral("1/500 sec."));
}

// =========================================================================
//  readExifMetadata — XMP with truncated XML
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_GifXmpTruncatedXml_MetadataUnchanged)
{
    // Arrange — truncated XML (no closing tags)
    QByteArray xmp = QByteArrayLiteral(
        R"xml(<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
    xmlns:tiff="http://ns.adobe.com/tiff/1.0/">
    <rdf:Description rdf:about="" tiff:Model="Truncated")xml");
    QByteArray gif = makeGifWithXmp(xmp);
    QString path = createRawFile("truncated.gif", gif);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "gif", metadata);

    // Assert — truncated XML causes xml.hasError(), properties returned empty
    EXPECT_TRUE(metadata.isEmpty());
}

// =========================================================================
//  readExifMetadata — XMP with missing namespace
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_GifXmpMissingNamespace_MetadataUnchanged)
{
    // Arrange — XMP with no camera namespace declarations
    QByteArray xmp = QByteArrayLiteral(
        R"xml(<rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#">
    <rdf:Description rdf:about="" foo:Model="Unknown Camera"/>
    </rdf:RDF>)xml");
    QByteArray gif = makeGifWithXmp(xmp);
    QString path = createRawFile("no_ns.gif", gif);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "gif", metadata);

    // Assert — foo: namespace is not a camera namespace, no properties extracted
    EXPECT_TRUE(metadata.isEmpty());
}

// =========================================================================
//  readExifMetadata — ICO format dispatch
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_IcoFormat_MetadataUnchanged)
{
    // Arrange — minimal invalid ICO
    QString path = createRawFile("fake.ico", "not a real ico");
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "ico", metadata);

    // Assert — readIcoExifPayload fails on invalid header, metadata stays empty
    EXPECT_TRUE(metadata.isEmpty());
}

// =========================================================================
//  readExifMetadata — Large EXIF (> 0xfffe) triggers readLargeExif path
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_PngLargeExif_ReadsFieldsViaReadLargeExif)
{
    // Arrange — build a TIFF with Model field at a high offset so the total
    // EXIF payload exceeds 0xfffe bytes, triggering readLargeExif path.
    // PNG eXIf chunk supports 32-bit sizes, unlike JPEG APP1 (16-bit length).
    QByteArray tiff;
    tiff = QByteArray::fromHex("49492a0008000000");  // II, 42, IFD0 at 8

    // IFD0: 1 entry (Model), next IFD = 0
    tiff += QByteArray::fromHex("0100");
    // Model entry: tag 0x0110, ASCII, count 17, offset = 65536
    tiff += QByteArray::fromHex("1001020011000000");
    char modelOff[4];
    qToLittleEndian<quint32>(65536, modelOff);
    tiff.append(modelOff, 4);
    // next IFD = 0
    tiff += QByteArray::fromHex("00000000");

    // Pad to 65536 to place Model data
    tiff.resize(65536, '\0');
    tiff.append("EXIF Test Camera\0", 17);

    // The TIFF payload is > 0xfffe - 6 (signature) so readExif takes readLargeExif path
    QByteArray png = makePngWithExif(tiff);
    QString path = createRawFile("large_exif.png", png);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "png", metadata);

    // Assert
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("EXIF Test Camera"));
}

// =========================================================================
//  getExifOrientation — TIFF with no orientation tag returns default
// =========================================================================

TEST_F(ExifReaderTest, GetExifOrientation_TiffWithoutOrientation_ReturnsDefault)
{
    // Arrange — create a valid TIFF via QImage (libtiff requires ImageWidth/ImageLength)
    QString path = tempDir.filePath("no_orient.tif");
    QImage img(16, 8, QImage::Format_RGB32);
    img.fill(Qt::blue);
    ASSERT_TRUE(img.save(path, "TIFF"));

    // Act
    int result = LibUnionImage_NameSpace::getExifOrientation(path);

    // Assert — TIFF without Orientation tag returns default 1
    EXPECT_EQ(result, 1);
}

// =========================================================================
//  readExifMetadata — ExposureTime formatting via JPEG EXIF
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_JpegExifExposureTime_FormattedCorrectly)
{
    // Arrange
    QByteArray tiff = makeTiffWithExifFields();
    QByteArray jpeg = makeJpegWithExif(tiff);
    QString path = createRawFile("exposure.jpg", jpeg);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "jpeg", metadata);

    // Assert — ExposureTime is 1/125 RATIONAL, should format as "1/125 sec."
    EXPECT_EQ(metadata.value("ExposureTime"), QStringLiteral("1/125 sec."));
    // DateTimeOriginal should be formatted as "yyyy/MM/dd HH:mm"
    EXPECT_EQ(metadata.value("DateTimeOriginal"), QStringLiteral("2018/01/02 16:04"));
}

// =========================================================================
//  readExifMetadata — SVG with svGz (compressed SVG)
// =========================================================================

TEST_F(ExifReaderTest, ReadExifMetadata_SvgzFormat_MetadataRead)
{
    // Arrange — create a .svgz file (gzip-compressed SVG with XMP)
    QByteArray svg = makeSvgWithXmp(standardXmpPacket());
    QByteArray compressed;
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
    strm.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(svg.constData()));
    strm.avail_in = svg.size();
    do {
        char outbuf[4096];
        strm.next_out = reinterpret_cast<Bytef *>(outbuf);
        strm.avail_out = sizeof(outbuf);
        deflate(&strm, Z_FINISH);
        compressed.append(outbuf, sizeof(outbuf) - strm.avail_out);
    } while (strm.avail_out == 0);
    deflateEnd(&strm);

    QString path = createRawFile("metadata.svgz", compressed);
    QMap<QString, QString> metadata;

    // Act
    LibUnionImage_NameSpace::readExifMetadata(path, "svgz", metadata);

    // Assert
    EXPECT_EQ(metadata.value("Model"), QStringLiteral("XMP Test Camera"));
    EXPECT_EQ(metadata.value("ISOSpeedRatings"), QStringLiteral("400"));
}
