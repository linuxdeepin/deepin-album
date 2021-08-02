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
#ifndef ALBUMDELEGATE_H
#define ALBUMDELEGATE_H

#include <QObject>
#include <QDateTime>
#include <QStyledItemDelegate>
#include <QDebug>

#include "albumgloabl.h"

class TDThumbnailThread;
class ThumbnailDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    enum DelegateType {
        NullType = 0,
        AllPicViewType,//所有照片

        TimeLineViewType,//时间线

        SearchViewType,//搜索

        AlbumViewImportTimeLineViewType,//相册-最近导入
        AlbumViewTrashType,//相册-已删除
        AlbumViewFavoriteType,//相册-收藏
        AlbumViewCustomType,//相册-自定义
        AlbumViewPhoneType//相册-设备
    };

    explicit ThumbnailDelegate(DelegateType type, QObject *parent = nullptr);
    void setIsDataLocked(bool value);
//    void setNeedPaint(bool value);

    void setItemSize(QSize size);
    //绘制图片和视频
    void drawImgAndVideo(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;

protected:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const Q_DECL_OVERRIDE;

    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index) Q_DECL_OVERRIDE;

signals:
    void sigCancelFavorite(const QModelIndex &index);
//    void sigPageNeedResize(const int &index) const;

private:
    ItemInfo itemData(const QModelIndex &index) const;

public:
    QString m_imageTypeStr;

private:
    QPixmap selectedPixmapLight;
    QPixmap selectedPixmapDark;
    QColor m_borderColor;
    QString  m_defaultThumbnail;
    bool m_itemdata = false;
    DelegateType m_delegatetype = NullType;
    bool bneedpaint = true;
    QSize m_size;
};

#endif // ALBUMDELEGATE_H
