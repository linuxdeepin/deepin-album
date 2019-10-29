#ifndef TIMELINEVIEW_H
#define TIMELINEVIEW_H

#include "application.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "widgets/thumbnaillistview.h"
#include "widgets/timelinelist.h"
#include "widgets/timelineitem.h"
#include "importview/importview.h"
#include "searchview/searchview.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <DListWidget>
#include <DStackedWidget>
#include <DCommandLinkButton>
#include <DApplicationHelper>

class Title : public QWidget{
public:
    Title(){};
protected:
    void paintEvent(QPaintEvent *event){

        qDebug() << "x is "<<x();
        qDebug() << "pos.x is "<<pos().x();
    }
    void moveEvent(QMoveEvent *event){
        qDebug() << "moveEvent x is "<<x();
        qDebug() << "moveEvent pos.x is "<<pos().x();
    }

};
class TimeLineView : public DStackedWidget
{
public:
    TimeLineView();

    void updateStackedWidget();
public slots:
    void on_AddLabel(QString date,QString num);
    void on_DelLabel();
    void on_MoveLabel(int y);

protected:
    void resizeEvent(QResizeEvent *ev) override;

private:
    void initTimeLineViewWidget();
    void initConnections();
    void sigImprotPicsIntoThumbnailView();
    void getImageInfos();
    void updataLayout();
    void initMainStackWidget();
    void onPixMapRotate(QStringList paths);
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;

private:
    TimelineList *m_mainListWidget=nullptr;
    QLayout *m_mainLayout=nullptr;
    QList<QString> m_timelines;
    QWidget *m_dateItem=nullptr;
    DCommandLinkButton *pSuspensionChose;
    DWidget* pTimeLineViewWidget;
    ImportView* pImportView;
    SearchView* pSearchView;

    int m_index;
};

#endif // TIMELINEVIEW_H
