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
#ifndef ALBUMGLOBAL_H
#define ALBUMGLOBAL_H

#include <QPixmap>
#include <QList>
#include <QPixmap>
#include <QIcon>
#include <QFileInfo>
#include <QSize>
#include <QBuffer>
#include <QPointer>
#include <QDateTime>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

namespace albumGlobal {

const QString CACHE_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                           + QDir::separator() + "deepin" + QDir::separator() + "deepin-album"/* + QDir::separator()*/;
}

//数据库存储的文件类型
enum DbFileType {
    DbFileTypeNull = 0,
    DbFileTypePic,      //图片
    DbFileTypeVideo     //视频
};

struct DBAlbumInfo {
    QString name;
//    int count;
    QDateTime beginTime;
    QDateTime endTime;
};

enum ItemType {
    ItemTypeNull = 1,
    ItemTypeBlank,         //空白项，列表上方，悬浮控件下方高度
    ItemTypePic,
    ItemTypeVideo,
    ItemTypeTimeLineTitle, //时间线标题
    ItemTypeImportTimeLineTitle, //已导入时间线标题
    ItemTypeMountImg //设备图片
};

struct DBImgInfo {
    //数据库
    QString filePath;
    QString fileName;
    QString dirHash;
    QString dataHash;
    QDateTime time;         // 图片创建时间
    QDateTime changeTime;   // 文件修改时间
    QDateTime importTime;   // 导入时间 Or 删除时间
    QString albumname;      // 图片所属相册名，以","分隔
    QString albumSize;      //原图片分辨率
    QString videoDuration = "00:00";  //视频时长
    ItemType itemType = ItemTypePic;//类型，空白，图片，视频

    //显示
    int imgWidth = 0;
    int imgHeight = 0;
    QString remainDays = "30天";
    bool isSelected = false;
    QPixmap image = QPixmap();
    QPixmap damagedPixmap = QPixmap();
    bool bNotSupportedOrDamaged = false;
    bool bNeedDelete = false;//删除时间线与已导入标题时使用
    QString date;
    QString num;

    bool operator==(const DBImgInfo &other) const
    {
        return (dataHash == other.dataHash);
    }

    friend QDebug operator<<(QDebug &dbg, const DBImgInfo &info)
    {
        dbg << "(DBImgInfo)["
            << " Path:" << info.filePath
            << " Name:" << info.fileName
            << " Dir:" << info.dirHash
            << " Time:" << info.time
            << " ChangeTime:" << info.changeTime
            << " ImportTime:" << info.importTime
            << " AlbumName:" << info.albumname
            << " AlbumSize:" << info.albumSize
            << "]";
        return dbg;
    }
};
typedef QList<DBImgInfo> DBImgInfoList;

//struct DBImgInfo {
//    QString name = "";
//    QString path = "";
//    int imgWidth = 0;
//    int imgHeight = 0;
//    QString remainDays = "30天";
//    bool isSelected = false;
//    DBImgInfoType itemType = ItemTypePic;//类型，空白，图片，视频
//    QPixmap image = QPixmap();
//    QPixmap damagedPixmap = QPixmap();
//    bool bNotSupportedOrDamaged = false;
//    bool bNeedDelete = false;//删除时间线与已导入标题时使用
//    int fileType = 1;//判断文件类型，图片，视频，默认图片
//    QString videoDuration = "00:00";//视频时长

//    QString date;
//    QString num;
//};

enum OpenImgViewType {
    VIEW_MAINWINDOW_ALLPIC = 0,
    VIEW_MAINWINDOW_TIMELINE = 1,
    VIEW_MAINWINDOW_ALBUM = 2 //
};

Q_DECLARE_METATYPE(DBImgInfo)
#endif // ALBUMGLOBAL_H
