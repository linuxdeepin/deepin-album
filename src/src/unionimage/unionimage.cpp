// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "unionimage.h"

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QDate>
#include <QTime>
#include <QtMath>
#include <QMovie>
#include <QTransform>
#include <QPainter>
#include <QSvgGenerator>
#include <QImageReader>
#include <QMimeDatabase>
#include <QtSvg/QSvgRenderer>
#include <QDir>
#include <QDebug>
#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QColorSpace>
#endif

#include "unionimage/imageutils.h"

#include <cstring>

#define SAVE_QUAITY_VALUE 100

const QString DATETIME_FORMAT_NORMAL = "yyyy.MM.dd";
const QString DATETIME_FORMAT_EXIF = "yyyy:MM:dd HH:mm";

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
/**
   @brief 转换图片颜色空间到sRGB，解决CMYK等颜色空间图片显示颜色不正确的问题
   @param image 原始图片
   @return 转换后的图片，如果不需要转换或转换失败则返回原图
   @note 此功能仅在Qt6中可用
 */
static QImage convertToSRgbColorSpace(const QImage &image)
{
    if (image.isNull()) {
        return image;
    }

    QColorSpace srgbColorSpace = QColorSpace::SRgb;

    bool needsConversion = false;
    if (image.colorSpace().isValid()) {
        qDebug() << "Image has color space:" << image.colorSpace().description();

        if (image.colorSpace() != srgbColorSpace) {
            needsConversion = true;
            qDebug() << "Converting color space from" << image.colorSpace().description()
                                    << "to sRGB";
        }
    } else {
        qDebug() << "Image has no valid color space, checking format for potential CMYK";
        if (image.format() == QImage::Format_CMYK8888) {
            needsConversion = true;
            qDebug() << "CMYK format detected, attempting conversion";
        }
    }

    if (!needsConversion) {
        qDebug() << "No color space conversion needed";
        return image;
    }

    QImage convertedImage = QImage();

    try {
        convertedImage = image.convertedToColorSpace(srgbColorSpace);
        if (!convertedImage.isNull()) {
            qDebug() << "Color space conversion method 1 (convertedToColorSpace) succeeded";
        } else {
            qDebug() << "Color space conversion method 1 failed";
        }
    } catch (...) {
        qDebug() << "Color space conversion method 1 threw exception";
    }

    if (convertedImage.isNull()) {
        qDebug() << "Trying color space conversion method 2: manual color space setting";

        convertedImage = image.copy();
        convertedImage.setColorSpace(srgbColorSpace);

        if (convertedImage.format() != QImage::Format_RGB888 &&
            convertedImage.format() != QImage::Format_ARGB32 &&
            convertedImage.format() != QImage::Format_ARGB32_Premultiplied) {
            convertedImage = convertedImage.convertToFormat(QImage::Format_RGB888);
        }

        if (!convertedImage.isNull()) {
            qDebug() << "Color space conversion method 2 succeeded";
        }
    }

    if (convertedImage.isNull()) {
        qDebug() << "Trying color space conversion method 3: basic format conversion";
        convertedImage = image.convertToFormat(QImage::Format_RGB888);
        convertedImage.setColorSpace(srgbColorSpace);

        if (!convertedImage.isNull()) {
            qDebug() << "Color space conversion method 3 succeeded";
        }
    }

    if (convertedImage.isNull()) {
        qWarning() << "All color space conversion methods failed, returning original image";
        return image;
    }

    return convertedImage;
}
#endif

namespace LibUnionImage_NameSpace {

class UnionImage_Private
{

public:
    UnionImage_Private()
    {
        qDebug() << "Initializing UnionImage_Private with supported formats";
        /*
         * 由于原设计方案采用多个key对应一个value的方案，在判断可读可写的过程中是通过value去找key因此造成了多种情况而在下方变量中未将key，写完整因此补全
         * */
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
        /*<< "PGM" << "PBM"*/
        m_qtrotate << "BMP"
                   << "JPG"
                   << "JPEG"
                   << "PNG"
                   << "PGM"
                   << "PPM"
                   << "XPM"
                   << "ICO"
                   << "ICNS";
        qDebug() << "Initialized with" << m_qtSupported.size() << "supported formats," 
                 << m_canSave.size() << "saveable formats, and" 
                 << m_qtrotate.size() << "rotatable formats";
    }
    ~UnionImage_Private()
    {
        qDebug() << "Destroying UnionImage_Private";
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
    qDebug() << "Creating empty QImage";
    static QImage none(0, 0, QImage::Format_Invalid);
    return none;
}

UNIONIMAGESHARED_EXPORT const QStringList unionImageSupportFormat()
{
    qDebug() << "Getting supported image formats";
    static QStringList res;
    if (res.empty()) {
        QStringList list = union_image_private.m_qtSupported;
        res.append(list);
        qDebug() << "Found" << res.size() << "supported formats";
    }
    return res;
}
UNIONIMAGESHARED_EXPORT const QStringList videoFiletypes()
{
    qDebug() << "Getting supported video file types";
    const QStringList m_videoFiletypes = {"avs2"/*支持avs2视频格式*/, "3g2", "3ga", "3gp", "3gp2"
                                          , "3gpp", "amv", "asf", "asx", "avf", "avi", "bdm"
                                          , "bdmv", "bik", "clpi", "cpi", "dat", "divx", "drc"
                                          , "dv", "dvr-ms", "f4v", "flv", "gvi", "gxf", "hdmov"
                                          , "hlv", "iso", "letv", "lrv", "m1v", "m2p", "m2t"
                                          , "m2ts", "m2v", "m3u", "m3u8", "m4v", "mkv", "moov"
                                          , "mov", "mov", "mp2", "mp2v", "mp4", "mp4v", "mpe"
                                          , "mpeg", "mpeg1", "mpeg2", "mpeg4", "mpg", "mpl", "mpls"
                                          , "mpv", "mpv2", "mqv", "mts", "mts", "mtv", "mxf", "mxg"
                                          , "nsv", "nuv", "ogg", "ogm", "ogv", "ogx", "ps", "qt"
                                          , "qtvr", "ram", "rec", "rm", "rm", "rmj", "rmm", "rms"
                                          , "rmvb", "rmx", "rp", "rpl", "rv", "rvx", "thp", "tod"
                                          , "tp", "trp", "ts", "tts", "txd", "vcd", "vdr", "vob"
                                          , "vp8", "vro", "webm", "wm", "wmv", "wtv", "xesc", "xspf"
                                         };
    qDebug() << "Found" << m_videoFiletypes.size() << "supported video formats";
    return m_videoFiletypes;
}

UNIONIMAGESHARED_EXPORT const QStringList supportStaticFormat()
{
    qDebug() << "Getting supported static formats";
    return (union_image_private.m_qtSupported);
}

UNIONIMAGESHARED_EXPORT const QStringList supportMovieFormat()
{
    qDebug() << "Getting supported movie formats";
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
    qDebug() << "Converting size to human readable format:" << bytes << "bytes";
    qlonglong kb = 1024;
    QString result;
    if (bytes < kb) {
        result = QString::number(bytes) + " B";
    } else if (bytes < kb * kb) {
        QString vs = QString::number(static_cast<double>(bytes) / kb, 'f', 1);
        if (qCeil(vs.toDouble()) == qFloor(vs.toDouble())) {
            result = QString::number(static_cast<int>(vs.toDouble())) + " KB";
        } else {
            result = vs + " KB";
        }
    } else if (bytes < kb * kb * kb) {
        QString vs = QString::number(static_cast<double>(bytes) / kb / kb, 'f', 1);
        if (qCeil(vs.toDouble()) == qFloor(vs.toDouble())) {
            result = QString::number(static_cast<int>(vs.toDouble())) + " MB";
        } else {
            result = vs + " MB";
        }
    } else {
        //修改了当超过一个G的图片,应该用G返回,不应该返回一堆数字,bug68094
        QString vs = QString::number(static_cast<double>(bytes) / kb / kb / kb, 'f', 1);
        if (qCeil(vs.toDouble()) == qFloor(vs.toDouble())) {
            result = QString::number(static_cast<int>(vs.toDouble())) + " GB";
        } else {
            result = vs + " GB";
        }
    }
    qDebug() << "Converted size:" << result;
    return result;
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
    qDebug() << "Getting file format for:" << path;
    QFileInfo fi(path);
    QString suffix = fi.suffix();
    qDebug() << "File format:" << suffix;
    return suffix;
}

UNIONIMAGESHARED_EXPORT bool canSave(const QString &path)
{
    qDebug() << "Checking if file can be saved:" << path;
    QImageReader r(path);
    if (r.imageCount() > 1) {
        qDebug() << "File has multiple images, cannot save";
        return false;
    }
    QFileInfo info(path);
    bool canSave = union_image_private.m_canSave.contains(info.suffix().toUpper());
    qDebug() << "File can be saved:" << canSave;
    return canSave;
}

UNIONIMAGESHARED_EXPORT QString unionImageVersion()
{
    qDebug() << "Getting UnionImage version";
    QString ver;
    ver.append("UnionImage Version:");
    ver.append("0.0.4");
    ver.append("\n");
    return ver;
}

UNIONIMAGESHARED_EXPORT bool creatNewImage(QImage &res, int width, int height, int depth, SupportType type)
{
    qDebug() << "Creating new image with dimensions:" << width << "x" << height << "depth:" << depth;
    Q_UNUSED(type);
    if (depth == 8) {
        res = QImage(width, height, QImage::Format_RGB888);
    } else if (depth == 16) {
        res = QImage(width, height, QImage::Format_RGB16);
    } else {
        res = QImage(width, height, QImage::Format_RGB32);
    }
    qDebug() << "Created new image with format:" << res.format();
    return true;
}

QString PrivateDetectImageFormat(const QString &filepath);
UNIONIMAGESHARED_EXPORT bool loadStaticImageFromFile(const QString &path, QImage &res, QString &errorMsg, const QString &format_bar)
{
    qDebug() << "Loading static image from file:" << path;
    QFileInfo file_info(path);
    if (file_info.size() == 0) {
        qWarning() << "File is empty:" << path;
        res = QImage();
        errorMsg = "error file!";
        return false;
    }
    QMap<QString, QString> dataMap = getAllMetaData(path);
    QString file_suffix_upper = dataMap.value("FileFormat").toUpper();

    QByteArray temp_path;
    temp_path.append(path.toUtf8());
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
                qDebug() << "Failed to read image with QImageReader, trying alternative method";
                QString format = PrivateDetectImageFormat(path);
                QImageReader readerF(path, format.toLatin1());
                QImage try_res;
                readerF.setAutoTransform(true);
                if (readerF.canRead()) {
                    try_res = readerF.read();
                } else {
                    errorMsg = "can't read image:" + readerF.errorString() + format;
                    try_res = QImage(path);
                }
                if (try_res.isNull()) {
                    errorMsg = "load image by qt faild, use format:" + reader.format() + " ,path:" + path;
                    res = QImage();
                    return false;
                }
                errorMsg = "use old method to load QImage";
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                // 应用颜色空间转换，解决CMYK等格式的颜色显示问题 (仅Qt6)
                res = convertToSRgbColorSpace(try_res);
#else
                res = try_res;
#endif
                return true;
            }
            errorMsg = "use QImage";
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
            // 应用颜色空间转换，解决CMYK等格式的颜色显示问题 (仅Qt6)
            res = convertToSRgbColorSpace(res_qt);
#else
            res = res_qt;
#endif
        } else {
            qWarning() << "No images found in file:" << path;
            res = QImage();
            return false;
        }
        qDebug() << "Successfully loaded image from file";
        return true;
    }
    qWarning() << "Unsupported file format:" << file_suffix_upper;
    return false;
}

UNIONIMAGESHARED_EXPORT QString detectImageFormat(const QString &path)
{
    qDebug() << "Detecting image format for:" << path;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for format detection:" << path;
        return "";
    }

    const QByteArray data = file.read(1024);

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

UNIONIMAGESHARED_EXPORT bool isNoneQImage(const QImage &qi)
{
    return (qi == noneQImage());
}

UNIONIMAGESHARED_EXPORT bool rotateImage(int angel, QImage &image)
{
    qDebug() << "Rotating image by" << angel << "degrees";
    if (angel % 90 != 0) {
        qWarning() << "Invalid rotation angle:" << angel;
        return false;
    }
    if (image.isNull()) {
        qWarning() << "Cannot rotate null image";
        return false;
    }
    QImage image_copy(image);
    if (!image_copy.isNull()) {
        QTransform rotatematrix;
        rotatematrix.rotate(angel);
        image = image_copy.transformed(rotatematrix, Qt::SmoothTransformation);
        qDebug() << "Successfully rotated image";
        return true;
    }
    qWarning() << "Failed to create image copy for rotation";
    return false;
}

/**
 * @brief 根据翻转、旋转类型 \a orientation 对传入的图片 \a image 进行翻转旋转操作。
 * @param image         传入图片
 * @param orientation   翻转、旋转类型
 * @return 翻转、旋转后的图片
 */
QImage adjustImageToRealPosition(const QImage &image, int orientation)
{
    qDebug() << "Adjusting image to real position with orientation:" << orientation;
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

    qDebug() << "Successfully adjusted image position";
    return result;
}

UNIONIMAGESHARED_EXPORT bool rotateImageFIle(int angel, const QString &path, QString &erroMsg, const QString &targetPath)
{
    qDebug() << "Rotating image file:" << path << "by" << angel << "degrees";
    if (angel % 90 != 0) {
        erroMsg = "unsupported angel";
        qWarning() << "Invalid rotation angle:" << angel;
        return false;
    }

    // 保存文件路径，若未设置则保存至原文件
    QString savePath = targetPath.isEmpty() ? path : targetPath;
    qDebug() << "Saving rotated image to:" << savePath;

    QString format = detectImageFormat(path);
    if (format == "SVG") {
        qDebug() << "Rotating SVG file";
        QImage image_copy;
        if (!loadStaticImageFromFile(path, image_copy, erroMsg)) {
            erroMsg = "rotate load QImage faild, path:" + path + "  ,format:+" + format;
            qWarning() << "Failed to load SVG for rotation:" << erroMsg;
            return false;
        }
        QSvgGenerator generator;
        generator.setFileName(savePath);
        generator.setViewBox(QRect(0, 0, image_copy.width(), image_copy.height()));
        QPainter rotatePainter;
        rotatePainter.begin(&generator);
        rotatePainter.resetTransform();
        rotatePainter.setRenderHint(QPainter::Antialiasing, true);
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
        qDebug() << "Successfully rotated SVG file";
        return true;

    } else if (union_image_private.m_qtrotate.contains(format)) {
        //由于Qt内部不会去读图片的EXIF信息来判断当前的图像矩阵的真实位置，同时回写数据的时候会丢失全部的EXIF数据
        int orientation = getOrientation(path);
        QImage image_copy(path);
        image_copy = adjustImageToRealPosition(image_copy, orientation);
        if (!image_copy.isNull()) {
            QTransform rotatematrix;
            rotatematrix.rotate(angel);
            image_copy = image_copy.transformed(rotatematrix, Qt::SmoothTransformation);
            if (image_copy.save(savePath, format.toLatin1().data(), SAVE_QUAITY_VALUE)) {
                qDebug() << "Successfully rotated and saved image";
                return true;
            } else {
                qWarning() << "Failed to save rotated image";
                return false;
            }
        }
        erroMsg = "rotate by qt failed";
        qWarning() << "Failed to rotate image with Qt:" << erroMsg;
        return false;
    }
    qWarning() << "Unsupported format for rotation:" << format;
    return false;
}

UNIONIMAGESHARED_EXPORT bool rotateImageFIleWithImage(int angel, QImage &img, const QString &path, QString &erroMsg)
{
    qDebug() << "Rotating image file with provided image:" << path << "by" << angel << "degrees";
    if (angel % 90 != 0) {
        erroMsg = "unsupported angel";
        qWarning() << "Invalid rotation angle:" << angel;
        return false;
    }
    QImage image_copy;
    if (img.isNull()) {
        qWarning() << "Cannot rotate null image";
        return false;
    } else {
        image_copy = img;
    }

    QString format = detectImageFormat(path);
    if (format == "SVG") {
        qDebug() << "Rotating SVG file with provided image";
        QSvgGenerator generator;
        generator.setFileName(path);
        generator.setViewBox(QRect(0, 0, image_copy.width(), image_copy.height()));
        QPainter rotatePainter;
        rotatePainter.begin(&generator);
        rotatePainter.resetTransform();
        rotatePainter.setRenderHint(QPainter::Antialiasing, true);
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
        qDebug() << "Successfully rotated SVG file with provided image";
        return true;
    } else if (format == "JPG" || format == "JPEG") {
        qDebug() << "Rotating JPEG file with provided image";
        QImage image_copy(path, "JPG");
        if (!image_copy.isNull()) {
            QPainter rotatePainter(&image_copy);
            rotatePainter.rotate(angel);
            rotatePainter.end();
            if (image_copy.save(path, "jpg", SAVE_QUAITY_VALUE)) {
                qDebug() << "Successfully rotated and saved JPEG file";
                return true;
            }
        }
        qWarning() << "Failed to rotate JPEG file";
    }
    qWarning() << "Unsupported format for rotation with image:" << format;
    return false;
}

UNIONIMAGESHARED_EXPORT QMap<QString, QString> getAllMetaData(const QString &path)
{
    qDebug() << "Getting all metadata for:" << path;
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

           // The value of width and height might incorrect
    QImageReader reader(path);
    int w = reader.size().width();
    int h = reader.size().height();
    admMap.insert("Dimension", QString::number(w) + "x" + QString::number(h));
    // 记录图片宽高
    admMap.insert("Width", QString::number(w));
    admMap.insert("Height", QString::number(h));

    admMap.insert("FileName", info.fileName());
    //应该使用qfileinfo的格式
    admMap.insert("FileFormat", getFileFormat(path));
    admMap.insert("FileSize", size2Human(info.size()));

    qDebug() << "Found" << admMap.size() << "metadata entries";
    return admMap;
}

UNIONIMAGESHARED_EXPORT bool isImageSupportRotate(const QString &path)
{
    return canSave(path) ;
}

UNIONIMAGESHARED_EXPORT int getOrientation(const QString &path)
{
    Q_UNUSED(path);
    return 1;
}

imageViewerSpace::ImageType getImageType(const QString &imagepath)
{
    qDebug() << "Getting image type for:" << imagepath;
    imageViewerSpace::ImageType type = imageViewerSpace::ImageType::ImageTypeBlank;
    //新增获取图片是属于静态图还是动态图还是多页图
    if (!imagepath.isEmpty()) {
        QFileInfo fi(imagepath);
        if (!fi.exists()) {
            // 文件不存在返回空
            qWarning() << "File does not exist:" << imagepath;
            return imageViewerSpace::ImageTypeBlank;
        }

        QString strType = fi.suffix().toLower();
        //解决bug57394 【专业版1031】【看图】【5.6.3.74】【修改引入】pic格式图片变为翻页状态，不为动图且首张显示序号为0
        QMimeDatabase db;
        QMimeType mt = db.mimeTypeForFile(imagepath, QMimeDatabase::MatchContent);
        QMimeType mt1 = db.mimeTypeForFile(imagepath, QMimeDatabase::MatchExtension);
        QString path1 = mt.name();
        QString path2 = mt1.name();

        QImageReader imgreader(imagepath);
        int nSize = imgreader.imageCount();
        //
        if (strType == "svg" && QSvgRenderer().load(imagepath)) {
            type = imageViewerSpace::ImageTypeSvg;
        } else if ((strType == "mng")
                   || ((strType == "gif") && nSize > 1)
                   || (strType == "webp" && nSize > 1)
                   || ((mt.name().startsWith("image/gif")) && nSize > 1)
                   || ((mt1.name().startsWith("image/gif")) && nSize > 1)
                   || ((mt.name().startsWith("video/x-mng")))
                   || ((mt1.name().startsWith("video/x-mng")))) {
            type = imageViewerSpace::ImageTypeDynamic;
        } else if (nSize > 1) {
            type = imageViewerSpace::ImageTypeMulti;
        } else {
            type = imageViewerSpace::ImageTypeStatic;
        }
    }
    qDebug() << "Image type:" << type;
    return type;
}

imageViewerSpace::PathType getPathType(const QString &imagepath)
{
    //判断文件路径来自于哪里
    qDebug() << "Getting path type for:" << imagepath;
    imageViewerSpace::PathType type = imageViewerSpace::PathType::PathTypeLOCAL;
    if (imagepath.indexOf("smb-share:server=") != -1) {
        type = imageViewerSpace::PathTypeSMB;
    } else if (imagepath.indexOf("mtp:host=") != -1) {
        type = imageViewerSpace::PathTypeMTP;
    } else if (imagepath.indexOf("gphoto2:host=") != -1) {
        type = imageViewerSpace::PathTypePTP;
    } else if (imagepath.indexOf("gphoto2:host=Apple") != -1) {
        type = imageViewerSpace::PathTypeAPPLE;
    } else if (Libutils::image::isVaultFile(imagepath)) {
        type = imageViewerSpace::PathTypeSAFEBOX;
    } else if (imagepath.contains(QDir::homePath() + "/.local/share/Trash")) {
        type = imageViewerSpace::PathTypeRECYCLEBIN;
    }
    //todo
    qDebug() << "Path type:" << type;
    return type;
}

QString PrivateDetectImageFormat(const QString &filepath)
{
    qDebug() << "Detecting image format (private) for:" << filepath;
    QFile file(filepath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for format detection:" << filepath;
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

UNIONIMAGESHARED_EXPORT QString hashByString(const QString &str)
{
    return Libutils::base::hashByString(str);
}

UNIONIMAGESHARED_EXPORT void getAllFileInDir(const QDir &dir, QFileInfoList &result)
{
    qDebug() << "Getting all files in directory:" << dir.path();
    return Libutils::image::getAllFileInDir(dir, result);
}

UNIONIMAGESHARED_EXPORT std::pair<QDateTime, bool> analyzeDateTime(const QVariant &data)
{
    return Libutils::base::analyzeDateTime(data);
}

UNIONIMAGESHARED_EXPORT QString getDeleteFullPath(const QString &hash, const QString &fileName)
{
    qDebug() << "Getting delete full path for hash:" << hash << "fileName:" << fileName;
    return Libutils::base::getDeleteFullPath(hash, fileName);
}

UNIONIMAGESHARED_EXPORT bool syncCopy(const QString &srcFileName, const QString &dstFileName)
{
    qDebug() << "Performing synchronous copy from:" << srcFileName << "to:" << dstFileName;
    return Libutils::base::syncCopy(srcFileName, dstFileName);
}

UNIONIMAGESHARED_EXPORT bool isVaultFile(const QString &path)
{
    qDebug() << "Checking if file is in vault:" << path;
    return Libutils::image::isVaultFile(path);
}

UNIONIMAGESHARED_EXPORT bool trashFile(const QString &file)
{
    qDebug() << "Moving file to trash:" << file;
    return Libutils::base::trashFile(file);
}

UNIONIMAGESHARED_EXPORT QFileInfoList getImagesAndVideoInfo(const QString &dir, bool recursive)
{
    qDebug() << "Getting images and video info from directory:" << dir << "recursive:" << recursive;
    return Libutils::image::getImagesAndVideoInfo(dir, recursive);
}

UNIONIMAGESHARED_EXPORT bool isVideo(QString path)
{
    qDebug() << "Checking if file is video:" << path;
    return Libutils::image::isVideo(path);
}

UNIONIMAGESHARED_EXPORT bool imageSupportRead(const QString &path)
{
    qDebug() << "Checking if image format is supported for reading:" << path;
    return Libutils::image::imageSupportRead(path);
}

UNIONIMAGESHARED_EXPORT void getAllDirInDir(const QDir &dir, QFileInfoList &result)
{
    qDebug() << "Getting all directories in:" << dir.path();
    QDir root(dir);
    auto list = root.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (const auto &eachInfo : list) {
        if (eachInfo.isDir()) {
            result.push_back(eachInfo);
            getAllDirInDir(eachInfo.absoluteFilePath(), result);
        }
    }
    qDebug() << "Found" << result.size() << "directories";
}

UNIONIMAGESHARED_EXPORT bool isImage(const QString &path)
{
    qDebug() << "Checking if file is an image:" << path;
    bool bRet = false;
    //路径为空直接跳出
    if (!path.isEmpty()) {
        QMimeDatabase db;
        QMimeType mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
        QMimeType mt1 = db.mimeTypeForFile(path, QMimeDatabase::MatchExtension);
        if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng") ||
                mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
            bRet = true;
        }
    }
    qDebug() << "File is an image:" << bRet;
    return bRet;
}

UNIONIMAGESHARED_EXPORT QString localPath(const QUrl &url)
{
    qDebug() << "Getting local path for URL:" << url;
    QString path = url.toLocalFile();
    if (path.isEmpty()) {
        path = url.toString().isEmpty() ? url.path() : url.toString();
    }

    qDebug() << "Local path:" << path;
    return path;
}

UNIONIMAGESHARED_EXPORT QPixmap renderSVG(const QString &path, const QSize &size)
{
    qDebug() << "Rendering SVG file:" << path << "with size:" << size;
    return Libutils::base::renderSVG(path, size);
}

};



