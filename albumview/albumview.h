#ifndef ALBUMVIEW_H
#define ALBUMVIEW_H

#include "widgets/thumbnaillistview.h"
#include "dbmanager/dbmanager.h"
#include "controller/signalmanager.h"

#include <QWidget>
#include <QSplitter>
#include <DListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE

class AlbumView : public QSplitter
{
    Q_OBJECT

public:
    AlbumView();

private:
    void initConnections();
    void initLeftView();
    void initRightView();
    void updateLeftView();
    void updateRightView();
    void leftTabClicked(const QModelIndex &index);
    void showLeftMenu(const QPoint &pos);
    void appendAction(const QString &text);

private slots:
    void onLeftMenuClicked(QAction *action);

private:
    QString m_currentAlbum;
    QStringList m_allAlbumNames;

    DListWidget* m_pLeftTabList;
    ThumbnailListView* m_pRightThumbnailList;

    DMenu* m_pLeftMenu;
};

#endif // ALBUMVIEW_H
