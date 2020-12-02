#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "albumview/albumview.h"
#include "allpicview/allpicview.h"
#include "timelineview/timelineview.h"
#include "dbmanager/dbmanager.h"
#include "searchview/searchview.h"
#include "controller/commandline.h"
#include "controller/exporter.h"
#include "controller/dbusclient.h"
#include "widgets/dialogs/imginfodialog.h"
#include "module/slideshow/slideshowpanel.h"

#include <DMainWindow>
#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
//#include <DSegmentedControl>
#include <DTitlebar>
#include <QStackedWidget>
#include <DSearchEdit>
#include <DLabel>
#include <QStatusBar>
#include <DTabBar>
#include <QButtonGroup>
#include <DSuggestButton>
#include <DProgressBar>

#define DEFAULT_WINDOWS_WIDTH   1300
#define DEFAULT_WINDOWS_HEIGHT  640
#define MIX_WINDOWS_WIDTH       1300
#define MIX_WINDOWS_HEIGHT      848
//#define TITLEBAR_BLANK_WIDTH    235
#define TITLEBAR_BLANK_WIDTH 365
#define TITLEBAR_BTNWIDGET_WIDTH 240
//#define TITLEBAR_BTNWIDGET_WIDTH 280
#define TITLEBAR_ICON_WIDTH 50

DWIDGET_USE_NAMESPACE

extern bool bfirstopen;
extern bool bfirstandviewimage;
//namespace Ui {
//class MainWindow;
//}
class MainWindow : public DMainWindow, public ImageEngineImportObject
{
    Q_OBJECT

public:
    explicit MainWindow();
    ~MainWindow() override;

    bool imageImported(bool success) override;
    void initConnections();
    void initDBus() ;//lmh0407
    //初始化UI
    void initUI();
    void initWaitDialog();
    void initShortcut();
    void initShortcutKey();
    void initTitleBar();
    void initCentralWidget();
    void setWaitDialogColor();
    int getCurrentViewType();
    void showCreateDialog(QStringList imgpaths);
    void onShowImageInfo(const QString &path);
    void loadZoomRatio();
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    QJsonObject createShorcutJson();
private:
    void thumbnailZoomIn();
    void thumbnailZoomOut();
    void saveWindowState();
    void loadWindowState();
    void saveZoomRatio();
    bool compareVersion();
//    void viewImageClose();
    void floatMessage(const QString &str, const QIcon &icon);
protected:
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;

signals:
    void sigTitleMenuImportClicked();
    bool sigImageImported(bool success);

public slots:
    void allPicBtnClicked();
    void timeLineBtnClicked();
    void albumBtnClicked();
    void onCreateAlbum(QStringList imagepaths);
    void closeFromMenu();
#if 1
    void onViewCreateAlbum(QString imgpath, bool bmodel = true);
#endif
    void onSearchEditFinished();
    void onTitleBarMenuClicked(QAction *action);
    void onImprotBtnClicked();
//    void onUpdateCentralWidget();
    void onNewAPPOpen(qint64 pid, const QStringList &arguments);
    void onLoadingFinished();
private:
    int m_iCurrentView;
    bool m_bTitleMenuImportClicked;
    bool m_bImport = false;
    QWidget *m_titleBtnWidget;
    DMenu *m_pTitleBarMenu;
    QStackedWidget *m_pCenterWidget = nullptr;
public:
    AlbumView *m_pAlbumview;                    //相册照片界面视图
    QWidget *m_pAlbumWidget;
    AllPicView *m_pAllPicView;                  //所有照片界面视图
    TimeLineView *m_pTimeLineView;              //时间线界面视图
    QWidget *m_pTimeLineWidget;
    SearchView *m_pSearchView;                  //搜索界面视图
    QWidget *m_pSearchViewWidget = nullptr;
    SlideShowPanel *m_slidePanel;               //幻灯片播放视图
    DSearchEdit *m_pSearchEdit;
    CommandLine *m_commandLine;
private:
    DBManager *m_pDBManager;
    QMap<QString, ImgInfoDialog *> m_propertyDialogs{};
    int m_backIndex;
    int m_backIndex_fromSlide;
    int m_pSliderPos;       //滑动条步进

    QButtonGroup *btnGroup;
    DPushButton *m_pAllPicBtn;
    DPushButton *m_pTimeBtn;
    DPushButton *m_pAlbumBtn;

    DDialog  *m_waitdailog;
    DProgressBar *m_importBar;
    DLabel *m_waitlabel;
    DLabel *m_countLabel;

    QString       m_SearchKey;      //搜索框查询信息

    dbusclient *m_pDBus;//LMH0407DBus
    bool m_isFirstStart = true;
    bool m_processOptionIsEmpty = false;
    QSettings *m_settings;
public:
    QButtonGroup *getButG();
};

#endif // MAINWINDOW_H
