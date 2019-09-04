#ifndef ALBUMVIEW_H
#define ALBUMVIEW_H

#include "widgets/thumbnaillistview.h"
#include "dbmanager/dbmanager.h"
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
    void initUI();
    void initLeftView();
    void initRightView();
    void updateRightView();

private:
    QString m_currentAlbum;

    DListWidget* m_pLeftMenuList;
    ThumbnailListView* m_pRightThumbnailList;
};

#endif // ALBUMVIEW_H
