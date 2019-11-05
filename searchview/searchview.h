#ifndef SEARCHVIEW_H
#define SEARCHVIEW_H

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

class SearchView : public QWidget
{
    Q_OBJECT

public:
    SearchView();
    void improtSearchResultsIntoThumbnailView(QString s);

private:
    void initConnections();
    void initNoSearchResultView();
    void initSearchResultView();
    void initMainStackWidget();
    void updateSearchResultsIntoThumbnailView();

private:
    DStackedWidget *m_stackWidget;

    DWidget* m_pNoSearchResultView;
    DLabel* m_pNoSearchResultLabel;

    DWidget* m_pSearchResultView;
    DPushButton* m_pSlideShowBtn;
    DLabel* m_pSearchResultLabel;
    QString m_keywords;

public:
    ThumbnailListView *m_pThumbnailListView;
};

#endif // SEARCHVIEW_H
