// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
#include <DComboBox>

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

    bool imageImported(bool success) override;
    bool imageMountImported(QStringList &filelist) override;

    void iniWaitDiolag();
    void SearchReturnUpdate();
    void restorePicNum();
    void updatePicNum();
    void updateTotalLabelNum();
    void updateRightView();
    void updateAlbumView(int UID);
    void updateDeviceLeftList();
    void setCurrentItemType(int type);
    void leftTabClicked();
    void onAddNewNotifyDir(const QString &dirPath);
    bool checkIfNotified(const QString &dirPath);
private:
    void initConnections();
    void initLeftView();
    void initRightView();
    //初始化最近删除
    void initTrashWidget();
    //初始化自定义相册列表
    void initCustomAlbumWidget();
    //初始化图片分类列表
    void initClassWidget();
    //初始化收藏列表
    void initFavoriteWidget();
    //初始化设备列表
    void initPhoneWidget();
    //显示照片与视频总数
    void resetLabelCount(int photosCount, int videosCount, DLabel *lable);

    void updateRightCustomAlbumView();
    void updateRightTrashView();
    void updateRightImportView();
    void updateRightMyFavoriteView();
    void updateRightClassView();
    void updateRightClassViewDetail(const QString &className);
    void updateRightMountView();
    //打开图片
    void onOpenImageClass(int row, const QString &path, bool bFullScreen);
    //打开图片
    void onOpenImageFav(int row, const QString &path, bool bFullScreen);
    //打开图片
    void onOpenImageCustom(int row, const QString &path, bool bFullScreen);
    //幻灯片播放
    void onSlideShowFav(const QString &path);
    //幻灯片播放
    void onSlideShowCustom(const QString &path);
    //幻灯片播放，真实实现
    void runSlideShow(const QString &path, ThumbnailListView *listView);

    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void showEvent(QShowEvent *e) override;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

    void onVfsMountChangedAdd(QExplicitlySharedDataPointer<DGioMount> mount);
    void onVfsMountChangedRemove(QExplicitlySharedDataPointer<DGioMount> mount);        //拔掉外设移除
    const QList<QExplicitlySharedDataPointer<DGioMount> > getVfsMountList();

    void initExternalDevice();
    //更新外部设备的显示
    void updateExternalDevice(QExplicitlySharedDataPointer<DGioMount> mount, QString strPath = QString());
    bool findPicturePathByPhone(QString &path);
    void updateImportComboBox();
    void importAllBtnClicked();
    void importSelectBtnClicked();
    void importFromMountDevice(const QStringList &paths);

    void initLeftMenu();
    void importComboBoxChange(QString strText);
    void getAllDeviceName();

    //恢复标题显示
    void restoreTitleDisplay();

    // 字体改变、尺寸改变，同步调整标题栏区域控件显示大小
    void adjustTitleContent();
signals:
    void sigSearchEditIsDisplay(bool bIsDisp);
    void sigLoadMountImagesStart(QString mountName, QString path);

private slots:
    void onTrashRecoveryBtnClicked();
    void onUpdataAlbumRightTitle(const QString &titlename);
    void onUnMountSignal(const QString &unMountPath);          //手动卸载设备
    void onCreateNewAlbumFromDialog(const QString &newalbumname, int UID);
    void onCreateNewAlbumFrom(const QString &albumname, int UID);
    void onLeftListDropEvent(QModelIndex dropIndex);
    void onKeyDelete();
    void onKeyF2();
    void needUnMount(const QString &path);
    void importDialog();
    void onWaitDialogClose();
    void onWaitDialogIgnore();
    // change lambda to normal slt
    void onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, int UID);
    void ongMouseMove();
    void onSelectAll();
    void onInsertedIntoAlbum(int UID, QStringList pathlist);
    void onFileSystemAdded(const QString &dbusPath);
    void onBlockDeviceAdded(const QString &blks);
    void onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType);
    void onRightPhoneCustomContextMenuRequested();
    void onRightPhoneThumbnailListMouseRelease();
    void onImportViewImportBtnClicked();
    void onImportFailedToView();
    void onWaitDailogTimeout();
    void onDelayLoadMountTimeout();
    void onLeftListViewMountListWidgetClicked(const QModelIndex &index);
    void onTrashUpdate();

    //接收到设备中文件列表加载完成信号
    void sltLoadMountFileList(const QString &path, QStringList fileList);
    //筛选显示，当先列表中内容为无结果
    void slotNoPicOrNoVideo(bool isNoResult);
    //缩略图选中项改变
    void sltSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    //最近删除中有变化
    void onTrashInfosChanged();
    //删除描述语自适应变化
    void adaptiveTrashDescritionLabel();
    //批量操作时判断标题栏是否有遮挡情况
    void onBatchSelectChanged(bool isBatchSelect);
    //默认导入路径已被销毁
    void onMonitorDestroyed(int UID);
    //筛选条件按钮宽度变化响应槽
    void onFilterBtnWidthChanged(int width);

    void slotClassBackClicked();
public:
    int m_iAlubmPicsNum;
    QString m_currentAlbum;
    int m_currentUID;
    QString m_currentType;
    QString m_currentClassName;
    AblumType m_currentItemType = photosType;
    int m_selPicNum;
    bool m_itemClicked;

    DStackedWidget *m_pRightStackWidget;
    LeftListView *m_pLeftListView;
    StatusBar *m_pStatusBar;
    DWidget *m_pRightWidget;

    ThumbnailListView *m_pRightPhoneThumbnailList;       //设备相关列表

    DWidget *m_pwidget;

    ThumbnailListView *m_customThumbnailList = nullptr;               //自定义
    ThumbnailListView *m_pRightTrashThumbnailList = nullptr;          //最近删除
    ThumbnailListView *m_classThumbnailList = nullptr;                //图片分类封面页
    ThumbnailListView *m_classDetailThumbnailList = nullptr;          //图片分类详情页
    ThumbnailListView *m_favoriteThumbnailList = nullptr;             //我的收藏

    DWidget *pImportTimeLineWidget;
    Waitdevicedialog *m_waitDeviceScandialog;
    ImportView *m_pImportView;
    // 已导入窗体
    ImportTimeLineView *m_pImpTimeLineView;

    const static int custom_title_height = 60;
    const static int favorite_title_height = 60;
    const static int trash_title_height = 60;
    const static int magin_offset = 20;//标题不居中偏移
private:
    //自定义相册标题
    DLabel *m_customAlbumTitleLabel = nullptr;
    DLabel *m_pRightPicTotal = nullptr;
    BatchOperateWidget *m_customBatchOperateWidget = nullptr;
    DWidget *m_customAlbumTitle = nullptr;           //自定义相册悬浮标题
    DWidget *m_pCustomAlbumWidget = nullptr;          //自定义相册右侧展示界面外层窗口
    NoResultWidget *m_customNoResultWidget = nullptr;
    QString m_CustomRightPicTotalFullStr;
    //图片分类标题栏
    DWidget *m_pClassWidget = nullptr;
    DWidget *m_ClassTitleWidget = nullptr;
    DLabel *m_pClassTitle = nullptr;
    DLabel *m_pClassPicTotal = nullptr;
    DPushButton *m_pClassBackBtn = nullptr;
    BatchOperateWidget *m_classBatchOperateWidget = nullptr;
    NoResultWidget *m_classNoResultWidget = nullptr;
    QString m_ClassPicTotalFullStr;
    bool m_bHasClassified = false;
    //我的收藏标题栏
    DWidget *m_pFavoriteWidget = nullptr;
    DWidget *m_FavoriteTitleWidget = nullptr;
    DLabel *m_pFavoriteTitle;
    DLabel *m_pFavoritePicTotal;
    BatchOperateWidget *m_favoriteBatchOperateWidget = nullptr;
    NoResultWidget *m_favoriteNoResultWidget = nullptr;
    QString m_FavoritePicTotalFullStr;
    //外部设备
    DLabel *m_pPhoneTitle;
    DLabel *m_pPhonePicTotal;
    SearchView *m_pSearchView;
    DGioVolumeManager *m_vfsManager;
    DDiskManager *m_diskManager;
    QString m_PhonePicTotalFullStr;
    QString m_phoneTitleFullStr;
    //最近删除标题
    DLabel *m_TrashTitleLab = nullptr;
    DLabel *m_TrashDescritionLab = nullptr;
    DWidget *m_TrashTitleWidget = nullptr;
    BatchOperateWidget *m_trashBatchOperateWidget = nullptr;
    DWidget *m_pTrashWidget = nullptr;                                //最近删除外层界面
    NoResultWidget *m_trashNoResultWidget = nullptr;
    QString m_trashNoticeFullStr;
    //手机照片导入窗体
    DWidget *m_importByPhoneWidget;
    DComboBox *m_importByPhoneComboBox;
    DPushButton *m_importAllByPhoneBtn;
    DSuggestButton *m_importSelectByPhoneBtn;
    QList<QExplicitlySharedDataPointer<DGioMount>> m_mounts;     //外部设备挂载

    int m_mountPicNum;

    QMap<QUrl, QString> durlAndNameMap;
    DSpinner *m_spinner = nullptr;
    //add start 3975
    AlbumDeleteDialog *m_deleteDialog = nullptr;
    //add end 3975
    DWidget *pPhoneWidget;
    DWidget *phonetopwidget;
    bool isIgnore;
    QTimer *m_waitDailog_timer;
    QTimer *m_delayLoadMount_timer; //延迟加载挂载路径-定时器
    QString m_delayLoadMountPath; //延迟加载路径
    QMap<QString, bool> mountLoadStatus;
};

#endif // ALBUMVIEW_H
