#include "importtimelineview.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/snifferimageformat.h"
#include "imageengine/imageengineapi.h"
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

namespace  {
const int SUBTITLE_HEIGHT = 37;
const int VIEW_MAINWINDOW_ALBUM = 2;
} //namespace

ImportTimeLineView::ImportTimeLineView(DWidget *parent): DWidget(parent)
{
    setAcceptDrops(true);
    m_index = 0;

    m_oe = new QGraphicsOpacityEffect;
    m_oet = new QGraphicsOpacityEffect;
    m_oe->setOpacity(0.5);
    m_oet->setOpacity(0.75);

    pTimeLineViewWidget = new DWidget();

    QVBoxLayout *pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins(0, 0, 0, 0);
    pVBoxLayout->addWidget(pTimeLineViewWidget);
    this->setLayout(pVBoxLayout);

    initTimeLineViewWidget();

//    updataLayout();
    clearAndStartLayout();

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
    }
}

void ImportTimeLineView::initConnections()
{
    connect(m_mainListWidget, &TimelineList::sigNewTime, this, [ = ](QString date, QString num, int index) {
        m_index = index;
        on_AddLabel(date, num);
    });

    connect(m_mainListWidget, &TimelineList::sigDelTime, this, [ = ]() {
        on_DelLabel();
    });

    connect(m_mainListWidget, &TimelineList::sigMoveTime, this, [ = ](int y, QString date, QString num, QString choseText) {
        on_MoveLabel(y, date, num, choseText);
    });

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &ImportTimeLineView::themeChangeSlot);
}

void ImportTimeLineView::themeChangeSlot(DGuiApplicationHelper::ColorType themeType)
{
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

//    DPalette pal = DApplicationHelper::instance()->palette(pNum_up);
//    QColor color_BT = pal.color(DPalette::BrightText);
//    if (themeType == DGuiApplicationHelper::LightType)
//    {
//        color_BT.setAlphaF(0.5);
//        pal.setBrush(DPalette::Text, color_BT);
//        pNum_up->setForegroundRole(DPalette::Text);
//        pNum_up->setPalette(pal);
//    }
//    else if (themeType == DGuiApplicationHelper::DarkType)
//    {
//        color_BT.setAlphaF(0.75);
//        pal.setBrush(DPalette::Text, color_BT);
//        pNum_up->setForegroundRole(DPalette::Text);
//        pNum_up->setPalette(pal);
//    }

//    for(int i = 1; i < m_mainListWidget->count(); i++)
//    {
//        TimelineItem *item = (TimelineItem*)m_mainListWidget->itemWidget(m_mainListWidget->item(i));
//        QList<DLabel*> pLabelList = item->findChildren<DLabel*>();
//        DPalette color = DApplicationHelper::instance()->palette(pLabelList[0]);
//        color.setBrush(DPalette::Text, color.color(DPalette::ToolTipText));
//        pLabelList[0]->setForegroundRole(DPalette::Text);
//        pLabelList[0]->setPalette(color);

//        DPalette pal = DApplicationHelper::instance()->palette(pLabelList[1]);
//        QColor color_BT = pal.color(DPalette::BrightText);
//        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
//        if (themeType == DGuiApplicationHelper::LightType)
//        {
//            color_BT.setAlphaF(0.5);
//            pal.setBrush(DPalette::Text, color_BT);
//            pLabelList[1]->setForegroundRole(DPalette::Text);
//            pLabelList[1]->setPalette(pal);
//        }
//        else if (themeType == DGuiApplicationHelper::DarkType)
//        {
//            color_BT.setAlphaF(0.75);
//            pal.setBrush(DPalette::Text, color_BT);
//            pLabelList[1]->setForegroundRole(DPalette::Text);
//            pLabelList[1]->setPalette(pal);
//        }
//    }
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

    m_mainListWidget = new TimelineList;
    m_mainListWidget->setResizeMode(QListWidget::Adjust);
    m_mainListWidget->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    m_mainListWidget->verticalScrollBar()->setSingleStep(5);
    m_mainLayout->addWidget(m_mainListWidget);
    m_mainListWidget->setFrameShape(DTableView::NoFrame);

    //添加悬浮title

    //add start 3975
    m_pImportTitle = new DLabel(pTimeLineViewWidget);
    m_pImportTitle->setText(tr("Import"));
    m_pImportTitle->setContentsMargins(17, 0, 0, 0);
    DFontSizeManager::instance()->bind(m_pImportTitle, DFontSizeManager::T3, QFont::DemiBold);
    m_pImportTitle->setForegroundRole(DPalette::TextTitle);
    m_pImportTitle->setFixedSize(width() - 10, 47);
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
    TitleViewLayout->setContentsMargins(18, 0, 0, 0);
    m_dateItem->setLayout(TitleViewLayout);

    m_pDate = new DLabel();
    DFontSizeManager::instance()->bind(m_pDate, DFontSizeManager::T6, QFont::Medium);
    m_pDate->setForegroundRole(DPalette::TextTips);
//    QFont ft3 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
//    ft3.setFamily("SourceHanSansSC");
//    ft3.setWeight(QFont::DemiBold);
//    DPalette color = DApplicationHelper::instance()->palette(m_pDate);
//    color.setBrush(DPalette::Text, color.color(DPalette::ToolTipText));

//    m_pDate->setFixedHeight(24);
//    m_pDate->setFont(ft3);
//    m_pDate->setForegroundRole(DPalette::Text);
//    m_pDate->setPalette(color);

    pNum_up = new DLabel();
    DFontSizeManager::instance()->bind(pNum_up, DFontSizeManager::T6, QFont::Medium);
    pNum_up->setForegroundRole(DPalette::TextTips);
//    QFont ft6 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
//    ft6.setFamily("SourceHanSansSC");
//    ft6.setWeight(QFont::Medium);
//    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
//    DPalette pal = DApplicationHelper::instance()->palette(pNum_up);
//    QColor color_BT = pal.color(DPalette::BrightText);
//    if (themeType == DGuiApplicationHelper::LightType)
//    {
//        color_BT.setAlphaF(0.5);
//        pal.setBrush(DPalette::Text, color_BT);
//        pNum_up->setForegroundRole(DPalette::Text);
//        pNum_up->setPalette(pal);
//    }
//    else if (themeType == DGuiApplicationHelper::DarkType)
//    {
//        color_BT.setAlphaF(0.75);
//        pal.setBrush(DPalette::Text, color_BT);
//        pNum_up->setForegroundRole(DPalette::Text);
//        pNum_up->setPalette(pal);
//    }

//    pNum_up->setFixedHeight(24);
//    pNum_up->setFont(ft6);
//    pNum_up->setForegroundRole(DPalette::Text);
//    pNum_up->setPalette(pal);

    TitleViewLayout->addWidget(m_pDate);
    TitleViewLayout->addWidget(pNum_up);

    QHBoxLayout *Layout = new QHBoxLayout();
    pSuspensionChose = new DCommandLinkButton(QObject::tr("Select"));
    DFontSizeManager::instance()->bind(pSuspensionChose, DFontSizeManager::T5);
    pSuspensionChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    pSuspensionChose->setFixedHeight(32);
    pSuspensionChose->resize(36, 27);

    pNum_up->setLayout(Layout);
    Layout->addStretch(1);
    Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    Layout->setContentsMargins(0, 0, 12, 0);
    Layout->addWidget(pSuspensionChose);
    connect(pSuspensionChose, &DCommandLinkButton::clicked, this, [ = ] {
        if (QObject::tr("Select") == pSuspensionChose->text())
        {
            pSuspensionChose->setText(QObject::tr("Unselect"));
            QList<ThumbnailListView *> p = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<ThumbnailListView *>();
            p[0]->selectAll();
            emit sigUpdatePicNum();
            for (int i = 0; i < m_allChoseButton.length(); i++) {
                if (m_allThumbnailListView[i] == p[0]) {
                    lastClickedIndex = i;
                    lastRow = 0;
                    lastChanged = true;
                }
            }
        } else
        {
            pSuspensionChose->setText(QObject::tr("Select"));
            QList<ThumbnailListView *> p = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<ThumbnailListView *>();
            p[0]->clearSelection();
            emit sigUpdatePicNum();
        }
#if 1
        QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
        b[0]->setText(pSuspensionChose->text());
#endif
    });

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
    TimelineItem *blankWidget = new TimelineItem;
    blankWidget->m_type = "blank";
    QListWidgetItem *blankItem = new QListWidgetItem();
    blankItem->setFlags(Qt::NoItemFlags);
    m_mainListWidget->addItemForWidget(blankItem);
    m_mainListWidget->setItemWidget(blankItem, blankWidget);
//    blankItem->setSizeHint(QSize(width(), m_pImportTitle->height()));
    //add end 3975
//    updataLayout();
    addTimelineLayout();
}

//void ImportTimeLineView::updataLayout()
void ImportTimeLineView::addTimelineLayout()
{
    if (currentTimeLineLoad >= m_timelines.size()) {
        return;
    }
    int nowTimeLineLoad = currentTimeLineLoad;
//    for (int i = 0; i < m_timelines.size(); i++) {
    //获取当前时间照片
    DBImgInfoList ImgInfoList = DBManager::instance()->getInfosByImportTimeline(m_timelines.at(nowTimeLineLoad));

    QListWidgetItem *item = new QListWidgetItem;
    TimelineItem *listItem = new TimelineItem;
    listItem->adjustSize();
    QVBoxLayout *listItemlayout = new QVBoxLayout();
    listItem->setLayout(listItemlayout);
//        listItemlayout->setMargin(2);
//        listItemlayout->setSpacing(0);
    listItemlayout->setContentsMargins(0, 0, 0, 0);

    //添加title
    DWidget *TitleView = new DWidget;
    QHBoxLayout *TitleViewLayout = new QHBoxLayout();
    TitleViewLayout->setContentsMargins(10, 0, 0, 0);
    TitleView->setLayout(TitleViewLayout);
    DLabel *pDate = new DLabel();

    DFontSizeManager::instance()->bind(pDate, DFontSizeManager::T6, QFont::Medium);
    pDate->setForegroundRole(DPalette::TextTips);
//        pDate->setFixedHeight(24);
    QStringList dateTimeList = m_timelines.at(nowTimeLineLoad).split(" ");
    QStringList datelist = dateTimeList.at(0).split(".");
    if (datelist.count() > 2) {
        if (dateTimeList.count() == 2) {
            listItem->m_sdate = QString(QObject::tr("Import on ") + QObject::tr("%1/%2/%3 %4")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]).arg(dateTimeList[1]);
        } else {
            listItem->m_sdate = QString(QObject::tr("Import on ") + QObject::tr("%1/%2/%3")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]);
        }
    }
    pDate->setText(listItem->m_sdate);

//        DPalette color = DApplicationHelper::instance()->palette(pDate);
//        color.setBrush(DPalette::Text, color.color(DPalette::ToolTipText));

//        QFont ft3 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
//        ft3.setFamily("SourceHanSansSC");
//        ft3.setWeight(QFont::DemiBold);

//        pDate->setFont(ft3);
//        pDate->setForegroundRole(DPalette::Text);
//        pDate->setPalette(color);

    listItem->m_date = pDate;

    pNum_dn = new DLabel();
    listItem->m_snum = QString(QObject::tr("%1 photo(s)")).arg(ImgInfoList.size());
    pNum_dn->setText(listItem->m_snum);

    DFontSizeManager::instance()->bind(pNum_dn, DFontSizeManager::T6, QFont::Medium);
    pNum_dn->setForegroundRole(DPalette::TextTips);

//        QFont ft6 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
//        ft6.setFamily("SourceHanSansSC");
//        ft6.setWeight(QFont::Medium);
//        DPalette pal = DApplicationHelper::instance()->palette(pNum_dn);
//        QColor color_BT = pal.color(DPalette::BrightText);
//        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
//        if (themeType == DGuiApplicationHelper::LightType)
//        {
//            color_BT.setAlphaF(0.5);
//            pal.setBrush(DPalette::Text, color_BT);
//            pNum_dn->setForegroundRole(DPalette::Text);
//            pNum_dn->setPalette(pal);
//        }
//        else if (themeType == DGuiApplicationHelper::DarkType)
//        {
//            color_BT.setAlphaF(0.75);
//            pal.setBrush(DPalette::Text, color_BT);
//            pNum_dn->setForegroundRole(DPalette::Text);
//            pNum_dn->setPalette(pal);
//        }

//        pNum_dn->setFixedHeight(24);
//        pNum_dn->setFont(ft6);

    QHBoxLayout *Layout = new QHBoxLayout();
    DCommandLinkButton *pChose = new DCommandLinkButton(QObject::tr("Select"));
    m_allChoseButton << pChose;
    DFontSizeManager::instance()->bind(pChose, DFontSizeManager::T5);
    pChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
//    pChose->setFixedHeight(24);
    pChose->resize(36, 30);

    pNum_dn->setLayout(Layout);
    Layout->addStretch(1);
    Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    Layout->setContentsMargins(0, 0, 1, 0);
    Layout->addWidget(pChose);

    listItem->m_Chose = pChose;
    listItem->m_num = pNum_dn;
    TitleViewLayout->addWidget(pDate);
    TitleViewLayout->addWidget(pNum_dn);
    TitleView->setFixedHeight(SUBTITLE_HEIGHT);
    listItem->m_title = TitleView;

    //添加照片
    ThumbnailListView *pThumbnailListView = new ThumbnailListView(ThumbnailDelegate::NullType, COMMON_STR_RECENT_IMPORTED);
    int m_Baseheight =  getIBaseHeight();
    if (m_Baseheight == 0) {
        return;
    } else {
        pThumbnailListView->setIBaseHeight(m_Baseheight);
    }

    connect(pThumbnailListView, &ThumbnailListView::loadEnd, this, [ = ]() {
        addTimelineLayout();
    });
//        connect(pThumbnailListView, &ThumbnailListView::loadend, this, [ = ](int h) {
    connect(pThumbnailListView, &ThumbnailListView::needResize, this, [ = ](int h) {
//        return ;
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
//            listItem->setFixedHeight(TitleView->height() + mh);
//            item->setSizeHint(pThumbnailListView->size());
        }

    });

#if 1
    m_allThumbnailListView.append(pThumbnailListView);
#endif
    pThumbnailListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pThumbnailListView->setContextMenuPolicy(Qt::CustomContextMenu);
    pThumbnailListView->setContentsMargins(0, 0, 0, 0);
    pThumbnailListView->setFrameShape(DTableView::NoFrame);

//        using namespace utils::image;
//        QList<ThumbnailListView::ItemInfo> thumbnaiItemList;
//        for (int j = 0; j < ImgInfoList.size(); j++) {
//            ThumbnailListView::ItemInfo vi;
//            vi.name = ImgInfoList.at(j).fileName;
//            vi.path = ImgInfoList.at(j).filePath;
////            vi.image = dApp->m_imagemap.value(ImgInfoList.at(j).filePath);
//            if (dApp->m_imagemap.value(ImgInfoList.at(j).filePath).isNull()) {
//                QSize imageSize = getImageQSize(vi.path);

//                vi.width = imageSize.width();
//                vi.height = imageSize.height();
//            } else {
//                vi.width = dApp->m_imagemap.value(ImgInfoList.at(j).filePath).width();
//                vi.height = dApp->m_imagemap.value(ImgInfoList.at(j).filePath).height();
//            }

//            thumbnaiItemList.append(vi);
//        }
    //保存当前时间照片
//        pThumbnailListView->insertThumbnails(thumbnaiItemList);
    pThumbnailListView->loadFilesFromLocal(ImgInfoList);
    pThumbnailListView->m_imageType = COMMON_STR_RECENT_IMPORTED;

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

        auto photolist = DBManager::instance()->getAllInfos();

        if (paths.size() > 1) {
            info.paths = paths;
        } else {
            if (photolist.size() > 1) {
                for (auto image : photolist) {
                    info.paths << image.filePath;
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
//        for (int j = 0; j < m_allThumbnailListView.length(); j++) {
//            if (pThumbnailListView != m_allThumbnailListView[j]) {
//                m_allThumbnailListView[j]->clearSelection();
//            }
//        }
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
//                qDebug() << "lastClickedIndex" << lastClickedIndex << endl
//                         <<  "m_lastShiftClickedIndex"  << m_lastShiftClickedIndex << endl
//                         << lastRow << endl
//                         <<m_lastShiftRow;
//                if(m_lastShiftClickedIndex < lastClickedIndex){
//                    m_allThumbnailListView[m_lastShiftClickedIndex]->clearSelectionRear(m_lastShiftRow);
//                    m_allThumbnailListView[lastClickedIndex]->clearSelectionFront(lastRow);
//                    for(int i = m_lastShiftClickedIndex+1;i < lastClickedIndex;i++){
//                        m_allThumbnailListView[i]->clearSelection();
//                    }
//                }else if(m_lastShiftClickedIndex > lastClickedIndex){
//                    m_allThumbnailListView[m_lastShiftClickedIndex]->clearSelectionFront(m_lastShiftRow);
//                    m_allThumbnailListView[lastClickedIndex]->clearSelectionRear(lastRow);
//                    for(int i = lastClickedIndex+1;i < m_lastShiftClickedIndex;i++){
//                        m_allThumbnailListView[i]->clearSelection();
//                    }
//                }else if(m_lastShiftClickedIndex == lastClickedIndex){
//                    if(m_lastShiftRow <= lastRow)
//                        pThumbnailListView->clearSelectionExtent(m_lastShiftRow,lastRow);
//                    else
//                        pThumbnailListView->clearSelectionExtent(lastRow,m_lastShiftRow);
//                }
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
        for (int j = 0; j < m_allThumbnailListView.length(); j++)
        {
            m_allThumbnailListView[j]->selectAll();
        }
        emit sigUpdatePicNum();
        updateChoseText();
        QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
        pSuspensionChose->setText(b[0]->text());
    });

//        connect(pThumbnailListView, &ThumbnailListView::sigDrop, this, [ = ] {
//            for (int i = 0; i < m_allThumbnailListView.length(); i++)
//            {
//                if (pThumbnailListView != m_allThumbnailListView[i]) {
//                    m_allThumbnailListView[i]->clearSelection();
//                }
//            }
//            emit sigUpdatePicNum();
//            updateChoseText();
//        });

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
//    }

//    for (int j = 0; j < m_allThumbnailListView.size(); j++) {
    connect(m_allThumbnailListView[nowTimeLineLoad], &ThumbnailListView::sigKeyEvent, this, &ImportTimeLineView::on_KeyEvent);
//    }

    emit sigUpdatePicNum();
    currentTimeLineLoad++;
}

void ImportTimeLineView::getFatherStatusBar(DSlider *s)
{
    this->m_DSlider = s;
}

//void ImportTimeLineView::updataLayout()
//{
//    //获取所有时间线
//    m_mainListWidget->clear();
//    m_allChoseButton.clear();
//    m_timelines.clear();
//    m_timelines = DBManager::instance()->getImportTimelines();
//    qDebug() << __func__ << m_timelines.size();

//#if 1
//    m_allThumbnailListView.clear();
//#endif
//    if (0 < m_timelines.size()) {

//    } else {
//        m_dateItem->setVisible(false);
//    }
//    //add start 3975
//    TimelineItem *blankWidget = new TimelineItem;
//    blankWidget->m_type = "blank";
//    QListWidgetItem *blankItem = new QListWidgetItem();
//    blankItem->setFlags(Qt::NoItemFlags);
//    m_mainListWidget->addItemForWidget(blankItem);
//    m_mainListWidget->setItemWidget(blankItem, blankWidget);
//    blankItem->setSizeHint(QSize(width(), m_pImportTitle->height()));
//    //add end 3975
//    for (int i = 0; i < m_timelines.size(); i++) {
//        //获取当前时间照片
//        DBImgInfoList ImgInfoList = DBManager::instance()->getInfosByImportTimeline(m_timelines.at(i));

//        QListWidgetItem *item = new QListWidgetItem;
//        TimelineItem *listItem = new TimelineItem;
//        QVBoxLayout *listItemlayout = new QVBoxLayout();
//        listItem->setLayout(listItemlayout);
////        listItemlayout->setMargin(2);
////        listItemlayout->setSpacing(0);
//        listItemlayout->setContentsMargins(0, 0, 0, 0);

//        //添加title
//        DWidget *TitleView = new DWidget;
//        QHBoxLayout *TitleViewLayout = new QHBoxLayout();
//        TitleViewLayout->setContentsMargins(10, 0, 0, 0);
//        TitleView->setLayout(TitleViewLayout);
//        DLabel *pDate = new DLabel();

//        DFontSizeManager::instance()->bind(pDate, DFontSizeManager::T6, QFont::Medium);
//        pDate->setForegroundRole(DPalette::TextTips);
////        pDate->setFixedHeight(24);
//        QStringList dateTimeList = m_timelines.at(i).split(" ");
//        QStringList datelist = dateTimeList.at(0).split(".");
//        if (datelist.count() > 2) {
//            if (dateTimeList.count() == 2) {
//                listItem->m_sdate = QString(QObject::tr("Import on ") + QObject::tr("%1/%2/%3 %4")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]).arg(dateTimeList[1]);
//            } else {
//                listItem->m_sdate = QString(QObject::tr("Import on ") + QObject::tr("%1/%2/%3")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]);
//            }
//        }
//        pDate->setText(listItem->m_sdate);

////        DPalette color = DApplicationHelper::instance()->palette(pDate);
////        color.setBrush(DPalette::Text, color.color(DPalette::ToolTipText));

////        QFont ft3 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
////        ft3.setFamily("SourceHanSansSC");
////        ft3.setWeight(QFont::DemiBold);

////        pDate->setFont(ft3);
////        pDate->setForegroundRole(DPalette::Text);
////        pDate->setPalette(color);

//        listItem->m_date = pDate;

//        pNum_dn = new DLabel();
//        listItem->m_snum = QString(QObject::tr("%1 photo(s)")).arg(ImgInfoList.size());
//        pNum_dn->setText(listItem->m_snum);

//        DFontSizeManager::instance()->bind(pNum_dn, DFontSizeManager::T6, QFont::Medium);
//        pNum_dn->setForegroundRole(DPalette::TextTips);

////        QFont ft6 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
////        ft6.setFamily("SourceHanSansSC");
////        ft6.setWeight(QFont::Medium);
////        DPalette pal = DApplicationHelper::instance()->palette(pNum_dn);
////        QColor color_BT = pal.color(DPalette::BrightText);
////        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
////        if (themeType == DGuiApplicationHelper::LightType)
////        {
////            color_BT.setAlphaF(0.5);
////            pal.setBrush(DPalette::Text, color_BT);
////            pNum_dn->setForegroundRole(DPalette::Text);
////            pNum_dn->setPalette(pal);
////        }
////        else if (themeType == DGuiApplicationHelper::DarkType)
////        {
////            color_BT.setAlphaF(0.75);
////            pal.setBrush(DPalette::Text, color_BT);
////            pNum_dn->setForegroundRole(DPalette::Text);
////            pNum_dn->setPalette(pal);
////        }

////        pNum_dn->setFixedHeight(24);
////        pNum_dn->setFont(ft6);

//        QHBoxLayout *Layout = new QHBoxLayout();
//        DCommandLinkButton *pChose = new DCommandLinkButton(QObject::tr("Select"));
//        m_allChoseButton << pChose;
//        DFontSizeManager::instance()->bind(pChose, DFontSizeManager::T5);
//        pChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
//        pChose->setFixedHeight(24);
//        pChose->resize(36, 27);

//        pNum_dn->setLayout(Layout);
//        Layout->addStretch(1);
//        Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
//        Layout->setContentsMargins(0, 0, 1, 0);
//        Layout->addWidget(pChose);

//        listItem->m_Chose = pChose;
//        listItem->m_num = pNum_dn;
//        TitleViewLayout->addWidget(pDate);
//        TitleViewLayout->addWidget(pNum_dn);
//        TitleView->setFixedHeight(SUBTITLE_HEIGHT);
//        listItem->m_title = TitleView;

//        //添加照片
//        ThumbnailListView *pThumbnailListView = new ThumbnailListView(ThumbnailDelegate::NullType, COMMON_STR_RECENT_IMPORTED);
////        connect(pThumbnailListView, &ThumbnailListView::loadend, this, [ = ](int h) {
//        connect(pThumbnailListView, &ThumbnailListView::needResize, this, [ = ](int h) {
//            if (isVisible()) {
//                int mh = h;
//                if (0 == i) {
//                    mh += 50;
//                }
//                if (i == m_timelines.size() - 1) {
//                    mh += 27;
//                }
//                pThumbnailListView->setFixedHeight(mh);
//                listItem->setFixedHeight(TitleView->height() + mh);
//                item->setSizeHint(listItem->rect().size());
////                emit albumviewResize();
////                setFixedSize(QSize(size().width() + 1, size().height()));
////                setFixedSize(QSize(size().width() - 1, size().height())); //触发resizeevent
////                setMinimumSize(0, 0);
////                setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));  //触发后还原状态
//            }

//        });

//#if 1
//        m_allThumbnailListView.append(pThumbnailListView);
//#endif
//        pThumbnailListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
//        pThumbnailListView->setContextMenuPolicy(Qt::CustomContextMenu);
//        pThumbnailListView->setContentsMargins(0, 0, 0, 0);
//        pThumbnailListView->setFrameShape(DTableView::NoFrame);

////        using namespace utils::image;
////        QList<ThumbnailListView::ItemInfo> thumbnaiItemList;
////        for (int j = 0; j < ImgInfoList.size(); j++) {
////            ThumbnailListView::ItemInfo vi;
////            vi.name = ImgInfoList.at(j).fileName;
////            vi.path = ImgInfoList.at(j).filePath;
//////            vi.image = dApp->m_imagemap.value(ImgInfoList.at(j).filePath);
////            if (dApp->m_imagemap.value(ImgInfoList.at(j).filePath).isNull()) {
////                QSize imageSize = getImageQSize(vi.path);

////                vi.width = imageSize.width();
////                vi.height = imageSize.height();
////            } else {
////                vi.width = dApp->m_imagemap.value(ImgInfoList.at(j).filePath).width();
////                vi.height = dApp->m_imagemap.value(ImgInfoList.at(j).filePath).height();
////            }

////            thumbnaiItemList.append(vi);
////        }
//        //保存当前时间照片
////        pThumbnailListView->insertThumbnails(thumbnaiItemList);
//        pThumbnailListView->loadFilesFromLocal(ImgInfoList);
//        pThumbnailListView->m_imageType = COMMON_STR_RECENT_IMPORTED;

//        if (0 == i) {
//            DWidget *topwidget = new DWidget;
//            topwidget->setFixedHeight(50);
//            listItemlayout->addWidget(topwidget);
//        }
//        listItemlayout->addWidget(TitleView);
//        listItemlayout->addSpacing(-8);
//        listItemlayout->addWidget(pThumbnailListView);
//        if (i == m_timelines.size() - 1) {
//            DWidget *bottomwidget = new DWidget;
//            bottomwidget->setFixedHeight(27);
//            listItemlayout->addWidget(bottomwidget);
//        }
//        item->setFlags(Qt::NoItemFlags);
//        m_mainListWidget->addItemForWidget(item);
//        m_mainListWidget->setItemWidget(item, listItem);
//        connect(pThumbnailListView, &ThumbnailListView::openImage, this, [ = ](int index) {
//            SignalManager::ViewInfo info;
//            info.album = "";
//            info.lastPanel = nullptr;
//            if (ImgInfoList.size() <= 1) {
//                info.paths.clear();
//            } else {
//                for (auto image : ImgInfoList) {
//                    info.paths << image.filePath;
//                }
//            }
//            info.path = ImgInfoList[index].filePath;
//            info.viewType = COMMON_STR_RECENT_IMPORTED;
//            info.viewMainWindowID = VIEW_MAINWINDOW_ALBUM;
//            emit dApp->signalM->viewImage(info);
//            emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALBUM);
//        });
//        connect(pThumbnailListView, &ThumbnailListView::menuOpenImage, this, [ = ](QString path, QStringList paths, bool isFullScreen, bool isSlideShow) {
//            SignalManager::ViewInfo info;
//            info.album = "";
//            info.lastPanel = nullptr;

//            auto photolist = DBManager::instance()->getAllInfos();

//            if (paths.size() > 1) {
//                info.paths = paths;
//            } else {
//                if (photolist.size() > 1) {
//                    for (auto image : photolist) {
//                        info.paths << image.filePath;
//                    }
//                } else {
//                    info.paths.clear();
//                }
//            }
//            info.path = path;
//            info.fullScreen = isFullScreen;
//            info.slideShow = isSlideShow;
//            info.viewType = COMMON_STR_RECENT_IMPORTED;
//            info.viewMainWindowID = VIEW_MAINWINDOW_ALBUM;
//            if (info.slideShow) {
//                if (ImgInfoList.count() == 1) {
//                    info.paths = paths;
//                }

//                QStringList pathlist;
//                pathlist.clear();
//                for (auto path : info.paths) {
//                    if (QFileInfo(path).exists()) {
//                        pathlist << path;
//                    }
//                }

//                info.paths = pathlist;
//                emit dApp->signalM->startSlideShow(info);
//                emit dApp->signalM->showSlidePanel(VIEW_MAINWINDOW_ALBUM);
//            } else {
//                emit dApp->signalM->viewImage(info);
//                emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALBUM);
//            }
//        });
//        connect(pChose, &DCommandLinkButton::clicked, this, [ = ] {
//            if (QObject::tr("Select") == pChose->text())
//            {
//                pChose->setText(QObject::tr("Unselect"));
//                pThumbnailListView->selectAll();
//                for (int j = 0; j < m_allChoseButton.length(); j++) {
//                    if (pChose == m_allChoseButton[j])
//                        lastClickedIndex = j;
//                }
//                lastRow = 0;
//                lastChanged = true;
//            } else
//            {
//                pChose->setText(QObject::tr("Select"));
//                pThumbnailListView->clearSelection();
//            }
//            emit sigUpdatePicNum();
//        });
//#if 1

//        connect(pThumbnailListView, &ThumbnailListView::sigMousePress, this, [ = ](QMouseEvent * event) {
//            lastRow = -1;
//            for (int j = 0; j < m_allThumbnailListView.length(); j++) {
//                if (pThumbnailListView == m_allThumbnailListView[j]) {
//                    lastClickedIndex = j;
//                    lastRow = pThumbnailListView->getRow(QPoint(event->x(), event->y()));
//                    if (-1 != lastRow)
//                        lastChanged = true;
//                }
//            }
//        });

//        connect(pThumbnailListView, &ThumbnailListView::sigShiftMousePress, this, [ = ](QMouseEvent * event) {
//            int curClickedIndex = -1;
//            int curRow = -1;
//            for (int j = 0; j < m_allThumbnailListView.length(); j++) {
//                if (pThumbnailListView == m_allThumbnailListView[j]) {
//                    curClickedIndex = j;
//                    curRow = pThumbnailListView->getRow(QPoint(event->x(), event->y()));
//                }
//            }

//            if (!lastChanged && -1 != curRow && -1 != m_lastShiftRow) {
//                for (int j = 0; j < m_allThumbnailListView.length(); j++) {
//                    m_allThumbnailListView[j]->clearSelection();
//                }
////                qDebug() << "lastClickedIndex" << lastClickedIndex << endl
////                         <<  "m_lastShiftClickedIndex"  << m_lastShiftClickedIndex << endl
////                         << lastRow << endl
////                         <<m_lastShiftRow;
////                if(m_lastShiftClickedIndex < lastClickedIndex){
////                    m_allThumbnailListView[m_lastShiftClickedIndex]->clearSelectionRear(m_lastShiftRow);
////                    m_allThumbnailListView[lastClickedIndex]->clearSelectionFront(lastRow);
////                    for(int i = m_lastShiftClickedIndex+1;i < lastClickedIndex;i++){
////                        m_allThumbnailListView[i]->clearSelection();
////                    }
////                }else if(m_lastShiftClickedIndex > lastClickedIndex){
////                    m_allThumbnailListView[m_lastShiftClickedIndex]->clearSelectionFront(m_lastShiftRow);
////                    m_allThumbnailListView[lastClickedIndex]->clearSelectionRear(lastRow);
////                    for(int i = lastClickedIndex+1;i < m_lastShiftClickedIndex;i++){
////                        m_allThumbnailListView[i]->clearSelection();
////                    }
////                }else if(m_lastShiftClickedIndex == lastClickedIndex){
////                    if(m_lastShiftRow <= lastRow)
////                        pThumbnailListView->clearSelectionExtent(m_lastShiftRow,lastRow);
////                    else
////                        pThumbnailListView->clearSelectionExtent(lastRow,m_lastShiftRow);
////                }
//            }

//            if (curRow == -1 || lastRow == -1) {
//                for (int j = 0; j < m_allThumbnailListView.length(); j++) {
//                    m_allThumbnailListView[j]->clearSelection();
//                }
//            } else {
//                if (lastClickedIndex < curClickedIndex) {
//                    m_allThumbnailListView[lastClickedIndex]->selectRear(lastRow);
//                    m_allThumbnailListView[curClickedIndex]->selectFront(curRow);
//                    for (int j = lastClickedIndex + 1; j < curClickedIndex; j++) {
//                        m_allThumbnailListView[j]->selectAll();
//                    }
//                } else if (lastClickedIndex > curClickedIndex) {
//                    m_allThumbnailListView[lastClickedIndex]->selectFront(lastRow);
//                    m_allThumbnailListView[curClickedIndex]->selectRear(curRow);
//                    for (int j = curClickedIndex + 1; j < lastClickedIndex; j++) {
//                        m_allThumbnailListView[j]->selectAll();
//                    }
//                } else if (lastClickedIndex == curClickedIndex) {
//                    if (lastRow <= curRow)
//                        pThumbnailListView->selectExtent(lastRow, curRow);
//                    else
//                        pThumbnailListView->selectExtent(curRow, lastRow);
//                }
//                emit sigUpdatePicNum();
//                updateChoseText();
//                m_lastShiftRow = curRow;
//                m_lastShiftClickedIndex = curClickedIndex;
//                curRow = -1;
//                lastChanged = false;
//            }
//        });

//        connect(pThumbnailListView, &ThumbnailListView::sigCtrlMousePress, this, [ = ](QMouseEvent * event) {
//            for (int j = 0; j < m_allThumbnailListView.length(); j++) {
//                if (pThumbnailListView == m_allThumbnailListView[j]) {
//                    lastClickedIndex = j;
//                    lastRow = pThumbnailListView->getRow(QPoint(event->x(), event->y()));
//                    if (-1 != lastRow)
//                        lastChanged = true;
//                }
//            }
//            emit sigUpdatePicNum();
//            updateChoseText();
//        });

//        connect(pThumbnailListView, &ThumbnailListView::sigGetSelectedPaths, this, [ = ](QStringList * pPaths) {
//            pPaths->clear();
//            for (int j = 0; j < m_allThumbnailListView.size(); j++) {
//                pPaths->append(m_allThumbnailListView[j]->selectedPaths());
//            }
//        });

//        connect(pThumbnailListView, &ThumbnailListView::sigSelectAll, this, [ = ] {
//            for (int j = 0; j < m_allThumbnailListView.length(); j++)
//            {
//                m_allThumbnailListView[j]->selectAll();
//            }
//            emit sigUpdatePicNum();
//            updateChoseText();
//            QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
//            pSuspensionChose->setText(b[0]->text());
//        });

////        connect(pThumbnailListView, &ThumbnailListView::sigDrop, this, [ = ] {
////            for (int i = 0; i < m_allThumbnailListView.length(); i++)
////            {
////                if (pThumbnailListView != m_allThumbnailListView[i]) {
////                    m_allThumbnailListView[i]->clearSelection();
////                }
////            }
////            emit sigUpdatePicNum();
////            updateChoseText();
////        });

//        connect(pThumbnailListView, &ThumbnailListView::sigMouseMove, this, [ = ] {
//            emit sigUpdatePicNum();
//            updateChoseText();
//        });

//        connect(pThumbnailListView, &ThumbnailListView::sigMouseRelease, this, [ = ] {
//            if (!m_ctrlPress)
//            {
//                for (int j = 0; j < m_allThumbnailListView.length(); j++) {
//                    if (pThumbnailListView != m_allThumbnailListView[j]) {
//                        m_allThumbnailListView[j]->clearSelection();
//                    }
//                }
//            }
//            emit sigUpdatePicNum();
//            updateChoseText();
//            QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
//            pSuspensionChose->setText(b[0]->text());
//        });

//        connect(pThumbnailListView, &ThumbnailListView::customContextMenuRequested, this, [ = ] {
//            QStringList paths = pThumbnailListView->selectedPaths();
//            if (pThumbnailListView->model()->rowCount() == paths.length() && QObject::tr("Select") == pChose->text())
//            {
//                pChose->setText(QObject::tr("Unselect"));
//            }

//            if (pThumbnailListView->model()->rowCount() != paths.length() && QObject::tr("Unselect") == pChose->text())
//            {
//                pChose->setText(QObject::tr("Select"));
//            }
//            emit sigUpdatePicNum();
//        });
//        connect(pThumbnailListView, &ThumbnailListView::sigMenuItemDeal, this, [ = ](QAction * action) {
//            QStringList paths;
//            paths.clear();
//            for (int j = 0; j < m_allThumbnailListView.size(); j++) {
//                paths << m_allThumbnailListView[i]->selectedPaths();
//            }
//            pThumbnailListView->menuItemDeal(paths, action);
//        });

//        connect(listItem, &TimelineItem::sigMousePress, this, [ = ] {
//            for (int j = 0; j < m_allThumbnailListView.length(); j++)
//            {
//                m_allThumbnailListView[j]->clearSelection();
//            }
//            lastRow = -1;
//            emit sigUpdatePicNum();
//            updateChoseText();
//        });
//#endif
//    }

//    for (int j = 0; j < m_allThumbnailListView.size(); j++) {
//        connect(m_allThumbnailListView[j], &ThumbnailListView::sigKeyEvent, this, &ImportTimeLineView::on_KeyEvent);
//    }

//    emit sigUpdatePicNum();
//}

void ImportTimeLineView::on_AddLabel(QString date, QString num)
{
    if ((nullptr != m_dateItem) && (nullptr != m_mainListWidget)) {
        QList<QLabel *> labelList = m_dateItem->findChildren<QLabel *>();
//        QList<DCommandLinkButton*> buttonList = m_dateItem->findChildren<DCommandLinkButton*>();
        labelList[0]->setText(date);
        labelList[1]->setText(num);
        m_dateItem->setVisible(true);

//        QWidget * pwidget = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index));
//        QList<DCommandLinkButton*> buttonList1 = pwidget->findChildren<DCommandLinkButton*>();
//        if(buttonList.count() > 0 && buttonList1.count() > 0)
//        {
//            buttonList.at(0)->setText(buttonList1.at(0)->text());
//        }

        m_dateItem->move(0, 50 + m_pImportTitle->height()); //edit 3975
    }
#if 1
    QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
    pSuspensionChose->setText(b[0]->text());
#endif
}

void ImportTimeLineView::on_DelLabel()
{
    if (nullptr != m_dateItem) {
        m_dateItem->setVisible(false);
    }
#if 1
    QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
    pSuspensionChose->setText(b[0]->text());
#endif
}

#if 1
void ImportTimeLineView::on_MoveLabel(int y, QString date, QString num, QString choseText)
#endif
{
    if ((nullptr != m_dateItem) && (nullptr != m_mainListWidget)) {
        QList<QLabel *> labelList = m_dateItem->findChildren<QLabel *>();
        labelList[0]->setText(date);
        labelList[1]->setText(num);
        pSuspensionChose->setText(choseText);
        m_dateItem->setVisible(true);
//        m_dateItem->move(0, y + 1); //del 3975
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
    m_dateItem->setFixedSize(width() - 15, SUBTITLE_HEIGHT);
    m_pImportTitle->setFixedSize(width() - 15, 47); //add 3975
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

//    if (paths.isEmpty()) {
//        return;
//    }

//    // 判断当前导入路径是否为外接设备
//    int isMountFlag = 0;
//    DGioVolumeManager *pvfsManager = new DGioVolumeManager;
//    QList<QExplicitlySharedDataPointer<DGioMount>> mounts = pvfsManager->getMounts();
//    for (auto mount : mounts) {
//        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
//        QString strPath = LocationFile->path();
//        if (0 == paths.first().compare(strPath)) {
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
//        foreach (QString strPath, paths) {
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

//        paths.clear();
//        paths = newImagePaths;
//    }

//    DBImgInfoList dbInfos;

//    using namespace utils::image;

//    for (auto path : paths) {
//        if (! imageSupportRead(path)) {
//            continue;
//        }

//        QFileInfo fi(path);
//        using namespace utils::image;
//        using namespace utils::base;
//        auto mds = getAllMetaData(path);
//        QString value = mds.value("DateTimeOriginal");
////        qDebug() << value;
//        DBImgInfo dbi;
//        dbi.fileName = fi.fileName();
//        dbi.filePath = path;
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
//        dApp->m_imageloader->ImportImageLoader(dbInfos);

//    } else {
//        emit dApp->signalM->ImportFailed();
//    }

    event->accept();
}

void ImportTimeLineView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void ImportTimeLineView::dragLeaveEvent(QDragLeaveEvent *e)
{

}

void ImportTimeLineView::keyPressEvent(QKeyEvent *e)
{
    qDebug() << "ImportTimeLineView::keyPressEvent()";
    if (e->key() == Qt::Key_Control) {
        m_ctrlPress = true;
    }
}

void ImportTimeLineView::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Control) {
        m_ctrlPress = false;
    }
}

void ImportTimeLineView::mousePressEvent(QMouseEvent *e)
{
    if (!m_ctrlPress && e->button() == Qt::LeftButton) {
        for (int i = 0; i < m_allThumbnailListView.length(); i++) {
            m_allThumbnailListView[i]->clearSelection();
        }
        emit sigUpdatePicNum();
        updateChoseText();
    }
    DWidget::mousePressEvent(e);
}
