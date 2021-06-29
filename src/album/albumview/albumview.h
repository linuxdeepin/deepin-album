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
#ifndef ALBUMVIEW_H
#define ALBUMVIEW_H

#include "thumbnail/thumbnaillistview.h"
#include "dbmanager/dbmanager.h"
#include "controller/signalmanager.h"
#include "widgets/albumlefttabitem.h"
#include "importview/importview.h"
#include "searchview/searchview.h"
#include "widgets/statusbar.h"
#include "importtimelineview/importtimelineview.h"
#include "leftlistview.h"
#include "waitdevicedialog.h"
#include "dialogs/albumdeletedialog.h"

#include <QWidget>
#include <QSplitter>
#include <QUrl>
#include <DListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <DLineEdit>
#include <DStackedWidget>
#include <DPushButton>
#include <dgiomount.h>
#include <DApplicationHelper>
#include <DSpinner>
#include <DSuggestButton>
#include <DDialog>
#include <ddiskmanager.h>
#include <dblockdevice.h>
#include <ddiskdevice.h>
#include "leftlistwidget.h"
#include <QRunnable>
#include <QThreadPool>

//#define GIO_COMPILATION
#undef signals
extern "C" {
#include <gio/gio.h>
}


#define signals public

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

class DGioVolumeManager;
class AlbumView;

/*相册界面右边展示界面类*/
class AlbumViewListWidget : public DListWidget
{
    Q_OBJECT
public:
    explicit AlbumViewListWidget(QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *e) override;

signals:
public slots:
    void on_rangeChanged(int min, int max);
private:
    int m_scrollbartopdistance;
    int m_scrollbarbottomdistance;
};

class AlbumView : public QWidget, public ImageEngineImportObject, public ImageMountGetPathsObject, public ImageMountImportPathsObject
{
    Q_OBJECT

public:
    enum MenuItemId {
        IdStartSlideShow,
        IdCreateAlbum,
        IdRenameAlbum,
        IdExport,
        IdDeleteAlbum,
    };

    enum AblumType {
        photosType,
        ablumType,
        devType
    };

    AlbumView();
    ~AlbumView() override;

    bool imageImported(bool success) override
    {
        Q_UNUSED(success);
        emit dApp->signalM->closeWaitDialog();
        return true;
    }
    bool imageGeted(QStringList &filelist, QString path) override;
    bool imageMountImported(QStringList &filelist) override;

    void iniWaitDiolag();
    void SearchReturnUpdate();
    void restorePicNum();
    void updatePicNum();
    void updateRightView();
    void updateAlbumView(const QString &album);
    void updateDeviceLeftList();
    void setCurrentItemType(int type);
    void leftTabClicked();
private:
    void initConnections();
    void initLeftView();
    void initRightView();
    void updateRightNoTrashView();
    void updateRightTrashView();
    void updateRightImportView();
    void updateRightMyFavoriteView();
    void updateRightMountView();
    void openImage(int index);
    void menuOpenImage(const QString &path, QStringList paths, bool isFullScreen, bool isSlideShow);

    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

    void onVfsMountChangedAdd(QExplicitlySharedDataPointer<DGioMount> mount);
    void onVfsMountChangedRemove(QExplicitlySharedDataPointer<DGioMount> mount);        //拔掉外设移除
    const QList<QExplicitlySharedDataPointer<DGioMount> > getVfsMountList();

    void initExternalDevice();
    void updateExternalDevice(QExplicitlySharedDataPointer<DGioMount> mount, QString strPath = QString());
    bool findPicturePathByPhone(QString &path);
    void updateImportComboBox();
    void importAllBtnClicked();
    void importSelectBtnClicked();

    void initLeftMenu();
    void importComboBoxChange(QString strText);
    void getAllDeviceName();

signals:
    void sigSearchEditIsDisplay(bool bIsDisp);
    void sigLoadMountImagesStart(QString mountName, QString path);
    void sigReCalcTimeLineSizeIfNeed();

private slots:
    void onTrashRecoveryBtnClicked();
    void onTrashDeleteBtnClicked();
    void onTrashListClicked();
    void onUpdataAlbumRightTitle(const QString &titlename);
    void onUnMountSignal(const QString &unMountPath);          //手动卸载设备
    void onCreateNewAlbumFromDialog(const QString &newalbumname);
    void onCreateNewAlbumFrom(const QString &albumname);
    void onLeftListDropEvent(QModelIndex dropIndex);
    void onKeyDelete();
    void onKeyF2();
    void needUnMount(const QString &path);
    void importDialog();
    void onWaitDialogClose();
    void onWaitDialogIgnore();
    // change lambda to normal slt
    void onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, const QString &albumName);
    void onRightFavoriteThumbnailListNeedResize(int h);
    void onRightTrashThumbnailListNeedResize(int h);
    void onRightThumbnailListNeedResize(int h);
    void ongMouseMove();
    void onSelectAll();
    void onInsertedIntoAlbum(const QString &albumname, QStringList pathlist);
    void onFinishLoad();
    void onFileSystemAdded(const QString &dbusPath);
    void onBlockDeviceAdded(const QString &blks);
    void onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType);
    void onRightPhoneCustomContextMenuRequested();
    void onRightPhoneThumbnailListMouseRelease();
    void onImportViewImportBtnClicked();
    void onImportFailedToView();
    void onUpdateFavoriteNum();
    void onWaitDailogTimeout();
    void onLeftListViewMountListWidgetClicked(const QModelIndex &index);
    void onMoveScroll(QAbstractScrollArea *obj, int distence);
public:
    int m_iAlubmPicsNum;
    QString m_currentAlbum;
    QString m_currentType;
    AblumType m_currentItemType = photosType;
    int m_selPicNum;
    bool m_itemClicked;

    DStackedWidget *m_pRightStackWidget;
    LeftListView *m_pLeftListView;
    StatusBar *m_pStatusBar;
    DWidget *m_pRightWidget;

    ThumbnailListView *m_pRightPhoneThumbnailList;
    QMap<QString, QStringList> m_phoneNameAndPathlist;
    //LMH0424
    QStringList m_pictrueallPathlist;

    DWidget *m_pwidget;

    ThumbnailListView *m_pRightThumbnailList;               //自定义
    ThumbnailListView *m_pRightTrashThumbnailList;          //最近删除
    ThumbnailListView *m_pRightFavoriteThumbnailList;       //我的收藏

    DWidget *pImportTimeLineWidget;
    DWidget *m_pTrashWidget;                                //最近删除外层界面
    DWidget *m_pFavoriteWidget;
    Waitdevicedialog *m_waitDeviceScandialog;
    ImportView *m_pImportView;
    // 已导入窗体
    ImportTimeLineView *m_pImpTimeLineView;

private:
    DPushButton *m_pRecoveryBtn;
    DPushButton *m_pDeleteBtn;
    //自定义相册标题
    DLabel *m_pRightTitle;
    DLabel *m_pRightPicTotal;
    //我的收藏标题栏
    DLabel *m_pFavoriteTitle;
    DLabel *m_pFavoritePicTotal;
    //外部设备
    DLabel *m_pPhoneTitle;
    DLabel *m_pPhonePicTotal;
    SearchView *m_pSearchView;
    DGioVolumeManager *m_vfsManager;
    DDiskManager *m_diskManager;
    //最近删除标题
    DLabel *m_TrashTitleLab;
    DLabel *m_TrashDescritionLab;

    //手机照片导入窗体
    DWidget *m_importByPhoneWidget;
    DComboBox *m_importByPhoneComboBox;
    DPushButton *m_importAllByPhoneBtn;
    DSuggestButton *m_importSelectByPhoneBtn;
    QList<QExplicitlySharedDataPointer<DGioMount>> m_mounts;     //外部设备挂载

    DBImgInfoList m_curThumbnaiItemList_info;
    QStringList m_curThumbnaiItemList_str;
    QStringList m_curPhoneItemList_str;         //外部设备图片的路径
    int m_mountPicNum;

    QMap<QUrl, QString> durlAndNameMap;
    DSpinner *m_spinner;
    //add start 3975
    QListWidgetItem *m_noTrashItem;
    DWidget *m_pNoTrashTitle;
    DWidget *m_pNoTrashWidget;//自定义相册右侧展示界面外层窗口
    QListWidgetItem *m_FavoriteItem;
    DWidget *m_FavoriteTitle;
    QListWidgetItem *m_TrashitemItem;
    DWidget *m_TrashTitle;
    AlbumDeleteDialog *m_deleteDialog = nullptr;
    //add end 3975
    DWidget *fatherwidget;
    DWidget *pPhoneWidget;
    DWidget *phonetopwidget;
    bool isIgnore;
    QTimer *m_waitDailog_timer;
    bool isMountThreadRunning;
    int m_currentViewPictureCount;

public:
    AlbumViewListWidget *m_TrashListWidget;
    AlbumViewListWidget *m_noTrashListWidget;
    AlbumViewListWidget *m_FavListWidget;
};

#endif // ALBUMVIEW_H
