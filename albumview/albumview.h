#ifndef ALBUMVIEW_H
#define ALBUMVIEW_H

#include "widgets/thumbnaillistview.h"
#include "dbmanager/dbmanager.h"
#include "controller/signalmanager.h"
#include "widgets/albumlefttabitem.h"
#include "importview/importview.h"
#include "searchview/searchview.h"
#include "widgets/statusbar.h"
#include "importtimelineview/importtimelineview.h"

#include <QWidget>
#include <QSplitter>
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
#include "leftlistwidget.h"

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

class DGioVolumeManager;
class AlbumView;

class MountLoader : public QObject
{
    Q_OBJECT
public:
    explicit MountLoader(AlbumView* parent);

private:
    bool findPicturePathByPhone(QString &path);

public slots:
    void onLoadMountImagesStart(QString mountName, QString path);

signals:
    void sigFinishiLoad();
    void sigLoadMountImagesStart(QString mountName, QString path);

private:
    AlbumView* m_parent;
    QStringList m_phoneImgPathList;
    QMap<QString, QPixmap> m_phonePathImage;
};

class AlbumView : public QWidget
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
    ~AlbumView();

    void createNewAlbum(QStringList imagepaths);
    void SearchReturnUpdate();
    void restorePicNum();
    void updatePicNum();

private:
    void initConnections();
    void initLeftView();
    void initRightView();
    void updateRightView();
    void updateRightNoTrashView();
    void updateRightTrashView();
    void leftTabClicked(const QModelIndex &index);
    void showLeftMenu(const QPoint &pos);
    void appendAction(int id, const QString &text, const QString &shortcut);
    void openImage(int index);
    void menuOpenImage(QString path,QStringList paths,bool isFullScreen, bool isSlideShow);
    QString getNewAlbumName();
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;

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
    int getNewAlbumItemIndex();
    void onUnMountSignal(QString unMountPath);
    void loadMountPicture(QString path);
    void initLeftMenu();
    void importComboBoxChange(QString strText);

signals:
    void sigSearchEditIsDisplay(bool);
    void sigLoadMountImagesStart(QString mountName, QString path);

private slots:
    void onLeftMenuClicked(QAction *action);
    void onTrashRecoveryBtnClicked();
    void onTrashDeleteBtnClicked();
    void onTrashListClicked();
    void onUpdataAlbumRightTitle(QString titlename);

    void onCreateNewAlbumFromDialog(QString albumname);
#if 1
    void onCreateNewAlbumFrom(QString albumname);
#endif
    void onLoadMountImagesEnd(QString mountname);
    void onLeftListDropEvent(QModelIndex dropIndex);

public:
    int m_iAlubmPicsNum;
    QString m_currentAlbum;
    int m_selPicNum;

    DStackedWidget* m_pRightStackWidget;
    LeftListWidget* m_pLeftTabList;

    StatusBar* m_pStatusBar;
    DWidget* m_pWidget;
    ThumbnailListView* m_pRightThumbnailList;
    QString albumname;
    QMap<QString, QStringList> m_phoneNameAndPathlist;
    QMap<QString, QPixmap> m_phonePathAndImage;

private:

    QStringList m_allAlbumNames;

    DWidget* m_pLeftWidget;
    ImportView* m_pImportView;
    ThumbnailListView* m_pRightTrashThumbnailList;
    ThumbnailListView* m_pRightFavoriteThumbnailList;
    ThumbnailListView* m_pRightPhoneThumbnailList;
    DPushButton* m_pRecoveryBtn;
    DPushButton* m_pDeleteBtn;
    DMenu* m_pLeftMenu;
    DLabel* m_pRightTitle;
    DLabel* m_pRightPicTotal;
    DLabel* m_pImportPicTotal;
    DLabel* m_pFavoriteTitle;
    DLabel* m_pFavoritePicTotal;
    DLabel* m_pPhoneTitle;
    DLabel* m_pPhonePicTotal;
    SearchView* m_pSearchView;
    DGioVolumeManager *m_vfsManager;
    DLabel* pLabel1;
    DLabel* pLabel2;
    // 已导入窗体
    ImportTimeLineView* m_pImpTimeLineWidget;

    //手机照片导入窗体
    DWidget* m_importByPhoneWidget;
    DComboBox *m_importByPhoneComboBox;
    DPushButton *m_importAllByPhoneBtn;
    DSuggestButton *m_importSelectByPhoneBtn;
    QList<QExplicitlySharedDataPointer<DGioMount>> m_mounts;     //外部设备挂载
    QList<ThumbnailListView::ItemInfo> m_curThumbnaiItemList;
    QListWidgetItem *m_curListWidgetItem;
    QMap<QString, QPixmap> m_phonePicMap;
    QMap<QString, QAction*> m_MenuActionMap;

    int m_mountPicNum;

    MountLoader* m_mountloader;
    QThread* m_LoadThread;

    QMap<QString, MountLoader*> m_mountLoaderList;
    QMap<QString, QThread*> m_loadThreadList;
};

#endif // ALBUMVIEW_H
