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
#include "mainwindow.h"
#include "config.h"
#include "dialogs/albumcreatedialog.h"
#include "utils/unionimage.h"
#include "imageengine/imageengineapi.h"
#include "accessibledefine.h"
#include "viewerthememanager.h"
#include "ac-desktop-define.h"

#include <QGraphicsDropShadowEffect>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QDesktopWidget>
#include <QShortcut>
#include <QDir>
#include <QCommandLineParser>
#include <QMimeDatabase>

#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <dgiovolume.h>
#include <DTableView>
#include <DApplicationHelper>
#include <DFileDialog>
#include <DApplication>
#include <DMessageManager>
#include <DFloatingMessage>
#include <DWidgetUtil>
#include <DStandardPaths>

#include "imagedataservice.h"
#include "movieservice.h"
#include "imageviewer.h"
#include "imageengine.h"

bool bfirstopen = true;
bool bfirstandviewimage = false;
namespace  {
const int VIEW_ALLPIC = 0;
const int VIEW_TIMELINE = 1;
const int VIEW_ALBUM = 2;
const int VIEW_SEARCH = 3;
const int VIEW_IMAGE = 4;
const int VIEW_SLIDE = 5;

}//namespace

using namespace utils::common;
MainWindow::MainWindow()
    : m_iCurrentView(VIEW_ALLPIC)
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
    , m_backIndex(0)
    , m_pSliderPos(2)
    , btnGroup(nullptr)
    , m_pAllPicBtn(nullptr)
    , m_pTimeBtn(nullptr)
    , m_pAlbumBtn(nullptr)
    , m_waitdailog(nullptr)
    , m_importBar(nullptr)
    , m_waitlabel(nullptr)
    , m_countLabel(nullptr)
    , m_settings(nullptr)
    , m_fileInotifygroup(new FileInotifyGroup(this))
{
    this->setObjectName("drawMainWindow");
    initShortcutKey();          //初始化各种快捷键
    initUI();
    initTitleBar();             //初始化顶部状态栏
    initCentralWidget();
    initNoPhotoNormalTabOrder();
    initInstallFilter();
    //性能优化，此句在构造时不需要执行，增加启动时间,放在showevent之后队列执行
    loadZoomRatio();
    m_bVector << true << true << true;

    //平板模式下屏蔽最大化最小化及关闭按钮
#ifdef tablet_PC
    setWindowFlags(windowFlags() & ~(Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint));
#endif
}

MainWindow::~MainWindow()
{
    emit dApp->signalM->sigPauseOrStart(false); //唤醒外设后台挂载,防止析构时线程挂起卡住页面无法退出
    ImageEngineApi::instance()->close();
    QThreadPool::globalInstance()->clear();
    QThreadPool::globalInstance()->waitForDone();
    PrintHelper::getIntance()->deconstruction();
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);
    int m_SearchEditWidth = titlebar()->width() - m_titleBtnWidget->width() - TITLEBAR_BLANK_WIDTH;
    if (m_SearchEditWidth <= 350) {
        m_pSearchEdit->setFixedSize(m_SearchEditWidth - 20, 36);
    } else {
        m_SearchEditWidth = 350;
        m_pSearchEdit->setFixedSize(m_SearchEditWidth, 36);
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    // 键盘交互处理
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

        switch (keyEvent->key()) {
        default:
            break;
        case Qt::Key_Tab:
            if (initAllViewTabKeyOrder(obj)) {
                return true;
            }
        case Qt::Key_Enter:
        case Qt::Key_Return:
            initEnterkeyAction(obj);
            break;
        case Qt::Key_Right:
            if (initRightKeyOrder(obj)) {
                return true;
            }
        case Qt::Key_Left:
            if (initLeftKeyOrder(obj)) {
                return true;
            }
        case Qt::Key_Down:
            if (initDownKeyOrder()) {
                return true;
            }
        case Qt::Key_Up:
            if (initUpKeyOrder()) {
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

//初始化所有连接
void MainWindow::initConnections()
{
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList &");
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &MainWindow::setWaitDialogColor);
    //主界面切换（所有照片、时间线、相册）
    connect(btnGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, &MainWindow::onButtonClicked);
//    connect(dApp->signalM, &SignalManager::updatePicView, btnGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked));
    //图片导入槽函数
    connect(this, &MainWindow::sigImageImported, this, &MainWindow::onImageImported);
    connect(dApp->signalM, &SignalManager::createAlbum, this, &MainWindow::onCreateAlbum);
    connect(dApp->signalM, &SignalManager::viewCreateAlbum, this, &MainWindow::onViewCreateAlbum);
    connect(m_pSearchEdit, &DSearchEdit::returnPressed, this, &MainWindow::onSearchEditFinished);
    //connect(m_pSearchEdit, &DSearchEdit::textChanged, this, &MainWindow::onSearchEditTextChanged);
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
    connect(dApp->signalM, &SignalManager::startSlideShow, [this](const SignalManager::ViewInfo & vinfo) {
        this->onSigViewImage(vinfo, Operation_StartSliderShow, false, "", -1);
    });
    //隐藏幻灯片显示
    connect(ImageEngine::instance(), &ImageEngine::exitSlideShow, this, &MainWindow::onHideSlidePanel);
    //导出图片
    connect(dApp->signalM, &SignalManager::exportImage, this, &MainWindow::onExportImage);
//    connect(dApp->signalM, &SignalManager::showImageInfo, this, &MainWindow::onShowImageInfo);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::newProcessInstance, this, &MainWindow::onNewAPPOpen);
    //右下角滑动条
    connect(dApp->signalM, &SignalManager::sigMainwindowSliderValueChg, this, &MainWindow::onMainwindowSliderValueChg);
    connect(dApp->signalM, &SignalManager::sigAlbDelToast, this, &MainWindow::onAlbDelToast);
    // 添加到相册底部提示
    connect(dApp->signalM, &SignalManager::sigAddDuplicatePhotos, this, &MainWindow::onAddDuplicatePhotos);
    connect(dApp->signalM, &SignalManager::RepeatImportingTheSamePhotos, this, &MainWindow::onRepeatImportingTheSamePhotos);
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
    //没找到图片或视频提示框
    connect(dApp->signalM, &SignalManager::ImportDonotFindPicOrVideo, this, &MainWindow::onImportDonotFindPicOrVideo);
}

//初始化快捷键
void MainWindow::initShortcut()
{
    // Album View画面按DEL快捷键
    QShortcut *del = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    del->setContext(Qt::ApplicationShortcut);
    connect(del, &QShortcut::activated, this, &MainWindow::onDelShortcutActivated);

    // Album View画面按F2快捷键
    QShortcut *keyF2 = new QShortcut(QKeySequence(Qt::Key_F2), this);
    keyF2->setContext(Qt::ApplicationShortcut);
    connect(keyF2, &QShortcut::activated, this, &MainWindow::onKeyF2ShortcutActivated);

    //Ctrl+Up 缩略图放大
    m_CtrlUp = new QShortcut(QKeySequence(CTRLUP_SHORTCUT), this);
    m_CtrlUp->setContext(Qt::ApplicationShortcut);
    connect(m_CtrlUp, &QShortcut::activated, this, &MainWindow::thumbnailZoomIn);

    m_ReCtrlUp = new QShortcut(QKeySequence(RECTRLUP_SHORTCUT), this);
    m_ReCtrlUp->setContext(Qt::ApplicationShortcut);
    connect(m_ReCtrlUp, &QShortcut::activated, this, &MainWindow::thumbnailZoomIn);

    //Ctrl+Down 缩略图缩小
    m_CtrlDown = new QShortcut(QKeySequence(CTRLDOWN_SHORTCUT), this);
    m_CtrlDown->setContext(Qt::ApplicationShortcut);
    connect(m_CtrlDown, &QShortcut::activated, this, &MainWindow::thumbnailZoomOut);

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
    m_titleBtnWidget->setFocusPolicy(Qt::NoFocus);
    setFocusPolicy(Qt::NoFocus);
    AC_SET_OBJECT_NAME(m_titleBtnWidget, MainWindow_TitleBtn_Widget);
    AC_SET_ACCESSIBLE_NAME(m_titleBtnWidget, MainWindow_TitleBtn_Widget);
    btnGroup = new QButtonGroup();
    btnGroup->setExclusive(true);
    QHBoxLayout *pTitleBtnLayout = new QHBoxLayout();

    m_pAllPicBtn = new DPushButton();
    m_pAllPicBtn->setFocusPolicy(Qt::TabFocus);
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
    m_pTimeBtn->setFocusPolicy(Qt::TabFocus);
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
    m_pAlbumBtn->setFocusPolicy(Qt::TabFocus);
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
    m_pSearchEdit = new DSearchEdit(m_titleBtnWidget);
    m_pSearchEdit->lineEdit()->setFocusPolicy(Qt::ClickFocus);
    m_pSearchEdit->setMaximumSize(350, 36);
    if (0 < DBManager::instance()->getImgsCount()) {
        m_pSearchEdit->setEnabled(true);
    } else {
        m_pSearchEdit->setEnabled(false);
    }

    connect(m_pSearchEdit, &DSearchEdit::focusChanged,
    m_pSearchEdit, [ = ](bool onFocus) {
        if (!onFocus) {
            if (m_pSearchEdit->lineEdit()) {
                m_pSearchEdit->lineEdit()->clearFocus();
            }
            m_pSearchEdit->clearFocus();
        }
    });

//    pTitleSearchLayout->addWidget(m_pSearchEdit);
    //m_titleSearchWidget->setLayout(pTitleSearchLayout);

    // TitleBar Menu
    QAction *pNewAlbum = new QAction(this);
    addAction(pNewAlbum);
    pNewAlbum->setObjectName("New album");
    pNewAlbum->setText(tr("New album"));
    pNewAlbum->setShortcut(QKeySequence(CTRLSHIFTN_SHORTCUT));

    QAction *pNewPath = new QAction(this);
    addAction(pNewPath);
    pNewPath->setObjectName("Import folders");
    pNewPath->setText(tr("Import folders"));
    connect(pNewPath, &QAction::triggered, this, &MainWindow::onNewPathAction);

    m_pTitleBarMenu = new DMenu();
    m_pTitleBarMenu->addAction(pNewAlbum);
    m_pTitleBarMenu->addAction(pNewPath);
    m_pTitleBarMenu->addSeparator();

    titlebar()->addWidget(m_titleBtnWidget, Qt::AlignLeft);
#ifndef tablet_PC
    titlebar()->addWidget(m_pSearchEdit, Qt::AlignHCenter);
#endif

    m_addImageBtn = new DIconButton(DStyle::SP_IncreaseElement, this);
    connect(m_addImageBtn, &DIconButton::clicked, this, &MainWindow::onAddImageBtnClicked);
    m_addImageBtn->setToolTip(tr("Import photos and videos"));
    m_addImageBtn->setFixedSize(QSize(36, 36));
    m_addImageBtn->setShortcut(QKeySequence(CTRLO_SHORTCUT));
    titlebar()->addWidget(m_addImageBtn, Qt::AlignRight);

    titlebar()->setIcon(QIcon::fromTheme("deepin-album"));
    titlebar()->setMenu(m_pTitleBarMenu);
    titlebar()->setFocusPolicy(Qt::FocusPolicy::NoFocus);
    m_pSearchEdit->setFocusPolicy(Qt::ClickFocus);

//    titlebar()->setBlurBackground(true);// 0308 zy ui确认，取消标题栏透明效果
    AC_SET_OBJECT_NAME(titlebar(), MainWindow_Titlebar);
    AC_SET_ACCESSIBLE_NAME(titlebar(), MainWindow_Titlebar);

#ifdef tablet_PC
    m_pSearchEdit->hide();
#endif
}

QUrl UrlInfo1(QString path)
{
    QUrl url;
    // Just check if the path is an existing file.
    if (QFile::exists(path)) {
        url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
        return url;
    }

    const auto match = QRegularExpression(QStringLiteral(":(\\d+)(?::(\\d+))?:?$")).match(path);

    if (match.isValid()) {
        // cut away line/column specification from the path.
        path.chop(match.capturedLength());
    }

    // make relative paths absolute using the current working directory
    // prefer local file, if in doubt!
    url = QUrl::fromUserInput(path, QDir::currentPath(), QUrl::AssumeLocalFile);

    // in some cases, this will fail, e.g.
    // assume a local file and just convert it to an url.
    if (!url.isValid()) {
        // create absolute file path, we will e.g. pass this over dbus to other processes
        url = QUrl::fromLocalFile(QDir::current().absoluteFilePath(path));
    }
    return url;
}

bool MainWindow::processOption(QStringList &paslist)
{
    QCommandLineParser parser;

    if (!parser.parse(dApp->getDAppNew()->arguments())) {
        fputs(qPrintable(parser.helpText()), stdout);
        return false;
    }

    QString defaulttheme = dApp->setter->value("APP", "AppTheme").toString();

    if (DGuiApplicationHelper::LightType == DGuiApplicationHelper::instance()->themeType()) {
        dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Light);
    } else {
        dApp->viewerTheme->setCurrentTheme(ViewerThemeManager::Dark);
    }

    QStringList arguments = parser.positionalArguments();

    QString filepath = "";
    bool bneedexit = true;
    QStringList videos;
    for (const QString &path : arguments) {
        filepath = UrlInfo1(path).toLocalFile();

        QFileInfo info(filepath);
        QMimeDatabase db;
        QMimeType mt = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchContent);
        QMimeType mt1 = db.mimeTypeForFile(info.filePath(), QMimeDatabase::MatchExtension);

        QString str = info.suffix().toLower();
//        if (str.isEmpty()) {
        if (mt.name().startsWith("image/") || mt.name().startsWith("video/x-mng")
                || mt1.name().startsWith("image/") || mt1.name().startsWith("video/x-mng")) {
            if (utils::image::supportedImageFormats().contains(str, Qt::CaseInsensitive)) {
                bneedexit = false;
//                break;
                paslist << info.filePath();
                ImageEngineApi::instance()->insertImage(info.filePath(), "");
            } else if (str.isEmpty()) {
                bneedexit = false;
                paslist << info.filePath();
                ImageEngineApi::instance()->insertImage(info.filePath(), "");
//                break;
            }
        } else if (utils::base::isVideo(filepath)) {
            bneedexit = false;
            videos.push_back(path);
        }
    }
    if (!videos.isEmpty()) {
        ImageEngineApi::instance()->ImportImagesFromFileList(videos, "", -1, this, true);
    }
    if ("" != filepath && bneedexit) {
        exit(0);
    }
    return false;
}

//初始化中心界面
void MainWindow::initCentralWidget()
{
    m_pCenterWidget = new QStackedWidget(this);
    this->setCentralWidget(m_pCenterWidget);
    m_pCenterWidget->setFocusPolicy(Qt::NoFocus);
    AC_SET_OBJECT_NAME(m_pCenterWidget, MainWindow_Center_Widget);
    AC_SET_ACCESSIBLE_NAME(m_pCenterWidget, MainWindow_Center_Widget);
    m_pCenterWidget->lower();

    m_pAllPicView = new AllPicView();             //所有照片界面
    m_pTimeLineWidget = new QWidget();
    //m_pTimeLineView = new TimeLineView();       //时间线界面
    m_pAlbumWidget = new QWidget();
    //m_pAlbumview = new AlbumView();             //相册界面
    //m_pSearchView = new SearchView();           //搜索界面
    m_pSearchViewWidget = new QWidget();

//    m_commandLine = CommandLine::instance();
    m_imageViewer = new ImageViewer(imageViewerSpace::ImgViewerType::ImgViewerTypeAlbum, albumGlobal::CACHE_PATH, nullptr, m_pCenterWidget);
    m_imageViewer->setDropEnabled(false);
    connect(dApp->signalM, &SignalManager::sigViewImage, this, &MainWindow::onSigViewImage);
    m_back = m_imageViewer->getBottomtoolbarButton(imageViewerSpace::ButtonType::ButtonTypeBack);
    connect(m_back, &DIconButton::clicked, this, &MainWindow::onHideImageView);
    m_del = m_imageViewer->getBottomtoolbarButton(imageViewerSpace::ButtonType::ButtonTypeTrash);
    m_collect = m_imageViewer->getBottomtoolbarButton(imageViewerSpace::ButtonType::ButtonTypeCollection);
    //刷新收藏按钮
    connect(ImageEngine::instance(), &ImageEngine::sigUpdateCollectBtn, this, &MainWindow::updateCollectButton);
    connect(m_collect, &DIconButton::clicked, this, &MainWindow::onCollectButtonClicked);

    //刷新收藏按钮
    connect(ImageEngine::instance(), &ImageEngine::sigDel, this, &MainWindow::onLibDel);
    //获取所有自定义相册
    connect(ImageEngine::instance(), &ImageEngine::sigGetAlbumName, this, &MainWindow::onSendAlbumName);
    //公共库添加或新建相册请求
    connect(ImageEngine::instance(), &ImageEngine::sigAddToAlbumWithUID, this, &MainWindow::onAddToAlbum);
    //公共库收藏/取消收藏操作
    connect(ImageEngine::instance(), &ImageEngine::sigAddOrRemoveToFav, this, &MainWindow::onAddOrRemoveToFav);
    //公共库导出操作
    connect(ImageEngine::instance(), &ImageEngine::sigExport, this, &MainWindow::onExport);
    //公共库：从相册中移除操作
    connect(ImageEngine::instance(), &ImageEngine::sigRemoveFromCustomWithUID, this, &MainWindow::onRemoveFromCustom);
    //公共库：按下ESC键
    connect(ImageEngine::instance(), &ImageEngine::escShortcutActivated, this, &MainWindow::onEscShortcutActivated);
    //公共库：viewpanel界面图片全部删除
    connect(ImageEngine::instance(), &ImageEngine::sigPicCountIsNull, this, &MainWindow::onHideImageView);
    //公共库：旋转
    connect(ImageEngine::instance(), &ImageEngine::sigRotatePic, this, &MainWindow::onRotatePic);

    m_pCenterWidget->addWidget(m_pAllPicView);

    m_pCenterWidget->addWidget(m_pTimeLineWidget);
    m_pCenterWidget->addWidget(m_pAlbumWidget);

    m_pCenterWidget->addWidget(m_pSearchViewWidget);
    m_pCenterWidget->addWidget(m_imageViewer);

    QStringList parselist;
    processOption(parselist);
    if (parselist.length() > 0) {
        QStringList absoluteFilePaths;
        for (int i = 0; i < parselist.size(); i++) {
            QFileInfo inf(parselist.at(i));
            absoluteFilePaths << inf.absoluteFilePath();
        }
        ImageDataService::instance()->readThumbnailByPaths(absoluteFilePaths);
        ImageEngineApi::instance()->ImportImagesFromFileList(absoluteFilePaths, "", -1, this);
        //暂不支持视频首帧查看，只支持导入，视频和图片分开处理
        QString firstStr = absoluteFilePaths.at(0);
        if (utils::base::isVideo(firstStr)) {
            m_processOptionIsEmpty = true;
            m_pCenterWidget->setCurrentIndex(VIEW_ALLPIC);
        } else {
            m_processOptionIsEmpty = false;

            //查看图片
            SignalManager::ViewInfo info;
            info.paths = absoluteFilePaths;
            info.path = absoluteFilePaths.at(0);
            info.viewMainWindowID = VIEW_MAINWINDOW_ALLPIC;
            emit SignalManager::instance()->sigViewImage(info, Operation_NoOperation);
        }
    } else {
        //性能优化，此句在构造时不需要执行，增加启动时间
        m_processOptionIsEmpty = true;
        m_pCenterWidget->setCurrentIndex(VIEW_ALLPIC);
    }
}

void MainWindow::initInstallFilter()
{
    foreach (auto widget, m_emptyAllViewTabOrder) {
        widget->installEventFilter(this);
    }
    m_pSearchEdit->lineEdit()->installEventFilter(this);
}

// 界面无图时 tab切换顺序
void MainWindow::initNoPhotoNormalTabOrder()
{
    // titlebar tab键循环时会focus,隐掉
    m_emptyAllViewTabOrder.clear();
    m_emptyAllViewTabOrder.insert(0, m_pAllPicBtn);
    m_emptyAllViewTabOrder.insert(1, m_pTimeBtn);
    m_emptyAllViewTabOrder.insert(2, m_pAlbumBtn);
    QWidget *optionButton =  titlebar()->findChild<QWidget *>("DTitlebarDWindowOptionButton");
    QWidget *minButton =  titlebar()->findChild<QWidget *>("DTitlebarDWindowMinButton");
    QWidget *maxButton =  titlebar()->findChild<QWidget *>("DTitlebarDWindowMaxButton");
    QWidget *closeButton =  titlebar()->findChild<QWidget *>("DTitlebarDWindowCloseButton");
    m_emptyAllViewTabOrder.insert(3, optionButton);
    m_emptyAllViewTabOrder.insert(4, minButton);
    m_emptyAllViewTabOrder.insert(5, maxButton);
    m_emptyAllViewTabOrder.insert(6, closeButton);
    m_emptyAllViewTabOrder.insert(7, m_pAllPicView->m_pImportView->m_pImportBtn);

    // allpicview
    if (m_iCurrentView == VIEW_ALLPIC && m_pAllPicView->m_pStackedWidget->currentIndex() == 0) {
        m_emptyAllViewTabOrder[7] = m_pAllPicView->m_pImportView->m_pImportBtn;
    }
    // timelineview
    else if (m_iCurrentView == VIEW_TIMELINE && m_pTimeLineView->m_pStackedWidget->currentIndex() == 0) {
        m_emptyAllViewTabOrder[7] = m_pTimeLineView->pImportView->m_pImportBtn;
    }
    // albumview
    else if (m_iCurrentView == VIEW_ALBUM && m_pAlbumview->m_pRightStackWidget->currentIndex() == 0) {
        m_emptyAllViewTabOrder[7] = m_pAlbumview->m_pImportView->m_pImportBtn;
    }
    for (int idx = 0; idx < m_emptyAllViewTabOrder.count(); idx++) {
        m_emptyAllViewTabOrder.at(idx)->setFocusPolicy(Qt::TabFocus);
    }
    for (int idx = 0; idx < m_emptyAllViewTabOrder.count() - 1; idx++) {
        this->setTabOrder(m_emptyAllViewTabOrder.at(idx), m_emptyAllViewTabOrder.at(idx + 1));
    }
}

// allpicview界面存在图片时 tab切换顺序
void MainWindow::initAllpicViewTabOrder()
{
    if (!m_pAllPicView->getThumbnailListView())
        return;
    m_AllpicViewTabOrder.clear();
    m_AllpicViewTabOrder.insert(0, m_pAllPicBtn);
    m_AllpicViewTabOrder.insert(1, m_pTimeBtn);
    m_AllpicViewTabOrder.insert(2, m_pAlbumBtn);
    QWidget *optionButton =  titlebar()->findChild<QWidget *>("DTitlebarDWindowOptionButton");
    QWidget *minButton =  titlebar()->findChild<QWidget *>("DTitlebarDWindowMinButton");
    QWidget *maxButton =  titlebar()->findChild<QWidget *>("DTitlebarDWindowMaxButton");
    QWidget *closeButton =  titlebar()->findChild<QWidget *>("DTitlebarDWindowCloseButton");
    m_AllpicViewTabOrder.insert(3, m_pSearchEdit->lineEdit());
    m_AllpicViewTabOrder.insert(4, optionButton);
    m_AllpicViewTabOrder.insert(5, minButton);
    m_AllpicViewTabOrder.insert(6, maxButton);
    m_AllpicViewTabOrder.insert(7, closeButton);
    m_AllpicViewTabOrder.insert(8, m_pAllPicView->getThumbnailListView());

    if (m_AlbumViewTabOrder.count() > 0) {
        m_pAlbumBtn->setFocusPolicy(Qt::NoFocus);
        m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->setFocusPolicy(Qt::NoFocus);
    }
    for (int j = 0; j < m_AllpicViewTabOrder.count(); j++) {
        if (m_AllpicViewTabOrder.at(j)) {
            m_AllpicViewTabOrder.at(j)->setFocusPolicy(Qt::TabFocus);
        }
    }
    m_pSearchEdit->lineEdit()->setFocusPolicy(Qt::StrongFocus);
    for (int k = 0; k < m_AllpicViewTabOrder.count() - 1; k++) {
        this->setTabOrder(m_AllpicViewTabOrder.at(k), m_AllpicViewTabOrder.at(k + 1));
    }
}

void MainWindow::initTimeLineViewTabOrder()
{
    // 时间线listview空
    if (!m_pTimeLineView->getThumbnailListView())
        return;
    m_pTimeLineView->getThumbnailListView()->setFocusPolicy(Qt::TabFocus);
    m_TimelineViewTabOrder.clear();
    m_TimelineViewTabOrder.insert(1, m_pTimeBtn);
    m_TimelineViewTabOrder.insert(2, m_pTimeLineView->getThumbnailListView());
    for (int idx = 0; idx < m_emptyAllViewTabOrder.count(); idx++) {
        if (m_emptyAllViewTabOrder.at(idx) != nullptr) {
            m_emptyAllViewTabOrder.at(idx)->setFocusPolicy(Qt::NoFocus);
        }
    }

#ifndef tablet_PC
    m_pSearchEdit->lineEdit()->setFocusPolicy(Qt::ClickFocus);
#endif

    for (int idx = 0; idx < m_TimelineViewTabOrder.count(); idx++) {
#ifndef tablet_PC
        m_TimelineViewTabOrder.at(idx)->setFocusPolicy(Qt::TabFocus);
#else
        m_TimelineViewTabOrder.at(idx)->setFocusPolicy(Qt::NoFocus);
#endif
    }

    this->setTabOrder(m_TimelineViewTabOrder.at(0), m_TimelineViewTabOrder.at(1));
}

void MainWindow::initAlbumViewTabOrder()
{
    // 时间线listview空
    if (!m_pAlbumview->m_pImpTimeLineView->getListView())
        return;
    m_pAlbumview->m_pImpTimeLineView->getListView()->setFocusPolicy(Qt::TabFocus);
    m_AlbumViewTabOrder.clear();
    m_AlbumViewTabOrder.insert(0, m_pAlbumBtn);
    m_AlbumViewTabOrder.insert(1, m_pAlbumview->m_pLeftListView->m_pPhotoLibListView);
    m_AlbumViewTabOrder.insert(2, m_pAlbumview->m_pImpTimeLineView->getListView());
    for (int idx = 0; idx < m_emptyAllViewTabOrder.count(); idx++) {
        if (m_emptyAllViewTabOrder.at(idx) != nullptr) {
            m_emptyAllViewTabOrder.at(idx)->setFocusPolicy(Qt::NoFocus);
        }
    }

#ifndef tablet_PC
    m_pSearchEdit->lineEdit()->setFocusPolicy(Qt::ClickFocus);
#endif

    this->setTabOrder(m_AlbumViewTabOrder.at(0), m_AlbumViewTabOrder.at(1));
    this->setTabOrder(m_AlbumViewTabOrder.at(1), m_AlbumViewTabOrder.at(2));

    for (int idx = 0; idx < m_AlbumViewTabOrder.count(); idx++) {
#ifndef tablet_PC
        m_AlbumViewTabOrder.at(idx)->setFocusPolicy(Qt::TabFocus);
#else
        m_AlbumViewTabOrder.at(idx)->setFocusPolicy(Qt::NoFocus);
#endif
    }
}

void MainWindow::initEnterkeyAction(QObject *obj)
{
    if (m_pAllPicView->m_pStackedWidget->currentIndex() == 0 || m_iCurrentView == VIEW_ALLPIC) {
        // 相册内没有图时,应用初始启动无图时enter键进入
        QPushButton *pTempBtn = dynamic_cast<QPushButton *>(obj);
        if (pTempBtn && m_emptyAllViewTabOrder.contains(pTempBtn)) {
            emit pTempBtn->click();
        }
    }
    if (m_iCurrentView == VIEW_ALBUM) {
        // 相册内没有图时,应用初始启动无图时enter键进入
        QPushButton *pTempBtn = dynamic_cast<QPushButton *>(obj);
        if (pTempBtn && m_AlbumViewTabOrder.contains(pTempBtn)) {
            emit pTempBtn->click();
        } else if (obj == m_pAlbumview->m_pLeftListView->m_pPhotoLibListView) {
            QModelIndex tempIndex = m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->currentIndex();
            emit m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->pressed(tempIndex);
        } else if (obj == m_pAlbumview->m_pLeftListView->m_pMountListWidget) {
            QModelIndex tempIndex = m_pAlbumview->m_pLeftListView->m_pMountListWidget->currentIndex();
            emit m_pAlbumview->m_pLeftListView->m_pMountListWidget->pressed(tempIndex);
        } else if (obj == m_pAlbumview->m_pLeftListView->m_pCustomizeListView) {
            QModelIndex tempIndex = m_pAlbumview->m_pLeftListView->m_pCustomizeListView->currentIndex();
            emit m_pAlbumview->m_pLeftListView->m_pCustomizeListView->pressed(tempIndex);
        }
    }
}

bool MainWindow::initRightKeyOrder(QObject *obj)
{
    // 无图且当前界面非所有照片界面，屏蔽btngroup左右键切换
    if (m_pAllPicView->m_pStackedWidget->currentIndex() == 0 && m_iCurrentView != VIEW_ALLPIC) {
        if (m_emptyAllViewTabOrder.contains(dynamic_cast<QWidget *>(obj))) {
            if (m_emptyAllViewTabOrder.indexOf(dynamic_cast<QWidget *>(obj)) < 3) {
                return true;
            }
        }
    }
    // 无图且当前界面是所有照片界面时,标题栏焦点可以左右键切换，其他界面不允许切换
    else if (m_pAllPicView->m_pStackedWidget->currentIndex() == 0 && m_iCurrentView == VIEW_ALLPIC) {
        if (m_emptyAllViewTabOrder.contains(dynamic_cast<QWidget *>(obj))) {
            int index = m_emptyAllViewTabOrder.indexOf(dynamic_cast<QWidget *>(obj));
            // 焦点范围限定在标题栏
            QWidget *pTempWidget = dynamic_cast<QWidget *>(m_emptyAllViewTabOrder.at((index + 1) % (m_emptyAllViewTabOrder.count() - 1)));
            pTempWidget->setFocus();
            return true;
        }
    }
    // 有图所有照片界面
    else if (m_iCurrentView == VIEW_ALLPIC) {
        // 必须初始化tab order,因为第一次tab键不触发事件
        initAllpicViewTabOrder();
        if (m_AllpicViewTabOrder.contains(dynamic_cast<QWidget *>(obj)) && obj != m_pAllPicView->getThumbnailListView()) {
            int index = m_AllpicViewTabOrder.indexOf(dynamic_cast<QWidget *>(obj));
            // 焦点范围限定在标题栏
            QWidget *pTempWidget = dynamic_cast<QWidget *>(m_AllpicViewTabOrder.at((index + 1) % (m_AllpicViewTabOrder.count() - 1)));
            pTempWidget->setFocus();
            return true;
        }
    }
    return false;
}

bool MainWindow::initLeftKeyOrder(QObject *obj)
{
    // 无图且当前界面非所有照片界面，屏蔽btngroup左右键切换
    if (m_pAllPicView->m_pStackedWidget->currentIndex() == 0 && m_iCurrentView != VIEW_ALLPIC) {
        if (m_emptyAllViewTabOrder.contains(dynamic_cast<QWidget *>(obj))) {
            if (m_emptyAllViewTabOrder.indexOf(dynamic_cast<QWidget *>(obj)) < 3) {
                return true;
            }
        }
    }
    // 无图且当前界面是所有照片界面时,标题栏左右键切换
    if (m_pAllPicView->m_pStackedWidget->currentIndex() == 0 && m_iCurrentView == VIEW_ALLPIC) {
        if (m_emptyAllViewTabOrder.contains(dynamic_cast<QWidget *>(obj))) {
            int wgtIndex = m_emptyAllViewTabOrder.indexOf(dynamic_cast<QWidget *>(obj));
            // 焦点范围限定在标题栏
            QWidget *pTempWidget = dynamic_cast<QWidget *>(m_emptyAllViewTabOrder.at((wgtIndex + m_emptyAllViewTabOrder.count() - 2) % (m_emptyAllViewTabOrder.count() - 1)));
            pTempWidget->setFocus();
            return true;
        }
    }
    // 有图所有照片界面
    else if (m_iCurrentView == VIEW_ALLPIC) {
        if (m_AllpicViewTabOrder.contains(dynamic_cast<QWidget *>(obj)) && obj != m_pAllPicView->getThumbnailListView()) {
            // 针对listview不能左右键切换
            int wgtIndex = m_AllpicViewTabOrder.indexOf(dynamic_cast<QWidget *>(obj));
            // 焦点范围限定在标题栏
            QWidget *pTempWidget = dynamic_cast<QWidget *>(m_AllpicViewTabOrder.at((wgtIndex + m_AllpicViewTabOrder.count() - 2) % (m_AllpicViewTabOrder.count() - 1)));
            pTempWidget->setFocus();
            return true;
        }
    }
    return false;
}

bool MainWindow::initDownKeyOrder()
{
    // 相册界面，左边栏焦点向下循环切换
    if (m_iCurrentView == VIEW_ALBUM) {
        QList<LeftListWidget *> tempLeftListView;
        tempLeftListView.append(m_pAlbumview->m_pLeftListView->m_pPhotoLibListView);
        tempLeftListView.append(m_pAlbumview->m_pLeftListView->m_pMountListWidget);
        tempLeftListView.append(m_pAlbumview->m_pLeftListView->m_pCustomizeListView);
        for (int i = 0; i < tempLeftListView.count(); i++) {
            if (tempLeftListView.at(i)->hasFocus()) {
                if (tempLeftListView.at(i)->count() > 0 && tempLeftListView.at(i)->currentRow() == tempLeftListView.at(i)->count() - 1) {
                    bool isNotEmpty = true;
                    int cnt = 0;
                    // 设备左边栏列表可能不存在，跳过
                    while (isNotEmpty) {
                        if (tempLeftListView.at((i + 1 + cnt) % 3)->count() > 0) {
                            tempLeftListView.at(i)->setCurrentRow(-1);
                            tempLeftListView.at((i + 1 + cnt) % 3)->setFocus();
                            tempLeftListView.at((i + 1 + cnt) % 3)->setCurrentRow(0);
                            isNotEmpty = false;
                        } else {
                            cnt ++;
                        }
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

bool MainWindow::initUpKeyOrder()
{
    // 相册界面，左边栏焦点向上循环切换
    if (m_iCurrentView == VIEW_ALBUM) {
        QList<LeftListWidget *> tempLeftListView;
        tempLeftListView.append(m_pAlbumview->m_pLeftListView->m_pPhotoLibListView);
        tempLeftListView.append(m_pAlbumview->m_pLeftListView->m_pMountListWidget);
        tempLeftListView.append(m_pAlbumview->m_pLeftListView->m_pCustomizeListView);
        int showListViewCount = 0;
        foreach (auto listView, tempLeftListView) {
            if (listView->count() > 0) {
                showListViewCount ++;
            }
        }
        for (int i = 0; i < tempLeftListView.count(); i++) {
            if (tempLeftListView.at(i)->hasFocus()) {
                if (tempLeftListView.at(i)->count() > 0 && tempLeftListView.at(i)->currentRow() == 0) {
                    bool isNotEmpty = true;
                    int cnt = 0;
                    // 设备左边栏列表可能不存在，跳过
                    while (isNotEmpty) {
                        int loopListView = (i + showListViewCount - 1 + cnt) % 3;
                        if (tempLeftListView.at(loopListView)->count() > 0) {
                            tempLeftListView.at(i)->setCurrentRow(-1);
                            tempLeftListView.at(loopListView)->setFocus();
                            tempLeftListView.at(loopListView)->setCurrentRow(tempLeftListView.at(loopListView)->count() - 1);
                            isNotEmpty = false;
                        } else {
                            cnt ++;
                        }
                    }
                    return true;
                }
            }
        }
    }
    return false;
}

bool MainWindow::initAllViewTabKeyOrder(QObject *obj)
{
    if (m_pAllPicView->m_pStackedWidget->currentIndex() == 0) {
        // 相册内没有图时,应用初始启动无图时显示m_pImportView, tab键切换
        initNoPhotoNormalTabOrder();
    } else if (m_iCurrentView == VIEW_ALLPIC) {// 相册中有图，且在所有照片界面时
        // 所有照片界面 and listveiw tab键切换
        initAllpicViewTabOrder();
        // 当焦点在thumbnailListview中时,选中第一个
        QWidget *pTempWidget = static_cast<QWidget *>(obj);
        int index = m_AllpicViewTabOrder.indexOf(pTempWidget);
        // 当焦点在thumbnaillistview之前的一个widget上时，再次tab，默认选中第一个图片
        if (index == m_AllpicViewTabOrder.count() - 2) {
            //选中时先清除以前的选中状态
            m_pAllPicView->getThumbnailListView()->clearSelection();
            m_pAllPicView->getThumbnailListView()->setFocus();
            m_pAllPicView->getThumbnailListView()->selectFirstPhoto();
            return true;
        }
        // 当焦点在thumbnaillistview上时，第一个没选中/其他有选中，不可以tab键切换返回
        if (obj == m_pAllPicView->getThumbnailListView()) {
            if (!m_pAllPicView->getThumbnailListView()->isFirstPhotoSelected() && !m_pAllPicView->getThumbnailListView()->isNoPhotosSelected()) {
                return true;
            } else {
                m_pAllPicView->getThumbnailListView()->clearSelection();
            }
        }
    } else if (m_iCurrentView == VIEW_TIMELINE) {
        //  时间线界面
        initTimeLineViewTabOrder();
        ThumbnailListView *tempListView = m_pTimeLineView->getThumbnailListView();
        if (obj == m_pTimeBtn && tempListView) {
            tempListView->setFocus();
            //清除其他选中后再选中第一张
            m_pTimeLineView->clearAllSelection();
            tempListView->selectFirstPhoto();
            return true;
        } else if (obj == tempListView) {
            tempListView->clearSelection();
            m_pTimeBtn->setFocus();
            return true;
        }
    } else if (m_iCurrentView == VIEW_ALBUM) {
        // 相册内没有图时,应用初始启动无图时显示m_pImportView, tab键切换
        initAlbumViewTabOrder();
        ThumbnailListView *tempListView = m_pAlbumview->m_pImpTimeLineView->getListView();
        if (obj == m_AlbumViewTabOrder.at(1) && tempListView != nullptr) {
            tempListView->setFocus();
            //清除其他选中后再选中第一张
            m_pAlbumview->m_pImpTimeLineView->clearAllSelection();
            tempListView->selectFirstPhoto();
            m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->setCurrentRow(-1);
            return true;
        } else if (obj == tempListView) {
            tempListView->clearSelection();
            m_pAlbumBtn->setFocus();
            return true;
        } else if (obj == m_AlbumViewTabOrder.at(0)) {
            tempListView->clearSelection();
            m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->setCurrentRow(0);
        }
    }
    return false;
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

//显示所有照片
void MainWindow::allPicBtnClicked()
{
    clearFocus();
    m_pSearchEdit->clearEdit();
    m_SearchKey.clear();

    m_iCurrentView = VIEW_ALLPIC;
    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
    m_pAllPicView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
    m_pAllPicView->updateStackedWidget();
    m_pAllPicView->updatePicNum();

    m_pTimeBtn->setCheckable(true);
    m_pAlbumBtn->setCheckable(true);

    m_pAllPicBtn->setCheckable(true);
    m_pAllPicBtn->setChecked(true);
    initAllpicViewTabOrder();
    if (m_AllpicViewTabOrder.count() > 0 && m_bVector.at(0)) {
        m_pAllPicView->m_pImportView->m_pImportBtn->installEventFilter(this);
        m_bVector[0] = false;
    }
}

//显示时间线照片
void MainWindow::timeLineBtnClicked()
{
    clearFocus();
    if (nullptr == m_pTimeLineView) {
        m_pCenterWidget->removeWidget(m_pTimeLineWidget);
        int index = m_pCenterWidget->indexOf(m_pAllPicView) + 1;
        m_pTimeLineView = new TimeLineView();
        m_pCenterWidget->insertWidget(index, m_pTimeLineView);
//        m_pTimeLineView->clearAndStartLayout();
    }
    m_pSearchEdit->clearEdit();
    m_SearchKey.clear();
    m_iCurrentView = VIEW_TIMELINE;
    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
    m_pTimeLineView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
    m_pTimeLineView->updateStackedWidget();
    m_pTimeLineView->updatePicNum();
    m_pTimeLineView->m_pwidget->setFocus();

    m_pAllPicBtn->setCheckable(true);
    m_pAlbumBtn->setCheckable(true);
    m_pTimeBtn->setCheckable(true);
    m_pTimeBtn->setChecked(true);

    initTimeLineViewTabOrder();
    if (m_TimelineViewTabOrder.count() > 0 && m_bVector.at(1)) {
        if (m_pTimeLineView->getThumbnailListView())
            m_pTimeLineView->getThumbnailListView()->installEventFilter(this);
        // timelineview
        m_pTimeLineView->pImportView->m_pImportBtn->installEventFilter(this);
        m_bVector[1] = false;
    }
}

//显示相册
void MainWindow::albumBtnClicked()
{
    clearFocus();
    if (nullptr == m_pAlbumview) {
        createAlbumView();
    }
    m_pSearchEdit->clearEdit();
    m_SearchKey.clear();
    m_iCurrentView = VIEW_ALBUM;
    qDebug() << "------" << __FUNCTION__ << "count = " << m_pCenterWidget->count();
    m_pCenterWidget->setCurrentIndex(m_iCurrentView);

    m_pAlbumview->SearchReturnUpdate();
    m_pAlbumview->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
    qDebug() << "------" << __FUNCTION__ << "";
    m_pAlbumview->updateRightView();
    m_pAlbumview->m_pwidget->setFocus();

    m_pAllPicBtn->setCheckable(true);
    m_pTimeBtn->setCheckable(true);
    m_pAlbumBtn->setCheckable(true);
    m_pAlbumBtn->setChecked(true);

    initAlbumViewTabOrder();

    // albumview left list view up and down key event filter
    if (m_AlbumViewTabOrder.count() > 0 && m_bVector.at(2)) {
        m_pAlbumview->m_pImportView->m_pImportBtn->installEventFilter(this);
        if (m_pAlbumview->m_pImpTimeLineView->getListView())
            m_pAlbumview->m_pImpTimeLineView->getListView()->installEventFilter(this);
        m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->installEventFilter(this);
        m_pAlbumview->m_pLeftListView->m_pMountListWidget->installEventFilter(this);
        m_pAlbumview->m_pLeftListView->m_pCustomizeListView->installEventFilter(this);
        m_bVector[2] = false;
    }
}

//标题菜单栏槽函数
void MainWindow::onTitleBarMenuClicked(QAction *action)
{
    if (tr("New album") == action->text()) {
        emit dApp->signalM->createAlbum(QStringList(" "));
    }
}

//创建相册槽函数
void MainWindow::onCreateAlbum(const QStringList &imagepaths)
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

#ifdef tablet_PC
    ComDeepinImInterface::instance().setImActive(true);//拉起虚拟键盘
    QTimer::singleShot(200, []() { //平板未知问题，导致虚拟键盘会显示后瞬间消失，暂时这样规避
        ComDeepinImInterface::instance().setImActive(true);
    });
#endif

    //此处需要同时接收dialog获取的UID和album name
    connect(d, &AlbumCreateDialog::albumAdded, this, [ = ](int UID) {
        DBManager::instance()->insertIntoAlbum(UID, imgpath.isEmpty() ? QStringList(" ") : QStringList(imgpath));
        emit dApp->signalM->sigCreateNewAlbumFrom(d->getCreateAlbumName(), UID);
        QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess.svg");
        QString str = tr("Successfully added to “%1”");
        floatMessage(str.arg(d->getCreateAlbumName()), icon);
    });
}
#endif
//创建相册弹出窗
void MainWindow::showCreateDialog(QStringList imgpaths)
{
    AlbumCreateDialog *d = new AlbumCreateDialog(this);
    d->show();
    d->move(this->x() + (this->width() - d->width()) / 2, this->y() + (this->height() - d->height()) / 2);

#ifdef tablet_PC
    ComDeepinImInterface::instance().setImActive(true);//拉起虚拟键盘
    QTimer::singleShot(200, []() { //平板未知问题，导致虚拟键盘会显示后瞬间消失，暂时这样规避
        ComDeepinImInterface::instance().setImActive(true);
    });
#endif

    connect(d, &AlbumCreateDialog::albumAdded, this, [ = ](int UID) {
        //double insert problem from here ,first insert at AlbumCreateDialog::createAlbum(albumname)
        if (nullptr == m_pAlbumview) {
            createAlbumView();
        }
        emit dApp->signalM->sigCreateNewAlbumFromDialog(d->getCreateAlbumName(), UID);
        m_pAlbumBtn->setChecked(true);
        m_pSearchEdit->clearEdit();
        m_SearchKey.clear();
        m_pAlbumview->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
        m_backIndex = VIEW_ALBUM;
        DBManager::instance()->insertIntoAlbum(UID, imgpaths);
        emit dApp->signalM->insertedIntoAlbum(UID, imgpaths);
        emit dApp->signalM->hideImageView();    //该信号针对查看界面新建相册(快捷键 crtl+n)，正常退出
    });
}

void MainWindow::onNewPathAction()
{
    if (m_pAlbumview == nullptr) {
        createAlbumView();
    }

    auto url = QFileDialog::getExistingDirectoryUrl(nullptr, "", QUrl());

    if (url.isEmpty()) {
        return;
    }

    if (!url.isLocalFile() || !url.isValid()) { //远程路径、非法路径
        onNotSupportedNotifyPath();
        return;
    }

    auto path = url.toLocalFile();
    QDir dir(path);
    if (path.startsWith("/media/") || // U盘
            utils::base::isVaultFile(path) || //保险箱
            path == QDir::homePath() || //主文件夹
            path == "/" || //根目录
            !dir.exists()) { //目录不存在
        onNotSupportedNotifyPath();
        return;
    }

    if (m_pAlbumview->checkIfNotified(path)) { //已处于监控
        onNotifyPathIsExists();
        return;
    }

    //执行添加
    m_pAlbumview->onAddNewNotifyDir(path);
}

void MainWindow::createAlbumView()
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
}

//搜索框
void MainWindow::onSearchEditFinished()
{
    qDebug() << __FUNCTION__ << m_pSearchEdit->hasFocus();
    QString keywords = m_pSearchEdit->text();
    if (m_SearchKey == keywords) //两次搜索条件相同，跳过
        return;
    m_SearchKey = keywords;
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
                emit dApp->signalM->sigSendKeywordsIntoALLPic(keywords, COMMON_STR_FAVORITES, DBManager::SpUID::u_Favorite);
            } else if (COMMON_STR_CUSTOM == m_pAlbumview->m_pLeftListView->getItemCurrentType()) {
                emit dApp->signalM->sigSendKeywordsIntoALLPic(keywords, m_pAlbumview->m_pLeftListView->getItemCurrentName(), m_pAlbumview->m_pLeftListView->getItemCurrentUID());
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
    //添加视频过滤
    for (const QString &i : utils::base::m_videoFiletypes)
        sList << i;
    QString filter = tr("All photos and videos");
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
    dialog.setWindowTitle(tr("Import Photos and Videos"));
    dialog.setAllowMixedSelection(true);
    const int mode = dialog.exec();
    if (mode != QDialog::Accepted) {
        return;
    }
    const QStringList &file_list = dialog.selectedFiles();
    if (file_list.isEmpty())
        return;
    ImageDataService::instance()->readThumbnailByPaths(file_list);
    if (m_iCurrentView == VIEW_ALBUM) {
        if (m_pAlbumview->m_currentType == ALBUM_PATHTYPE_BY_PHONE || m_pAlbumview->m_currentItemType == 0) {
            ImageEngineApi::instance()->ImportImagesFromFileList(file_list, "", -1, this, true);
        } else {
            ImageEngineApi::instance()->ImportImagesFromFileList(file_list, m_pAlbumview->m_currentAlbum, m_pAlbumview->m_currentUID, this, true);
        }
    } else {
        ImageEngineApi::instance()->ImportImagesFromFileList(file_list, "", -1, this, true);
    }

}

bool MainWindow::imageImported(bool success)
{
    emit dApp->signalM->closeWaitDialog();
    emit sigImageImported(success);
    return true;
}

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
        pwidget = m_imageViewer;
        break;
    default:
        pwidget = m_pAllPicView->m_pwidget;
        break;
    }
    DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, pwidget);
    pDFloatingMessage->setBlurBackgroundEnabled(true);
    pDFloatingMessage->raise();
    pDFloatingMessage->setMessage(tempStr);
    pDFloatingMessage->setIcon(icon);
    if (pwidget) {
        if (icon.isNull()) {
            QLabel *tempLabel = new QLabel(pDFloatingMessage);
            tempLabel->setFixedWidth(8);
            pDFloatingMessage->setWidget(tempLabel);
        }
        DMessageManager::instance()->sendMessage(pwidget, pDFloatingMessage);
    }
}

void MainWindow::updateCollectButton()
{
    if (m_imageViewer == nullptr) {
        return;
    }
    QString path = m_imageViewer->getCurrentPath();
    if (path.isEmpty()) {
        return;
    }
    if (DBManager::instance()->isImgExistInAlbum(DBManager::SpUID::u_Favorite, path, AlbumDBType::Favourite)) {
        m_collect->setToolTip(tr("Unfavorite"));
        m_collect->setIcon(QIcon::fromTheme("dcc_ccollection"));
        m_collect->setIconSize(QSize(36, 36));
    } else {
        m_collect->setToolTip(tr("Favorite"));
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        Q_UNUSED(themeType);
        m_collect->setIcon(QIcon::fromTheme("dcc_collection_normal"));
        m_collect->setIconSize(QSize(36, 36));
    }
}

void MainWindow::onLibDel()
{
    if (m_imageViewer == nullptr) {
        return;
    }
    QString path = m_imageViewer->getCurrentPath();
    if (path.isEmpty()) {
        return;
    }

    DBImgInfoList infos;
    DBImgInfo info;
    info = DBManager::instance()->getInfoByPath(path);
    info.importTime = QDateTime::currentDateTime();
    infos << info;
    DBManager::instance()->insertTrashImgInfos(infos);
    DBManager::instance()->removeImgInfos(QStringList(path));
}

void MainWindow::onAddToAlbum(bool isNew, int UID, const QString &path)
{
    if (isNew) {
        emit dApp->signalM->viewCreateAlbum(path, false);
    } else {
        if (! DBManager::instance()->isImgExistInAlbum(UID, path)) {
            emit dApp->signalM->sigAddToAlbToast(DBManager::instance()->getAlbumNameFromUID(UID));//提示
        }
        DBManager::instance()->insertIntoAlbum(UID, {path});
    }
}

void MainWindow::onAddOrRemoveToFav(const QString &path, bool isAdd)
{
    Q_UNUSED(isAdd)
    if (!DBManager::instance()->isImgExistInAlbum(DBManager::SpUID::u_Favorite, path, AlbumDBType::Favourite)) {//收藏
        DBManager::instance()->insertIntoAlbum(DBManager::SpUID::u_Favorite, QStringList(path), AlbumDBType::Favourite);
        emit dApp->signalM->insertedIntoAlbum(DBManager::SpUID::u_Favorite, QStringList(path));
    } else { //取消收藏
        DBManager::instance()->removeFromAlbum(DBManager::SpUID::u_Favorite, QStringList(path), AlbumDBType::Favourite);
    }
}

void MainWindow::onExport(const QString &path)
{
    emit dApp->signalM->exportImage(QStringList(path));
}

void MainWindow::onRemoveFromCustom(const QString &path, int UID)
{
    if (UID > 0) {
        DBManager::instance()->removeFromAlbum(UID, QStringList(path));
    }
}

void MainWindow::onRotatePic(const QString &path)
{
    if (!path.isEmpty()) {
        //公共库默认只传输路径，图片需相册加载
        QImage tImg;
        dApp->m_imageloader->updateImageLoader(QStringList(path), QList<QImage>({tImg}));
    }
}

//外部使用相册打开图片
void MainWindow::onNewAPPOpen(qint64 pid, const QStringList &arguments)
{
    qDebug() << __FUNCTION__ << "---";
    Q_UNUSED(pid);
    QStringList paths;
    if (arguments.length() > 1) {
        //arguments第1个参数是进程名，图片paths参数需要从下标1开始
        for (int i = 1; i < arguments.size(); ++i) {
            QString qpath = arguments.at(i);
            //BUG#79815，添加文件URL解析（BUG是平板上的，为防止UOS的其它个别版本也改成传URL，干脆直接全部支持）
            auto urlPath = QUrl(qpath);
            if (urlPath.scheme() == "file") {
                qpath = urlPath.toLocalFile();
            }
            paths.append(qpath);
        }
        if (paths.count() > 0) {
            QString firstStr = paths.at(0);
            ImageEngineApi::instance()->ImportImagesFromFileList(paths, "", -1, this);
            if (utils::base::isVideo(firstStr)) {
                if (m_pCenterWidget->currentIndex() == VIEW_IMAGE) {
                    onHideImageView();
                }
                m_pCenterWidget->setCurrentIndex(VIEW_ALLPIC);

                //获取视频信息
                MovieInfo movieInfo = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(firstStr));
                //线程加载缩略图
                if (movieInfo.valid) {
                    ImageDataService::instance()->readThumbnailByPaths(paths);
                }
            } else {
                SignalManager::ViewInfo info;
                info.path = paths.at(0);
                info.paths = paths;
                info.viewMainWindowID = VIEW_MAINWINDOW_ALLPIC;
                emit dApp->signalM->sigViewImage(info, Operation_NoOperation);
            }
        }
        m_pAllPicBtn->setChecked(true);
    }
    this->activateWindow();
}

void MainWindow::onSigViewImage(const SignalManager::ViewInfo &info, OpenImgAdditionalOperation operation, bool isCustom, const QString &album, int UID)
{
    m_backIndex = info.viewMainWindowID;
    auto finalUseAlbum = album;

    if (operation == Operation_NoOperation || operation == Operation_FullScreen) {
        if (DBManager::instance()->getAlbumDBTypeFromUID(UID) == AutoImport) { //如果是自动导入的相册，则禁用从相册删除功能
            finalUseAlbum.clear();
            UID = -1;
            isCustom = false;
        }
    }

    switch (operation) {
    case Operation_NoOperation:
        m_imageViewer->startdragImageWithUID(info.paths, info.path, isCustom, finalUseAlbum, UID);
        m_backIndex_fromFullScreen = VIEW_IMAGE;
        break;
    case Operation_FullScreen:
        if (window()->isMaximized()) {
            m_backToMaxWindow = true;
        } else if (window()->isFullScreen()) {
            ;
        } else {
            m_backToMaxWindow = false;
        }
        m_imageViewer->startdragImageWithUID(info.paths, info.path, isCustom, finalUseAlbum, UID);
        m_imageViewer->switchFullScreen();
        m_backIndex_fromFullScreen = info.viewMainWindowID;
        break;
    case Operation_StartSliderShow:
        //BUG#100660 过滤掉视频
        auto paths = info.paths;
        auto iter = std::remove_if(paths.begin(), paths.end(), [](const QString & path) {
            return utils::base::isVideo(path);
        });
        paths.erase(iter, paths.end());
        if (utils::base::isVideo(info.path) && !paths.isEmpty()) {
            m_imageViewer->startSlideShow(paths, paths.at(0));
        } else {
            m_imageViewer->startSlideShow(paths, info.path);
        }
        m_backIndex_fromFullScreen = info.viewMainWindowID;
        break;
    }

    setTitleBarHideden(true);
    m_pCenterWidget->setCurrentIndex(VIEW_IMAGE);

    //禁用冲突的快捷键
    setConflictShortcutEnabled(false);
}

void MainWindow::onHideImageView()
{
    if (window()->isFullScreen()) {
        m_imageViewer->switchFullScreen();
        if (m_backToMaxWindow) {
            window()->showNormal();
            window()->showMaximized();
        }
    } else if (m_backToMaxWindow) {
        window()->showNormal();
        window()->showMaximized();
    }

    setTitleBarHideden(false);
    m_pCenterWidget->setCurrentIndex(m_backIndex);

    //外部打开图片，返回时默认跳转到所有照片界面
    if (m_backIndex == VIEW_ALLPIC) {
        allPicBtnClicked();
    }

    setConflictShortcutEnabled(true);
}

void MainWindow::onCollectButtonClicked()
{
    qDebug() << __FUNCTION__ << "---";
    if (m_imageViewer == nullptr) {
        return;
    }
    QString path = m_imageViewer->getCurrentPath();
    if (path.isEmpty()) {
        return;
    }
    if (DBManager::instance()->isImgExistInAlbum(DBManager::SpUID::u_Favorite, path, AlbumDBType::Favourite)) {
        DBManager::instance()->removeFromAlbum(DBManager::SpUID::u_Favorite, QStringList(path), AlbumDBType::Favourite);
        m_collect->setToolTip(tr("Favorite"));
        m_collect->setIcon(QIcon::fromTheme("dcc_collection_normal"));
        m_collect->setIconSize(QSize(36, 36));
    } else {
        DBManager::instance()->insertIntoAlbum(DBManager::SpUID::u_Favorite, QStringList(path), AlbumDBType::Favourite);
        m_collect->setToolTip(tr("Unfavorite"));
        m_collect->setIcon(QIcon::fromTheme("dcc_ccollection"));
        m_collect->setIconSize(QSize(36, 36));
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

            initShortcut();
            initConnections();

            if (m_needBanShortcutButNotReady) {
                m_needBanShortcutButNotReady = false;
                setConflictShortcutEnabled(false);
            }

            if (0 < DBManager::instance()->getImgsCount()) {
                m_pSearchEdit->setEnabled(true);
            } else {
                m_pSearchEdit->setEnabled(false);
            }
        }
        m_isFirstStart = false;
    }, Qt::QueuedConnection);

    //启动路径监控
    QStringList monitorPaths;
    QStringList monitorAlbumNames;
    QList<int>  monitorAlbumUIDs;

    auto stdPicPaths = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    if (!stdPicPaths.isEmpty()) {
        auto stdPicPath = stdPicPaths[0];

        monitorPaths.push_back(stdPicPath + "/" + "Camera");
        monitorPaths.push_back(stdPicPath + "/" + "Screen Capture");
        monitorPaths.push_back(stdPicPath + "/" + "Draw");

        monitorAlbumNames.push_back(tr("Camera"));
        monitorAlbumNames.push_back(tr("Screen Capture"));
        monitorAlbumNames.push_back(tr("Draw"));

        monitorAlbumUIDs.push_back(DBManager::SpUID::u_Camera);
        monitorAlbumUIDs.push_back(DBManager::SpUID::u_ScreenCapture);
        monitorAlbumUIDs.push_back(DBManager::SpUID::u_Draw);
    }

    auto stdMoviePaths = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    if (!stdMoviePaths.isEmpty()) {
        auto stdMoviePath = stdMoviePaths[0];

        monitorPaths.push_back(stdMoviePath + "/" + "Camera");
        monitorPaths.push_back(stdMoviePath + "/" + "Screen Capture");

        monitorAlbumNames.push_back(tr("Camera"));
        monitorAlbumNames.push_back(tr("Screen Capture"));

        monitorAlbumUIDs.push_back(DBManager::SpUID::u_Camera);
        monitorAlbumUIDs.push_back(DBManager::SpUID::u_ScreenCapture);
    }

    startMonitor(monitorPaths, monitorAlbumNames, monitorAlbumUIDs);

    //读取并加载自定义的自动导入路径
    ImportImagesFromCustomAutoPaths();
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
        dApp->signalM->emitSliderValueChg(m_pSliderPos);
    } else {
        m_pSliderPos = 4;
    }

    m_pAllPicView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
    dApp->signalM->emitSliderValueChg(m_pSliderPos);
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
    m_settings = new QSettings(userConfigPath, QSettings::IniFormat, this);
}

//缩略图放大
void MainWindow::thumbnailZoomIn()
{
    if (VIEW_IMAGE == m_pCenterWidget->currentIndex()) {
//        emit dApp->signalM->sigCtrlADDKeyActivated();
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

            dApp->signalM->emitSliderValueChg(m_pSliderPos);
        }
    }
}

//缩略图缩小
void MainWindow::thumbnailZoomOut()
{
    if (VIEW_IMAGE == m_pCenterWidget->currentIndex()) {
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

            dApp->signalM->emitSliderValueChg(m_pSliderPos);
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
    shortcut10.insert("name", tr("Export photos"));
    shortcut10.insert("value", "Ctrl+E");
    QJsonObject shortcut11;
    shortcut11.insert("name", tr("Import photos/videos"));
    shortcut11.insert("value", "Ctrl+O");
    QJsonObject shortcut12;
    shortcut12.insert("name", tr("Select all"));
    shortcut12.insert("value", "Ctrl+A");
    QJsonObject shortcut13;
    shortcut13.insert("name", tr("Copy"));
    shortcut13.insert("value", "Ctrl+C");
    QJsonObject shortcut14;
    shortcut14.insert("name", tr("Delete"));
    shortcut14.insert("value", "Delete");
    QJsonObject shortcut15;
    shortcut15.insert("name", tr("Photo/Video info"));
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

void MainWindow::startMonitor(const QStringList &paths, const QStringList &albumNames, const QList<int> UIDs)
{
    for (int i = 0; i != paths.size(); ++i) {
        if (paths.at(i).size() > 0) {
            m_fileInotifygroup->startWatch(paths.at(i), albumNames.at(i), UIDs.at(i));
        }
    }
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

void MainWindow::setTitleBarHideden(bool hide)
{
    if (hide) {
        titlebar()->setFixedHeight(0);
        setTitlebarShadowEnabled(false);
    } else {
        titlebar()->setFixedHeight(50);
        setTitlebarShadowEnabled(true);
    }
}

void MainWindow::closeFromMenu()
{
    //这个函数是从overrideVfptrFun进来的，要指定obj对象
    instance().close();
}

void MainWindow::onButtonClicked(int id)
{
    if (0 == id) {
        allPicBtnClicked();
#ifdef tablet_PC
#else
        m_pSearchEdit->setVisible(true);
#endif
    }
    int index = 0;
    if (1 == id) {
        if (nullptr == m_pTimeLineView) {
            m_pCenterWidget->removeWidget(m_pTimeLineWidget);
            index = m_pCenterWidget->indexOf(m_pAllPicView) + 1;
            m_pTimeLineView = new TimeLineView();
            m_pCenterWidget->insertWidget(index, m_pTimeLineView);
//            m_pTimeLineView->clearAndStartLayout();
        }
        timeLineBtnClicked();
#ifdef tablet_PC
#else
        m_pSearchEdit->setVisible(true);
#endif
    }
    if (2 == id) {
        if (nullptr == m_pAlbumview) {
            createAlbumView();
        }
        albumBtnClicked();
        // 如果是最近删除或者移动设备,则搜索框不显示
        if (2 == m_pAlbumview->m_pRightStackWidget->currentIndex() || 5 == m_pAlbumview->m_pRightStackWidget->currentIndex()) {
            m_pSearchEdit->setVisible(false);
        } else {
#ifdef tablet_PC
#else
            m_pSearchEdit->setVisible(true);
#endif
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
        countText  = QString(QObject::tr("%1/%2 items imported")).arg(completefiles).arg(allfiles);
    } else {
        countText = QString(QObject::tr("%1/%2 items deleted")).arg(completefiles).arg(allfiles);
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
        if (waittext.compare(tr("Importing..."), Qt::CaseInsensitive) >= 0) {
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

void MainWindow::ImportImagesFromCustomAutoPaths()
{
    auto customAutoImportUIDAndPaths = DBManager::instance()->getAllCustomAutoImportUIDAndPath();

    //first -> UID, secend -> path
    for (auto &eachItem : customAutoImportUIDAndPaths) {
        //1.获取原有的路径
        auto originPaths = DBManager::instance()->getPathsByAlbum(eachItem.first, AutoImport);

        //2.获取现在的路径
        QFileInfoList infos;
        utils::image::getAllFileInDir(eachItem.second, infos);
        QStringList currentPaths;
        for (auto &info : infos) {
            currentPaths.push_back(info.absoluteFilePath());
        }

        //3.获取已不存在的路径
        QStringList deleteFiles;
        for (int i = 0; i != originPaths.size(); ++i) {
            if (!currentPaths.contains(originPaths[i])) {
                deleteFiles << originPaths[i];
            }
        }

        //注意：导入新图由于需要制作缩略图，因此是异步多线程的，而移除不存在的图只需要操作数据库，所以是单线程的
        //所以先执行移除，再执行导入

        //4.删除不存在的路径
        ImageEngineApi::instance()->removeImageFromAutoImport(deleteFiles, eachItem.first);

        //5.执行导入
        ImageEngineApi::instance()->ImportImagesFromFileList(currentPaths, eachItem.second.split('/').last(), eachItem.first, this, true, AutoImport);
    }
}

void MainWindow::setConflictShortcutEnabled(bool enable)
{
    //BUG#101119 快捷键初始化位于show event中，但命令行启动的时候，显示图片的指令会先于show event函数执行，因此需要加flag判断
    if (!enable && (m_CtrlUp == nullptr || m_ReCtrlUp == nullptr || m_CtrlDown == nullptr)) {
        m_needBanShortcutButNotReady = true;
        return;
    } else if (enable && (m_CtrlUp == nullptr || m_ReCtrlUp == nullptr || m_CtrlDown == nullptr)) {
        m_needBanShortcutButNotReady = false;
        return;
    }

    m_CtrlUp->setEnabled(enable);
    m_ReCtrlUp->setEnabled(enable);
    m_CtrlDown->setEnabled(enable);
}

void MainWindow::onAddImageBtnClicked(bool checked)
{
    Q_UNUSED(checked)
    emit dApp->signalM->startImprot();
    emit sigTitleMenuImportClicked();
}

void MainWindow::onHideSlidePanel()
{
    if (VIEW_IMAGE != m_backIndex_fromFullScreen) {
        setTitleBarHideden(false);
    }

    m_pCenterWidget->setCurrentIndex(m_backIndex_fromFullScreen);
}

void MainWindow::onExportImage(QStringList paths)
{
    Exporter::instance()->exportImage(paths);
}

void MainWindow::onMainwindowSliderValueChg(int step)
{
    m_pSliderPos = step;
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
    QIcon icon;
    QString str = QObject::tr("The photo/video already exists");
    floatMessage(str, icon);
}

void MainWindow::onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, int UID)
{
    Q_UNUSED(UID)
    QIcon icon(":/images/logo/resources/images/other/info_ash.svg");
    QString str;

    if (duplicatePaths.size() == 1 && importPaths.isEmpty()) { //仅导入一个文件且这个文件还已经在相册里了
        str = QObject::tr("The photo/video already exists");
    } else {
        str = QObject::tr("%1 items imported, %2 items exist already").arg(importPaths.count()).arg(duplicatePaths.count());
    }
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
    QString str = QObject::tr("%1 items imported, %2 items failed");
    QString str1 = QString::number(successful, 10);
    QString str2 = QString::number(failed, 10);
    QString res = str.arg(str1).arg(str2);
    floatMessage(res, icon);
}

void MainWindow::onImportDonotFindPicOrVideo()
{
    QIcon icon(":/images/logo/resources/images/other/warning_new.svg");
    QString str = tr("No photos or videos found");
    floatMessage(str, icon);
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

void MainWindow::onNotSupportedNotifyPath()
{
    QIcon icon(":/images/logo/resources/images/other/warning_new.svg");
    QString str = tr("Cannot add this path, please try another one");
    floatMessage(str, icon);
}

void MainWindow::onNotifyPathIsExists()
{
    QIcon icon(":/images/logo/resources/images/other/warning_new.svg");
    QString str = tr("The path already exists");
    floatMessage(str, icon);
}

void MainWindow::onEscShortcutActivated(bool isSwitchFullScreen)
{
    if (!(isSwitchFullScreen && m_backIndex_fromFullScreen == VIEW_IMAGE)) {
        onHideImageView();
    }
}

void MainWindow::onDelShortcutActivated()
{
    emit dApp->signalM->sigShortcutKeyDelete();

    //BUG#101527 同其它界面的策略一样，仅在界面切换至此时触发删除动作
    if (m_pCenterWidget->currentIndex() == VIEW_IMAGE) {
        m_del->click();
    }
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

    QProcess *shortcutViewProcess = new QProcess(this);
    shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

    connect(shortcutViewProcess, SIGNAL(finished(int)), shortcutViewProcess, SLOT(deleteLater()));
}

void MainWindow::onCtrlFShortcutActivated()
{
    m_pSearchEdit->lineEdit()->setFocus();
}

void MainWindow::onSearchEditIsDisplay(bool bIsDisp)
{
    if (m_pCenterWidget->currentIndex() == VIEW_ALBUM) {
#ifdef tablet_PC
        if (!bIsDisp) {
            m_pSearchEdit->setVisible(bIsDisp);
        }
#else
        m_pSearchEdit->setVisible(bIsDisp);
#endif
    }
}

void MainWindow::onSendAlbumName(const QString &path)
{
    auto allalbums = DBManager::instance()->getAllAlbumNames(Custom);

    QMap<int, std::pair<QString, bool>> map; //UID，相册名称，是否包含
    QStringList paths;
    paths.append(path);

    for (auto &album : allalbums) {
        if (DBManager::instance()->isAllImgExistInAlbum(album.first, paths, AlbumDBType::Custom)) {
            map.insert(album.first, std::make_pair(album.second, true));
        } else {
            map.insert(album.first, std::make_pair(album.second, false));
        }
    }
    bool isFav = false;
    if (DBManager::instance()->isImgExistInAlbum(DBManager::SpUID::u_Favorite, path, AlbumDBType::Favourite)) {
        isFav = true;
    }

    m_imageViewer->setCustomAlbumNameAndUID(map, isFav);
}
