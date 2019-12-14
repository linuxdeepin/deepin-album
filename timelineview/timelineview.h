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
#include "widgets/statusbar.h"
#include "allpicview/allpicview.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <DListWidget>
#include <DStackedWidget>
#include <DCommandLinkButton>
#include <DApplicationHelper>
#include <QGraphicsOpacityEffect>

class Title : public QWidget{
public:
    Title(){}
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
class TimeLineView : public DWidget
{
public:
    TimeLineView();

    void updateStackedWidget();
public slots:
    void on_AddLabel(QString date,QString num);
    void on_DelLabel();
#if 1
    void on_MoveLabel(int y,QString date,QString num,QString choseText);
#endif
    void on_KeyEvent(int key);

protected:
    void resizeEvent(QResizeEvent *ev) override;

private:
    void initTimeLineViewWidget();
    void initConnections();
    void sigImprotPicsIntoThumbnailView();
    void getImageInfos();
    void updataLayout();
    void initMainStackWidget();
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;

    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;

    void mousePressEvent(QMouseEvent *e) override;
public:
    void updatePicNum();
    void updateChoseText();

    void restorePicNum();
    void themeChangeSlot(DGuiApplicationHelper::ColorType themeType);

private:
    TimelineList *m_mainListWidget=nullptr;
    QLayout *m_mainLayout=nullptr;
    QList<QString> m_timelines;
    QWidget *m_dateItem=nullptr;
    DCommandLinkButton *pSuspensionChose;
    DWidget* pTimeLineViewWidget;
    ImportView* pImportView;
    QMap<ThumbnailListView*, QStringList> selpicQmap;
    int allnum;
    DLabel* m_pDate;
    DLabel* pNum_up;
    DLabel* pNum_dn;

    QList<ThumbnailListView*> m_allThumbnailListView;
    QList<DCommandLinkButton*> m_allChoseButton;

    QGraphicsOpacityEffect * m_oe;
    QGraphicsOpacityEffect * m_oet;

    bool m_ctrlPress = false;

public:
    DStackedWidget* m_pStackedWidget;
    StatusBar* m_pStatusBar;
    SearchView* pSearchView;

    QWidget* m_pwidget;

    int m_index;
    int m_selPicNum;
    DSpinner* m_spinner;
};

#endif // TIMELINEVIEW_H
