// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "unionimage.h"
#include "../../src/album/albumgloabl.h"

extern "C" {
#include "3rdparty/tiff-tools/converttiff.h"
}
//#include "giflib/cmanagerattributeservice.h"

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QDate>
#include <QTime>
#include <QtMath>
#include <QMovie>
#include <QMatrix>
#include <QPainter>
#include <QSvgGenerator>
#include <QImageReader>
#include <QUuid>
#include <QDir>
#include <QDebug>
#include <QtSvg/QSvgRenderer>

#include <cstring>

#define SAVE_QUAITY_VALUE 100

const QString DATETIME_FORMAT_NORMAL = "yyyy.MM.dd";
const QString DATETIME_FORMAT_EXIF = "yyyy:MM:dd HH:mm";

namespace UnionImage_NameSpace {

//    enum SupportFormat {
//        UNKNOWN = -1,
//        BMP     =  0,
//        ICO     =  1,
//        JPG     =  2,
//        JPE     =  2,
//        JPS     =  2,
//        JPEG    =  2,
//        JNG     =  3,
//        KOALA   =  4,
//        KOA     =  KOALA,
//        LBM     =  5,
//        IFF     =  LBM,
//        MNG     =  6,
//        PBM     =  7,
//        PBMRAW  =  8,
//        PCD     =  9,
//        PCX     =  10,
//        PGM     =  11,
//        PGMRAW  =  12,
//        PNG     =  13,
//        PPM     =  14,
//        PPMRAW  =  15,
//        RAS     =  16,
//        TGA     =  17,
//        TARGA   =  17,
//        TIFF    =  18,
//        WBMP    =  19,
//        PSD     =  20,
//        CUT     =  21,
//        XBM     =  22,
//        XPM     =  23,
//        DDS     =  24,
//        GIF     =  25,
//        FHDR    =  26,
//        FAXG3   =  27,
//        SGI     =  28,
//        EXR     =  29,
//        J2K     =  30,
//        J2C     =  30,
//        JPC     =  30,
//        JP2     =  31,
//        PFM     =  32,
//        PCT     =  33,
//        PIC     =  33,
//        PICT    =  33,
//        RAW     =  34,
//        WEBP    =  35,
//        JXR     =  36
//    };


class UnionImage_Private
{



public:
    UnionImage_Private()
    {
        m_qtSupported << "BMP"
                      << "JPG"
                      << "JPEG"
                      << "JPS"
                      << "JPE"
                      << "PNG"
                      << "PBM"
                      << "PGM"
                      << "PPM"
                      << "PNM"
                      << "WBMP"
                      << "WEBP"
                      << "SVG"
                      << "ICNS"
                      << "GIF"
                      << "MNG"
                      << "TIF"
                      << "TIFF"
                      << "BMP"
                      << "XPM"
                      << "DNG"
                      << "RAF"
                      << "CR2"
                      << "MEF"
                      << "ORF"
                      << "ICO"
                      << "RAW"
                      << "MRW"
                      << "NEF"
                      << "JP2"
                      << "HEIF"
                      << "HEIC"
                      << "HEJ2"
                      << "AVIF"
                      << "TGA"
                      << "PSD"
                      << "PXM"
                      << "PIC"
                      << "PEF"
                      << "XBM"
                      << "ARW"
                      << "HDR"
                      << "J2K"
                      << "ICNS"
                      << "AVI"
                      << "VIFF"
                      << "IFF"
                      << "JP2"
                      << "WMF"
                      << "CRW"
                      << "X3F"
                      << "EPS"
                      << "SR2"
                      << "AVIFS";
        m_canSave << "BMP"
            << "JPG"
            << "JPEG"
            << "PNG"
            << "PGM"
            << "PPM"
            << "XPM"
            << "ICO"
            << "ICNS";
        m_qtrotate << "BMP"
            << "JPG"
            << "JPEG"
            << "PNG"
            << "PGM"
            << "PPM"
            << "XPM"
            << "ICO"
            << "ICNS";
    }
    ~UnionImage_Private()
    {

    }
    QStringList m_qtSupported;
    QHash<QString, int> m_movie_formats;
    QStringList m_canSave;
    QStringList m_qtrotate;
};

static UnionImage_Private union_image_private;

/**
 * @brief noneQImage
 * @return QImage
 * 返回空图片
 */
UNIONIMAGESHARED_EXPORT QImage noneQImage()
{
    static QImage none(0, 0, QImage::Format_Invalid);
    return none;
}

UNIONIMAGESHARED_EXPORT const QStringList unionImageSupportFormat()
{
    static QStringList res;
    if (res.empty()) {
        QStringList list = union_image_private.m_qtSupported;
        res.append(list);
    }
    return res;
}

UNIONIMAGESHARED_EXPORT const QStringList supportStaticFormat()
{
    return (union_image_private.m_qtSupported);
}

UNIONIMAGESHARED_EXPORT const QStringList supportMovieFormat()
{
    return (union_image_private.m_movie_formats.keys());
}

/**
 * @brief size2Human
 * @param bytes
 * @author LMH
 * @return QString
 * 照片尺寸转化为QString格式
 */
UNIONIMAGESHARED_EXPORT QString size2Human(const qlonglong bytes)
{
    qlonglong kb = 1024;
    if (bytes < kb) {
        return QString::number(bytes) + " B";
    } else if (bytes < kb * kb) {
        QString vs = QString::number(static_cast<double>(bytes) / kb, 'f', 1);
        if (qCeil(vs.toDouble()) == qFloor(vs.toDouble())) {
            return QString::number(static_cast<int>(vs.toDouble())) + " KB";
        } else {
            return vs + " KB";
        }
    } else if (bytes < kb * kb * kb) {
        QString vs = QString::number(static_cast<double>(bytes) / kb / kb, 'f', 1);
        if (qCeil(vs.toDouble()) == qFloor(vs.toDouble())) {
            return QString::number(static_cast<int>(vs.toDouble())) + " MB";
        } else {
            return vs + " MB";
        }
    } else {
        //修改了当超过一个G的图片,应该用G返回,不应该返回一堆数字,bug68094
        QString vs = QString::number(static_cast<double>(bytes) / kb / kb / kb, 'f', 1);
        if (qCeil(vs.toDouble()) == qFloor(vs.toDouble())) {
            return QString::number(static_cast<int>(vs.toDouble())) + " GB";
        } else {
            return vs + " GB";
        }
    }
}

/**
 * @brief getFileFormat
 * @param path
 * @author LMH
 * @return QString
 * 文件路径获取文件后缀名
 */
UNIONIMAGESHARED_EXPORT const QString getFileFormat(const QString &path)
{
    QFileInfo fi(path);
    QString suffix = fi.suffix();
    return suffix;
}

/**
 * @brief string2DateTime
 * @param time
 * @author LMH
 * @return QDateTime
 * string转换时间
 */
UNIONIMAGESHARED_EXPORT QDateTime string2DateTime(const QString &time)
{
    QDateTime dt = QDateTime::fromString(time, DATETIME_FORMAT_EXIF);
    if (! dt.isValid()) {
        dt = QDateTime::fromString(time, DATETIME_FORMAT_NORMAL);
    }
    return dt;
}
UNIONIMAGESHARED_EXPORT bool canSave(const QString &path)
{
    QFileInfo info(path);
    if (!info.exists() || !QFile::permissions(path).testFlag(QFile::WriteOwner) || !QFile::permissions(path).testFlag(QFile::ReadOwner)) {
        return false;
    }
    QImageReader r(path);
    if (r.imageCount() > 1) {
        return false;
    }
    if (union_image_private.m_canSave.contains(info.suffix().toUpper()))
        return true;
    return false;
}

UNIONIMAGESHARED_EXPORT QString unionImageVersion()
{
    QString ver;
    ver.append("UnionImage Version:");
    ver.append("0.0.4");
    ver.append("\n");
    return ver;
}

UNIONIMAGESHARED_EXPORT bool creatNewImage(QImage &res, int width, int height, int depth, SupportType type)
{
    Q_UNUSED(type);
    if (depth == 8) {
        res = QImage(width, height, QImage::Format_RGB888);
    } else if (depth == 16) {
        res = QImage(width, height, QImage::Format_RGB16);
    } else {
        res = QImage(width, height, QImage::Format_RGB32);
    }
    return true;
}

QString PrivateDetectImageFormat(const QString &filepath);
UNIONIMAGESHARED_EXPORT bool loadStaticImageFromFile(const QString &path, QImage &res, QString &errorMsg, const QString &format_bar)
{
    QFileInfo file_info(path);
    if (file_info.size() == 0) {
        res = QImage();
        errorMsg = "error file!";
        return false;
    }
    QString file_suffix_upper = file_info.suffix().toUpper();
    QString file_suffix_lower = file_suffix_upper.toLower();
    if (union_image_private.m_qtSupported.contains(file_suffix_upper)) {
        QImageReader reader;
        QImage res_qt;
        reader.setFileName(path);
        if (format_bar.isEmpty()) {
            reader.setFormat(file_suffix_lower.toLatin1());
        } else {
            reader.setFormat(format_bar.toLatin1());
        }
        reader.setAutoTransform(true);
        if (reader.imageCount() > 0 || file_suffix_upper != "ICNS") {
            res_qt = reader.read();
            if (res_qt.isNull()) {
                //try old loading method
                QString format = PrivateDetectImageFormat(path);
                QImageReader readerF(path, format.toLatin1());
                QImage try_res;
                readerF.setAutoTransform(true);
                if (readerF.canRead() && readerF.imageCount() > 0) {
                    try_res = readerF.read();
                } else {
                    errorMsg = "can't read image:" + readerF.errorString() + format;
                    try_res = QImage();
                }

                // 单独处理TIF格式情况
                if (try_res.isNull() && (file_suffix_upper == "TIF" || file_suffix_upper == "TIFF")) {
                    // 读取失败，tif需要单独处理，尝试通过转换函数处理
                    QFileInfo imageFile(path);
                    QString cachDir = albumGlobal::CACHE_PATH + QDir::separator() + "TIF";
                    QDir dir(cachDir);
                    if (!dir.exists())
                        dir.mkpath(cachDir);
                    QString cacheFile = cachDir + QDir::separator() + imageFile.fileName();
                    // 转换图像编码格式
                    int nRet = convertOldStyleImage(imageFile.absoluteFilePath().toUtf8().data(), cacheFile.toUtf8().data());
                    // 转换成功标识
                    static const int s_nFlagSucc = 0;
                    if (s_nFlagSucc == nRet) {
                        if (QFile::exists(cacheFile)) {
                            // 成功获取图片内容后，删除缓存的tiff转换文件
                            try_res = QImage(cacheFile);
                            QFile::remove(cacheFile);
                        }
                    }
                }

                if (try_res.isNull()) {
                    errorMsg = "load image by qt failed, use format:" + reader.format() + " ,path:" + path;
                    res = QImage();
                    return false;
                }
                errorMsg = "use old method to load QImage";
                res = try_res;
                return true;
            }
        }
        errorMsg = "use QImage";
        res = res_qt;
        return true;
    }
    return false;
}

UNIONIMAGESHARED_EXPORT QString detectImageFormat(const QString &path)
{
    QFileInfo file_info(path);
    QString file_suffix_upper = file_info.suffix().toUpper();
    QString res = file_suffix_upper;
    if (res.isEmpty()) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            return "";
        }

        //    const QByteArray data = file.read(1024);
        const QByteArray data = file.read(64);

        // Check bmp file.
        if (data.startsWith("BM")) {
            return "BMP";
        }

        // Check dds file.
        if (data.startsWith("DDS")) {
            return "DDS";
        }

        // Check gif file.
        if (data.startsWith("GIF8")) {
            return "GIF";
        }

        // Check Max OS icons file.
        if (data.startsWith("icns")) {
            return "ICNS";
        }

        // Check jpeg file.
        if (data.startsWith("\xff\xd8")) {
            return "JPG";
        }

        // Check mng file.
        if (data.startsWith("\x8a\x4d\x4e\x47\x0d\x0a\x1a\x0a")) {
            return "MNG";
        }

        // Check net pbm file (BitMap).
        if (data.startsWith("P1") || data.startsWith("P4")) {
            return "PBM";
        }

        // Check pgm file (GrayMap).
        if (data.startsWith("P2") || data.startsWith("P5")) {
            return "PGM";
        }

        // Check ppm file (PixMap).
        if (data.startsWith("P3") || data.startsWith("P6")) {
            return "PPM";
        }

        // Check png file.
        if (data.startsWith("\x89PNG\x0d\x0a\x1a\x0a")) {
            return "PNG";
        }

        // Check svg file.
        if (data.indexOf("<svg") > -1) {
            return "SVG";
        }

        // TODO(xushaohua): tga file is not supported yet.

        // Check tiff file.
        if (data.startsWith("MM\x00\x2a") || data.startsWith("II\x2a\x00")) {
            // big-endian, little-endian.
            return "TIFF";
        }

        // TODO(xushaohua): Support wbmp file.

        // Check webp file.
        if (data.startsWith("RIFFr\x00\x00\x00WEBPVP")) {
            return "WEBP";
        }

        // Check xbm file.
        if (data.indexOf("#define max_width ") > -1 && data.indexOf("#define max_height ") > -1) {
            return "XBM";
        }

        // Check xpm file.
        if (data.startsWith("/* XPM */")) {
            return "XPM";
        }
        return "";
    }
    return res;
}
UNIONIMAGESHARED_EXPORT bool isNoneQImage(const QImage &qi)
{
    return (qi == noneQImage());
}

UNIONIMAGESHARED_EXPORT bool rotateImage(int angel, QImage &image)
{
    if (angel % 90 != 0)
        return false;
    if (image.isNull()) {
        return false;
    }
    QImage image_copy(image);
    if (!image_copy.isNull()) {
        QMatrix rotatematrix;
        rotatematrix.rotate(angel);
        image = image_copy.transformed(rotatematrix, Qt::SmoothTransformation);
        return true;
    }
    return false;
}

QImage adjustImageToRealPosition(const QImage &image, int orientation)
{
    QImage result = image;

    switch (orientation) {
    case 1: //不做操作
    default:
        break;
    case 2: //水平翻转
        result = result.mirrored(true, false);
        break;
    case 3: //180度翻转
        rotateImage(180, result);
        break;
    case 4: //垂直翻转
        result = result.mirrored(false, true);
        break;
    case 5: //顺时针90度+水平翻转
        rotateImage(90, result);
        result = result.mirrored(true, false);
        break;
    case 6: //顺时针90度
        rotateImage(90, result);
        break;
    case 7: //顺时针90度+垂直翻转
        rotateImage(90, result);
        result = result.mirrored(false, true);
        break;
    case 8: //逆时针90度
        rotateImage(-90, result);
        break;
    };

    return result;
}

UNIONIMAGESHARED_EXPORT bool rotateImageFile(int angel, const QString &path, QString &erroMsg)
{
    if (angel % 90 != 0) {
        erroMsg = "unsupported angel";
        return false;
    }
    QString format = detectImageFormat(path);
    if (format == "SVG") {
        QImage image_copy;
        if (!loadStaticImageFromFile(path, image_copy, erroMsg)) {
            erroMsg = "rotate load QImage failed, path:" + path + "  ,format:+" + format;
            return false;
        }
        QPixmap svgpixmap(path);
        QMatrix matrix;
        matrix.rotate(angel);
        svgpixmap = svgpixmap.transformed(QTransform(matrix));
        QSvgGenerator generator;
        generator.setFileName(path);
        generator.setViewBox(svgpixmap.rect());
        QPainter rotatePainter;
        rotatePainter.begin(&generator);
        rotatePainter.drawPixmap(svgpixmap.rect(), svgpixmap);
        rotatePainter.end();
        return true;
    } else if (union_image_private.m_qtrotate.contains(format)) {
        //由于Qt内部不会去读图片的EXIF信息来判断当前的图像矩阵的真实位置，同时回写数据的时候会丢失全部的EXIF数据
        int orientation = getOrientation(path);
        QImage image_copy(path);
        image_copy = adjustImageToRealPosition(image_copy, orientation);
        if (!image_copy.isNull()) {
            QMatrix rotatematrix;
            rotatematrix.rotate(angel);
            image_copy = image_copy.transformed(rotatematrix, Qt::SmoothTransformation);
            if (image_copy.save(path, format.toLatin1().data(), SAVE_QUAITY_VALUE))
                return true;
            else {
                return false;
            }
        }
        erroMsg = "rotate by qt failed";
        return false;
    }
    return false;
}

UNIONIMAGESHARED_EXPORT bool rotateImageFileWithImage(int angel, QImage &img, const QString &path, QString &erroMsg)
{
    Q_UNUSED(img)
    if (angel % 90 != 0) {
        erroMsg = "unsupported angel";
        return false;
    }

    QString format = detectImageFormat(path);
    if (format == "SVG") {
        QImage image_copy;
        if (!loadStaticImageFromFile(path, image_copy, erroMsg)) {
            erroMsg = "rotate load QImage failed, path:" + path + "  ,format:+" + format;
            return false;
        }
        QSvgGenerator generator;
        generator.setFileName(path);
        generator.setViewBox(QRect(0, 0, image_copy.width(), image_copy.height()));
        QPainter rotatePainter;
        rotatePainter.begin(&generator);
        rotatePainter.resetTransform();
        rotatePainter.setRenderHint(QPainter::HighQualityAntialiasing, true);
        int realangel = angel / 90;
        if (realangel > 0) {
            for (int i = 0; i < qAbs(realangel); i++) {
                rotatePainter.translate(image_copy.width(), 0);
                rotatePainter.rotate(90 * (realangel / qAbs(realangel)));
            }
        } else {
            for (int i = 0; i < qAbs(realangel); i++) {
                rotatePainter.translate(0, image_copy.height());
                rotatePainter.rotate(90 * (realangel / qAbs(realangel)));
            }
        }
        rotatePainter.drawImage(image_copy.rect(), image_copy.scaled(image_copy.width(), image_copy.height()));
        rotatePainter.resetTransform();
        generator.setSize(QSize(image_copy.width(), image_copy.height()));
        rotatePainter.end();
        return true;
    } else if (union_image_private.m_qtrotate.contains(format)) {
        QPixmap image_copy(path);
        if (!image_copy.isNull()) {
            QMatrix rotatematrix;
            rotatematrix.rotate(angel);
            image_copy = image_copy.transformed(rotatematrix, Qt::SmoothTransformation);
            return image_copy.save(path, format.toLatin1().data(), SAVE_QUAITY_VALUE);
        }
        erroMsg = "rotate by qt failed";
        return false;
    }
    return false;
}

UNIONIMAGESHARED_EXPORT QMap<QString, QString> getAllMetaData(const QString &path)
{
    QMap<QString, QString> admMap;
    //移除秒　　2020/6/5 DJH
    //需要转义才能读出：或者/　　2020/8/21 DJH
    QFileInfo info(path);
    if (admMap.contains("DateTime")) {
        QDateTime time = QDateTime::fromString(admMap["DateTime"], "yyyy:MM:dd hh:mm:ss");
        admMap["DateTimeOriginal"] = time.toString("yyyy/MM/dd hh:mm");
    } else {
        admMap.insert("DateTimeOriginal",  info.lastModified().toString("yyyy/MM/dd HH:mm"));
    }
    admMap.insert("DateTimeDigitized",  info.lastModified().toString("yyyy/MM/dd HH:mm"));

//    // The value of width and height might incorrect
    QImageReader reader(path);
    int w = reader.size().width();
    int h = reader.size().height();
    admMap.insert("Dimension", QString::number(w) + "x" + QString::number(h));

    admMap.insert("FileName", info.fileName());
    admMap.insert("FileFormat", detectImageFormat(path));
    admMap.insert("FileSize", size2Human(info.size()));
    return admMap;
}

UNIONIMAGESHARED_EXPORT bool isImageSupportRotate(const QString &path)
{
    return canSave(path);
}


UNIONIMAGESHARED_EXPORT int getOrientation(const QString &path)
{
    int result = 1;   //1代表不做操作，维持原样
    return result;
}
QString PrivateDetectImageFormat(const QString &filepath)
{
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        return "";
    }

    const QByteArray data = file.read(1024);

    // Check bmp file.
    if (data.startsWith("BM")) {
        return "bmp";
    }

    // Check dds file.
    if (data.startsWith("DDS")) {
        return "dds";
    }

    // Check gif file.
    if (data.startsWith("GIF8")) {
        return "gif";
    }

    // Check Max OS icons file.
    if (data.startsWith("icns")) {
        return "icns";
    }

    // Check jpeg file.
    if (data.startsWith("\xff\xd8")) {
        return "jpg";
    }

    // Check mng file.
    if (data.startsWith("\x8a\x4d\x4e\x47\x0d\x0a\x1a\x0a")) {
        return "mng";
    }

    // Check net pbm file (BitMap).
    if (data.startsWith("P1") || data.startsWith("P4")) {
        return "pbm";
    }

    // Check pgm file (GrayMap).
    if (data.startsWith("P2") || data.startsWith("P5")) {
        return "pgm";
    }

    // Check ppm file (PixMap).
    if (data.startsWith("P3") || data.startsWith("P6")) {
        return "ppm";
    }

    // Check png file.
    if (data.startsWith("\x89PNG\x0d\x0a\x1a\x0a")) {
        return "png";
    }

    // Check svg file.
    if (data.indexOf("<svg") > -1) {
        return "svg";
    }

    // TODO(xushaohua): tga file is not supported yet.

    // Check tiff file.
    if (data.startsWith("MM\x00\x2a") || data.startsWith("II\x2a\x00")) {
        // big-endian, little-endian.
        return "tiff";
    }

    // TODO(xushaohua): Support wbmp file.

    // Check webp file.
    if (data.startsWith("RIFFr\x00\x00\x00WEBPVP")) {
        return "webp";
    }

    // Check xbm file.
    if (data.indexOf("#define max_width ") > -1 &&
            data.indexOf("#define max_height ") > -1) {
        return "xbm";
    }

    // Check xpm file.
    if (data.startsWith("/* XPM */")) {
        return "xpm";
    }
    return "";
}

};



