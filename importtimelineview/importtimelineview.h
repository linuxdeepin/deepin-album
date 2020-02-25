#ifndef IMPORTTIMELINEVIEW_H
#define IMPORTTIMELINEVIEW_H

#include "application.h"
#include "controller/signalmanager.h"
#include "dbmanager/dbmanager.h"
#include "thumbnail/thumbnaillistview.h"
#include "widgets/timelinelist.h"
#include "widgets/timelineitem.h"
#include "importview/importview.h"
#include "widgets/statusbar.h"
#include "allpicview/allpicview.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QScrollArea>
#include <DListWidget>
#include <DCommandLinkButton>
#include <DApplicationHelper>
#include <QGraphicsOpacityEffect>

class ImportTimeLineView : public DWidget, public ImageEngineImportObject
{
    Q_OBJECT
public:
    ImportTimeLineView(DWidget *parent);
    ~ImportTimeLineView()
    {
        void clearAndStop();
    }

    bool imageImported(bool success) override
    {
        emit dApp->signalM->closeWaitDialog();
        return true;
    }
    void updateStackedWidget();
//signals:
//    void albumviewResize();
public slots:
    void on_AddLabel(QString date, QString num);
    void on_DelLabel();
#if 1
    void on_MoveLabel(int y, QString date, QString num, QString choseText);
#endif
    void on_KeyEvent(int key);

protected:
    void resizeEvent(QResizeEvent *ev) override;

private:
    void initTimeLineViewWidget();
    void initConnections();
    void sigImprotPicsIntoThumbnailView();
    void getImageInfos();

    void initMainStackWidget();
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;

    void keyPressEvent(QKeyEvent *e) override;
    void keyReleaseEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
public:
//    void updataLayout();
    void clearAndStartLayout();
    void addTimelineLayout();
    void themeChangeSlot(DGuiApplicationHelper::ColorType themeType);
#if 1
    QStringList selectPaths();
    void updateChoseText();
#endif
signals:
    void sigUpdatePicNum();

private:
    void clearAndStop();
    QLayout *m_mainLayout = nullptr;
    QList<QString> m_timelines;
    DWidget *m_dateItem = nullptr;
    DCommandLinkButton *pSuspensionChose;
    DWidget *pTimeLineViewWidget;
    ImportView *pImportView;
    QMap<ThumbnailListView *, QStringList> selpicQmap;
    int allnum;
    DLabel *m_pDate;
    DLabel *pNum_up;
    DLabel *pNum_dn;
    DLabel *m_pImportTitle; //add 3975
    QList<ThumbnailListView *> m_allThumbnailListView;
    QList<DCommandLinkButton *> m_allChoseButton;

    QGraphicsOpacityEffect *m_oe;
    QGraphicsOpacityEffect *m_oet;

    bool m_ctrlPress;

    int lastClickedIndex;
    int lastRow = -1;
    int m_lastShiftRow = -1;
    int m_lastShiftClickedIndex = -1;
    bool lastChanged = false;
public:
    int m_index;
    int m_selPicNum;
    TimelineList *m_mainListWidget = nullptr;
    int currentTimeLineLoad = 0;
};

#endif // IMPORTTIMELINEVIEW_H
