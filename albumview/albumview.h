#ifndef ALBUMVIEW_H
#define ALBUMVIEW_H

#include "widgets/thumbnaillistview.h"
#include "dbmanager/dbmanager.h"
#include "controller/signalmanager.h"
#include "widgets/albumlefttabitem.h"
#include "importview/importview.h"
#include "searchview/searchview.h"

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
    AlbumView();

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
    void appendAction(const QString &text);
    void openImage(int index);
    void menuOpenImage(QString path,QStringList paths,bool isFullScreen, bool isSlideShow);
    QString getNewAlbumName();
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;

    void onVfsMountChanged(QExplicitlySharedDataPointer<DGioMount> mount);
    const QList<QExplicitlySharedDataPointer<DGioMount> > getVfsMountList();
    bool findPictureFile(const QString & path, QList<ThumbnailListView::ItemInfo> &thumbnaiItemList);
    void updateExternalDevice(QList<QExplicitlySharedDataPointer<DGioMount> > mounts);

private slots:
    void onLeftMenuClicked(QAction *action);
    void onTrashRecoveryBtnClicked();
    void onTrashDeleteBtnClicked();
    void onTrashListClicked();
    void onUpdataAlbumRightTitle(QString titlename);

public:
    int m_iAlubmPicsNum;

    DStackedWidget* m_pRightStackWidget;
    DListWidget* m_pLeftTabList;

private:

    QString m_currentAlbum;
    QStringList m_allAlbumNames;

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
};

#endif // ALBUMVIEW_H
