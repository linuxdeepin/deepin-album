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
#include "importtimelineview.h"
#include "mainwindow.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "imageengine/imageengineapi.h"
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

#include <QTimer>

namespace  {
const int SUBTITLE_HEIGHT = 37;
const int VIEW_MAINWINDOW_ALBUM = 2;
} //namespace

ImportTimeLineView::ImportTimeLineView(DWidget *parent)
    : DWidget(parent), m_mainLayout(nullptr), m_dateItem(nullptr)
    , pSuspensionChose(nullptr), pTimeLineViewWidget(nullptr), pImportView(nullptr)
    , allnum(0), m_pDate(nullptr), pNum_up(nullptr)
    , pNum_dn(nullptr), m_pImportTitle(nullptr), m_DSlider(nullptr)
    , m_oe(nullptr), m_oet(nullptr), m_ctrlPress(false)
    , lastClickedIndex(-1), lastRow(-1), m_lastShiftRow(-1)
    , m_lastShiftClickedIndex(-1), lastChanged(false), m_iBaseHeight(0)
    , m_index(0), m_mainListWidget(nullptr), currentTimeLineLoad(0)
{
    setAcceptDrops(true);
    m_oe = new QGraphicsOpacityEffect(this);
    m_oet = new QGraphicsOpacityEffect(this);
    m_oe->setOpacity(0.5);
    m_oet->setOpacity(0.75);

    pTimeLineViewWidget = new DWidget();
    QVBoxLayout *pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins(0, 0, 0, 0);
    pVBoxLayout->addWidget(pTimeLineViewWidget);
    this->setLayout(pVBoxLayout);
    initTimeLineViewWidget();
    initConnections();
}

int ImportTimeLineView::getIBaseHeight()
{
    if (m_DSlider == nullptr) {
        return 0;
    }

    int value = m_DSlider->value();
    switch (value) {
    case 0:
        return  80;
    case 1:
        return  90;
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

void ImportTimeLineView::getCurrentSelectPics()
{
    bool bDeleteAll = false;
    bool first = true;
    QStringList paths;

    for (int i = 0; i < m_allThumbnailListView.size(); i++) {
        paths << m_allThumbnailListView[i]->selectedPaths();
        bDeleteAll = m_allThumbnailListView[i]->isAllPicSeleted();
        if (first && paths.length() > 0) {
            if (!bDeleteAll) {
                selectPrePaths = m_allThumbnailListView[i]->m_model->index(m_allThumbnailListView[i]->m_timeLineSelectPrePic, 0).data().toList().at(1).toString();
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
            first = false;
        }
    }
}


void ImportTimeLineView::initConnections()
{
    connect(m_mainListWidget, &TimelineListWidget::sigNewTime, this, &ImportTimeLineView::onNewTime);
//    connect(m_mainListWidget, &TimelineListWidget::sigDelTime, this, &ImportTimeLineView::on_DelLabel);
    connect(m_mainListWidget, &TimelineListWidget::sigMoveTime, this, &ImportTimeLineView::on_MoveLabel);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &ImportTimeLineView::themeChangeSlot);
    // 重复导入图片选中
    connect(dApp->signalM, &SignalManager::RepeatImportingTheSamePhotos, this, &ImportTimeLineView::onRepeatImportingTheSamePhotos);
}

void ImportTimeLineView::themeChangeSlot(DGuiApplicationHelper::ColorType themeType)
{
//    Q_UNUSED(themeType);
    DPalette palcolor = DApplicationHelper::instance()->palette(pTimeLineViewWidget);
    palcolor.setBrush(DPalette::Base, palcolor.color(DPalette::Window));
    pTimeLineViewWidget->setPalette(palcolor);

    DPalette pa1 = DApplicationHelper::instance()->palette(m_dateItem);
    pa1.setBrush(DPalette::Background, pa1.color(DPalette::Base));
    m_dateItem->setForegroundRole(DPalette::Background);
    m_dateItem->setPalette(pa1);

    //add start 3975
    DPalette ppal_light2 = DApplicationHelper::instance()->palette(m_pImportTitle);
    ppal_light2.setBrush(DPalette::Background, ppal_light2.color(DPalette::Base));
    m_pImportTitle->setPalette(ppal_light2);
    //add end 3975
//    DPalette pa = DApplicationHelper::instance()->palette(m_pDate);
//    pa.setBrush(DPalette::Text, pa.color(DPalette::ToolTipText));
//    m_pDate->setForegroundRole(DPalette::Text);
//    m_pDate->setPalette(pa);

    DPalette pal1 = DApplicationHelper::instance()->palette(pNum_up);
    QColor color_BT1 = pal1.color(DPalette::BrightText);
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT1.setAlphaF(0.5);
        pal1.setBrush(DPalette::Text, color_BT1);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT1.setAlphaF(0.75);
        pal1.setBrush(DPalette::Text, color_BT1);
    }
    pNum_up->setForegroundRole(DPalette::Text);
    m_pDate->setForegroundRole(DPalette::Text);
    m_pDate->setPalette(pal1);
    pNum_up->setPalette(pal1);

    for (int i = 1; i < m_mainListWidget->count(); i++) {
        TimelineItem *item = static_cast<TimelineItem *>(m_mainListWidget->itemWidget(m_mainListWidget->item(i)));
        QList<DLabel *> pLabelList = item->findChildren<DLabel *>();
//        DPalette color = DApplicationHelper::instance()->palette(pLabelList[0]);
//        color.setBrush(DPalette::Text, color.color(DPalette::ToolTipText));
//        pLabelList[0]->setForegroundRole(DPalette::Text);
//        pLabelList[0]->setPalette(color);
        if (pLabelList.size() < 2) {
            break;
        }
        DPalette pal = DApplicationHelper::instance()->palette(pLabelList[1]);
        QColor color_BT = pal.color(DPalette::BrightText);
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            color_BT.setAlphaF(0.5);
            pal.setBrush(DPalette::Text, color_BT);
        } else if (themeType == DGuiApplicationHelper::DarkType) {
            color_BT.setAlphaF(0.75);
            pal.setBrush(DPalette::Text, color_BT);
        }
        pLabelList[1]->setForegroundRole(DPalette::Text);
        pLabelList[0]->setForegroundRole(DPalette::Text);
        pLabelList[0]->setPalette(pal);
        pLabelList[1]->setPalette(pal);
    }
}

void ImportTimeLineView::resizeHand()
{
    for (ThumbnailListView *list : m_allThumbnailListView) {
        list->resizeHand();
    }
}

ThumbnailListView *ImportTimeLineView::getFirstListView()
{
    if (m_allThumbnailListView.count() > 0)
        return m_allThumbnailListView.at(0);
    else
        return nullptr;
}

void ImportTimeLineView::updateSize()
{
    for (int i = 0; i < m_allThumbnailListView.length(); i++) {
        m_allThumbnailListView[i]->setFixedWidth(width() + 2);
        ThumbnailListView *view = m_allThumbnailListView[i];
        emit view->needResizeLabel();
    }
    m_dateItem->setFixedSize(width() - 15, SUBTITLE_HEIGHT);
    m_pImportTitle->setFixedSize(width() - 15, 47); //add 3
}

void ImportTimeLineView::onNewTime(QString date, QString num, int index)
{
    m_index = index;
    on_AddLabel(date, num);
}

void ImportTimeLineView::onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, QString albumName)
{
    Q_UNUSED(importPaths)
    // 导入的照片重复照片提示
    if (duplicatePaths.size() > 0 && albumName.length() < 1 && dApp->getMainWindow()->getCurrentViewType() == 2) {
        QTimer::singleShot(100, this, [ = ] {
            for (ThumbnailListView *list : m_allThumbnailListView)
            {
                // 注意导入界面为多行listview处理类型
                list->selectDuplicatePhotos(duplicatePaths, true);
            }
        });
    }
}

void ImportTimeLineView::onSuspensionChoseBtnClicked()
{
    if (QObject::tr("Select") == pSuspensionChose->text()) {
        pSuspensionChose->setText(QObject::tr("Unselect"));
        QList<ThumbnailListView *> p = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<ThumbnailListView *>();
        if (p.size() > 0) {
            p[0]->selectAll();
            emit sigUpdatePicNum();
        }
        for (int i = 0; i < m_allChoseButton.length(); i++) {
            if (m_allThumbnailListView[i] == p[0]) {
                lastClickedIndex = i;
                lastRow = 0;
                lastChanged = true;
            }
        }
    } else {
        pSuspensionChose->setText(QObject::tr("Select"));
        QList<ThumbnailListView *> p = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<ThumbnailListView *>();
        if (p.size() > 0) {
            p[0]->clearSelection();
            emit sigUpdatePicNum();
        }
    }
#if 1
    QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
    if (b.size() > 0) {
        b[0]->setText(pSuspensionChose->text());
    }
#endif
}

QStringList ImportTimeLineView::selectPaths()
{
    QStringList paths;
    for (int i = 0; i < m_allThumbnailListView.length(); i++) {
        paths << m_allThumbnailListView[i]->selectedPaths();
    }
    return paths;
}

void ImportTimeLineView::updateChoseText()
{
    for (int i = 0; i < m_allChoseButton.length(); i++) {
        if (m_allThumbnailListView[i]->model()->rowCount() == m_allThumbnailListView[i]->selectedPaths().length() && QObject::tr("Select") == m_allChoseButton[i]->text()) {
            m_allChoseButton[i]->setText(QObject::tr("Unselect"));
        }

        if (m_allThumbnailListView[i]->model()->rowCount() != m_allThumbnailListView[i]->selectedPaths().length() && QObject::tr("Unselect") == m_allChoseButton[i]->text()) {
            m_allChoseButton[i]->setText(QObject::tr("Select"));
        }
    }
}

void ImportTimeLineView::initTimeLineViewWidget()
{
    m_mainLayout = new QVBoxLayout();
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    pTimeLineViewWidget->setLayout(m_mainLayout);

    DPalette palcolor = DApplicationHelper::instance()->palette(pTimeLineViewWidget);
    palcolor.setBrush(DPalette::Base, palcolor.color(DPalette::Window));
    pTimeLineViewWidget->setPalette(palcolor);

    m_mainListWidget = new TimelineListWidget(this);
    m_mainListWidget->setFocusPolicy(Qt::NoFocus);
    m_mainListWidget->setResizeMode(QListWidget::Adjust);
    m_mainListWidget->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    m_mainListWidget->verticalScrollBar()->setSingleStep(20);
    m_mainLayout->addWidget(m_mainListWidget);
    m_mainListWidget->setFrameShape(DTableView::NoFrame);

    //添加悬浮title

    //add start 3975
    m_pImportTitle = new DLabel(pTimeLineViewWidget);
    m_pImportTitle->setText(tr("Import"));
    m_pImportTitle->setContentsMargins(17, 6, 0, 5);
    DFontSizeManager::instance()->bind(m_pImportTitle, DFontSizeManager::T3, QFont::DemiBold);
    m_pImportTitle->setForegroundRole(DPalette::TextTitle);
//    m_pImportTitle->setFixedSize(width() - 10, 47);
    m_pImportTitle->setFixedHeight(36);
    m_pImportTitle->move(0, 50);

    DPalette ppal_light2 = DApplicationHelper::instance()->palette(m_pImportTitle);
    ppal_light2.setBrush(DPalette::Background, ppal_light2.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light2 = new QGraphicsOpacityEffect;
    opacityEffect_light2->setOpacity(0.95);
    m_pImportTitle->setPalette(ppal_light2);
    m_pImportTitle->setGraphicsEffect(opacityEffect_light2);
    m_pImportTitle->setAutoFillBackground(true);
    //add end 3975

    m_dateItem = new DWidget(pTimeLineViewWidget);
    QHBoxLayout *TitleViewLayout = new QHBoxLayout();
//    TitleViewLayout->setContentsMargins(17, 0, 0, 6);
    TitleViewLayout->setContentsMargins(17, 0, 0, 0);
    m_dateItem->setLayout(TitleViewLayout);

    m_pDate = new DLabel();
    DFontSizeManager::instance()->bind(m_pDate, DFontSizeManager::T6, QFont::Medium);
    m_pDate->setForegroundRole(DPalette::Text);

    pNum_up = new DLabel();
    DFontSizeManager::instance()->bind(pNum_up, DFontSizeManager::T6, QFont::Medium);
    pNum_up->setForegroundRole(DPalette::Text);
    //原先注释的地方
    QFont ft6 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft6.setFamily("SourceHanSansSC");
    ft6.setWeight(QFont::Medium);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    DPalette pal = DApplicationHelper::instance()->palette(pNum_up);
    QColor color_BT = pal.color(DPalette::BrightText);
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);

    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
        pNum_up->setForegroundRole(DPalette::Text);
        pNum_up->setPalette(pal);
    }
    pNum_up->setForegroundRole(DPalette::Text);
    m_pDate->setForegroundRole(DPalette::Text);
    m_pDate->setPalette(pal);
    pNum_up->setPalette(pal);

    pNum_up->setFont(ft6);
    m_pDate->setFont(ft6);
    pNum_up->setForegroundRole(DPalette::Text);
    pNum_up->setPalette(pal);
    //end xiaolong

    TitleViewLayout->addWidget(m_pDate);
    TitleViewLayout->addWidget(pNum_up);

    QHBoxLayout *Layout = new QHBoxLayout();
    pSuspensionChose = new DCommandLinkButton(QObject::tr("Select"));
    pSuspensionChose->setFocusPolicy(Qt::NoFocus);
    AC_SET_OBJECT_NAME(pSuspensionChose, Import_Time_Line_Choose_Button);
    AC_SET_ACCESSIBLE_NAME(pSuspensionChose, Import_Time_Line_Choose_Button);

    DFontSizeManager::instance()->bind(pSuspensionChose, DFontSizeManager::T5);
    pSuspensionChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    pSuspensionChose->setFixedHeight(32);
    pSuspensionChose->resize(36, 30);

    pNum_up->setLayout(Layout);
    Layout->addStretch(1);
    Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    Layout->setContentsMargins(0, 0, 27, 0);
    Layout->addWidget(pSuspensionChose);
    connect(pSuspensionChose, &DCommandLinkButton::clicked, this, &ImportTimeLineView::onSuspensionChoseBtnClicked);
    DPalette ppal_light = DApplicationHelper::instance()->palette(m_dateItem);
    ppal_light.setBrush(DPalette::Background, ppal_light.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light = new QGraphicsOpacityEffect;
    opacityEffect_light->setOpacity(0.95);
    m_dateItem->setPalette(ppal_light);
    m_dateItem->setGraphicsEffect(opacityEffect_light);
    m_dateItem->setAutoFillBackground(true);
    m_dateItem->setFixedSize(this->width() - 10, SUBTITLE_HEIGHT);
    m_dateItem->setContentsMargins(0, 0, 0, 0);
    m_dateItem->move(0, 50 + m_pImportTitle->height()); //edit 3975
    m_dateItem->show();
    m_dateItem->setVisible(true);
}

void ImportTimeLineView::clearAndStop()
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

void ImportTimeLineView::clearAndStartLayout()
{
    for (ThumbnailListView *list : m_allThumbnailListView) {
        list->stopLoadAndClear();
        delete list;
    }
    m_allChoseButton.clear();
    m_allThumbnailListView.clear();
    m_mainListWidget->clear();
    m_timelines.clear();
    //获取所有时间线
    m_timelines = DBManager::instance()->getImportTimelines();
    qDebug() << __func__ << m_timelines.size();


    if (0 < m_timelines.size()) {
    } else {
        m_dateItem->setVisible(false);
    }
    currentTimeLineLoad = 0;
    //add start 3975
    TimelineItem *blankWidget = new TimelineItem(this);
    blankWidget->m_type = "blank";
    QListWidgetItem *blankItem = new QListWidgetItem();
    blankItem->setFlags(Qt::NoItemFlags);
    m_mainListWidget->addItemForWidget(blankItem);
    m_mainListWidget->setItemWidget(blankItem, blankWidget);
    blankItem->setSizeHint(QSize(0, m_pImportTitle->height()));

    addTimelineLayout();
}

void ImportTimeLineView::addTimelineLayout()
{
    if (currentTimeLineLoad >= m_timelines.size()) {
        return;
    }
    int nowTimeLineLoad = currentTimeLineLoad;
    //获取当前时间照片
    DBImgInfoList ImgInfoList = DBManager::instance()->getInfosByImportTimeline(m_timelines.at(nowTimeLineLoad));

    QListWidgetItem *item = new QListWidgetItem;
    TimelineItem *listItem = new TimelineItem(this);
    listItem->adjustSize();
    QVBoxLayout *listItemlayout = new QVBoxLayout();
    listItem->setLayout(listItemlayout);
    listItemlayout->setContentsMargins(0, 0, 0, 0);

    //添加title
    DWidget *TitleView = new DWidget;
    QHBoxLayout *TitleViewLayout = new QHBoxLayout();
    TitleViewLayout->setContentsMargins(12, 0, 0, 0);
    TitleView->setLayout(TitleViewLayout);
    DLabel *pDate = new DLabel();
    DFontSizeManager::instance()->bind(pDate, DFontSizeManager::T6, QFont::Medium);
    pNum_dn = new DLabel();
    DFontSizeManager::instance()->bind(pNum_dn, DFontSizeManager::T6, QFont::Medium);
    pDate->setForegroundRole(DPalette::Text);
    pDate->setFixedHeight(20);
    pNum_dn->setFixedHeight(20);
    QStringList dateTimeList = m_timelines.at(nowTimeLineLoad).split(" ");
    QStringList datelist = dateTimeList.at(0).split(".");
    if (datelist.count() > 2) {
        if (dateTimeList.count() == 2) {
            listItem->m_sdate = QString(QObject::tr("Imported on") + QObject::tr(" %1-%2-%3 %4"))
                                .arg(datelist[0]).arg(datelist[1]).arg(datelist[2]).arg(dateTimeList[1]);
        } else {
            listItem->m_sdate = QString(QObject::tr("Imported on ") + QObject::tr("%1/%2/%3"))
                                .arg(datelist[0]).arg(datelist[1]).arg(datelist[2]);
        }
    }

    QFont ft6 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft6.setFamily("SourceHanSansSC");
    ft6.setWeight(QFont::Medium);
    DPalette pal = DApplicationHelper::instance()->palette(pNum_dn);
    QColor color_BT = pal.color(DPalette::BrightText);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);

    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
    }
    pDate->setForegroundRole(DPalette::Text);
    pNum_dn->setForegroundRole(DPalette::Text);
    pDate->setFont(ft6);
    pNum_dn->setFont(ft6);
    pDate->setPalette(pal);
    pNum_dn->setPalette(pal);
    pDate->setText(listItem->m_sdate);
    listItem->m_date = pDate;
    listItem->m_snum = QString(QObject::tr("%1 photo(s)")).arg(ImgInfoList.size());
    pNum_dn->setForegroundRole(DPalette::Text);
    pNum_dn->setText(listItem->m_snum);

    QHBoxLayout *Layout = new QHBoxLayout();
    DCommandLinkButton *pChose = new DCommandLinkButton(QObject::tr("Select"));
    pChose->setFocusPolicy(Qt::NoFocus);
    m_allChoseButton << pChose;
    DFontSizeManager::instance()->bind(pChose, DFontSizeManager::T5);
    pChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    pChose->resize(36, 30);

    pNum_dn->setLayout(Layout);
    Layout->addStretch(1);
    Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    Layout->setContentsMargins(0, 0, 36, 0);
    Layout->addWidget(pChose);

    listItem->m_Chose = pChose;
    listItem->m_num = pNum_dn;
    TitleViewLayout->addWidget(pDate);
    TitleViewLayout->addWidget(pNum_dn);
    TitleView->setFixedHeight(SUBTITLE_HEIGHT);
    listItem->m_title = TitleView;

    //添加照片
    ThumbnailListView *pThumbnailListView = new ThumbnailListView(ThumbnailDelegate::NullType, COMMON_STR_RECENT_IMPORTED);
    pThumbnailListView->setFocusPolicy(Qt::NoFocus);
    int m_Baseheight =  getIBaseHeight();
    if (m_Baseheight == 0) {
        return;
    } else {
        pThumbnailListView->setIBaseHeight(m_Baseheight);
    }

    connect(pThumbnailListView, &ThumbnailListView::loadEnd, this, &ImportTimeLineView::addTimelineLayout);
    connect(this, &ImportTimeLineView::sigResizeTimelineBlock, pThumbnailListView, &ThumbnailListView::slotReCalcTimelineSize);
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

    m_allThumbnailListView.append(pThumbnailListView);
    pThumbnailListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pThumbnailListView->setContextMenuPolicy(Qt::CustomContextMenu);
    pThumbnailListView->setContentsMargins(0, 0, 0, 0);
    pThumbnailListView->setFrameShape(DTableView::NoFrame);
    //保存当前时间照片
    pThumbnailListView->loadFilesFromLocal(ImgInfoList);
    pThumbnailListView->m_imageType = COMMON_STR_RECENT_IMPORTED;
    connect(pThumbnailListView, &ThumbnailListView::sigMoveToTrash, this, &ImportTimeLineView::getCurrentSelectPics);

    if (0 == nowTimeLineLoad) {
        DWidget *topwidget = new DWidget;
        topwidget->setFixedHeight(50);
        listItemlayout->addWidget(topwidget);
    }
    listItemlayout->addWidget(TitleView);
    listItemlayout->addSpacing(-8);
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
        info.viewType = COMMON_STR_RECENT_IMPORTED;
        info.viewMainWindowID = VIEW_MAINWINDOW_ALBUM;
        emit dApp->signalM->viewImage(info);
        emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALBUM);
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
        info.viewType = COMMON_STR_RECENT_IMPORTED;
        info.viewMainWindowID = VIEW_MAINWINDOW_ALBUM;
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
            emit dApp->signalM->showSlidePanel(VIEW_MAINWINDOW_ALBUM);
        } else {
            emit dApp->signalM->viewImage(info);
            emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALBUM);
        }
    });
    connect(pChose, &DCommandLinkButton::clicked, this, [ = ] {
        if (QObject::tr("Select") == pChose->text())
        {
            pChose->setText(QObject::tr("Unselect"));
            pThumbnailListView->selectAll();
            for (int j = 0; j < m_allChoseButton.length(); j++) {
                if (pChose == m_allChoseButton[j])
                    lastClickedIndex = j;
            }
            lastRow = 0;
            lastChanged = true;
            m_ctrlPress = true;
        } else
        {
            pChose->setText(QObject::tr("Select"));
            pThumbnailListView->clearSelection();
        }
        emit sigUpdatePicNum();
    });
#if 1

    connect(pThumbnailListView, &ThumbnailListView::sigMousePress, this, [ = ](QMouseEvent * event) {
        lastRow = -1;
//       If required only select one image at a time add this code
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

    connect(pThumbnailListView, &ThumbnailListView::sigShiftMousePress, this, [ = ](QMouseEvent * event) {
        int curClickedIndex = -1;
        int curRow = -1;
        for (int j = 0; j < m_allThumbnailListView.length(); j++) {
            if (pThumbnailListView == m_allThumbnailListView[j]) {
                curClickedIndex = j;
                curRow = pThumbnailListView->getRow(QPoint(event->x(), event->y()));
            }
        }

        if (!lastChanged && -1 != curRow && -1 != m_lastShiftRow) {
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
                for (int j = lastClickedIndex + 1; j < curClickedIndex; j++) {
                    m_allThumbnailListView[j]->selectAll();
                }
            } else if (lastClickedIndex > curClickedIndex) {
                m_allThumbnailListView[lastClickedIndex]->selectFront(lastRow);
                m_allThumbnailListView[curClickedIndex]->selectRear(curRow);
                for (int j = curClickedIndex + 1; j < lastClickedIndex; j++) {
                    m_allThumbnailListView[j]->selectAll();
                }
            } else if (lastClickedIndex == curClickedIndex) {
                if (lastRow <= curRow)
                    pThumbnailListView->selectExtent(lastRow, curRow);
                else
                    pThumbnailListView->selectExtent(curRow, lastRow);
            }
            emit sigUpdatePicNum();
            updateChoseText();
            m_lastShiftRow = curRow;
            m_lastShiftClickedIndex = curClickedIndex;
            curRow = -1;
            lastChanged = false;
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
        emit sigUpdatePicNum();
        updateChoseText();
    });

    connect(pThumbnailListView, &ThumbnailListView::sigGetSelectedPaths, this, [ = ](QStringList * pPaths) {
        pPaths->clear();
        for (int j = 0; j < m_allThumbnailListView.size(); j++) {
            pPaths->append(m_allThumbnailListView[j]->selectedPaths());
        }
    });

    connect(pThumbnailListView, &ThumbnailListView::sigSelectAll, this, [ = ] {
        m_ctrlPress = true;
        for (int j = 0; j < m_allThumbnailListView.length(); j++)
        {
            m_allThumbnailListView[j]->selectAll();
        }
        emit sigUpdatePicNum();
        updateChoseText();
        QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
        pSuspensionChose->setText(b[0]->text());
    });

    connect(pThumbnailListView, &ThumbnailListView::sigMouseMove, this, [ = ] {
        emit sigUpdatePicNum();
        updateChoseText();
    });

    connect(pThumbnailListView, &ThumbnailListView::sigMouseRelease, this, [ = ] {
        if (!m_ctrlPress)
        {
            for (int j = 0; j < m_allThumbnailListView.length(); j++) {
                if (pThumbnailListView != m_allThumbnailListView[j]) {
                    m_allThumbnailListView[j]->clearSelection();
                }
            }
        }
        emit sigUpdatePicNum();
        updateChoseText();
        QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
        pSuspensionChose->setText(b[0]->text());
    });

    connect(pThumbnailListView, &ThumbnailListView::customContextMenuRequested, this, [ = ] {
        QStringList paths = pThumbnailListView->selectedPaths();
        if (pThumbnailListView->model()->rowCount() == paths.length() && QObject::tr("Select") == pChose->text())
        {
            pChose->setText(QObject::tr("Unselect"));
        }

        if (pThumbnailListView->model()->rowCount() != paths.length() && QObject::tr("Unselect") == pChose->text())
        {
            pChose->setText(QObject::tr("Select"));
        }
        emit sigUpdatePicNum();
    });
    connect(pThumbnailListView, &ThumbnailListView::sigMenuItemDeal, this, [ = ](QAction * action) {
        QStringList paths;
        paths.clear();
        for (int j = 0; j < m_allThumbnailListView.size(); j++) {
            paths << m_allThumbnailListView[j]->selectedPaths();
        }
        pThumbnailListView->menuItemDeal(paths, action);
    });
    connect(pThumbnailListView, &ThumbnailListView::needResizeLabel, this, [ = ] {
        listItem->m_title->setFixedWidth(width() - 14);
    });
    connect(listItem, &TimelineItem::sigMousePress, this, [ = ] {
        for (int j = 0; j < m_allThumbnailListView.length(); j++)
        {
            m_allThumbnailListView[j]->clearSelection();
        }
        lastRow = -1;
        emit sigUpdatePicNum();
        updateChoseText();
    });
#endif
    connect(m_allThumbnailListView[nowTimeLineLoad], &ThumbnailListView::sigKeyEvent, this, &ImportTimeLineView::on_KeyEvent);
    emit sigUpdatePicNum();
    currentTimeLineLoad++;

    //界面可见时,调整整体大小
    if (m_bshow) {
        updateSize();
    }

    //判断跳转位置图片是否包含在当前listview
    if (selectPrePaths.length() > 0 && !isFindPic) {
        for (DBImgInfo &imgInfo : ImgInfoList) {
            if (imgInfo.filePath == selectPrePaths) {
                hasPicViewNum = nowTimeLineLoad;
                qDebug() << "have find pic view num: " << hasPicViewNum;
                isFindPic = true;
                break;
            }
        }
    }
    QTimer::singleShot(150, this, [ = ] {
        if (nowTimeLineLoad == m_timelines.size() - 1 && hasPicViewNum >= 0)
        {
            int height = 0;
            for (int i = 0; i < m_timelines.size(); i++) {
                if (i < hasPicViewNum) {
                    height = height + m_allThumbnailListView[i]->height() + m_allChoseButton[0]->height();
                }
                if (i == hasPicViewNum) {
                    for (int j = 0; j < m_allThumbnailListView[i]->m_model->rowCount(); j ++) {
                        QModelIndex index = m_allThumbnailListView[i]->m_model->index(j, 0);
                        QVariantList lst = index.model()->data(index, Qt::DisplayRole).toList();
                        int rowcount = 0;
                        int allrowcount = 0;
                        if (lst.count() >= 12) {
                            QString path = lst.at(1).toString();
                            if (path == selectPrePaths) {
                                if (index.row() % m_allThumbnailListView[i]->m_rowSizeHint == 0) {
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
                                int thumbnailheight =  m_allThumbnailListView[i]->m_height;
                                double finalheight = tempheight * thumbnailheight;
                                height = height + static_cast<int>(finalheight) ;

                                if (hasPicViewNum == 0)
                                    height = height - m_allThumbnailListView[i]->m_onePicWidth;
                                break;
                            }
                        }
                    }
                    break;
                }
            }
            //相对界面调整位置
            int modify = (dApp->getMainWindow()->height() - 140 - m_allThumbnailListView[0]->m_onePicWidth) / 2;
            height -= modify;
            m_mainListWidget->verticalScrollBar()->setValue(height);
            hasPicViewNum = -1;
            selectPrePaths = "";
            isFindPic = false;
        }
    });
}

void ImportTimeLineView::getFatherStatusBar(DSlider *s)
{
    this->m_DSlider = s;
}

void ImportTimeLineView::on_AddLabel(QString date, QString num)
{
    if ((nullptr != m_dateItem) && (nullptr != m_mainListWidget)) {
        QList<QLabel *> labelList = m_dateItem->findChildren<QLabel *>();
        labelList[0]->setText(date);
        labelList[1]->setText(num);
        m_dateItem->setVisible(true);
        m_dateItem->move(0, 50 + m_pImportTitle->height()); //edit 3975
    }
#if 1
    QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
    pSuspensionChose->setText(b[0]->text());
#endif
}

//void ImportTimeLineView::on_DelLabel()
//{
//    if (nullptr != m_dateItem) {
//        m_dateItem->setVisible(false);
//    }
//#if 1
//    QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
//    pSuspensionChose->setText(b[0]->text());
//#endif
//}

#if 1
void ImportTimeLineView::on_MoveLabel(int y, QString date, QString num, QString choseText)
#endif
{
    Q_UNUSED(y);
    if ((nullptr != m_dateItem) && (nullptr != m_mainListWidget)) {
        QList<QLabel *> labelList = m_dateItem->findChildren<QLabel *>();
        labelList[0]->setText(date);
        labelList[1]->setText(num);
        pSuspensionChose->setText(choseText);
        m_dateItem->setVisible(true);
    }
}

void ImportTimeLineView::on_KeyEvent(int key)
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

void ImportTimeLineView::resizeEvent(QResizeEvent *ev)
{
    Q_UNUSED(ev);
    updateSize();
}

void ImportTimeLineView::showEvent(QShowEvent *ev)
{
    Q_UNUSED(ev)
    m_bshow = true;
}

void ImportTimeLineView::dragEnterEvent(QDragEnterEvent *e)
{
    const QMimeData *mimeData = e->mimeData();
    if (!utils::base::checkMimeData(mimeData)) {
        return;
    }
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void ImportTimeLineView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    ImageEngineApi::instance()->ImportImagesFromUrlList(urls, nullptr, this);
    event->accept();
}

void ImportTimeLineView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void ImportTimeLineView::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_UNUSED(e);
}

void ImportTimeLineView::keyPressEvent(QKeyEvent *e)
{
    qDebug() << "ImportTimeLineView::keyPressEvent()";
    if (e->key() == Qt::Key_Control) {
//        m_ctrlPress = true;
    }
}

void ImportTimeLineView::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Control) {
//        m_ctrlPress = false;
    }
}

void ImportTimeLineView::mousePressEvent(QMouseEvent *e)
{
    qDebug() << "鼠标按下：";
    if (!m_ctrlPress && e->button() == Qt::LeftButton) {
        for (int i = 0; i < m_allThumbnailListView.length(); i++) {
            m_allThumbnailListView[i]->clearSelection();
        }
        emit sigUpdatePicNum();
        updateChoseText();
    }
    DWidget::mousePressEvent(e);
}

void ImportTimeLineView::clearAllSelection()
{
    for (int j = 0; j < m_allThumbnailListView.length(); j++) {
        m_allThumbnailListView[j]->clearSelection();
    }
}
