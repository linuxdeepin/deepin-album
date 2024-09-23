// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "albumview/albumview.h"
#include "allpicview/allpicview.h"
#include "timelineview/timelineview.h"
#include "dbmanager/dbmanager.h"
#include "searchview/searchview.h"
#include "controller/exporter.h"
#include "widgets/dialogs/imginfodialog.h"
#include "widgets/flatbutton.h"
#include "fileinotifygroup.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QStackedWidget>
#include <QStatusBar>
#include <QButtonGroup>
#include <QDBusInterface>

#include <DMainWindow>
#include <DTitlebar>
#include <DSearchEdit>
#include <DLabel>
#include <DTabBar>
#include <DSuggestButton>
#include <DProgressBar>

#define DEFAULT_WINDOWS_WIDTH   1300
#define DEFAULT_WINDOWS_HEIGHT  640
#define MIX_WINDOWS_WIDTH       1300
#define MIX_WINDOWS_HEIGHT      848
#define TITLEBAR_BLANK_WIDTH 365
#define TITLEBAR_BTNWIDGET_WIDTH 240
#define TITLEBAR_ICON_WIDTH 50

DWIDGET_USE_NAMESPACE

extern bool bfirstopen;
extern bool bfirstandviewimage;

class QShortcut;

class DDialogEx : public DDialog
{
    Q_OBJECT

public:
    explicit DDialogEx(QWidget *parent = nullptr): DDialog (parent) {};
    ~DDialogEx() {};

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
};

class MainWindow : public DMainWindow, public ImageEngineImportObject
{
    Q_OBJECT

    enum ProgressType{
        Progress_Unknown = -1,
        Progress_Import,
        Progress_Delete,
        Progress_Classify,
    };

public:
    explicit MainWindow();
    ~MainWindow() override;

    bool imageImported(bool success) override;
    void initConnections();
    //初始化UI
    void initUI();
    void initWaitDialog();
    void initShortcut();
    void initShortcutKey();
    void initTitleBar();
    void initCentralWidget();
    // install filter
    void initInstallFilter();
    // 设置初始启动,没有图片时的allpicview tab切换顺序
    void initNoPhotoNormalTabOrder();
    // 设置有图片时的allpicview tab tab切换顺序
    void initAllpicViewTabOrder();
    // 设置有图片时的allpicview tab tab切换顺序
    void initTimeLineViewTabOrder();
    // 设置有图片时的AlbumView tab tab切换顺序
    void initAlbumViewTabOrder();
    // enter key
    void initEnterkeyAction(QObject *obj);
    // right key
    bool initRightKeyOrder(QObject *obj);
    // left key
    bool initLeftKeyOrder(QObject *obj);
    // down key
    bool initDownKeyOrder();
    // up key
    bool initUpKeyOrder();
    // all view tab key order
    bool initAllViewTabKeyOrder(QObject *obj);

    void setWaitDialogColor();
    int getCurrentViewType();
    void showCreateDialog(QStringList imgpaths);
//    void onShowImageInfo(const QString &path);
    void loadZoomRatio();
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;
    void showEvent(QShowEvent *event) Q_DECL_OVERRIDE;
    QJsonObject createShorcutJson();
    //开始监控
    void startMonitor(const QList<QStringList> &paths, const QStringList &albumNames, const QList<int> &UIDs);

    bool isTitleBarVisible() const;

private:
    void thumbnailZoomIn();
    void thumbnailZoomOut();
    void saveWindowState();
    void loadWindowState();
    void saveZoomRatio();
    bool compareVersion();
//    void viewImageClose();
    void floatMessage(const QString &str, const QIcon &icon);
    void setTitleBarHideden(bool hide);
    void setConflictShortcutEnabled(bool enable);
    bool processOption(QStringList &paslist);
    void waitImportantProcessBeforeExit();

    // 字体改变、尺寸改变，同步调整标题栏区域控件显示大小
    void adjustTitleContent();
    /**
     * @brief pathControl 返回输入sPath文件是否被读写权限管控
     */
    bool pathControl(const QString &sPath);
protected:
    void wheelEvent(QWheelEvent *event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *e) Q_DECL_OVERRIDE;
    bool eventFilter(QObject *obj, QEvent *event) Q_DECL_OVERRIDE;

signals:
    void sigTitleMenuImportClicked();
    bool sigImageImported(bool success);

public slots:
    void allPicBtnClicked();
    void timeLineBtnClicked();
    void albumBtnClicked();
    void onCreateAlbum(const QStringList &imagepaths);
    void onButtonClicked(int id);
    void onImageImported(bool success);
    //void onSearchEditTextChanged(QString text);
    void onImagesInserted();
    void onStartImprot();
    void onProgressOfWaitDialog(int allfiles, int completefiles);
    void onPopupWaitDialog(QString waittext, bool bneedprogress);
    void onCloseWaitDialog();
    void onImagesRemoved();
    void onHideImageView();
    void onAddImageBtnClicked(bool checked);
    void onHideFromFullScreen();
    void onExportImage(QStringList paths);
    void onMainwindowSliderValueChg(int step);
    void onAlbDelToast(QString str1);
    void onAddDuplicatePhotos();
    //成功与已存在数量
    void onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, int UID);
    void onAddToAlbToast(QString album);
    void onImportSuccess();
    void onSearchEditClear();
    void onImportFailed();
    void onImportDonotFindPicOrVideo();
    void onImportSomeFailed(int successful, int failed);
    void onImgExportFailed();
    void onImgExportSuccess();
    void onAlbExportFailed();
    void onAlbExportSuccess();
    void onNotifyPathIsExists();
    void onNotSupportedNotifyPath();
    void onImportInterrupted();
    void onEscShortcutActivated(bool isSwitchFullScreen);
    void onDelShortcutActivated();
    void onKeyF2ShortcutActivated();
    void onCtrlShiftSlashShortcutActivated();
    void onCtrlFShortcutActivated();
    void onSearchEditIsDisplay(bool bIsDisp);
    void onSendAlbumName(const QString &path);
    void onRestoreFailed(const QStringList &failedFiles);
    //主界面创建相册响应槽
    void onViewCreateAlbum(QString imgpath, bool bmodel = true);
    void onSearchEditFinished();
    void onTitleBarMenuClicked(QAction *action);
    void onImprotBtnClicked();
    void onNewAPPOpen(qint64 pid, const QStringList &arguments);
    void ImportImagesFromCustomAutoPaths(); //从自定义自动导入路径刷新图片
    void onSigViewImage(const SignalManager::ViewInfo &info, OpenImgAdditionalOperation operation, bool isCustom, const QString &album, int UID);
    void onCollectButtonClicked();
    void updateCollectButton();
    void onLibDel(QString path);
    void deleteSaveImage();
    void onNewPathAction(); //响应新自定义路径创建
    void createAlbumView(); //创建相册界面
    //响应公共库添加或新建相册请求
    void onAddToAlbum(bool isNew, int UID, const QString &path);
    //响应公共库收藏/取消收藏操作
    void onAddOrRemoveToFav(const QString &path, bool isAdd);
    //响应公共库导出操作
    void onExport(const QString &path);
    //响应公共库：从相册中移除操作
    void onRemoveFromCustom(const QString &path, int UID);
    //响应公共库：旋转请求
    void onRotatePic(const QString &path);
    //设置windowtitle
    void setWindowTitleInfo();

private:
    int m_iCurrentView;
    ProgressType m_progressType = Progress_Unknown;
    bool m_bImport = false;
    QWidget *m_titleBtnWidget = nullptr;
    DMenu *m_pTitleBarMenu = nullptr;
    QStackedWidget *m_pCenterWidget = nullptr;
public:
    AlbumView *m_pAlbumview = nullptr;                    //相册照片界面视图
    QWidget *m_pAlbumWidget = nullptr;
    AllPicView *m_pAllPicView = nullptr;                  //所有照片界面视图
    TimeLineView *m_pTimeLineView = nullptr;              //时间线界面视图
    QWidget *m_pTimeLineWidget = nullptr;
    SearchView *m_pSearchView = nullptr;                  //搜索界面视图
    QWidget *m_pSearchViewWidget = nullptr;
    DSearchEdit *m_pSearchEdit = nullptr;
    ImageViewer *m_imageViewer = nullptr;       //公共库大图预览
    DIconButton *m_back = nullptr;
    DIconButton *m_collect = nullptr;
    DIconButton *m_del = nullptr;

private:
    int m_backIndex = 0;
    int m_backIndex_fromFullScreen = 0;
    bool m_needBanShortcutButNotReady = false;
    int m_pSliderPos = 2;       //滑动条步进

    QButtonGroup *btnGroup = nullptr;
    FlatButton *m_pAllPicBtn = nullptr;
    FlatButton *m_pTimeBtn = nullptr;
    FlatButton *m_pAlbumBtn = nullptr;

    DDialogEx  *m_waitdailog = nullptr;
    DProgressBar *m_importBar = nullptr;
    DLabel *m_waitlabel = nullptr;
    DLabel *m_countLabel = nullptr;

    QTimer *m_tdeleteSaveImage = nullptr; //预览delete延时
    DBImgInfoList m_deleteInfo;//删除缓存

    QString       m_SearchKey;      //搜索框查询信息

    bool m_isFirstStart = true;
    bool m_processOptionIsEmpty = false;
    bool m_backToMaxWindow = false; //记录全屏前是normal还是max
    QSettings *m_settings = nullptr;
    // 所有照片空白界面时的taborder
    QList<QWidget *> m_emptyAllViewTabOrder;
    // 所有照片非空白界面时
    QList<QWidget *> m_AllpicViewTabOrder;
    // 时间线非空白界面时
    QList<QWidget *> m_TimelineViewTabOrder;
    // 相册非空白界面时
    QList<QWidget *> m_AlbumViewTabOrder;
    // isfirst init
    QVector<bool> m_bVector;
    FileInotifyGroup *m_fileInotifygroup;//固定文件夹监控

    // 添加图片按钮
    DIconButton         *m_addImageBtn = nullptr;

    //启动看图插件后需要关闭的快捷键
    QShortcut *m_CtrlUp = nullptr;
    QShortcut *m_ReCtrlUp = nullptr;
    QShortcut *m_CtrlDown = nullptr;

    bool m_bTitleBarVisible = true;      // 标题栏是否可见标识

    MainWindow(const MainWindow &) = delete;
    MainWindow operator=(const MainWindow &) = delete;

public:
    QButtonGroup *getButG();
};

#endif // MAINWINDOW_H
