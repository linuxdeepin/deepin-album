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

bool imageSupportSave(const QString &path)
{
    const QString suffix = QFileInfo(path).suffix();

    // RAW image decode is too slow, and most of these does not support saving
    // RAW formats render incorrectly by freeimage
    const QStringList raws = QStringList()
                             << "CR2" << "CRW"   // Canon cameras
                             << "DCR" << "KDC"   // Kodak cameras
                             << "MRW"            // Minolta cameras
                             << "NEF"            // Nikon cameras
                             << "ORF"            // Olympus cameras
                             << "PEF"            // Pentax cameras
                             << "RAF"            // Fuji cameras
                             << "SRF"            // Sony cameras
                             << "PSD"
                             << "ICO"
                             << "TGA"
                             << "WEBP"
                             << "PBM"
                             << "XPM"
                             << "PPM"
                             << "PGM"
                             << "X3F"           // Sigma cameras
                             << "SVG";          // need support SVG

    //dynamic image can not be supported
    if (QMovie::supportedFormats().contains(suffix.toLower().toUtf8().data())) {
        QMovie  movie(path);
        return movie.frameCount() == 1 ? true : false;
    }

    //some images that decode slow also should be written.
    if (raws.indexOf(suffix.toUpper()) != -1
            || (QImageReader(path).imageCount() > 1 )) {
        return true;
    } else {
        return UnionImage_NameSpace::canSave(path);
    }
}

const QFileInfoList getImagesInfo(const QString &dir, bool recursive)
{
    QFileInfoList infos;

    if (! recursive) {
        auto nsl = QDir(dir).entryInfoList(QDir::Files);
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

const QString getOrientation(const QString &path)
{
    return UnionImage_NameSpace::getOrientation(path);
}


const QMap<QString, QString> getAllMetaData(const QString &path)
{
    return UnionImage_NameSpace::getAllMetaData(path);
}

const QPixmap cachePixmap(const QString &path)
{
    QPixmap pp;
    if (! QPixmapCache::find(path, &pp)) {
        pp = QPixmap(path);
        QPixmapCache::insert(path, pp);
    }
    return pp;
}

const QString toMd5(const QByteArray &data)
{
    return QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex();
}

/*!
 * \brief thumbnailAttribute
 * Read the attributes of file for generage thumbnail
 * \param url
 * \return
 */
QMap<QString, QString> thumbnailAttribute(const QUrl &url)
{
    QMap<QString, QString> set;

    if (url.isLocalFile()) {
        const QString path = url.path();
        QFileInfo info(path);
        set.insert("Thumb::Mimetype", QMimeDatabase().mimeTypeForFile(path).name());
        set.insert("Thumb::Size", QString::number(info.size()));
        set.insert("Thumb::URI", url.toString());
        set.insert("Thumb::MTime", QString::number(info.lastModified().toTime_t()));
        set.insert("Software", "Deepin Image Viewer");

        QImageReader reader(path);
        if (reader.canRead()) {
            set.insert("Thumb::Image::Width", QString::number(reader.size().width()));
            set.insert("Thumb::Image::Height", QString::number(reader.size().height()));
        }
        return set;
    } else {
        //TODO for other's scheme
    }

    return set;
}

const QString thumbnailCachePath()
{
    QString cacheP;

    QStringList systemEnvs = QProcess::systemEnvironment();
    for (QString it : systemEnvs) {
        QStringList el = it.split("=");
        if (el.length() == 2 && el.first() == "XDG_CACHE_HOME") {
            cacheP = el.last();
            break;
        }
    }
    cacheP = cacheP.isEmpty() ? (QDir::homePath() + "/.cache") : cacheP;

    // Check specific size dir
    const QString thumbCacheP = cacheP + "/thumbnails_album";
    QDir().mkpath(thumbCacheP + "/normal");
    QDir().mkpath(thumbCacheP + "/large");
    QDir().mkpath(thumbCacheP + "/fail");
    return thumbCacheP;
}

const QString thumbnailPath(const QString &path, ThumbnailType type)
{
    const QString cacheP = thumbnailCachePath();
    const QUrl url = QUrl::fromLocalFile(path);
    const QString md5s = toMd5(url.toString(QUrl::FullyEncoded).toLocal8Bit());
    QString tp;
    switch (type) {
    case ThumbNormal:
        tp = cacheP + "/normal/" + md5s + ".png";
        break;
    case ThumbLarge:
        tp = cacheP + "/large/" + md5s + ".png";
        break;
    case ThumbFail:
        tp = cacheP + "/fail/" + md5s + ".png";
        break;
    }
    return tp;
}

void removeThumbnail(const QString &path)
{
    QFile(thumbnailPath(path, ThumbLarge)).remove();
    QFile(thumbnailPath(path, ThumbNormal)).remove();
    QFile(thumbnailPath(path, ThumbFail)).remove();
}

bool thumbnailExist(const QString &path, ThumbnailType type)
{
    if (QFileInfo(thumbnailPath(path, type)).exists()
//            || QFileInfo(thumbnailPath(path, ThumbNormal)).exists()
//            || QFileInfo(thumbnailPath(path, ThumbFail)).exists()
       ) {
        return true;
    } else {
        return false;
    }
}

static QStringList fromByteArrayList(const QByteArrayList &list)
{
    QStringList sList;

    for (const QByteArray &i : list)
        sList << "*." + QString::fromLatin1(i);

    // extern image format//add by luzhou for Bug2672
    sList << "*.cr2"
          << "*.dng"
          << "*.nef"
          << "*.mef"
          << "*.mrw";

    return sList;
}

QStringList supportedImageFormats()
{
    //static QStringList list = fromByteArrayList(QImageReader::supportedImageFormats());

    //return list;
    return UnionImage_NameSpace::unionImageSupportFormat();
}

bool checkFileType(const QString &path)
{
    if (imageSupportRead(path)) {
        QFileInfo info(path);
        QMimeDatabase db;
        QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);

        QString str = info.suffix().toLower();
        if (mt.name().startsWith("image/*") || mt.name().startsWith("video/x-mng")) {
            if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
                return true;
            }
        }
        /**
         * 2020/4/29
         * QMimeType能识别的图片类型有限，相册能支持的格式已经不止这些，使用ImageSupporter后替换该函数
         */
        if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

QStringList checkImage(const QString  path)
{
    QStringList imagelist;
    QDir dir(path);

    if (!dir.exists()) {
        return imagelist;
    }

    QFileInfoList dirlist = dir.entryInfoList(QDir::Dirs);

    foreach (QFileInfo e_dir, dirlist) {
        if (e_dir.fileName() == "." || e_dir.fileName() == "..") {
            continue;
        }
        if (e_dir.exists()) {
            imagelist << checkImage(e_dir.filePath());
        }
    }

    static QStringList sList;

    for (const QByteArray &i : QImageReader::supportedImageFormats())
        sList << "*." + QString::fromLatin1(i);

    dir.setNameFilters(sList);

    int dircount = static_cast<int>(dir.count());
    for (int i = 0; i < dircount; i++) {
        QString ImageName  = dir[i];
        if (checkFileType(path + QDir::separator() + ImageName)) {
            imagelist << path + QDir::separator() + ImageName;
        }
    }

    return imagelist;
}

const QSize getImageQSize(const QString &path)
{


    QSize tSize;
    QStringList rl = getAllMetaData(path).value("Dimension").split("x");
    if (rl.length() == 2) {
        tSize = QSize(QString(rl.first()).toInt(), QString(rl.last()).toInt());
    }

    return tSize;
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

}  // namespace image

}  //namespace utils
