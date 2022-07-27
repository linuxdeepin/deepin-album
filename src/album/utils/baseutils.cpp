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
#include "unionimage.h"
#include "imagedataservice.h"
#include "movieservice.h"
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
#include <QRegularExpression>

#include <DApplication>
#include <DDesktopServices>
#include <QImageReader>
#include <QMimeDatabase>

DWIDGET_USE_NAMESPACE

namespace utils {

namespace base {

const QString DATETIME_FORMAT_NORMAL = "yyyy.MM.dd";
const QString DATETIME_FORMAT_EXIF = "yyyy:MM:dd HH:mm:ss";

QString reorganizationStr(const QFont &font, const QString &fullStr, int maxWidth)
{
    QFontMetrics fontWidth(font); //字体信息
    int width = fontWidth.width(fullStr) + 10; //计算字符串宽度,+10提前进入省略，避免右边遮挡
    QString result;
    if (width > maxWidth) {
        result = fontWidth.elidedText(fullStr, Qt::ElideRight, maxWidth); //超过最大长度，右边搞成省略号
    } else {
        result = fullStr; //没超最大长度，完整显示字符串
    }
    return result;
}

QDateTime stringToDateTime(const QString &time)
{
    QDateTime dt = QDateTime::fromString(time, DATETIME_FORMAT_EXIF);
    if (! dt.isValid()) {
        dt = QDateTime::fromString(time, DATETIME_FORMAT_NORMAL);
    }
    return dt;
}

//这个东西主要用于兼容老版本数据库的时间表示
//first: 时间值 secend: 是否需要升级
std::pair<QDateTime, bool> analyzeDateTime(const QVariant &data)
{
    auto str = data.toString();
    QDateTime result = QDateTime::fromString(str, DATETIME_FORMAT_DATABASE);
    if (!result.isValid()) {
        result = stringToDateTime(str);
    }
    if (result.isValid()) {
        return std::make_pair(result, true);
    } else {
        return std::make_pair(data.toDateTime(), false);
    }
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
    return QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5).toHex();
}

QString hashByData(const QString &str)
{
    QFile file(str);
    QString  stHashValue;
    if (file.open(QIODevice::ReadOnly)) { //只读方式打开
        QCryptographicHash hash(QCryptographicHash::Md5);

        QByteArray buf = file.read(1 * 1024 * 1024); // 每次读取10M
        buf = buf.append(str.toUtf8());
        hash.addData(buf);  // 将数据添加到Hash中
        stHashValue.append(hash.result().toHex());
    }
    return stHashValue;
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
    QFileInfo temDir(filePath);
    //如果hash为空，制作新的hash
    if (dataHash.isEmpty()) {
        dataHash = hashByData(filePath);
    }

    return albumGlobal::CACHE_PATH + temDir.path() + "/" + dataHash + ".png";
}

bool checkMimeUrls(const QList<QUrl> &urls)
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

QUrl UrlInfo(QString path)
{
    QUrl url;
    // Just check if the path is an existing file.
    if (QFile::exists(path)) {
        url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
        return url;
    }

    const auto match = QRegularExpression(QStringLiteral(":(\\d+)(?::(\\d+))?:?$")).match(path);

    if (match.isValid()) {
        // cut away line/column specification from the path.
        path.chop(match.capturedLength());
    }

    // make relative paths absolute using the current working directory
    // prefer local file, if in doubt!
    url = QUrl::fromUserInput(path, QDir::currentPath(), QUrl::AssumeLocalFile);

    // in some cases, this will fail, e.g.
    // assume a local file and just convert it to an url.
    if (!url.isValid()) {
        // create absolute file path, we will e.g. pass this over dbus to other processes
        url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
    }
    return url;
}

bool isVideo(QString path)
{
    QFileInfo temDir(path);
    QString fileName = temDir.suffix().toLower(); //扩展名
    return m_videoFiletypes.contains(fileName);
}

QString makeVaultLocalPath(const QString &path, const QString &base)
{
    QString basePath = base;
    if (basePath.isEmpty()) {
        basePath = VAULT_DECRYPT_DIR_NAME;
    }
    return VAULT_BASE_PATH + QDir::separator() + basePath + (path.startsWith('/') ? "" : "/") + path;
}

bool isVaultFile(const QString &path)
{
    bool bVaultFile = false;
    QString rootPath = makeVaultLocalPath("", "");
    if (rootPath.back() == "/") {
        rootPath.chop(1);
    }

    if (path.contains(rootPath) && path.left(6) != "search") {
        bVaultFile = true;
    }

    return bVaultFile;
}

//生成回收站文件名的函数，从文管里面抄的
static QString getNotExistsTrashFileName(const QString &fileName)
{
    QByteArray name = fileName.toUtf8();

    int index = name.lastIndexOf('/');

    if (index >= 0)
        name = name.mid(index + 1);

    index = name.lastIndexOf('.');
    QByteArray suffix;

    if (index >= 0)
        suffix = name.mid(index);

    if (suffix.size() > 200)
        suffix = suffix.left(200);

    name.chop(suffix.size());
    name = name.left(200 - suffix.size());

    QString trashpath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.local/share/Trash";

    while (true) {
        QFileInfo info(trashpath + name + suffix);
        // QFile::exists ==> If the file is a symlink that points to a non-existing file, false is returned.
        if (!info.isSymLink() && !info.exists()) {
            break;
        }

        name = QCryptographicHash::hash(name, QCryptographicHash::Md5).toHex();
    }

    return QString::fromUtf8(name + suffix);
}
//如果删除失败,返回使用dtk接口删除
bool trashFile(const QString &file)
{
    QString trashPath;
    QString trashInfoPath;
    QString trashFilesPath;

    QString home = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    // There maby others location for trash like $HOME/.trash or
    // $XDG_DATA_HOME/Trash, but our stupid FileManager coder said we should
    // assume that the trash lcation is $HOME/.local/share/Trash,so...
    trashPath = home + "/.local/share/Trash";
    trashInfoPath = trashPath + "/info";
    trashFilesPath = trashPath + "/files";
    if (! QDir(trashFilesPath).exists()) {
        QDir().mkpath(trashFilesPath);
    }
    if (! QDir(trashInfoPath).exists()) {
        QDir().mkpath(trashInfoPath);
    }

    QFileInfo originalInfo(file);
    if (! originalInfo.exists()) {
        qWarning() << "File doesn't exists, can't move to trash";
        return DDesktopServices::trash(file);
    }
    // Info for restore
    QString infoStr;
    infoStr += "[Trash Info]\nPath=";
    infoStr += QString(originalInfo.absoluteFilePath().toUtf8().toPercentEncoding("/"));
    infoStr += "\nDeletionDate=";
    infoStr += QDateTime::currentDateTime().toString("yyyy-MM-ddThh:mm:ss.zzzZ");
    infoStr += "\n";

    QString trashname = getNotExistsTrashFileName(originalInfo.fileName());
    QString infopath = trashInfoPath + "/" + trashname + ".trashinfo";
    QString filepath = trashFilesPath + "/" + trashname;
    int nr = 1;
    while (QFileInfo(infopath).exists() || QFileInfo(filepath).exists()) {
        nr++;
        trashname = originalInfo.baseName() + "." + QString::number(nr);
        if (!originalInfo.completeSuffix().isEmpty()) {
            trashname += QString(".") + originalInfo.completeSuffix();
        }
        infopath = trashInfoPath + "/" + trashname + ".trashinfo";
        filepath = trashFilesPath + "/" + trashname;
    }
    QFile infoFile(infopath);
    if (infoFile.open(QIODevice::WriteOnly)) {
        infoFile.write(infoStr.toUtf8());
        infoFile.close();

        if (!QDir().rename(originalInfo.absoluteFilePath(), filepath)) {
            qWarning() << "move to trash failed!";
            return DDesktopServices::trash(file);
        }
    } else {
        qDebug() << "Move to trash failed! Could not write *.trashinfo!";
        return DDesktopServices::trash(file);
    }
    return true;
}

QString getDeleteFullPath(const QString &hash, const QString &fileName)
{
    //防止文件过长,采用只用hash的名称;
    return albumGlobal::DELETE_PATH + "/" + hash + "." + QFileInfo(fileName).suffix();
}

//产品需求：需要支持同步文件拷贝，以实时监控拷贝进度
bool syncCopy(const QString &srcFileName, const QString &dstFileName)
{
    QFile src(srcFileName);
    QFile dst(dstFileName);

    src.open(QIODevice::ReadOnly);
    dst.open(QIODevice::WriteOnly);

    //0.预分配空间
    auto fileSize = src.size();
    if (!dst.resize(fileSize)) { //预分配空间失败
        dst.close();
        dst.remove();
        return false;
    }

    //1.执行拷贝
    dst.seek(0);
    while (1) {
        auto data = src.read(4 * 1024 * 1024);
        if (data.isEmpty()) { //没有更多的数据
            break;
        }

        dst.write(data);

        //等待数据写入，这是和QFile::copy的区别
        dst.waitForBytesWritten(30000);
    }

    return true;
}

//可重入版本的getMounts
QList<QExplicitlySharedDataPointer<DGioMount>> getMounts_safe()
{
    static QMutex mutex;
    mutex.lock();
    auto result = DGioVolumeManager::getMounts();
    mutex.unlock();
    return result;
}

QImage cheatScaled(const QImage &srcImg, int size, int side)
{
    QImage tImg;
    if (side == 0) {//w
        tImg = srcImg.scaledToWidth(size * 4, Qt::FastTransformation);
        tImg = srcImg.scaledToWidth(size, Qt::SmoothTransformation);
    } else {//h
        tImg = srcImg.scaledToHeight(size * 4, Qt::FastTransformation);
        tImg = srcImg.scaledToHeight(size, Qt::SmoothTransformation);
    }
    return tImg;
}

QImage getThumbnailFromImage(const QImage &srcImg, int size)
{
    QImage tImg(srcImg);

    if (!tImg.isNull() && 0 != tImg.height() && 0 != tImg.width() && (tImg.height() / tImg.width()) < 10 && (tImg.width() / tImg.height()) < 10) {
        bool cache_exist = false;
        if (tImg.height() != size && tImg.width() != size) {
            if (tImg.height() >= tImg.width()) {
                cache_exist = true;
                tImg = cheatScaled(tImg, size, 0);
            } else if (tImg.height() <= tImg.width()) {
                cache_exist = true;
                tImg = cheatScaled(tImg, size, 1);
            }
        }
        if (!cache_exist) {
            if ((static_cast<float>(tImg.height()) / (static_cast<float>(tImg.width()))) > 3) {
                tImg = cheatScaled(tImg, size, 0);
            } else {
                tImg = cheatScaled(tImg, size, 1);
            }
        }
    }
    if (!tImg.isNull()) {
        int width = tImg.width();
        int height = tImg.height();
        if (abs((width - height) * 10 / width) >= 1) {
            QRect rect = tImg.rect();
            int x = rect.x() + width / 2;
            int y = rect.y() + height / 2;
            if (width > height) {
                x = x - height / 2;
                y = 0;
                tImg = tImg.copy(x, y, height, height);
            } else {
                y = y - width / 2;
                x = 0;
                tImg = tImg.copy(x, y, width, width);
            }
        }
    }

    return tImg;
}

void multiLoadImage_helper(const QString &path)
{
    QString srcPath = path;
    if (!QFileInfo(srcPath).exists()) {
        return;
    }
    using namespace UnionImage_NameSpace;
    QImage tImg;
    QString errMsg;
    //读图
    if (utils::base::isVideo(srcPath)) {
        tImg = MovieService::instance()->getMovieCover(QUrl::fromLocalFile(srcPath));

        //获取视频信息 demo
        MovieInfo mi = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(srcPath));
        ImageDataService::instance()->addMovieDurationStr(srcPath, mi.duration);
    } else {
        if (!loadStaticImageFromFile(srcPath, tImg, errMsg)) {
            qDebug() << errMsg;
            ImageDataService::instance()->addImage(srcPath, tImg);
            return;
        }
    }
    //裁切
    tImg = getThumbnailFromImage(tImg, 200);

    ImageDataService::instance()->addImage(srcPath, tImg);
}

//多线程预加载图片
//启用条件：数据库无图片，且后台没在做自动导入
QFuture<void> multiLoadImage(const QStringList &paths)
{
    QFuture<void> watcher = QtConcurrent::map(paths, [](const QString & path) {
        multiLoadImage_helper(path);
    });
    return watcher;
}

}  // namespace base

}  // namespace utils
