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
} //namespace

TimeLineView::TimeLineView()
{
    setAcceptDrops(true);
    fatherwidget = new DWidget(this);
    fatherwidget->setFixedSize(this->size());
    m_index = 0;

    m_oe = new QGraphicsOpacityEffect(this);
    m_oet = new QGraphicsOpacityEffect(this);
    m_oe->setOpacity(0.5);
    m_oet->setOpacity(0.75);

    m_pStackedWidget = new DStackedWidget();

    pTimeLineViewWidget = new DWidget();
    pImportView = new ImportView();
    pSearchView = new SearchView();

    m_pStackedWidget->addWidget(pImportView);
    m_pStackedWidget->addWidget(pTimeLineViewWidget);
    m_pStackedWidget->addWidget(pSearchView);

    m_pStatusBar = new StatusBar(this);
    m_pStatusBar->raise();
    m_pStatusBar->setFixedWidth(this->width());
    m_pStatusBar->move(0, this->height() - m_pStatusBar->height());
//    m_pStatusBar->setParent(this);

    QVBoxLayout *pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins(2, 0, 0, 0);
    pVBoxLayout->addWidget(m_pStackedWidget);
//    pVBoxLayout->addWidget(m_pStatusBar);
    fatherwidget->setLayout(pVBoxLayout);

    initTimeLineViewWidget();

//    updataLayout();

    initConnections();

//    m_pwidget = new QWidget(this);

    m_spinner = new DSpinner(this);
    m_spinner->setFixedSize(40, 40);
    m_spinner->hide();
}

void TimeLineView::initConnections()
{
    connect(dApp->signalM, &SignalManager::sigLoadOnePhoto, this, &TimeLineView::updataLayout);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &TimeLineView::updataLayout);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &TimeLineView::updataLayout);
    connect(dApp, &Application::sigFinishLoad, this, [ = ] {
        m_mainListWidget->update();
    });
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

    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &TimeLineView::updataLayout);
    connect(m_pStatusBar->m_pSlider, &DSlider::valueChanged, dApp->signalM, &SignalManager::sigMainwindowSliderValueChg);

    connect(pSearchView->m_pThumbnailListView, &ThumbnailListView::clicked, this, &TimeLineView::updatePicNum);

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &TimeLineView::themeChangeSlot);
    connect(pImportView->m_pImportBtn, &DPushButton::clicked, this, [ = ] {
        m_spinner->show();
        m_spinner->start();
        m_pStackedWidget->setCurrentIndex(VIEW_TIMELINE);
        pImportView->onImprotBtnClicked();
    });
    connect(dApp->signalM, &SignalManager::sigImportFailedToView, this, [ = ] {
        if (isVisible())
        {
            m_spinner->hide();
            m_spinner->stop();
            updateStackedWidget();
        }
    });
    connect(dApp->signalM, &SignalManager::sigShortcutKeyDelete, this, &TimeLineView::onKeyDelete);
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

//    m_mainListWidget->setStyleSheet("Background:blue");

    m_mainListWidget->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    m_mainListWidget->verticalScrollBar()->setSingleStep(5);
    m_mainLayout->addWidget(m_mainListWidget);
    m_mainListWidget->setFrameShape(DTableView::NoFrame);

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
    pSuspensionChose = new DCommandLinkButton(QObject::tr("Select"));
    pSuspensionChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    pSuspensionChose->setFixedHeight(24);
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
            updatePicNum();
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
            updatePicNum();
        }
#if 1
        QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
        b[0]->setText(pSuspensionChose->text());
#endif
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

void TimeLineView::updataLayout()
{
    m_spinner->hide();
    m_spinner->stop();
    //获取所有时间线
    m_mainListWidget->clear();
    m_timelines = DBManager::instance()->getAllTimelines();
    qDebug() << m_timelines.size();

#if 1
    m_allThumbnailListView.clear();
    m_allChoseButton.clear();
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
        DCommandLinkButton *pChose = new DCommandLinkButton(QObject::tr("Select"));
        m_allChoseButton << pChose;
        pChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
        pChose->setFixedHeight(24);
        pChose->resize(36, 27);

        pNum_dn->setLayout(Layout);
        Layout->addStretch(1);
        Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
//        Layout->setContentsMargins(0, 0, 0, 0);
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
        pThumbnailListView->setFixedWidth(width() + 2);
        connect(pThumbnailListView, &ThumbnailListView::loadend, this, [ = ](int h) {
            if (isVisible()) {
                int mh = h;
                if (0 == i) {
                    mh += 50;
                }
                if (i == m_timelines.size() - 1) {
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

        using namespace utils::image;
        QList<ThumbnailListView::ItemInfo> thumbnaiItemList;
        for (int j = 0; j < ImgInfoList.size(); j++) {
            ThumbnailListView::ItemInfo vi;
            vi.name = ImgInfoList.at(j).fileName;
            vi.path = ImgInfoList.at(j).filePath;
//            vi.image = dApp->m_imagemap.value(ImgInfoList.at(j).filePath);
            if (dApp->m_imagemap.value(ImgInfoList.at(j).filePath).isNull()) {
                QSize imageSize = getImageQSize(vi.path);

                vi.width = imageSize.width();
                vi.height = imageSize.height();
            } else {
                vi.width = dApp->m_imagemap.value(ImgInfoList.at(j).filePath).width();
                vi.height = dApp->m_imagemap.value(ImgInfoList.at(j).filePath).height();
            }

            thumbnaiItemList.append(vi);
        }
        //保存当前时间照片

        pThumbnailListView->insertThumbnails(thumbnaiItemList);

        if (0 == i) {
            DWidget *topwidget = new DWidget;
            topwidget->setFixedHeight(50);
            listItemlayout->addWidget(topwidget);
        }
        listItemlayout->addWidget(TitleView);
        listItemlayout->addWidget(pThumbnailListView);
        if (i == m_timelines.size() - 1) {
            DWidget *bottomwidget = new DWidget;
            bottomwidget->setFixedHeight(27);
            listItemlayout->addWidget(bottomwidget);
        }
        item->setFlags(Qt::NoItemFlags);
        m_mainListWidget->addItemForWidget(item);
        m_mainListWidget->setItemWidget(item, listItem);
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
            info.viewMainWindowID = VIEW_MAINWINDOW_TIMELINE;
            emit dApp->signalM->viewImage(info);
            emit dApp->signalM->showImageView(VIEW_MAINWINDOW_TIMELINE);


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
        connect(pChose, &DCommandLinkButton::clicked, this, [ = ] {
            if (QObject::tr("Select") == pChose->text())
            {
                pChose->setText(QObject::tr("Unselect"));
                for (int i = 0; i < m_allChoseButton.length(); i++) {
                    if (pChose == m_allChoseButton[i])
                        lastClickedIndex = i;
                }
                lastRow = 0;
                lastChanged = true;
                pThumbnailListView->selectAll();
            } else
            {
                pChose->setText(QObject::tr("Select"));
                pThumbnailListView->clearSelection();
            }
            updatePicNum();
        });
#if 1
        connect(pThumbnailListView, &ThumbnailListView::sigGetSelectedPaths, this, [ = ](QStringList * pPaths) {
            pPaths->clear();
            for (int i = 0; i < m_allThumbnailListView.size(); i++) {
                pPaths->append(m_allThumbnailListView[i]->selectedPaths());
            }
        });

        connect(pThumbnailListView, &ThumbnailListView::sigMousePress, this, [ = ] (QMouseEvent * event) {
            lastRow = -1;
            for (int i = 0; i < m_allThumbnailListView.length(); i++) {
                if (pThumbnailListView != m_allThumbnailListView[i]) {
                    m_allThumbnailListView[i]->clearSelection();
                }
            }
            for (int i = 0; i < m_allThumbnailListView.length(); i++) {
                if (pThumbnailListView == m_allThumbnailListView[i]) {
                    lastClickedIndex = i;
                    lastRow = pThumbnailListView->getRow(QPoint(event->x(), event->y()));
                    if (-1 != lastRow)
                        lastChanged = true;
                }
            }
        });

        connect(pThumbnailListView, &ThumbnailListView::sigCtrlMousePress, this, [ = ] (QMouseEvent * event) {
            for (int i = 0; i < m_allThumbnailListView.length(); i++) {
                if (pThumbnailListView == m_allThumbnailListView[i]) {
                    lastClickedIndex = i;
                    lastRow = pThumbnailListView->getRow(QPoint(event->x(), event->y()));
                    if (-1 != lastRow)
                        lastChanged = true;
                }
            }
        });

        connect(pThumbnailListView, &ThumbnailListView::sigShiftMousePress, this, [ = ] (QMouseEvent * event) {
            int curClickedIndex = -1;
            int curRow = -1;
            for (int i = 0; i < m_allThumbnailListView.length(); i++) {
                if (pThumbnailListView == m_allThumbnailListView[i]) {
                    curClickedIndex = i;
                    curRow = pThumbnailListView->getRow(QPoint(event->x(), event->y()));
                }
            }
            if (!lastChanged && -1 != curRow) {
                for (int i = 0; i < m_allThumbnailListView.length(); i++) {
                    m_allThumbnailListView[i]->clearSelection();
                }
            }
            if (curRow == -1 || lastRow == -1)
                for (int i = 0; i < m_allThumbnailListView.length(); i++) {
                    m_allThumbnailListView[i]->clearSelection();
                } else {
                if (lastClickedIndex < curClickedIndex) {
                    m_allThumbnailListView[lastClickedIndex]->selectRear(lastRow);
                    m_allThumbnailListView[curClickedIndex]->selectFront(curRow);
                    for (int i = lastClickedIndex + 1; i < curClickedIndex; i++) {
                        m_allThumbnailListView[i]->selectAll();
                    }
                } else if (lastClickedIndex > curClickedIndex) {
                    m_allThumbnailListView[lastClickedIndex]->selectFront(lastRow);
                    m_allThumbnailListView[curClickedIndex]->selectRear(curRow);
                    for (int i = curClickedIndex + 1; i < lastClickedIndex; i++) {
                        m_allThumbnailListView[i]->selectAll();
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

        connect(pThumbnailListView, &ThumbnailListView::sigSelectAll, this, [ = ] {
            for (int i = 0; i < m_allThumbnailListView.length(); i++)
            {
                m_allThumbnailListView[i]->selectAll();
            }
            updatePicNum();
            updateChoseText();
            QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
            pSuspensionChose->setText(b[0]->text());
        });

        connect(pThumbnailListView, &ThumbnailListView::sigMouseRelease, this, [ = ] {
            updatePicNum();
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

        connect(listItem, &TimelineItem::sigMousePress, this, [ = ] {
            for (int i = 0; i < m_allThumbnailListView.length(); i++)
            {
                m_allThumbnailListView[i]->clearSelection();
            }
            lastRow = -1;
            updatePicNum();
            updateChoseText();
        });
#endif
    }

    for (int i = 0; i < m_allThumbnailListView.size(); i++) {
        connect(m_allThumbnailListView[i], &ThumbnailListView::sigKeyEvent, this, &TimeLineView::on_KeyEvent);
    }

    if (VIEW_SEARCH == m_pStackedWidget->currentIndex()) {
        // donothing
    } else {
        updateStackedWidget();
    }

    updatePicNum();
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

        m_dateItem->move(0, TITLEHEIGHT);
    }
#if 1
    QList<DCommandLinkButton *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<DCommandLinkButton *>();
    pSuspensionChose->setText(b[0]->text());
#endif
}

void TimeLineView::on_DelLabel()
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
void TimeLineView::on_MoveLabel(int y, QString date, QString num, QString choseText)
#endif
{
    if ((nullptr != m_dateItem) && (nullptr != m_mainListWidget)) {
        QList<QLabel *> labelList = m_dateItem->findChildren<QLabel *>();
        labelList[0]->setText(date);
        labelList[1]->setText(num);
        pSuspensionChose->setText(choseText);
        m_dateItem->setVisible(true);
        m_dateItem->move(0, TITLEHEIGHT + y + 1);
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

void TimeLineView::resizeEvent(QResizeEvent *ev)
{
    m_spinner->move(width() / 2 - 20, (height() - 50) / 2 - 20);
    m_dateItem->setFixedSize(width() - 15, 87);
    for (int i = 0; i < m_allThumbnailListView.length(); i++) {
//        m_allThumbnailListView[i]->setStyleSheet("Background:yellow");
        m_allThumbnailListView[i]->setFixedWidth(width() + 2);
        QList<DLabel *> b = m_mainListWidget->itemWidget(m_mainListWidget->item(i))->findChildren<DLabel *>();
        b[1]->setFixedWidth(width() - 14);
    }
//    m_pwidget->setFixedWidth(this->width() / 2);
//    m_pwidget->setFixedHeight(54);
//    m_pwidget->move(this->width() / 4, this->height() - 81);
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

    // 判断当前导入路径是否为外接设备
    int isMountFlag = 0;
    DGioVolumeManager *pvfsManager = new DGioVolumeManager;
    QList<QExplicitlySharedDataPointer<DGioMount>> mounts = pvfsManager->getMounts();
    for (auto mount : mounts) {
        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
        QString strPath = LocationFile->path();
        if (0 == paths.first().compare(strPath)) {
            isMountFlag = 1;
            break;
        }
    }

    // 当前导入路径
    if (isMountFlag) {
        QString strHomePath = QDir::homePath();
        //获取系统现在的时间
        QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
        QDir dir;
        if (!dir.exists(basePath)) {
            dir.mkpath(basePath);
        }

        QStringList newImagePaths;
        foreach (QString strPath, paths) {
            //取出文件名称
            QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
            QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
            QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(), QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

            newImagePaths << strNewPath;
            //判断新路径下是否存在目标文件，若存在，下一次张
            if (dir.exists(strNewPath)) {
                continue;
            }

            // 外接设备图片拷贝到系统
            if (QFile::copy(strPath, strNewPath)) {

            }
        }

        paths.clear();
        paths = newImagePaths;
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
        if (fi.birthTime().isValid()) {
            dbi.time = fi.birthTime();
        } else if (fi.metadataChangeTime().isValid()) {
            dbi.time = fi.metadataChangeTime();
        } else {
            dbi.time = QDateTime::currentDateTime();
        }
        dbi.changeTime = QDateTime::currentDateTime();

        dbInfos << dbi;
    }

    if (! dbInfos.isEmpty()) {
        dApp->m_imageloader->ImportImageLoader(dbInfos);

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

void TimeLineView::keyPressEvent(QKeyEvent *e)
{
    qDebug() << "TimeLineView::keyPressEvent()";
    if (e->key() == Qt::Key_Control) {
        m_ctrlPress = true;
    }
}

void TimeLineView::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Control) {
        m_ctrlPress = false;
    }
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

    for (int i = 0; i < m_allThumbnailListView.size(); i++) {
        paths << m_allThumbnailListView[i]->selectedPaths();
    }

    if (0 >= paths.length()) {
        return;
    }

    DBImgInfoList infos;
    for (auto path : paths) {
        DBImgInfo info;
        info = DBManager::instance()->getInfoByPath(path);
        info.changeTime = QDateTime::currentDateTime();

        QStringList allalbumnames = DBManager::instance()->getAllAlbumNames();
        for (auto eachname : allalbumnames) {
            if (DBManager::instance()->isImgExistInAlbum(eachname, path)) {
                info.albumname += (eachname + ",");
            }
        }
        infos << info;
    }

    dApp->m_imageloader->addTrashImageLoader(paths);
    DBManager::instance()->insertTrashImgInfos(infos);
    DBManager::instance()->removeImgInfos(paths);
}
