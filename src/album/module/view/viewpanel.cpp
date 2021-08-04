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
#include "viewpanel.h"
#include "application.h"
#include "navigationwidget.h"
#include "controller/signalmanager.h"
#include "controller/configsetter.h"
#include "contents/ttbcontent.h"
#include "scen/imageview.h"
#include "utils/imageutils.h"
#include "utils/baseutils.h"
#include "widgets/imagebutton.h"
#include "widgets/printhelper.h"
#include "widgets/dialogs/imgdeletedialog.h"
#include "imageengine/imageengineapi.h"
#include "ac-desktop-define.h"

#include <QApplication>
#include <QDebug>
#include <QFileInfo>
#include <QProxyStyle>
#include <QFileSystemWatcher>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QPixmapCache>
#include <QProcess>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QtConcurrent>
#include <QFileDialog>
#include <QPainter>
#include <QMovie>
#include <QDesktopWidget>
#include <QPropertyAnimation>

#include <DRecentManager>
#include <DWidgetUtil>

using namespace Dtk::Core;
using namespace Dtk::Widget;

namespace {

const int DELAY_HIDE_CURSOR_INTERVAL = 3000;
//const QSize ICON_SIZE = QSize(48, 40);
const int LOAD_ALL = 51;           //一次加载数量
const int LOAD_LEFT_RIGHT = 25;     //动态加载数量

}  // namespace

ViewPanel::ViewPanel(QWidget *parent)
    : ModulePanel(parent)
    , m_hideCursorTid(0)
    , m_iSlideShowTimerId(0)
    , m_isInfoShowed(false)
    , m_viewB(nullptr)
    , m_stack(nullptr)
    , m_deletetimer(nullptr)
    , m_bFirstFullScreen(false)
{
    onThemeChanged(dApp->viewerTheme->getCurrentTheme());
    initShortcut();
    initStack();
    initFloatingComponent();

    initConnect();

    initPopupMenu();

    setAcceptDrops(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    installEventFilter(this);
    AC_SET_ACCESSIBLE_NAME(this, VIEW_PANEL_WIDGET);
    AC_SET_OBJECT_NAME(this, VIEW_PANEL_WIDGET);

    AC_SET_ACCESSIBLE_NAME(m_stack, VIEW_PANEL_STACK);
    AC_SET_OBJECT_NAME(m_stack, VIEW_PANEL_STACK);
}

ViewPanel::~ViewPanel()
{

}

QString ViewPanel::moduleName()
{
    return "ViewPanel";
}

void ViewPanel::loadFilesFromLocal(QStringList files)
{
    imageLocalLoaded(files);
}

void ViewPanel::loadFilesFromLocal(DBImgInfoList files)
{
    ImageEngineApi::instance()->loadImagesFromLocal(files, this);
}

bool ViewPanel::imageLocalLoaded(QStringList &filelist)
{
    onViewImage(filelist);
    return true;
}

void ViewPanel::initConnect()
{
    connect(dApp->signalM, &SignalManager::deleteByMenu, this, &ViewPanel::onDeleteByMenu);
    connect(dApp->signalM, &SignalManager::gotoPanel, this, &ViewPanel::onGotoPanel);
    connect(dApp->signalM, &SignalManager::showExtensionPanel, this, &ViewPanel::onShowExtensionPanel);
    connect(dApp->signalM, &SignalManager::hideExtensionPanel, this, &ViewPanel::onHideExtensionPanel);
    connect(dApp->signalM, &SignalManager::hideImageView, this, &ViewPanel::onHideImageView);
    qRegisterMetaType<SignalManager::ViewInfo>("SignalManager::ViewInfo");
    connect(dApp->signalM, &SignalManager::viewImageNoNeedReload, this, &ViewPanel::onViewImageNoNeedReload);
    connect(dApp->signalM, &SignalManager::viewImage, this, &ViewPanel::onSigViewImage);
    connect(m_viewB, &ImageView::mouseHoverMoved, this, &ViewPanel::onMouseHoverMoved);
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList &");
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &ViewPanel::onImagesInserted);
    connect(dApp->signalM, &SignalManager::sigESCKeyActivated, this, &ViewPanel::onESCKeyActivated);
}

void ViewPanel::mousePressEvent(QMouseEvent *e)
{
    emit dApp->signalM->hideExtensionPanel();
    if (e->button() == Qt::BackButton) {
        if (window()->isFullScreen()) {
            showNormal();
        } else {
            backToLastPanel();
        }
    }

    if (0 != m_iSlideShowTimerId) {
        killTimer(m_iSlideShowTimerId);
        m_iSlideShowTimerId = 0;
    }

    ModulePanel::mousePressEvent(e);
}

void ViewPanel::onThemeChanged(ViewerThemeManager::AppTheme theme)
{
    Q_UNUSED(theme);
}

void ViewPanel::feedBackCurrentIndex(int index, const QString &path)
{
    openImage(path, false);
}

void ViewPanel::showNormal()
{
    if (m_isMaximized) {
        window()->showNormal();
        window()->showMaximized();
    } else {
        window()->showNormal();
    }

    emit dApp->signalM->showTopToolbar();
}


void ViewPanel::showFullScreen()
{
    m_isMaximized = window()->isMaximized();
    //加入显示动画效果，以透明度0-1显示，动态加载，视觉效果掩盖左上角展开
#ifndef tablet_PC
    QPropertyAnimation *animation = new QPropertyAnimation(window(), "windowOpacity");
    animation->setDuration(50);
    animation->setEasingCurve(QEasingCurve::Linear);
    animation->setStartValue(0);
    animation->setEndValue(1);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
#endif
    window()->showFullScreen();
    m_hideCursorTid = startTimer(DELAY_HIDE_CURSOR_INTERVAL);
    emit dApp->signalM->sigShowFullScreen();
}

//QWidget *ViewPanel::toolbarBottomContent()
//{
//    return nullptr;
//}

//QWidget *ViewPanel::toolbarTopLeftContent()
//{
//    return nullptr;
//}

void ViewPanel::bottomTopLeftContent()
{
    emit dApp->signalM->sigPauseOrStart(true);
    m_ttbc = new TTBContent(m_vinfo.inDatabase, this);
    m_ttbc->setObjectName("TTBContent");

    connect(m_ttbc, &TTBContent::feedBackCurrentIndex, this, &ViewPanel::feedBackCurrentIndex);
    //测试使用
    connect(this, &ViewPanel::ttbcDeleteImage, m_ttbc, &TTBContent::deleteImage);
    connect(this, &ViewPanel::viewImageFrom, m_ttbc, [ = ](const QString & dir) {
        m_ttbc->setCurrentDir(dir);
    });
    connect(this, &ViewPanel::imageChanged, m_ttbc, &TTBContent::setImage);
    connect(m_ttbc, &TTBContent::rotateClockwise, this, &ViewPanel::onRotateClockwise);
    connect(m_ttbc, &TTBContent::rotateCounterClockwise, this, &ViewPanel::onRotateCounterClockwise);
    connect(m_ttbc, &TTBContent::removed, this, &ViewPanel::onRemoved);
    connect(m_ttbc, &TTBContent::resetTransform, this, &ViewPanel::onResetTransform);
    connect(m_viewB, &ImageView::disCheckAdaptImageBtn, m_ttbc, &TTBContent::disCheckAdaptImageBtn);
    connect(m_viewB, &ImageView::disCheckAdaptScreenBtn, m_ttbc, &TTBContent::disCheckAdaptScreenBtn);
    connect(m_viewB, &ImageView::checkAdaptImageBtn, m_ttbc, &TTBContent::checkAdaptImageBtn);
    connect(m_viewB, &ImageView::checkAdaptScreenBtn, m_ttbc, &TTBContent::checkAdaptScreenBtn);
    connect(dApp->signalM, &SignalManager::insertedIntoAlbum, m_ttbc, &TTBContent::updateCollectButton);
    connect(dApp->signalM, &SignalManager::removedFromAlbum, m_ttbc, &TTBContent::updateCollectButton);

    connect(m_viewB, &ImageView::previousRequested, m_ttbc, &TTBContent::onPreButton);
    connect(m_viewB, &ImageView::nextRequested, m_ttbc, &TTBContent::onNextButton);
#ifdef tablet_PC
    //退出时，重置ttb显隐状态
    connect(m_ttbc, &TTBContent::resetShoworHide, this, [ = ] {
        m_showorhide = true;
    });
#endif

    m_ttbc->setAllFileInfo(m_vinfo);
    m_ttbc->m_imageType = m_vinfo.viewType;
}
//QWidget *ViewPanel::toolbarTopMiddleContent()
//{
//    QWidget *w = new QWidget();
//    return w;
//}

//ImageView *ViewPanel::getImageView()
//{
//    return m_viewB;
//}

void ViewPanel::onDeleteByMenu()
{
    if (m_deletetimer->isActive()) {
        return;
    }
    m_deletetimer->start();
    if (m_ttbc) {
        m_ttbc->deleteImage();
    }
}

void ViewPanel::onGotoPanel(ModulePanel *p)
{
    if (p != this) {
        emit dApp->signalM->showTopToolbar();
    } else {
        emit dApp->signalM->showTopToolbar();
        emit dApp->signalM->showBottomToolbar();
    }
}

void ViewPanel::onShowExtensionPanel()
{
    m_isInfoShowed = true;
}

void ViewPanel::onHideExtensionPanel()
{
    m_isInfoShowed = false;
}

void ViewPanel::onHideImageView()
{
    if (m_vinfo.fullScreen) {   //全屏退出
        toggleFullScreen();
    }
    m_viewB->clear();
}

bool ViewPanel::onSigViewImage(const SignalManager::ViewInfo &info)
{
    SignalManager::ViewInfo vinfo = info;
    m_vinfo = vinfo;
    m_filepathlist = m_vinfo.paths;
    if (m_ttbc == nullptr) {
        bottomTopLeftContent();
        emit dApp->signalM->updateBottomToolbarContent(m_ttbc, true);
    }
    m_bFirstFullScreen = m_vinfo.fullScreen;
    QList<QByteArray> fList = QMovie::supportedFormats(); //"gif","mng","webp"
    QString strfixL = QFileInfo(vinfo.path).suffix().toLower();
    if (fList.contains(strfixL.toUtf8().data()) || vinfo.fullScreen) {
        m_currentpath = m_vinfo.path;
        if (m_vinfo.paths.size() < 1) {
            m_vinfo.paths << m_vinfo.path;
        }
        loadFilesFromLocal(m_vinfo.paths);
        emit dApp->signalM->showImageView(m_vinfo.viewMainWindowID);
        if (nullptr == m_vinfo.lastPanel) {
            return false;
        } else if (m_vinfo.lastPanel->moduleName() == "AlbumPanel" ||
                   m_vinfo.lastPanel->moduleName() == "ViewPanel") {
            m_currentImageLastDir = m_vinfo.album;
            emit viewImageFrom(m_vinfo.album);
        } else if (m_vinfo.lastPanel->moduleName() == "TimelinePanel") {
            m_currentImageLastDir = "Timeline";
            emit viewImageFrom("Timeline");
        }
        return false;
        //TODO: there will be some others panel
    } else {
        m_currentpath = m_vinfo.path;
        if (m_vinfo.paths.size() < 1) {
            m_vinfo.paths << m_vinfo.path;
        }
        loadFilesFromLocal(m_vinfo.paths);
        if (nullptr == m_vinfo.lastPanel) {
            return false;
        } else if (m_vinfo.lastPanel->moduleName() == "AlbumPanel" ||
                   m_vinfo.lastPanel->moduleName() == "ViewPanel") {
            m_currentImageLastDir = m_vinfo.album;
            emit viewImageFrom(m_vinfo.album);
        } else if (m_vinfo.lastPanel->moduleName() == "TimelinePanel") {
            m_currentImageLastDir = "Timeline";
            emit viewImageFrom("Timeline");
        }
        return false;
    }

    //TODO: there will be some others panel
}

void ViewPanel::onMouseHoverMoved()
{
    emit mouseMoved();
}

void ViewPanel::onESCKeyActivated()
{
#if 1
    if (isVisible()) {
        if (0 != m_iSlideShowTimerId) {
            killTimer(m_iSlideShowTimerId);
            m_iSlideShowTimerId = 0;
        }
        if (m_bFirstFullScreen)
            emit dApp->signalM->hideImageView();
        else {
            toggleFullScreen();
        }
    }
    m_vinfo.fullScreen = false;
    emit dApp->signalM->showBottomToolbar();
#ifdef tablet_PC
    m_showorhide = true;//重置显隐状态
#endif
#endif
}

void ViewPanel::onImagesInserted()
{
    if (!m_ttbc) {
        return;
    }
    int size = m_ttbc->itemLoadedSize();
//    bottomTopLeftContent();
    qDebug() << "---" << __FUNCTION__ << "---11111111";
//    emit dApp->signalM->updateBottomToolbarContent(m_ttbc, (size  > 1));
}

void ViewPanel::onViewImageNoNeedReload(int &fileindex)
{
    //todo
}

void ViewPanel::onRotateClockwise()
{
    rotateImage(true);
}

void ViewPanel::onRotateCounterClockwise()
{
    rotateImage(false);
}

void ViewPanel::onRemoved()
{
    if (COMMON_STR_TRASH == m_vinfo.viewType) {
        ImgDeleteDialog *dialog = new ImgDeleteDialog(this, 1);
        dialog->setObjectName("deteledialog");
        if (dialog->exec() > 0) {
            DBManager::instance()->removeTrashImgInfos(QStringList(m_currentpath));
            removeCurrentImage();
            DDesktopServices::trash(m_currentpath);
        }
    } else {
        DBImgInfoList infos;
        DBImgInfo info;
        info = DBManager::instance()->getInfoByPath(m_currentpath);
        info.importTime = QDateTime::currentDateTime();
        infos << info;
        DBManager::instance()->insertTrashImgInfos(infos);
        DBManager::instance()->removeImgInfos(QStringList(m_currentpath));
        removeCurrentImage();
    }
}

void ViewPanel::onResetTransform(bool fitWindow)
{
    if (fitWindow) {
        m_viewB->fitWindow();
    } else {
        m_viewB->fitImage();
    }
}

void ViewPanel::onDoubleClicked()
{
    if (m_bFirstFullScreen)
        emit dApp->signalM->hideImageView();
    else {
        toggleFullScreen();
    }
}

void ViewPanel::onViewBClicked()
{
    dApp->signalM->hideExtensionPanel();
#ifdef tablet_PC
    if (!window()->isFullScreen()) {
        toggleFullScreen();
    } else {
        //BUG#82053，隔壁部门要求添加DBUS调用，以实现单指点击循环显示和隐藏工具栏
        auto &statusBarDbus = ComDeepinDueStatusbarInterface::instance();
        statusBarDbus.setVisible(!statusBarDbus.visible());
        //BUG#82053 end

        emit dApp->signalM->sigMouseMove(m_showorhide);
        m_showorhide = m_showorhide ? false : true;
    }
#endif
}

void ViewPanel::onViewBImageChanged(const QString &path)
{
    emit imageChanged(path);
    // Pixmap is cache in thread, make sure the size would correct after
    // cache is finish
    //m_viewB->autoFit(); 放出来会导致切换图片时偶发引起1:1图标闪烁
}

void ViewPanel::onFIleDelete()
{
    m_viewB->setImage(m_currentpath);    //设置当前显示图片
    if (!QFileInfo(m_currentpath).exists()) {
        ImageDataSt data;
        if (ImageEngineApi::instance()->getImageData(m_currentpath, data))
            m_emptyWidget->setThumbnailImage(/*dApp->m_imagemap.value(path)*/data.imgpixmap);
        m_stack->setCurrentIndex(1);
    }
}

bool ViewPanel::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj)
    if (e->type() == QEvent::HideToParent) {
        m_viewB->clear();
    }
    if (e->type() == QEvent::Resize && this->isVisible()) {
        emit sigResize();
    }

    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(e);

        if (keyEvent->key() == Qt::Key_Delete) {
            emit SignalManager::instance()->deleteByMenu();
        }
    }

    return false;
}

void ViewPanel::resizeEvent(QResizeEvent *e)
{
    ModulePanel::resizeEvent(e);
    // There will be several times the size change during switch to full process
    // So correct it every times
    if (window()->isFullScreen()) {
        Q_EMIT dApp->signalM->hideExtensionPanel(true);
        Q_EMIT dApp->signalM->hideTopToolbar(true);
    }
    if (!window()->isFullScreen()) {
        m_isMaximized = window()->isMaximized();
    }

    if (m_viewB->isFitImage()) {
        m_viewB->fitImage();
    } else if (m_viewB->isFitWindow()) {
        m_viewB->fitWindow();
    }
}

void ViewPanel::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_hideCursorTid &&
            !m_menu->isVisible() && !m_printDialogVisible && qApp->modalWindow() == nullptr) {
        m_viewB->viewport()->setCursor(Qt::BlankCursor);
    }

    if (e->timerId() == m_iSlideShowTimerId) {
        m_ttbc->onNextButton();
    }

    ModulePanel::timerEvent(e);
}

void ViewPanel::wheelEvent(QWheelEvent *e)
{
    if (m_viewB && !m_viewB->path().isEmpty())
        qApp->sendEvent(m_viewB->viewport(), e);
}

void ViewPanel::onViewImage(const QStringList &vinfo)
{
    using namespace utils::base;
    this->setCursor(Qt::ArrowCursor);
    if (m_vinfo.fullScreen) {
        showFullScreen();
    }
    if (m_vinfo.slideShow) {
        m_iSlideShowTimerId = startTimer(3000);
    } else {
        if (0 != m_iSlideShowTimerId) {
            killTimer(m_iSlideShowTimerId);
            m_iSlideShowTimerId = 0;
        }
    }
    emit dApp->signalM->gotoPanel(this);


    if (vinfo.size() == 1) {
        m_imageDirIterator.reset(new QDirIterator(QFileInfo(vinfo.first()).absolutePath(),
                                                  utils::image::supportedImageFormats(), QDir::Files | QDir::Readable));
    } else {
        m_imageDirIterator.reset();
    }

    bottomTopLeftContent();
    emit dApp->signalM->updateBottomToolbarContent(m_ttbc, (vinfo.size() >= 1));
}


void ViewPanel::toggleFullScreen()
{
    if (window()->isFullScreen()) {
        showNormal();
        killTimer(m_hideCursorTid);
        m_hideCursorTid = 0;
        m_vinfo.fullScreen = false;
        emit dApp->signalM->showBottomToolbar();
        m_viewB->viewport()->setCursor(Qt::ArrowCursor);
    } else {
//        window()->setWindowFlags (Qt::Window);
        showFullScreen();
        m_vinfo.fullScreen = true;
        if (!m_menu->isVisible()  && qApp->modalWindow() != nullptr) {
            m_viewB->viewport()->setCursor(Qt::BlankCursor);
        }
        m_hideCursorTid = startTimer(DELAY_HIDE_CURSOR_INTERVAL);
    }
}

void ViewPanel::removeCurrentImage()
{
//    m_infos.removeAt(m_current);
//    if (m_infos.isEmpty()) {
    if (m_ttbc == nullptr) {
        return;
    }
    if (m_ttbc->getAllFileCount() <= 0) {
        qDebug() << "No images to show!";
//        if (window()->isFullScreen())
//            showNormal();
        if (m_bFirstFullScreen)
            emit dApp->signalM->hideImageView();
        else {
            toggleFullScreen();
        }

        emit imageChanged("");

        m_emptyWidget->setThumbnailImage(QPixmap());
        m_stack->setCurrentIndex(1);
        emit dApp->signalM->hideImageView();

        bottomTopLeftContent();
        emit dApp->signalM->updateBottomToolbarContent(m_ttbc, (m_filepathlist.size() > 1));
    } else {
        emit dApp->signalM->updateBottomToolbar(m_ttbc->getAllFileCount() > 1);
    }
}

void ViewPanel::initStack()
{
    m_stack = new QStackedWidget;
    m_stack->setMouseTracking(true);
    m_stack->setContentsMargins(0, 0, 0, 0);

    // View frame
    initViewContent();
    QVBoxLayout *vl = new QVBoxLayout(this);
    connect(dApp->signalM, &SignalManager::showTopToolbar, this, [ = ] {
        vl->setContentsMargins(0, 0, 0, 0);
    });
    connect(dApp->signalM, &SignalManager::hideTopToolbar, this, [ = ] {
        vl->setContentsMargins(0, 0, 0, 0);
    });
    vl->addWidget(m_stack);

    // Empty frame
    m_emptyWidget = new ThumbnailWidget("", "");


    m_stack->addWidget(m_viewB);
    m_stack->addWidget(m_emptyWidget);
    //Lock frame: if the deepin-image-viewer
    //can't access to read the image
    m_lockWidget = new LockWidget("", "");
    m_stack->addWidget(m_lockWidget);
}

void ViewPanel::backToLastPanel()
{
    if (window()->isFullScreen()) {
        showNormal();
    }
    if (m_vinfo.lastPanel) {
        emit dApp->signalM->gotoPanel(m_vinfo.lastPanel);
        emit dApp->signalM->hideExtensionPanel(true);
        emit dApp->signalM->showBottomToolbar();
    } else {
    }
}

void ViewPanel::rotateImage(bool clockWise)
{
    if (clockWise) {
        m_viewB->rotateClockWise();
    } else {
        m_viewB->rotateCounterclockwise();
    }
}

void ViewPanel::initViewContent()
{
    m_viewB = new ImageView(this);
    connect(m_viewB, &ImageView::doubleClicked, this, &ViewPanel::onDoubleClicked);
    connect(m_viewB, &ImageView::clicked, this, &ViewPanel::onViewBClicked);
    connect(m_viewB, &ImageView::imageChanged, this, &ViewPanel::onViewBImageChanged);
    connect(m_viewB, &ImageView::sigFIleDelete, this, &ViewPanel::onFIleDelete);
}

void ViewPanel::openImage(const QString &path, bool bjudge)
{
    qDebug() << "---" << __FUNCTION__ << "---";
    if (bjudge && m_currentpath == path)
        return;
    m_currentpath = path;
    m_viewB->setImage(path);    //设置当前显示图片
    //m_ttbc->setButtonDisabled(!QFileInfo(path).exists());
    updateMenuContent();
    if (!QFileInfo(path).exists()) {
//        m_emptyWidget->setThumbnailImage(utils::image::getThumbnail(path));
        ImageDataSt data;
        if (ImageEngineApi::instance()->getImageData(path, data))
            m_emptyWidget->setThumbnailImage(/*dApp->m_imagemap.value(path)*/data.imgpixmap);
        m_stack->setCurrentIndex(1);
    } else if (!QFileInfo(path).isReadable()) {
        m_stack->setCurrentIndex(2);
    } else if (QFileInfo(path).isReadable() && !QFileInfo(path).isWritable()) {
        m_stack->setCurrentIndex(0);
    } else {
        m_stack->setCurrentIndex(0);

        // open success.
        DRecentData data;
        data.appName = "Deepin Image Viewer";
        data.appExec = "deepin-image-viewer";
        DRecentManager::addItem(path, data);
    }
}
