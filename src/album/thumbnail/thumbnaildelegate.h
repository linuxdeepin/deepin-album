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


class TDThumbnailThread;
class ThumbnailDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    enum DelegateType {
        NullType = 0,
        AllPicViewType,
        AlbumViewType,
        AlbumViewPhoneType,
        TimeLineViewType,
        SearchViewType
    };
    struct ItemData {
        QString name;
        QString path = QString();
        int imgWidth;
        int imgHeight;
        bool isSelected;
        QString remainDays = "30天";
        QPixmap image;
        QString firstorlast = "NotFirstOrLast";
        bool bNotSupportedOrDamaged = false;
    };

    explicit ThumbnailDelegate(DelegateType type, QObject *parent = nullptr);
    void setIsDataLocked(bool value);
//    void setNeedPaint(bool value);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const Q_DECL_OVERRIDE;
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
    ItemData itemData(const QModelIndex &index) const;

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
};

#endif // ALBUMDELEGATE_H
