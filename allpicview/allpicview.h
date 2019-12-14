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
#include "widgets/statusbar.h"
#include "widgets/statusbar.h"

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

class AllPicView : public DWidget
{
    Q_OBJECT

public:
    AllPicView();

    void updateStackedWidget();
    void restorePicNum();
    void updatePicNum();

private:
    void initConnections();
    void initStackedWidget();
//    void initThumbnailListView();
    void updatePicsIntoThumbnailView();
    void onUpdateAllpicsNumLabel();

    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private slots:

public:
    DStackedWidget* m_pStackedWidget;
    StatusBar* m_pStatusBar;

    QWidget* m_pwidget;

    int step;

private:
    ThumbnailListView* m_pThumbnailListView;
    ImportView* m_pImportView;
    SearchView* m_pSearchView;
    DSpinner* m_spinner;
};

#endif // ALLPICVIEW_H
