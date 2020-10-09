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


class MountLoader : public QObject
{
    Q_OBJECT
public:
    explicit MountLoader(AlbumView *parent);
    ~MountLoader()
    {
//        qtpool.waitForDone();
        QThreadPool::globalInstance()->waitForDone();
    }
    bool isRunning()
    {
        return bIsRunning;
    }
    void stopRunning(QString path)
    {
        bIsRunning = false;
        bneedunmountpath = true;
        m_unmountpath = path;
    }
private:
    bool findPicturePathByPhone(QString &path);

public slots:
//    void onLoadMountImagesStart(QString mountName, QString path);
    void onCopyPhotoFromPhone(QStringList phonepaths, QStringList systempaths);

signals:
    void sigFinishiLoad();
//    void sigLoadMountImagesStart(QString mountName, QString path);
    void sigCopyPhotoFromPhone(QStringList phonepaths, QStringList systempaths);
    void needUnMount(QString path);

private:
    AlbumView *m_parent;
    QStringList m_phoneImgPathList;
    QMap<QString, QPixmap> m_phonePathImage;
//    QThreadPool qtpool;
    bool bIsRunning = false;
    QString m_unmountpath;
    bool bneedunmountpath = false;
};

class AlbumViewList : public DListWidget
{
    Q_OBJECT
public:
    explicit AlbumViewList(QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *e) override;

signals:
public slots:
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

    //void createNewAlbum(QStringList imagepaths);
    void iniWaitDiolag();
    void SearchReturnUpdate();
    void restorePicNum();
    void updatePicNum();
    void updateRightView();
    void updateAlbumView(const QString &album);
private:
    void initConnections();
    void initLeftView();
    void initRightView();
    //void updateRightView();
    void updateRightNoTrashView();
    void updateRightTrashView();
    void updateRightImportView();
    void updateRightMyFavoriteView();
    void updateRightMountView();
    void leftTabClicked();
    void openImage(int index);
    void menuOpenImage(QString path, QStringList paths, bool isFullScreen, bool isSlideShow);
    //QString getNewAlbumName();
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

    void onVfsMountChangedAdd(QExplicitlySharedDataPointer<DGioMount> mount);
    void onVfsMountChangedRemove(QExplicitlySharedDataPointer<DGioMount> mount);        //拔掉外设移除
    const QList<QExplicitlySharedDataPointer<DGioMount> > getVfsMountList();
    bool findPictureFile(QString &path, QList<ThumbnailListView::ItemInfo> &thumbnaiItemList);
    void initExternalDevice();
    void updateExternalDevice(QExplicitlySharedDataPointer<DGioMount> mount, QString strPath = QString());
    bool findPicturePathByPhone(QString &path);
    void updateImportComboBox();
    void importAllBtnClicked();
    void importSelectBtnClicked();
//    void onUnMountSignal(QString unMountPath);
    void loadMountPicture(QString path);
    void initLeftMenu();
    void importComboBoxChange(QString strText);

signals:
    void sigSearchEditIsDisplay(bool bIsDisp);
    void sigLoadMountImagesStart(QString mountName, QString path);
    void sigReCalcTimeLineSizeIfNeed();

private slots:
    void onTrashRecoveryBtnClicked();
    void onTrashDeleteBtnClicked();
    void onTrashListClicked();
    void onUpdataAlbumRightTitle(QString titlename);
    void onUnMountSignal(QString unMountPath);          //手动卸载设备
    void onCreateNewAlbumFromDialog(QString albumname);
    void onCreateNewAlbumFrom(QString albumname);
    void onLoadMountImagesEnd(QString mountname);
    void onLeftListDropEvent(QModelIndex dropIndex);
    void onKeyDelete();
    void onKeyF2();
    void needUnMount(QString path);
    void importDialog();
    void onWaitDialogClose();
    void onWaitDialogIgnore();
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
    QString albumname;
    QMap<QString, QStringList> m_phoneNameAndPathlist;
    //LMH0424
    QMap<QString, ThumbnailListView *> m_phoneViewlist;//UNUSED
    QStringList m_pictrueallPathlist;
//    ThumbnailListView *m_pAllPicture = nullptr;
    int m_currentLoadingPictrue = 0;

    QMap<QString, QPixmap> m_phonePathAndImage; //UNUSED
    DWidget *m_pwidget;

    ThumbnailListView *m_pRightThumbnailList;               //已导入
    ThumbnailListView *m_pRightTrashThumbnailList;          //最近删除
    ThumbnailListView *m_pRightFavoriteThumbnailList;       //我的收藏

    DWidget *pImportTimeLineWidget;
    DWidget *m_pTrashWidget;
    DWidget *m_pFavoriteWidget;
    Waitdevicedialog *m_waitDeviceScandialog;
private:
    ImportView *m_pImportView;

    DPushButton *m_pRecoveryBtn;
    DPushButton *m_pDeleteBtn;
    DLabel *m_pRightTitle;
    DLabel *m_pRightPicTotal;
    DLabel *m_pImportPicTotal;
    DLabel *m_pFavoriteTitle;
    DLabel *m_pFavoritePicTotal;
    DLabel *m_pPhoneTitle;
    DLabel *m_pPhonePicTotal;
    SearchView *m_pSearchView;
    DGioVolumeManager *m_vfsManager;
    DDiskManager *m_diskManager;
    DLabel *pLabel1;
    DLabel *pLabel2;

    // 已导入窗体
    ImportTimeLineView *m_pImpTimeLineWidget;
    //手机照片导入窗体
    DWidget *m_importByPhoneWidget;
    DComboBox *m_importByPhoneComboBox;
    DPushButton *m_importAllByPhoneBtn;
    DSuggestButton *m_importSelectByPhoneBtn;
    QList<QExplicitlySharedDataPointer<DGioMount>> m_mounts;     //外部设备挂载
//    QList<ThumbnailListView::ItemInfo> m_curThumbnaiItemList;
    DBImgInfoList m_curThumbnaiItemList_info;
    QStringList m_curThumbnaiItemList_str;
    QStringList m_curPhoneItemList_str;         //外部设备图片的路径
    QMap<QString, QStringList>   m_phonePath;   //多个设备对应路径


    QMap<QString, QPixmap> m_phonePicMap;

    int m_mountPicNum;

    MountLoader *m_mountloader;
    QThread *m_LoadThread;

//    QMap<QString, MountLoader *> m_mountLoaderList;
//    QMap<QString, QThread *> m_loadThreadList;


    QMap<QUrl, QString> durlAndNameMap;
    void getAllDeviceName();
    DSpinner *m_spinner;
    DLabel *m_pImportTitle;

    //add start 3975
    QListWidgetItem *m_noTrashItem;
    DWidget *m_pNoTrashTitle;
    DWidget *m_pNoTrashWidget;

    QListWidgetItem *m_FavoriteItem;
    DWidget *m_FavoriteTitle;

    QListWidgetItem *m_TrashitemItem;
    DWidget *m_TrashTitle;

    AlbumDeleteDialog *m_deleteDialog = nullptr;

    //add end 3975

//    QGridLayout *pVBoxLayout = nullptr;
    DWidget *fatherwidget;
    DWidget *pPhoneWidget;
    // DBlurEffectWidget *phonetopwidget = nullptr;
    DWidget *phonetopwidget;

    bool isIgnore;
    QTimer *m_waitDailog_timer;
    QThread *m_updateMountViewThread;
    bool isMountThreadRunning;

    int m_currentViewPictureCount;

    QMutex m_mutex;

};

#endif // ALBUMVIEW_H
