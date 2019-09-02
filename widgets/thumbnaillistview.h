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
#ifndef THUMBNAILLISTVIEW_H
#define THUMBNAILLISTVIEW_H

#include "thumbnaildelegate.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QListView>
#include <QPixmap>
#include <DLabel>
#include <QFileInfo>
#include <QSize>
#include <QStandardItemModel>
#include <QBuffer>

class ThumbnailListView : public QListView
{
    Q_OBJECT
public:
    struct ItemInfo
    {
        QString name = QString();
        QString path = QString();
        int width;
        int height;
    };

    explicit ThumbnailListView(QWidget *parent = 0);
    ~ThumbnailListView();


    void insertThumbnails(const QList<ItemInfo> &itemList);

signals:


protected:

private:
    void calBasePixMapWandH();
    void calWidgetItemWandH();
    void addThumbnailView();
    void updateThumbnailView();

    virtual void resizeEvent(QResizeEvent *e);

private:
    int m_iDefaultWidth;
    QList<ItemInfo> m_ItemList;
    QList<QList<ItemInfo>> m_gridItem;

    ThumbnailDelegate *m_delegate;
    QStandardItemModel *m_model;

};

#endif // THUMBNAILLISTVIEW_H
