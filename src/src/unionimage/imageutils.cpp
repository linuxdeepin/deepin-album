// SPDX-FileCopyrightText: 2020-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "baseutils.h"
#include "ddlog.h"
#include "imageutils.h"
#include "unionimage.h"
#include <fstream>

#include <QBuffer>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QMimeDatabase>
#include <QMutexLocker>
#include <QPixmapCache>
#include <QProcess>
#include <QReadWriteLock>
#include <QUrl>
#include <QApplication>

namespace Libutils {

namespace image {

const QImage scaleImage(const QString &path, const QSize &size)
{
    qCDebug(albumLog) << "Scaling image:" << path << "to size:" << size;
    if (!imageSupportRead(path)) {
        qCWarning(albumLog) << "Image format not supported for scaling:" << path;
        return QImage();
    }
    /*lmh0724使用USE_UNIONIMAGE*/

    QImage tImg(size, QImage::Format_ARGB32);
    QString errMsg;
    QSize realSize;
//    if (!UnionImage_NameSpace::loadStaticImageFromFile(path, tImg, realSize, errMsg)) {
//        qCDebug(albumLog) << errMsg;
//    }
    if (!LibUnionImage_NameSpace::loadStaticImageFromFile(path, tImg, errMsg)) {
        qCWarning(albumLog) << "Failed to load image for scaling:" << path << "Error:" << errMsg;
    }
    if (tImg.size() != size) { //调用加速接口失败，主动进行缩放
        qCDebug(albumLog) << "Scaling image from" << tImg.size() << "to" << size;
        tImg = tImg.scaled(size);
    }
    return tImg;
}

const QDateTime getCreateDateTime(const QString &path)
{
    qCDebug(albumLog) << "Getting creation date time for:" << path;
    QDateTime dt; /*= libexif::getCreateDateTime(path);*/

    // fallback to metadata.
    if (!dt.isValid()) {
        QString s;
        s = getAllMetaData(path).value("DateTimeOriginal");
        if (s.isEmpty()) {
            s = getAllMetaData(path).value("DateTimeDigitized");
        }
        if (s.isEmpty()) {
            qCDebug(albumLog) << "No metadata found, using current time";
            s = QDateTime::currentDateTime().toString();
        }
        dt = QDateTime::fromString(s, "yyyy.MM.dd HH:mm:ss");
    }

    // fallback to file create time.
    if (!dt.isValid()) {
        qCDebug(albumLog) << "No valid metadata, using file birth time";
        QFileInfo finfo(path);
        dt = finfo.birthTime();
    }

    // fallback to today.
    if (!dt.isValid()) {
        qCDebug(albumLog) << "No valid birth time, using current time";
        dt = QDateTime::currentDateTime();
    }

    qCDebug(albumLog) << "Final creation date time:" << dt;
    return dt;
}

bool imageSupportRead(const QString &path)
{
    qCDebug(albumLog) << "Checking if image format is supported for reading:" << path;
    /*lmh0724使用USE_UNIONIMAGE*/
    //修正论坛上提出的格式判断错误，应该采用真实格式
    //20210220真实格式来做判断
    QMap<QString, QString> dataMap = getAllMetaData(path);
    const QString suffix = dataMap.value("FileFormat");
    QStringList errorList;
    errorList << "X3F";
    if (errorList.indexOf(suffix.toUpper()) != -1) {
        qCDebug(albumLog) << "Format" << suffix << "is in error list, not supported";
        return false;
    }
    return LibUnionImage_NameSpace::unionImageSupportFormat().contains(suffix.toUpper());
}

bool imageSupportSave(const QString &path)
{
    qCDebug(albumLog) << "Checking if image format is supported for saving:" << path;
    /*lmh0724使用USE_UNIONIMAGE*/
    return LibUnionImage_NameSpace::canSave(path);
}

bool rotate(const QString &path, int degree)
{
    qCDebug(albumLog) << "Rotating image:" << path << "by" << degree << "degrees";
    /*lmh0724使用USE_UNIONIMAGE*/
    QString erroMsg;
    return LibUnionImage_NameSpace::rotateImageFIle(degree, path, erroMsg);
}

/*!
 * \brief cutSquareImage
 * Cut square image
 * \param pixmap
 * \return
 */
const QPixmap cutSquareImage(const QPixmap &pixmap)
{
    qCDebug(albumLog) << "Cutting square image from pixmap of size:" << pixmap.size();
    return Libutils::image::cutSquareImage(pixmap, pixmap.size());
}

/*!
 * \brief cutSquareImage
 * Scale and cut a square image
 * \param pixmap
 * \param size
 * \return
 */
const QPixmap cutSquareImage(const QPixmap &pixmap, const QSize &size)
{
    qCDebug(albumLog) << "Cutting square image from pixmap of size:" << pixmap.size() << "to target size:" << size;
    const qreal ratio = qApp->devicePixelRatio();
    QImage img = pixmap.toImage().scaled(size * ratio,
                                         Qt::KeepAspectRatioByExpanding,
                                         Qt::SmoothTransformation);
    const QSize s(size * ratio);
    const QRect r(0, 0, s.width(), s.height());

    img = img.copy(QRect(img.rect().center() - r.center(), s));
    img.setDevicePixelRatio(ratio);

    return QPixmap::fromImage(img);
}

/*!
 * \brief getImagesInfo
        types<< ".BMP";
        types<< ".GIF";
        types<< ".JPG";
        types<< ".JPEG";
        types<< ".PNG";
        types<< ".PBM";
        types<< ".PGM";
        types<< ".PPM";
        types<< ".XBM";
        types<< ".XPM";
        types<< ".SVG";

        types<< ".DDS";
        types<< ".ICNS";
        types<< ".JP2";
        types<< ".MNG";
        types<< ".TGA";
        types<< ".TIFF";
        types<< ".WBMP";
        types<< ".WEBP";
 * \param dir
 * \param recursive
 * \return
 */
const QFileInfoList getImagesInfo(const QString &dir, bool recursive)
{
    qCDebug(albumLog) << "Getting image info from directory:" << dir << "recursive:" << recursive;
    QFileInfoList infos;

    if (! recursive) {
        auto nsl = QDir(dir).entryInfoList(QDir::Files);
        for (QFileInfo info : nsl) {
            if (imageSupportRead(info.absoluteFilePath())) {
                infos << info;
            }
        }
        qCDebug(albumLog) << "Found" << infos.size() << "images in directory";
        return infos;
    }

    QDirIterator dirIterator(dir,
                             QDir::Files,
                             QDirIterator::Subdirectories);
    while (dirIterator.hasNext()) {
        dirIterator.next();
        if (imageSupportRead(dirIterator.fileInfo().absoluteFilePath())) {
            infos << dirIterator.fileInfo();
#include "imageutils.h"
        }
    }

    qCDebug(albumLog) << "Found" << infos.size() << "images recursively";
    return infos;
}

int getOrientation(const QString &path)
{
    return LibUnionImage_NameSpace::getOrientation(path);
}

/*!
 * \brief getRotatedImage
 * Rotate image base on the exif orientation
 * \param path
 * \return
 */
const QImage getRotatedImage(const QString &path)
{
    qCDebug(albumLog) << "Getting rotated image for:" << path;
    QImage tImg;
    /*lmh0724使用USE_UNIONIMAGE*/
    QString errMsg;
    QSize realSize;
    if (!LibUnionImage_NameSpace::loadStaticImageFromFile(path, tImg, errMsg)) {
        qCWarning(albumLog) << "Failed to load image for rotation:" << path << "Error:" << errMsg;
    }
    return tImg;
}

const QMap<QString, QString> getAllMetaData(const QString &path)
{
    qCDebug(albumLog) << "Getting all metadata for:" << path;
    /*lmh0724使用USE_UNIONIMAGE*/
    return LibUnionImage_NameSpace::getAllMetaData(path);
}

const QPixmap cachePixmap(const QString &path)
{
    qCDebug(albumLog) << "Getting cached pixmap for:" << path;
    QPixmap pp;
    if (! QPixmapCache::find(path, &pp)) {
        qCDebug(albumLog) << "Pixmap not found in cache, loading from file";
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
QMap<QString, QString> thumbnailAttribute(const QUrl  &url)
{
    qCDebug(albumLog) << "Getting thumbnail attributes for URL:" << url;
    QMap<QString, QString> set;

    if (url.isLocalFile()) {
        const QString path = url.path();
        QFileInfo info(path);
        set.insert("Thumb::Mimetype", QMimeDatabase().mimeTypeForFile(path).name());
        set.insert("Thumb::Size", QString::number(info.size()));
        set.insert("Thumb::URI", url.toString());
        set.insert("Thumb::MTime", QString::number(info.lastModified().toSecsSinceEpoch()));
        set.insert("Software", "Deepin Image Viewer");

        QImageReader reader(path);
        if (reader.canRead()) {
            set.insert("Thumb::Image::Width", QString::number(reader.size().width()));
            set.insert("Thumb::Image::Height", QString::number(reader.size().height()));
        }
        qCDebug(albumLog) << "Generated thumbnail attributes for local file:" << path;
    } else {
        //TODO for other's scheme
    }

    return set;
}

const QString thumbnailCachePath()
{
    qCDebug(albumLog) << "Getting thumbnail cache path";
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
    const QString thumbCacheP = cacheP + "/thumbnails";
    QDir().mkpath(thumbCacheP + "/normal");
    QDir().mkpath(thumbCacheP + "/large");
    QDir().mkpath(thumbCacheP + "/fail");

    qCDebug(albumLog) << "Thumbnail cache path:" << thumbCacheP;
    return thumbCacheP;
}

QMutex mutex;
const QPixmap getThumbnail(const QString &path, bool cacheOnly)
{
    QMutexLocker locker(&mutex);
    qCDebug(albumLog) << "Getting thumbnail for:" << path << "cacheOnly:" << cacheOnly;
    //优先读取自身缓存的图片
//    if (dApp->m_imagemap.value(path).isNull()) {
//        return dApp->m_imagemap.value(path);
//    }
    const QString cacheP = thumbnailCachePath();
    const QUrl url = QUrl::fromLocalFile(path);
    const QString md5s = toMd5(url.toString(QUrl::FullyEncoded).toLocal8Bit());
    const QString encodePath = cacheP + "/large/" + md5s + ".png";
    const QString failEncodePath = cacheP + "/fail/" + md5s + ".png";
    if (QFileInfo(encodePath).exists()) {
        qCDebug(albumLog) << "Found existing thumbnail at:" << encodePath;
        return QPixmap(encodePath);
    }
    /*lmh0724使用USE_UNIONIMAGE*/
    else if (QFileInfo(failEncodePath).exists()) {
        qCDebug(albumLog) << "Found failed thumbnail, won't regenerate:" << failEncodePath;
        return QPixmap();
    } else {
        // Try to generate thumbnail and load it later
        if (! cacheOnly && generateThumbnail(path)) {
            qCDebug(albumLog) << "Generated new thumbnail at:" << encodePath;
            return QPixmap(encodePath);
        } else {
            qCDebug(albumLog) << "No thumbnail available";
            return QPixmap();
        }
    }
}

/*!
 * \brief generateThumbnail
 * Generate and save thumbnail for specific size
 * \return
 */
bool generateThumbnail(const QString &path)
{
    qCDebug(albumLog) << "Generating thumbnail for:" << path;
    const QUrl url = QUrl::fromLocalFile(path);
    const QString md5 = toMd5(url.toString(QUrl::FullyEncoded).toLocal8Bit());
    const auto attributes = thumbnailAttribute(url);
    const QString cacheP = thumbnailCachePath();

    // Large thumbnail
    QImage lImg = scaleImage(path,
                             QSize(THUMBNAIL_MAX_SIZE, THUMBNAIL_MAX_SIZE));

    // Normal thumbnail
    QImage nImg = lImg.scaled(
                      QSize(THUMBNAIL_NORMAL_SIZE, THUMBNAIL_NORMAL_SIZE)
                      , Qt::KeepAspectRatio
                      , Qt::SmoothTransformation);

    // Create filed thumbnail
    if (lImg.isNull() || nImg.isNull()) {
        qCWarning(albumLog) << "Failed to generate thumbnail images for:" << path;
        const QString failedP = cacheP + "/fail/" + md5 + ".png";
        QImage img(1, 1, QImage::Format_ARGB32_Premultiplied);
        const auto keys = attributes.keys();
        for (QString key : keys) {
            img.setText(key, attributes[key]);
        }

        bool saved = img.save(failedP,  "png");
        qCDebug(albumLog) << "Saved failed thumbnail:" << saved << "at:" << failedP;
        return false;
    } else {
        for (QString key : attributes.keys()) {
            lImg.setText(key, attributes[key]);
            nImg.setText(key, attributes[key]);
        }
        const QString largeP = cacheP + "/large/" + md5 + ".png";
        const QString normalP = cacheP + "/normal/" + md5 + ".png";
        bool success = lImg.save(largeP, "png", 50) && nImg.save(normalP, "png", 50);
        qCDebug(albumLog) << "Saved thumbnail images:" << success;
        return success;
    }
}

const QString thumbnailPath(const QString &path, ThumbnailType type)
{
    qCDebug(albumLog) << "Getting thumbnail path for:" << path << "type:" << type;
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
    default:
        break;
    }
    qCDebug(albumLog) << "Thumbnail path:" << tp;
    return tp;
}

void removeThumbnail(const QString &path)
{
    qCDebug(albumLog) << "Removing thumbnails for:" << path;
    QFile(thumbnailPath(path, ThumbLarge)).remove();
    QFile(thumbnailPath(path, ThumbNormal)).remove();
    QFile(thumbnailPath(path, ThumbFail)).remove();
}

bool thumbnailExist(const QString &path, ThumbnailType type)
{
    qCDebug(albumLog) << "Checking if thumbnail exists for:" << path << "type:" << type;
    if (QFileInfo(thumbnailPath(path, type)).exists()
//            || QFileInfo(thumbnailPath(path, ThumbNormal)).exists()
//            || QFileInfo(thumbnailPath(path, ThumbFail)).exists()
       ) {
        qCDebug(albumLog) << "Thumbnail exists:" << true;
        return true;
    } else {
        qCDebug(albumLog) << "Thumbnail exists:" << false;
        return false;
    }
}
/*
static QStringList fromByteArrayList(const QByteArrayList &list)
{
    QStringList sList;

    for (const QByteArray &i : list)
        sList << "*." + QString::fromLatin1(i);

    // extern image format
    sList << "*.cr2"
          << "*.dng"
          << "*.nef"
          << "*.mef"
          << "*.raf"
          << "*.raw"
          << "*.orf"
          << "*.mrw"
          << "*.jpe"
          << "*.xbm";

    return sList;
}
*/
QStringList supportedImageFormats()
{
    qCDebug(albumLog) << "Getting supported image formats";
    /*lmh0724使用USE_UNIONIMAGE*/
    QStringList list ;
    for (auto str : LibUnionImage_NameSpace::unionImageSupportFormat()) {
        str = "*." + str;
        list += str;
    }
    qCDebug(albumLog) << "Found" << list.size() << "supported formats";
    return list;

}




bool imageSupportWallPaper(const QString &path)
{
    qCDebug(albumLog) << "Checking if image supports wallpaper:" << path;
    bool iRet = false;
    QStringList listsupportWallPaper;
    listsupportWallPaper << "bmp"
//                         << "cod"
                         << "png"
                         << "gif"
                         << "ief"
                         << "jpe"
                         << "jpeg"
                         << "jpg"
                         << "jfif"
//                         << "svg"
                         << "tif"
                         << "tiff"
//                         << "ras"
//                         << "cmx"
//                         << "ico"
//                         << "pnm"
//                         << "pbm"
//                         << "pgm"
//                         << "ppm"
//                         << "rgb"
//                         << "xbm"
//                         << "xpm"
//                         << "xwd"
                         ;
    //
    QImageReader reader(path);
    if (reader.imageCount() > 0) {

        //2020/11/12 bug54279
        if (listsupportWallPaper.contains(reader.format().toLower()) && listsupportWallPaper.contains(QFileInfo(path).suffix().toLower())) {
            iRet = true;
        }
        //20201012 lmh ico不支持设置壁纸
//        else {
//            const QString suffix = QFileInfo(path).suffix();
//            if(suffix=="ico")
//            {
//                iRet=true;
//            }
//        }
    }

    qCDebug(albumLog) << "Wallpaper support status:" << iRet;
    return iRet;
}

//bool suffixisImage(const QString &path)
//{
//#ifdef USE_UNIONIMAGE
//    return UnionImage_NameSpace::suffixisImage(path);
//#else
//    bool iRet = false;
//    QFileInfo info(path);
//    QMimeDatabase db;
//    QMimeType mt = db.mimeTypeForFile(path, QMimeDatabase::MatchContent);
//    QMimeType mt1 = db.mimeTypeForFile(path, QMimeDatabase::MatchExtension);
//    QString str = info.suffix();
//    // if (!m_nosupportformat.contains(str, Qt::CaseSensitive)) {
//    if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng") ||
//            mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
//        iRet = true;
//    }
//    return iRet;
//#endif
//}

QString makeVaultLocalPath(const QString &path, const QString &base)
{
    qCDebug(albumLog) << "Making vault local path for:" << path << "base:" << base;
    QString basePath = base;
    if (basePath.isEmpty()) {
        basePath = VAULT_DECRYPT_DIR_NAME;
    }
    return VAULT_BASE_PATH + QDir::separator() + basePath + (path.startsWith('/') ? "" : "/") + path;
}

bool isVaultFile(const QString &path)
{
    qCDebug(albumLog) << "Checking if file is in vault:" << path;
    bool bVaultFile = false;
    QString rootPath = makeVaultLocalPath("", "");
    if (rootPath.back() == QChar('/')) {
        rootPath.chop(1);
    }

    if (path.contains(rootPath) && path.left(6) != "search") {
        bVaultFile = true;
    }

    qCDebug(albumLog) << "Vault file status:" << bVaultFile;
    return bVaultFile;

}
bool isCanRemove(const QString &path)
{
    qCDebug(albumLog) << "Checking if file can be removed:" << path;
    bool bRet = true;
    QString trashPath = QDir::homePath() + "/.local/share/Trash";
    //新增保险箱的判断,回收站判断
    if (isVaultFile(path) || path.contains(trashPath)) {
        bRet = false;
    }
    qCDebug(albumLog) << "File can be removed:" << bRet;
    return bRet;
}

void getAllFileInDir(const QDir &dir, QFileInfoList &result)
{
    qCDebug(albumLog) << "Getting all files in directory:" << dir.path();
    QDir root(dir);
    auto list = root.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    for (const auto &eachInfo : list) {
        if (eachInfo.isDir()) {
            getAllFileInDir(eachInfo.absoluteFilePath(), result);
        } else {
            result.push_back(eachInfo);
        }
    }
    qCDebug(albumLog) << "Found" << result.size() << "files in directory";
}

QFileInfoList getImagesAndVideoInfo(const QString &dir, bool recursive)
{
    qCDebug(albumLog) << "Getting images and video info from directory:" << dir << "recursive:" << recursive;
    QFileInfoList infos;

    if (! recursive) {
        QFileInfoList nsl;
        getAllFileInDir(dir, nsl);
        for (QFileInfo info : nsl) {
            if (imageSupportRead(info.absoluteFilePath()) ||
                    isVideo(info.absoluteFilePath())) {
                infos << info;
            }
        }
        qCDebug(albumLog) << "Found" << infos.size() << "media files in directory";
        return infos;
    }

    QDirIterator dirIterator(dir,
                             QDir::Files,
                             QDirIterator::Subdirectories);
    while (dirIterator.hasNext()) {
        dirIterator.next();
        if ( imageSupportRead(dirIterator.fileInfo().absoluteFilePath())
                || isVideo(dirIterator.fileInfo().absoluteFilePath())) {
            infos << dirIterator.fileInfo();
        }
    }

    qCDebug(albumLog) << "Found" << infos.size() << "media files recursively";
    return infos;
}

QPixmap getDamagePixmap(bool bLight)
{
    qCDebug(albumLog) << "Getting damage pixmap, light mode:" << bLight;
    static QPixmap pix_light, pix_dark;
    if (bLight) {
        if (pix_light.isNull())
            pix_light = Libutils::base::renderSVG(view::LIGHT_DAMAGEICON, QSize(150, 150));
        return pix_light;
    } else {
        if (pix_dark.isNull())
            pix_dark = Libutils::base::renderSVG(view::DARK_DAMAGEICON, QSize(150, 150));
        return pix_dark;
    }
}

bool isVideo(QString path)
{
    qCDebug(albumLog) << "Checking if file is video:" << path;
    QFileInfo temDir(path);
    QString fileName = temDir.suffix().toLower(); //扩展名

    // ts文件可能为翻译文件，使用QMimeDataBase::mimeTypeForFile做精确判断，判断是否为视频
    if (fileName == "ts") {
        bool isVideo = QMimeDatabase().mimeTypeForFile(path).name() == "video/mp2t";
        qCDebug(albumLog) << "TS file video status:" << isVideo;
        return isVideo;
    }

    return m_videoFiletypes.contains(fileName);
}

}  // namespace image

}  //namespace utils
