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
#include "baseutils.h"
#include "imageutils.h"
#include "application.h"
#include <stdio.h>
#include <fcntl.h>
#include <fstream>
#include <linux/fs.h>
#include <QApplication>
#include <QClipboard>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFontMetrics>
#include <QFileInfo>
#include <QImage>
#include <QMimeData>
#include <QProcess>
#include <QUrl>
#include <QUrlQuery>
#include <QDebug>
#include <QTextStream>
#include <QtMath>

#include <DApplication>
#include <DDesktopServices>
#include <QImageReader>
#include <QMimeDatabase>

#include "player_engine.h"

DWIDGET_USE_NAMESPACE

QStringList VideoSupportTypeList;
dmr::PlaylistModel *g_playlistModel = nullptr;

namespace utils {

namespace base {

const QString DATETIME_FORMAT_NORMAL = "yyyy.MM.dd";
const QString DATETIME_FORMAT_EXIF = "yyyy:MM:dd HH:mm:ss";

int stringHeight(const QFont &f, const QString &str)
{
    QFontMetrics fm(f);
    return fm.boundingRect(str).height();
}

QDateTime stringToDateTime(const QString &time)
{
    QDateTime dt = QDateTime::fromString(time, DATETIME_FORMAT_EXIF);
    if (! dt.isValid()) {
        dt = QDateTime::fromString(time, DATETIME_FORMAT_NORMAL);
    }
    return dt;
}

void showInFileManager(const QString &path)
{
    if (path.isEmpty() || !QFile::exists(path)) {
        return;
    }
    QString m_Path = static_cast<QString>(path);

    QStringList spc {"#", "&", "@", "!", "?"};
    for (QString c : spc) {
        m_Path.replace(c,  QUrl::toPercentEncoding(c));
    }
    QUrl url = QUrl::fromUserInput(/*"\"" + */m_Path/* + "\""*/);
    url.setPath(m_Path, QUrl::TolerantMode);
    Dtk::Widget::DDesktopServices::showFileItem(url);
}

//void copyOneImageToClipboard(const QString &path)
//{
//    QImage img(path);
//    if (img.isNull())
//        return;
////    Q_ASSERT(!img.isNull());
//    QClipboard *cb = QApplication::clipboard();
//    cb->setImage(img, QClipboard::Clipboard);
//}

void copyImageToClipboard(const QStringList &paths)
{
    //  Get clipboard
    QClipboard *cb = qApp->clipboard();

    // Ownership of the new data is transferred to the clipboard.
    QMimeData *newMimeData = new QMimeData();
    QByteArray gnomeFormat = QByteArray("copy\n");
    QString text;
    QList<QUrl> dataUrls;
    for (QString path : paths) {
        if (!path.isEmpty())
            text += path + '\n';
        dataUrls << QUrl::fromLocalFile(path);
        gnomeFormat.append(QUrl::fromLocalFile(path).toEncoded()).append("\n");
    }

    newMimeData->setText(text.endsWith('\n') ? text.left(text.length() - 1) : text);
    newMimeData->setUrls(dataUrls);
    gnomeFormat.remove(gnomeFormat.length() - 1, 1);
    newMimeData->setData("x-special/gnome-copied-files", gnomeFormat);

    // Copy Image Date
//    QImage img(paths.first());
//    Q_ASSERT(!img.isNull());
//    newMimeData->setImageData(img);

    // Set the mimedata
    cb->setMimeData(newMimeData, QClipboard::Clipboard);
}

QString getFileContent(const QString &file)
{
    QFile f(file);
    QString fileContent = "";
    if (f.open(QFile::ReadOnly)) {
        fileContent = QLatin1String(f.readAll());
        f.close();
    }
    return fileContent;
}

QString SpliteText(const QString &text, const QFont &font, int nLabelSize)
{
//LMH0424，之前是递归，现在改了算法，判断换行
    QFontMetrics fm(font);
    double dobuleTextSize = fm.horizontalAdvance(text);
    double dobuleLabelSize = nLabelSize;
    if (dobuleTextSize > dobuleLabelSize && dobuleLabelSize > 0 && dobuleTextSize < 10000) {
        double splitCount = dobuleTextSize / dobuleLabelSize;
        int nCount = int(splitCount + 1);
        QString textSplite;
        QString textTotal = text;
        for (int index = 0; index < nCount; ++index) {
            int nPos = 0;
            long nOffset = 0;
            for (int i = 0; i < text.size(); i++) {
                nOffset += fm.width(text.at(i));
                if (nOffset >= nLabelSize) {
                    nPos = i;
                    break;
                }
            }
            nPos = (nPos - 1 < 0) ? 0 : nPos - 1;
            QString qstrLeftData;
            if (nCount - 1 == index) {
                qstrLeftData = textTotal;
                textSplite += qstrLeftData;
            } else {
                qstrLeftData = textTotal.left(nPos);
                textSplite += qstrLeftData + "\n";
            }
            textTotal = textTotal.mid(nPos);
        }
        return textSplite;
    } else {
        return text;
    }
}

QString hashByString(const QString &str)
{
    return QString(QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5).toHex());
}

QString hashByData(const QString &str)
{
    QFile file(str);
    QString  stHashValue;
    if (file.open(QIODevice::ReadOnly)) { //只读方式打开
        QCryptographicHash hash(QCryptographicHash::Md5);

        QByteArray buf = file.read(10 * 1024 * 1024); // 每次读取10M
        buf = buf.append(str.toUtf8());
        hash.addData(buf);  // 将数据添加到Hash中
        stHashValue.append(hash.result().toHex());
    }
    return stHashValue;
}

bool checkMimeData(const QMimeData *mimeData)
{
    if (1 > mimeData->urls().size()) {
        return false;
    }
    QList<QUrl> urlList = mimeData->urls();
    using namespace utils::image;
    for (QUrl url : urlList) {
        const QString path = url.toLocalFile();
        QFileInfo fileinfo(path);
        if (fileinfo.isDir()) {
            auto finfos =  getImagesAndVideoInfo(path, false);
            for (auto finfo : finfos) {
                if (imageSupportRead(finfo.absoluteFilePath()) || isVideo(finfo.absoluteFilePath())) {
                    QFileInfo info(finfo.absoluteFilePath());
                    QMimeDatabase db;
                    QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
                    QMimeType mt1 = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchExtension);
                    QString str = info.suffix().toLower();

                    if (str.isEmpty()) {
                        if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng")) {
                            if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
                                return true;
                            } else if (str.isEmpty()) {
                                return true;
                            }
                        }
                        if (mt.name().startsWith("video/") || mt.name().startsWith("application/x-matroska")) {
                            return true;
                        }
                    } else {
                        if (mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
                            if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
                                return true;
                            }
                        }
                        if (mt1.name().startsWith("video/") || mt1.name().startsWith("application/x-matroska")) {
                            return true;
                        }
                    }
                }
            }
        } else if (imageSupportRead(path) || isVideo(path)) {
//            paths << path;
            QFileInfo info(path);
            QMimeDatabase db;
            QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
            QMimeType mt1 = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchExtension);
            QString str = info.suffix().toLower();
            if (str.isEmpty()) {
                if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng")) {
                    if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
                        return true;
                    } else if (str.isEmpty()) {
                        return true;
                    }
                }
                if (mt.name().startsWith("video/") || mt.name().startsWith("application/x-matroska")) {
                    return true;
                }
            } else {
                if (mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
                    if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
                        return true;
                    }
                }
                if (mt1.name().startsWith("video/") || mt1.name().startsWith("application/x-matroska")) {
                    return true;
                }
            }
            return false;
        }
    }
    return false;
}

QPixmap renderSVG(const QString &filePath, const QSize &size)
{
    QImageReader reader;
    QPixmap pixmap;

    reader.setFileName(filePath);

    if (reader.canRead() && reader.imageCount() > 0) {
        const qreal ratio = dApp->getDAppNew()->devicePixelRatio();
        reader.setScaledSize(size * ratio);
        pixmap = QPixmap::fromImage(reader.read());
        pixmap.setDevicePixelRatio(ratio);
    } else {
        pixmap.load(filePath);
    }

    return pixmap;
}

QString mkMutiDir(const QString &path)   //创建多级目录
{
    QDir dir(path);
    if (dir.exists(path)) {
        return path;
    }
    QString parentDir = mkMutiDir(path.mid(0, path.lastIndexOf('/')));
    QString dirname = path.mid(path.lastIndexOf('/') + 1);
    QDir parentPath(parentDir);
    if (!dirname.isEmpty())
        parentPath.mkpath(dirname);
    return parentDir + "/" + dirname;
}
//根据源文件路径生产缩略图路径
QString filePathToThumbnailPath(const QString &filePath, QString dataHash)
{
    QString thumbnailPath = albumGlobal::CACHE_PATH + filePath;

    QFileInfo temDir(filePath);
    //如果hash为空，制作新的hash
    if (dataHash.isEmpty()) {
        dataHash = hashByData(filePath);
    }

    thumbnailPath = albumGlobal::CACHE_PATH + temDir.path() + "/" + dataHash + ".png";
    return thumbnailPath;
}

bool isVideo(QString path)
{
    QFileInfo temDir(path);
    QString fileName = "*." + temDir.suffix().toLower(); //扩展名
    if(VideoSupportTypeList.isEmpty())
    {
        dmr::PlayerEngine *e = new dmr::PlayerEngine(nullptr);
        VideoSupportTypeList = e->video_filetypes;
        delete e;
        e = nullptr;
    }
    return VideoSupportTypeList.contains(fileName);
}

dmr::MovieInfo getMovieInfo(const QString &path)
{
    if (g_playlistModel == nullptr) {
        g_playlistModel = new dmr::PlaylistModel(nullptr);
    }
    bool is = false;
    //获取视频信息 demo
    dmr::MovieInfo mi = g_playlistModel->getMovieInfo(QUrl::fromLocalFile(path), &is);
    return mi;
}

bool checkMimeUrls(const QList<QUrl> urls)
{
    if (1 > urls.size()) {
        return false;
    }
    QList<QUrl> urlList = urls;
    using namespace utils::image;
    for (QUrl url : urlList) {
        const QString path = url.toLocalFile();
        QFileInfo fileinfo(path);
        if (fileinfo.isDir()) {
            auto finfos =  getImagesAndVideoInfo(path, false);
            for (auto finfo : finfos) {
                if (imageSupportRead(finfo.absoluteFilePath()) || isVideo(finfo.absoluteFilePath())) {
                    return true;
                }
            }
        } else if (imageSupportRead(path) || isVideo(path)) {
            return true;
        }
    }
    return false;
}

}  // namespace base

}  // namespace utils
