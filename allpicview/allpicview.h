#ifndef ALLPICVIEW_H
#define ALLPICVIEW_H

#include "application.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "controller/configsetter.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "widgets/thumbnaillistview.h"
#include "importview/importview.h"
#include "searchview/searchview.h"

#include <QWidget>
#include <QVBoxLayout>
#include <DLabel>
#include <QPixmap>
#include <QStandardPaths>
#include <QImageReader>
#include <DPushButton>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <DStackedWidget>
#include <DSlider>
#include <DSpinner>

DWIDGET_USE_NAMESPACE

class AllPicView : public DStackedWidget
{
    Q_OBJECT

public:
    AllPicView();

signals:

private:
    void initConnections();
    void initStackedWidget();
//    void initThumbnailListView();
    void updateStackedWidget();
    void updatePicsIntoThumbnailView();

    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

    void removeDBAllInfos();
private:
    ThumbnailListView* m_pThumbnailListView;
    ImportView* m_pImportView;
    SearchView* m_pSearchView;
    DSpinner* m_spinner=nullptr;
};

#endif // ALLPICVIEW_H
