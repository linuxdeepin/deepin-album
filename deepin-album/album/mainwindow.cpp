#include "mainwindow.h"
#include "config.h"
#include "controller/commandline.h"
#include "dialogs/albumcreatedialog.h"
#include "utils/unionimage.h"
#include "imageengine/imageengineapi.h"
#include "accessibledefine.h"
#include "viewerthememanager.h"
#include "ac-desktop-define.h"

#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <dgiovolume.h>
#include <QShortcut>
#include <DTableView>
#include <DApplicationHelper>
#include <DFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <DApplication>
#include <QDesktopWidget>
#include <DMessageManager>
#include <DFloatingMessage>
#include <DWidgetUtil>
#include <DStandardPaths>
bool bfirstopen = true;
bool bfirstandviewimage = false;
namespace  {
const int VIEW_ALLPIC = 0;
const int VIEW_TIMELINE = 1;
const int VIEW_ALBUM = 2;
const int VIEW_SEARCH = 3;
const int VIEW_IMAGE = 4;
const int VIEW_SLIDE = 5;

//const QString TITLEBAR_NEWALBUM = "新建相册";
//const QString TITLEBAR_IMPORT = "导入照片";
//const QString TITLEBAR_NEWALBUM = "New album";
//const QString TITLEBAR_IMPORT = "Import photos";

}//namespace

using namespace utils::common;
MainWindow::MainWindow()
    : m_iCurrentView(VIEW_ALLPIC)
    , m_bTitleMenuImportClicked(false)
    , m_bImport(false)
    , m_titleBtnWidget(nullptr)
    , m_pTitleBarMenu(nullptr)
    , m_pCenterWidget(nullptr)
    , m_pAlbumview(nullptr)
    , m_pAlbumWidget(nullptr)
    , m_pAllPicView(nullptr)
    , m_pTimeLineView(nullptr)
    , m_pTimeLineWidget(nullptr)
    , m_pSearchView(nullptr)
    , m_pSearchEdit(nullptr)
    , m_commandLine(nullptr)
    , m_pDBManager(nullptr)
    , m_backIndex(0)
    , m_backIndex_fromSlide(0)
    , m_pSliderPos(2)
    , btnGroup(nullptr)
    , m_pAllPicBtn(nullptr)
    , m_pTimeBtn(nullptr)
    , m_pAlbumBtn(nullptr)
    , m_waitdailog(nullptr)
    , m_importBar(nullptr)
    , m_waitlabel(nullptr)
    , m_countLabel(nullptr)
    , m_pDBus(nullptr)
    , m_settings(nullptr)
{
    this->setObjectName("drawMainWindow");
//    QString userConfigPath = DStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
//                             + "/config.conf";
//    m_settings = new QSettings(userConfigPath, QSettings::IniFormat);
    initShortcutKey();          //初始化各种快捷键
    initUI();
    initTitleBar();             //初始化顶部状态栏
    initCentralWidget();
    //性能优化，此句在构造时不需要执行，增加启动时间,放在showevent之后队列执行
    loadZoomRatio();

    connect(dApp->signalM, &SignalManager::showImageView, this, &MainWindow::onShowImageView);
}

MainWindow::~MainWindow()
{
//    delete m_pAlbumview;                    //相册照片界面视图
//    delete m_pAllPicView;                   //所有照片界面视图
//    delete m_pTimeLineView;                 //时间线界面视图
//    delete m_pSearchView;                   //搜索界面视图
    emit dApp->signalM->sigPauseOrStart(false); //唤醒外设后台挂载,防止析构时线程挂起卡住页面无法退出
    ImageEngineApi::instance()->close();
    QThreadPool::globalInstance()->clear();
    QThreadPool::globalInstance()->waitForDone();
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);
    if (m_pCenterWidget)
        m_pCenterWidget->setFixedSize(size());

    int m_SearchEditWidth = titlebar()->width() - m_titleBtnWidget->width() - TITLEBAR_BLANK_WIDTH;
    if (m_SearchEditWidth <= 350) {
        m_pSearchEdit->setFixedSize(m_SearchEditWidth - 20, 36);
    } else {
        m_SearchEditWidth = 350;
        m_pSearchEdit->setFixedSize(m_SearchEditWidth, 36);
    }
}

//初始化所有连接
void MainWindow::initConnections()
{
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList &");
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &MainWindow::setWaitDialogColor);
    //主界面切换（所有照片、时间线、相册）
    connect(btnGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &MainWindow::onButtonClicked);
    connect(dApp->signalM, &SignalManager::updatePicView, btnGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked));
    //图片导入槽函数
    connect(this, &MainWindow::sigImageImported, this, &MainWindow::onImageImported);
    connect(dApp->signalM, &SignalManager::createAlbum, this, &MainWindow::onCreateAlbum);
#if 1
    connect(dApp->signalM, &SignalManager::viewCreateAlbum, this, &MainWindow::onViewCreateAlbum);
#endif
    connect(m_pSearchEdit, &DSearchEdit::editingFinished, this, &MainWindow::onSearchEditFinished);
    connect(m_pSearchEdit, &DSearchEdit::textChanged, this, &MainWindow::onSearchEditTextChanged);
    connect(m_pTitleBarMenu, &DMenu::triggered, this, &MainWindow::onTitleBarMenuClicked);
    connect(this, &MainWindow::sigTitleMenuImportClicked, this, &MainWindow::onImprotBtnClicked);
    //当有图片添加时，搜索栏可用
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &MainWindow::onImagesInserted);
    //开始导入时，清空状态
    connect(dApp->signalM, &SignalManager::startImprot, this, &MainWindow::onStartImprot);
    //更新等待进度条
    connect(dApp->signalM, &SignalManager::progressOfWaitDialog, this, &MainWindow::onProgressOfWaitDialog);
    //更新等待窗口文本信息
    connect(dApp->signalM, &SignalManager::popupWaitDialog, this, &MainWindow::onPopupWaitDialog);
    //关闭等待窗口
    connect(dApp->signalM, &SignalManager::closeWaitDialog, this, &MainWindow::onCloseWaitDialog);
    //图片移除，判断是否更新搜索框
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &MainWindow::onImagesRemoved);
    //隐藏图片视图
    connect(dApp->signalM, &SignalManager::hideImageView, this, &MainWindow::onHideImageView);
    //幻灯片显示
    connect(dApp->signalM, &SignalManager::showSlidePanel, this, &MainWindow::onShowSlidePanel);
    //隐藏幻灯片显示
    connect(dApp->signalM, &SignalManager::hideSlidePanel, this, &MainWindow::onHideSlidePanel);
    //导出图片
    connect(dApp->signalM, &SignalManager::exportImage, this, &MainWindow::onExportImage);
    connect(dApp->signalM, &SignalManager::showImageInfo, this, &MainWindow::onShowImageInfo);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::newProcessInstance, this, &MainWindow::onNewAPPOpen);
    connect(dApp, &Application::sigFinishLoad, this, &MainWindow::onLoadingFinished);
    //右下角滑动条
    connect(dApp->signalM, &SignalManager::sigMainwindowSliderValueChg, this, &MainWindow::onMainwindowSliderValueChg);
    connect(dApp->signalM, &SignalManager::sigAlbDelToast, this, &MainWindow::onAlbDelToast);
    // 添加到相册底部提示
    connect(dApp->signalM, &SignalManager::sigAddDuplicatePhotos, this, &MainWindow::onAddDuplicatePhotos);
    // 添加到相册底部提示
    connect(dApp->signalM, &SignalManager::sigAddToAlbToast, this, &MainWindow::onAddToAlbToast);
    //底部，弹出导入成功提示框
    connect(dApp->signalM, &SignalManager::ImportSuccess, this, &MainWindow::onImportSuccess);
    connect(dApp->signalM, &SignalManager::SearchEditClear, this, &MainWindow::onSearchEditClear);
    //导入失败提示框
    connect(dApp->signalM, &SignalManager::ImportFailed, this, &MainWindow::onImportFailed);
    //部分导入失败提示框
    connect(dApp->signalM, &SignalManager::ImportSomeFailed, this, &MainWindow::onImportSomeFailed);
    //图片导出失败提示框
    connect(dApp->signalM, &SignalManager::ImgExportFailed, this, &MainWindow::onImgExportFailed);
    //图片导出成功提示框
    connect(dApp->signalM, &SignalManager::ImgExportSuccess, this, &MainWindow::onImgExportSuccess);
    //相册导出失败提示框
    connect(dApp->signalM, &SignalManager::AlbExportFailed, this, &MainWindow::onAlbExportFailed);
    //相册导出成功提示框
    connect(dApp->signalM, &SignalManager::AlbExportSuccess, this, &MainWindow::onAlbExportSuccess);
}
//初始化DBus
void MainWindow::initDBus()
{
    m_pDBus = new dbusclient();
}

//初始化快捷键
void MainWindow::initShortcut()
{
    QShortcut *esc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    esc->setContext(Qt::WindowShortcut);
    connect(esc, &QShortcut::activated, this, &MainWindow::onEscShortcutActivated);

    // Album View画面按DEL快捷键
    QShortcut *del = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    del->setContext(Qt::ApplicationShortcut);
    connect(del, &QShortcut::activated, this, &MainWindow::onDelShortcutActivated);

    // Album View画面按F2快捷键
    QShortcut *keyF2 = new QShortcut(QKeySequence(Qt::Key_F2), this);
    keyF2->setContext(Qt::ApplicationShortcut);
    connect(keyF2, &QShortcut::activated, this, &MainWindow::onKeyF2ShortcutActivated);

    //Ctrl+Up 缩略图放大
    QShortcut *CtrlUp = new QShortcut(QKeySequence(CTRLUP_SHORTCUT), this);
    CtrlUp->setContext(Qt::ApplicationShortcut);
    connect(CtrlUp, &QShortcut::activated, this, &MainWindow::thumbnailZoomIn);

    QShortcut *ReCtrlUp = new QShortcut(QKeySequence(RECTRLUP_SHORTCUT), this);
    ReCtrlUp->setContext(Qt::ApplicationShortcut);
    connect(ReCtrlUp, &QShortcut::activated, this, &MainWindow::thumbnailZoomIn);

    //Ctrl+Down 缩略图缩小
    QShortcut *CtrlDown = new QShortcut(QKeySequence(CTRLDOWN_SHORTCUT), this);
    CtrlDown->setContext(Qt::ApplicationShortcut);
    connect(CtrlDown, &QShortcut::activated, this, &MainWindow::thumbnailZoomOut);

    //Ctrl+Shift+/ 显示快捷键预览
    QShortcut *CtrlShiftSlash = new QShortcut(QKeySequence(CTRLSHIFTSLASH_SHORTCUT), this);
    CtrlShiftSlash->setContext(Qt::ApplicationShortcut);
    connect(CtrlShiftSlash, &QShortcut::activated, this, &MainWindow::onCtrlShiftSlashShortcutActivated);

    //Ctrl+F/ 搜索
    QShortcut *CtrlF = new QShortcut(QKeySequence(CTRLF_SHORTCUT), this);
    CtrlF->setContext(Qt::ApplicationShortcut);
    connect(CtrlF, &QShortcut::activated, this, &MainWindow::onCtrlFShortcutActivated);
}

//初始化UI
void MainWindow::initUI()
{
    initWaitDialog();
    setMinimumSize(880, 500);
    resize(1300, 848);
    loadWindowState();
}

//初始化等待窗口
void MainWindow::initWaitDialog()
{
    m_waitdailog = new DDialog(this);

    m_waitdailog->setCloseButtonVisible(false);
    m_waitdailog->setWindowModality(Qt::WindowModal);
    m_waitdailog->setFixedSize(QSize(480, 93));

    m_waitlabel = new DLabel(m_waitdailog);
    m_waitlabel->setFixedSize(400, 30);//160,30
    m_waitlabel->move(40, 7);
    DFontSizeManager::instance()->bind(m_waitlabel, DFontSizeManager::T5, QFont::Medium);

    m_countLabel = new DLabel(m_waitdailog);
    m_countLabel->setFixedSize(350, 26);
    m_countLabel->move(40, 37);
    DFontSizeManager::instance()->bind(m_countLabel, DFontSizeManager::T6, QFont::DemiBold);

    m_importBar = new DProgressBar(m_waitdailog);
    m_importBar->setFixedSize(400, 6);
    m_importBar->move(40, 67);

    setWaitDialogColor();

}

//初始化顶部状态栏
void MainWindow::initTitleBar()
{
    // TitleBar Button
    if (m_titleBtnWidget) {
        delete  m_titleBtnWidget;
        m_titleBtnWidget = nullptr;
    }
    m_titleBtnWidget = new DWidget();
    AC_SET_OBJECT_NAME(m_titleBtnWidget, MainWindow_TitleBtn_Widget);
    AC_SET_ACCESSIBLE_NAME(m_titleBtnWidget, MainWindow_TitleBtn_Widget);
    btnGroup = new QButtonGroup();
    btnGroup->setExclusive(true);
    QHBoxLayout *pTitleBtnLayout = new QHBoxLayout();

    m_pAllPicBtn = new DPushButton();
    AC_SET_OBJECT_NAME(m_pAllPicBtn, Main_All_Picture_Button);
    AC_SET_ACCESSIBLE_NAME(m_pAllPicBtn, Main_All_Picture_Button);

//    m_pAllPicBtn = new DSuggestButton();
    m_pAllPicBtn->setFlat(true);
//    m_pAllPicBtn->setFixedSize(80, 36);
    m_pAllPicBtn->setMaximumSize(110, 36);
    m_pAllPicBtn->setCheckable(true);
    m_pAllPicBtn->setChecked(true);
    m_pAllPicBtn->setText(tr("All Photos"));
    btnGroup->addButton(m_pAllPicBtn, 0);
    DFontSizeManager::instance()->bind(m_pAllPicBtn, DFontSizeManager::T6);
    pTitleBtnLayout->addWidget(m_pAllPicBtn);

    m_pTimeBtn = new DPushButton();
    AC_SET_OBJECT_NAME(m_pTimeBtn, Main_Time_Line_Button);
    AC_SET_ACCESSIBLE_NAME(m_pTimeBtn, Main_Time_Line_Button);
//    m_pTimeBtn = new DSuggestButton();
    m_pTimeBtn->setFlat(true);
//    m_pTimeBtn->setFixedSize(60, 36);
    m_pTimeBtn->setMaximumSize(110, 36);
    m_pTimeBtn->setCheckable(true);
    m_pTimeBtn->setText(tr("Timelines"));
    btnGroup->addButton(m_pTimeBtn, 1);
    DFontSizeManager::instance()->bind(m_pTimeBtn, DFontSizeManager::T6);
    pTitleBtnLayout->addSpacing(-6);
    pTitleBtnLayout->addWidget(m_pTimeBtn);

    m_pAlbumBtn = new DPushButton();
    AC_SET_OBJECT_NAME(m_pAlbumBtn, Main_Album_Button);
    AC_SET_ACCESSIBLE_NAME(m_pAlbumBtn, Main_Album_Button);
//    m_pAlbumBtn = new DSuggestButton();
    m_pAlbumBtn->setFlat(true);
//    m_pAlbumBtn->setFixedSize(60, 36);
    m_pAlbumBtn->setMaximumSize(90, 36);
    m_pAlbumBtn->setCheckable(true);
    m_pAlbumBtn->setText(tr("Albums"));
    btnGroup->addButton(m_pAlbumBtn, 2);
    DFontSizeManager::instance()->bind(m_pTimeBtn, DFontSizeManager::T6);
    pTitleBtnLayout->addSpacing(-6);
    pTitleBtnLayout->addWidget(m_pAlbumBtn);

    m_titleBtnWidget->setLayout(pTitleBtnLayout);

    // TitleBar Search
    //QWidget *m_titleSearchWidget = new QWidget();
//    QHBoxLayout *pTitleSearchLayout = new QHBoxLayout();
    m_pSearchEdit = new DSearchEdit();
//    m_pSearchEdit->setFixedSize(350, 36);
    m_pSearchEdit->setMaximumSize(350, 36);
    if (0 < DBManager::instance()->getImgsCount()) {
        m_pSearchEdit->setEnabled(true);
    } else {
        m_pSearchEdit->setEnabled(false);
    }

//    pTitleSearchLayout->addWidget(m_pSearchEdit);
    //m_titleSearchWidget->setLayout(pTitleSearchLayout);

    // TitleBar Menu
    m_pTitleBarMenu = new DMenu();
    QAction *pNewAlbum = new QAction();
    addAction(pNewAlbum);

    pNewAlbum->setText(tr("New album"));
    pNewAlbum->setShortcut(QKeySequence(CTRLSHIFTN_SHORTCUT));
    m_pTitleBarMenu->addAction(pNewAlbum);

    QAction *pImport = new QAction();
    AC_SET_OBJECT_NAME(pImport, Import_Image_View);
    addAction(pImport);

    pImport->setText(tr("Import photos"));
    pImport->setShortcut(QKeySequence(CTRLO_SHORTCUT));
    m_pTitleBarMenu->addAction(pImport);
    m_pTitleBarMenu->addSeparator();

    titlebar()->addWidget(m_titleBtnWidget, Qt::AlignLeft);
    titlebar()->addWidget(m_pSearchEdit, Qt::AlignHCenter);
    titlebar()->setIcon(QIcon::fromTheme("deepin-album"));
    titlebar()->setMenu(m_pTitleBarMenu);
    titlebar()->setBlurBackground(true);
    AC_SET_OBJECT_NAME(titlebar(), MainWindow_Titlebar);
    AC_SET_ACCESSIBLE_NAME(titlebar(), MainWindow_Titlebar);

//    if (0 < DBManager::instance()->getImgsCount()) {
//        // dothing
//    } else {
//        m_pSearchEdit->setEnabled(false);
//    }
}

//初始化中心界面
void MainWindow::initCentralWidget()
{
    m_pCenterWidget = new QStackedWidget(this);
    AC_SET_OBJECT_NAME(m_pCenterWidget, MainWindow_Center_Widget);
    AC_SET_ACCESSIBLE_NAME(m_pCenterWidget, MainWindow_Center_Widget);
    m_pCenterWidget->setFixedSize(size());
    m_pCenterWidget->lower();

    m_pAllPicView = new AllPicView();             //所有照片界面
    m_pTimeLineWidget = new QWidget();
    //m_pTimeLineView = new TimeLineView();       //时间线界面
    m_pAlbumWidget = new QWidget();
    //m_pAlbumview = new AlbumView();             //相册界面
    //m_pSearchView = new SearchView();           //搜索界面
    m_pSearchViewWidget = new QWidget();

    m_commandLine = CommandLine::instance();
    m_commandLine->setThreads(this);
    m_slidePanel = new SlideShowPanel();

    m_pCenterWidget->addWidget(m_pAllPicView);

    m_pCenterWidget->addWidget(m_pTimeLineWidget);
    //m_pCenterWidget->addWidget(m_pTimeLineView);
    m_pCenterWidget->addWidget(m_pAlbumWidget);
    //m_pCenterWidget->addWidget(m_pAlbumview);

    //m_pCenterWidget->addWidget(m_pSearchView);
    m_pCenterWidget->addWidget(m_pSearchViewWidget);
    m_pCenterWidget->addWidget(m_commandLine);
    m_pCenterWidget->addWidget(m_slidePanel);

    QStringList pas;
    m_commandLine->processOption(pas);
    if (pas.length() > 0) {
        m_processOptionIsEmpty = false;
        titlebar()->setVisible(false);
        setTitlebarShadowEnabled(false);
        m_commandLine->viewImage(QFileInfo(pas.at(0)).absoluteFilePath(), pas);
        m_pCenterWidget->setCurrentIndex(VIEW_IMAGE);
        m_backIndex = VIEW_ALLPIC;
    } else {
        //性能优化，此句在构造时不需要执行，增加启动时间
        m_processOptionIsEmpty = true;
        //m_commandLine->viewImage("", {});
        m_pCenterWidget->setCurrentIndex(VIEW_ALLPIC);
    }
}

//设置等待窗口颜色
void MainWindow::setWaitDialogColor()
{
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        DPalette pa1;
        pa1 = m_waitlabel->palette();
        pa1.setColor(DPalette::WindowText, QColor("#001A2E"));
        m_waitlabel->setPalette(pa1);

        DPalette pa2;
        pa2 = m_countLabel->palette();
        pa2.setColor(DPalette::WindowText, QColor("#6A829F"));
        m_countLabel->setPalette(pa2);
    }
    if (themeType == DGuiApplicationHelper::DarkType) {
        DPalette pa1;
        pa1 = m_waitlabel->palette();
        pa1.setColor(DPalette::WindowText, QColor("#A8B7D1"));
        m_waitlabel->setPalette(pa1);

        DPalette pa2;
        pa2 = m_countLabel->palette();
        pa2.setColor(DPalette::WindowText, QColor("#6D7C88"));
        m_countLabel->setPalette(pa2);
    }
}

int MainWindow::getCurrentViewType()
{
    return m_iCurrentView;
}

//void MainWindow::onUpdateCentralWidget()
//{
//    emit dApp->signalM->hideExtensionPanel();
//    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
//}

//显示所有照片
void MainWindow::allPicBtnClicked()
{
    emit dApp->signalM->hideExtensionPanel();
//    m_pSearchEdit->clear();
    m_pSearchEdit->clearEdit();
    m_SearchKey.clear();

    m_iCurrentView = VIEW_ALLPIC;
    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
    m_pAllPicView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
    m_pAllPicView->updateStackedWidget();
    m_pAllPicView->updatePicNum();
    m_pAllPicView->getThumbnailListView()->setFocus();
}

//显示时间线照片
void MainWindow::timeLineBtnClicked()
{
    if (nullptr == m_pTimeLineView) {
        m_pCenterWidget->removeWidget(m_pTimeLineWidget);
        int index = m_pCenterWidget->indexOf(m_pAllPicView) + 1;
        m_pTimeLineView = new TimeLineView();
        m_pCenterWidget->insertWidget(index, m_pTimeLineView);
    }
    emit dApp->signalM->hideExtensionPanel();
    m_pSearchEdit->clearEdit();
    m_SearchKey.clear();
    m_iCurrentView = VIEW_TIMELINE;
    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
    m_pTimeLineView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
    m_pTimeLineView->updateStackedWidget();
    m_pTimeLineView->updatePicNum();
    m_pTimeLineView->setFocus();
}

//显示相册
void MainWindow::albumBtnClicked()
{
    if (nullptr == m_pAlbumview) {
        int index = 0;
        if (nullptr == m_pTimeLineView) {
            m_pCenterWidget->removeWidget(m_pTimeLineWidget);
            index = m_pCenterWidget->indexOf(m_pAllPicView) + 1;
            m_pTimeLineView = new TimeLineView();
            m_pCenterWidget->insertWidget(index, m_pTimeLineView);
        }
        m_pCenterWidget->removeWidget(m_pAlbumWidget);
        index = m_pCenterWidget->indexOf(m_pTimeLineView) + 1;
        m_pAlbumview = new AlbumView();
        connect(m_pAlbumview, &AlbumView::sigSearchEditIsDisplay, this, &MainWindow::onSearchEditIsDisplay);
        m_pCenterWidget->insertWidget(index, m_pAlbumview);
    }
    emit dApp->signalM->hideExtensionPanel();
    m_pSearchEdit->clearEdit();
    m_SearchKey.clear();
    m_iCurrentView = VIEW_ALBUM;
    m_pCenterWidget->setCurrentIndex(m_iCurrentView);

    //手动更新界面，重新计算大小
    DWidget *pwidget = nullptr;
    pwidget = m_pAlbumview->m_pRightStackWidget->currentWidget();
    if (pwidget == m_pAlbumview->pImportTimeLineWidget) { //当前为导入界面
        m_pAlbumview->m_pRightThumbnailList->resizeHand();
    } else if (pwidget == m_pAlbumview->m_pTrashWidget) {
        m_pAlbumview->m_pRightTrashThumbnailList->resizeHand();
    } else if (pwidget == m_pAlbumview->m_pFavoriteWidget) {
        m_pAlbumview->m_pRightFavoriteThumbnailList->resizeHand();
    }

    m_pAlbumview->SearchReturnUpdate();
    m_pAlbumview->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
    m_pAlbumview->updatePicNum();
    m_pAlbumview->setFocus();
    emit m_pAlbumview->sigReCalcTimeLineSizeIfNeed();
}

//标题菜单栏槽函数
void MainWindow::onTitleBarMenuClicked(QAction *action)
{
    if (tr("New album") == action->text()) {
        emit dApp->signalM->createAlbum(QStringList(" "));
    }

    else if (tr("Import photos") == action->text()) {
        emit dApp->signalM->startImprot();
        emit sigTitleMenuImportClicked();
    } else {

    }
}

//创建相册槽函数
void MainWindow::onCreateAlbum(QStringList imagepaths)
{
    showCreateDialog(imagepaths);
}
#if 1
void MainWindow::onViewCreateAlbum(QString imgpath, bool bmodel)
{
    AlbumCreateDialog *d = new AlbumCreateDialog(this);
    d->setModal(bmodel);
    d->show();
    d->move(this->x() + (this->width() - d->width()) / 2, this->y() + (this->height() - d->height()) / 2);
    connect(d, &AlbumCreateDialog::albumAdded, this, [ = ] {
        emit dApp->signalM->hideExtensionPanel();
        DBManager::instance()->insertIntoAlbum(d->getCreateAlbumName(), imgpath.isEmpty() ? QStringList(" ") : QStringList(imgpath));
        emit dApp->signalM->sigCreateNewAlbumFrom(d->getCreateAlbumName());
        QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess.svg");
        QString str = tr("Successfully added to “%1”");
        floatMessage(str.arg(d->getCreateAlbumName()), icon);
        if (imgpath.count() > 0 && imgpath != " ")
        {
            ImageEngineApi::instance()->setImgPathAndAlbumNames(DBManager::instance()->getAllPathAlbumNames());
            QStringList paths;
            paths << imgpath;
            emit SignalManager::instance()->sigSyncListviewModelData(paths, d->getCreateAlbumName(), 4);
        }
    });
}
#endif
//创建相册弹出窗
void MainWindow::showCreateDialog(QStringList imgpaths)
{
    AlbumCreateDialog *d = new AlbumCreateDialog(this);
    d->show();
    d->move(this->x() + (this->width() - d->width()) / 2, this->y() + (this->height() - d->height()) / 2);
    connect(d, &AlbumCreateDialog::albumAdded, this, [ = ] {
        //double insert problem from here ,first insert at AlbumCreateDialog::createAlbum(albumname)
        if (nullptr == m_pAlbumview)
        {
            int index = 0;
            if (nullptr == m_pTimeLineView) {
                m_pCenterWidget->removeWidget(m_pTimeLineWidget);
                index = m_pCenterWidget->indexOf(m_pAllPicView) + 1;
                m_pTimeLineView = new TimeLineView();
                m_pCenterWidget->insertWidget(index, m_pTimeLineView);
            }
            m_pCenterWidget->removeWidget(m_pAlbumWidget);
            index = m_pCenterWidget->indexOf(m_pTimeLineView) + 1;
            m_pAlbumview = new AlbumView();
            connect(m_pAlbumview, &AlbumView::sigSearchEditIsDisplay, this, &MainWindow::onSearchEditIsDisplay);
            m_pCenterWidget->insertWidget(index, m_pAlbumview);
//            emit dApp->signalM->sigCreateNewAlbumFromDialog(d->getCreateAlbumName());
            m_pAlbumBtn->setChecked(true);
            m_pSearchEdit->clearEdit();
            m_SearchKey.clear();
            m_pAlbumview->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
        } else
        {
//            DBManager::instance()->insertIntoAlbum(d->getCreateAlbumName(), imgpaths);
            emit dApp->signalM->sigCreateNewAlbumFromDialog(d->getCreateAlbumName());
            m_pAlbumBtn->setChecked(true);
            m_pSearchEdit->clearEdit();
            m_SearchKey.clear();
            m_pAlbumview->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
        }
        m_backIndex = VIEW_ALBUM;
        DBManager::instance()->insertIntoAlbum(d->getCreateAlbumName(), imgpaths);
        emit dApp->signalM->insertedIntoAlbum(m_pAlbumview->m_currentAlbum, imgpaths);
        emit dApp->signalM->hideImageView();    //该信号针对查看界面新建相册(快捷键 crtl+n)，正常退出
        // 相册照片更新时的．更新路径相册名缓存,用于listview的setdata userrole + 2
        // " " 新建空的相册
        if (imgpaths.first() != " ")
        {
            ImageEngineApi::instance()->setImgPathAndAlbumNames(DBManager::instance()->getAllPathAlbumNames());
            emit SignalManager::instance()->sigSyncListviewModelData(imgpaths, d->getCreateAlbumName(), 4);
        }
    });
}

//搜索框
void MainWindow::onSearchEditFinished()
{
    QString keywords = m_pSearchEdit->text();
    if (m_SearchKey == keywords) //两次搜索条件相同，跳过
        return;
    m_SearchKey = keywords;
    emit dApp->signalM->hideExtensionPanel();
    if (VIEW_ALLPIC == m_iCurrentView) {
        if (keywords.isEmpty()) {
            allPicBtnClicked();
            // donothing
        } else {
            emit dApp->signalM->sigSendKeywordsIntoALLPic(keywords, COMMON_STR_ALLPHOTOS);
            m_pAllPicView->m_pStackedWidget->setCurrentIndex(2);
            m_pAllPicView->restorePicNum();
        }
    } else if (VIEW_TIMELINE == m_iCurrentView) {
        if (keywords.isEmpty()) {
            timeLineBtnClicked();
            // donothing
        } else {
            emit dApp->signalM->sigSendKeywordsIntoALLPic(keywords, COMMON_STR_TIMELINE);
            m_pTimeLineView->m_pStackedWidget->setCurrentIndex(2);
            m_pTimeLineView->restorePicNum();
        }
    } else if (VIEW_ALBUM == m_iCurrentView) {
        if (keywords.isEmpty()) {
            albumBtnClicked();
            // donothing
        } else {
            if (COMMON_STR_RECENT_IMPORTED == m_pAlbumview->m_pLeftListView->getItemCurrentType()) {
                emit dApp->signalM->sigSendKeywordsIntoALLPic(keywords, COMMON_STR_RECENT_IMPORTED);
            }
            //LMH0514,为了解决26092 【相册】【5.6.9.14】在我的收藏相册下无法搜索到收藏的照片，加入我的收藏枚举
            else if (COMMON_STR_FAVORITES == m_pAlbumview->m_pLeftListView->getItemCurrentType()) {
                emit dApp->signalM->sigSendKeywordsIntoALLPic(keywords, COMMON_STR_FAVORITES);
            } else if (COMMON_STR_CUSTOM == m_pAlbumview->m_pLeftListView->getItemCurrentType()) {
                emit dApp->signalM->sigSendKeywordsIntoALLPic(keywords, m_pAlbumview->m_pLeftListView->getItemCurrentName());
            }

            m_pAlbumview->m_pRightStackWidget->setCurrentIndex(4);
            m_pAlbumview->restorePicNum();
        }
    }
}

//标题菜单导入照片槽函数
void MainWindow::onImprotBtnClicked()
{
    static QStringList sList;
    for (const QString &i : UnionImage_NameSpace::unionImageSupportFormat())
        sList << ("*." + i);
    QString filter = tr("All Photos");
    filter.append('(');
    filter.append(sList.join(" "));
    filter.append(')');
    static QString cfgGroupName = QStringLiteral("General"), cfgLastOpenPath = QStringLiteral("LastOpenPath");
    QString pictureFolder = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QDir existChecker(pictureFolder);
    if (!existChecker.exists()) {
        pictureFolder = QDir::currentPath();
    }
    pictureFolder = dApp->setter->value(cfgGroupName, cfgLastOpenPath, pictureFolder).toString();
    DFileDialog dialog(this);
    dialog.setFileMode(DFileDialog::ExistingFiles);
    dialog.setDirectory(pictureFolder);
    dialog.setNameFilter(filter);
    dialog.setOption(QFileDialog::HideNameFilterDetails);
    dialog.setWindowTitle(tr("Import Photos"));
    dialog.setAllowMixedSelection(true);
    const int mode = dialog.exec();
    if (mode != QDialog::Accepted) {
        return;
    }
    const QStringList &file_list = dialog.selectedFiles();
    if (file_list.isEmpty())
        return;
    ImageEngineApi::instance()->SaveImagesCache(file_list);
    if (m_iCurrentView == VIEW_ALBUM) {
        if (m_pAlbumview->m_currentType == ALBUM_PATHTYPE_BY_PHONE || m_pAlbumview->m_currentItemType == 0) {
            ImageEngineApi::instance()->ImportImagesFromFileList(file_list, "", this, true);
        } else {
            ImageEngineApi::instance()->ImportImagesFromFileList(file_list, m_pAlbumview->m_currentAlbum, this, true);
        }
    } else {
        ImageEngineApi::instance()->ImportImagesFromFileList(file_list, "", this, true);
    }

}

bool MainWindow::imageImported(bool success)
{
    emit dApp->signalM->closeWaitDialog();
    emit sigImageImported(success);
    return true;
}

//显示图片详细信息
void MainWindow::onShowImageInfo(const QString &path)
{
    ImgInfoDialog *dialog;
    if (m_propertyDialogs.contains(path)) {
        m_propertyDialogs.remove(path);
        dialog = new ImgInfoDialog(path);
        m_propertyDialogs.insert(path, dialog);
        dialog->show();
        dialog->move((this->width() - dialog->width() - 50 + mapToGlobal(QPoint(0, 0)).x()), 100 + mapToGlobal(QPoint(0, 0)).y());
        dialog->setWindowState(Qt::WindowActive);
        connect(dialog, &ImgInfoDialog::closed, this, [ = ] {
            dialog->deleteLater();
            m_propertyDialogs.remove(path);
        });
    } else {
        dialog = new ImgInfoDialog(path, this);
        dialog->setObjectName("ImgInfoDialog");
        m_propertyDialogs.insert(path, dialog);
        dialog->show();
        dialog->move((this->width() - dialog->width() - 50 + mapToGlobal(QPoint(0, 0)).x()), 100 + mapToGlobal(QPoint(0, 0)).y());
        dialog->setWindowState(Qt::WindowActive);
        connect(dialog, &ImgInfoDialog::closed, this, [ = ] {
            dialog->deleteLater();
            m_propertyDialogs.remove(path);
        });
    }
}

//void MainWindow::viewImageClose()
//{
//    if (bfirstandviewimage) {
//        exit(0);
//    }
//}

void MainWindow::floatMessage(const QString &str, const QIcon &icon)
{
    QString tempStr = str;
    QWidget *pwidget = nullptr;
    switch (m_pCenterWidget->currentIndex()) {
    case 0:
        pwidget = m_pAllPicView->m_pwidget;
        break;
    case 1:
        pwidget = m_pTimeLineView->m_pwidget;
        break;
    case 2:
        pwidget = m_pAlbumview->m_pwidget;
        break;
    case 4:
        pwidget = m_commandLine;
        break;
    default:
        pwidget = m_pAllPicView->m_pwidget;
        break;
    }
    DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, pwidget);
    pDFloatingMessage->setBlurBackgroundEnabled(true);
    pDFloatingMessage->setMessage(tempStr);
    pDFloatingMessage->setIcon(icon);
    pDFloatingMessage->raise();
    if (pwidget)
        DMessageManager::instance()->sendMessage(pwidget, pDFloatingMessage);
}

//外部使用相册打开图片
void MainWindow::onNewAPPOpen(qint64 pid, const QStringList &arguments)
{
    qDebug() << "onNewAPPOpen";
    Q_UNUSED(pid);
    QStringList paths;
    if (arguments.length() > 1) {
        //arguments第1个参数是进程名，图片paths参数需要从下标1开始
        for (int i = 1; i < arguments.size(); ++i) {
            QString qpath = arguments.at(i);
            paths.append(qpath);
            ImageEngineApi::instance()->insertImage(qpath, "");
        }
        if (paths.count() > 0) {
            SignalManager::ViewInfo info;
            info.album = "";
#ifndef LITE_DIV
            info.inDatabase = false;
#endif
            info.lastPanel = nullptr;
            info.path = paths.at(0);
            info.paths = paths;

            emit dApp->signalM->viewImage(info);
            emit dApp->signalM->showImageView(0);

            //更改为调用线程api
            ImageEngineApi::instance()->loadImagesFromNewAPP(paths, this);
        }
        m_pAllPicBtn->setChecked(true);
//        dApp->LoadDbImage();
    }
    this->activateWindow();
}

//主程序加载完毕，更新搜索框是否可用
void MainWindow::onLoadingFinished()
{
//    m_pTimeLineBtn->setEnabled(true);
//    m_pAlbumBtn->setEnabled(true);
    if (0 < DBManager::instance()->getImgsCount()) {
        m_pSearchEdit->setEnabled(true);
    } else {
        m_pSearchEdit->setEnabled(false);
    }
}

QButtonGroup *MainWindow::getButG()
{
    return (nullptr != static_cast<QButtonGroup *>(btnGroup) ? btnGroup : nullptr);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveWindowState();
    if (VIEW_IMAGE == m_pCenterWidget->currentIndex()) {
        emit dApp->signalM->hideImageView();
        emit dApp->signalM->sigPauseOrStart(false);     //唤醒外设后台挂载
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::showEvent(QShowEvent *event)
{
    Q_UNUSED(event)
    int m_SearchEditWidth = titlebar()->width() - m_titleBtnWidget->width() - TITLEBAR_BLANK_WIDTH;
    if (m_SearchEditWidth <= 350) {
        m_pSearchEdit->setFixedSize(m_SearchEditWidth - 20, 36);
    } else {
        m_SearchEditWidth = 350;
        m_pSearchEdit->setFixedSize(m_SearchEditWidth, 36);
    }
    QMetaObject::invokeMethod(this, [ = ]() {
        if (m_isFirstStart) {
            if (nullptr == m_pSearchView) {
                int index = m_pCenterWidget->indexOf(m_pSearchViewWidget);
                m_pSearchView = new SearchView();
                m_pCenterWidget->insertWidget(index, m_pSearchView);
                m_pCenterWidget->removeWidget(m_pSearchViewWidget);
            }
            if (m_processOptionIsEmpty) {
                m_commandLine->viewImage("", {});
            }

            initShortcut();
            initConnections();
            initDBus();

            //loadZoomRatio();
            if (0 < DBManager::instance()->getImgsCount()) {
                // dothing
            } else {
                m_pSearchEdit->setEnabled(false);
            }
        }
        m_isFirstStart = false;
        m_pCenterWidget->setFixedSize(size());
    }, Qt::QueuedConnection);
}

void MainWindow::saveWindowState()
{
    m_settings->setValue("album-geometry", saveGeometry());
    m_settings->setValue("album-isMaximized", isMaximized());
    m_settings->setValue("album-version", VERSION);
    saveZoomRatio();
}

//加载主界面状态（上次退出时）
void MainWindow::loadWindowState()
{
    const QByteArray geometry = m_settings->value("album-geometry").toByteArray();
    const bool isMaximized = m_settings->value("album-isMaximized").toBool();
//    const QByteArray pos = settings.value("album-pos").toByteArray();
    if (!geometry.isEmpty()) {
        if (m_settings->contains("album-version")) {
            if (m_settings->value("album-version").toString().isEmpty()) {
                restoreGeometry(geometry);
                if (isMaximized) {
                    resize(1300, 848);
                    Dtk::Widget::moveToCenter(this);
                }
            } else if (compareVersion()) {
                resize(1300, 848);
            } else {
                restoreGeometry(geometry);
                if (isMaximized) {
                    resize(1300, 848);
                    Dtk::Widget::moveToCenter(this);
                }
            }
        } else {
            resize(1300, 848);
        }
    }
//    settings.endGroup();
    getButG();
}

//保存缩放比例
void MainWindow::saveZoomRatio()
{
    m_settings->setValue("album-zoomratio", m_pSliderPos);
}

bool MainWindow::compareVersion()
{
    QString versionBuild(VERSION);
    QStringList versionBuildList = versionBuild.split(".");
    QStringList versionConfigList = m_settings->value("album-version").toString().split(".");
    if (versionBuildList.size() == 4 && versionConfigList.size() == 4) {
        for (int i = 0; i < 4; i++) {
            if (versionBuildList.at(i).toInt() > versionConfigList.at(i).toInt()) {
                return true;
            } else if (versionConfigList.at(i).toInt() == versionBuildList.at(i).toInt()) {
                continue;
            } else {
                return false;
            }
        }
        return false;
    } else {
        return false;
    }
}

//加载缩放比例（上次退出时）
void MainWindow::loadZoomRatio()
{
    qDebug() << "zy------MainWindow::loadZoomRatio begin";
    if (m_settings->contains("album-version")) {
        if (m_settings->value("album-version").toString().isEmpty()) {
            if (!m_settings->contains("album-zoomratio")) {
                m_pSliderPos = 4;
            } else {
                m_pSliderPos = m_settings->value("album-zoomratio").toInt();
            }
        } else if (compareVersion()) {
            m_pSliderPos = 4;
        } else {
            if (!m_settings->contains("album-zoomratio")) {
                m_pSliderPos = 4;
            } else {
                m_pSliderPos = m_settings->value("album-zoomratio").toInt();
            }
        }
        dApp->signalM->sigMainwindowSliderValueChg(m_pSliderPos);
    } else {
        m_pSliderPos = 4;
    }

    m_pAllPicView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
    dApp->signalM->sigMainwindowSliderValueChg(m_pSliderPos);
    qDebug() << "zy------MainWindow::loadZoomRatio end";
}

//初始化各种快捷键
void MainWindow::initShortcutKey()
{
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, VIEW_CONTEXT_MENU, ENTER_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, FULLSCREEN_CONTEXT_MENU, F11_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, SLIDESHOW_CONTEXT_MENU, F5_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, COPYTOCLIPBOARD_CONTEXT_MENU, CTRLC_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, DELETE_CONTEXT_MENU, DELETE_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, THROWTOTRASH_CONTEXT_MENU, DELETE_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, REMOVEFROMALBUM_CONTEXT_MENU, SHIFTDEL_SHORTCUT);
//    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, UNFAVORITE_CONTEXT_MENU, CTRLSHIFTK_SHORTCUT);
//    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, FAVORITE_CONTEXT_MENU, CTRLK_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, FAVORITE_CONTEXT_MENU, SENTENCE_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, ROTATECLOCKWISE_CONTEXT_MENU, CTRLR_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, ROTATECOUNTERCLOCKWISE_CONTEXT_MENU, CTRLSHIFTR_SHORTCUT);
//    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, SETASWALLPAPER_CONTEXT_MENU, CTRLF8_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, SETASWALLPAPER_CONTEXT_MENU, CTRLF9_SHORTCUT);
//    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, DISPLAYINFILEMANAGER_CONTEXT_MENU, CTRLD_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, DISPLAYINFILEMANAGER_CONTEXT_MENU, ALTD_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, ImageInfo_CONTEXT_MENU, CTRLI_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, COMMON_STR_CREATEALBUM, CTRLSHIFTN_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, COMMON_STR_RENAMEALBUM, F2_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, SHOW_SHORTCUT_PREVIEW, CTRLSHIFTSLASH_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, EXPORT_CONTEXT_MENU, CTRLE_SHORTCUT);

    QString userConfigPath = DStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
                             + "/config.conf";
    m_settings = new QSettings(userConfigPath, QSettings::IniFormat);
}

//缩略图放大
void MainWindow::thumbnailZoomIn()
{
    if (VIEW_IMAGE == m_pCenterWidget->currentIndex()) {
        emit dApp->signalM->sigCtrlADDKeyActivated();
    } else {
        if (m_pSliderPos != m_pAllPicView->m_pStatusBar->m_pSlider->maximum()) {
            m_pSliderPos = m_pSliderPos + 1;
            if (m_pCenterWidget->currentIndex() == VIEW_ALLPIC) {
                m_pAllPicView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
            } else if (m_pCenterWidget->currentIndex() == VIEW_TIMELINE) {
                m_pTimeLineView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
            } else if (m_pCenterWidget->currentIndex() == VIEW_ALBUM) {
                m_pAlbumview->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
            }

            emit dApp->signalM->sigMainwindowSliderValueChg(m_pSliderPos);
        }
    }
}

//缩略图缩小
void MainWindow::thumbnailZoomOut()
{
    if (VIEW_IMAGE == m_pCenterWidget->currentIndex()) {
        emit dApp->signalM->sigCtrlSubtractKeyActivated();
    } else {
        if (m_pSliderPos != m_pAllPicView->m_pStatusBar->m_pSlider->minimum()) {
            m_pSliderPos = m_pSliderPos - 1;
            if (m_pCenterWidget->currentIndex() == VIEW_ALLPIC) {
                m_pAllPicView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
            } else if (m_pCenterWidget->currentIndex() == VIEW_TIMELINE) {
                m_pTimeLineView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
            } else if (m_pCenterWidget->currentIndex() == VIEW_ALBUM) {
                m_pAlbumview->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
            }

            dApp->signalM->sigMainwindowSliderValueChg(m_pSliderPos);
        }
    }
}

QJsonObject MainWindow::createShorcutJson()
{
    //Translations
    QJsonObject shortcut1;
    shortcut1.insert("name", "Window sizing");
    shortcut1.insert("value", "Ctrl+Alt+F");
    QJsonObject shortcut2;
    shortcut2.insert("name", tr("Fullscreen"));
    shortcut2.insert("value", "F11");
    QJsonObject shortcut3;
    shortcut3.insert("name", tr("Exit fullscreen/slideshow"));
    shortcut3.insert("value", "Esc");
    QJsonObject shortcut4;
    shortcut4.insert("name", "Close application");
    shortcut4.insert("value", "Alt+F4");
    QJsonObject shortcut5;
    shortcut5.insert("name", tr("Help"));
    shortcut5.insert("value", "F1");
    QJsonObject shortcut6;
    shortcut6.insert("name", tr("Display shortcuts"));
    shortcut6.insert("value", "Ctrl+Shift+?");
    QJsonObject shortcut7;
    shortcut7.insert("name", tr("Display in file manager"));
    shortcut7.insert("value", "Alt+D");
    QJsonObject shortcut8;
    shortcut8.insert("name", tr("Slide show"));
    shortcut8.insert("value", "F5");
    QJsonObject shortcut9;
    shortcut9.insert("name", tr("View"));
    shortcut9.insert("value", "Enter");
    QJsonObject shortcut10;
    shortcut10.insert("name", tr("Export"));
    shortcut10.insert("value", "Ctrl+E");
    QJsonObject shortcut11;
    shortcut11.insert("name", tr("Import"));
    shortcut11.insert("value", "Ctrl+O");
    QJsonObject shortcut12;
    shortcut12.insert("name", tr("Select all"));
    shortcut12.insert("value", "Ctrl+A");
    QJsonObject shortcut13;
    shortcut13.insert("name", tr("Copy"));
    shortcut13.insert("value", "Ctrl+C");
    QJsonObject shortcut14;
    shortcut14.insert("name", tr("Delete photo/album"));
    shortcut14.insert("value", "Delete");
    QJsonObject shortcut15;
    shortcut15.insert("name", tr("Photo info"));
    shortcut15.insert("value", "Ctrl+I");
    QJsonObject shortcut16;
    shortcut16.insert("name", tr("Set as wallpaper"));
    shortcut16.insert("value", "Ctrl+F9");
    QJsonObject shortcut17;
    shortcut17.insert("name", tr("Rotate clockwise"));
    shortcut17.insert("value", "Ctrl+R");
    QJsonObject shortcut18;
    shortcut18.insert("name", tr("Rotate counterclockwise"));
    shortcut18.insert("value", "Ctrl+Shift+R");
    QJsonObject shortcut19;
    shortcut19.insert("name", " ");
    shortcut19.insert("value", "  ");
    QJsonObject shortcut20;
    shortcut20.insert("name", tr("Zoom in"));
    shortcut20.insert("value", "Ctrl+'+'");
    QJsonObject shortcut21;
    shortcut21.insert("name", tr("Zoom out"));
    shortcut21.insert("value", "Ctrl+'-'");
    QJsonObject shortcut22;
    shortcut22.insert("name", tr("Previous"));
    shortcut22.insert("value", "Left");
    QJsonObject shortcut23;
    shortcut23.insert("name", tr("Next"));
    shortcut23.insert("value", "Right");
    QJsonObject shortcut24;
    shortcut24.insert("name", tr("Favorite"));
    shortcut24.insert("value", ".");
    QJsonObject shortcut25;
    shortcut25.insert("name", tr("Unfavorite"));
    shortcut25.insert("value", ".");
    QJsonObject shortcut26;
    shortcut26.insert("name", tr("New album"));
    shortcut26.insert("value", "Ctrl+Shift+N");
    QJsonObject shortcut27;
    shortcut27.insert("name", tr("Rename album"));
    shortcut27.insert("value", "F2");
    QJsonObject shortcut28;
    shortcut28.insert("name", tr("Page up"));
    shortcut28.insert("value", "PageUp");
    QJsonObject shortcut29;
    shortcut29.insert("name", tr("Page down"));
    shortcut29.insert("value", "PageDown");



    QJsonArray shortcutArray1;
    shortcutArray1.append(shortcut2);
    shortcutArray1.append(shortcut8);
    shortcutArray1.append(shortcut3);
    shortcutArray1.append(shortcut9);
    shortcutArray1.append(shortcut10);
    shortcutArray1.append(shortcut11);
    shortcutArray1.append(shortcut12);
    shortcutArray1.append(shortcut13);
    shortcutArray1.append(shortcut14);
    shortcutArray1.append(shortcut15);
    shortcutArray1.append(shortcut16);
    shortcutArray1.append(shortcut17);
    shortcutArray1.append(shortcut18);
    shortcutArray1.append(shortcut7);
//    shortcutArray1.append(shortcut19);
    shortcutArray1.append(shortcut20);
    shortcutArray1.append(shortcut21);
    shortcutArray1.append(shortcut28);
    shortcutArray1.append(shortcut29);
    shortcutArray1.append(shortcut22);
    shortcutArray1.append(shortcut23);
    shortcutArray1.append(shortcut24);
    shortcutArray1.append(shortcut25);
    QJsonArray shortcutArray2;
    shortcutArray2.append(shortcut26);
    shortcutArray2.append(shortcut27);
    QJsonArray shortcutArray3;
    shortcutArray3.append(shortcut5);
    shortcutArray3.append(shortcut6);

//    shortcutArray.append(shortcut1);
//    shortcutArray.append(shortcut4);

    QJsonObject shortcut_group1;
//    shortcut_group.insert("groupName", tr("热键"));
//    shortcut_group.insert("groupName", tr("Hotkey"));
    shortcut_group1.insert("groupName", tr("Photos"));
    shortcut_group1.insert("groupItems", shortcutArray1);
    QJsonObject shortcut_group2;
    shortcut_group2.insert("groupName", tr("Albums"));
    shortcut_group2.insert("groupItems", shortcutArray2);
    QJsonObject shortcut_group3;
    shortcut_group3.insert("groupName", tr("Settings"));
    shortcut_group3.insert("groupItems", shortcutArray3);

    QJsonArray shortcutArrayall;
    shortcutArrayall.append(shortcut_group1);
    shortcutArrayall.append(shortcut_group2);
    shortcutArrayall.append(shortcut_group3);

    QJsonObject main_shortcut;
    main_shortcut.insert("shortcut", shortcutArrayall);

    return main_shortcut;
}

void MainWindow::wheelEvent(QWheelEvent *event)
{
    if (DApplication::keyboardModifiers() == Qt::ControlModifier) {
        if (event->delta() > 0) {
            thumbnailZoomIn();
        } else {
            thumbnailZoomOut();
        }
        event->accept();
    }
}

void MainWindow::closeFromMenu()
{
    close();
}

void MainWindow::onButtonClicked(int id)
{
    if (0 == id) {
        allPicBtnClicked();
        m_pSearchEdit->setVisible(true);
    }
    int index = 0;
    if (1 == id) {
        if (nullptr == m_pTimeLineView) {
            m_pCenterWidget->removeWidget(m_pTimeLineWidget);
            index = m_pCenterWidget->indexOf(m_pAllPicView) + 1;
            m_pTimeLineView = new TimeLineView();
            m_pCenterWidget->insertWidget(index, m_pTimeLineView);
        }
        timeLineBtnClicked();
        m_pSearchEdit->setVisible(true);
    }
    if (2 == id) {
        if (nullptr == m_pAlbumview) {
            if (nullptr == m_pTimeLineView) {
                m_pCenterWidget->removeWidget(m_pTimeLineWidget);
                index = m_pCenterWidget->indexOf(m_pAllPicView) + 1;
                m_pTimeLineView = new TimeLineView();
                m_pCenterWidget->insertWidget(index, m_pTimeLineView);
            }
            m_pCenterWidget->removeWidget(m_pAlbumWidget);
            index = m_pCenterWidget->indexOf(m_pTimeLineView) + 1;
            m_pAlbumview = new AlbumView();
            connect(m_pAlbumview, &AlbumView::sigSearchEditIsDisplay, this, &MainWindow::onSearchEditIsDisplay);
            m_pCenterWidget->insertWidget(index, m_pAlbumview);
        }
        albumBtnClicked();
        // 如果是最近删除或者移动设备,则搜索框不显示
        if (2 == m_pAlbumview->m_pRightStackWidget->currentIndex() || 5 == m_pAlbumview->m_pRightStackWidget->currentIndex()) {
            m_pSearchEdit->setVisible(false);
        } else {
            m_pSearchEdit->setVisible(true);
        }
    }
}

void MainWindow::onImageImported(bool success)
{
    if (success) {
        if (nullptr == m_pAlbumview)
            return ;
        if (ALBUM_PATHTYPE_BY_PHONE == m_pAlbumview->m_pLeftListView->getItemCurrentType()) {
            //2020/5/20 DJH 修复在设备界面本地导入会导致名称变换 type 写成了 album
            //m_pAlbumview->m_currentAlbum = ALBUM_PATHTYPE_BY_PHONE;
            m_pAlbumview->m_currentType = ALBUM_PATHTYPE_BY_PHONE;
        }
    }
}

void MainWindow::onSearchEditTextChanged(QString text)
{
    if (text.isEmpty()) {
        m_SearchKey.clear();
        switch (m_iCurrentView) {
        case VIEW_ALLPIC: {
            m_pAllPicView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
            m_pAllPicView->updateStackedWidget();
            m_pAllPicView->updatePicNum();
        }
        break;
        case VIEW_TIMELINE: {
            m_pTimeLineView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
            m_pTimeLineView->updateStackedWidget();
            m_pTimeLineView->updatePicNum();
        }
        break;
        case VIEW_ALBUM: {
            DWidget *pwidget = nullptr;
            pwidget = m_pAlbumview->m_pRightStackWidget->currentWidget();
            if (pwidget == m_pAlbumview->pImportTimeLineWidget) { //当前为导入界面
                m_pAlbumview->m_pRightThumbnailList->resizeHand();
            } else if (pwidget == m_pAlbumview->m_pTrashWidget) {
                m_pAlbumview->m_pRightTrashThumbnailList->resizeHand();
            } else if (pwidget == m_pAlbumview->m_pFavoriteWidget) {
                m_pAlbumview->m_pRightFavoriteThumbnailList->resizeHand();
            }

            m_pAlbumview->SearchReturnUpdate();
            m_pAlbumview->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
            m_pAlbumview->updatePicNum();
            emit m_pAlbumview->sigReCalcTimeLineSizeIfNeed();

        }
        break;
        }
    }
}

void MainWindow::onImagesInserted()
{
    m_pSearchEdit->setEnabled(true);
}

void MainWindow::onStartImprot()
{
    m_countLabel->setText("");
    m_importBar->setValue(0);
}

void MainWindow::onProgressOfWaitDialog(int allfiles, int completefiles)
{
    QString countText = "";
    if (m_bImport) {
        countText  = QString(tr("%1/%2 photos imported")).arg(completefiles).arg(allfiles);
    } else {
        countText = QString(tr("%1/%2 photos deleted")).arg(completefiles).arg(allfiles);
    }

    m_countLabel->setText(countText);
    m_countLabel->show();
    m_importBar->setRange(0, allfiles);
    m_importBar->setAlignment(Qt::AlignCenter);
    m_importBar->setTextVisible(false);
    m_importBar->setValue(completefiles);
}

void MainWindow::onPopupWaitDialog(QString waittext, bool bneedprogress)
{
    if (bneedprogress == false) {
        m_importBar->hide();
        m_countLabel->hide();
        m_waitlabel->setText(waittext);
        m_waitlabel->move(40, 30);
        m_waitlabel->show();
        m_waitdailog->show();
    } else {
        if (!waittext.compare(tr("Importing..."))) {
            m_bImport = true;
        } else {
            m_bImport = false;
        }
        m_waitlabel->move(40, 7);
        m_waitlabel->setText(waittext);
        m_importBar->show();
        m_waitlabel->show();
        m_waitdailog->show();
    }
}

void MainWindow::onCloseWaitDialog()
{
    m_bImport = false;
    m_countLabel->setText("");
    m_waitdailog->close();
}

void MainWindow::onImagesRemoved()
{
    if (0 < DBManager::instance()->getImgsCount()) {
        m_pSearchEdit->setEnabled(true);
    } else {
        m_pSearchEdit->setEnabled(false);
    }
}

void MainWindow::onHideImageView()
{
    titlebar()->setVisible(true);   //显示状态栏
    setTitlebarShadowEnabled(true);
    m_pCenterWidget->setCurrentIndex(m_backIndex);
}

void MainWindow::onShowSlidePanel(int index)
{
    m_backIndex_fromSlide = index;
    titlebar()->setVisible(false);
    setTitlebarShadowEnabled(false);
    m_pCenterWidget->setCurrentIndex(VIEW_SLIDE);
}

void MainWindow::onHideSlidePanel()
{
    emit dApp->signalM->hideExtensionPanel();
    if (VIEW_IMAGE != m_backIndex_fromSlide) {
        titlebar()->setVisible(true);
        setTitlebarShadowEnabled(true);
    }

    m_pCenterWidget->setCurrentIndex(m_backIndex_fromSlide);
}

void MainWindow::onExportImage(QStringList paths)
{
    Exporter::instance()->exportImage(paths);
}

void MainWindow::onMainwindowSliderValueChg(int step)
{
    m_pSliderPos = step;
    emit SignalManager::instance()->sliderValueChange(step);
    saveZoomRatio();
}

void MainWindow::onAlbDelToast(QString str1)
{
    QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg");
    QString str2 = tr("Album “%1” removed");
    QString str = str2.arg(str1);
    floatMessage(str, icon);
}

void MainWindow::onAddDuplicatePhotos()
{
    QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg");
    QString str = tr("图片已存在");
    floatMessage(str, icon);
}

void MainWindow::onAddToAlbToast(QString album)
{
    QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg");
    QString str2 = tr("Successfully added to “%1”");
    QString str = str2.arg(album);
    floatMessage(str, icon);
}

void MainWindow::onImportSuccess()
{
    QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg");
    QString str = tr("Import successful");
    floatMessage(str, icon);
}

void MainWindow::onSearchEditClear()
{
    m_pSearchEdit->clearEdit();
    m_SearchKey.clear();
}

void MainWindow::onImportFailed()
{
    QIcon icon(":/images/logo/resources/images/other/warning_new.svg");
    QString str = tr("Import failed");
    floatMessage(str, icon);
}

void MainWindow::onImportSomeFailed(int successful, int failed)
{
    QIcon icon(":/images/logo/resources/images/other/warning_new.svg");
    QString str = tr("%1 photos imported, %2 photos failed");
    QString str1 = QString::number(successful, 10);
    QString str2 = QString::number(failed, 10);
    QString res = str.arg(str1).arg(str2);
    floatMessage(res, icon);
}

void MainWindow::onImgExportFailed()
{
    QIcon icon(":/images/logo/resources/images/other/warning_new.svg");
    QString str = tr("Export failed");
    floatMessage(str, icon);
}

void MainWindow::onImgExportSuccess()
{
    QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg");
    QString str = tr("Export successful");
    floatMessage(str, icon);
}

void MainWindow::onAlbExportFailed()
{
    QIcon icon(":/images/logo/resources/images/other/warning_new.svg");
    QString str = tr("Export failed");
    floatMessage(str, icon);
}

void MainWindow::onAlbExportSuccess()
{
    QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg");
    QString str = tr("Export successful");
    floatMessage(str, icon);
}

void MainWindow::onEscShortcutActivated()
{
    if (window()->isFullScreen()) {
        emit dApp->signalM->sigESCKeyActivated();
        emit dApp->signalM->sigESCKeyStopSlide();
    } else if (VIEW_IMAGE == m_pCenterWidget->currentIndex()) {
        this->close();
    }
    emit dApp->signalM->hideExtensionPanel();
}

void MainWindow::onDelShortcutActivated()
{
    emit dApp->signalM->sigShortcutKeyDelete();
}

void MainWindow::onKeyF2ShortcutActivated()
{
    emit dApp->signalM->sigShortcutKeyF2();
}

void MainWindow::onCtrlShiftSlashShortcutActivated()
{
    QRect rect = window()->geometry();
    QPoint pos(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
    QStringList shortcutString;
    QJsonObject json = createShorcutJson();

    QString param1 = "-j=" + QString(QJsonDocument(json).toJson());
    QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());
    shortcutString << param1 << param2;

    QProcess *shortcutViewProcess = new QProcess();
    shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

    connect(shortcutViewProcess, SIGNAL(finished(int)), shortcutViewProcess, SLOT(deleteLater()));
}

void MainWindow::onCtrlFShortcutActivated()
{
    m_pSearchEdit->lineEdit()->setFocus();
}

void MainWindow::onShowImageView(int index)
{
    m_backIndex = index;
    titlebar()->setVisible(false);  //隐藏顶部状态栏
    setTitlebarShadowEnabled(false);
    m_pCenterWidget->setCurrentIndex(VIEW_IMAGE);
}

void MainWindow::onSearchEditIsDisplay(bool bIsDisp)
{
    if (m_pCenterWidget->currentIndex() == VIEW_ALBUM) {
        m_pSearchEdit->setVisible(bIsDisp);
    }
}
