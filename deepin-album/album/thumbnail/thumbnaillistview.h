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
//#include "application.h"
#include "controller/configsetter.h"
#include "controller/signalmanager.h"
//#include "controller/wallpapersetter.h"
#include "dbmanager/dbmanager.h"
#include "widgets/printhelper.h"
//#include "dlmenuarrow.h"
#include <QListWidget>
#include <QListWidgetItem>
#include <QListView>
#include <QList>
#include <DPushButton>
#include <DImageButton>
#include <DIconButton>
#include <DListView>
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
#include <QPointer>
#include <DApplicationHelper>
#include "imageengine/imageengineobject.h"
#include "widgets/timelineitem.h"

DWIDGET_USE_NAMESPACE

class ThumbnailListView : public DListView, public ImageEngineObject
{
    Q_OBJECT

public:
    enum MenuItemId {
        IdView,
        IdFullScreen,
        Idprint,
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
        IdTrashRecovery,
        IdDrawingBoard//lmh0407画板
    };

    struct ItemInfo {
        QString name = "";
        QString path = "";
        int baseWidth = 0;
        int baseHeight = 0;
        int width = 0;
        int height = 0;
        int imgWidth = 0;
        int imgHeight = 0;
        QString remainDays = "30天";
        QPixmap image = QPixmap();
        bool bNotSupportedOrDamaged = false;


        friend bool operator== (const ItemInfo &left, const ItemInfo &right)
        {

            if (left.image == right.image)
                return true;
            return false;
        }
    };

    explicit ThumbnailListView(ThumbnailDelegate::DelegateType type = ThumbnailDelegate::NullType, QString imgtype = "All Photos", QWidget *parent = nullptr);
    ~ThumbnailListView() override;

    //------------------
    void loadFilesFromLocal(QStringList files, bool needcache = true, bool needcheck = true);
    void loadFilesFromLocal(DBImgInfoList files, bool needcache = true, bool needcheck = true);
    void loadFilesFromTrash(DBImgInfoList files);
    void loadFilesFromDB(QString name = "", int loadCount = 0);
    bool imageLocalLoaded(QStringList &filelist) Q_DECL_OVERRIDE;
    bool imageFromDBLoaded(QStringList &filelist) Q_DECL_OVERRIDE;
    bool imageLoaded(QString filepath) Q_DECL_OVERRIDE;
    void insertThumbnail(const ItemInfo &iteminfo);
    void stopLoadAndClear(bool bClearModel = true);    //为true则清除模型中的数据
    QStringList getAllFileList();
    void setVScrollbarDistance(int topdistance, int bottomdistance);
    void setListWidgetItem(QListWidgetItem *item);
    void setIBaseHeight(int iBaseHeight);
    bool checkResizeNum();
    bool isLoading();
    bool isAllPicSeleted();
    //------------------

//    void insertThumbnails(const QList<ItemInfo> &itemList);
    QStringList selectedPaths();
//    QList<ItemInfo> getAllPaths();
    QStringList getAllPaths();
    QStringList getDagItemPath();

    void menuItemDeal(QStringList paths, QAction *action);
    QModelIndexList getSelectedIndexes();
    int getListViewHeight();     //add 3975
    void selectCurrent(int row);
    int getRow(QPoint point);
    void selectRear(int row);
    void selectFront(int row);
    void selectExtent(int start, int end);
    void clearSelectionRear(int row);
    void clearSelectionFront(int row);
    void clearSelectionExtent(int start, int end);
    void resizeHand();  //手动发送信号，计算大小
signals:
//    void loadend(int);
    void needResize(int);
    void loadEnd();
    void openImage(int);
    void menuOpenImage(QString path, QStringList paths, bool isFullScreen, bool isSlideShow = false);
    void exportImage(QString, QStringList);
    void showExtensionPanel();
    void hideExtensionPanel(bool immediately = false);
    void showImageInfo(const QString &path);
    void trashRecovery();
//    void trashDelete();
    void sigGetSelectedPaths(QStringList *pPaths);
    void sigKeyEvent(int key);
#if 1
    void sigMouseRelease();
//    void sigCtrlMouseRelease();
    void sigMousePress(QMouseEvent *event);
    void sigShiftMousePress(QMouseEvent *event);
    void sigCtrlMousePress(QMouseEvent *event);
    void sigMenuItemDeal(QAction *action);
    void sigSelectAll();
    void sigMouseMove();
    void needResizeLabel();
//    void sigDrop();
#endif

    void sigLoad80ThumbnailsFinish();
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
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
private slots:
    void onMenuItemClicked(QAction *action);
    void onShowMenu(const QPoint &pos);
    void onPixMapScale(int value);
    void onCancelFavorite(const QModelIndex &index);
    void onTimerOut();
//    void onResizeEventTimerOut();
//    void slotPageNeedResize(int index);
    void resizeEventF();
    void sltChangeDamagedPixOnThemeChanged();

public slots:
    void slotReCalcTimelineSize();
    void slotLoad80ThumbnailsFinish();

public:
    void updateThumbnailView(QString updatePath = "");
private:
    //------------------
    void requestSomeImages();
    //------------------

    void initConnections();
    //------------------
    void calBasePixMap(ItemInfo &info);
    void calBasePixMapWandH();
    void calWidgetItem();
    void calWidgetItemWandH();
    void addThumbnailViewNew(QList<QList<ItemInfo>> gridItem);
    void addThumbnailView();
    void sendNeedResize(/*int height*/);

    //------------------
//    void updateThumbnailView();
    void updateMenuContents();
    void appendAction(int id, const QString &text, const QString &shortcut);
    void onShowImageInfo(const QString &path);
    void initMenuAction();
    DMenu *createAlbumMenu();
    void resizeEvent(QResizeEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *e) override;

    QPixmap getDamagedPixmap();

    /**
     * @brief updateThumbnaillistview
     *
     *  重新调整界面大小（当滑动条改变时即缩放）
     */
    void updateThumbnaillistview();

public:
    QString m_imageType;
    QStandardItemModel *m_model = nullptr;

private:
    int m_iDefaultWidth = 0;
    int m_iBaseHeight = 0;
    int m_height = 0;

    QList<ItemInfo> m_ItemList;
    QList<QList<ItemInfo>> m_gridItem;
    ThumbnailDelegate *m_delegate = nullptr;

    DMenu *m_pMenu = nullptr;
    QMap<QString, QAction *> m_MenuActionMap;
    DMenu *m_albumMenu = nullptr;

    QList<QString> m_timelines;
    QStringList m_dragItemPath;
    ThumbnailDelegate::DelegateType m_delegatetype = ThumbnailDelegate::NullType;

    //------------------
    QStringList m_allfileslist;
    QStringList m_filesbeleft;
    bool bneedloadimage = true;
    bool brequestallfiles = false;
    QList<ItemInfo> m_ItemListLeft;
    int m_requestCount = 0;
    int m_allNeedRequestFilesCount = 0;
    bool blastload = false;
    bool bfirstload = true;
    bool bneedcache = true;
    int m_scrollbartopdistance = 0;
    int m_scrollbarbottomdistance = 0;
    QListWidgetItem *m_item = nullptr;
    QTimer *m_dt = nullptr;
    bool bneedsendresize = false;
    int lastresizeheight = 0;
    int resizenum = 0;
//    QTimer *m_dtresizeevent = nullptr;
//    bool bneedresize = false;
//    int lastwidth = 0;
//    int newwidth = 0;
    //------------------

    //---触屏判断--------
    // 用于实现触屏滚动视图和框选文件不冲突，手指在屏幕上按下短时间内就开始移动
    // 会被认为触发滚动视图，否则为触发文件选择（时间默认为300毫秒）
    QPointer<QTimer> updateEnableSelectionByMouseTimer;

    // 记录触摸按下事件，在mouse move事件中使用，用于判断手指移动的距离，当大于
    // QPlatformTheme::TouchDoubleTapDistance 的值时认为触发触屏滚动
    QPoint lastTouchBeginPos;
    int touchTapDistance;
};

#endif // THUMBNAILLISTVIEW_H
