#ifndef ALBUMVIEW_H
#define ALBUMVIEW_H

#include "widgets/thumbnaillistview.h"
#include "dbmanager/dbmanager.h"
#include "controller/signalmanager.h"
#include "widgets/albumlefttabitem.h"

#include <QWidget>
#include <QSplitter>
#include <DListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <DLineEdit>
#include <DStackedWidget>
#include <DPushButton>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

class AlbumView : public QSplitter
{
    Q_OBJECT

public:
    AlbumView();

    void createNewAlbum();

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
    void menuOpenImage(QString path,QStringList paths,bool isFullScreen);

private slots:
    void onLeftMenuClicked(QAction *action);
    void onTrashRecoveryBtnClicked();
    void onTrashDeleteBtnClicked();

public:
    int m_iAlubmPicsNum;

private:

    QString m_currentAlbum;
    QStringList m_allAlbumNames;

    DListWidget* m_pLeftTabList;
    DStackedWidget* m_pRightStackWidget;
    ThumbnailListView* m_pRightThumbnailList;
    ThumbnailListView* m_pRightTrashThumbnailList;
    ThumbnailListView* m_pRightFavoriteThumbnailList;
    DPushButton* m_pRecoveryBtn;
    DPushButton* m_pDeleteBtn;
    DMenu* m_pLeftMenu;
};

#endif // ALBUMVIEW_H
