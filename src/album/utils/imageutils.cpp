/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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

void getAllFileInDir(const QDir &dir, QFileInfoList &result)
{
    QDir root(dir);
    auto list = root.entryInfoList();
    for (const auto &eachInfo : list) {
        if (eachInfo.filePath().endsWith("/.") || eachInfo.filePath().endsWith("/..")) {
            continue;
        }

        if (eachInfo.isDir()) {
            getAllFileInDir(eachInfo.absoluteFilePath(), result);
        } else {
            result.push_back(eachInfo);
        }
    }
}

const QFileInfoList getImagesInfo(const QString &dir, bool recursive)
{
    QFileInfoList infos;

    if (! recursive) {
        QFileInfoList nsl;
        getAllFileInDir(dir, nsl);
        for (QFileInfo info : nsl) {
            if (imageSupportRead(info.absoluteFilePath())) {
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
        if (imageSupportRead(dirIterator.fileInfo().absoluteFilePath())) {
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
            pix_light = utils::base::renderSVG(view::LIGHT_DAMAGEICON, QSize(40, 40));
        return pix_light;
    } else {
        if (pix_dark.isNull())
            pix_dark = utils::base::renderSVG(view::DARK_DAMAGEICON, QSize(40, 40));
        return pix_dark;
    }
}

}  // namespace image

}  //namespace utils
