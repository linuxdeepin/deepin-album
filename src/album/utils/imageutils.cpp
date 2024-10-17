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
    // take them here for good.
    QStringList errorList;
    errorList << "X3F";
    if (errorList.indexOf(suffix.toUpper()) != -1) {
        return false;
    }
    //return QImageReader::supportedImageFormats().contains(suffix.toUtf8());
    return UnionImage_NameSpace::unionImageSupportFormat().contains(suffix.toUpper());
}

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
