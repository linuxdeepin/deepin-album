/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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

namespace utils {

namespace image {

const QFileInfoList getImagesInfo(const QString &dir, bool recursive)
{
    QFileInfoList infos;
    if (! recursive) {
        auto nsl = QDir(dir).entryInfoList(QDir::Files);
        for (QFileInfo info : nsl) {
            if (UnionImage_NameSpace::unionImageSupportFormat().contains(info.absoluteFilePath())) {
                infos << info;
            }
        }
        return infos;
    }
    QDirIterator dirIterator(dir,QDir::Files,QDirIterator::Subdirectories);
    while (dirIterator.hasNext()) {
        dirIterator.next();
        if (UnionImage_NameSpace::unionImageSupportFormat().contains(dirIterator.fileInfo().absoluteFilePath())) {
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
        if (pix_light.isNull ())
            pix_light = utils::base::renderSVG (view::LIGHT_DAMAGEICON, QSize(40, 40));
        return pix_light;
    } else {
        if (pix_dark.isNull ())
            pix_dark = utils::base::renderSVG (view::DARK_DAMAGEICON, QSize(40, 40));
        return pix_dark;
    }
}

bool imageSupportRead(const QString &path)
{
    if(UnionImage_NameSpace::unionImageSupportFormat().contains(path))
        return QFileInfo(path).exists();
    else return false;
}

}  // namespace image

}  //namespace utils
