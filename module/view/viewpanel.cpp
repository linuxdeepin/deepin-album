
/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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
#include "controller/divdbuscontroller.h"
#include "controller/signalmanager.h"
#include "controller/configsetter.h"
#include "contents/imageinfowidget.h"
#include "contents/ttbcontent.h"
#include "contents/ttmcontent.h"
#include "contents/ttlcontent.h"
#include "scen/imageview.h"
#include "utils/imageutils.h"
#include "utils/baseutils.h"
#include "widgets/imagebutton.h"
#include "widgets/printoptionspage.h"
#include "widgets/printhelper.h"
#include "utils/snifferimageformat.h"
#include "widgets/dialogs/imgdeletedialog.h"
#include "imageengine/imageengineapi.h"
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
#include <DRecentManager>
#include <DWidgetUtil>
#include <QDesktopWidget>

using namespace Dtk::Core;
using namespace Dtk::Widget;

namespace {

const int DELAY_HIDE_CURSOR_INTERVAL = 3000;
//const QSize ICON_SIZE = QSize(48, 40);

}  // namespace

ViewPanel::ViewPanel(QWidget *parent)
    : ModulePanel(parent)
    , m_hideCursorTid(0)
    , m_isInfoShowed(false)
    , m_viewB(nullptr)
    , m_info(nullptr)
    , m_stack(nullptr)
    , m_iSlideShowTimerId(0)
//    , m_viewType(utils::common::VIEW_ALLPIC_SRN)
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
    m_ttbc = nullptr;
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
//    ImageEngineApi::instance()->loadImagesFromLocal(files, this);
    imageLocalLoaded(files);
}

void ViewPanel::loadFilesFromLocal(DBImgInfoList files)
{
    ImageEngineApi::instance()->loadImagesFromLocal(files, this);
}

bool ViewPanel::imageLocalLoaded(QStringList &filelist)
{
    onViewImage(filelist);
//        emit dApp->signalM->updateTopToolbarLeftContent(toolbarTopLeftContent());
//    emit dApp->signalM->updateBottomToolbarContent(bottomTopLeftContent(), (m_ttbc->itemLoadedSize() > 1));
//        emit dApp->signalM->updateTopToolbarMiddleContent(toolbarTopMiddleContent());
//    if (NULL == m_vinfo.lastPanel) {
//        return false;
//    } else if (m_vinfo.lastPanel->moduleName() == "AlbumPanel" ||
//               m_vinfo.lastPanel->moduleName() == "ViewPanel") {
//        m_currentImageLastDir = m_vinfo.album;
//        emit viewImageFrom(m_vinfo.album);
//    } else if (m_vinfo.lastPanel->moduleName() == "TimelinePanel") {
//        m_currentImageLastDir = "Timeline";
//        emit viewImageFrom("Timeline");
//    }
    return true;
}

void ViewPanel::initConnect()
{
    connect(dApp->signalM, &SignalManager::deleteByMenu, this, [ = ] {

        if (m_dt->isActive())
        {
            return;
        }
        m_dt->start();


        emit ttbcDeleteImage();
        if (m_vinfo.fullScreen)
        {
            emit dApp->signalM->hideBottomToolbar();
        }
    });

    connect(dApp->signalM, &SignalManager::gotoPanel,
    this, [ = ](ModulePanel * p) {
        if (p != this) {
            emit dApp->signalM->showTopToolbar();
        } else {
            emit dApp->signalM->showTopToolbar();
//            emit dApp->signalM->hideBottomToolbar(true);
            emit dApp->signalM->showBottomToolbar();
        }
    });
    connect(dApp->signalM, &SignalManager::showExtensionPanel, this, [ = ] {
        m_isInfoShowed = true;
    });
    connect(dApp->signalM, &SignalManager::hideExtensionPanel, this, [ = ] {
        m_isInfoShowed = false;
    });
    connect(dApp->signalM, &SignalManager::hideImageView, this, [ = ] {
        m_viewB->clear();
        showNormal();
    });

    qRegisterMetaType<SignalManager::ViewInfo>("SignalManager::ViewInfo");
    connect(dApp->signalM, &SignalManager::viewImageNoNeedReload,
    this, [ = ](int &fileindex) {
//        emit imageChanged(filename);
//        openImage(filename);
        showImage(fileindex, 0);
    });

    connect(dApp->signalM, &SignalManager::viewImage,
    this, [ = ](const SignalManager::ViewInfo & vinfo) {
        m_vinfo = vinfo;
        QList<QByteArray> fList =  QMovie::supportedFormats(); //"gif","mng","webp"
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
            //LMH0415
            SignalManager::ViewInfo newinfo = vinfo;
            newinfo.paths.clear();
            int index = -1;
            if (m_vinfo.paths.size() == 0) { //only a piece of image
                index = m_vinfo.path.compare(newinfo.path) == 0 ? 0 : index;
            } else {
                index = m_vinfo.paths.indexOf(newinfo.path);
            }

            if (index < 0) {
                return false;
            }
            if (index > 50) {
                for (int i = index - 50; i < index; i++) {
                    newinfo.paths << vinfo.paths[i];
                }
            } else {
                for (int i = 0; i < index; i++) {
                    newinfo.paths << vinfo.paths[i];
                }
            }
            if (index + 50 < vinfo.paths.count()) {
                for (int i = index; i < index + 50; i++) {
                    newinfo.paths << vinfo.paths[i];
                }
            } else {
                for (int i = index; i < vinfo.paths.count(); i++) {
                    newinfo.paths << vinfo.paths[i];
                }
            }
            m_currentpath = m_vinfo.path;
            if (newinfo.paths.size() < 1) {
                newinfo.paths << newinfo.path;
            }
            loadFilesFromLocal(newinfo.paths);
            emit dApp->signalM->showImageView(newinfo.viewMainWindowID);
            emit dApp->signalM->sigOpenPicture(newinfo.path);
            QCoreApplication::processEvents();

            m_currentpath = m_vinfo.path;
            if (m_vinfo.paths.size() < 1) {
                m_vinfo.paths << m_vinfo.path;
            }
            loadFilesFromLocal(m_vinfo.paths);
            m_currentpath = m_vinfo.path;
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
    });

    connect(m_viewB, &ImageView::mouseHoverMoved, this, [ = ] {
        emit mouseMoved();
    });

    //connect(m_emptyWidget, &ThumbnailWidget::mouseHoverMoved, this, &ViewPanel::mouseMoved);
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList &");
    connect(dApp->signalM, &SignalManager::imagesInserted, this, [ = ] {
        if (!m_ttbc)
        {
            return;
        }
        int size = m_ttbc->itemLoadedSize();
        QWidget *pttbc = bottomTopLeftContent();
        emit dApp->signalM->updateBottomToolbarContent(pttbc, (size  > 1));
        emit dynamic_cast<TTBContent *>(pttbc)->sigRequestSomeImages();
//        emit dApp->signalM->updateBottomToolbarContent(bottomTopLeftContent(), (m_ttbc->itemLoadedSize() > 1));
    });

    connect(dApp->signalM, &SignalManager::sigESCKeyActivated, this, [ = ] {
#if 1
        if (isVisible())
        {
            if (0 != m_iSlideShowTimerId) {
                killTimer(m_iSlideShowTimerId);
                m_iSlideShowTimerId = 0;
            }
            toggleFullScreen();
        }
        m_vinfo.fullScreen = false;
        emit dApp->signalM->showBottomToolbar();

#endif
    });
}

//void ViewPanel::updateLocalImages()
//{
//    const QString cp = m_infos.at(m_current).filePath;
//    m_infos = getImageInfos(getFileInfos(cp));
//    m_current = 0;
//    for (; m_current < m_infos.size(); m_current ++) {
//        if (m_infos.at(m_current).filePath == cp) {
//            return;
//        }
//    }
//}

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

void ViewPanel::feedBackCurrentIndex(int index, QString path)
{
    m_current = index;
    m_currentpath = path;
    if (m_current >= m_filepathlist.size()) {
        return;
    }
    openImageFirst(path);
}

void ViewPanel::showNormal()
{
    if (m_isMaximized) {
//        window()->setWindowFlags (Qt::SubWindow);
        window()->showNormal();
        window()->showMaximized();
    } else {
//        window()->setWindowFlags (Qt::SubWindow);
        window()->showNormal();
    }

    emit dApp->signalM->showTopToolbar();
}


void ViewPanel::showFullScreen()
{
    m_isMaximized = window()->isMaximized();
    //加入显示动画效果，以透明度0-1显示，动态加载，视觉效果掩盖左上角展开
    QPropertyAnimation *animation = new QPropertyAnimation(window(), "windowOpacity");
    animation->setDuration(50);
    animation->setEasingCurve(QEasingCurve::Linear);
    animation->setStartValue(0);
    animation->setEndValue(1);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    window()->showFullScreen();
    m_hideCursorTid = startTimer(DELAY_HIDE_CURSOR_INTERVAL);
    emit dApp->signalM->sigShowFullScreen();
}

//int ViewPanel::imageIndex(const QString &path)
//{
//    for (int i = 0; i < m_infos.length(); i ++) {
//        if (m_infos.at(i).filePath == path) {
//            return i;
//        }
//    }

//    return -1;
//}

//DBImgInfoList ViewPanel::getImageInfos(const QFileInfoList &infos)
//{
//    DBImgInfoList imageInfos;
//    for (QFileInfo info : infos) {
//        DBImgInfo imgInfo;

//        // 在 Qt 5.6 上的一个Bug，QFileInfo("").absoluteFilePath()会返回当前目录的绝对路径
////        if (info.isFile())
//        {
//            imgInfo.fileName = info.fileName();
//            imgInfo.filePath = info.absoluteFilePath();
//        }

//        imageInfos << imgInfo;
//    }

//    return imageInfos;
//}

//const QStringList ViewPanel::paths() const
//{
//    QStringList list;
//    for (DBImgInfo info : m_infos) {
//        list << info.filePath;
//    }

//    return list;
//}

QFileInfoList ViewPanel::getFileInfos(const QString &path)
{
    return utils::image::getImagesInfo(QFileInfo(path).path(), false);
}

QWidget *ViewPanel::toolbarBottomContent()
{
    return nullptr;
}

QWidget *ViewPanel::toolbarTopLeftContent()
{
    return nullptr;
}
//QWidget *ViewPanel::toolbarTopLeftContent()
//{
//    TTLContent *ttlc = new TTLContent(m_vinfo.inDatabase);
//    ttlc->setCurrentDir(m_currentImageLastDir);
//    if (! m_infos.isEmpty() && m_current < m_infos.size()) {
//        ttlc->setImage(m_infos.at(m_current).filePath, m_infos);
//    } else {
//        ttlc->setImage("", m_infos);
//    }

//    connect(ttlc, &TTLContent::clicked, this, &ViewPanel::backToLastPanel);
//    connect(this, &ViewPanel::viewImageFrom, ttlc, [ = ](const QString & dir) {
//        ttlc->setCurrentDir(dir);
//    });
////    connect(ttlc, &TTLContent::contentWidthChanged,
////            this, &ViewPanel::updateTopLeftWidthChanged);
////    connect(this, &ViewPanel::updateCollectButton,
////            ttlc, &TTLContent::updateCollectButton);
//    connect(this, &ViewPanel::imageChanged, ttlc, &TTLContent::setImage);
//    connect(ttlc, &TTLContent::rotateClockwise, this, [ = ] {
//        rotateImage(true);
//    });
//    connect(ttlc, &TTLContent::rotateCounterClockwise, this, [ = ] {
//        rotateImage(false);
//    });
//    connect(ttlc, &TTLContent::removed, this, [ = ] {
//        if (m_vinfo.inDatabase)
//        {
//            popupDelDialog(m_infos.at(m_current).filePath);
//        } else
//        {
//            const QString path = m_infos.at(m_current).filePath;
//            removeCurrentImage();
//            utils::base::trashFile(path);
//        }
//    });
//    connect(ttlc, &TTLContent::resetTransform, this, [ = ](bool fitWindow) {
//        if (fitWindow) {
//            m_viewB->fitWindow();
//        } else {
//            m_viewB->fitImage();
//        }
//    });
//    connect(dApp->signalM, &SignalManager::insertedIntoAlbum,
//            ttlc, &TTLContent::updateCollectButton);
//    connect(dApp->signalM, &SignalManager::removedFromAlbum,
//            ttlc, &TTLContent::updateCollectButton);

//    return ttlc;
//}

QWidget *ViewPanel::bottomTopLeftContent()
{
    //如果外设加载，则暂停线程
    emit dApp->signalM->sigPauseOrStart(true);

    if (m_ttbc != nullptr)
        delete m_ttbc;
//        m_ttbc->deleteLater();
    m_ttbc = new TTBContent(m_vinfo.inDatabase, m_filepathlist, this);
    connect(m_ttbc, &TTBContent::feedBackCurrentIndex,
            this, &ViewPanel::feedBackCurrentIndex);
    m_ttbc->m_imageType = m_vinfo.viewType;
//    ttlc->setCurrentDir(m_currentImageLastDir);
    if (! m_filepathlist.isEmpty() /*&& m_current < m_filepathlist.size()*/) {
        m_ttbc->setImage(m_currentpath);
    } else {
        m_ttbc->setImage("");
    }
    connect(m_ttbc, &TTBContent::ttbcontentClicked, this, [ = ] {
        if (0 != m_iSlideShowTimerId)
        {
            killTimer(m_iSlideShowTimerId);
            m_iSlideShowTimerId = 0;
        }
    });
    connect(this, &ViewPanel::ttbcDeleteImage, m_ttbc, &TTBContent::deleteImage);
    connect(this, &ViewPanel::viewImageFrom, m_ttbc, [ = ](const QString & dir) {
        m_ttbc->setCurrentDir(dir);
    });
//    connect(ttlc, &TTLContent::contentWidthChanged,
//            this, &ViewPanel::updateTopLeftWidthChanged);
//    connect(this, &ViewPanel::updateCollectButton,
//            ttlc, &TTLContent::updateCollectButton);
    connect(this, &ViewPanel::imageChanged, m_ttbc, &TTBContent::setImage);
    connect(m_ttbc, &TTBContent::rotateClockwise, this, [ = ] {
        rotateImage(true);
    });
    connect(m_ttbc, &TTBContent::rotateCounterClockwise, this, [ = ] {
        rotateImage(false);
    });
    connect(m_ttbc, &TTBContent::removed, this, [ = ] {
//        SignalManager::instance()->deleteByMenu();
        if (COMMON_STR_TRASH == m_vinfo.viewType)
        {
            ImgDeleteDialog *dialog = new ImgDeleteDialog(this, 1);
            dialog->show();
            connect(dialog, &ImgDeleteDialog::imgdelete, this, [ = ] {
//                dApp->m_imagetrashmap.remove(m_infos.at(m_current).filePath);
                DBManager::instance()->removeTrashImgInfos(QStringList(m_currentpath));
                removeCurrentImage();
                DDesktopServices::trash(m_currentpath);
            });
        } else
        {
            DBImgInfoList infos;
            DBImgInfo info;
            info = DBManager::instance()->getInfoByPath(m_currentpath);
#if 1
//            info.time = QDateTime::currentDateTime();
            info.changeTime = QDateTime::currentDateTime();
#endif
            infos << info;
//            dApp->m_imageloader->addTrashImageLoader(QStringList(m_currentpath));
//            dApp->m_imagemap.remove(m_infos.at(m_current).filePath);
            DBManager::instance()->insertTrashImgInfos(infos);
            DBManager::instance()->removeImgInfos(QStringList(m_currentpath));
            removeCurrentImage();
//            DDesktopServices::trash(m_currentpath);
        }
    });

    connect(m_ttbc, &TTBContent::resetTransform, this, [ = ](bool fitWindow) {
        if (fitWindow) {
            m_viewB->fitWindow();
        } else {
            m_viewB->fitImage();
        }
    });
    connect(m_viewB, &ImageView::disCheckAdaptImageBtn,
            m_ttbc, &TTBContent::disCheckAdaptImageBtn);
    connect(m_viewB, &ImageView::disCheckAdaptScreenBtn,
            m_ttbc, &TTBContent::disCheckAdaptScreenBtn);
    connect(m_viewB, &ImageView::checkAdaptImageBtn,
            m_ttbc, &TTBContent::checkAdaptImageBtn);
    connect(m_viewB, &ImageView::checkAdaptScreenBtn,
            m_ttbc, &TTBContent::checkAdaptScreenBtn);
    connect(dApp->signalM, &SignalManager::insertedIntoAlbum,
            m_ttbc, &TTBContent::updateCollectButton);
    connect(dApp->signalM, &SignalManager::removedFromAlbum,
            m_ttbc, &TTBContent::updateCollectButton);
    connect(m_ttbc, &TTBContent::showPrevious, this, [ = ]() {
        this->showPrevious();
    });
    connect(m_ttbc, &TTBContent::showNext, this, [ = ]() {
        this->showNext();
    });
    connect(m_ttbc, &TTBContent::imageClicked, this, [ = ](int index, int addIndex) {
        this->showImage(index, addIndex);
    });

//    m_ttbc->requestSomeImages();
//    emit m_ttbc->sigRequestSomeImages();
    return m_ttbc;
}
QWidget *ViewPanel::toolbarTopMiddleContent()
{
    QWidget *w = new QWidget();
    return w;
}

QWidget *ViewPanel::extensionPanelContent()
{
    QWidget *w = new QWidget;
    w->setAttribute(Qt::WA_TranslucentBackground);
    QVBoxLayout *l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 0);
    if (! m_info) {
        m_info = new ImageInfoWidget("", "");
    }
    l->addSpacing(0);
    l->addWidget(m_info);
    return w;
}

const SignalManager::ViewInfo ViewPanel::viewInfo() const
{
    return m_vinfo;
}

bool ViewPanel::eventFilter(QObject *obj, QEvent *e)
{
    Q_UNUSED(obj)
    if (e->type() == QEvent::HideToParent) {
        m_viewB->clear();
    }
    if (e->type() == QEvent::Resize && this->isVisible()) {
//        emit dApp->signalM->updateTopToolbarLeftContent(toolbarTopLeftContent());
//        emit dApp->signalM->updateBottomToolbarContent(bottomTopLeftContent(), (m_infos.size() > 1));
        emit sigResize();
//        emit dApp->signalM->updateTopToolbarMiddleContent(toolbarTopMiddleContent());
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
//    if (window()->isMaximized()) {
//        emit dApp->signalM->updateTopToolbarLeftContent(toolbarTopLeftContent());
//        emit dApp->signalM->updateBottomToolbarContent(bottomTopLeftContent(), (m_infos.size() > 1));
//        emit sigResize();
//        emit dApp->signalM->updateTopToolbarMiddleContent(toolbarTopMiddleContent());
//    }

    if (m_viewB->isFitImage()) {
        m_viewB->fitImage();
    } else if (m_viewB->isFitWindow()) {
        m_viewB->fitWindow();
    }
}

void ViewPanel::timerEvent(QTimerEvent *e)
{
    if (e->timerId() == m_hideCursorTid &&
            !m_menu->isVisible() && !m_printDialogVisible) {
        m_viewB->viewport()->setCursor(Qt::BlankCursor);
    }

    if (e->timerId() == m_iSlideShowTimerId) {
        showNext();
    }

    ModulePanel::timerEvent(e);
}

void ViewPanel::wheelEvent(QWheelEvent *e)
{
    if (m_viewB && !m_viewB->path().isEmpty())
        qApp->sendEvent(m_viewB->viewport(), e);
}

//void ViewPanel::dropEvent(QDropEvent *event)
//{
//    QList<QUrl> urls = event->mimeData()->urls();
//    if (urls.isEmpty()) {
//        return;
//    }

//    using namespace utils::image;
//    QStringList paths;
//    for (QUrl url : urls) {
//        const QString path = url.toLocalFile();
//        if (QFileInfo(path).isDir()) {
//            auto finfos =  getImagesInfo(path, false);
//            for (auto finfo : finfos) {
//                if (imageSupportRead(finfo.absoluteFilePath())) {
//                    paths << finfo.absoluteFilePath();
//                }
//            }
//        } else if (imageSupportRead(path)) {
//            paths << path;
//        }
//    }

//    if (! paths.isEmpty()) {
//#ifdef LITE_DIV
//        SignalManager::ViewInfo vinfo;

//        vinfo.path = paths.first();
//        vinfo.paths = paths;

//        onViewImage(vinfo);
//#else
//        viewOnNewProcess(paths);
//#endif
//    }

//    event->accept();
//    ModulePanel::dropEvent(event);
//}

//void ViewPanel::dragEnterEvent(QDragEnterEvent *event)
//{
//    event->setDropAction(Qt::CopyAction);
//    event->accept();
//    ModulePanel::dragEnterEvent(event);
//}

void ViewPanel::onViewImage(const QStringList &vinfo)
{
    using namespace utils::base;
    this->setCursor(Qt::ArrowCursor);
    if (m_vinfo.fullScreen) {
//        m_isMaximized = m_vinfo.fullScreen;
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
    m_current = -1;
    m_filepathlist = vinfo;
    // The control buttons is difference
//    if (! m_vinfo.inDatabase) {
////        emit dApp->signalM->updateTopToolbarLeftContent(toolbarTopLeftContent());
//        emit dApp->signalM->updateBottomToolbarContent(bottomTopLeftContent(), (vinfo.size() > 1));
////        emit dApp->signalM->updateTopToolbarMiddleContent(toolbarTopMiddleContent());
////        return;
//    }

//    // Get view range
//    if (! vinfo.paths.isEmpty()) {
//        QFileInfoList list;
//        for (QString path : vinfo.paths) {
//            list << QFileInfo(path);
//        }
//        m_infos = getImageInfos(list);
//    } else {
//        QFileInfo info(vinfo.path);

//        m_infos = getImageInfos({info});
//    }
    if (vinfo.size() == 1) {
        m_imageDirIterator.reset(new QDirIterator(QFileInfo(vinfo.first()).absolutePath(),
                                                  utils::image::supportedImageFormats(), QDir::Files | QDir::Readable));
    } else {
        m_imageDirIterator.reset();
    }

    QWidget *pttbc = bottomTopLeftContent();
    emit dApp->signalM->updateBottomToolbarContent(pttbc, (vinfo.size() > 1));
    emit dynamic_cast<TTBContent *>(pttbc)->sigRequestSomeImages();
}

//void ViewPanel::onViewImage(const SignalManager::ViewInfo &vinfo)
//{
//    using namespace utils::base;
//    this->setCursor(Qt::ArrowCursor);
//    if (vinfo.fullScreen) {
//        showFullScreen();
//    }

//    if (vinfo.slideShow) {
//        m_iSlideShowTimerId = startTimer(3000);
//    } else {
//        if (0 != m_iSlideShowTimerId) {
//            killTimer(m_iSlideShowTimerId);
//            m_iSlideShowTimerId = 0;
//        }
//    }

//    emit dApp->signalM->gotoPanel(this);

//    // The control buttons is difference
//    if (! vinfo.inDatabase) {
////        emit dApp->signalM->updateTopToolbarLeftContent(toolbarTopLeftContent());
//        if (!m_ttbc) {
//            return;
//        }
//        emit dApp->signalM->updateBottomToolbarContent(bottomTopLeftContent(), (m_infos.size() > 1));
////        emit dApp->signalM->updateTopToolbarMiddleContent(toolbarTopMiddleContent());
//    }

//    // Get view range
//    if (! vinfo.paths.isEmpty()) {
//        QFileInfoList list;
//        for (QString path : vinfo.paths) {
//            list << QFileInfo(path);
//        }
//        m_infos = getImageInfos(list);
//    } else {
//        QFileInfo info(vinfo.path);

//        m_infos = getImageInfos({info});
//    }

//    if (m_infos.size() == 1) {
//        m_imageDirIterator.reset(new QDirIterator(QFileInfo(m_infos.first().filePath).absolutePath(),
//                                                  utils::image::supportedImageFormats(), QDir::Files | QDir::Readable));
//    } else {
//        m_imageDirIterator.reset();
//    }
//    // Get the image which need to open currently
//    m_current = 0;
//    if (! vinfo.path.isEmpty()) {
//        for (; m_current < m_infos.size(); m_current ++) {
//            if (m_infos.at(m_current).filePath == vinfo.path) {
//                break;
//            }
//        }
//    }

//    if (m_current == m_infos.size()) {
//        qWarning() << "The specify path not in view range: "
//                   << vinfo.path << vinfo.paths;
//        return;
//    }
//    if (m_infos.at(m_current).filePath != nullptr) {
//        openImage(m_infos.at(m_current).filePath);
////        eatImageDirIterator();
//    }
//    emit dApp->signalM->updateBottomToolbarContent(bottomTopLeftContent(), (m_infos.size() > 1));
//}

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
        if (!m_menu->isVisible()) {
            m_viewB->viewport()->setCursor(Qt::BlankCursor);
        }
        m_hideCursorTid = startTimer(DELAY_HIDE_CURSOR_INTERVAL);
    }
}

bool ViewPanel::showPrevious()
{
    if (m_dt->isActive()) {
        return false;
    }
    m_dt->start();
#ifdef LITE_DIV
//    eatImageDirIterator();
#endif

    if (m_filepathlist.isEmpty()) {
        return false;
    }

    if (m_current <= 0) {
//        m_current = m_infos.size()-1;
    } else {
        --m_current;
    }

    openImage(m_ttbc->getIndexPath(m_current), m_vinfo.inDatabase);
    return true;
}

bool ViewPanel::showNext()
{
    if (m_dt->isActive()) {
        return false;
    }
    m_dt->start();
#ifdef LITE_DIV
//    eatImageDirIterator();
#endif

    if (m_filepathlist.isEmpty()) {
        return false;
    }

    if (m_current == m_filepathlist.size() - 1) {
//        m_current = 0;
    } else {
        ++m_current;
    }


    openImage(m_ttbc->getIndexPath(m_current), m_vinfo.inDatabase);
    return true;
}
bool ViewPanel::showImage(int index, int addindex)
{
    Q_UNUSED(addindex);
    if (m_filepathlist.isEmpty()) {
        return false;
    }
    m_current = index;

    openImage(m_ttbc->getIndexPath(m_current), m_vinfo.inDatabase);
    return true;
}

void ViewPanel::removeCurrentImage()
{
    if (m_filepathlist.isEmpty()) {
        return;
    }

    m_filepathlist.removeAt(m_current);
    if (m_current >= m_filepathlist.size()) {
        m_current = 0;
    }
//    m_infos.removeAt(m_current);
//    if (m_infos.isEmpty()) {
    if (m_filepathlist.isEmpty()) {
        qDebug() << "No images to show!";
        m_current = 0;
        if (window()->isFullScreen())
            showNormal();
        emit imageChanged("");

        m_emptyWidget->setThumbnailImage(QPixmap());
        m_stack->setCurrentIndex(1);
        emit dApp->signalM->hideImageView();

        QWidget *pttbc = bottomTopLeftContent();
        emit dApp->signalM->updateBottomToolbarContent(pttbc, (m_filepathlist.size() > 1));
        emit dynamic_cast<TTBContent *>(pttbc)->sigRequestSomeImages();
    } else {
        m_vinfo.paths = m_filepathlist;
        m_vinfo.path = m_filepathlist[m_current];
//        dApp->signalM->viewImage(m_vinfo);
        openImage(m_filepathlist[m_current]/*, m_vinfo.inDatabase*/);
        emit dApp->signalM->updateBottomToolbar(m_filepathlist.size() > 1);
    }
}

void ViewPanel::viewOnNewProcess(const QStringList &paths)
{
    const QString pro = "deepin-image-viewer";
    QProcess *p = new QProcess;
    connect(p, SIGNAL(finished(int)), p, SLOT(deleteLater()));

    QStringList options;
    for (QString path : paths) {
        options << "-o" << path;
    }
    p->start(pro, options);
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
#ifndef LITE_DIV
        // Use dbus interface to make sure it will always back to the
        // main process
        DIVDBusController().backToMainWindow();
#endif
    }
}

void ViewPanel::rotateImage(bool clockWise)
{
    if (clockWise) {
        m_viewB->rotateClockWise();
    } else {
        m_viewB->rotateCounterclockwise();
    }
    m_viewB->autoFit();
    m_info->updateInfo();

//    if (COMMON_STR_TRASH == m_vinfo.viewType) {
//        dApp->m_imageloader->updateTrashImageLoader(QStringList(m_currentpath));
//    } else {
//    dApp->m_imageloader->updateImageLoader(QStringList(m_currentpath));
//    }

//    openImage(m_infos.at(m_current).filePath, m_vinfo.inDatabase);
//    emit imageChanged(m_currentpath);
//    m_ttbc->reLoad();

}

void ViewPanel::initViewContent()
{
    m_viewB = new ImageView(this);

    connect(m_viewB, &ImageView::doubleClicked, [this]() {
        toggleFullScreen();
    });
    connect(m_viewB, &ImageView::clicked, this, [ = ] {
        dApp->signalM->hideExtensionPanel();
    });
    connect(m_viewB, &ImageView::imageChanged, this, [ = ](QString path) {
        emit imageChanged(path);
        // Pixmap is cache in thread, make sure the size would correct after
        // cache is finish
        m_viewB->autoFit();
    });
    connect(m_viewB, &ImageView::previousRequested, this, &ViewPanel::showPrevious);
    connect(m_viewB, &ImageView::nextRequested, this, &ViewPanel::showNext);
}
void ViewPanel::openImageFirst(const QString &path, bool inDB)
{

    m_currentpath = path;

    if (inDB) {

    }
    m_viewB->setImageFirst(path);

    updateMenuContent();

    if (m_info) {
        m_info->setImagePath(path);
    }

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
    if (inDB) {
        emit updateTopLeftContentImage(path);
//        emit updateCollectButton();
    }

    QTimer::singleShot(100, m_viewB, &ImageView::autoFit);
}
void ViewPanel::openImage(const QString &path, bool inDB)
{
    m_currentpath = path;
//    if (! QFileInfo(path).exists()) {
    // removeCurrentImage() will cause timerEvent be trigered again by
    // showNext() or showPrevious(), so delay to remove current image
    // to break the loop
//        TIMER_SINGLESHOT(100, {removeCurrentImage();}, this);
//        return;
//    }

    if (inDB) {
        // TODO
        // Check whether the thumbnail is been rotated in outside
//        QtConcurrent::run(utils::image::removeThumbnail, path);
    }

    m_viewB->setImage(path);
    updateMenuContent();

    if (m_info) {
        m_info->setImagePath(path);
    }

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
    if (inDB) {
        emit updateTopLeftContentImage(path);
//        emit updateCollectButton();
    }

    QTimer::singleShot(100, m_viewB, &ImageView::autoFit);
}
