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
#include "controller/commandline.h"
#include "controller/exporter.h"
#include "widgets/dialogs/imginfodialog.h"
#include "fileinotify.h"

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
#define TITLEBAR_BLANK_WIDTH 365
#define TITLEBAR_BTNWIDGET_WIDTH 240
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
    void onShowSlidePanel(int index);
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
    void onEscShortcutActivated();
    void onDelShortcutActivated();
    void onKeyF2ShortcutActivated();
    void onCtrlShiftSlashShortcutActivated();
    void onCtrlFShortcutActivated();
    void onShowImageView(int index);
    void onSearchEditIsDisplay(bool bIsDisp);


#if 1
    void onViewCreateAlbum(QString imgpath, bool bmodel = true);
#endif
    void onSearchEditFinished();
    void onTitleBarMenuClicked(QAction *action);
    void onImprotBtnClicked();
//    void onUpdateCentralWidget();
    void onNewAPPOpen(qint64 pid, const QStringList &arguments);

    void onSigViewImage(const QStringList &paths, const QString &firstPath);
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
    //SlideShowPanel *m_slidePanel;               //幻灯片播放视图  //todo imageviewer
    DSearchEdit *m_pSearchEdit;
//    CommandLine *m_commandLine;  //todo imageviewer
    ImageViewer *m_commandLine = nullptr;  //todo imageviewer
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

    bool m_isFirstStart = true;
    bool m_processOptionIsEmpty = false;
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

public:
    QButtonGroup *getButG();
};

#endif // MAINWINDOW_H
