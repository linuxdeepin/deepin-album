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

    void createNewAlbum();
    void picsIntoAlbum(QStringList paths);
    void SearchReturnUpdate();

private:
    void initConnections();
    void initLeftView();
    void initRightView();
    void updateLeftView();
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
    void updateExternalDevice();
    bool findPicturePathByPhone(QString &path);
    void updateImportComboBox();
    void importAllBtnClicked();
    void importSelectBtnClicked();
    int getNewAlbumItemIndex();
    void onUnMountSignal(QString unMountPath);
    void loadMountPicture(QString path);

private slots:
    void onLeftMenuClicked(QAction *action);
    void onTrashRecoveryBtnClicked();
    void onTrashDeleteBtnClicked();
    void onTrashListClicked();
    void onUpdataAlbumRightTitle(QString titlename);
    void onPixMapRotate(QStringList paths);

public:
    int m_iAlubmPicsNum;
    QString m_currentAlbum;

    DStackedWidget* m_pRightStackWidget;
    DListWidget* m_pLeftTabList;

    StatusBar* m_pStatusBar;
    DWidget* m_pWidget;
private:

    QStringList m_allAlbumNames;
    QStringList m_customAlbumNames;

    DWidget* m_pLeftWidget;
    ImportView* m_pImportView;
    ThumbnailListView* m_pRightThumbnailList;
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

    //手机图片导入窗体
    DWidget* m_importByPhoneWidget;
    DComboBox *m_importByPhoneComboBox;
    DPushButton *m_importAllByPhoneBtn;
    DPushButton *m_importSelectByPhoneBtn;
    QList<QExplicitlySharedDataPointer<DGioMount>> m_mounts;     //外部设备挂载
    QList<ThumbnailListView::ItemInfo> m_curThumbnaiItemList;
    QListWidgetItem *m_curListWidgetItem;

    QMap<QString, QPixmap> m_phonePicMap;
};

#endif // ALBUMVIEW_H
