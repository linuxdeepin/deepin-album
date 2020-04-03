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

//class ThreadRenderImage : public QObject, public QRunnable
//{
//    Q_OBJECT
//public:
//    ThreadRenderImage();
//    void setData(QFileInfo fileinfo, QString path, QMap<QString, QPixmap> *map, QStringList *list);
////    void setRestart();
////    bool isRunning();
////    void setRunningTrue();

//protected:
//    virtual void run();

////signals:
////    void signal_RenderFinish(QPixmap, QString);
//private:
//    QString m_path = "";
//    QFileInfo m_fileinfo;
//    QMap<QString, QPixmap> *m_map = nullptr;
//    QStringList *m_pathlist = nullptr;
////    bool restart;
////    double m_width;
////    double m_height;
////    bool b_running;
//};

class MountLoader : public QObject
{
    Q_OBJECT
public:
    explicit MountLoader(AlbumView *parent);
    ~MountLoader()
    {
        qtpool.waitForDone();
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
    QThreadPool qtpool;
    bool bIsRunning = false;
    QString m_unmountpath = "";
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
    int m_scrollbartopdistance = 130;
    int m_scrollbarbottomdistance = 27;
};

class AlbumView : public QWidget, public ImageEngineImportObject, public ImageMountGetPathsObject, public ImageMountImportPathsObject
//class AlbumView : public DSplitter
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

    AlbumView();
    ~AlbumView() override;

    bool imageImported(bool success) override
    {
        emit dApp->signalM->closeWaitDialog();
        return true;
    }
    bool imageGeted(QStringList &filelist, QString path) override;
    bool imageMountImported(QStringList &filelist) override;

    void createNewAlbum(QStringList imagepaths);
    void iniWaitDiolag();
    void SearchReturnUpdate();
    void restorePicNum();
    void updatePicNum();

    void updateRightView();
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
    QString getNewAlbumName();
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;


    void onVfsMountChangedAdd(QExplicitlySharedDataPointer<DGioMount> mount);
    void onVfsMountChangedRemove(QExplicitlySharedDataPointer<DGioMount> mount);
    const QList<QExplicitlySharedDataPointer<DGioMount> > getVfsMountList();
    bool findPictureFile(QString &path, QList<ThumbnailListView::ItemInfo> &thumbnaiItemList);
    void initExternalDevice();
    void updateExternalDevice(QExplicitlySharedDataPointer<DGioMount> mount);
    bool findPicturePathByPhone(QString &path);
    void updateImportComboBox();
    void importAllBtnClicked();
    void importSelectBtnClicked();
//    void onUnMountSignal(QString unMountPath);
    void loadMountPicture(QString path);
    void initLeftMenu();
    void importComboBoxChange(QString strText);

signals:
    void sigSearchEditIsDisplay(bool);
    void sigLoadMountImagesStart(QString mountName, QString path);

private slots:
    void onTrashRecoveryBtnClicked();
    void onTrashDeleteBtnClicked();
    void onTrashListClicked();
    void onUpdataAlbumRightTitle(QString titlename);
    void onUpdateThumbnailViewSize();
    void onUnMountSignal(QString unMountPath);

    void onCreateNewAlbumFromDialog(QString albumname);
#if 1
    void onCreateNewAlbumFrom(QString albumname);
#endif
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
    int m_selPicNum;
    bool m_itemClicked = false;

    DStackedWidget *m_pRightStackWidget;
    LeftListView *m_pLeftListView;
    StatusBar *m_pStatusBar;
    DWidget *m_pRightWidget;
    ThumbnailListView *m_pRightThumbnailList;
    ThumbnailListView *m_pRightPhoneThumbnailList;
    QString albumname;
    QMap<QString, QStringList> m_phoneNameAndPathlist;
    QMap<QString, QPixmap> m_phonePathAndImage;
    DWidget *m_pwidget;

private:
    ImportView *m_pImportView;
    ThumbnailListView *m_pRightTrashThumbnailList;
    ThumbnailListView *m_pRightFavoriteThumbnailList;

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
//    bool bcurThumbnaiItemList_str = true;
    QListWidgetItem *m_curListWidgetItem;
    QMap<QString, QPixmap> m_phonePicMap;

    int m_mountPicNum;

    MountLoader *m_mountloader;
    QThread *m_LoadThread;

//    QMap<QString, MountLoader *> m_mountLoaderList;
//    QMap<QString, QThread *> m_loadThreadList;

    DWidget *pImportTimeLineWidget;
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
    DWidget *m_pFavoriteWidget;

    QListWidgetItem *m_TrashitemItem;
    DWidget *m_TrashTitle;
    DWidget *m_pTrashWidget;
    //add end 3975

//    QGridLayout *pVBoxLayout = nullptr;
    DWidget *fatherwidget = nullptr;
    DWidget *pPhoneWidget = nullptr;
    DBlurEffectWidget *phonetopwidget = nullptr;

    Waitdevicedialog *m_waitDeviceScandialog;
    bool isWaitDialog = true;
    QTimer *m_waitDailog_timer;
};

#endif // ALBUMVIEW_H
