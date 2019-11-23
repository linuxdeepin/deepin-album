#include "timelineview.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/snifferimageformat.h"
#include <QScrollBar>
#include <QScroller>
#include <DPushButton>
#include <QMimeData>
#include <DTableView>
#include <QGraphicsOpacityEffect>

namespace  {
const int VIEW_IMPORT = 0;
const int VIEW_TIMELINE = 1;
const int VIEW_SEARCH = 2;
const int VIEW_MAINWINDOW_TIMELINE = 1;
} //namespace

TimeLineView::TimeLineView()
{
    setAcceptDrops(true);
    m_index = 0;

    m_oe = new QGraphicsOpacityEffect;
    m_oet = new QGraphicsOpacityEffect;
    m_oe->setOpacity(0.5);
    m_oet->setOpacity(0.75);

    m_pStackedWidget = new DStackedWidget();

    pTimeLineViewWidget = new DWidget();
    pImportView = new ImportView();
    pSearchView = new SearchView();

    m_pStackedWidget->addWidget(pImportView);
    m_pStackedWidget->addWidget(pTimeLineViewWidget);
    m_pStackedWidget->addWidget(pSearchView);

    m_pStatusBar = new StatusBar();
    m_pStatusBar->setParent(this);

    QVBoxLayout *pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins(0, 0, 0, 0);
    pVBoxLayout->addWidget(m_pStackedWidget);
    pVBoxLayout->addWidget(m_pStatusBar);
    this->setLayout(pVBoxLayout);

    initTimeLineViewWidget();

    updataLayout();

    initConnections();
}

void TimeLineView::initConnections()
{
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &TimeLineView::updataLayout);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &TimeLineView::updataLayout);
    connect(dApp, &Application::sigFinishLoad, this, &TimeLineView::updataLayout);
    connect(m_mainListWidget, &TimelineList::sigNewTime, this, [ = ](QString date, QString num, int index) {
        m_index = index;
        on_AddLabel(date, num);
    });

    connect(m_mainListWidget, &TimelineList::sigDelTime, this, [ = ]() {
        on_DelLabel();
    });

    connect(m_mainListWidget, &TimelineList::sigMoveTime, this, [ = ](int y) {
        on_MoveLabel(y);
    });

    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &TimeLineView::updataLayout);
    connect(m_pStatusBar->m_pSlider, &DSlider::valueChanged, dApp->signalM, &SignalManager::sigMainwindowSliderValueChg);

    connect(pSearchView->m_pThumbnailListView, &ThumbnailListView::clicked, this, &TimeLineView::updatePicNum);
    connect(pSearchView->m_pThumbnailListView, &ThumbnailListView::sigTimeLineItemBlankArea, this, &TimeLineView::restorePicNum);

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &TimeLineView::themeChangeSlot);

//    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, pLabel,[=]{
//        QPixmap pixmap;
//        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
//        if (themeType == DGuiApplicationHelper::LightType)
//        {
//            pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo.svg", QSize(128, 128));
//        }
//        if (themeType == DGuiApplicationHelper::DarkType)
//        {
//            pixmap = utils::base::renderSVG(":/resources/images/other/icon_import_photo_dark.svg", QSize(128, 128));
//        }
//        pLabel->setPixmap(pixmap);
//    });
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

    for (int i = 1; i < m_mainListWidget->count(); i++) {
        TimelineItem *item = (TimelineItem *)m_mainListWidget->itemWidget(m_mainListWidget->item(i));
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

void TimeLineView::initTimeLineViewWidget()
{
    m_mainLayout = new QVBoxLayout();
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    pTimeLineViewWidget->setLayout(m_mainLayout);

    DPalette palcolor = DApplicationHelper::instance()->palette(pTimeLineViewWidget);
    palcolor.setBrush(DPalette::Window, palcolor.color(DPalette::Base));
    pTimeLineViewWidget->setPalette(palcolor);

    m_mainListWidget = new TimelineList;
    m_mainListWidget->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    m_mainListWidget->verticalScrollBar()->setSingleStep(5);
    m_mainLayout->addWidget(m_mainListWidget);

    //添加悬浮title
    m_dateItem = new QWidget(pTimeLineViewWidget);
    QVBoxLayout *TitleViewLayout = new QVBoxLayout();
    m_dateItem->setLayout(TitleViewLayout);

    m_pDate = new DLabel();
    QFont ft3 = DFontSizeManager::instance()->get(DFontSizeManager::T3);
    ft3.setFamily("SourceHanSansSC");
    ft3.setWeight(QFont::DemiBold);
    DPalette color = DApplicationHelper::instance()->palette(m_pDate);
    color.setBrush(DPalette::Text, color.color(DPalette::ToolTipText));

    m_pDate->setFixedHeight(24);
    m_pDate->setFont(ft3);
    m_pDate->setForegroundRole(DPalette::Text);
    m_pDate->setPalette(color);

    pNum_up = new DLabel();
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

    pNum_up->setFixedHeight(24);
    pNum_up->setFont(ft6);
    pNum_up->setForegroundRole(DPalette::Text);
    pNum_up->setPalette(pal);

    TitleViewLayout->addWidget(m_pDate);
    TitleViewLayout->addWidget(pNum_up);

    QHBoxLayout *Layout = new QHBoxLayout();
//    pSuspensionChose = new DCommandLinkButton("选择");
    pSuspensionChose = new DCommandLinkButton(QObject::tr("Select"));
    pSuspensionChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    pSuspensionChose->setFixedHeight(24);
    pSuspensionChose->resize(36, 27);

    pNum_up->setLayout(Layout);
    Layout->addStretch(1);
    Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    Layout->setContentsMargins(0, 0, 22, 0);
    Layout->addWidget(pSuspensionChose);
    connect(pSuspensionChose, &DCommandLinkButton::clicked, this, [ = ] {
//        if ("选择" == pSuspensionChose->text())
        if (QObject::tr("Select") == pSuspensionChose->text())
        {
//            pSuspensionChose->setText("取消选择");
            pSuspensionChose->setText(QObject::tr("Unselect"));
            QList<ThumbnailListView *> p = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<ThumbnailListView *>();
            p[0]->selectAll();
        } else
        {
//            pSuspensionChose->setText("选择");
            pSuspensionChose->setText(QObject::tr("Select"));
            QList<ThumbnailListView *> p = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<ThumbnailListView *>();
            p[0]->clearSelection();
        }
    });

//    DPalette ppal_light = DApplicationHelper::instance()->palette(m_dateItem);
//    ppal_light.setBrush(DPalette::Background, ppal_light.color(DPalette::Base));
//    DPalette ppal_dark = DApplicationHelper::instance()->palette(m_dateItem);
//    ppal_dark.setBrush(DPalette::Background, ppal_dark.color(DPalette::Window));
//    QGraphicsOpacityEffect *opacityEffect_light = new QGraphicsOpacityEffect;
//    opacityEffect_light->setOpacity(0.95);
//    QGraphicsOpacityEffect *opacityEffect_dark = new QGraphicsOpacityEffect;
//    opacityEffect_dark->setOpacity(0.8);

//    if (themeType == DGuiApplicationHelper::LightType)
//    {
//        m_dateItem->setPalette(ppal_light);
//        m_dateItem->setGraphicsEffect(opacityEffect_light);
//    }
//    else if (themeType == DGuiApplicationHelper::LightType)
//    {
//        m_dateItem->setPalette(ppal_dark);
//        m_dateItem->setGraphicsEffect(opacityEffect_dark);
//    }

    DPalette ppal_light = DApplicationHelper::instance()->palette(m_dateItem);
    ppal_light.setBrush(DPalette::Background, ppal_light.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light = new QGraphicsOpacityEffect;
    opacityEffect_light->setOpacity(0.95);
    m_dateItem->setPalette(ppal_light);
    m_dateItem->setGraphicsEffect(opacityEffect_light);
    m_dateItem->setAutoFillBackground(true);
    m_dateItem->setFixedSize(this->width(), 87);
    m_dateItem->setContentsMargins(10, 0, 0, 0);
    m_dateItem->move(0, 0);
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

void TimeLineView::updataLayout()
{
    //获取所有时间线
    m_mainListWidget->clear();
    m_timelines = DBManager::instance()->getAllTimelines();
    qDebug() << m_timelines.size();

#if 1
    m_allThumbnailListView.clear();
#endif
    for (int i = 0; i < m_timelines.size(); i++) {
        //获取当前时间照片
        DBImgInfoList ImgInfoList = DBManager::instance()->getInfosByTimeline(m_timelines.at(i));

        QListWidgetItem *item = new QListWidgetItem;
        TimelineItem *listItem = new TimelineItem;
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

        pDate->setFixedHeight(24);
        QStringList datelist = m_timelines.at(i).split(".");
        if (datelist.count() > 2) {
//            listItem->m_sdate=QString("%1年%2月%3日").arg(datelist[0]).arg(datelist[1]).arg(datelist[2]);
            listItem->m_sdate = QString("%1年%2月%3日").arg(datelist[0]).arg(datelist[1]).arg(datelist[2]);
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
//        listItem->m_snum = QString("%1张照片").arg(ImgInfoList.size());
        listItem->m_snum = QString(tr("%1 Photos")).arg(ImgInfoList.size());
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

        pNum_dn->setFixedHeight(24);
        pNum_dn->setFont(ft6);

        QHBoxLayout *Layout = new QHBoxLayout();
//        DCommandLinkButton *pChose = new DCommandLinkButton("选择");
        DCommandLinkButton *pChose = new DCommandLinkButton(QObject::tr("Select"));

        pChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
        pChose->setFixedHeight(24);
        pChose->resize(36, 27);

        pNum_dn->setLayout(Layout);
        Layout->addStretch(1);
        Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        Layout->setContentsMargins(0, 0, 0, 0);
        Layout->addWidget(pChose);

        listItem->m_Chose = pChose;
        listItem->m_num = pNum_dn;
        TitleViewLayout->addWidget(pDate);
        TitleViewLayout->addWidget(pNum_dn);
        TitleView->setFixedHeight(87);
        listItem->m_title = TitleView;

        //添加照片
        ThumbnailListView *pThumbnailListView = new ThumbnailListView(COMMON_STR_VIEW_TIMELINE);

#if 1
        m_allThumbnailListView.append(pThumbnailListView);
#endif
        pThumbnailListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        pThumbnailListView->setContextMenuPolicy(Qt::CustomContextMenu);
        pThumbnailListView->setContentsMargins(0, 0, 0, 0);
        pThumbnailListView->setFrameShape(DTableView::NoFrame);


        QList<ThumbnailListView::ItemInfo> thumbnaiItemList;
        for (int j = 0; j < ImgInfoList.size(); j++) {
            ThumbnailListView::ItemInfo vi;
            vi.name = ImgInfoList.at(j).fileName;
            vi.path = ImgInfoList.at(j).filePath;
            vi.image = dApp->m_imagemap.value(ImgInfoList.at(j).filePath);
            thumbnaiItemList.append(vi);
        }
        //保存当前时间照片
        pThumbnailListView->insertThumbnails(thumbnaiItemList);

        listItemlayout->addWidget(TitleView);
        listItemlayout->addWidget(pThumbnailListView);
        item->setFlags(Qt::NoItemFlags);
        m_mainListWidget->addItemForWidget(item);
        m_mainListWidget->setItemWidget(item, listItem);
        connect(pThumbnailListView, &ThumbnailListView::loadend, this, [ = ](int h) {
            if (isVisible()) {
                pThumbnailListView->setFixedHeight(h);
                listItem->setFixedHeight(TitleView->height() + h);
                item->setSizeHint(listItem->rect().size());
            }

        });
        connect(pThumbnailListView, &ThumbnailListView::openImage, this, [ = ](int index) {
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
//            info.path = path;
//            info.fullScreen = isFullScreen;
//            info.slideShow = isSlideShow;
//            info.viewType = utils::common::VIEW_TIMELINE_SRN;

//            if (info.slideShow) {
//                if (ImgInfoList.count() == 1) {
//                    info.paths = paths;
//                }
//                emit dApp->signalM->startSlideShow(info);
//            } else {
//                emit dApp->signalM->viewImage(info);
//            }
//            emit dApp->signalM->showImageView(VIEW_MAINWINDOW_TIMELINE);

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

            emit dApp->signalM->viewImage(info);
            emit dApp->signalM->showImageView(VIEW_MAINWINDOW_TIMELINE);


        });
        connect(pChose, &DCommandLinkButton::clicked, this, [ = ] {
//           if ("选择" == pChose->text())
            if (QObject::tr("Select") == pChose->text())
            {
//               pChose->setText("取消选择");
                pChose->setText(QObject::tr("Unselect"));
                pThumbnailListView->selectAll();
            } else
            {
//               pChose->setText("选择");
                pChose->setText(QObject::tr("Select"));
                pThumbnailListView->clearSelection();
            }
            updatePicNum();
        });
        connect(pThumbnailListView, &ThumbnailListView::clicked, this, [ = ] {
            QStringList paths = pThumbnailListView->selectedPaths();
//            if (pThumbnailListView->model()->rowCount() == paths.length() && "选择" == pChose->text())
            if (pThumbnailListView->model()->rowCount() == paths.length() && QObject::tr("Select") == pChose->text())
            {
//                pChose->setText("取消选择");
                pChose->setText(QObject::tr("Unselect"));

//                QList<DCommandLinkButton*> btnList = m_dateItem->findChildren<DCommandLinkButton*>();
//                if( = m_dateItem)
//                {
//                    btnList[0]->setText("取消选择");
//                }


            }

//            if (pThumbnailListView->model()->rowCount() != paths.length() && "取消选择" == pChose->text())
            if (pThumbnailListView->model()->rowCount() != paths.length() && QObject::tr("Unselect") == pChose->text())
            {
//                pChose->setText("选择");
                pChose->setText(QObject::tr("Select"));
            }

            selpicQmap.insert(pThumbnailListView, paths);
            allnum = 0;
            for (auto key : selpicQmap.keys())
            {
                allnum = allnum + selpicQmap.value(key).length();
            }

            if (0 == allnum)
            {
                m_pStatusBar->onUpdateAllpicsNumLabel();
            } else
            {
//                QString str = tr("已选择%1张照片");
                QString str1 = QObject::tr("Selected %1 photos");
                m_pStatusBar->m_pAllPicNumLabel->setText(str1.arg(allnum));
            }
        });

#if 1
        connect(pThumbnailListView, &ThumbnailListView::sigMouseRelease, this, [ = ] {
            QStringList paths = pThumbnailListView->selectedPaths();
            if (pThumbnailListView->model()->rowCount() == paths.length() && QObject::tr("Select") == pChose->text())
            {
                pChose->setText(QObject::tr("Unselect"));
            }

            if (pThumbnailListView->model()->rowCount() != paths.length() && QObject::tr("Unselect") == pChose->text())
            {
                pChose->setText(QObject::tr("Select"));
            }
            updatePicNum();
        });

        connect(pThumbnailListView, &ThumbnailListView::sigMenuItemDeal, this, [ = ](QAction * action) {
            QStringList paths;
            paths.clear();
            for (int i = 0; i < m_allThumbnailListView.size(); i++) {
                paths << m_allThumbnailListView[i]->selectedPaths();
            }
            pThumbnailListView->menuItemDeal(paths, action);
        });
#endif
        connect(pThumbnailListView, &ThumbnailListView::sigBoxToChooseTimeLineAllPic, this, [ = ] {
            QStringList paths = pThumbnailListView->selectedPaths();
            if (pThumbnailListView->model()->rowCount() == paths.length() && QObject::tr("Select") == pChose->text())
            {
                pChose->setText(QObject::tr("Unselect"));
            }

            if (pThumbnailListView->model()->rowCount() != paths.length() && QObject::tr("Unselect") == pChose->text())
            {
                pChose->setText(QObject::tr("Select"));
            }
        });

        connect(pThumbnailListView, &ThumbnailListView::sigTimeLineItemBlankArea, this, [ = ] {
            QStringList paths = pThumbnailListView->selectedPaths();
            if (pThumbnailListView->model()->rowCount() == paths.length() && QObject::tr("Select") == pChose->text())
            {
                pChose->setText(QObject::tr("Unselect"));
            }

            if (pThumbnailListView->model()->rowCount() != paths.length() && QObject::tr("Unselect") == pChose->text())
            {
                pChose->setText(QObject::tr("Select"));
            }

            selpicQmap.insert(pThumbnailListView, paths);
            updatePicNum();

        });

    }

    if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
        // donothing
    } else {
        updateStackedWidget();
    }
}

void TimeLineView::on_AddLabel(QString date, QString num)
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

        m_dateItem->move(0, 0);
    }
}

void TimeLineView::on_DelLabel()
{
    if (nullptr != m_dateItem) {
        m_dateItem->setVisible(false);
    }
}

void TimeLineView::on_MoveLabel(int y)
{
    if ((nullptr != m_dateItem) && (nullptr != m_mainListWidget)) {
        m_dateItem->move(0, y + 1);
    }
}

void TimeLineView::resizeEvent(QResizeEvent *ev)
{
    m_dateItem->setFixedSize(width(), 87);
}

void TimeLineView::dragEnterEvent(QDragEnterEvent *e)
{
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void TimeLineView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }

    using namespace utils::image;
    QStringList paths;
    for (QUrl url : urls) {
        const QString path = url.toLocalFile();
        if (QFileInfo(path).isDir()) {
            auto finfos =  getImagesInfo(path, false);
            for (auto finfo : finfos) {
                if (imageSupportRead(finfo.absoluteFilePath())) {
                    paths << finfo.absoluteFilePath();
                }
            }
        } else if (imageSupportRead(path)) {
            paths << path;
        }
    }

    if (paths.isEmpty()) {
        return;
    }

    DBImgInfoList dbInfos;

    using namespace utils::image;

    for (auto path : paths) {
        if (! imageSupportRead(path)) {
            continue;
        }

//        // Generate thumbnail and storage into cache dir
//        if (! utils::image::thumbnailExist(path)) {
//            // Generate thumbnail failed, do not insert into DB
//            if (! utils::image::generateThumbnail(path)) {
//                continue;
//            }
//        }

        QFileInfo fi(path);
        DBImgInfo dbi;
        dbi.fileName = fi.fileName();
        dbi.filePath = path;
        dbi.dirHash = utils::base::hash(QString());
        dbi.time = fi.birthTime();

        dbInfos << dbi;
    }

    if (! dbInfos.isEmpty()) {
        QStringList paths;
        for (auto info : dbInfos) {
            paths << info.filePath;
        }

        dApp->m_imageloader->addImageLoader(paths);
        DBManager::instance()->insertImgInfos(dbInfos);
        emit dApp->signalM->updateStatusBarImportLabel(paths);
    } else {
        emit dApp->signalM->ImportFailed();
    }

    event->accept();
}

void TimeLineView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void TimeLineView::dragLeaveEvent(QDragLeaveEvent *e)
{

}

void TimeLineView::updatePicNum()
{
    QString str3 = QObject::tr("Selected %1 photos");

    if (2 == m_pStackedWidget->currentIndex()) {
        QStringList paths = pSearchView->m_pThumbnailListView->selectedPaths();
        m_selPicNum = paths.length();
        m_pStatusBar->m_pAllPicNumLabel->setText(str3.arg(m_selPicNum));
    } else {
        allnum = 0;

        for (int i = 0; i < m_allThumbnailListView.size(); i++) {
            allnum += m_allThumbnailListView[i]->selectedPaths().size();
        }

        if (0 == allnum) {
            m_pStatusBar->onUpdateAllpicsNumLabel();
        } else {
            QString str = QObject::tr("Selected %1 photos");
            m_pStatusBar->m_pAllPicNumLabel->setText(str.arg(allnum));
        }
    }
}

void TimeLineView::restorePicNum()
{
    m_pStatusBar->onUpdateAllpicsNumLabel();
}
