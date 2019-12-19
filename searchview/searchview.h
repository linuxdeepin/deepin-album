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
#include <DWidget>
#include <DGuiApplicationHelper>
#include <QMouseEvent>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE

class SlideShowButton : public DPushButton
{
    Q_OBJECT
public:
    explicit SlideShowButton(DWidget *parent = 0);
    void mSetText(QString text);
protected:
    void paintEvent(QPaintEvent *event) override;
    void  enterEvent(QEvent *e) override;
    void  leaveEvent(QEvent *e) override;
    void  mouseEvent(QMouseEvent *e);

private:
    qreal m_filletradii;
    bool  israised;
};

class SearchView : public QWidget
{
    Q_OBJECT

public:
    SearchView();
    void improtSearchResultsIntoThumbnailView(QString s, QString album);

private:
    void initConnections();
    void initNoSearchResultView();
    void initSearchResultView();
    void initMainStackWidget();
    void updateSearchResultsIntoThumbnailView();
    void changeTheme();
    void onKeyDelete();

private:
    DStackedWidget *m_stackWidget;
    DWidget *m_pNoSearchResultView;
    DLabel *m_pNoSearchResultLabel;
    DWidget *m_pSearchResultView;
//    DPushButton *m_pSlideShowBtn;
    SlideShowButton *m_pSlideShowBtn;
    DLabel *m_pSearchResultLabel;
    QString m_keywords;
    DLabel *pNoResult;
    DLabel *pLabel1;
    QString m_albumName;

public:
    int m_searchPicNum;
    ThumbnailListView *m_pThumbnailListView;
};

#endif // SEARCHVIEW_H
