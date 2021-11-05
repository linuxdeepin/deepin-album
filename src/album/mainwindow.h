/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "albumview/albumview.h"
#include "allpicview/allpicview.h"
#include "timelineview/timelineview.h"
#include "dbmanager/dbmanager.h"
#include "searchview/searchview.h"
#include "controller/exporter.h"
#include "widgets/dialogs/imginfodialog.h"
#include "fileinotify.h"

#include <QListWidget>
#include <QListWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QStackedWidget>
#include <QStatusBar>
#include <QButtonGroup>

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

class MainWindow : public DMainWindow, public ImageEngineImportObject
{
    Q_OBJECT

public:
    //增加单例接口，方便closeFromMenu调用
    static MainWindow &instance()
    {
        static MainWindow w;
        return w;
    }

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
    void startMonitor();
private:
    explicit MainWindow(); //修改为单例后，构造函数也挪走了
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
    void closeFromMenu();
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
    void onHideSlidePanel();
    void onExportImage(QStringList paths);
    void onMainwindowSliderValueChg(int step);
    void onAlbDelToast(QString str1);
    void onAddDuplicatePhotos();
    //成功与已存在数量
    void onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, const QString &albumName);
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
    void onEscShortcutActivated(bool isSwitchFullScreen);
    void onDelShortcutActivated();
    void onKeyF2ShortcutActivated();
    void onCtrlShiftSlashShortcutActivated();
    void onCtrlFShortcutActivated();
    void onSearchEditIsDisplay(bool bIsDisp);
    void onSendAlbumName(const QString &path);
    //主界面创建相册响应槽
    void onViewCreateAlbum(QString imgpath, bool bmodel = true);
    void onSearchEditFinished();
    void onTitleBarMenuClicked(QAction *action);
    void onImprotBtnClicked();
    void onNewAPPOpen(qint64 pid, const QStringList &arguments);

    void onSigViewImage(const SignalManager::ViewInfo &info, OpenImgAdditionalOperation operation, bool isCustom, const QString &album);
    void onCollectButtonClicked();
    void updateCollectButton();
    void onLibDel();
    //响应公共库添加或新建相册请求
    void onAddToAlbum(bool isNew, const QString &album, const QString &path);
    //响应公共库收藏/取消收藏操作
    void onAddOrRemoveToFav(const QString &path, bool isAdd);
    //响应公共库导出操作
    void onExport(const QString &path);
    //响应公共库：从相册中移除操作
    void onRemoveFromCustom(const QString &path, const QString &album);
    //响应公共库：旋转请求
    void onRotatePic(const QString &path);

private:
    int m_iCurrentView;
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
    DSearchEdit *m_pSearchEdit;
    ImageViewer *m_imageViewer = nullptr;       //公共库大图预览
    DIconButton *m_back = nullptr;
    DIconButton *m_collect = nullptr;
    DIconButton *m_del = nullptr;

private:
    int m_backIndex;
    int m_backIndex_fromFullScreen;
    bool m_needBanShortcutButNotReady = false;
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

    bool m_isFirstStart = true;
    bool m_processOptionIsEmpty = false;
    bool m_backToMaxWindow = false; //记录全屏前是normal还是max
    QSettings *m_settings;
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
    FileInotify *m_fileInotify;//固定文件夹监控

    // 添加图片按钮
    DIconButton         *m_addImageBtn = nullptr;

    //启动看图插件后需要关闭的快捷键
    QShortcut *m_CtrlUp;
    QShortcut *m_ReCtrlUp;
    QShortcut *m_CtrlDown;

public:
    QButtonGroup *getButG();
};

#endif // MAINWINDOW_H
