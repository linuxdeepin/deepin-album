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
#ifndef BASEUTILS_H
#define BASEUTILS_H

#include <QObject>
#include <QTimer>
#include <QColor>
#include <QMimeData>
#include <dgiovolumemanager.h>
#include <QtConcurrent/QtConcurrent>

#if QT_VERSION >= 0x050500
#define TIMER_SINGLESHOT(Time, Code, captured, ...) \
    { \
        QTimer::singleShot(Time, [captured] { Code }); \
    }
#else
#define TIMER_SINGLESHOT(Time, Code, captured...){ \
        QTimer *timer = new QTimer;\
        timer->setSingleShot(true);\
        QObject::connect(timer, &QTimer::timeout, [timer, captured] {\
                                                                     timer->deleteLater();\
                                                                     Code\
                                                                    });\
        timer->start(Time);\
    }

#endif

#define COMMON_STR_ALLPHOTOS "所有照片"
#define COMMON_STR_TIMELINE "时间线"
#define COMMON_STR_RECENT_IMPORTED "已导入"
#define COMMON_STR_TRASH "最近删除"
#define COMMON_STR_FAVORITES "我的收藏"
#define COMMON_STR_SEARCH "搜索"
#define ALBUM_PATHNAME_BY_PHONE "DCIM"
#define ALBUM_PATHTYPE_BY_PHONE "External Devices"
#define ALBUM_PATHTYPE_BY_U "U Devices"
#define COMMON_STR_SLIDESHOW "幻灯片放映"
#define COMMON_STR_CREATEALBUM "新建相册"
#define COMMON_STR_RENAMEALBUM "重命名相册"
#define COMMON_STR_EXPORT "导出相册"
#define COMMON_STR_DELETEALBUM "删除相册"
#define COMMON_STR_VIEW_TIMELINE "timelineview"
#define COMMON_STR_CUSTOM "自定义"


#define VIEW_CONTEXT_MENU "View"
#define FULLSCREEN_CONTEXT_MENU "Fullscreen"
#define PRINT_CONTEXT_MENU "Print"
#define SLIDESHOW_CONTEXT_MENU "Slide show"
#define PRINT_CONTEXT_MENU "Print"
#define EXPORT_CONTEXT_MENU "Export"
#define COPYTOCLIPBOARD_CONTEXT_MENU "Copy"
#define DELETE_CONTEXT_MENU "Delete"
#define THROWTOTRASH_CONTEXT_MENU "Move to trash"
#define REMOVEFROMALBUM_CONTEXT_MENU "Delete from album"
#define UNFAVORITE_CONTEXT_MENU "Unfavorite"
#define FAVORITE_CONTEXT_MENU "Favorite"
#define ROTATECLOCKWISE_CONTEXT_MENU "Rotate clockwise"
#define ROTATECOUNTERCLOCKWISE_CONTEXT_MENU "Rotate counterclockwise"
#define SETASWALLPAPER_CONTEXT_MENU "Set as wallpaper"
#define DISPLAYINFILEMANAGER_CONTEXT_MENU "Display in file manager"
#define ImageInfo_CONTEXT_MENU "Image info"
#define VideoInfo_CONTEXT_MENU "Video info"
#define BUTTON_RECOVERY "Recovery"
#define SHOW_SHORTCUT_PREVIEW "Show shortcut preview"

#define VAULT_DECRYPT_DIR_NAME          "vault_unlocked"
#define VAULT_BASE_PATH (QDir::homePath() + QString("/.local/share/applications"))  //! 获取保险箱创建的目录地址

namespace utils {
namespace common {
const int TOP_TOOLBAR_THEIGHT = 40;
const int BOTTOM_TOOLBAR_HEIGHT = 22;

const int BORDER_RADIUS = 8;
const int SHADOW_BORDER_RADIUS = 14;
const int BORDER_WIDTH = 0;
const int BORDER_WIDTH_SELECTED = 7;
const int THUMBNAIL_MAX_SCALE_SIZE = 192;


const QColor DARK_BACKGROUND_COLOR = QColor("#252525");
const QColor DARK_BACKGROUND_COLOR2 = QColor(0, 0, 0, 76);
const QColor LIGHT_BACKGROUND_COLOR = QColor("#F8F8F8");

const QColor LIGHT_CHECKER_COLOR = QColor(0, 0, 0, 0);
const QColor DARK_CHECKER_COLOR = QColor("#CCCCCC00");

const QColor DARK_BORDER_COLOR = QColor(255, 255, 255, 26);
const QColor LIGHT_BORDER_COLOR = QColor(0, 0, 0, 15);

const QColor DARK_TITLE_COLOR = QColor("#FFFFFF");
const QColor LIGHT_TITLE_COLOR = QColor(48, 48, 48);

const QString DARK_DEFAULT_THUMBNAIL = ":/resources/dark/images/default_thumbnail.png";
const QString LIGHT_DEFAULT_THUMBNAIL = ":/resources/light/images/default_thumbnail.png";

const QColor BORDER_COLOR_UNSELECTED = QColor("white");
const QColor BORDER_COLOR_SELECTED = QColor("#c3c3c3");
const QColor SELECTED_RECT_COLOR = QColor(44, 167, 248, 26);
const QColor TOP_LINE2_COLOR_DARK = QColor(255, 255, 255, 13);
const QColor TOP_LINE2_COLOR_LIGHT = QColor(255, 255, 255, 153);
const QColor TITLE_SELECTED_COLOR = QColor("#2ca7f8");
const QString VIEW_ALLPIC_SRN = "viewAllpicSrn";
const QString VIEW_TIMELINE_SRN = "viewTimelineSrn";
const QString VIEW_SEARCH_SRN = "viewSearchSrn";
const QString SHORTCUTVIEW_GROUP = "SHORTCUTVIEW";

//快捷键
const QString ENTER_SHORTCUT = "Enter";
const QString RETURN_SHORTCUT = "Return";
const QString F11_SHORTCUT = "F11";
const QString F5_SHORTCUT = "F5";
const QString CTRLC_SHORTCUT = "Ctrl+C";
const QString DELETE_SHORTCUT = "Delete";
const QString SHIFTDEL_SHORTCUT = "Shift+Del";
const QString CTRLK_SHORTCUT = "Ctrl+K";
const QString CTRLSHIFTK_SHORTCUT = "Ctrl+Shift+K";
const QString CTRLR_SHORTCUT = "Ctrl+R";
const QString CTRLSHIFTR_SHORTCUT = "Ctrl+Shift+R";
//const QString CTRLF8_SHORTCUT = "Ctrl+F8";
const QString CTRLF9_SHORTCUT = "Ctrl+F9";
//const QString CTRLD_SHORTCUT = "Ctrl+D";
const QString ALTD_SHORTCUT = "Alt+D";
//const QString ALTRETURN_SHORTCUT = "Alt+Return";
const QString CTRLI_SHORTCUT = "Ctrl+I";
const QString CTRLSHIFTN_SHORTCUT = "Ctrl+Shift+N";
const QString F2_SHORTCUT = "F2";
const QString CTRLO_SHORTCUT = "Ctrl+O";
const QString CTRLQ_SHORTCUT = "Ctrl+Q";
const QString CTRLUP_SHORTCUT = "Ctrl+=";
const QString CTRLDOWN_SHORTCUT = "Ctrl+-";
const QString CTRLSHIFTSLASH_SHORTCUT = "Ctrl+Shift+/";
const QString CTRLE_SHORTCUT = "Ctrl+E";
const QString RECTRLUP_SHORTCUT = "Ctrl++";
const QString CTRLF_SHORTCUT = "Ctrl+F";
const QString SENTENCE_SHORTCUT = ".";
}
namespace timeline {
const QColor DARK_SEPERATOR_COLOR = QColor(255, 255, 255, 20);
const QColor LIGHT_SEPERATOR_COLOR = QColor(0, 0, 0, 20);
}
namespace album {
const QColor DARK_DATELABEL_COLOR = QColor(255, 255, 255, 153);
const QColor LIGHT_DATELABEL_COLOR = QColor(48, 48, 48, 255);

const QString DARK_CREATEALBUM_NORMALPIC = ":/resources/dark/images/"
                                           "create_album_normal.png";
const QString DARK_CREATEALBUM_HOVERPIC = ":/resources/dark/images/"
                                          "create_album_hover.png";
const QString DARK_CREATEALBUM_PRESSPIC = ":/resources/dark/images/"
                                          "create_album_press.png";
const QString LIGHT_CREATEALBUM_NORMALPIC = ":/resources/light/images/"
                                            "create_album_normal.png";
const QString LIGHT_CREATEALBUM_HOVERPIC = ":/resources/light/images/"
                                           "create_album_hover.png";
const QString LIGHT_CREATEALBUM_PRESSPIC = ":/resources/light/images/"
                                           "create_album_press.png";

const QString DARK_ADDPIC = ":/resources/dark/images/album_add.svg";
const QString LIGHT_ADDPIC = ":/resources/light/images/album_add.svg";

const QString DARK_ALBUM_BG_NORMALPIC = ":/resources/dark/images/"
                                        "album_bg_normal.png";
const QString DARK_ALBUM_BG_PRESSPIC = ":/resources/dark/images/"
                                       "album_bg_press.png";

const QString LIGHT_ALBUM_BG_NORMALPIC = ":/resources/light/images/"
                                         "album_bg_normal.svg";
const QString LIGHT_ALBUM_BG_HOVERPIC = ":/resources/light/images/"
                                        "album_bg_hover.svg";
const QString LIGHT_ALBUM_BG_PRESSPIC = ":/resources/light/images/"
                                        "album_bg_press.svg";
}
namespace view {
const QString DARK_DEFAULT_THUMBNAIL =
    ":/resources/dark/images/empty_defaultThumbnail.png";
const QString LIGHT_DEFAULT_THUMBNAIL =
    ":/resources/light/images/empty_defaultThumbnail.png";
const QString DARK_LOADINGICON =
    ":/resources/dark/images/dark_loading.gif";
const QString LIGHT_LOADINGICON =
    ":/resources/light/images/light_loading.gif";
const QString DARK_DAMAGEICON =
    ":/resources/images/other/picture damaged_dark.svg";
const QString LIGHT_DAMAGEICON =
    ":/resources/images/other/picture damaged_light.svg";

namespace naviwindow {
const QString DARK_BG_IMG = ":/resources/dark/naviwindow_bg.svg";
const QColor DARK_BG_COLOR = QColor(0, 0, 0, 100);
const QColor DARK_MR_BG_COLOR = QColor(0, 0, 0, 150);
const QColor DARK_MR_BORDER_Color = QColor(255, 255, 255, 80);
const QColor DARK_IMG_R_BORDER_COLOR = QColor(255, 255, 255, 50);

const QString LIGHT_BG_IMG = ":/resources/light/naviwindow_bg.svg";
const QColor LIGHT_BG_COLOR = QColor(255, 255, 255, 104);
const QColor LIGHT_MR_BG_COLOR = QColor(0, 0, 0, 101);
const QColor LIGHT_MR_BORDER_Color = QColor(255, 255, 255, 80);
const QColor LIGHT_IMG_R_BORDER_COLOR = QColor(255, 255, 255, 50);
}
}

namespace base {
//void        copyOneImageToClipboard(const QString &path);
void        copyImageToClipboard(const QStringList &paths);
void        showInFileManager(const QString &path);
QString     hashByString(const QString &str);
QString     hashByData(const QString &str);
QString     SpliteText(const QString &text, const QFont &font, int nLabelSize);
std::pair<QDateTime, bool> analyzeDateTime(const QVariant &data);
QString     getFileContent(const QString &file);
QPixmap     renderSVG(const QString &filePath, const QSize &size);
//bool        checkMimeData(const QMimeData *mimeData);
bool        checkMimeUrls(const QList<QUrl> &urls);
QString     mkMutiDir(const QString &path);
//根据源文件路径生产缩略图路径
QString     filePathToThumbnailPath(const QString &filePath, QString dataHash = "");
QUrl        UrlInfo(QString path);  //main中的全局函数移至此处
//视频相关
static unsigned int  DAYSECONDS = 86400;
const QStringList m_audioFiletypes = {"mp3", "ogg", "wav", "wma", "m4a", "aac", "ac3"
                                      , "ape", "flac", "ra", "mka", "dts", "opus", "amr"
                                     };
const QStringList m_videoFiletypes = {"avs2"/*支持avs2视频格式*/, "3g2", "3ga", "3gp", "3gp2"
                                      , "3gpp", "amv", "asf", "asx", "avf", "avi", "bdm"
                                      , "bdmv", "bik", "clpi", "cpi", "dat", "divx", "drc"
                                      , "dv", "dvr-ms", "f4v", "flv", "gvi", "gxf", "hdmov"
                                      , "hlv", "iso", "letv", "lrv", "m1v", "m2p", "m2t"
                                      , "m2ts", "m2v", "m3u", "m3u8", "m4v", "mkv", "moov"
                                      , "mov", "mov", "mp2", "mp2v", "mp4", "mp4v", "mpe"
                                      , "mpeg", "mpeg1", "mpeg2", "mpeg4", "mpg", "mpl", "mpls"
                                      , "mpv", "mpv2", "mqv", "mts", "mts", "mtv", "mxf", "mxg"
                                      , "nsv", "nuv", "ogg", "ogm", "ogv", "ogx", "ps", "qt"
                                      , "qtvr", "ram", "rec", "rm", "rm", "rmj", "rmm", "rms"
                                      , "rmvb", "rmx", "rp", "rpl", "rv", "rvx", "thp", "tod"
                                      , "tp", "trp", "ts", "tts", "txd", "vcd", "vdr", "vob"
                                      , "vp8", "vro", "webm", "wm", "wmv", "wtv", "xesc", "xspf"
                                     };
QString     Time2str(const qint64 &seconds);
QString     videoIndex2str(const int &index);
QString     audioIndex2str(const int &index);
//判断是否视频
bool        isVideo(QString path);
//字符串自适应长度，多的变省略号
QString reorganizationStr(const QFont &font, const QString &fullStr, int maxWidth);
//判断是否保险箱路径
bool isVaultFile(const QString &path);
//移动至回收站，抄的看图的代码，看图抄的文管的代码
bool trashFile(const QString &file);
//生成deepin-album-delete下的文件路径，hash：原始路径hash，fileName：文件名
QString getDeleteFullPath(const QString &hash, const QString &fileName);
//同步文件拷贝，用于区分QFile::copy的异步拷贝
bool syncCopy(const QString &srcFileName, const QString &dstFileName);
//可重入版本的getMounts
QList<QExplicitlySharedDataPointer<DGioMount>> getMounts_safe();
//快速加载图片
QFuture<void> multiLoadImage(const QStringList &paths);
}  // namespace base

}  // namespace utils

#endif // BASEUTILS_H
