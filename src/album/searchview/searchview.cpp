// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "searchview.h"
#include <DApplicationHelper>
#include "imageengine/imageengineapi.h"
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QDebug>

SearchView::SearchView()
    : m_stackWidget(nullptr), m_pNoSearchResultView(nullptr), m_pNoSearchResultLabel(nullptr)
    , m_searchResultViewbody(nullptr), m_searchResultViewTop(nullptr)
    , m_pSlideShowBtn(nullptr), m_pSearchResultLabel(nullptr), pNoResult(nullptr)
    , pLabel1(nullptr), m_searchPicNum(0), m_pThumbnailListView(nullptr)
{
    initNoSearchResultView();
    initSearchResultView();
    initMainStackWidget();
    initConnections();
}

void SearchView::initConnections()
{
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList &");
    connect(m_pSlideShowBtn, &DPushButton::clicked, this, &SearchView::onSlideShowBtnClicked);
    connect(dApp->signalM, &SignalManager::sigSendKeywordsIntoALLPic, this, &SearchView::improtSearchResultsIntoThumbnailView);
    connect(m_pThumbnailListView, &ThumbnailListView::openImage, this, &SearchView::onOpenImage);
    connect(m_pThumbnailListView, &ThumbnailListView::sigSlideShow, this, &SearchView::onSlideShow);
    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &SearchView::updateSearchResultsIntoThumbnailView);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &SearchView::updateSearchResultsIntoThumbnailView);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &SearchView::updateSearchResultsIntoThumbnailView);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &SearchView::changeTheme);
    connect(dApp->signalM, &SignalManager::sigShortcutKeyDelete, this, &SearchView::onKeyDelete);
}

void SearchView::initNoSearchResultView()
{
    m_pNoSearchResultView = new DWidget();
    QVBoxLayout *pNoSearchResultLayout = new QVBoxLayout();
    pNoResult = new DLabel();
    pNoResult->setText(tr("No search results"));
    pNoResult->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T4));
    DPalette palette = DApplicationHelper::instance()->palette(pNoResult);
    QColor color_TTT = palette.color(DPalette::ToolTipText);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        color_TTT.setAlphaF(0.3);
        palette.setBrush(DPalette::Text, color_TTT);
        pNoResult->setForegroundRole(DPalette::Text);
        pNoResult->setPalette(palette);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_TTT.setAlphaF(0.4);
        palette.setBrush(DPalette::Text, color_TTT);
        pNoResult->setForegroundRole(DPalette::Text);
        pNoResult->setPalette(palette);
    }

    m_pNoSearchResultLabel = new DLabel();
    pNoSearchResultLayout->addStretch();
    pNoSearchResultLayout->addWidget(pNoResult, 0, Qt::AlignCenter);
    pNoSearchResultLayout->addSpacing(10);
    pNoSearchResultLayout->addWidget(m_pNoSearchResultLabel, 0, Qt::AlignCenter);
    pNoSearchResultLayout->addStretch();
    m_pNoSearchResultView->setLayout(pNoSearchResultLayout);
}

void SearchView::initSearchResultView()
{
    m_pSearchResultWidget = new DWidget();
    pLabel1 = new DLabel();
    pLabel1->setText(tr("Search results"));
    QFont font = DFontSizeManager::instance()->get(DFontSizeManager::T3);
    font.setWeight(QFont::DemiBold);
    pLabel1->setFont(font);
    DPalette pa = DApplicationHelper::instance()->palette(pLabel1);
    pa.setBrush(DPalette::Text, pa.color(DPalette::ToolTipText));
    pLabel1->setForegroundRole(DPalette::Text);
    pLabel1->setPalette(pa);
    pLabel1->setContentsMargins(0, 0, 0, 0);

    QHBoxLayout *pHBoxLayout = new QHBoxLayout();
    pHBoxLayout->setSpacing(5);
    //LMH0417 bug号20706
    pHBoxLayout->setContentsMargins(0, 0, 0, 15);

    m_pSlideShowBtn = new DPushButton(this);
    m_pSlideShowBtn ->setFocusPolicy(Qt::NoFocus);
    auto playAllPalette = m_pSlideShowBtn->palette();
    playAllPalette.setColor(DPalette::ButtonText, Qt::white);
    playAllPalette.setColor(DPalette::Dark, QColor("#FD5E5E"));
    playAllPalette.setColor(DPalette::Light, QColor("#ED5656"));
    m_pSlideShowBtn->setPalette(playAllPalette);
    m_pSlideShowBtn->setIcon(QIcon::fromTheme("album_slider_show"));
//    m_pSlideShowBtn->setObjectName(Search_Slider_Show_Button);
    m_pSlideShowBtn->setText(tr("Slide Show"));
    m_pSlideShowBtn->setFixedHeight(30);
    m_pSlideShowBtn->setIconSize(QSize(18, 18));
    DFontSizeManager::instance()->bind(m_pSlideShowBtn, DFontSizeManager::T6, QFont::Medium);


    m_pSearchResultLabel = new DLabel();
    m_pSearchResultLabel->setContentsMargins(0, 0, 0, 0);
    pHBoxLayout->addWidget(m_pSlideShowBtn);
    pHBoxLayout->addSpacing(5);
    pHBoxLayout->addWidget(m_pSearchResultLabel);
    pHBoxLayout->addStretch(0);


    m_searchResultViewTop = new DWidget(m_pSearchResultWidget);

    QGraphicsOpacityEffect *opacityEffect_light = new QGraphicsOpacityEffect;
    opacityEffect_light->setOpacity(0.95);
    m_searchResultViewTop->setGraphicsEffect(opacityEffect_light);
    m_searchResultViewTop->setAutoFillBackground(true);
    m_searchResultViewTop->setBackgroundRole(DPalette::Window);

    m_searchResultViewbody = new DWidget(m_pSearchResultWidget);

    QVBoxLayout *pSearchResultbodyLayout = new QVBoxLayout();
    pSearchResultbodyLayout->setContentsMargins(8, 0, 0, 0);//搜索界面各边距
    //LMH0417 bug号20706
    m_pThumbnailListView = new ThumbnailListView(ThumbnailDelegate::SearchViewType, -1, COMMON_STR_SEARCH);

    m_pThumbnailListView->setFrameShape(QListView::NoFrame);

    pSearchResultbodyLayout->addWidget(m_pThumbnailListView);
    // zy 给搜索标题一个布局，将label固定在左侧，避免维语显示在右侧，bug69661
    QVBoxLayout *pSearchResultLayout = new QVBoxLayout();
    QHBoxLayout *searchlabellayout = new QHBoxLayout();
    searchlabellayout->setContentsMargins(0, 0, 0, 0);
    searchlabellayout->addWidget(pLabel1);
    searchlabellayout->addStretch();
    QWidget *wi = new QWidget();
    wi->setContentsMargins(0, 0, 0, 0);
    wi->setLayout(searchlabellayout);

    pSearchResultLayout->setContentsMargins(13, 0, 0, 0);
    pSearchResultLayout->setSpacing(0);
    pSearchResultLayout->addSpacing(5);
    pSearchResultLayout->addWidget(wi);
    pSearchResultLayout->addSpacing(5);
    pSearchResultLayout->addItem(pHBoxLayout);

    m_searchResultViewTop->setFixedHeight(90);
    m_searchResultViewTop->move(0, 0);
    m_searchResultViewbody->setLayout(pSearchResultbodyLayout);
    m_searchResultViewTop->setLayout(pSearchResultLayout);
    m_searchResultViewTop->raise();
}

void SearchView::initMainStackWidget()
{
    m_stackWidget = new DStackedWidget();
    m_stackWidget->setContentsMargins(0, 0, 0, 0);
    m_stackWidget->addWidget(m_pNoSearchResultView);
    m_stackWidget->addWidget(m_pSearchResultWidget);

    QLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stackWidget);
}

void SearchView::improtSearchResultsIntoThumbnailView(QString s, const QString &album, int UID)
{
    m_albumName = album;
    m_UID = UID;
    using namespace utils::image;
    m_keywords = s;
    QList<DBImgInfo> thumbnaiItemList;
    DBImgInfoList infos;
    if (COMMON_STR_ALLPHOTOS == m_albumName
            || COMMON_STR_TIMELINE == m_albumName
            || COMMON_STR_RECENT_IMPORTED == m_albumName) {
        infos = DBManager::instance()->getInfosForKeyword(s);
    } else if (COMMON_STR_TRASH == m_albumName) {
        infos = DBManager::instance()->getTrashInfosForKeyword(s);
    } else {
        infos = DBManager::instance()->getInfosForKeyword(m_UID, s);
    }

    if (0 < infos.length()) {
        m_pThumbnailListView->clearAll();
        //插入空白项
        m_pThumbnailListView->insertBlankOrTitleItem(ItemTypeBlank, "", "", 90);
        //插入信息
        m_pThumbnailListView->insertThumbnailByImgInfos(infos);
        int photoCount = m_pThumbnailListView->getAppointTypeItemCount(ItemTypePic);
        int videoCount = m_pThumbnailListView->getAppointTypeItemCount(ItemTypeVideo);
        QString searchStr;
        if (photoCount > 0 && videoCount == 0) {
            if (photoCount == 1) {
                searchStr = tr("1 photo found");
            } else {
                searchStr = tr("%n photos found", "", photoCount);
            }
            setSliderShowBtnEnable(true);
        } else if (photoCount == 0 && videoCount > 0) {
            if (videoCount == 1) {
                searchStr = tr("1 video found");
            } else {
                searchStr = tr("%n videos found", "", videoCount);
            }
            setSliderShowBtnEnable(false);
        } else if (photoCount > 0 && videoCount > 0) {
            searchStr = tr("%n items found", "", (photoCount + videoCount));
            setSliderShowBtnEnable(true);
        }
        QString str = QString::number(infos.length());
        m_searchPicNum = photoCount + videoCount;
        m_pSearchResultLabel->setText(searchStr);
        m_pSearchResultLabel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
        DPalette palette = DApplicationHelper::instance()->palette(m_pSearchResultLabel);
        QColor color_BT = palette.color(DPalette::BrightText);
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType) {
            color_BT.setAlphaF(0.5);
            palette.setBrush(DPalette::Text, color_BT);
            m_pSearchResultLabel->setForegroundRole(DPalette::Text);
            m_pSearchResultLabel->setPalette(palette);
        } else if (themeType == DGuiApplicationHelper::DarkType) {
            color_BT.setAlphaF(0.75);
            palette.setBrush(DPalette::Text, color_BT);
            m_pSearchResultLabel->setForegroundRole(DPalette::Text);
            m_pSearchResultLabel->setPalette(palette);
        }

        m_stackWidget->setCurrentIndex(1);
    } else {
        m_searchPicNum = 0;
        m_stackWidget->setCurrentIndex(0);
    }
}

void SearchView::onSlideShowBtnClicked()
{
    DBImgInfoList imagelist;
    if (COMMON_STR_ALLPHOTOS == m_albumName
            || COMMON_STR_TIMELINE == m_albumName
            || COMMON_STR_RECENT_IMPORTED == m_albumName) {
        imagelist = DBManager::instance()->getInfosForKeyword(m_keywords);
    } else if (COMMON_STR_TRASH == m_albumName) {
        imagelist = DBManager::instance()->getTrashInfosForKeyword(m_keywords);
    } else {
        imagelist = DBManager::instance()->getInfosForKeyword(m_UID, m_keywords);
    }

    QStringList paths;
    for (auto image : imagelist) {
        paths << image.filePath;
    }

    QString path = "";
    if (paths.size() > 0) {
        path = paths.first();
        onSlideShow(path);
    }
}

void SearchView::onOpenImage(int row, const QString &path, bool bFullScreen)
{
    SignalManager::ViewInfo info;
    info.album = "";
//    info.lastPanel = nullptr;  //todo imageviewer
    info.fullScreen = bFullScreen;
    auto imagelist = m_pThumbnailListView->getFileList(row, ItemType::ItemTypePic);
    if (imagelist.size() > 0) {
        info.paths << imagelist;
        info.path = path;
    } else {
        info.paths.clear();
    }
    info.dBImgInfos = m_pThumbnailListView->getAllFileInfo(row);
    info.viewType = utils::common::VIEW_SEARCH_SRN;

    if (COMMON_STR_ALLPHOTOS == m_albumName) {
        info.viewMainWindowID = 0;
    } else if (COMMON_STR_TIMELINE == m_albumName) {
        info.viewMainWindowID = 1;
    } else {
        info.viewMainWindowID = 2;
    }

    if (bFullScreen) {
        emit dApp->signalM->sigViewImage(info, Operation_FullScreen);
    } else {
        emit dApp->signalM->sigViewImage(info, Operation_NoOperation);
    }
}

void SearchView::onSlideShow(const QString &path)
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
    info.viewType = utils::common::VIEW_SEARCH_SRN;
    emit dApp->signalM->startSlideShow(info);
}

void SearchView::updateSearchResultsIntoThumbnailView()
{
    improtSearchResultsIntoThumbnailView(m_keywords, m_albumName, m_UID);
}

void SearchView::changeTheme()
{

    DPalette pale = DApplicationHelper::instance()->palette(pLabel1);
    pale.setBrush(DPalette::Text, pale.color(DPalette::ToolTipText));
    pLabel1->setPalette(pale);

    DPalette pa = DApplicationHelper::instance()->palette(pNoResult);
    QColor color_TTT = pa.color(DPalette::ToolTipText);
    DPalette pat = DApplicationHelper::instance()->palette(m_pSearchResultLabel);
    QColor color_BT = pat.color(DPalette::BrightText);

    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        color_TTT.setAlphaF(0.3);
        pa.setBrush(DPalette::Text, color_TTT);
        pNoResult->setPalette(pa);
        m_pNoSearchResultLabel->setPalette(pa);

        color_BT.setAlphaF(0.5);
        pat.setBrush(DPalette::Text, color_BT);
        m_pSearchResultLabel->setPalette(pat);

        auto playAllPalette = m_pSlideShowBtn->palette();
        playAllPalette.setColor(DPalette::ButtonText, Qt::white);
        playAllPalette.setColor(DPalette::Light, QColor("#FD5E5E"));
        playAllPalette.setColor(DPalette::Dark, QColor("#ED5656"));
        m_pSlideShowBtn->setPalette(playAllPalette);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_TTT.setAlphaF(0.4);
        pa.setBrush(DPalette::Text, color_TTT);
        pNoResult->setPalette(pa);
        m_pNoSearchResultLabel->setPalette(pa);

        color_BT.setAlphaF(0.75);
        pat.setBrush(DPalette::Text, color_BT);
        m_pSearchResultLabel->setPalette(pat);

        auto playAllPalette = m_pSlideShowBtn->palette();
        playAllPalette.setColor(DPalette::ButtonText, "#FFFFFF");
        playAllPalette.setColor(DPalette::Light, QColor("#DA2D2D"));
        playAllPalette.setColor(DPalette::Dark, QColor("#A51B1B"));
        m_pSlideShowBtn->setPalette(playAllPalette);
    }

}

void SearchView::setSliderShowBtnEnable(bool enabled)
{
    if (enabled) {
        changeTheme();
    } else {
        //结果中含视频则置灰
        auto playAllPalette = m_pSlideShowBtn->palette();
        playAllPalette.setColor(DPalette::ButtonText, QColor("white"));
        QColor color = QColor("#F82C47");
        color.setAlphaF(0.4);
        playAllPalette.setColor(DPalette::Light, color);
        playAllPalette.setColor(DPalette::Dark, color);
        m_pSlideShowBtn->setPalette(playAllPalette);
    }
    m_pSlideShowBtn->setEnabled(enabled);
}

void SearchView::paintEvent(QPaintEvent *event)
{
    QFont font;
    int currentSize = DFontSizeManager::instance()->fontPixelSize(font);
    if (currentSize != m_currentFontSize) {
        m_currentFontSize = currentSize;
    }
    QWidget::paintEvent(event);
    m_searchResultViewTop->setFixedWidth(m_pSearchResultWidget->width() - 10);
    m_searchResultViewbody->setFixedSize(m_pSearchResultWidget->size());
}

void SearchView::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    m_searchResultViewTop->setFixedWidth(m_pSearchResultWidget->width() - 10);
    m_searchResultViewbody->setFixedSize(m_pSearchResultWidget->size());
}

void SearchView::onKeyDelete()
{
    if (!isVisible()) return;

    QStringList paths;
    paths.clear();

    paths = m_pThumbnailListView->selectedPaths();
    if (0 >= paths.length()) {
        return;
    }
    m_pThumbnailListView->clearSelection();
    ImageEngineApi::instance()->moveImagesToTrash(paths);
}

