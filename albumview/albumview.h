#ifndef ALBUMVIEW_H
#define ALBUMVIEW_H

#include "widgets/thumbnaillistview.h"
#include "dbmanager/dbmanager.h"
#include "controller/signalmanager.h"
#include "widgets/albumlefttabitem.h"
#include "importview/importview.h"
#include "searchview/searchview.h"
#include "widgets/statusbar.h"

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
#include "leftlistwidget.h"

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

class DGioVolumeManager;
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

private slots:
    void onLeftMenuClicked(QAction *action);
    void onTrashRecoveryBtnClicked();
    void onTrashDeleteBtnClicked();
    void onTrashListClicked();
    void onUpdataAlbumRightTitle(QString titlename);

    void onCreateNewAlbumFromDialog(QString albumname);
    void onLoadMountImagesEnd(QString mountname);
    void updatePicNum();
    void restorePicNum();

public:
    int m_iAlubmPicsNum;
    QString m_currentAlbum;
    int m_selPicNum;

    DStackedWidget* m_pRightStackWidget;
    LeftListWidget* m_pLeftTabList;

    StatusBar* m_pStatusBar;
    DWidget* m_pWidget;
    ThumbnailListView* m_pRightThumbnailList;

private:

    QStringList m_allAlbumNames;
    QStringList m_customAlbumNames;

    DWidget* m_pLeftWidget;
    ImportView* m_pImportView;
    ThumbnailListView* m_pRightTrashThumbnailList;
    ThumbnailListView* m_pRightFavoriteThumbnailList;
    DPushButton* m_pRecoveryBtn;
    DPushButton* m_pDeleteBtn;
    DMenu* m_pLeftMenu;
    DLabel* m_pRightTitle;
    DLabel* m_pRightPicTotal;
    DLabel* m_pFavoriteTitle;
    DLabel* m_pFavoritePicTotal;
    SearchView* m_pSearchView;
    DGioVolumeManager *m_vfsManager;
    DLabel* pLabel1;
    DLabel* pLabel2;

    //手机照片导入窗体
    DWidget* m_importByPhoneWidget;
    DComboBox *m_importByPhoneComboBox;
    DPushButton *m_importAllByPhoneBtn;
    DPushButton *m_importSelectByPhoneBtn;
    QList<QExplicitlySharedDataPointer<DGioMount>> m_mounts;     //外部设备挂载
    QList<ThumbnailListView::ItemInfo> m_curThumbnaiItemList;
    QListWidgetItem *m_curListWidgetItem;

    QMap<QString, QPixmap> m_phonePicMap;
    DSpinner *m_pSpinner;
    QMap<QString, QAction*> m_MenuActionMap;

    int m_loadMountFlag;                        // 0:全部加载完成 1:有未加载完成
    QMap<QExplicitlySharedDataPointer<DGioMount>, int> m_loadMountMap;          // key: 外设 value: 标记是否加载完成
};

#endif // ALBUMVIEW_H
