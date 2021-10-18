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
#include <QMimeData>
#include <QGraphicsOpacityEffect>
#include <QTimer>

#include <DPushButton>
#include <DTableView>
#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <dgiovolume.h>

#include "batchoperatewidget.h"
#include "noresultwidget.h"

ImportTimeLineView::ImportTimeLineView(DWidget *parent)
    : DWidget(parent), m_mainLayout(nullptr)
    , m_DSlider(nullptr)
    , m_oe(nullptr), m_oet(nullptr), m_ctrlPress(false)
{
    setAcceptDrops(true);
    m_oe = new QGraphicsOpacityEffect(this);
    m_oet = new QGraphicsOpacityEffect(this);
    m_oe->setOpacity(0.5);
    m_oet->setOpacity(0.75);
    m_timeLineViewWidget = new DWidget(this);
    m_timeLineViewWidget->setFocusPolicy(Qt::ClickFocus);
    QVBoxLayout *pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins(0, 0, 0, 0);
    pVBoxLayout->addWidget(m_timeLineViewWidget);
    this->setLayout(pVBoxLayout);
    initTimeLineViewWidget();
    initConnections();
}

bool ImportTimeLineView::imageImported(bool success)
{
    Q_UNUSED(success);
    emit dApp->signalM->closeWaitDialog();
    return true;
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

void ImportTimeLineView::initConnections()
{
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &ImportTimeLineView::themeChangeSlot);
    // 重复导入图片选中
    connect(dApp->signalM, &SignalManager::RepeatImportingTheSamePhotos, this, &ImportTimeLineView::onRepeatImportingTheSamePhotos);
    // ctrl+all 全选
    connect(m_importTimeLineListView, &ThumbnailListView::sigSelectAll, this, [ = ]() {
        m_suspensionChoseBtn->setText(QObject::tr("Unselect"));
    });
}

void ImportTimeLineView::themeChangeSlot(DGuiApplicationHelper::ColorType themeType)
{
//    Q_UNUSED(themeType);
    DPalette palcolor = DApplicationHelper::instance()->palette(m_timeLineViewWidget);
    palcolor.setBrush(DPalette::Base, palcolor.color(DPalette::Window));
    m_timeLineViewWidget->setPalette(palcolor);

    DPalette pa1 = DApplicationHelper::instance()->palette(m_choseBtnItem);
    pa1.setBrush(DPalette::Background, pa1.color(DPalette::Base));
    m_choseBtnItem->setForegroundRole(DPalette::Background);
    m_choseBtnItem->setPalette(pa1);

    //add start 3975
    DPalette ppal_light2 = DApplicationHelper::instance()->palette(m_pImportTitle);
    ppal_light2.setBrush(DPalette::Background, ppal_light2.color(DPalette::Base));
    m_pImportTitle->setPalette(ppal_light2);
    //add end 3975

    DPalette pal1 = DApplicationHelper::instance()->palette(m_NumLabel);
    QColor color_BT1 = pal1.color(DPalette::BrightText);
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT1.setAlphaF(0.5);
        pal1.setBrush(DPalette::Text, color_BT1);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT1.setAlphaF(0.75);
        pal1.setBrush(DPalette::Text, color_BT1);
    }
    m_NumLabel->setForegroundRole(DPalette::Text);
    m_DateLabel->setForegroundRole(DPalette::Text);
    m_DateLabel->setPalette(pal1);
    m_NumLabel->setPalette(pal1);
    //todo
//    for (int i = 1; i < m_mainListWidget->count(); i++) {
//        TimelineItem *item = static_cast<TimelineItem *>(m_mainListWidget->itemWidget(m_mainListWidget->item(i)));
//        QList<DLabel *> pLabelList = item->findChildren<DLabel *>();
//        if (pLabelList.size() < 2) {
//            break;
//        }
//        DPalette pal = DApplicationHelper::instance()->palette(pLabelList[1]);
//        QColor color_BT = pal.color(DPalette::BrightText);
//        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
//        if (themeType == DGuiApplicationHelper::LightType) {
//            color_BT.setAlphaF(0.5);
//            pal.setBrush(DPalette::Text, color_BT);
//        } else if (themeType == DGuiApplicationHelper::DarkType) {
//            color_BT.setAlphaF(0.75);
//            pal.setBrush(DPalette::Text, color_BT);
//        }
//        pLabelList[1]->setForegroundRole(DPalette::Text);
//        pLabelList[0]->setForegroundRole(DPalette::Text);
//        pLabelList[0]->setPalette(pal);
//        pLabelList[1]->setPalette(pal);
//    }
}

ThumbnailListView *ImportTimeLineView::getListView()
{
    return m_importTimeLineListView;
}

void ImportTimeLineView::updateSize()
{
    m_TitleItem->setFixedSize(width() - 15, title_HEIGHT);
    m_pImportTitle->move(m_TitleItem->width() / 2 - m_pImportTitle->width() / 2, 0);
    m_pImportTitle->raise();//图层上移
    m_choseBtnItem->setFixedSize(width() - 15, ChoseBtn_HEIGHT);
    m_choseBtnItem->move(0, m_TitleItem->geometry().bottom());
}

void ImportTimeLineView::onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, const QString &albumName)
{
    Q_UNUSED(importPaths)
    // 导入的照片重复照片提示
    if (duplicatePaths.size() > 0 && albumName.length() < 1 && dApp->getMainWindow()->getCurrentViewType() == 2) {
        m_importTimeLineListView->selectDuplicatePhotos(duplicatePaths);
    }
}

void ImportTimeLineView::onSuspensionChoseBtnClicked()
{
    bool isSelect = false;
    if (QObject::tr("Select") == m_suspensionChoseBtn->text()) {
        m_suspensionChoseBtn->setText(QObject::tr("Unselect"));
        isSelect = true;
    } else {
        m_suspensionChoseBtn->setText(QObject::tr("Select"));
    }
    QString date_str = m_DateLabel->text();
    m_importTimeLineListView->timeLimeFloatBtnClicked(date_str, isSelect);
}

void ImportTimeLineView::slotBatchSelectChanged(bool isBatchSelect)
{
    if (isBatchSelect) {
        m_choseBtnItem->setVisible(true);
        m_importTimeLineListView->resetBlankItemHeight(ChoseBtn_HEIGHT + title_HEIGHT);
    } else {
        m_choseBtnItem->setVisible(false);
        m_importTimeLineListView->resetBlankItemHeight(title_HEIGHT);
    }
}

void ImportTimeLineView::slotNoPicOrNoVideo(bool isNoResult)
{
    m_noResultWidget->setVisible(isNoResult);
    m_importTimeLineListView->setVisible(!isNoResult);
    m_DateLabel->setVisible(!isNoResult);
    m_NumLabel->setVisible(!isNoResult);
    m_pImportTitle->setVisible(!isNoResult);
    emit sigNoPicOrNoVideo(isNoResult);
}

void ImportTimeLineView::onDelete()
{
    m_suspensionChoseBtn->setText(QObject::tr("Select"));
}

QStringList ImportTimeLineView::selectPaths()
{
    QStringList paths;
    paths << m_importTimeLineListView->selectedPaths();
    return paths;
}

void ImportTimeLineView::initTimeLineViewWidget()
{
    m_mainLayout = new QVBoxLayout(m_timeLineViewWidget);
    //左侧距离分界线20px，8+缩略图spacing+缩略图边框
    m_mainLayout->setContentsMargins(8, 0, 0, 0);
    m_timeLineViewWidget->setLayout(m_mainLayout);

    DPalette palcolor = DApplicationHelper::instance()->palette(m_timeLineViewWidget);
    palcolor.setBrush(DPalette::Base, palcolor.color(DPalette::Window));
    m_timeLineViewWidget->setPalette(palcolor);

    m_importTimeLineListView = new ThumbnailListView(ThumbnailDelegate::AlbumViewImportTimeLineViewType, COMMON_STR_RECENT_IMPORTED);
    m_importTimeLineListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_importTimeLineListView->setContextMenuPolicy(Qt::CustomContextMenu);
    m_importTimeLineListView->setContentsMargins(0, 0, 0, 0);
    m_importTimeLineListView->setFocusPolicy(Qt::NoFocus);
    m_importTimeLineListView->m_imageType = COMMON_STR_RECENT_IMPORTED;
    m_mainLayout->addWidget(m_importTimeLineListView);
    //初始化筛选无结果窗口
    m_noResultWidget = new NoResultWidget(this);
    m_mainLayout->addWidget(m_noResultWidget);
    m_noResultWidget->setVisible(false);

    connect(m_importTimeLineListView, &ThumbnailListView::sigShowEvent, this, &ImportTimeLineView::clearAndStartLayout);
    //滑动列表，刷新上方悬浮标题
    connect(m_importTimeLineListView, &ThumbnailListView::sigTimeLineDataAndNum, this, &ImportTimeLineView::slotTimeLineDataAndNum);
    //打开图片
    connect(m_importTimeLineListView, &ThumbnailListView::openImage, this, &ImportTimeLineView::onOpenImage);
    //幻灯片播放
    connect(m_importTimeLineListView, &ThumbnailListView::sigSlideShow, this, &ImportTimeLineView::onSlideShow);
    connect(m_importTimeLineListView, &ThumbnailListView::sigMouseMove, this, &ImportTimeLineView::sigUpdatePicNum);
    //筛选显示，当先列表中内容为无结果
    connect(m_importTimeLineListView, &ThumbnailListView::sigNoPicOrNoVideo, this, &ImportTimeLineView::slotNoPicOrNoVideo);
    //响应删除
    connect(m_importTimeLineListView, &ThumbnailListView::sigMoveToTrash, this, &ImportTimeLineView::onDelete);
    connect(dApp->signalM, &SignalManager::sigShortcutKeyDelete, this, &ImportTimeLineView::onDelete);

    //添加悬浮title
    //优化悬浮title布局，适配维语
    m_TitleItem = new DWidget(m_timeLineViewWidget);
    m_TitleItem->setFocusPolicy(Qt::NoFocus);
    QHBoxLayout *TitleLayout = new QHBoxLayout();
    m_TitleItem->setLayout(TitleLayout);
    TitleLayout->setContentsMargins(20, 0, 19, 0);

    DPalette ppal_TitleItem = DApplicationHelper::instance()->palette(m_TitleItem);
    ppal_TitleItem.setBrush(DPalette::Background, ppal_TitleItem.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_TitleItem = new QGraphicsOpacityEffect;
    opacityEffect_TitleItem->setOpacity(0.95);
    m_TitleItem->setPalette(ppal_TitleItem);
    m_TitleItem->setGraphicsEffect(opacityEffect_TitleItem);
    m_TitleItem->setAutoFillBackground(true);
    //时间数量
    m_DateLabel = new DLabel();
    DFontSizeManager::instance()->bind(m_DateLabel, DFontSizeManager::T6, QFont::Medium);
    m_DateLabel->setForegroundRole(DPalette::Text);

    m_NumLabel = new DLabel();
    DFontSizeManager::instance()->bind(m_NumLabel, DFontSizeManager::T6, QFont::Medium);
    m_NumLabel->setForegroundRole(DPalette::Text);

    QFont ft6 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft6.setFamily("SourceHanSansSC");
    ft6.setWeight(QFont::Medium);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    DPalette pal = DApplicationHelper::instance()->palette(m_NumLabel);
    QColor color_BT = pal.color(DPalette::BrightText);
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);

    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
        m_NumLabel->setForegroundRole(DPalette::Text);
        m_NumLabel->setPalette(pal);
    }
    m_NumLabel->setForegroundRole(DPalette::Text);
    m_DateLabel->setForegroundRole(DPalette::Text);
    m_DateLabel->setPalette(pal);
    m_NumLabel->setPalette(pal);

    m_NumLabel->setFont(ft6);
    m_DateLabel->setFont(ft6);
    //end xiaolong

    TitleLayout->addWidget(m_DateLabel);
    TitleLayout->addWidget(m_NumLabel);
    QSpacerItem *spacerItem = new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Expanding);
    TitleLayout->addSpacerItem(spacerItem);
    m_TitleItem->move(0, 0);

    //右侧批量操作控件
    m_batchOperateWidget = new BatchOperateWidget(m_importTimeLineListView, BatchOperateWidget::NullType, this);
    //进入批量状态
    connect(m_batchOperateWidget, &BatchOperateWidget::signalBatchSelectChanged, this, &ImportTimeLineView::slotBatchSelectChanged);
    TitleLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    TitleLayout->addWidget(m_batchOperateWidget);

    //已导入
    m_pImportTitle = new DLabel(m_timeLineViewWidget);
    m_pImportTitle->setText(tr("Import"));
    m_pImportTitle->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    DFontSizeManager::instance()->bind(m_pImportTitle, DFontSizeManager::T3, QFont::DemiBold);
    m_pImportTitle->setForegroundRole(DPalette::TextTitle);
    m_pImportTitle->setFixedHeight(title_HEIGHT);
    DPalette ppal_light2 = DApplicationHelper::instance()->palette(m_pImportTitle);
    ppal_light2.setBrush(DPalette::Background, ppal_light2.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light2 = new QGraphicsOpacityEffect;
    opacityEffect_light2->setOpacity(0.95);
    m_pImportTitle->setPalette(ppal_light2);
    m_pImportTitle->setGraphicsEffect(opacityEffect_light2);
    m_pImportTitle->setAutoFillBackground(true);
    m_pImportTitle->setFixedWidth(150);//BUG#93470，手动调大控件长度（英语模式下最后那个t会部分遮挡）

    m_choseBtnItem = new DWidget(m_timeLineViewWidget);
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setContentsMargins(17, 0, 19, 0);
    m_choseBtnItem->setLayout(btnLayout);

    m_suspensionChoseBtn = new DCommandLinkButton(QObject::tr("Select"));
    m_suspensionChoseBtn->setFocusPolicy(Qt::NoFocus);
    AC_SET_OBJECT_NAME(m_suspensionChoseBtn, Import_Time_Line_Choose_Button);
    AC_SET_ACCESSIBLE_NAME(m_suspensionChoseBtn, Import_Time_Line_Choose_Button);

    DFontSizeManager::instance()->bind(m_suspensionChoseBtn, DFontSizeManager::T5);
    m_suspensionChoseBtn->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    m_suspensionChoseBtn->setFixedHeight(32);
    m_suspensionChoseBtn->resize(36, 30);

    //适配维语布局
    btnLayout->addStretch();
    btnLayout->addWidget(m_suspensionChoseBtn);

    connect(m_suspensionChoseBtn, &DCommandLinkButton::clicked, this, &ImportTimeLineView::onSuspensionChoseBtnClicked);
    DPalette ppal_light = DApplicationHelper::instance()->palette(m_choseBtnItem);
    ppal_light.setBrush(DPalette::Background, ppal_light.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light = new QGraphicsOpacityEffect;
    opacityEffect_light->setOpacity(0.95);
    m_choseBtnItem->setPalette(ppal_light);
    m_choseBtnItem->setGraphicsEffect(opacityEffect_light);
    m_choseBtnItem->setAutoFillBackground(true);
    m_choseBtnItem->setFixedSize(this->width() - 10, ChoseBtn_HEIGHT);
    m_choseBtnItem->setContentsMargins(0, 0, 0, 0);
    m_choseBtnItem->move(0, m_TitleItem->geometry().bottom());
    m_choseBtnItem->setVisible(false);
}

void ImportTimeLineView::clearAndStartLayout()
{
    //由于绘制需要使用listview的宽度，但是加载的时候listview还没有显示出来，宽度是不对的，所以在显示出来后用信号通知加载，记载完成后断开信号，
    //后面的listview就有了正确的宽度，该信号槽就不需要再连接
    disconnect(m_importTimeLineListView, &ThumbnailListView::sigShowEvent, this, &ImportTimeLineView::clearAndStartLayout);
    qDebug() << "------" << __FUNCTION__ << "";
    m_timelines.clear();
    //获取所有时间线
    m_timelines = DBManager::instance()->getImportTimelines();
    qDebug() << __func__ << m_timelines.size();


    if (0 < m_timelines.size()) {
    } else {
        m_choseBtnItem->setVisible(false);
    }
    addTimelineLayout();
}

void ImportTimeLineView::addTimelineLayout()
{
    m_importTimeLineListView->clearAll();
    for (int timelineIndex = 0; timelineIndex < m_timelines.size(); timelineIndex++) {
        //获取当前时间照片
        DBImgInfoList ImgInfoList = DBManager::instance()->getInfosByImportTimeline(m_timelines.at(timelineIndex));

        //加时间线标题
        QString data, num;
        QStringList dateTimeList = m_timelines.at(timelineIndex).split(" ");
        QStringList datelist = dateTimeList.at(0).split(".");
        if (datelist.count() > 2) {
            if (dateTimeList.count() == 2) {
                data = QString(QObject::tr("Imported on") + QObject::tr(" %1-%2-%3 %4"))
                       .arg(datelist[0]).arg(datelist[1]).arg(datelist[2]).arg(dateTimeList[1]);
            } else {
                data = QString(QObject::tr("Imported on ") + QObject::tr("%1/%2/%3"))
                       .arg(datelist[0]).arg(datelist[1]).arg(datelist[2]);
            }
        }
        int photoCount = 0;
        int videoCount = 0;
        for (int i = 0; i < ImgInfoList.size(); i++) {
            if (ImgInfoList.at(i).itemType == ItemTypePic) {
                photoCount++;
            } else if (ImgInfoList.at(i).itemType == ItemTypeVideo) {
                videoCount++;
            }
        }
        if (photoCount == 1 && videoCount == 0) {
            num = tr("1 photo");
        } else if (photoCount == 0 && videoCount == 1) {
            num = tr("1 video");
        } else if (photoCount > 1 && videoCount == 0) {
            num = tr("%n photos", "", photoCount);
        } else if (photoCount == 0 && videoCount > 1) {
            num = tr("%n videos", "", videoCount);
        } else if (photoCount > 1 && videoCount > 1) {
            num = tr("%n items", "", (photoCount + videoCount));
        }

        if (timelineIndex == 0) {
            m_DateLabel->setText(data);
            m_NumLabel->setText(num);
            //加空白栏
            if (!m_choseBtnItem->isHidden()) {
                m_importTimeLineListView->insertBlankOrTitleItem(ItemTypeBlank, data, num, (title_HEIGHT + ChoseBtn_HEIGHT));
            } else {
                m_importTimeLineListView->insertBlankOrTitleItem(ItemTypeBlank, data, num, (title_HEIGHT));
            }
        } else {
            //加已导入时间线标题
            m_importTimeLineListView->insertBlankOrTitleItem(ItemTypeImportTimeLineTitle, data, num, 40);
        }
        //加当前时间下的图片
        m_importTimeLineListView->insertThumbnailByImgInfos(ImgInfoList);
    }
}

void ImportTimeLineView::getFatherStatusBar(DSlider *s)
{
    this->m_DSlider = s;
}

void ImportTimeLineView::slotTimeLineDataAndNum(QString data, QString num, QString text)
{
    if (!data.isEmpty()) {
        m_DateLabel->setText(data);
    }
    if (!num.isEmpty()) {
        m_NumLabel->setText(num);
    }
    m_suspensionChoseBtn->setText(text);
}

void ImportTimeLineView::onOpenImage(int row, const QString &path, bool bFullScreen)
{
    SignalManager::ViewInfo info;
    info.album = "";
//    info.lastPanel = nullptr;  //todo imageviewer
    info.fullScreen = bFullScreen;
    auto imagelist = m_importTimeLineListView->getFileList(row, ItemType::ItemTypePic);
    if (imagelist.size() > 0) {
        info.paths << imagelist;
        info.path = path;
    } else {
        info.paths.clear();
    }

    info.dBImgInfos = m_importTimeLineListView->getAllFileInfo(row);
    info.viewType = COMMON_STR_RECENT_IMPORTED;
    info.viewMainWindowID = VIEW_MAINWINDOW_ALBUM;
    emit dApp->signalM->sigViewImage(info.paths, info.path);
    emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALBUM);
}

void ImportTimeLineView::onSlideShow(QString path)
{
    SignalManager::ViewInfo info;
    info.album = "";
//    info.lastPanel = nullptr;  //todo imageviewer

    auto photolist = m_importTimeLineListView->selectedPaths();
    if (photolist.size() > 1) {
        //如果选中数目大于1，则幻灯片播放选中项
        info.paths = photolist;
        info.path = photolist.at(0);
    } else {
        //如果选中项只有一项，则幻灯片播放全部
        info.paths = m_importTimeLineListView->getFileList(m_importTimeLineListView->getRow(path));
        info.path = path;
    }

    info.fullScreen = true;
    info.slideShow = true;
    info.viewType = COMMON_STR_RECENT_IMPORTED;
    info.viewMainWindowID = VIEW_MAINWINDOW_ALBUM;
    emit dApp->signalM->startSlideShow(info);
    emit dApp->signalM->showSlidePanel(VIEW_MAINWINDOW_ALBUM);
}

void ImportTimeLineView::resizeEvent(QResizeEvent *ev)
{
    Q_UNUSED(ev);
    updateSize();
}

void ImportTimeLineView::dragEnterEvent(QDragEnterEvent *e)
{
    if (!utils::base::checkMimeUrls(e->mimeData()->urls())) {
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

void ImportTimeLineView::mousePressEvent(QMouseEvent *e)
{
    qDebug() << "鼠标按下：";
    if (!m_ctrlPress && e->button() == Qt::LeftButton) {
        m_importTimeLineListView->clearSelection();
        m_suspensionChoseBtn->setText(QObject::tr("Select"));
        emit sigUpdatePicNum();
    }
    DWidget::mousePressEvent(e);
    // 焦点移除，需要同步各个选择按钮状态
    m_importTimeLineListView->updatetimeLimeBtnText();
}

void ImportTimeLineView::clearAllSelection()
{
    m_importTimeLineListView->clearSelection();
    m_suspensionChoseBtn->setText(QObject::tr("Select"));
}
