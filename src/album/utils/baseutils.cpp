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

int stringHeight(const QFont &f, const QString &str)
{
    QFontMetrics fm(f);
    return fm.boundingRect(str).height();
}

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

/*QByteArray hashByString(const QString &str)
{
    if (str.isEmpty()) {
        return "\xd4\x1d\x8c\xd9\x8f\x00\xb2\x04\xe9\x80\x09\x98\xec\xf8\x42\x7e";
    } else {
        return QCryptographicHash::hash(str.toUtf8(), QCryptographicHash::Md5);
    }
}*/

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

//bool checkMimeData(const QMimeData *mimeData)
//{
//    if (1 > mimeData->urls().size()) {
//        return false;
//    }
//    QList<QUrl> urlList = mimeData->urls();
//    using namespace utils::image;
//    for (QUrl url : urlList) {
//        const QString path = url.toLocalFile();
//        QFileInfo fileinfo(path);
//        if (fileinfo.isDir()) {
//            auto finfos =  getImagesAndVideoInfo(path, false);
//            for (auto finfo : finfos) {
//                if (imageSupportRead(finfo.absoluteFilePath()) || isVideo(finfo.absoluteFilePath())) {
//                    QFileInfo info(finfo.absoluteFilePath());
//                    QMimeDatabase db;
//                    QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
//                    QMimeType mt1 = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchExtension);
//                    QString str = info.suffix().toLower();

//                    if (str.isEmpty()) {
//                        if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng")) {
//                            if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
//                                return true;
//                            } else if (str.isEmpty()) {
//                                return true;
//                            }
//                        }
//                        if (mt.name().startsWith("video/") || mt.name().startsWith("application/x-matroska")) {
//                            return true;
//                        }
//                    } else {
//                        if (mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
//                            if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
//                                return true;
//                            }
//                        }
//                        if (mt1.name().startsWith("video/") || mt1.name().startsWith("application/x-matroska")) {
//                            return true;
//                        }
//                    }
//                }
//            }
//        } else if (imageSupportRead(path) || isVideo(path)) {
////            paths << path;
//            QFileInfo info(path);
//            QMimeDatabase db;
//            QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
//            QMimeType mt1 = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchExtension);
//            QString str = info.suffix().toLower();
//            if (str.isEmpty()) {
//                if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng")) {
//                    if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
//                        return true;
//                    } else if (str.isEmpty()) {
//                        return true;
//                    }
//                }
//                if (mt.name().startsWith("video/") || mt.name().startsWith("application/x-matroska")) {
//                    return true;
//                }
//            } else {
//                if (mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
//                    if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
//                        return true;
//                    }
//                }
//                if (mt1.name().startsWith("video/") || mt1.name().startsWith("application/x-matroska")) {
//                    return true;
//                }
//            }
//            return false;
//        }
//    }
//    return false;
//}

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

int daysDifferenceBetweenTime(const QDateTime &start, const QDateTime &end)
{
    int daysSec = 24 * 60 * 60;
    int stime = static_cast<int>(start.toTime_t());
    int etime = static_cast<int>(end.toTime_t());
    return ((etime - stime) / daysSec);
}

QString audioIndex2str(const int &index)
{
    QStringList audioList = {"mp2", "mp3", "aac", "ac3", "dts", "vorbis", "dvaudio", "wmav1", "wmav2", "mace3", "mace6",
                             "vmdaudio", "flac", "mp3adu", "mp3on4", "shorten", "alac", "westwood_snd1", "gsm", "qdm2",
                             "cook", "truespeech", "tta", "smackaudio", "qcelp", "wavpack", "dsicinaudio", "imc",
                             "musepack7", "mlp", "gsm_ms", "atrac3", "ape", "nellymoser", "musepack8", "speex", "wmavoice",
                             "wmapro", "wmalossless", "atrac3p", "eac3", "sipr", "mp1", "twinvq", "truehd", "mp4als",
                             "atrac1", "binkaudio_rdft", "binkaudio_dct", "aac_latm", "qdmc", "celt", "g723_1", "g729",
                             "8svx_exp", "8svx_fib", "bmv_audio", "ralf", "iac", "ilbc", "opus", "comfort_noise", "tak",
                             "metasound", "paf_audio", "on2avc", "dss_sp", "codec2", "ffwavesynth", "sonic", "sonic_ls",
                             "evrc", "smv", "dsd_lsbf", "dsd_msbf", "dsd_lsbf_planar", "dsd_msbf_planar", "4gv",
                             "interplay_acm", "xma1", "xma2", "dst", "atrac3al", "atrac3pal", "dolby_e", "aptx", "aptx_hd",
                             "sbc", "atrac9"
                            };
    QMap<int, QString> codecMap;
    for (int i = 0; i < audioList.size(); i++) {
        codecMap.insert(i + 86016, audioList[i]);
    }
    QString aa = codecMap[index];
    return aa;
}

QString videoIndex2str(const int &index)
{
    QStringList videoList = {"none", "mpeg1video", "mpeg2video", "h261", "h263", "rv10", "rv20",
                             "mjpeg", "mjpegb", "ljpeg", "sp5x", "jpegls", "mpeg4", "rawvideo", "msmpeg4v1",
                             "msmpeg4v2", "msmpeg4v3", "wmv1", "wmv2", "h263p", "h263i", "flv1", "svq1",
                             "svq3", "dvvideo", "huffyuv", "cyuv", "h264", "indeo3", "vp3", "theora",
                             "asv1", "asv2", "ffv1", "4xm", "vcr1", "cljr", "mdec", "roq", "interplay_video",
                             "xan_wc3", "xan_wc4", "rpza", "cinepak", "ws_vqa", "msrle", "msvideo1", "idcin",
                             "8bps", "smc", "flic", "truemotion1", "vmdvideo", "mszh", "zlib", "qtrle", "tscc",
                             "ulti", "qdraw", "vixl", "qpeg", "png", "ppm", "pbm", "pgm", "pgmyuv", "pam", "ffvhuff",
                             "rv30", "rv40", "vc1", "wmv3", "loco", "wnv1", "aasc", "indeo2", "fraps", "truemotion2",
                             "bmp", "cscd", "mmvideo", "zmbv", "avs", "smackvideo", "nuv", "kmvc", "flashsv",
                             "cavs", "jpeg2000", "vmnc", "vp5", "vp6", "vp6f", "targa", "dsicinvideo", "tiertexseqvideo",
                             "tiff", "gif", "dxa", "dnxhd", "thp", "sgi", "c93", "bethsoftvid", "ptx", "txd", "vp6a",
                             "amv", "vb", "pcx", "sunrast", "indeo4", "indeo5", "mimic", "rl2", "escape124", "dirac", "bfi",
                             "cmv", "motionpixels", "tgv", "tgq", "tqi", "aura", "aura2", "v210x", "tmv", "v210", "dpx",
                             "mad", "frwu", "flashsv2", "cdgraphics", "r210", "anm", "binkvideo", "iff_ilbm", "kgv1",
                             "yop", "vp8", "pictor", "ansi", "a64_multi", "a64_multi5", "r10k", "mxpeg", "lagarith",
                             "prores", "jv", "dfa", "wmv3image", "vc1image", "utvideo", "bmv_video", "vble", "dxtory",
                             "v410", "xwd", "cdxl", "xbm", "zerocodec", "mss1", "msa1", "tscc2", "mts2", "cllc", "mss2",
                             "vp9", "aic", "escape130", "g2m", "webp", "hnm4_video", "hevc", "fic", "alias_pix",
                             "brender_pix", "paf_video", "exr", "vp7", "sanm", "sgirle", "mvc1", "mvc2", "hqx", "tdsc",
                             "hq_hqa", "hap", "dds", "dxv", "screenpresso", "rscc", "avs2"
                            };
    QStringList PCMList = {"pcm_s16le", "pcm_s16be", "pcm_u16le", "pcm_u16be", "pcm_s8", "pcm_u8", "pcm_mulaw"
                           "pcm_alaw", "pcm_s32le", "pcm_s32be", "pcm_u32le", "pcm_u32be", "pcm_s24le", "pcm_s24be"
                           "pcm_u24le", "pcm_u24be", "pcm_s24daud", "pcm_zork", "pcm_s16le_planar", "pcm_dvd"
                           "pcm_f32be", "pcm_f32le", "pcm_f64be", "pcm_f64le", "pcm_bluray", "pcm_lxf", "s302m"
                           "pcm_s8_planar", "pcm_s24le_planar", "pcm_s32le_planar", "pcm_s16be_planar"
                          };
    QStringList ADPCMList = {"adpcm_ima_qt", "adpcm_ima_wav", "adpcm_ima_dk3", "adpcm_ima_dk4"
                             "adpcm_ima_ws", "adpcm_ima_smjpeg", "adpcm_ms", "adpcm_4xm", "adpcm_xa", "adpcm_adx"
                             "adpcm_ea", "adpcm_g726", "adpcm_ct", "adpcm_swf", "adpcm_yamaha", "adpcm_sbpro_4"
                             "adpcm_sbpro_3", "adpcm_sbpro_2", "adpcm_thp", "adpcm_ima_amv", "adpcm_ea_r1"
                             "adpcm_ea_r3", "adpcm_ea_r2", "adpcm_ima_ea_sead", "adpcm_ima_ea_eacs", "adpcm_ea_xas"
                             "adpcm_ea_maxis_xa", "adpcm_ima_iss", "adpcm_g722", "adpcm_ima_apc", "adpcm_vima"
                            };
    QStringList AMRList = {"amr_nb", "amr_wb"};
    QStringList realAudioList = {"ra_144", "ra_288" };
    QMap<int, QString> codecMap;
    for (int i = 0; i < videoList.size(); i++) {
        codecMap.insert(i, videoList[i]);
    }
    for (int i = 0; i < PCMList.size(); i++) {
        codecMap.insert(i + 65536, PCMList[i]);
    }
    for (int i = 0; i < ADPCMList.size(); i++) {
        codecMap.insert(i + 69632, ADPCMList[i]);
    }
    codecMap.insert(73728, "amr_nb");
    codecMap.insert(73729, "amr_wb");
    codecMap.insert(77824, "ra_144");
    codecMap.insert(77825, "ra_288");
    QString aa = codecMap[index];
    return aa;
}

QString Time2str(const qint64 &seconds)
{
    QTime d(0, 0, 0);
    if (seconds < DAYSECONDS) {
        d = d.addSecs(static_cast<int>(seconds));
        return d.toString("hh:mm:ss");
    } else {
        d = d.addSecs(static_cast<int>(seconds));
        int add = static_cast<int>(seconds / DAYSECONDS) * 24;
        QString dayOut =  d.toString("hh:mm:ss");
        dayOut.replace(0, 2, QString::number(add + dayOut.left(2).toInt()));
        return dayOut;
    }
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
    } else {
        qDebug() << "Move to trash failed! Could not write *.trashinfo!";
        return false;
    }
    return true;
}

QString getDeleteFullPath(const QString &hash, const QString &fileName)
{
    return albumGlobal::DELETE_PATH + "/" + hash + fileName;
}

}  // namespace base

}  // namespace utils
