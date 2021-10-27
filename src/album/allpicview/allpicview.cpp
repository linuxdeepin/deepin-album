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
#include "allpicview.h"
#include <QMimeData>
#include "imageengine/imageengineapi.h"
#include "mainwindow.h"
#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <dgiovolume.h>

#include "ac-desktop-define.h"
#include "batchoperatewidget.h"
#include "noresultwidget.h"
#include "imageviewer.h"

namespace {
struct MetaData {
    QString key;
    const char *name;
};

static MetaData MetaDataBasics[] = {
    {"FileName",            QT_TRANSLATE_NOOP("MetadataName", "Photo name")},
    {"FileFormat",          QT_TRANSLATE_NOOP("MetadataName", "Type")},
    {"FileSize",            QT_TRANSLATE_NOOP("MetadataName", "File size")},
    {"Dimension",           QT_TRANSLATE_NOOP("MetadataName", "Dimensions")},
    {"DateTimeOriginal",    QT_TRANSLATE_NOOP("MetadataName", "Date captured")},
    {"DateTimeDigitized",   QT_TRANSLATE_NOOP("MetadataName", "Date modified")},
    {"Tag",                 QT_TRANSLATE_NOOP("MetadataName", "Tag")},
    {"", ""}
};
};

namespace {
const int VIEW_IMPORT = 0;
const int VIEW_ALLPICS = 1;
const int VIEW_SEARCH = 2;
}  //namespace

AllPicView::AllPicView()
    : step(0)
{
    setAcceptDrops(true);
    fatherwidget = new DWidget(this);
    fatherwidget->setFixedSize(this->size());
    m_pStackedWidget = new DStackedWidget(this);
    AC_SET_OBJECT_NAME(m_pStackedWidget, All_Picture_StackedWidget);
    AC_SET_ACCESSIBLE_NAME(m_pStackedWidget, All_Picture_StackedWidget);
    m_pImportView = new ImportView();
    m_pThumbnailListView = new ThumbnailListView(ThumbnailDelegate::AllPicViewType, COMMON_STR_ALLPHOTOS);
//    m_pThumbnailListView->setStyleSheet("background-color:red;");
    AC_SET_OBJECT_NAME(m_pThumbnailListView, All_Picture_Thembnail);
    AC_SET_ACCESSIBLE_NAME(m_pThumbnailListView, All_Picture_Thembnail);
    m_thumbnailListViewWidget = new DWidget();
    QLayout *m_mainLayout = new QVBoxLayout();
    m_mainLayout->addWidget(m_pThumbnailListView);

    //初始化筛选无结果窗口
    m_noResultWidget = new NoResultWidget(this);
    m_mainLayout->addWidget(m_noResultWidget);
    m_noResultWidget->setVisible(false);

    m_thumbnailListViewWidget->setLayout(m_mainLayout);
    m_pSearchView = new SearchView();
    m_pStackedWidget->addWidget(m_pImportView);
    m_pStackedWidget->setCurrentIndex(VIEW_IMPORT);
    m_pStackedWidget->addWidget(m_thumbnailListViewWidget);
    m_pStackedWidget->addWidget(m_pSearchView);
    m_pStackedWidget->setCurrentIndex(VIEW_ALLPICS);
    m_pStatusBar = new StatusBar(this);
    m_pStatusBar->raise();
    m_pStatusBar->setFixedWidth(this->width());
    m_pStatusBar->move(0, this->height() - m_pStatusBar->height());
    QVBoxLayout *pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins(0, 0, 0, 0);
    pVBoxLayout->addWidget(m_pStackedWidget);
    fatherwidget->setLayout(pVBoxLayout);
    m_mainLayout->setContentsMargins(2, 0, 0, m_pStatusBar->height());
    //初始化悬浮窗
    initSuspensionWidget();
    initConnections();
//    m_spinner = new DSpinner(this);
//    m_spinner->setFixedSize(40, 40);
//    m_spinner->hide();
    connect(m_pThumbnailListView, &ThumbnailListView::sigUpdatePicNum, this, &AllPicView::updatePicsIntoThumbnailViewWithCache);
    connect(m_pThumbnailListView, &ThumbnailListView::sigDBImageLoaded, this, &AllPicView::updateStackedWidget);
    m_pwidget = new QWidget(this);
    m_pwidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    if (ImageEngineApi::instance()->getAllImageDataCount() > 0) {
        m_pThumbnailListView->slotLoadFirstPageThumbnailsFinish();
    } else {
        if (ImageEngineApi::instance()->m_firstPageIsLoaded) {
            //相册为空的情况。
            m_pThumbnailListView->slotLoadFirstPageThumbnailsFinish();
        } else {
            connect(ImageEngineApi::instance(), &ImageEngineApi::sigLoadFirstPageThumbnailsToView,
                    m_pThumbnailListView, &ThumbnailListView::slotLoadFirstPageThumbnailsFinish);
        }
    }
    AC_SET_OBJECT_NAME(this, All_Picture_View);
    AC_SET_ACCESSIBLE_NAME(this, All_Picture_View);
}

bool AllPicView::imageImported(bool success)
{
    Q_UNUSED(success);
    emit dApp->signalM->closeWaitDialog();
    return true;
}

void AllPicView::initConnections()
{
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList &");
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &AllPicView::updatePicsIntoThumbnailView);
    //有图片删除后，刷新列表
    connect(dApp->signalM, &SignalManager::imagesRemovedPar, this, &AllPicView::onImgRemoved);
    // 添加重复照片提示
    connect(dApp->signalM, &SignalManager::RepeatImportingTheSamePhotos, this, &AllPicView::onRepeatImportingTheSamePhotos);
    connect(m_pThumbnailListView, &ThumbnailListView::openImage, this, &AllPicView::onOpenImage);
    connect(m_pThumbnailListView, &ThumbnailListView::sigSlideShow, this, &AllPicView::onSlideShow);
    //图片旋转后更新图片
    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &AllPicView::updatePicsThumbnailView);
    connect(m_pStatusBar->m_pSlider, &DSlider::valueChanged, dApp->signalM, &SignalManager::emitSliderValueChg);
//    connect(m_pThumbnailListView, &ThumbnailListView::sigMouseMove, this, &AllPicView::updatePicNum);
//    connect(m_pThumbnailListView, &ThumbnailListView::sigMouseRelease, this, &AllPicView::updatePicNum);
//    connect(m_pThumbnailListView, &ThumbnailListView::customContextMenuRequested, this, &AllPicView::updatePicNum);

    //缩略图选中项发生变化
    connect(m_pThumbnailListView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AllPicView::sltSelectionChanged);
    connect(m_pSearchView->m_pThumbnailListView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AllPicView::sltSelectionChanged);
    //筛选显示，当先列表中内容为无结果
    connect(m_pThumbnailListView, &ThumbnailListView::sigNoPicOrNoVideo, this, &AllPicView::slotNoPicOrNoVideo);
//    connect(m_pSearchView->m_pThumbnailListView, &ThumbnailListView::sigMouseRelease, this, &AllPicView::updatePicNum);
//    connect(m_pSearchView->m_pThumbnailListView, &ThumbnailListView::customContextMenuRequested, this, &AllPicView::updatePicNum);
    connect(m_pImportView->m_pImportBtn, &DPushButton::clicked, this, &AllPicView::onImportViewImportBtnClicked);
    connect(dApp->signalM, &SignalManager::sigImportFailedToView, this, &AllPicView::onImportFailedToView);
//    connect(m_pThumbnailListView, &ThumbnailListView::sigSelectAll, this, &AllPicView::updatePicNum);
    connect(dApp->signalM, &SignalManager::sigShortcutKeyDelete, this, &AllPicView::onKeyDelete);
    connect(dApp->signalM, &SignalManager::sigMonitorChanged, this, &AllPicView::monitorHaveNewFile);
}

void AllPicView::initSuspensionWidget()
{
    //添加悬浮title
    m_SuspensionWidget = new QWidget(m_thumbnailListViewWidget);
    m_SuspensionWidget->setFocusPolicy(Qt::ClickFocus);
    //右侧批量操作控件
    QHBoxLayout *hlayoutDateLabel = new QHBoxLayout(m_SuspensionWidget);
    m_SuspensionWidget->setLayout(hlayoutDateLabel);

    m_batchOperateWidget = new BatchOperateWidget(m_pThumbnailListView, BatchOperateWidget::NullType, this);
    m_batchOperateWidget->setObjectName(All_Picture_BatchOperateWidget);
    m_batchOperateWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
    hlayoutDateLabel->addStretch(100);
    hlayoutDateLabel->setContentsMargins(0, 0, 19, 0);
    hlayoutDateLabel->addWidget(m_batchOperateWidget);

    DPalette ppal_light = DApplicationHelper::instance()->palette(m_SuspensionWidget);
//    ppal_light.setBrush(DPalette::Background, ppal_light.color(DPalette::Base));
    ppal_light.setBrush(DPalette::Base, ppal_light.color(DPalette::Window));
    QGraphicsOpacityEffect *opacityEffect_light = new QGraphicsOpacityEffect;
    opacityEffect_light->setOpacity(0.95);
    m_SuspensionWidget->setPalette(ppal_light);
    m_SuspensionWidget->setGraphicsEffect(opacityEffect_light);
    m_SuspensionWidget->setAutoFillBackground(true);

    m_SuspensionWidget->setContentsMargins(0, 0, 0, 0);
    m_SuspensionWidget->setGeometry(0, 0, this->width() - 15, SUSPENSION_WIDGET_HEIGHT);
}

void AllPicView::updateStackedWidget()
{
    if (0 < DBManager::instance()->getImgsCount()) {
        m_pStackedWidget->setCurrentIndex(VIEW_ALLPICS);
        m_pStatusBar->setVisible(true);
    } else {
        m_pStackedWidget->setCurrentIndex(VIEW_IMPORT);
        m_pStatusBar->setVisible(false);
    }
    updatePicNum();
}

void AllPicView::monitorHaveNewFile(QStringList list)
{
    ImageEngineApi::instance()->ImportImagesFromFileList(list, "", this, true);
}

void AllPicView::updatePicsIntoThumbnailView()
{
    m_pThumbnailListView->stopLoadAndClear(true);
    DBImgInfoList infoList = DBManager::instance()->getAllInfos();
    //加空白栏
    m_pThumbnailListView->insertBlankOrTitleItem(ItemTypeBlank, "", "", SUSPENSION_WIDGET_HEIGHT);
    m_pThumbnailListView->insertThumbnailByImgInfos(infoList);
    if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
        //donothing
    } else {
        updateStackedWidget();
    }
    m_pThumbnailListView->reloadImage();
    restorePicNum();
}

void AllPicView::updatePicsIntoThumbnailViewWithCache()
{
    m_pThumbnailListView->stopLoadAndClear(false);
    if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
        //donothing
    } else {
        updateStackedWidget();
    }
    restorePicNum();
}

void AllPicView::onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, const QString &albumName)
{
    Q_UNUSED(importPaths)
    if (albumName.length() == 0 && dApp->getMainWindow()->getCurrentViewType() == 0) {
        m_pThumbnailListView->selectDuplicatePhotos(duplicatePaths);
    }
}

void AllPicView::onOpenImage(int row, const QString &path, bool bFullScreen)
{
    SignalManager::ViewInfo info;
    info.album = "";
//    info.lastPanel = nullptr;  //todo imageviewer
    info.fullScreen = bFullScreen;
    auto imagelist = m_pThumbnailListView->getFileList(row, ItemTypePic);
    if (imagelist.size() > 0) {
        info.paths << imagelist;
        info.path = path;
    } else {
        info.paths.clear();
    }
    info.viewType = utils::common::VIEW_ALLPIC_SRN;
    info.viewMainWindowID = VIEW_MAINWINDOW_ALLPIC;
    info.dBImgInfos = m_pThumbnailListView->getAllFileInfo(row);

//    emit dApp->signalM->viewImage(info);
    if (bFullScreen) {
        emit dApp->signalM->sigViewImage(info, Operation_FullScreen);
    } else {
        emit dApp->signalM->sigViewImage(info, Operation_NoOperation);
    }
    emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALLPIC);
}

void AllPicView::onSlideShow(const QString &path)
{
    SignalManager::ViewInfo info;
    info.album = "";
//    info.lastPanel = nullptr;  //todo imageviewer
    auto photolist = m_pThumbnailListView->selectedPaths();
    if (photolist.size() > 1) {
        //如果选中数目大于1，则幻灯片播放选中项
        info.paths = photolist;
        info.path = photolist.at(0);
    } else {
        //如果选中项只有一项，则幻灯片播放全部
        info.paths = m_pThumbnailListView->getFileList();
        info.path = path;
    }
    info.fullScreen = true;
    info.slideShow = true;
    info.viewType = utils::common::VIEW_ALLPIC_SRN;
    info.viewMainWindowID = VIEW_MAINWINDOW_ALLPIC;
    emit dApp->signalM->startSlideShow(info);
}

void AllPicView::onImportViewImportBtnClicked()
{
    emit dApp->signalM->startImprot();
    m_pImportView->onImprotBtnClicked();
    //导入结束后判断是否有导入，有导入则切换显示列表
    if (ImageEngineApi::instance()->getAllImageDataCount() > 0) {
        m_pStackedWidget->setCurrentIndex(VIEW_ALLPICS);
    }
}

void AllPicView::onImportFailedToView()
{
    if (isVisible()) {
//        m_spinner->hide();
//        m_spinner->stop();
        updateStackedWidget();
    }
}

void AllPicView::onImgRemoved(const DBImgInfoList &infos)
{
    updateStackedWidget();
}

void AllPicView::sltSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    updatePicNum();
}

ThumbnailListView *AllPicView::getThumbnailListView()
{
    return m_pThumbnailListView;
}

void AllPicView::updatePicsThumbnailView(QStringList strpath)
{
    if (strpath.empty()) {
        updatePicsIntoThumbnailViewWithCache();
    } else {
        updatePicsIntoThumbnailView();
    }
}

void AllPicView::dragEnterEvent(QDragEnterEvent *e)
{
    if (!utils::base::checkMimeUrls(e->mimeData()->urls())) {
        return;
    }
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void AllPicView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    ImageEngineApi::instance()->ImportImagesFromUrlList(urls, nullptr, this);
    event->accept();
}

void AllPicView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void AllPicView::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_UNUSED(e);
}

void AllPicView::resizeEvent(QResizeEvent *e)
{
    Q_UNUSED(e);
//    m_spinner->move(width() / 2 - 20, (height() - 50) / 2 - 20);
    m_pwidget->setFixedHeight(this->height() - 23);
    m_pwidget->setFixedWidth(this->width());
    m_pwidget->move(0, 0);
    m_pStatusBar->setFixedWidth(this->width());
    m_pStatusBar->move(0, this->height() - m_pStatusBar->height());
    fatherwidget->setFixedSize(this->size());
    m_SuspensionWidget->setGeometry(0, 0, width() - 15, SUSPENSION_WIDGET_HEIGHT);
    updatePicNum();
}
//筛选显示，当先列表中内容为无结果
void AllPicView::slotNoPicOrNoVideo(bool isNoResult)
{
    qDebug() << __FUNCTION__ << "---" << isNoResult;
    m_noResultWidget->setVisible(isNoResult);
    m_pThumbnailListView->setVisible(!isNoResult);
    if (isNoResult) {
        m_pStatusBar->m_pAllPicNumLabel->setText(QObject::tr("No results"));
    } else {
        updatePicNum();
    }
}

void AllPicView::updatePicNum()
{
    int photoSelctCount = 0;
    int videoSelctCount = 0;
    if (VIEW_ALLPICS == m_pStackedWidget->currentIndex()) {
        photoSelctCount = m_pThumbnailListView->getAppointTypeSelectItemCount(ItemTypePic);
        videoSelctCount = m_pThumbnailListView->getAppointTypeSelectItemCount(ItemTypeVideo);
    } else if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
        photoSelctCount = m_pSearchView->m_pThumbnailListView->getAppointTypeSelectItemCount(ItemTypePic);
        videoSelctCount = m_pSearchView->m_pThumbnailListView->getAppointTypeSelectItemCount(ItemTypeVideo);
    }

    if (photoSelctCount > 0 || videoSelctCount > 0) {
        m_pStatusBar->resetSelectedStatue(photoSelctCount, videoSelctCount);
    } else {
        restorePicNum();
    }
}

const ThumbnailListView *AllPicView::getAllPicThumbnailListViewModel()
{
    return m_pThumbnailListView;
}

void AllPicView::restorePicNum()
{
    int photoCount = 0;
    int videoCount = 0;
    if (VIEW_ALLPICS == m_pStackedWidget->currentIndex()) {
        photoCount = m_pThumbnailListView->getAppointTypeItemCount(ItemTypePic);
        videoCount = m_pThumbnailListView->getAppointTypeItemCount(ItemTypeVideo);
    } else if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
        photoCount = m_pSearchView->m_pThumbnailListView->getAppointTypeItemCount(ItemTypePic);
        videoCount = m_pSearchView->m_pThumbnailListView->getAppointTypeItemCount(ItemTypeVideo);
    }
    m_pStatusBar->resetUnselectedStatue(photoCount, videoCount);
}

void AllPicView::onKeyDelete()
{
    if (!isVisible())
        return;
    if (VIEW_SEARCH == m_pStackedWidget->currentIndex())
        return;

    QStringList paths = m_pThumbnailListView->selectedPaths();
    m_pThumbnailListView->clearSelection();
    ImageEngineApi::instance()->moveImagesToTrash(paths);
}
