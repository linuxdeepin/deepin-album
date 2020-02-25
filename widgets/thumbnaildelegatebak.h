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
#ifndef ALBUMDELEGATE_H
#define ALBUMDELEGATE_H

#include <QObject>
#include <QDateTime>
#include <QStyledItemDelegate>
#include <QDebug>

#include <controller/viewerthememanager.h>

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
        int width;
        int height;
        int imgWidth;
        int imgHeight;
        int baseWidth;
        int baseHeight;
        QString remainDays = "30天";
        QPixmap image;
        QString firstorlast = "NotFirstOrLast";
    };

    explicit ThumbnailDelegate(DelegateType type, QObject *parent = nullptr);
    void setIsDataLocked(bool value);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const Q_DECL_OVERRIDE;
    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const Q_DECL_OVERRIDE;
    bool editorEvent(QEvent *event,
                     QAbstractItemModel *model,
                     const QStyleOptionViewItem &option,
                     const QModelIndex &index);
    int getSpacing() const;
private slots:
    void getSliderValue(int step);

signals:
    void sigCancelFavorite(const QModelIndex &index);
    void sigPageNeedResize(const int &index) const;

private:
    ItemData itemData(const QModelIndex &index) const;


public:
    QString m_imageTypeStr;

private:
    QColor m_borderColor;
    QString  m_defaultThumbnail;
    bool m_itemdata = false;
    DelegateType m_delegatetype = NullType;
    int step = -1;
};

#endif // ALBUMDELEGATE_H
