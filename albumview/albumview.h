#ifndef ALBUMVIEW_H
#define ALBUMVIEW_H

#include "widgets/thumbnaillistview.h"
#include "dbmanager/dbmanager.h"
#include "controller/signalmanager.h"

#include <QWidget>
#include <QSplitter>
#include <DListWidget>
#include <QListWidgetItem>

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
    void leftMenuClicked(const QModelIndex &index);

private:
    QString m_currentAlbum;
    QStringList m_allAlbumNames;

    DListWidget* m_pLeftMenuList;
    ThumbnailListView* m_pRightThumbnailList;
};

#endif // ALBUMVIEW_H
