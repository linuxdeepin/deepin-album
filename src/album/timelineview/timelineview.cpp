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
#include "timelineview.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "imageengine/imageengineapi.h"
#include "mainwindow.h"
#include "ac-desktop-define.h"
#include <QScrollBar>
#include <QScroller>
#include <DPushButton>
#include <QMimeData>
#include <DTableView>
#include <QGraphicsOpacityEffect>
#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <dgiovolume.h>
#include <QDesktopWidget>

namespace  {
const int VIEW_IMPORT = 0;
const int VIEW_TIMELINE = 1;
const int VIEW_SEARCH = 2;
const int VIEW_MAINWINDOW_TIMELINE = 1;
const int TITLEHEIGHT = 50;
const int TIMELINE_TITLEHEIGHT = 32;
} //namespace

TimeLineView::TimeLineView()
    : m_mainListWidget(nullptr), m_mainLayout(nullptr), m_dateItem(nullptr)
    , pSuspensionChose(nullptr)
    , allnum(0), m_pDate(nullptr), pNum_up(nullptr)
    , pNum_dn(nullptr), m_oe(nullptr), m_oet(nullptr)
    , m_ctrlPress(false), lastClickedIndex(0), lastRow(-1)
    , lastChanged(false), fatherwidget(nullptr), m_pStackedWidget(nullptr)
    , m_pStatusBar(nullptr), pSearchView(nullptr), pImportView(nullptr), pTimeLineViewWidget(nullptr)
    , m_pwidget(nullptr), m_index(0), m_selPicNum(0), m_spinner(nullptr)
    , currentTimeLineLoad(0)
{
    setAcceptDrops(true);
    fatherwidget = new QWidget(this);
    fatherwidget->setFixedSize(this->size());
    m_oe = new QGraphicsOpacityEffect(this);
    m_oet = new QGraphicsOpacityEffect(this);
    m_oe->setOpacity(0.5);
    m_oet->setOpacity(0.75);

    m_pStackedWidget = new QStackedWidget();
    pTimeLineViewWidget = new QWidget();
    pImportView = new ImportView();
    pSearchView = new SearchView();
    m_pStackedWidget->addWidget(pImportView);
    m_pStackedWidget->addWidget(pTimeLineViewWidget);
    m_pStackedWidget->addWidget(pSearchView);
    m_pStatusBar = new StatusBar(this);
    m_pStatusBar->raise();
    m_pStatusBar->setFixedWidth(this->width());
    m_pStatusBar->move(0, this->height() - m_pStatusBar->height());
    QVBoxLayout *pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins(2, 0, 0, 0);
    pVBoxLayout->addWidget(m_pStackedWidget);
    fatherwidget->setLayout(pVBoxLayout);

    initTimeLineViewWidget();
    initConnections();
    m_pwidget = new QWidget(this);
    m_pwidget->setFocusPolicy(Qt::NoFocus);
    m_pwidget->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_spinner = new DSpinner(this);
    m_spinner->setFixedSize(40, 40);
    m_spinner->hide();
    clearAndStartLayout();
}

void TimeLineView::initConnections()
{
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList &");
    connect(dApp->signalM, &SignalManager::sigLoadOnePhoto, this, &TimeLineView::clearAndStartLayout);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &TimeLineView::clearAndStartLayout);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &TimeLineView::clearAndStartLayout);
    connect(dApp, &Application::sigFinishLoad, this, &TimeLineView::onFinishLoad);
    connect(m_mainListWidget, &TimelineListWidget::sigNewTime, this, &TimeLineView::onNewTime);
//    connect(m_mainListWidget, &TimelineListWidget::sigDelTime, this, &TimeLineView::on_DelLabel);//未使用
    connect(m_mainListWidget, &TimelineListWidget::sigMoveTime, this, &TimeLineView::on_MoveLabel);
    connect(m_mainListWidget, &TimelineListWidget::sigNeedMoveScorll, this, &TimeLineView::on_MoveScroll);
    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &TimeLineView::updataLayout);
    connect(m_pStatusBar->m_pSlider, &DSlider::valueChanged, dApp->signalM, &SignalManager::sigMainwindowSliderValueChg);
    connect(pSearchView->m_pThumbnailListView, &ThumbnailListView::clicked, this, &TimeLineView::updatePicNum);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &TimeLineView::themeChangeSlot);
    connect(pImportView->m_pImportBtn, &DPushButton::clicked, this, &TimeLineView::onImportViewImportBtnClicked);
    connect(dApp->signalM, &SignalManager::sigImportFailedToView, this, &TimeLineView::onImportFailedToView);
    connect(dApp->signalM, &SignalManager::sigShortcutKeyDelete, this, &TimeLineView::onKeyDelete);
    // 重复导入图片选中
    connect(dApp->signalM, &SignalManager::RepeatImportingTheSamePhotos, this, &TimeLineView::onRepeatImportingTheSamePhotos);
}

void TimeLineView::themeChangeSlot(DGuiApplicationHelper::ColorType themeType)
{
    DPalette pa1 = DApplicationHelper::instance()->palette(m_dateItem);
    pa1.setBrush(DPalette::Background, pa1.color(DPalette::Base));
    m_dateItem->setForegroundRole(DPalette::Background);
    m_dateItem->setPalette(pa1);
    DPalette pa = DApplicationHelper::instance()->palette(m_pDate);
    pa.setBrush(DPalette::Text, pa.color(DPalette::ToolTipText));
    m_pDate->setForegroundRole(DPalette::Text);
    m_pDate->setPalette(pa);

    DPalette pal1 = DApplicationHelper::instance()->palette(pNum_up);
    QColor color_BT1 = pal1.color(DPalette::BrightText);
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT1.setAlphaF(0.5);
        pal1.setBrush(DPalette::Text, color_BT1);
        pNum_up->setForegroundRole(DPalette::Text);
        pNum_up->setPalette(pal1);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT1.setAlphaF(0.75);
        pal1.setBrush(DPalette::Text, color_BT1);
        pNum_up->setForegroundRole(DPalette::Text);
        pNum_up->setPalette(pal1);
    }

    for (int i = 1; i < m_mainListWidget->count(); i++) {
        TimelineItem *item = dynamic_cast<TimelineItem *>(m_mainListWidget->itemWidget(m_mainListWidget->item(i)));
        QList<DLabel *> pLabelList = item->findChildren<DLabel *>();
        DPalette color = DApplicationHelper::instance()->palette(pLabelList[0]);
        color.setBrush(DPalette::Text, color.color(DPalette::ToolTipText));
        pLabelList[0]->setForegroundRole(DPalette::Text);
        pLabelList[0]->setPalette(color);

        DPalette pal = DApplicationHelper::instance()->palette(pLabelList[1]);
        QColor color_BT = pal.color(DPalette::BrightText);
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            color_BT.setAlphaF(0.5);
            pal.setBrush(DPalette::Text, color_BT);
            pLabelList[1]->setForegroundRole(DPalette::Text);
            pLabelList[1]->setPalette(pal);
        } else if (themeType == DGuiApplicationHelper::DarkType) {
            color_BT.setAlphaF(0.75);
            pal.setBrush(DPalette::Text, color_BT);
            pLabelList[1]->setForegroundRole(DPalette::Text);
            pLabelList[1]->setPalette(pal);
        }
    }
}

ThumbnailListView *TimeLineView::getFirstListViewFromTimeline()
{
    if (m_allThumbnailListView.count() > 0)
        return m_allThumbnailListView.at(0);
    else
        return nullptr;
}

void TimeLineView::clearAllSelection()
{
    for (int j = 0; j < m_allThumbnailListView.length(); j++) {
        m_allThumbnailListView[j]->clearSelection();
    }
}

void TimeLineView::updataLayout(QStringList updatePathList)
{
    m_spinner->hide();
    m_spinner->stop();
    if (updatePathList.isEmpty())
        return;
    for (ThumbnailListView *list : m_allThumbnailListView) {
        list->updateThumbnailView(updatePathList.first());
    }
}

void TimeLineView::initTimeLineViewWidget()
{
    m_mainLayout = new QVBoxLayout();
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    pTimeLineViewWidget->setLayout(m_mainLayout);
    DPalette palcolor = DApplicationHelper::instance()->palette(pTimeLineViewWidget);
    palcolor.setBrush(DPalette::Window, palcolor.color(DPalette::Base));
    pTimeLineViewWidget->setPalette(palcolor);
    m_mainListWidget = new TimelineListWidget;
    m_mainListWidget->setFocusPolicy(Qt::NoFocus);
    QScrollBar *pHorizontalBar = m_mainListWidget->horizontalScrollBar();
    pHorizontalBar->setEnabled(false);
    pHorizontalBar->setVisible(false);
    m_mainListWidget->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    m_mainListWidget->verticalScrollBar()->setSingleStep(20);
    m_mainLayout->addWidget(m_mainListWidget);
    m_mainListWidget->setResizeMode(QListWidget::Adjust);
    m_mainListWidget->setFrameShape(DTableView::NoFrame);

    //添加悬浮title
    m_dateItem = new QWidget(pTimeLineViewWidget);
    QVBoxLayout *TitleViewLayout = new QVBoxLayout();
    m_dateItem->setLayout(TitleViewLayout);

    //时间线
    m_pDate = new DLabel();
    DFontSizeManager::instance()->bind(m_pDate, DFontSizeManager::T3, QFont::Medium);
    QFont ft3 = DFontSizeManager::instance()->get(DFontSizeManager::T3);
    ft3.setFamily("SourceHanSansSC");
    ft3.setWeight(QFont::DemiBold);
    DPalette color = DApplicationHelper::instance()->palette(m_pDate);
    color.setBrush(DPalette::Text, color.color(DPalette::ToolTipText));

    m_pDate->setFixedHeight(TIMELINE_TITLEHEIGHT);
    m_pDate->setFont(ft3);
    m_pDate->setForegroundRole(DPalette::Text);
    m_pDate->setPalette(color);

    //时间线下的计数
    pNum_up = new DLabel();
    DFontSizeManager::instance()->bind(pNum_up, DFontSizeManager::T6, QFont::Medium);
    QFont ft6 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft6.setFamily("SourceHanSansSC");
    ft6.setWeight(QFont::Medium);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    DPalette pal = DApplicationHelper::instance()->palette(pNum_up);
    QColor color_BT = pal.color(DPalette::BrightText);
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);
        pNum_up->setForegroundRole(DPalette::Text);
        pNum_up->setPalette(pal);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
        pNum_up->setForegroundRole(DPalette::Text);
        pNum_up->setPalette(pal);
    }

    pNum_up->setFixedHeight(TIMELINE_TITLEHEIGHT);
    pNum_up->setFont(ft6);
    pNum_up->setForegroundRole(DPalette::Text);
    pNum_up->setPalette(pal);

    TitleViewLayout->addWidget(m_pDate);
    TitleViewLayout->addWidget(pNum_up);

    //右侧选择文字
    QHBoxLayout *Layout = new QHBoxLayout();
    pSuspensionChose = new DCommandLinkButton(QObject::tr("Select"));
    pSuspensionChose->setFocusPolicy(Qt::NoFocus);
    AC_SET_OBJECT_NAME(pSuspensionChose, Time_Line_Choose_Button);
    AC_SET_ACCESSIBLE_NAME(pSuspensionChose, Time_Line_Choose_Button);
    DFontSizeManager::instance()->bind(pSuspensionChose, DFontSizeManager::T5);
    pSuspensionChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    pSuspensionChose->setFixedHeight(TIMELINE_TITLEHEIGHT);
    pSuspensionChose->resize(36, 27);

    pNum_up->setLayout(Layout);
    Layout->addStretch(1);
    Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    Layout->setContentsMargins(0, 0, 12, 0);
    Layout->addWidget(pSuspensionChose);
    connect(pSuspensionChose, &DCommandLinkButton::clicked, this, &TimeLineView::on_DCommandLinkButton);

    DPalette ppal_light = DApplicationHelper::instance()->palette(m_dateItem);
    ppal_light.setBrush(DPalette::Background, ppal_light.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light = new QGraphicsOpacityEffect;
    opacityEffect_light->setOpacity(0.95);
    m_dateItem->setPalette(ppal_light);
    m_dateItem->setGraphicsEffect(opacityEffect_light);
    m_dateItem->setAutoFillBackground(true);
    m_dateItem->setFixedSize(this->width() - 10, 87);
    m_dateItem->setContentsMargins(10, 0, 0, 0);
    m_dateItem->move(0, TITLEHEIGHT);
    m_dateItem->show();
    m_dateItem->setVisible(false);
}

void TimeLineView::updateStackedWidget()
{
    if (0 < DBManager::instance()->getImgsCount()) {
        m_pStackedWidget->setCurrentIndex(VIEW_TIMELINE);
    } else {
        m_pStackedWidget->setCurrentIndex(VIEW_IMPORT);
    }
}

int TimeLineView::getIBaseHeight()
{
    if (m_pStatusBar->m_pSlider == nullptr) {
        return 0;
    }

    int value = m_pStatusBar->m_pSlider->value();
    switch (value) {
    case 0:
        return 80;
    case 1:
        return 90;
    case 2:
        return 100;
    case 3:
        return 110;
    case 4:
        return 120;
    case 5:
        return 130;
    case 6:
        return 140;
    case 7:
        return 150;
    case 8:
        return 160;
    case 9:
        return 170;
    default:
        return 80;
    }
}

void TimeLineView::clearAndStop()
{
    for (ThumbnailListView *list : m_allThumbnailListView) {
        list->stopLoadAndClear();
        delete list;
    }
    m_mainListWidget->clear();
    m_allThumbnailListView.clear();
    m_allChoseButton.clear();
    currentTimeLineLoad = 0;

}

void TimeLineView::clearAndStartLayout()
{
    m_spinner->hide();
    m_spinner->stop();
    for (ThumbnailListView *list : m_allThumbnailListView) {
        list->stopLoadAndClear();
        delete list;
    }
    m_mainListWidget->clear();
    m_allThumbnailListView.clear();
    m_allChoseButton.clear();
    currentTimeLineLoad = 0;
    //获取所有时间线
    m_timelines = DBManager::instance()->getAllTimelines();
//    updataLayout();
    addTimelineLayout();
}

void TimeLineView::onFinishLoad()
{
    m_mainListWidget->update();
}

void TimeLineView::onNewTime(const QString &date, const QString &num, int index)
{
    m_index = index;
    on_AddLabel(date, num);
}

void TimeLineView::onImportViewImportBtnClicked()
{
    emit dApp->signalM->startImprot();
    pImportView->onImprotBtnClicked();
}

void TimeLineView::onImportFailedToView()
{
    if (isVisible()) {
        m_spinner->hide();
        m_spinner->stop();
        updateStackedWidget();
    }
}

void TimeLineView::onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, const QString &albumName)
{
    Q_UNUSED(importPaths)
    // 导入的照片重复照片提示
    if (duplicatePaths.size() > 0 && albumName.length() < 1 && dApp->getMainWindow()->getCurrentViewType() == 1) {
        QTimer::singleShot(100, this, [ = ] {
            for (ThumbnailListView *list : m_allThumbnailListView)
            {
                // 注意时间线界面为多行listview处理类型
                list->selectDuplicatePhotos(duplicatePaths, true);
            }
        });
    }
}

void TimeLineView::addTimelineLayout()
{
    if (currentTimeLineLoad >= m_timelines.size()) {
        updateStackedWidget();
        return;
    }
    int nowTimeLineLoad = currentTimeLineLoad;
    //获取当前时间照片
    DBImgInfoList ImgInfoList = DBManager::instance()->getInfosByTimeline(m_timelines.at(nowTimeLineLoad));

    QListWidgetItem *item = new QListWidgetItem;
    TimelineItem *listItem = new TimelineItem(this);
    listItem->adjustSize();
    QVBoxLayout *listItemlayout = new QVBoxLayout();
    listItem->setLayout(listItemlayout);
    listItemlayout->setMargin(0);
    listItemlayout->setSpacing(0);
    listItemlayout->setContentsMargins(0, 0, 0, 0);

    //添加title
    QWidget *TitleView = new QWidget;
    QVBoxLayout *TitleViewLayout = new QVBoxLayout();
    TitleView->setLayout(TitleViewLayout);
    DLabel *pDate = new DLabel();
    DFontSizeManager::instance()->bind(pDate, DFontSizeManager::T3, QFont::DemiBold);


    pDate->setFixedHeight(TIMELINE_TITLEHEIGHT);
    QStringList datelist = m_timelines.at(nowTimeLineLoad).split(".");
    if (datelist.count() > 2) {
        listItem->m_sdate = QString(QObject::tr("%1/%2/%3")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]);
    }
    pDate->setText(listItem->m_sdate);

    DPalette color = DApplicationHelper::instance()->palette(pDate);
    color.setBrush(DPalette::Text, color.color(DPalette::ToolTipText));

    QFont ft3 = DFontSizeManager::instance()->get(DFontSizeManager::T3);
    ft3.setFamily("SourceHanSansSC");
    ft3.setWeight(QFont::DemiBold);

    pDate->setFont(ft3);
    pDate->setForegroundRole(DPalette::Text);
    pDate->setPalette(color);

    listItem->m_date = pDate;

    pNum_dn = new DLabel();
    listItem->m_snum = QString(QObject::tr("%1 photo(s)")).arg(ImgInfoList.size());
    DFontSizeManager::instance()->bind(pNum_dn, DFontSizeManager::T6, QFont::Medium);
    pNum_dn->setText(listItem->m_snum);

    QFont ft6 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft6.setFamily("SourceHanSansSC");
    ft6.setWeight(QFont::Medium);
    DPalette pal = DApplicationHelper::instance()->palette(pNum_dn);
    QColor color_BT = pal.color(DPalette::BrightText);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);
        pNum_dn->setForegroundRole(DPalette::Text);
        pNum_dn->setPalette(pal);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
        pNum_dn->setForegroundRole(DPalette::Text);
        pNum_dn->setPalette(pal);
    }

    pNum_dn->setFixedHeight(TIMELINE_TITLEHEIGHT);
    pNum_dn->setFont(ft6);

    QHBoxLayout *Layout = new QHBoxLayout();
    DCommandLinkButton *pChose = new DCommandLinkButton(QObject::tr("Select"));
    pChose->setFocusPolicy(Qt::NoFocus);
    DFontSizeManager::instance()->bind(pChose, DFontSizeManager::T5);
    m_allChoseButton << pChose;
    pChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    pChose->setFixedHeight(TIMELINE_TITLEHEIGHT);
    pChose->resize(36, 27);

    pNum_dn->setLayout(Layout);
    Layout->addStretch(1);
    Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    Layout->setContentsMargins(0, 0, 37, 0);
    Layout->addWidget(pChose);

    listItem->m_Chose = pChose;
    listItem->m_num = pNum_dn;
    TitleViewLayout->addWidget(pDate);
    TitleViewLayout->addWidget(pNum_dn);
    TitleView->setFixedHeight(87);
    listItem->m_title = TitleView;

    //添加照片
    ThumbnailListView *pThumbnailListView = new ThumbnailListView(ThumbnailDelegate::TimeLineViewType, COMMON_STR_VIEW_TIMELINE);
    pThumbnailListView->setFocusPolicy(Qt::NoFocus);
    int m_Baseheight =  getIBaseHeight();
    if (m_Baseheight == 0) {
        return;
    } else {
        pThumbnailListView->setIBaseHeight(m_Baseheight);
    }
    pThumbnailListView->setFixedWidth(width() + 2);
    connect(pThumbnailListView, &ThumbnailListView::loadEnd, this, &TimeLineView::addTimelineLayout);
    connect(pThumbnailListView, &ThumbnailListView::needResize, this, [ = ](int h) {
        if (!pThumbnailListView->checkResizeNum())
            return ;
        if (isVisible()) {
            int mh = h;
            if (0 == nowTimeLineLoad) {
                mh += 50;
            }
            if (nowTimeLineLoad == m_timelines.size() - 1) {
                mh += 27;
            }
            pThumbnailListView->setFixedHeight(mh);
            listItem->setFixedHeight(TitleView->height() + mh);
            item->setSizeHint(listItem->rect().size());
        }
    });

#if 1
    m_allThumbnailListView.append(pThumbnailListView);
#endif
    pThumbnailListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pThumbnailListView->setContextMenuPolicy(Qt::CustomContextMenu);
    pThumbnailListView->setContentsMargins(0, 0, 0, 0);
    pThumbnailListView->setFrameShape(DTableView::NoFrame);
    pThumbnailListView->loadFilesFromLocal(ImgInfoList);
    if (0 == nowTimeLineLoad) {
        DWidget *topwidget = new DWidget;
        topwidget->setFixedHeight(50);
        listItemlayout->addWidget(topwidget);
    }
    listItemlayout->addWidget(TitleView);
    listItemlayout->addWidget(pThumbnailListView);
    if (nowTimeLineLoad == m_timelines.size() - 1) {
        DWidget *bottomwidget = new DWidget;
        bottomwidget->setFixedHeight(27);
        listItemlayout->addWidget(bottomwidget);
    }
    item->setFlags(Qt::NoItemFlags);
    m_mainListWidget->addItemForWidget(item);
    m_mainListWidget->setItemWidget(item, listItem);
    connect(pThumbnailListView, &ThumbnailListView::openImage, this, [ = ](int index) {
        SignalManager::ViewInfo info;
        info.album = "";
        info.lastPanel = nullptr;
        if (ImgInfoList.size() <= 1) {
            info.paths.clear();
        } else {
            for (auto image : ImgInfoList) {
                info.paths << image.filePath;
            }
        }
        info.path = ImgInfoList[index].filePath;
        info.viewType = utils::common::VIEW_TIMELINE_SRN;
        info.viewMainWindowID = VIEW_MAINWINDOW_TIMELINE;
        emit dApp->signalM->viewImage(info);
        emit dApp->signalM->showImageView(VIEW_MAINWINDOW_TIMELINE);
    });
    connect(pThumbnailListView, &ThumbnailListView::menuOpenImage, this, [ = ](QString path, QStringList paths, bool isFullScreen, bool isSlideShow) {
        SignalManager::ViewInfo info;
        info.album = "";
        info.lastPanel = nullptr;
        if (paths.size() > 1) {
            info.paths = paths;
        } else {
            auto photolist = pThumbnailListView->getAllFileList();
            if (photolist.size() > 1) {
                for (auto image : photolist) {
                    info.paths << image;
                }
            } else {
                info.paths.clear();
            }
        }
        info.path = path;
        info.fullScreen = isFullScreen;
        info.slideShow = isSlideShow;
        info.viewType = utils::common::VIEW_TIMELINE_SRN;
        info.viewMainWindowID = VIEW_MAINWINDOW_TIMELINE;
        if (info.slideShow) {
            if (ImgInfoList.count() == 1) {
                info.paths = paths;
            }

            QStringList pathlist;
            pathlist.clear();
            for (auto path : info.paths) {
                if (QFileInfo(path).exists()) {
                    pathlist << path;
                }
            }

            info.paths = pathlist;
            emit dApp->signalM->startSlideShow(info);
            emit dApp->signalM->showSlidePanel(VIEW_MAINWINDOW_TIMELINE);
        } else {
            emit dApp->signalM->viewImage(info);
            emit dApp->signalM->showImageView(VIEW_MAINWINDOW_TIMELINE);
        }
    });
    connect(pChose, &DCommandLinkButton::clicked, this, [ = ]() {
        if (QObject::tr("Select") == pChose->text()) {
            pChose->setText(QObject::tr("Unselect"));
            for (int j = 0; j < m_allChoseButton.length(); j++) {
                if (pChose == m_allChoseButton[j])
                    lastClickedIndex = j;
            }
            lastRow = 0;
            lastChanged = true;
            pThumbnailListView->selectAll();
#ifdef tablet_PC
            pThumbnailListView->m_isSelectAllBtn = true;
            pThumbnailListView->setSelectionMode(QAbstractItemView::MultiSelection);
#endif
        } else {
            pChose->setText(QObject::tr("Select"));
            pThumbnailListView->clearSelection();
#ifdef tablet_PC
            pThumbnailListView->m_isSelectAllBtn = false;
            pThumbnailListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
#endif
        }
        updatePicNum();
    });
#if 1
    connect(pThumbnailListView, &ThumbnailListView::sigGetSelectedPaths, this, &TimeLineView::on_GetSelectedPaths);

    connect(pThumbnailListView, &ThumbnailListView::sigMousePress, this, [ = ](QMouseEvent * event) {
        lastRow = -1;
        if (event->button() == Qt::LeftButton) {
            for (int j = 0; j < m_allThumbnailListView.length(); j++) {
                if (pThumbnailListView != m_allThumbnailListView[j]) {
                    m_allThumbnailListView[j]->clearSelection();
                }
            }
            m_ctrlPress = false;
        }

        for (int j = 0; j < m_allThumbnailListView.length(); j++) {
            if (pThumbnailListView == m_allThumbnailListView[j]) {
                lastClickedIndex = j;
                lastRow = pThumbnailListView->getRow(QPoint(event->x(), event->y()));
                if (-1 != lastRow)
                    lastChanged = true;
            }
        }
    });

    connect(pThumbnailListView, &ThumbnailListView::sigCtrlMousePress, this, [ = ](QMouseEvent * event) {
        m_ctrlPress = true;
        for (int j = 0; j < m_allThumbnailListView.length(); j++) {
            if (pThumbnailListView == m_allThumbnailListView[j]) {
                lastClickedIndex = j;
                lastRow = pThumbnailListView->getRow(QPoint(event->x(), event->y()));
                if (-1 != lastRow)
                    lastChanged = true;
            }
        }
    });

    connect(pThumbnailListView, &ThumbnailListView::sigShiftMousePress, this, [ = ](QMouseEvent * event) {
        int curClickedIndex = -1;
        int curRow = -1;
        for (int j = 0; j < m_allThumbnailListView.length(); j++) {
            if (pThumbnailListView == m_allThumbnailListView[j]) {
                curClickedIndex = j;
                curRow = pThumbnailListView->getRow(QPoint(event->x(), event->y()));
            }
        }
        if (!lastChanged && -1 != curRow) {
            for (int j = 0; j < m_allThumbnailListView.length(); j++) {
                m_allThumbnailListView[j]->clearSelection();
            }
        }
        if (curRow == -1 || lastRow == -1) {
            for (int j = 0; j < m_allThumbnailListView.length(); j++) {
                m_allThumbnailListView[j]->clearSelection();
            }
        } else {
            if (lastClickedIndex < curClickedIndex) {
                m_allThumbnailListView[lastClickedIndex]->selectRear(lastRow);
                m_allThumbnailListView[curClickedIndex]->selectFront(curRow);
                for (int k = lastClickedIndex + 1; k < curClickedIndex; k++) {
                    m_allThumbnailListView[k]->selectAll();
                }
            } else if (lastClickedIndex > curClickedIndex) {
                m_allThumbnailListView[lastClickedIndex]->selectFront(lastRow);
                m_allThumbnailListView[curClickedIndex]->selectRear(curRow);
                for (int k = curClickedIndex + 1; k < lastClickedIndex; k++) {
                    m_allThumbnailListView[k]->selectAll();
                }
            } else if (lastClickedIndex == curClickedIndex) {
                if (lastRow <= curRow)
                    pThumbnailListView->selectExtent(lastRow, curRow);
                else
                    pThumbnailListView->selectExtent(curRow, lastRow);
            }
            updatePicNum();
            updateChoseText();
            curRow = -1;
            lastChanged = false;
        }
    });

    connect(pThumbnailListView, &ThumbnailListView::sigSelectAll, this, [ = ]() {
        m_ctrlPress = true;
        for (int j = 0; j < m_allThumbnailListView.length(); j++) {
            m_allThumbnailListView[j]->selectAll();
        }
        updatePicNum();
        updateChoseText();
        QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
        pSuspensionChose->setText(b[0]->text());
    });

    connect(pThumbnailListView, &ThumbnailListView::sigMouseRelease, this, [ = ]() {
        if (!m_ctrlPress) {
            for (int j = 0; j < m_allThumbnailListView.length(); j++) {
                if (pThumbnailListView != m_allThumbnailListView[j]) {
                    m_allThumbnailListView[j]->clearSelection();
                }
            }
        }

        updatePicNum();
        updateChoseText();
        QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
        pSuspensionChose->setText(b[0]->text());
    });

    connect(pThumbnailListView, &ThumbnailListView::customContextMenuRequested, this, [ = ]() {
#ifdef tablet_PC
        return;
#endif
        QStringList paths = pThumbnailListView->selectedPaths();
        if (pThumbnailListView->model()->rowCount() == paths.length() && QObject::tr("Select") == pChose->text()) {
            pChose->setText(QObject::tr("Unselect"));
        }

        if (pThumbnailListView->model()->rowCount() != paths.length() && QObject::tr("Unselect") == pChose->text()) {
            pChose->setText(QObject::tr("Select"));
        }
        updatePicNum();
    });
    connect(pThumbnailListView, &ThumbnailListView::sigMenuItemDeal, this, [ = ](QAction * action) {
        QStringList paths;
        paths.clear();
        for (int j = 0; j < m_allThumbnailListView.size(); j++) {
            paths << m_allThumbnailListView[j]->selectedPaths();
        }
        pThumbnailListView->menuItemDeal(paths, action);
    });

    connect(listItem, &TimelineItem::sigMousePress, this, [ = ] {
        for (int j = 0; j < m_allThumbnailListView.length(); j++)
        {
            m_allThumbnailListView[j]->clearSelection();
        }
        lastRow = -1;
        updatePicNum();
        updateChoseText();
    });
    connect(pThumbnailListView, &ThumbnailListView::sigMoveToTrash, this, &TimeLineView::onKeyDelete);//跳转
#endif

    connect(m_allThumbnailListView[nowTimeLineLoad], &ThumbnailListView::sigKeyEvent, this, &TimeLineView::on_KeyEvent);
    connect(m_allThumbnailListView[nowTimeLineLoad], &ThumbnailListView::sigNeedMoveScorll, this, &TimeLineView::on_MoveScroll);

    if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
    } else {
        updateStackedWidget();
    }

    updatePicNum();
    currentTimeLineLoad++;

    //判断跳转位置图片是否包含在当前listview
    if (selectPrePaths.length() > 0) {
        for (DBImgInfo &imgInfo : ImgInfoList) {
            if (imgInfo.filePath == selectPrePaths) {
                hasPicView = nowTimeLineLoad;
                isFindPic = true;
                break;
            }
        }
    }

    if (nowTimeLineLoad == m_timelines.size() - 1 && hasPicView >= 0) {
        int height = 0;
        for (int i = 0; i < m_timelines.size(); i++) {
            if (i < hasPicView) {
                height = height + m_allThumbnailListView[i]->height() + m_allChoseButton[0]->height();
            } else if (i == hasPicView) {
                for (int j = 0; j < m_allThumbnailListView[i]->count(); j ++) {
                    QModelIndex index = m_allThumbnailListView[i]->m_model->index(j, 0);
                    QVariantList lst = index.model()->data(index, Qt::DisplayRole).toList();
                    int rowcount = 0;
                    int allrowcount = 0;
                    if (lst.count() >= 12) {
                        QString path = lst.at(1).toString();
                        if (path == selectPrePaths) {
                            if ((index.row() % m_allThumbnailListView[i]->m_rowSizeHint) == 0) {
                                rowcount = index.row() / m_allThumbnailListView[i]->m_rowSizeHint;
                            } else {
                                rowcount = index.row() / m_allThumbnailListView[i]->m_rowSizeHint + 1;
                            }
                            if ((m_allThumbnailListView[i]->m_model->rowCount() % m_allThumbnailListView[i]->m_rowSizeHint) == 0) {
                                allrowcount = m_allThumbnailListView[i]->m_model->rowCount() / m_allThumbnailListView[i]->m_rowSizeHint ;
                            } else {
                                allrowcount = m_allThumbnailListView[i]->m_model->rowCount() / m_allThumbnailListView[i]->m_rowSizeHint + 1;
                            }
                            double tempheight = rowcount / static_cast<double>(allrowcount);
                            int thumbnailheight =  m_allThumbnailListView.at(i)->height();
                            double finalheight = tempheight * thumbnailheight;
                            height = height + static_cast<int>(finalheight) ;

                            if (hasPicView == 0)
                                height = height - m_allThumbnailListView[i]->m_onePicWidth;
                            break;
                        }
                    }
                }
                break;
            }
        }
//        m_mainListWidget->setCurrentRow(hasPicView);
        m_mainListWidget->verticalScrollBar()->setValue(height);
        hasPicView = -1;
        selectPrePaths = "";
        isFindPic = false;
    }
}

void TimeLineView::on_AddLabel(QString date, QString num)
{
    if ((nullptr != m_dateItem) && (nullptr != m_mainListWidget)) {
        QList<QLabel *> labelList = m_dateItem->findChildren<QLabel *>();
        labelList[0]->setText(date);
        labelList[1]->setText(num);
        m_dateItem->setVisible(true);
        m_dateItem->move(0, TITLEHEIGHT);
    }
#if 1
    QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
    pSuspensionChose->setText(b[0]->text());
#endif
}

void TimeLineView::on_DCommandLinkButton()
{
    if (QObject::tr("Select") == pSuspensionChose->text()) {
        pSuspensionChose->setText(QObject::tr("Unselect"));
        QList<ThumbnailListView *> p = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<ThumbnailListView *>();
        if (p.size() > 0) {
            p[0]->selectAll();
#ifdef tablet_PC
            p[0]->m_isSelectAllBtn = true;
            p[0]->setSelectionMode(QAbstractItemView::MultiSelection);
#endif
        }
        updatePicNum();
        for (int i = 0; i < m_allChoseButton.length(); i++) {
            if (m_allThumbnailListView[i] == p[0]) {
                lastClickedIndex = i;
                lastRow = 0;
                lastChanged = true;
            }
        }
        m_ctrlPress = true;
    } else {
        pSuspensionChose->setText(QObject::tr("Select"));
        QList<ThumbnailListView *> p = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<ThumbnailListView *>();
        if (p.size() > 0) {
            p[0]->clearSelection();
#ifdef tablet_PC
            p[0]->m_isSelectAllBtn = false;
            p[0]->setSelectionMode(QAbstractItemView::ExtendedSelection);
#endif
            updatePicNum();
        }
    }
#if 1
    QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
    if (b.size() > 0)
        b[0]->setText(pSuspensionChose->text());
#endif
}

void TimeLineView::on_GetSelectedPaths(QStringList *pPaths)
{
    pPaths->clear();
    for (int j = 0; j < m_allThumbnailListView.size(); j++) {
        pPaths->append(m_allThumbnailListView[j]->selectedPaths());
    }
}

#if 1
void TimeLineView::on_MoveLabel(int y, const QString &date, const QString &num, const QString &choseText)
#endif
{
    if ((nullptr != m_dateItem) && (nullptr != m_mainListWidget)) {
        QList<QLabel *> labelList = m_dateItem->findChildren<QLabel *>();
        labelList[0]->setText(date);
        labelList[1]->setText(num);
        pSuspensionChose->setText(choseText);
        m_dateItem->setVisible(true);
//        m_dateItem->move(0, TITLEHEIGHT + y + 1);
        Q_UNUSED(y)
    }
}

void TimeLineView::on_KeyEvent(int key)
{
    qDebug() << key;

    if (key == Qt::Key_PageDown) {
        QScrollBar *vb = m_mainListWidget->verticalScrollBar();
        int posValue = vb->value();
        qDebug() << "posValue" << posValue;

        posValue += m_mainListWidget->height();
        vb->setValue(posValue);
    } else if (key == Qt::Key_PageUp) {
        QScrollBar *vb = m_mainListWidget->verticalScrollBar();
        int posValue = vb->value();
        qDebug() << "posValue" << posValue;

        posValue -= m_mainListWidget->height();
        vb->setValue(posValue);
    }

}

void TimeLineView::on_MoveScroll(int distance)
{
    auto scroll = m_mainListWidget->verticalScrollBar();
    scroll->setValue(scroll->value() + distance);
}

void TimeLineView::resizeEvent(QResizeEvent *ev)
{
    Q_UNUSED(ev);
    m_spinner->move(width() / 2 - 20, (height() - 50) / 2 - 20);
    m_dateItem->setFixedSize(width() - 15, 87);
    for (int i = 0; i < m_allThumbnailListView.length(); i++) {
        m_allThumbnailListView[i]->setFixedWidth(width() + 2);
        QList<DLabel *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(i))->findChildren<DLabel *>();
        b[1]->setFixedWidth(width() - 14);
    }
    m_pwidget->setFixedSize(this->width(), this->height() - 23);
    m_pwidget->move(0, 0);
    m_pStatusBar->setFixedWidth(this->width());
    m_pStatusBar->move(0, this->height() - m_pStatusBar->height());
    fatherwidget->setFixedSize(this->size());
}

void TimeLineView::dragEnterEvent(QDragEnterEvent *e)
{
    const QMimeData *mimeData = e->mimeData();
    if (!utils::base::checkMimeData(mimeData)) {
        return;
    }
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void TimeLineView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    ImageEngineApi::instance()->ImportImagesFromUrlList(urls, nullptr, this);
    event->accept();
}

void TimeLineView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void TimeLineView::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_UNUSED(e);
}

void TimeLineView::mousePressEvent(QMouseEvent *e)
{
    if (QApplication::keyboardModifiers() != Qt::ControlModifier && e->button() == Qt::LeftButton) {
        for (int i = 0; i < m_allThumbnailListView.length(); i++) {
            m_allThumbnailListView[i]->clearSelection();
        }
        updatePicNum();
        updateChoseText();
    }
    DWidget::mousePressEvent(e);
}

void TimeLineView::updatePicNum()
{
    QString str = QObject::tr("%1 photo(s) selected");

    if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
        QStringList paths = pSearchView->m_pThumbnailListView->selectedPaths();
        m_selPicNum = paths.length();
        m_pStatusBar->m_pAllPicNumLabel->setText(str.arg(m_selPicNum));
    } else {
        allnum = 0;

        for (int i = 0; i < m_allThumbnailListView.size(); i++) {
            allnum += m_allThumbnailListView[i]->selectedPaths().size();
        }

        if (0 == allnum) {
            restorePicNum();
        } else {
            QString str1 = QObject::tr("%1 photo(s) selected");
            m_pStatusBar->m_pAllPicNumLabel->setText(str1.arg(allnum));
        }
    }
}

void TimeLineView::updateChoseText()
{
#ifdef tablet_PC
    return;
#endif
    for (int i = 0; i < m_allChoseButton.length(); i++) {
        if (m_allThumbnailListView[i]->model()->rowCount() == m_allThumbnailListView[i]->selectedPaths().length() && QObject::tr("Select") == m_allChoseButton[i]->text()) {
            m_allChoseButton[i]->setText(QObject::tr("Unselect"));
        }

        if (m_allThumbnailListView[i]->model()->rowCount() != m_allThumbnailListView[i]->selectedPaths().length() && QObject::tr("Unselect") == m_allChoseButton[i]->text()) {
            m_allChoseButton[i]->setText(QObject::tr("Select"));
        }
    }
}

void TimeLineView::restorePicNum()
{
    QString str = QObject::tr("%1 photo(s)");
    int selPicNum = 0;

    if (VIEW_TIMELINE == m_pStackedWidget->currentIndex()) {
        selPicNum = DBManager::instance()->getImgsCount();

    } else if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
        selPicNum = pSearchView->m_searchPicNum;
    }

    m_pStatusBar->m_pAllPicNumLabel->setText(str.arg(QString::number(selPicNum)));
}

void TimeLineView::onKeyDelete()
{
    if (!isVisible()) return;
    if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) return;

    QStringList paths;
    paths.clear();

    bool bDeleteAll = true;
    //获取当前所有选中的
    for (int i = 0; i < m_allThumbnailListView.size(); i++) {

        paths << m_allThumbnailListView[i]->selectedPaths();
        QStringList currentPaths;
        currentPaths << m_allThumbnailListView[i]->selectedPaths();
        bDeleteAll = m_allThumbnailListView[i]->isAllPicSeleted();
        //改为跳转到最后一张选中位置
        if (/*first && */currentPaths.length() > 0) {
            if (!bDeleteAll) {
                selectPrePaths = m_allThumbnailListView[i]->m_model->index(0, 0).data().toList().at(1).toString();
                int index = 1;
                while (paths.contains(selectPrePaths)) {
                    selectPrePaths = m_allThumbnailListView[i]->m_model->index(index, 0).data().toList().at(1).toString();
                    index ++ ;
                    if (index == m_allThumbnailListView[i]->m_model->rowCount() - 1)
                        break;
                }
            } else {
                if (i > 1) {
                    selectPrePaths = m_allThumbnailListView[i - 1]->m_model->index(0, 0).data().toList().at(1).toString();
                } else {
                    selectPrePaths = "";
                }
            }
        }
    }
    if (0 >= paths.length()) {
        return;
    }

    if (bDeleteAll) {
        m_pStackedWidget->setCurrentIndex(VIEW_IMPORT);
    }

    ImageEngineApi::instance()->moveImagesToTrash(paths);
}
