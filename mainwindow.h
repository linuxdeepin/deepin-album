#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "albumview/albumview.h"
#include "allpicview/allpicview.h"
#include "timelineview/timelineview.h"

#include <DMainWindow>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <DSegmentedControl>
#include <DTitlebar>
#include <DtkWidgets>
#include <QStackedWidget>

#define DEFAULT_WINDOWS_WIDTH   960
#define DEFAULT_WINDOWS_HEIGHT  540
#define MIX_WINDOWS_WIDTH       650
#define MIX_WINDOWS_HEIGHT      420

DWIDGET_USE_NAMESPACE

namespace Ui {
class MainWindow;
}

class MainWindow : public DMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow();
    ~MainWindow();

    void initConnections();
    void initUI();
    void initTitleBar();
    void initCentralWidget();
    void initStatusBar();

private slots:
    void allPicBtnClicked();
    void timeLineBtnClicked();
    void albumBtnClicked();

private:
    Ui::MainWindow *ui;
    QListWidget* m_listWidget;

    QWidget* m_titleBtnWidget;
    QPushButton* m_allPicBtn;
    QPushButton* m_timeLineBtn;
    QPushButton* m_albumBtn;
    QStackedWidget* m_pCenterWidget;
    AlbumView* m_pAlbumview;
    AllPicView* m_pAllPicView;
    TimeLineView* m_pTimeLineView;
    QStatusBar* m_pStatusBar;
};

#endif // MAINWINDOW_H
