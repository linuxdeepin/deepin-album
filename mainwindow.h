#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "albumview/albumview.h"
#include "allpicview/allpicview.h"
#include "timelineview/timelineview.h"
#include "dbmanager/dbmanager.h"
#include "searchview/searchview.h"
#include "importview/importview.h"

#include <DMainWindow>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <DSegmentedControl>
#include <DTitlebar>
#include <QStackedWidget>
#include <DSearchEdit>
#include <DLabel>
#include <QStatusBar>

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
    void showCreateDialog(QStringList imgpaths);

signals:
    void sigImprotPicsIntoDB(const DBImgInfoList &infos);
    void sigTitleMenuImportClicked();

private slots:
    void allPicBtnClicked();
    void timeLineBtnClicked();
    void albumBtnClicked();
    void onCreateAlbum(QStringList imagepaths);
    void onSearchEditFinished();
    void onTitleBarMenuClicked(QAction *action);
    void onUpdateAllpicsNumLabel(int num);
    void onImprotBtnClicked();
    void onUpdateCentralWidget();

private:
    Ui::MainWindow *ui;
    QListWidget* m_listWidget;

    int m_allPicNum;
    int m_iCurrentView;

    QWidget* m_titleBtnWidget;
    DMenu* m_pTitleBarMenu;
    DPushButton* m_pAllPicBtn;
    DPushButton* m_pTimeLineBtn;
    DPushButton* m_pAlbumBtn;
    DSearchEdit* m_pSearchEdit;
    QStackedWidget* m_pCenterWidget;
    AlbumView* m_pAlbumview;
    AllPicView* m_pAllPicView;
    TimeLineView* m_pTimeLineView;
    SearchView* m_pSearchView;
    ImportView* m_pImportView;
    QStatusBar* m_pStatusBar;
    DLabel* m_pAllPicNumLabel;
    DSlider* m_pSlider;
    DBManager* m_pDBManager;
};

#endif // MAINWINDOW_H
