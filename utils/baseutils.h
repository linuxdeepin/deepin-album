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
#ifndef BASEUTILS_H
#define BASEUTILS_H

#include <QObject>
#include <QTimer>
#include <QColor>

#if QT_VERSION >= 0x050500
#define TIMER_SINGLESHOT(Time, Code, captured...){ \
    QTimer::singleShot(Time, [captured] {Code});\
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

#define VIEW_CONTEXT_MENU "查看"
#define FULLSCREEN_CONTEXT_MENU "全屏"
#define SLIDESHOW_CONTEXT_MENU "幻灯片放映"
#define EXPORT_CONTEXT_MENU "导出"
#define COPYTOCLIPBOARD_CONTEXT_MENU "复制"
#define DELETE_CONTEXT_MENU "删除"
#define THROWTOTRASH_CONTEXT_MENU "移动到回收站"
#define REMOVEFROMALBUM_CONTEXT_MENU "从相册内删除"
#define UNFAVORITE_CONTEXT_MENU "取消收藏"
#define FAVORITE_CONTEXT_MENU "收藏"
#define ROTATECLOCKWISE_CONTEXT_MENU "顺时针旋转"
#define ROTATECOUNTERCLOCKWISE_CONTEXT_MENU "逆时针旋转"
#define SETASWALLPAPER_CONTEXT_MENU "设为壁纸"
#define DISPLAYINFILEMANAGER_CONTEXT_MENU "在文件管理器中显示"
#define ImageInfo_CONTEXT_MENU "照片信息"
#define BUTTON_RECOVERY "恢复"

namespace utils {
namespace common {
const int TOP_TOOLBAR_THEIGHT = 40;
const int BOTTOM_TOOLBAR_HEIGHT = 22;

const int BORDER_RADIUS = 10;
const int BORDER_WIDTH = 0;
const int BORDER_WIDTH_SELECTED = 7;
const int THUMBNAIL_MAX_SCALE_SIZE = 192;

//const QColor DARK_BACKGROUND_COLOR = QColor("#202020");
//const QColor LIGHT_BACKGROUND_COLOR = QColor("#FFFFFF");
const QColor DARK_BACKGROUND_COLOR = QColor("#252525");
const QColor LIGHT_BACKGROUND_COLOR = QColor("#F8F8F8");

const QColor LIGHT_CHECKER_COLOR = QColor(0,0,0,0);
const QColor DARK_CHECKER_COLOR = QColor("#CCCCCC00");

const QColor DARK_BORDER_COLOR = QColor(255, 255, 255, 26);
const QColor LIGHT_BORDER_COLOR = QColor(0, 0, 0, 15);

const QColor DARK_TITLE_COLOR = QColor("#FFFFFF");
const QColor LIGHT_TITLE_COLOR = QColor(48, 48, 48);

const QString DARK_DEFAULT_THUMBNAIL = ":/resources/dark/images/default_thumbnail.png";
const QString LIGHT_DEFAULT_THUMBNAIL = ":/resources/light/images/default_thumbnail.png";

//const QColor BORDER_COLOR_SELECTED = QColor("#01bdff");
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
const QString CTRLF8_SHORTCUT = "Ctrl+F8";
const QString CTRLD_SHORTCUT = "Ctrl+D";
const QString ALTRETURN_SHORTCUT = "Alt+Return";
const QString CTRLSHIFTN_SHORTCUT = "Ctrl+Shift+N";
const QString F2_SHORTCUT = "F2";
const QString CTRLI_SHORTCUT = "Ctrl+O";
const QString CTRLQ_SHORTCUT = "Ctrl+Q";
const QString CTRLUP_SHORTCUT = "Ctrl+=";
const QString CTRLDOWN_SHORTCUT = "Ctrl+-";
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
    namespace naviwindow {
        const QString DARK_BG_IMG = ":/resources/dark/images/naviwindow_bg.png";
        const QColor DARK_BG_COLOR = QColor(0, 0, 0, 100);
        const QColor DARK_MR_BG_COLOR = QColor(0, 0, 0, 150);
        const QColor DARK_MR_BORDER_Color = QColor(255, 255, 255, 80);
        const QColor DARK_IMG_R_BORDER_COLOR = QColor(255, 255, 255, 50);

        const QString LIGHT_BG_IMG = ":/resources/light/images/naviwindow_bg.png";
        const QColor LIGHT_BG_COLOR = QColor(255, 255, 255, 104);
        const QColor LIGHT_MR_BG_COLOR = QColor(0, 0, 0, 101);
        const QColor LIGHT_MR_BORDER_Color = QColor(255, 255, 255, 80);
        const QColor LIGHT_IMG_R_BORDER_COLOR = QColor(255, 255, 255, 50);
    }
}
namespace widgets {

}
namespace base {
void        copyOneImageToClipboard(const QString& path);
void        copyImageToClipboard(const QStringList &paths);
void        showInFileManager(const QString &path);
int         stringWidth(const QFont &f, const QString &str);
int         stringHeight(const QFont &f, const QString &str);

QString     hash(const QString &str);
QString     wrapStr(const QString &str, const QFont &font, int maxWidth);
QString     sizeToHuman(const qlonglong bytes);
QString     timeToString(const QDateTime &time, bool normalFormat = false);
QDateTime   stringToDateTime(const QString &time);
QString     getFileContent(const QString &file);
QString     symFilePath(const QString &path);
bool        writeTextFile(QString filePath, QString content);

bool        trashFile(const QString &file);
bool        trashFiles(const QStringList &files);

bool        onMountDevice(const QString &path);
bool        mountDeviceExist(const QString &path);
bool        isCommandExist(const QString &command);
QPixmap     renderSVG(const QString &filePath, const QSize &size);
}  // namespace base

}  // namespace utils

#endif // BASEUTILS_H
