#ifndef ALLPICVIEW_H
#define ALLPICVIEW_H

#include "application.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "controller/configsetter.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "widgets/thumbnaillistview.h"


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

DWIDGET_USE_NAMESPACE

class AllPicView : public QWidget
{
    Q_OBJECT

public:
    AllPicView();

signals:
    void sigImprotPicsIntoDB(const DBImgInfoList &infos);

private:
    void initConnections();
    void initImportFrame();
    void initThumbnailListView();
    void initMainStackWidget();
    void updateMainStackWidget();

    void improtBtnClicked();

    void sigImprotPicsIntoThumbnailView();



    void removeDBAllInfos();
private:
    DStackedWidget *m_stackWidget = NULL;

    DWidget* m_pImportFrame;
    DPushButton* m_pImportBtn;

    ThumbnailListView *m_pThumbnailListView;
};

#endif // ALLPICVIEW_H
