// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils/baseutils.h"
#include "utils/imageutils.h"
//#include "utils/imageutils_libexif.h"
#include "utils/unionimage.h"
#include <QBuffer>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QtSvg>
#include <QMimeDatabase>
#include <QMutexLocker>
#include <QPixmapCache>
#include <QProcess>
#include <QReadWriteLock>
#include <QUrl>
#include <QApplication>
#include <QMovie>
#include <fstream>
#include <iostream>

namespace utils {

namespace image {

bool imageSupportRead(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix();

    //FIXME: file types below will cause freeimage to crash on loading,
    // take them here for good.
    QStringList errorList;
    errorList << "X3F";
    if (errorList.indexOf(suffix.toUpper()) != -1) {
        return false;
    }
    //return QImageReader::supportedImageFormats().contains(suffix.toUtf8());
    return UnionImage_NameSpace::unionImageSupportFormat().contains(suffix.toUpper());
}

//bool imageSupportSave(const QString &path)
//{
//    const QString suffix = QFileInfo(path).suffix();

//    // RAW image decode is too slow, and most of these does not support saving
//    // RAW formats render incorrectly by freeimage
//    const QStringList raws = QStringList()
//                             << "CR2" << "CRW"   // Canon cameras
//                             << "DCR" << "KDC"   // Kodak cameras
//                             << "MRW"            // Minolta cameras
//                             << "NEF"            // Nikon cameras
//                             << "ORF"            // Olympus cameras
//                             << "PEF"            // Pentax cameras
//                             << "RAF"            // Fuji cameras
//                             << "SRF"            // Sony cameras
//                             << "PSD"
//                             << "ICO"
//                             << "TGA"
//                             << "WEBP"
//                             << "PBM"
//                             << "XPM"
//                             << "PPM"
//                             << "PGM"
//                             << "X3F"           // Sigma cameras
//                             << "SVG";          // need support SVG

//    //dynamic image can not be supported
//    if (QMovie::supportedFormats().contains(suffix.toLower().toUtf8().data())) {
//        QMovie movie(path);
//        return movie.frameCount() == 1 ? true : false;
//    }

//    //some images that decode slow also should be written.
//    if (raws.indexOf(suffix.toUpper()) != -1
//            || (QImageReader(path).imageCount() > 1 )) {
//        return true;
//    } else {
//        return UnionImage_NameSpace::canSave(path);
//    }
//}

void getAllDirInDir(const QDir &dir, QFileInfoList &result)
{
    QDir root(dir);
    auto list = root.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot);
    for (const auto &eachInfo : list) {
        if (eachInfo.isDir()) {
            result.push_back(eachInfo);
            getAllDirInDir(eachInfo.absoluteFilePath(), result);
        }
    }
}

void getAllFileInDir(const QDir &dir, QFileInfoList &result)
{
    QDir root(dir);
    auto list = root.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    for (const auto &eachInfo : list) {
        if (eachInfo.isDir()) {
            getAllFileInDir(eachInfo.absoluteFilePath(), result);
        } else {
            result.push_back(eachInfo);
        }
    }
}

const QFileInfoList getImagesAndVideoInfo(const QString &dir, bool recursive)
{
    QFileInfoList infos;

    if (! recursive) {
        QFileInfoList nsl;
        getAllFileInDir(dir, nsl);
        for (QFileInfo info : nsl) {
            if (imageSupportRead(info.absoluteFilePath()) ||
                    utils::base::isVideo(info.absoluteFilePath())) {
                infos << info;
            }
        }
        return infos;
    }

    QDirIterator dirIterator(dir,
                             QDir::Files,
                             QDirIterator::Subdirectories);
    while (dirIterator.hasNext()) {
        dirIterator.next();
        if ( imageSupportRead(dirIterator.fileInfo().absoluteFilePath())
                || utils::base::isVideo(dirIterator.fileInfo().absoluteFilePath())) {
            infos << dirIterator.fileInfo();
        }
    }

    return infos;
}

QStringList supportedImageFormats()
{
    return UnionImage_NameSpace::unionImageSupportFormat();
}

//bool checkFileType(const QString &path)
//{
//    if (imageSupportRead(path)) {
//        QFileInfo info(path);
//        QMimeDatabase db;
//        QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
//        QString str = info.suffix().toLower();
//        if (mt.name().startsWith("image/*") || mt.name().startsWith("video/x-mng")) {
//            if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
//                return true;
//            }
//        }
//        /**
//         * 2020/4/29
//         * QMimeType能识别的图片类型有限，相册能支持的格式已经不止这些，使用ImageSupporter后替换该函数
//         */
//        if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
//            return true;
//        }
//    }
//    return false;
//}

QPixmap getDamagePixmap(bool bLight)
{
    static QPixmap pix_light, pix_dark;
    if (bLight) {
        if (pix_light.isNull())
            pix_light = utils::base::renderSVG(view::LIGHT_DAMAGEICON, QSize(150, 150));
        return pix_light;
    } else {
        if (pix_dark.isNull())
            pix_dark = utils::base::renderSVG(view::DARK_DAMAGEICON, QSize(150, 150));
        return pix_dark;
    }
}

}  // namespace image

}  //namespace utils
