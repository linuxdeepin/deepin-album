// SPDX-FileCopyrightText: 2020 - 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "baseutils.h"
#include "imageutils.h"
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
#include <QImageReader>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QDesktopServices>

#include <DDesktopServices>

#include "unionimage.h"

namespace Libutils {

namespace base {

const QString DATETIME_FORMAT_NORMAL = "yyyy.MM.dd";
const QString DATETIME_FORMAT_EXIF = "yyyy:MM:dd HH:mm:ss";

QPixmap renderSVG(const QString &filePath, const QSize &size)
{
    /*lmh0724使用USE_UNIONIMAGE*/
    qDebug() << "Rendering SVG file:" << filePath << "with size:" << size;
    QImage tImg(size, QImage::Format_ARGB32);
    QString errMsg;
    QSize realSize;
    if (!LibUnionImage_NameSpace::loadStaticImageFromFile(filePath, tImg, errMsg)) {
        qWarning() << "Failed to load SVG file:" << filePath << "Error:" << errMsg;
    }
    QPixmap pixmap;
    pixmap = QPixmap::fromImage(tImg);

    return pixmap;
}

//QString sizeToHuman(const qlonglong bytes)
//{
//    qlonglong sb = 1024;
//    if (bytes < sb) {
//        return QString::number(bytes) + " B";
//    } else if (bytes < sb * sb) {
//        QString vs = QString::number((double)bytes / sb, 'f', 1);
//        if (qCeil(vs.toDouble()) == qFloor(vs.toDouble())) {
//            return QString::number((int)vs.toDouble()) + " KB";
//        } else {
//            return vs + " KB";
//        }
//    } else if (bytes < sb * sb * sb) {
//        QString vs = QString::number((double)bytes / sb / sb, 'f', 1);
//        if (qCeil(vs.toDouble()) == qFloor(vs.toDouble())) {
//            return QString::number((int)vs.toDouble()) + " MB";
//        } else {
//            return vs + " MB";
//        }
//    } else {
//        return QString::number(bytes);
//    }
//}

QString timeToString(const QDateTime &time, bool normalFormat)
{
    // qDebug() << "Converting time to string";
    QString result = normalFormat ? time.toString(DATETIME_FORMAT_NORMAL) : time.toString(DATETIME_FORMAT_EXIF);
    qDebug() << "Converting time:" << time << "to string format:" << (normalFormat ? "normal" : "exif") << "Result:" << result;
    return result;
}

int stringWidth(const QFont &f, const QString &str)
{
    // qDebug() << "Calculating string width";
    QFontMetrics fm(f);
    return fm.boundingRect(str).width();
}

int stringHeight(const QFont &f, const QString &str)
{
    // qDebug() << "Calculating string height";
    QFontMetrics fm(f);
    return fm.boundingRect(str).height();
}

QDateTime stringToDateTime(const QString &time)
{
    // qDebug() << "Converting string to date time";
    QDateTime dt = QDateTime::fromString(time, DATETIME_FORMAT_EXIF);
    if (!dt.isValid()) {
        qDebug() << "Failed to parse time with EXIF format, trying normal format:" << time;
        dt = QDateTime::fromString(time, DATETIME_FORMAT_NORMAL);
    }
    return dt;
}

void showInFileManager(const QString &path)
{
    // qDebug() << "Showing file manager for path:" << path;
    if (path.isEmpty() || !QFile::exists(path)) {
        qWarning() << "Invalid path for file manager:" << path;
        return;
    }
    qDebug() << "Opening file manager for path:" << path;
    QString m_Path = static_cast<QString>(path);

    QStringList spc {"#", "&", "@", "!", "?"};
    for (QString c : spc) {
        m_Path.replace(c,  QUrl::toPercentEncoding(c));
    }
    QUrl url = QUrl::fromUserInput(/*"\"" + */m_Path/* + "\""*/);
    url.setPath(m_Path, QUrl::TolerantMode);
    Dtk::Gui::DDesktopServices::showFileItem(url);
}

void copyImageToClipboard(const QStringList &paths)
{
    qDebug() << "Copying" << paths.size() << "images to clipboard";
    QClipboard *cb = qApp->clipboard();

    // Ownership of the new data is transferred to the clipboard.
    QMimeData *newMimeData = new QMimeData();

    // Copy old mimedata
//    const QMimeData* oldMimeData = cb->mimeData();
//    for ( const QString &f : oldMimeData->formats())
//        newMimeData->setData(f, oldMimeData->data(f));

    // Copy file (gnome)
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

    cb->setMimeData(newMimeData, QClipboard::Clipboard);
    qDebug() << "Successfully copied images to clipboard";
}

QString getFileContent(const QString &file)
{
    qDebug() << "Reading content from file:" << file;
    QFile f(file);
    QString fileContent = "";
    if (f.open(QFile::ReadOnly)) {
        fileContent = QLatin1String(f.readAll());
        f.close();
        qDebug() << "Successfully read" << fileContent.length() << "bytes from file";
    } else {
        qWarning() << "Failed to open file for reading:" << file;
    }
    return fileContent;
}

//bool writeTextFile(QString filePath, QString content)
//{
//    QFile file(filePath);
//    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
//        QTextStream in(&file);
//        in << content << endl;
//        file.close();
//        return true;
//    }

//    return false;
//}

QString getNotExistsTrashFileName(const QString &fileName)
{
    qDebug() << "Generating unique trash filename for:" << fileName;
    QByteArray name = fileName.toUtf8();

    int index = name.lastIndexOf('/');
    if (index >= 0)
        name = name.mid(index + 1);

    index = name.lastIndexOf('.');
    QByteArray suffix;
    if (index >= 0)
        suffix = name.mid(index);

    if (suffix.size() > 200) {
        qDebug() << "Truncating suffix from" << suffix.size() << "to 200 characters";
        suffix = suffix.left(200);
    }

    name.chop(suffix.size());
    name = name.left(200 - suffix.size());

    QString trashpath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.local/share/Trash";

    while (true) {
        QFileInfo info(trashpath + name + suffix);
        // QFile::exists ==> If the file is a symlink that points to a non-existing file, false is returned.
        if (!info.isSymLink() && !info.exists()) {
            break;
        }
        qDebug() << "Name collision detected, generating new hash for:" << name;
        name = QCryptographicHash::hash(name, QCryptographicHash::Md5).toHex();
    }

    return QString::fromUtf8(name + suffix);
}

bool trashFile(const QString &file)
{
#ifdef QT_GUI_LIB
    qDebug() << "Moving file to trash:" << file;
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
    
    if (!QDir(trashFilesPath).exists()) {
        qDebug() << "Creating trash files directory:" << trashFilesPath;
        QDir().mkpath(trashFilesPath);
    }
    if (!QDir(trashInfoPath).exists()) {
        qDebug() << "Creating trash info directory:" << trashInfoPath;
        QDir().mkpath(trashInfoPath);
    }

    QFileInfo originalInfo(file);
    if (!originalInfo.exists()) {
        qWarning() << "File doesn't exist, can't move to trash:" << file;
        return false;
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
        //转为hash值去替换,避免文件名长度超出限制
        QString hash = LibUnionImage_NameSpace::hashByString(trashname);
        trashname = hash + QString(".") + originalInfo.completeSuffix();
        infopath = trashInfoPath + "/" + trashname + ".trashinfo";
        filepath = trashFilesPath + "/" + trashname;
    }

    QFile infoFile(infopath);
    if (infoFile.open(QIODevice::WriteOnly)) {
        infoFile.write(infoStr.toUtf8());
        infoFile.close();

        if (!QDir().rename(originalInfo.absoluteFilePath(), filepath)) {
            qWarning() << "move to trash failed!";
            return false;
        }
        qDebug() << "Successfully moved file to trash:" << file;
    } else {
        qWarning() << "Failed to write trash info file:" << infopath;
        return false;
    }
    // Remove thumbnail
    Libutils::image::removeThumbnail(file);
    return true;
#else
    Q_UNUSED(file);
    qWarning() << "Trash in server-mode not supported";
    return false;
#endif
}

//bool trashFiles(const QStringList &files)
//{
//    bool v = true;
//    for (QString file : files) {
//        if (! trashFile(file))
//            v = false;
//    }

//    return v;
//}

///*!
// * \brief wrapStr
// * Split info string by Space
// * \param str
// * \param font
// * \param maxWidth
// * \return
// */
//QString wrapStr(const QString &str, const QFont &font, int maxWidth)
//{
//    QFontMetrics fm(font);
//    QString ns;
//    QString ss;
//    for (int i = 0; i < str.length(); i ++) {
//        if (/*str.at(i).isSpace()||*/ fm.boundingRect(ss).width() > maxWidth) {
//            ss = QString();
//            ns += "\n";
//        }
//        ns += str.at(i);
//        ss += str.at(i);
//    }
//    return ns;
////    return str;
//}


QString SpliteText(const QString &text, const QFont &font, int nLabelSize, bool bReturn)
{
    qDebug() << "Splitting text:" << text << "with font:" << font.toString() << "label size:" << nLabelSize;
    QFontMetrics fm(font);
    int nTextSize = fm.horizontalAdvance(text);
    if (nTextSize > nLabelSize) {
        int nPos = 0;
        long nOffset = 0;
        for (int i = 0; i < text.size(); i++) {
            nOffset += fm.horizontalAdvance(text.at(i));
            if (nOffset >= nLabelSize) {
                nPos = i;
                break;
            }
        }

        nPos = (nPos - 1 < 0) ? 0 : nPos - 1;

        QString qstrLeftData = text.left(nPos);
        QString qstrMidData = text.mid(nPos);
        if (bReturn) {
            qstrLeftData.replace(" ", "\n");
            qstrMidData.replace(" ", "\n");
            if (qstrLeftData != "")
                return qstrLeftData + SpliteText(qstrMidData, font, nLabelSize);
        } else {
            if (qstrLeftData != "")
                return qstrLeftData + "\n" + SpliteText(qstrMidData, font, nLabelSize);
        }
    }
    return text;
}


//QString symFilePath(const QString &path)
//{
//    QFileInfo fileInfo(path);
//    if (fileInfo.isSymLink()) {
//        return fileInfo.symLinkTarget();
//    } else {
//        return path;
//    }
//}

QString hash(const QString &str)
{
    // qDebug() << "Hashing string:" << str;
    return QString(QCryptographicHash::hash(str.toUtf8(),
                                            QCryptographicHash::Md5).toHex());
}

QString hashByString(const QString &str)
{
    // qDebug() << "Hashing string:" << str;
    return QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5).toHex();
}

QString hashByData(const QString &str)
{
    qDebug() << "Generating hash for file:" << str;
    QFile file(str);
    QString  stHashValue;
    if (file.open(QIODevice::ReadOnly)) { //只读方式打开
        QCryptographicHash hash(QCryptographicHash::Md5);

        QByteArray buf = file.read(1 * 1024 * 1024); // 每次读取10M
        buf = buf.append(str.toUtf8());
        hash.addData(buf);  // 将数据添加到Hash中
        stHashValue.append(hash.result().toHex());
        qDebug() << "Successfully generated hash for file:" << str << "Result:" << stHashValue;
    } else {
        qWarning() << "Failed to open file for hashing:" << str;
    }
    return stHashValue;
}

bool onMountDevice(const QString &path)
{
    qDebug() << "Checking if path is on mount device:" << path;
    bool result = (path.startsWith("/media/") || path.startsWith("/run/media/"));
    qDebug() << "Checking if path is on mount device:" << path << "Result:" << result;
    return result;
}

bool mountDeviceExist(const QString &path)
{
    qDebug() << "Checking if mount device exists for path:" << path;
    QString mountPoint;
    if (path.startsWith("/media/")) {
        const int sp = path.indexOf("/", 7) + 1;
        const int ep = path.indexOf("/", sp) + 1;
        mountPoint = path.mid(0, ep);
    } else if (path.startsWith("/run/media/")) {
        const int sp = path.indexOf("/", 11) + 1;
        const int ep = path.indexOf("/", sp) + 1;
        mountPoint = path.mid(0, ep);
    }

    return QFileInfo(mountPoint).exists();
}

QString filePathToThumbnailPath(const QString &filePath, QString dataHash)
{
    qDebug() << "Converting file path to thumbnail path:" << filePath;
    QFileInfo temDir(filePath);
    //如果hash为空，制作新的hash
    if (dataHash.isEmpty()) {
        dataHash = hashByData(filePath);
    }

    return albumGlobal::CACHE_PATH + temDir.path() + "/" + dataHash + ".png";
}

QString getDeleteFullPath(const QString &hash, const QString &fileName)
{
    // qDebug() << "Getting delete full path for hash:" << hash << "and file name:" << fileName;
    //防止文件过长,采用只用hash的名称;
    return albumGlobal::DELETE_PATH + "/" + hash + "." + QFileInfo(fileName).suffix();
}

std::pair<QDateTime, bool> analyzeDateTime(const QVariant &data)
{
    qDebug() << "Analyzing date time from data:" << data;
    auto str = data.toString();
    QDateTime result = QDateTime::fromString(str, DATETIME_FORMAT_DATABASE);
    if (!result.isValid()) {
        result = stringToDateTime(str);
    }
    if (result.isValid()) {
        qDebug() << "Successfully parsed date time:" << result;
        return std::make_pair(result, true);
    } else {
        qWarning() << "Failed to parse date time from data:" << data;
        return std::make_pair(data.toDateTime(), false);
    }
}

bool syncCopy(const QString &srcFileName, const QString &dstFileName)
{
    qDebug() << "Starting synchronous copy from:" << srcFileName << "to:" << dstFileName;
    QFile src(srcFileName);
    QFile dst(dstFileName);

    src.open(QIODevice::ReadOnly);
    dst.open(QIODevice::WriteOnly);

    //0.预分配空间
    auto fileSize = src.size();
    if (!dst.resize(fileSize)) { //预分配空间失败
        qWarning() << "Failed to pre-allocate space for destination file:" << dstFileName;
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

    qDebug() << "Successfully completed synchronous copy";
    return true;
}

QString mkMutiDir(const QString &path)   //创建多级目录
{
    qDebug() << "Creating multi-level directory:" << path;
    QDir dir(path);
    if (dir.exists(path)) {
        qDebug() << "Directory already exists:" << path;
        return path;
    }
    QString parentDir = mkMutiDir(path.mid(0, path.lastIndexOf('/')));
    QString dirname = path.mid(path.lastIndexOf('/') + 1);
    QDir parentPath(parentDir);
    if (!dirname.isEmpty()) {
        parentPath.mkpath(dirname);
        qDebug() << "Created directory:" << dirname << "in parent:" << parentDir;
    }
    return parentDir + "/" + dirname;
}

bool checkMimeUrls(const QList<QUrl> &urls)
{
    qDebug() << "Checking MIME types for" << urls.size() << "URLs";
    if (1 > urls.size()) {
        qWarning() << "Empty URL list provided";
        return false;
    }
    QList<QUrl> urlList = urls;
    using namespace Libutils::image;
    for (QUrl url : urlList) {
        const QString path = url.toLocalFile();
        QFileInfo fileinfo(path);
        if (fileinfo.isDir()) {
            qDebug() << "Checking directory:" << path;
            auto finfos = getImagesAndVideoInfo(path, false);
            for (auto finfo : finfos) {
                if (imageSupportRead(finfo.absoluteFilePath()) || isVideo(finfo.absoluteFilePath())) {
                    qDebug() << "Found supported file in directory:" << finfo.absoluteFilePath();
                    return true;
                }
            }
        } else if (imageSupportRead(path) || isVideo(path)) {
            qDebug() << "Found supported file:" << path;
            return true;
        }
    }
    qDebug() << "No supported files found in URL list";
    return false;
}

/**
   @return 图片 \a path 是否支持设置为壁纸，同步看图代码
 */
bool isSupportWallpaper(const QString &path)
{
    qDebug() << "Checking if file supports wallpaper:" << path;
    QMimeDatabase db;
    QMimeType mt = db.mimeTypeForFile(path, QMimeDatabase::MatchDefault);
    bool result = mt.name().startsWith("image")
           && !mt.name().endsWith("svg+xml")
           && !mt.name().endsWith("raf")
           && !mt.name().endsWith("crw")
           && !mt.name().endsWith("x-portable-anymap");
    qDebug() << "File" << path << "wallpaper support:" << result << "MIME type:" << mt.name();
    return result;
}


//bool        isCommandExist(const QString &command)
//{
//    QProcess *proc = new QProcess;
//    QString cm = QString("which %1\n").arg(command);
//    proc->start(cm);
//    proc->waitForFinished(1000);

//    if (proc->exitCode() == 0) {
//        return true;
//    } else {
//        return false;
//    }

//}
}  // namespace base

}  // namespace utils
