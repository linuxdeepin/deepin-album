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
#include "application.h"
#include "controller/configsetter.h"
#include "controller/wallpapersetter.h"
#include "dbmanager/dbmanager.h"
#include <QListWidget>
#include <QListWidgetItem>
#include <QListView>
#include <QPixmap>
#include <DLabel>
#include <QFileInfo>
#include <QSize>
#include <QStandardItemModel>
#include <QBuffer>
#include <DMenu>
#include <QMouseEvent>

class ThumbnailListView : public QListView
{
    Q_OBJECT

public:
    enum MenuItemId {
        IdView,
        IdFullScreen,
        IdStartSlideShow,
        IdPrint,
        IdAddToAlbum,
        IdExport,
        IdCopy,
        IdCopyToClipboard,
        IdMoveToTrash,
        IdRemoveFromAlbum,
        IdEdit,
        IdAddToFavorites,
        IdRemoveFromFavorites,
        IdRotateClockwise,
        IdRotateCounterclockwise,
        IdLabel,
        IdSetAsWallpaper,
        IdDisplayInFileManager,
        IdImageInfo,
        IdSubMenu,
        IdSeparator
    };

    struct ItemInfo
    {
        QString name = QString();
        QString path = QString();
        int width;
        int height;
    };

    explicit ThumbnailListView(QString imgtype = "All pics");
    ~ThumbnailListView();


    void insertThumbnails(const QList<ItemInfo> &itemList);
    QStringList selectedPaths();

signals:
    void loadend(int);

protected:

private:
    void initConnections();
    void calBasePixMapWandH();
    void calWidgetItemWandH();
    void addThumbnailView();
    void updateThumbnailView();
    void onShowMenu(const QPoint &pos);
    void updateMenuContents();
    void appendAction(int id, const QString &text, const QString &shortcut);
    QMenu* createAlbumMenu();
    void onMenuItemClicked(QAction *action);
    void onShowImageInfo(const QString &path);

    void resizeEvent(QResizeEvent *e);

public:
    QString m_imageType;

private:
    int m_iDefaultWidth;
    int m_height;
    QList<ItemInfo> m_ItemList;
    QList<QList<ItemInfo>> m_gridItem;

    ThumbnailDelegate *m_delegate;
    QStandardItemModel *m_model;

    DMenu *m_pMenu;
};

#endif // THUMBNAILLISTVIEW_H
