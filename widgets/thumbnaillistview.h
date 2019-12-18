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
#include "controller/signalmanager.h"
#include "controller/wallpapersetter.h"
#include "dbmanager/dbmanager.h"
#include "dlmenuarrow.h"
#include <QListWidget>
#include <QListWidgetItem>
#include <QListView>
#include <QList>
#include <DPushButton>
#include <DImageButton>
#include <DIconButton>
#include <QWidgetAction>
#include <QPixmap>
#include <QIcon>
#include <DLabel>
#include <QFileInfo>
#include <QSize>
#include <QStandardItemModel>
#include <QBuffer>
#include <DMenu>
#include <QMouseEvent>
#include <DListView>
#include <DApplicationHelper>

class ThumbnailListView : public DListView
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
        IdSeparator,
        IdTrashRecovery
    };

    struct ItemInfo {
        QString name = QString();
        QString path = QString();
        int width;
        int height;
        int imgWidth;
        int imgHeight;
        QString remainDays = "30天";
        QPixmap image;
    };


    explicit ThumbnailListView(QString imgtype = "All Photos");
    ~ThumbnailListView();


    void insertThumbnails(const QList<ItemInfo> &itemList);
    QStringList selectedPaths();
    QList<ItemInfo> getAllPaths();
    QStringList getDagItemPath();

    void menuItemDeal(QStringList paths, QAction *action);
    QModelIndexList getSelectedIndexes();
    int getListViewHeight();     //add 3975
    void selectCurrent(int row);
    int getRow(QPoint point);
    void selectRear(int row);
    void selectFront(int row);
    void selectExtent(int start,int end);
    void clearSelectionRear(int row);
    void clearSelectionFront(int row);
    void clearSelectionExtent(int start,int end);

signals:
    void loadend(int);
    void openImage(int);
    void menuOpenImage(QString path, QStringList paths, bool isFullScreen, bool isSlideShow = false);
    void exportImage(QString, QStringList);
    void showExtensionPanel();
    void hideExtensionPanel(bool immediately = false);
    void showImageInfo(const QString &path);
    void trashRecovery();
    void trashDelete();
    void sigGetSelectedPaths(QStringList *pPaths);
    void sigKeyEvent(int key);
#if 1
    void sigMouseRelease();
//    void sigCtrlMouseRelease();
    void sigMousePress(QMouseEvent *event);
    void sigShiftMousePress(QMouseEvent *event);
    void sigCtrlMousePress(QMouseEvent *event);
    void sigMenuItemDeal( QAction *action);
    void sigSelectAll();
    void sigMouseMove();
//    void sigDrop();
#endif

protected:
    void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseMoveEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void mouseReleaseEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
    void keyPressEvent(QKeyEvent *event) Q_DECL_OVERRIDE;
    void dragEnterEvent(QDragEnterEvent *event) Q_DECL_OVERRIDE;
    void dragMoveEvent(QDragMoveEvent *event) Q_DECL_OVERRIDE;
    void dragLeaveEvent(QDragLeaveEvent *event) Q_DECL_OVERRIDE;
    void dropEvent(QDropEvent *event) Q_DECL_OVERRIDE;
    void startDrag(Qt::DropActions supportedActions) Q_DECL_OVERRIDE;

private slots:
    void onMenuItemClicked(QAction *action);
    void onShowMenu(const QPoint &pos);
    void onPixMapScale(int value);
    void onCancelFavorite(const QModelIndex &index);

private:
    void initConnections();
    void calBasePixMapWandH();
    void calWidgetItemWandH();
    void addThumbnailView();
    void updateThumbnailView();
    void updateMenuContents();
    void appendAction(int id, const QString &text, const QString &shortcut);
    void onShowImageInfo(const QString &path);
    void initMenuAction();
    DMenu *createAlbumMenu();

    void resizeEvent(QResizeEvent *e) override;


    bool eventFilter(QObject *obj, QEvent *e) override;

public:
    QString m_imageType;
    QStandardItemModel *m_model;

private:
    int m_iDefaultWidth;
    int m_iBaseHeight;
    int m_height;
    QList<ItemInfo> m_ItemList;
    QList<QList<ItemInfo>> m_gridItem;
    ThumbnailDelegate *m_delegate;

    DMenu *m_pMenu;
    QMap<QString, QAction *> m_MenuActionMap;
    DMenu *m_albumMenu;

    QList<QString> m_timelines;
    QStringList m_dragItemPath;
};

#endif // THUMBNAILLISTVIEW_H
