#include "mainwindow.h"
#include "controller/commandline.h"
#include "dialogs/albumcreatedialog.h"
#include "utils/snifferimageformat.h"
#include "imageengine/imageengineapi.h"
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
{
//    ImageEngineApi::instance(this);
    m_allPicNum = DBManager::instance()->getImgsCount();        //获取图片总数
    m_iCurrentView = VIEW_ALLPIC;       //默认显示当前所有图片视图
    m_bTitleMenuImportClicked = false;
    m_waitdailog = new DDialog(this);
    initShortcutKey();          //初始化各种快捷键
    initUI();
    initTitleBar();             //初始化顶部状态栏
//    initCentralWidget();
//    initShortcut();
//    initConnections();

    m_pCenterWidget = new QStackedWidget(this);
    m_pAllPicView = new AllPicView();           //所有照片界面
    m_pCenterWidget->addWidget(m_pAllPicView);
    m_pCenterWidget->setFixedSize(size());
    m_pCenterWidget->lower();

    initCentralWidget();
    initShortcut();
    initConnections();
//    setCentralWidget(m_pCenterWidget);

//    timer = startTimer(500);
    loadZoomRatio();
}

MainWindow::~MainWindow()
{
    //delete m_pAlbumview;                    //相册照片界面视图
    //delete m_pAllPicView;                  //所有照片界面视图
    //delete m_pTimeLineView;              //时间线界面视图
    //delete m_pSearchView;                  //搜索界面视图
    //exit(0);
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    m_pCenterWidget->setFixedSize(size());
}


//void MainWindow::timerEvent(QTimerEvent *e)
//{
//    /*
//       if (e->timerId() == timer) {
//           killTimer(timer);
//           timer = 0;
//           initCentralWidget();
//           initShortcut();
//           initConnections();
//       }*/
//}

//初始化所有连接
void MainWindow::initConnections()
{
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList &");
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [ = ] {
        setWaitDialogColor();
    });
    //主界面切换（所有照片、时间线、相册）
    connect(btnGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, [ = ](int id) {
        if (0 == id) {
            allPicBtnClicked();
            m_pSearchEdit->setVisible(true);
        }
        if (1 == id) {
            timeLineBtnClicked();
            m_pSearchEdit->setVisible(true);
        }
        if (2 == id) {
            albumBtnClicked();

            // 如果是最近删除或者移动设备,则搜索框不显示
            if (2 == m_pAlbumview->m_pRightStackWidget->currentIndex() || 5 == m_pAlbumview->m_pRightStackWidget->currentIndex()) {
                m_pSearchEdit->setVisible(false);
            } else {
                m_pSearchEdit->setVisible(true);
            }
        }
    });

    //图片导入槽函数
    connect(this, &MainWindow::sigImageImported, this, [ = ](bool success) {
        if (success) {
            if (ALBUM_PATHTYPE_BY_PHONE == m_pAlbumview->m_pLeftListView->getItemCurrentType()) {
                m_pAlbumview->m_currentAlbum = ALBUM_PATHTYPE_BY_PHONE;
            }
        }
        if (m_pCenterWidget->currentIndex() == VIEW_ALBUM
                && ALBUM_PATHTYPE_BY_PHONE == m_pAlbumview->m_pLeftListView->getItemCurrentType()) {
            m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->setCurrentRow(0);
        }
    });

    connect(dApp->signalM, &SignalManager::createAlbum, this, &MainWindow::onCreateAlbum);
#if 1
    connect(dApp->signalM, &SignalManager::viewModeCreateAlbum, this, &MainWindow::onViewCreateAlbum);
#endif

    connect(m_pSearchEdit, &DSearchEdit::editingFinished, this, &MainWindow::onSearchEditFinished);
    connect(m_pTitleBarMenu, &DMenu::triggered, this, &MainWindow::onTitleBarMenuClicked);
    connect(this, &MainWindow::sigTitleMenuImportClicked, this, &MainWindow::onImprotBtnClicked);
    //当有图片添加时，搜索栏可用
    connect(dApp->signalM, &SignalManager::imagesInserted, this, [ = ] {
        m_pSearchEdit->setEnabled(true);
    });
    //开始导入时，清空状态
    connect(dApp->signalM, &SignalManager::startImprot, this, [ = ] {
        m_countLabel->setText("");
        m_importBar->setValue(0);
    });
    //更新等待进度条
    connect(dApp->signalM, &SignalManager::progressOfWaitDialog, this, [ = ](int allfiles, int completefiles) {
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
    });
    //更新等待窗口文本信息
    connect(dApp->signalM, &SignalManager::popupWaitDialog, this, [ = ](QString waittext, bool bneedprogress) {
        if (bneedprogress == false) {
            m_importBar->hide();
            m_countLabel->hide();
            m_waitlabel->setText(waittext);
            m_waitlabel->move(40, 30);
            m_waitlabel->show();
            m_waitdailog->show();
        } else {

            if (!waittext.compare(tr("Importing..."))) {
                //            m_waitlabel->setText(waittext);
                m_bImport = true;
            } else {
                //            m_waitlabel->setText(waittext);
                m_bImport = false;
            }
            m_waitlabel->move(40, 7);
            m_waitlabel->setText(waittext);
            m_importBar->show();
            m_waitlabel->show();
            //        m_countLabel->hide();
            //        m_spinner->start();
            //        m_waitdailog.exec();
            m_waitdailog->show();
        }
    });
    //关闭等待窗口
    connect(dApp->signalM, &SignalManager::closeWaitDialog, this, [ = ]() {
//        m_spinner->stop();
        m_bImport = false;
        m_countLabel->setText("");
        m_waitdailog->close();
    });
    //图片移除，判断是否更新搜索框
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, [ = ] {
        if (0 < DBManager::instance()->getImgsCount())
        {
            m_pSearchEdit->setEnabled(true);
        } else
        {
            m_pSearchEdit->setEnabled(false);
        }
    });
    //显示图片视图
    connect(dApp->signalM, &SignalManager::showImageView, this, [ = ](int index) {
        m_backIndex = index;
        titlebar()->setVisible(false);  //隐藏顶部状态栏
        setTitlebarShadowEnabled(false);
        m_pCenterWidget->setCurrentIndex(VIEW_IMAGE);
    });
    //隐藏图片视图
    connect(dApp->signalM, &SignalManager::hideImageView, this, [ = ]() {
        //viewImageClose(); 2020/4/1 15:39 注释后不关闭相册主界面
        titlebar()->setVisible(true);   //显示状态栏
        setTitlebarShadowEnabled(true);
        m_pCenterWidget->setCurrentIndex(m_backIndex);
    });
    //幻灯片显示
    connect(dApp->signalM, &SignalManager::showSlidePanel, this, [ = ](int index) {
//        if (VIEW_IMAGE != index)
//        {
//            m_backIndex = index;
//        }
        m_backIndex_fromSlide = index;
        titlebar()->setVisible(false);
        setTitlebarShadowEnabled(false);
        m_pCenterWidget->setCurrentIndex(VIEW_SLIDE);
    });
    //隐藏幻灯片显示
    connect(dApp->signalM, &SignalManager::hideSlidePanel, this, [ = ]() {
        emit dApp->signalM->hideExtensionPanel();
        if (VIEW_IMAGE != m_backIndex_fromSlide) {
            titlebar()->setVisible(true);
            setTitlebarShadowEnabled(true);
        }

        m_pCenterWidget->setCurrentIndex(m_backIndex_fromSlide);
    });
    //导出图片
    connect(dApp->signalM, &SignalManager::exportImage, this, [ = ](QStringList paths) {
        Exporter::instance()->exportImage(paths);
    });
    connect(dApp->signalM, &SignalManager::showImageInfo, this, &MainWindow::onShowImageInfo);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &MainWindow::onUpdateAllpicsNumLabel);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &MainWindow::onUpdateAllpicsNumLabel);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::newProcessInstance, this, &MainWindow::onNewAPPOpen);
    connect(dApp, &Application::sigFinishLoad, this, &MainWindow::onLoadingFinished);
    //右下角滑动条
    connect(dApp->signalM, &SignalManager::sigMainwindowSliderValueChg, this, [ = ](int step) {
        m_pSliderPos = step;
        emit SignalManager::instance()->sliderValueChange(step);
        saveZoomRatio();
    });
    connect(dApp->signalM, &SignalManager::sigAlbDelToast, this, [ = ](QString str1) {
        QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg");
//        icon = utils::base::renderSVG(":/images/logo/resources/images/other/icon_toast_sucess_new.svg", QSize(20, 20));
        QString str2 = tr("Album “%1” removed");

        QWidget *pwidget = new QWidget();
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
            pwidget = m_commandLine->m_pwidget;
            break;
        default:
            pwidget = m_pAllPicView->m_pwidget;
            break;
        }
        DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, pwidget);
        pDFloatingMessage->setBlurBackgroundEnabled(true);
        pDFloatingMessage->setMessage(str2.arg(str1));
        pDFloatingMessage->setIcon(icon);
        DMessageManager::instance()->sendMessage(pwidget, pDFloatingMessage);
        //this->sendMessage(icon, str2.arg(str1));
    });

    connect(dApp->signalM, &SignalManager::sigAddToAlbToast, this, [ = ](QString album) {
        QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg");
//        icon = utils::base::renderSVG(":/images/logo/resources/images/other/icon_toast_sucess_new.svg", QSize(20, 20));

        QString str2 = tr("Successfully added to “%1”");

        QWidget *pwidget = new QWidget();
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
            pwidget = m_commandLine->m_pwidget;
            break;
        default:
            pwidget = m_pAllPicView->m_pwidget;
            break;
        }
        DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, pwidget);
        pDFloatingMessage->setBlurBackgroundEnabled(true);
        pDFloatingMessage->setMessage(str2.arg(album));
        pDFloatingMessage->setIcon(icon);
        DMessageManager::instance()->sendMessage(pwidget, pDFloatingMessage);
        //this->sendMessage(icon, str2.arg(album));
    });
    //底部，弹出导入成功提示框
    connect(dApp->signalM, &SignalManager::ImportSuccess, this, [ = ] {
        QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg");
//        icon = utils::base::renderSVG(":/images/logo/resources/images/other/icon_toast_sucess_new.svg", QSize(20, 20));

        QString str2 = tr("Import successful");

        QWidget *pwidget = new QWidget();
        switch (m_pCenterWidget->currentIndex())
        {
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
            pwidget = m_commandLine->m_pwidget;
            break;
        default:
            pwidget = m_pAllPicView->m_pwidget;
            break;
        }
        DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, pwidget);
        pDFloatingMessage->setBlurBackgroundEnabled(true);
        pDFloatingMessage->setMessage(str2);
        pDFloatingMessage->setIcon(icon);
        DMessageManager::instance()->sendMessage(pwidget, pDFloatingMessage);
        //this->sendMessage(icon, str2);
    });
    connect(dApp->signalM, &SignalManager::SearchEditClear, this, [ = ] {
        m_pSearchEdit->clear();
        m_SearchKey.clear();
    });
    //导入失败提示框
    connect(dApp->signalM, &SignalManager::ImportFailed, this, [ = ] {
        QIcon icon(":/images/logo/resources/images/other/warning_new.svg");
//        icon = utils::base::renderSVG(":/images/logo/resources/images/other/warning_new.svg", QSize(20, 20));

        QString str = tr("Import failed");

        QWidget *pwidget = new QWidget();
        switch (m_pCenterWidget->currentIndex())
        {
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
            pwidget = m_commandLine->m_pwidget;
            break;
        default:
            pwidget = m_pAllPicView->m_pwidget;
            break;
        }
        DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, pwidget);
        pDFloatingMessage->setBlurBackgroundEnabled(true);
        pDFloatingMessage->setMessage(str);
        pDFloatingMessage->setIcon(icon);
        DMessageManager::instance()->sendMessage(pwidget, pDFloatingMessage);
        m_waitdailog->close();
        //this->sendMessage(icon, str);
    });
    //部分导入失败提示框
    connect(dApp->signalM, &SignalManager::ImportSomeFailed, this, [ = ](int successful, int failed) {
        QIcon icon(":/images/logo/resources/images/other/warning_new.svg");
//        icon = utils::base::renderSVG(":/images/logo/resources/images/other/warning_new.svg", QSize(20, 20));
        QString str = tr("%1 photos imported, %2 photos failed");
        QString str1 = QString::number(successful, 10);
        QString str2 = QString::number(failed, 10);

        QWidget *pwidget = new QWidget();
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
            pwidget = m_commandLine->m_pwidget;
            break;
        default:
            pwidget = m_pAllPicView->m_pwidget;
            break;
        }
        DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, pwidget);
        pDFloatingMessage->setBlurBackgroundEnabled(true);
        pDFloatingMessage->setMessage(str.arg(str1).arg(str2));
        pDFloatingMessage->setIcon(icon);
        DMessageManager::instance()->sendMessage(pwidget, pDFloatingMessage);
        //this->sendMessage(icon, str);
    });
    //导出失败提示框
    connect(dApp->signalM, &SignalManager::ImgExportFailed, this, [ = ] {
        QIcon icon(":/images/logo/resources/images/other/warning_new.svg");
//        icon = utils::base::renderSVG(":/images/logo/resources/images/other/warning_new.svg", QSize(20, 20));

        QString str = tr("Export failed");

        QWidget *pwidget = new QWidget();
        switch (m_pCenterWidget->currentIndex())
        {
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
            pwidget = m_commandLine->m_pwidget;
            break;
        default:
            pwidget = m_pAllPicView->m_pwidget;
            break;
        }
        DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, pwidget);
        pDFloatingMessage->setBlurBackgroundEnabled(true);
        pDFloatingMessage->setMessage(str);
        pDFloatingMessage->setIcon(icon);
        DMessageManager::instance()->sendMessage(pwidget, pDFloatingMessage);
        //this->sendMessage(icon, str);
    });
    //导出成功提示框
    connect(dApp->signalM, &SignalManager::ImgExportSuccess, this, [ = ] {
        QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg");
//        icon = utils::base::renderSVG(":/images/logo/resources/images/other/icon_toast_sucess_new.svg", QSize(20, 20));

        QString str = tr("Export successful");

        QWidget *pwidget = new QWidget();
        switch (m_pCenterWidget->currentIndex())
        {
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
            pwidget = m_commandLine->m_pwidget;
            break;
        default:
            pwidget = m_pAllPicView->m_pwidget;
            break;
        }
        DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, pwidget);
        pDFloatingMessage->setBlurBackgroundEnabled(true);
        pDFloatingMessage->setMessage(str);
        pDFloatingMessage->setIcon(icon);
        DMessageManager::instance()->sendMessage(pwidget, pDFloatingMessage);
        //this->sendMessage(icon, str);
    });
    //相册导出失败提示框
    connect(dApp->signalM, &SignalManager::AlbExportFailed, this, [ = ] {
        QIcon icon(":/images/logo/resources/images/other/warning_new.svg");
//        icon = utils::base::renderSVG(":/images/logo/resources/images/other/warning_new.svg", QSize(20, 20));

        QString str = tr("Export failed");

        QWidget *pwidget = new QWidget();
        switch (m_pCenterWidget->currentIndex())
        {
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
            pwidget = m_commandLine->m_pwidget;
            break;
        default:
            pwidget = m_pAllPicView->m_pwidget;
            break;
        }
        DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, pwidget);
        pDFloatingMessage->setBlurBackgroundEnabled(true);
        pDFloatingMessage->setMessage(str);
        pDFloatingMessage->setIcon(icon);
        DMessageManager::instance()->sendMessage(pwidget, pDFloatingMessage);
        //this->sendMessage(icon, str);
    });
    //相册导出成功提示框
    connect(dApp->signalM, &SignalManager::AlbExportSuccess, this, [ = ] {
        QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg");
//        icon = utils::base::renderSVG(":/images/logo/resources/images/other/icon_toast_sucess_new.svg", QSize(20, 20));

        QString str = tr("Export successful");

        QWidget *pwidget = new QWidget();
        switch (m_pCenterWidget->currentIndex())
        {
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
            pwidget = m_commandLine->m_pwidget;
            break;
        default:
            pwidget = m_pAllPicView->m_pwidget;
            break;
        }
        DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, pwidget);
        pDFloatingMessage->setBlurBackgroundEnabled(true);
        pDFloatingMessage->setMessage(str);
        pDFloatingMessage->setIcon(icon);
        DMessageManager::instance()->sendMessage(pwidget, pDFloatingMessage);
//        DMessageManager::instance()->sendMessage(pwidget,QIcon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg"),tr("点击"));
        //this->sendMessage(icon, str);
    });
    connect(m_pAlbumview, &AlbumView::sigSearchEditIsDisplay, this, [ = ](bool bIsDisp) {
        if (m_pCenterWidget->currentIndex() == VIEW_ALBUM) {
            m_pSearchEdit->setVisible(bIsDisp);
        }
    });
    //设备导入成功
    /*
    connect(dApp->signalM, &SignalManager::DeviceImageLoadEnd, this, [ = ] {
        QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg");
    //        icon = utils::base::renderSVG(":/images/logo/resources/images/other/icon_toast_sucess_new.svg", QSize(20, 20));

        QString str = tr("device load successful");

        QWidget *pwidget = new QWidget();
        switch (m_pCenterWidget->currentIndex())
        {
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
            pwidget = m_commandLine->m_pwidget;
            break;
        default:
            pwidget = m_pAllPicView->m_pwidget;
            break;
        }
        DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, pwidget);
        pDFloatingMessage->setBlurBackgroundEnabled(true);
        pDFloatingMessage->setMessage(str);
        pDFloatingMessage->setIcon(icon);
        DMessageManager::instance()->sendMessage(pwidget, pDFloatingMessage);
    //        DMessageManager::instance()->sendMessage(pwidget,QIcon(":/images/logo/resources/images/other/icon_toast_sucess_new.svg"),tr("点击"));
        //this->sendMessage(icon, str);
    });
    */

}

//初始化快捷键
void MainWindow::initShortcut()
{
    QShortcut *esc = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    esc->setContext(Qt::WindowShortcut);
    connect(esc, &QShortcut::activated, this, [ = ] {
        if (window()->isFullScreen())
        {
            emit dApp->signalM->sigESCKeyActivated();
            emit dApp->signalM->sigESCKeyStopSlide();
        } else if (VIEW_IMAGE == m_pCenterWidget->currentIndex())
        {
            this->close();
        }
        emit dApp->signalM->hideExtensionPanel();
    });

    // Album View画面按DEL快捷键
    QShortcut *del = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    del->setContext(Qt::ApplicationShortcut);
    connect(del, &QShortcut::activated, this, [ = ] {
        emit dApp->signalM->sigShortcutKeyDelete();
    });

    // Album View画面按F2快捷键
    QShortcut *keyF2 = new QShortcut(QKeySequence(Qt::Key_F2), this);
    keyF2->setContext(Qt::ApplicationShortcut);
    connect(keyF2, &QShortcut::activated, this, [ = ] {
        emit dApp->signalM->sigShortcutKeyF2();
    });

    //Ctrl+Q退出
    QShortcut *CtrlQ = new QShortcut(QKeySequence(CTRLQ_SHORTCUT), this);
    CtrlQ->setContext(Qt::ApplicationShortcut);
    connect(CtrlQ, &QShortcut::activated, this, [ = ] {
        dApp->quit();
    });

    //Ctrl+Up 缩略图放大
    QShortcut *CtrlUp = new QShortcut(QKeySequence(CTRLUP_SHORTCUT), this);
    CtrlUp->setContext(Qt::ApplicationShortcut);
    connect(CtrlUp, &QShortcut::activated, this, [ = ] {
        thumbnailZoomIn();
    });

    QShortcut *ReCtrlUp = new QShortcut(QKeySequence(RECTRLUP_SHORTCUT), this);
    ReCtrlUp->setContext(Qt::ApplicationShortcut);
    connect(ReCtrlUp, &QShortcut::activated, this, [ = ] {
        thumbnailZoomIn();
    });

//    QShortcut *CtrlKeyUp = new QShortcut(QKeySequence(Qt::Key_Up), this);
//    CtrlKeyUp->setContext(Qt::ApplicationShortcut);
//    connect(CtrlKeyUp, &QShortcut::activated, this, [=] {
//        if (VIEW_IMAGE == m_pCenterWidget->currentIndex())
//        {
//            emit dApp->signalM->sigCtrlADDKeyActivated();
//        }
//        else
//        {
//            if (m_pSliderPos != m_pAllPicView->m_pStatusBar->m_pSlider->maximum())
//            {
//                m_pSliderPos = m_pSliderPos + 1;
//                if (m_pCenterWidget->currentIndex() ==VIEW_ALLPIC)
//                {
//                    m_pAllPicView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
//                }
//                else if(m_pCenterWidget->currentIndex() ==VIEW_TIMELINE)
//                {
//                    m_pTimeLineView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
//                }
//                else if (m_pCenterWidget->currentIndex() ==VIEW_ALBUM)
//                {
//                    m_pAlbumview->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
//                }

//                emit dApp->signalM->sigMainwindowSliderValueChg(m_pSliderPos);
//            }
//        }
//    });

    //Ctrl+Down 缩略图缩小
    QShortcut *CtrlDown = new QShortcut(QKeySequence(CTRLDOWN_SHORTCUT), this);
    CtrlDown->setContext(Qt::ApplicationShortcut);
    connect(CtrlDown, &QShortcut::activated, this, [ = ] {
        thumbnailZoomOut();
    });

    //Ctrl+Shift+/ 显示快捷键预览
    QShortcut *CtrlShiftSlash = new QShortcut(QKeySequence(CTRLSHIFTSLASH_SHORTCUT), this);
    CtrlShiftSlash->setContext(Qt::ApplicationShortcut);
    connect(CtrlShiftSlash, &QShortcut::activated, this, [this] {
        QRect rect = window()->geometry();
        QPoint pos(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
        QStringList shortcutString;
        QJsonObject json = createShorcutJson();

        QString param1 = "-j=" + QString(QJsonDocument(json).toJson());
        QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());
        shortcutString << param1 << param2;

        QProcess *shortcutViewProcess = new QProcess();
        shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

        connect(shortcutViewProcess, SIGNAL(finished(int)),
                shortcutViewProcess, SLOT(deleteLater()));
    });

//    QShortcut *CtrlKeyDown = new QShortcut(QKeySequence(Qt::Key_Down), this);
//    CtrlKeyDown->setContext(Qt::ApplicationShortcut);
//    connect(CtrlKeyDown, &QShortcut::activated, this, [=] {
//        if (VIEW_IMAGE == m_pCenterWidget->currentIndex())
//        {
//            emit dApp->signalM->sigCtrlSubtractKeyActivated();
//        }
//        else
//        {
//            if (m_pSliderPos != m_pAllPicView->m_pStatusBar->m_pSlider->minimum())
//            {
//                m_pSliderPos = m_pSliderPos - 1;
//                if (m_pCenterWidget->currentIndex() ==VIEW_ALLPIC)
//                {
//                    m_pAllPicView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
//                }
//                else if(m_pCenterWidget->currentIndex() ==VIEW_TIMELINE)
//                {
//                    m_pTimeLineView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
//                }
//                else if (m_pCenterWidget->currentIndex() ==VIEW_ALBUM)
//                {
//                    m_pAlbumview->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
//                }

//                dApp->signalM->sigMainwindowSliderValueChg(m_pSliderPos);
//            }
//        }
//    });
    //Ctrl+F/ 搜索
    QShortcut *CtrlF = new QShortcut(QKeySequence(CTRLF_SHORTCUT), this);
    CtrlF->setContext(Qt::ApplicationShortcut);
    connect(CtrlF, &QShortcut::activated, this, [this] {
        m_pSearchEdit->lineEdit()->setFocus();
    });
}

//初始化UI
void MainWindow::initUI()
{
//    resize(DEFAULT_WINDOWS_WIDTH, DEFAULT_WINDOWS_HEIGHT);
//      setMinimumSize(MIX_WINDOWS_WIDTH, MIX_WINDOWS_HEIGHT);
//    QRect rect = DApplication::desktop()->geometry();
//    setMinimumSize(rect.width() * 0.5, rect.height() * 0.5);

    initWaitDialog();
    setMinimumSize(880, 500);
    resize(1300, 848);
    loadWindowState();
}

//初始化等待窗口
void MainWindow::initWaitDialog()
{
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


    //    QSize size = m_waitlabel->size();

}

//初始化顶部状态栏
void MainWindow::initTitleBar()
{
    // TitleBar Img
    QLabel *pLabel = new QLabel();
    pLabel->setFixedSize(33, 32);

    QIcon icon = QIcon::fromTheme("deepin-album");

    pLabel->setPixmap(icon.pixmap(QSize(30, 30)));

    QHBoxLayout *pAllTitleLayout = new QHBoxLayout();
    m_ImgWidget = new QWidget();

    pAllTitleLayout->addSpacing(2);
    pAllTitleLayout->addWidget(pLabel);
    m_ImgWidget->setLayout(pAllTitleLayout);

    // TitleBar Button
    m_titleBtnWidget = new QWidget();
    btnGroup = new QButtonGroup();
    btnGroup->setExclusive(true);
    QHBoxLayout *pTitleBtnLayout = new QHBoxLayout();

    m_pAllPicBtn = new DPushButton();
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
    addAction(pImport);

    pImport->setText(tr("Import photos"));
    pImport->setShortcut(QKeySequence(CTRLI_SHORTCUT));
    m_pTitleBarMenu->addAction(pImport);
    m_pTitleBarMenu->addSeparator();

    titlebar()->addWidget(m_ImgWidget, Qt::AlignLeft);
    titlebar()->addWidget(m_titleBtnWidget, Qt::AlignLeft);
    titlebar()->addWidget(m_pSearchEdit, Qt::AlignHCenter);
    titlebar()->setMenu(m_pTitleBarMenu);
    titlebar()->setBlurBackground(true);

    if (0 < DBManager::instance()->getImgsCount()) {
        // dothing
    } else {
        m_pSearchEdit->setEnabled(false);
    }
}

//初始化中心界面
void MainWindow::initCentralWidget()
{
    QStringList pas;

    m_pAlbumview = new AlbumView();
    m_pTimeLineView = new TimeLineView();
    m_pSearchView = new SearchView();
    m_commandLine = CommandLine::instance();
    m_commandLine->setThreads(this);
    m_slidePanel = new SlideShowPanel();

    m_pCenterWidget->addWidget(m_pTimeLineView);
    m_pCenterWidget->addWidget(m_pAlbumview);
    m_pCenterWidget->addWidget(m_pSearchView);
    m_pCenterWidget->addWidget(m_commandLine);
    m_pCenterWidget->addWidget(m_slidePanel);

    m_commandLine->processOption(pas);
    if (pas.length() > 0) {
        titlebar()->setVisible(false);
        setTitlebarShadowEnabled(false);
        m_commandLine->viewImage(QFileInfo(pas.at(0)).absoluteFilePath(), pas);
        m_pCenterWidget->setCurrentIndex(VIEW_IMAGE);
        m_backIndex = VIEW_ALLPIC;
    } else {
        m_commandLine->viewImage("", {});
        m_pCenterWidget->setCurrentIndex(VIEW_ALLPIC);
    }

    //    setCentralWidget(m_pCenterWidget);
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

void MainWindow::onUpdateCentralWidget()
{
    emit dApp->signalM->hideExtensionPanel();

    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
}

//显示所有照片
void MainWindow::allPicBtnClicked()
{
    emit dApp->signalM->hideExtensionPanel();
    m_pSearchEdit->clear();
    m_SearchKey.clear();

    m_iCurrentView = VIEW_ALLPIC;
    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
    m_pAllPicView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
    m_pAllPicView->updateStackedWidget();
    m_pAllPicView->updatePicNum();

}

//显示时间线照片
void MainWindow::timeLineBtnClicked()
{
    emit dApp->signalM->hideExtensionPanel();
    m_pSearchEdit->clear();
    m_SearchKey.clear();
    m_iCurrentView = VIEW_TIMELINE;
    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
    m_pTimeLineView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
    m_pTimeLineView->updateStackedWidget();
    m_pTimeLineView->updatePicNum();

}

//显示相册
void MainWindow::albumBtnClicked()
{
    emit dApp->signalM->hideExtensionPanel();
    m_pSearchEdit->clear();
    m_SearchKey.clear();
    m_iCurrentView = VIEW_ALBUM;
    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
    m_pAlbumview->updateRightView();    //切换时手动更新界面相册显示界面

    m_pAlbumview->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
    m_pAlbumview->SearchReturnUpdate();
    m_pAlbumview->updatePicNum();

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
//    if (m_pCenterWidget->currentWidget() == m_pAlbumview)
//  {
//        m_pAlbumview->createNewAlbum(imagepaths);
//    }
//  else
//  {
    showCreateDialog(imagepaths);
//    }
}
#if 1
void MainWindow::onViewCreateAlbum(QString imgpath)
{
    AlbumCreateDialog *d = new AlbumCreateDialog(this);
    d->show();
    d->move(this->x() + (this->width() - d->width()) / 2, this->y() + (this->height() - d->height()) / 2);
    connect(d, &AlbumCreateDialog::albumAdded, this, [ = ] {

        emit dApp->signalM->hideExtensionPanel();

        DBManager::instance()->insertIntoAlbum(d->getCreateAlbumName(), imgpath.isEmpty() ? QStringList(" ") : QStringList(imgpath));
        emit dApp->signalM->sigCreateNewAlbumFrom(d->getCreateAlbumName());

        QIcon icon(":/images/logo/resources/images/other/icon_toast_sucess.svg");
//        icon = utils::base::renderSVG(":/images/logo/resources/images/other/icon_toast_sucess.svg", QSize(20, 20));

        QString str = "Create Album “%1” successfully";

//        QWidget *pwidget = new QWidget();
        QWidget *pwidget = nullptr;
        switch (m_pCenterWidget->currentIndex())
        {
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
            pwidget = m_commandLine->m_pwidget;
            break;
        default:
            pwidget = m_pAllPicView->m_pwidget;
            break;
        }
        DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, pwidget);
        pDFloatingMessage->setBlurBackgroundEnabled(true);
        pDFloatingMessage->setMessage(str.arg(d->getCreateAlbumName()));
        pDFloatingMessage->setIcon(icon);
        DMessageManager::instance()->sendMessage(pwidget, pDFloatingMessage);
        //this->sendMessage(icon, str.arg(d->getCreateAlbumName()));
    });
}
#endif
//创建相册弹出窗
void MainWindow::showCreateDialog(QStringList imgpaths)
{
    AlbumCreateDialog *d = new AlbumCreateDialog(this);
//    d->setWindowModality(Qt::WindowModal);
    d->show();
    d->move(this->x() + (this->width() - d->width()) / 2, this->y() + (this->height() - d->height()) / 2);

    connect(d, &AlbumCreateDialog::albumAdded, this, [ = ] {
        //emit dApp->signalM->hideExtensionPanel();

        DBManager::instance()->insertIntoAlbum(d->getCreateAlbumName(), imgpaths.isEmpty() ? QStringList(" ") : imgpaths);
        emit dApp->signalM->sigCreateNewAlbumFromDialog(d->getCreateAlbumName());

        m_pAlbumBtn->setChecked(true);

//        m_pAllPicBtn->setFlat(true);
//        m_pTimeLineBtn->setFlat(true);
//        m_pAlbumBtn->setFlat(false);

//        DPalette pal = DApplicationHelper::instance()->palette(m_pItemButton);
//        pal.setBrush(DPalette::Light, pal.color(DPalette::DarkLively));
//        pal.setBrush(DPalette::Dark, pal.color(DPalette::DarkLively));
//        pal.setBrush(DPalette::ButtonText, pal.color(DPalette::HighlightedText));
//        pal.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
//        m_pAlbumBtn->setPalette(pal);

//        DPalette pale = DApplicationHelper::instance()->palette(m_pAllPicBtn);
//        pale.setBrush(DPalette::Light, pale.color(DPalette::Base));
//        pale.setBrush(DPalette::Dark, pale.color(DPalette::Base));
//        pale.setBrush(DPalette::ButtonText, pale.color(DPalette::TextTitle));
//        pale.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
//        m_pAllPicBtn->setPalette(pale);
//        m_pTimeLineBtn->setPalette(pale);

//        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
//        if (themeType == DGuiApplicationHelper::LightType)
//        {
//            QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
//            effect->setOffset(0, 4);
//            effect->setColor(QColor(44, 167, 248, 102));
//            effect->setBlurRadius(6);
//            m_pAlbumBtn->setGraphicsEffect(effect);
//        }
//        else if (themeType == DGuiApplicationHelper::DarkType)
//        {
//            QGraphicsDropShadowEffect *effect = new QGraphicsDropShadowEffect();
//            effect->setOffset(0, 4);
//            effect->setColor(QColor(0, 42, 175, 102));
//            effect->setBlurRadius(6);
//            m_pAlbumBtn->setGraphicsEffect(effect);
//        }

//        QGraphicsDropShadowEffect *Noeffect = new QGraphicsDropShadowEffect();
//        Noeffect->setOffset(0, 0);
//        Noeffect->setColor(QColor(0, 0, 0, 0));
//        Noeffect->setBlurRadius(0);
//        m_pAllPicBtn->setGraphicsEffect(Noeffect);
//        m_pTimeLineBtn->setGraphicsEffect(Noeffect);

        //emit dApp->signalM->hideExtensionPanel();
        m_pSearchEdit->clear();
        m_SearchKey.clear();
        m_pAlbumview->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);

        m_backIndex = VIEW_ALBUM;
        emit dApp->signalM->hideImageView();    //该信号针对查看界面新建相册，正常退出
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
    if (0 == m_iCurrentView) {
        if (keywords.isEmpty()) {
            allPicBtnClicked();
            // donothing
        } else {
            emit dApp->signalM->sigSendKeywordsIntoALLPic(keywords, COMMON_STR_ALLPHOTOS);
            m_pAllPicView->m_pStackedWidget->setCurrentIndex(2);
            m_pAllPicView->restorePicNum();
        }
    } else if (1 == m_iCurrentView) {
        if (keywords.isEmpty()) {
            timeLineBtnClicked();
            // donothing
        } else {
            emit dApp->signalM->sigSendKeywordsIntoALLPic(keywords, COMMON_STR_TIMELINE);
            m_pTimeLineView->m_pStackedWidget->setCurrentIndex(2);
            m_pTimeLineView->restorePicNum();
        }
    } else if (2 == m_iCurrentView) {
        if (keywords.isEmpty()) {
            albumBtnClicked();
            // donothing
        } else {
            if (COMMON_STR_RECENT_IMPORTED == m_pAlbumview->m_pLeftListView->getItemCurrentType()) {
                emit dApp->signalM->sigSendKeywordsIntoALLPic(keywords, COMMON_STR_RECENT_IMPORTED);
            } else if (COMMON_STR_CUSTOM == m_pAlbumview->m_pLeftListView->getItemCurrentType()) {
                emit dApp->signalM->sigSendKeywordsIntoALLPic(keywords, m_pAlbumview->m_pLeftListView->getItemCurrentName());
            }

            m_pAlbumview->m_pRightStackWidget->setCurrentIndex(4);
            m_pAlbumview->restorePicNum();
        }
    }
}

//导入、删除图片时，更新图片总计数
void MainWindow::onUpdateAllpicsNumLabel()
{

    QString str = tr("%1 photo(s)");

    m_allPicNum = DBManager::instance()->getImgsCount();
//    m_pAllPicNumLabel->setText(str.arg(QString::number(m_allPicNum)));
}

//标题菜单导入照片槽函数
void MainWindow::onImprotBtnClicked()
{
    static QStringList sList;

    for (const QByteArray &i : QImageReader::supportedImageFormats())
        sList << "*." + QString::fromLatin1(i);


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

    DFileDialog dialog;
    dialog.setFileMode(DFileDialog::ExistingFiles);
//    dialog.setAllowMixedSelection(true);
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


    ImageEngineApi::instance()->ImportImagesFromFileList(file_list, m_pAlbumview->m_currentAlbum, this, true);
//    QStringList image_list;
//    foreach (QString path, file_list) {
//        QFileInfo file(path);
//        if (file.isDir()) {
//            image_list << utils::image::checkImage(path);
//        } else {
//            image_list << path;
//        }
//    }

//    if (image_list.size() < 1) {
//        emit dApp->signalM->ImportFailed();
//        return;
//    }
//    QFileInfo firstFileInfo(image_list.first());
//    dApp->setter->setValue(cfgGroupName, cfgLastOpenPath, firstFileInfo.path());

//    // 判断当前导入路径是否为外接设备
//    int isMountFlag = 0;
//    DGioVolumeManager *pvfsManager = new DGioVolumeManager;
//    QList<QExplicitlySharedDataPointer<DGioMount>> mounts = pvfsManager->getMounts();
//    for (auto mount : mounts) {
//        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
//        QString strPath = LocationFile->path();
//        if (0 == image_list.first().compare(strPath)) {
//            isMountFlag = 1;
//            break;
//        }
//    }

//    // 当前导入路径
//    if (isMountFlag) {
//        QString strHomePath = QDir::homePath();
//        //获取系统现在的时间
//        QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
//        QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
//        QDir dir;
//        if (!dir.exists(basePath)) {
//            dir.mkpath(basePath);
//        }

//        QStringList newImagePaths;
//        foreach (QString strPath, image_list) {
//            //取出文件名称
//            QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
//            QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
//            QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(), QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

//            newImagePaths << strNewPath;
//            //判断新路径下是否存在目标文件，若存在，下一次张
//            if (dir.exists(strNewPath)) {
//                continue;
//            }

//            // 外接设备图片拷贝到系统
//            if (QFile::copy(strPath, strNewPath)) {

//            }
//        }

//        image_list.clear();
//        image_list = newImagePaths;
//    }

//    DBImgInfoList dbInfos;

//    using namespace utils::image;

//    for (auto imagePath : image_list) {
//        if (! imageSupportRead(imagePath)) {
//            continue;
//        }

////        // Generate thumbnail and storage into cache dir
////        if (! utils::image::thumbnailExist(imagePath)) {
////            // Generate thumbnail failed, do not insert into DB
////            if (! utils::image::generateThumbnail(imagePath)) {
////                continue;
////            }
////        }

//        QFileInfo fi(imagePath);
//        using namespace utils::image;
//        using namespace utils::base;
//        auto mds = getAllMetaData(imagePath);
//        QString value = mds.value("DateTimeOriginal");
////        qDebug() << value;
//        DBImgInfo dbi;
//        dbi.fileName = fi.fileName();
//        dbi.filePath = imagePath;
//        dbi.dirHash = utils::base::hash(QString());
//        if ("" != value) {
//            dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm:ss");
//        } else if (fi.birthTime().isValid()) {
//            dbi.time = fi.birthTime();
//        } else if (fi.metadataChangeTime().isValid()) {
//            dbi.time = fi.metadataChangeTime();
//        } else {
//            dbi.time = QDateTime::currentDateTime();
//        }
//        dbi.changeTime = QDateTime::currentDateTime();

//        dbInfos << dbi;
//    }

//    if (! dbInfos.isEmpty()) {
////        if(VIEW_ALBUM == m_iCurrentView)
////        {
////            if (COMMON_STR_RECENT_IMPORTED != m_pAlbumview->m_currentAlbum
////                && COMMON_STR_TRASH != m_pAlbumview->m_currentAlbum
////                && COMMON_STR_FAVORITES != m_pAlbumview->m_currentAlbum
////                && ALBUM_PATHTYPE_BY_PHONE !=m_pAlbumview->m_pRightThumbnailList->m_imageType)
////            {
////                DBManager::instance()->insertIntoAlbumNoSignal(m_pAlbumview->m_currentAlbum, paths);
////            }
////        }


//        if (ALBUM_PATHTYPE_BY_PHONE == m_pAlbumview->m_pLeftListView->getItemCurrentType()) {
//            m_pAlbumview->m_currentAlbum = ALBUM_PATHTYPE_BY_PHONE;
//        }

//        dApp->m_imageloader->ImportImageLoader(dbInfos, m_pAlbumview->m_currentAlbum);
//    } else {
//        emit dApp->signalM->ImportFailed();
//    }

//    if (m_pCenterWidget->currentIndex() == VIEW_ALBUM
//            && ALBUM_PATHTYPE_BY_PHONE == m_pAlbumview->m_pLeftListView->getItemCurrentType()) {
//        m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->setCurrentRow(0);
//    }
}

bool MainWindow::imageImported(bool success)
{
//    if (success) {
//        if (ALBUM_PATHTYPE_BY_PHONE == m_pAlbumview->m_pLeftListView->getItemCurrentType()) {
//            m_pAlbumview->m_currentAlbum = ALBUM_PATHTYPE_BY_PHONE;
//        }
//    }
//    if (m_pCenterWidget->currentIndex() == VIEW_ALBUM
//            && ALBUM_PATHTYPE_BY_PHONE == m_pAlbumview->m_pLeftListView->getItemCurrentType()) {
//        m_pAlbumview->m_pLeftListView->m_pPhotoLibListView->setCurrentRow(0);
//    }
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
        dialog->setModal(true);
        m_propertyDialogs.insert(path, dialog);
        dialog->show();
//        dialog->move((width() - dialog->width()) / 2 +
//                     mapToGlobal(QPoint(0, 0)).x(),
//                     (window()->height() - dialog->height()) / 2 +
//                     window()->y() - 120);
//        dialog->move((width() - dialog->width() + mapToGlobal(QPoint(0, 0)).x()), (window()->height() - dialog->height()) - 650);
        dialog->move((this->width() - dialog->width() - 50 + mapToGlobal(QPoint(0, 0)).x()), 100 + mapToGlobal(QPoint(0, 0)).y());
        dialog->setWindowState(Qt::WindowActive);
        connect(dialog, &ImgInfoDialog::closed, this, [ = ] {
            dialog->deleteLater();
            m_propertyDialogs.remove(path);
        });
    } else {
        dialog = new ImgInfoDialog(path);
        dialog->setModal(true);
        m_propertyDialogs.insert(path, dialog);
        dialog->show();
//        dialog->move((width() - dialog->width()) / 2 +
//                     mapToGlobal(QPoint(0, 0)).x(),
//                     (window()->height() - dialog->height()) / 2 +
//                     window()->y() - 120);
        dialog->move((this->width() - dialog->width() - 50 + mapToGlobal(QPoint(0, 0)).x()), 100 + mapToGlobal(QPoint(0, 0)).y());
        dialog->setWindowState(Qt::WindowActive);
        connect(dialog, &ImgInfoDialog::closed, this, [ = ] {
            dialog->deleteLater();
            m_propertyDialogs.remove(path);
        });
    }

}

void MainWindow::viewImageClose()
{
    if (bfirstandviewimage) {
        exit(0);
    }
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

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveWindowState();
    if (VIEW_IMAGE == m_pCenterWidget->currentIndex()) {
//        if (bfirstandviewimage) {
//            event->accept();
//            return;
//        }
        emit dApp->signalM->hideImageView();
        event->ignore();
    } else {
        event->accept();
    }
}

void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
//    qDebug() << "m_ImgWidget width:          " << m_ImgWidget->width();
//    qDebug() << "m_titleBtnWidget width:     " << m_titleBtnWidget->width();
//    qDebug() << "m_pSearchEdit width:        " << m_pSearchEdit->width();
//    qDebug() << "titlebar()->width():        " << titlebar()->width();
//    qDebug() << "blank width:                " << titlebar()->width() - m_pSearchEdit->width() - m_titleBtnWidget->width() - m_ImgWidget->width();
//    int m_SearchEditWidth = titlebar()->width() - TITLEBAR_BLANK_WIDTH - TITLEBAR_BTNWIDGET_WIDTH - TITLEBAR_ICON_WIDTH - 120;
    int m_SearchEditWidth = titlebar()->width() - m_ImgWidget->width() -  m_titleBtnWidget->width() - TITLEBAR_BLANK_WIDTH;
//    qDebug() << "m_SearchEditWidth:            " << m_SearchEditWidth;
    if (m_SearchEditWidth <= 350) {
        m_pSearchEdit->setFixedSize(m_SearchEditWidth - 20, 36);
    } else {
        m_SearchEditWidth = 350;
        m_pSearchEdit->setFixedSize(m_SearchEditWidth, 36);
    }
}

void MainWindow::saveWindowState()
{
    QSettings settings(objectName());
    settings.setValue("album-geometry", saveGeometry());
    settings.setValue("album-isMaximized", isMaximized());
//    settings.setValue("album-pos", pos());
//    settings.endGroup();
}

//加载主界面状态（上次退出时）
void MainWindow::loadWindowState()
{
    QSettings settings(objectName());
    const QByteArray geometry = settings.value("album-geometry").toByteArray();
    const bool isMaximized = settings.value("album-isMaximized").toBool();
//    const QByteArray pos = settings.value("album-pos").toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
        if (isMaximized) {
            resize(1300, 848);
            Dtk::Widget::moveToCenter(this);
//            QDesktopWidget *desktop = QApplication::desktop(); // =qApp->desktop();也可以
//            move((desktop->width() - this->width()) / 2, (desktop->height() - this->height()) / 2);
        }
    }
//    settings.endGroup();
}

//保存缩放比例
void MainWindow::saveZoomRatio()
{
    QSettings settings(objectName());
    settings.setValue("album-zoomratio", m_pSliderPos);
}

//加载缩放比例（上次退出时）
void MainWindow::loadZoomRatio()
{
    QSettings settings(objectName());
    const int sliderpos = settings.value("album-zoomratio").toInt();
    m_pSliderPos = sliderpos;
//    if (m_pCenterWidget->currentIndex() == VIEW_ALLPIC) {
    m_pAllPicView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
//} else if (m_pCenterWidget->currentIndex() == VIEW_TIMELINE)
//{
//    m_pTimeLineView->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
//} else if (m_pCenterWidget->currentIndex() == VIEW_ALBUM)
//{
//    m_pAlbumview->m_pStatusBar->m_pSlider->setValue(m_pSliderPos);
//}
    dApp->signalM->sigMainwindowSliderValueChg(m_pSliderPos);
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
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, UNFAVORITE_CONTEXT_MENU, CTRLSHIFTK_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, FAVORITE_CONTEXT_MENU, CTRLK_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, ROTATECLOCKWISE_CONTEXT_MENU, CTRLR_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, ROTATECOUNTERCLOCKWISE_CONTEXT_MENU, CTRLSHIFTR_SHORTCUT);
//    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, SETASWALLPAPER_CONTEXT_MENU, CTRLF8_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, SETASWALLPAPER_CONTEXT_MENU, CTRLF9_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, DISPLAYINFILEMANAGER_CONTEXT_MENU, CTRLD_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, ImageInfo_CONTEXT_MENU, ALTRETURN_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, COMMON_STR_CREATEALBUM, CTRLSHIFTN_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, COMMON_STR_RENAMEALBUM, F2_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, SHOW_SHORTCUT_PREVIEW, CTRLSHIFTSLASH_SHORTCUT);
    ConfigSetter::instance()->setValue(SHORTCUTVIEW_GROUP, EXPORT_CONTEXT_MENU, CTRLE_SHORTCUT);
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
//    QJsonObject shortcut1;
//    shortcut1.insert("name", tr("窗口大小切换"));
//    shortcut1.insert("value", "Ctrl+Alt+F");
//    QJsonObject shortcut2;
//    shortcut2.insert("name", tr("全屏"));
//    shortcut2.insert("value", "F11");
//    QJsonObject shortcut3;
//    shortcut3.insert("name", tr("退出全屏/退出幻灯片放映"));
//    shortcut3.insert("value", "Esc");
//    QJsonObject shortcut4;
//    shortcut4.insert("name", tr("关闭应用"));
//    shortcut4.insert("value", "Alt+F4");
//    QJsonObject shortcut5;
//    shortcut5.insert("name", tr("帮助"));
//    shortcut5.insert("value", "F1");
//    QJsonObject shortcut6;
//    shortcut6.insert("name", tr("显示快捷键预览"));
//    shortcut6.insert("value", "Ctrl+Shift+/");
//    QJsonObject shortcut7;
//    shortcut7.insert("name", tr("在文件管理器中显示"));
//    shortcut7.insert("value", "Ctrl+D");
//    QJsonObject shortcut8;
//    shortcut8.insert("name", tr("幻灯片放映"));
//    shortcut8.insert("value", "F5");
//    QJsonObject shortcut9;
//    shortcut9.insert("name", tr("查看图片"));
//    shortcut9.insert("value", "Enter");
//    QJsonObject shortcut10;
//    shortcut10.insert("name", tr("导出图片"));
//    shortcut10.insert("value", "Ctrl+E");
//    QJsonObject shortcut11;
//    shortcut11.insert("name", tr("导入图片"));
//    shortcut11.insert("value", "Ctrl+O");
//    QJsonObject shortcut12;
//    shortcut12.insert("name", tr("全选图片"));
//    shortcut12.insert("value", "Ctrl+A");
//    QJsonObject shortcut13;
//    shortcut13.insert("name", tr("复制"));
//    shortcut13.insert("value", "Ctrl+C");
//    QJsonObject shortcut14;
//    shortcut14.insert("name", tr("删除照片/删除相册"));
//    shortcut14.insert("value", "Delete");
//    QJsonObject shortcut15;
//    shortcut15.insert("name", tr("图片信息"));
//    shortcut15.insert("value", "Alt+Enter");
//    QJsonObject shortcut16;
//    shortcut16.insert("name", tr("设为壁纸"));
//    shortcut16.insert("value", "Ctrl+F9");
//    QJsonObject shortcut17;
//    shortcut17.insert("name", tr("顺时针旋转"));
//    shortcut17.insert("value", "Ctrl+R");
//    QJsonObject shortcut18;
//    shortcut18.insert("name", tr("逆时针旋转"));
//    shortcut18.insert("value", "Ctrl+Shift+R");
//    QJsonObject shortcut19;
//    shortcut19.insert("name", tr("放大缩小图片"));
//    shortcut19.insert("value", "ctrl+鼠标滚轮缩放图片缩略图");
//    QJsonObject shortcut20;
//    shortcut20.insert("name", tr("放大图片"));
//    shortcut20.insert("value", "Ctrl+“+”");
//    QJsonObject shortcut21;
//    shortcut21.insert("name", tr("缩小图片"));
//    shortcut21.insert("value", "Ctrl+“-”");
//    QJsonObject shortcut22;
//    shortcut22.insert("name", tr("上一张"));
//    shortcut22.insert("value", "键盘<-");
//    QJsonObject shortcut23;
//    shortcut23.insert("name", tr("下一张"));
//    shortcut23.insert("value", "键盘->");
//    QJsonObject shortcut24;
//    shortcut24.insert("name", tr("收藏"));
//    shortcut24.insert("value", "Ctrl+K");
//    QJsonObject shortcut25;
//    shortcut25.insert("name", tr("取消收藏"));
//    shortcut25.insert("value", "Ctrl+Shift+K");
//    QJsonObject shortcut26;
//    shortcut26.insert("name", tr("新建相册"));
//    shortcut26.insert("value", "Ctrl+Shift+N");
//    QJsonObject shortcut27;
//    shortcut27.insert("name", tr("重命名相册"));
//    shortcut27.insert("value", "F2");

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
    shortcut7.insert("value", "Ctrl+D");
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
    shortcut15.insert("value", "Alt+Enter");
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
    shortcut24.insert("value", "Ctrl+K");
    QJsonObject shortcut25;
    shortcut25.insert("name", tr("Unfavorite"));
    shortcut25.insert("value", "Ctrl+Shift+K");
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
    if (QApplication::keyboardModifiers() == Qt::ControlModifier) {
        if (event->delta() > 0) {
            thumbnailZoomIn();
        } else {
            thumbnailZoomOut();
        }
        event->accept();
    }
}
