#include "searchview.h"
#include <DApplicationHelper>
#include "utils/snifferimageformat.h"
#include <QGraphicsDropShadowEffect>

namespace {
const int VIEW_MAINWINDOW_SEARCH = 3;
}  //namespace

SearchView::SearchView()
{
    m_searchPicNum = 0;
    m_keywords = "";
    initNoSearchResultView();
    initSearchResultView();
    initMainStackWidget();
    initConnections();
}

void SearchView::initConnections()
{
    connect(m_pSlideShowBtn, &DPushButton::clicked, this, [=] {
        auto imagelist = DBManager::instance()->getInfosForKeyword(m_keywords);
        QStringList paths;
        for(auto image : imagelist)
        {
            paths<<image.filePath;
        }

        const QString path = paths.first();

        emit m_pThumbnailListView->menuOpenImage(path, paths, true, true);
    });
    connect(dApp->signalM, &SignalManager::sigSendKeywordsIntoALLPic, this, &SearchView::improtSearchResultsIntoThumbnailView);
    connect(m_pThumbnailListView,&ThumbnailListView::openImage,this,[=](int index){
        SignalManager::ViewInfo info;
        info.album = "";
        info.lastPanel = nullptr;
        auto imagelist = DBManager::instance()->getInfosForKeyword(m_keywords);
        for(auto image : imagelist)
        {
            info.paths<<image.filePath;
        }
        info.path = info.paths[index];
        info.viewType = utils::common::VIEW_SEARCH_SRN;

        emit dApp->signalM->viewImage(info);
        emit dApp->signalM->showImageView(VIEW_MAINWINDOW_SEARCH);
    });
    connect(m_pThumbnailListView,&ThumbnailListView::menuOpenImage,this,[=](QString path,QStringList paths,bool isFullScreen, bool isSlideShow){
        SignalManager::ViewInfo info;
        info.album = "";
        info.lastPanel = nullptr;
        auto imagelist = DBManager::instance()->getInfosForKeyword(m_keywords);
        if(paths.size()>1){
            info.paths = paths;
        }else if(imagelist.size()>1){
            for(auto image : imagelist)
            {
                info.paths<<image.filePath;
            }
        }
        info.path = path;
        info.fullScreen = isFullScreen;
        info.slideShow = isSlideShow;
        info.viewType = utils::common::VIEW_SEARCH_SRN;

        if(info.slideShow)
        {
            if(imagelist.count() == 1)
            {
                info.paths = paths;
            }
            emit dApp->signalM->startSlideShow(info);
        }
        else {
            emit dApp->signalM->viewImage(info);
        }
        emit dApp->signalM->showImageView(VIEW_MAINWINDOW_SEARCH);

    });

    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &SearchView::updateSearchResultsIntoThumbnailView);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &SearchView::updateSearchResultsIntoThumbnailView);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &SearchView::updateSearchResultsIntoThumbnailView);
    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, this, &SearchView::changeTheme);
    connect(dApp, &Application::sigFinishLoad, this, [=] {
        m_pThumbnailListView->update();
    });
}

void SearchView::initNoSearchResultView()
{
    m_pNoSearchResultView = new DWidget();
    QVBoxLayout* pNoSearchResultLayout = new QVBoxLayout();
    pNoResult = new DLabel();

    pNoResult->setText(tr("No search results"));
    pNoResult->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T4));

//    DPalette pa = DApplicationHelper::instance()->palette(pNoResult);
//    pa.setBrush(DPalette::Text, pa.color(DPalette::PlaceholderText));
//    pNoResult->setPalette(pa);

    DPalette palette = DApplicationHelper::instance()->palette(pNoResult);
    QColor color_TTT = palette.color(DPalette::ToolTipText);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType)
    {
        color_TTT.setAlphaF(0.3);
        palette.setBrush(DPalette::Text, color_TTT);
        pNoResult->setForegroundRole(DPalette::Text);
        pNoResult->setPalette(palette);
    }
    else if (themeType == DGuiApplicationHelper::DarkType)
    {
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
    m_pSearchResultView = new DWidget();
    QVBoxLayout* pSearchResultLayout = new QVBoxLayout();
//    pSearchResultLayout->setSpacing(10);
    pLabel1 = new DLabel();

    pLabel1->setText(tr("Search results"));
    QFont font = DFontSizeManager::instance()->get(DFontSizeManager::T3);
    font.setWeight(QFont::DemiBold);
    pLabel1->setFont(font);
    DPalette pa = DApplicationHelper::instance()->palette(pLabel1);
    pa.setBrush(DPalette::Text, pa.color(DPalette::ToolTipText));
    pLabel1->setForegroundRole(DPalette::Text);
    pLabel1->setPalette(pa);
    pLabel1->setContentsMargins(13,0,0,0);

    QHBoxLayout* pHBoxLayout = new QHBoxLayout();
    pHBoxLayout->setSpacing(5);
    pHBoxLayout->setContentsMargins(8,0,0,0);

    m_pSlideShowBtn = new DPushButton();
    m_pSlideShowBtn ->setFocusPolicy(Qt::NoFocus);
//    m_pSlideShowBtn->setFixedSize(105, 31);

    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType)
    {
        QPalette pal = m_pSlideShowBtn->palette();
        pal.setColor(QPalette::Light,QColor(253,94,94));
        pal.setColor(QPalette::Dark,QColor(237,86,86));
        pal.setColor(QPalette::ButtonText,QColor(255,255,255));
        m_pSlideShowBtn->setPalette(pal);
//        QGraphicsDropShadowEffect *shadow_effect = new QGraphicsDropShadowEffect(m_pSlideShowBtn); //阴影效果
//        shadow_effect->setOffset(0,4);
//        shadow_effect->setColor(QColor(248,44,71,102));
//        shadow_effect->setBlurRadius(6);
//        m_pSlideShowBtn->setGraphicsEffect(shadow_effect);
    }
    if (themeType == DGuiApplicationHelper::DarkType)
    {
        QPalette pal = m_pSlideShowBtn->palette();
        pal.setColor(QPalette::Light,QColor(218,45,45));
        pal.setColor(QPalette::Dark,QColor(165,27,27));
        pal.setColor(QPalette::ButtonText,QColor(255,255,255));
        m_pSlideShowBtn->setPalette(pal);
//        QGraphicsDropShadowEffect *shadow_effect = new QGraphicsDropShadowEffect(m_pSlideShowBtn); //阴影效果
//        shadow_effect->setOffset(0,2);
//        shadow_effect->setColor(QColor(193,10,10,127));
//        shadow_effect->setBlurRadius(4);
//        m_pSlideShowBtn->setGraphicsEffect(shadow_effect);
    }

    QIcon icon;
    icon = utils::base::renderSVG(":/resources/images/other/play all_normal.svg", QSize(18, 18));
    m_pSlideShowBtn->setIcon(icon);
    m_pSlideShowBtn->setText(tr("Slide Show"));

//    DLabel* Label1 = new DLabel(m_pSlideShowBtn);
//    Label1->move(6,7);
//    DLabel* Label2 = new DLabel(m_pSlideShowBtn);
//    Label2->setFixedSize(70,18);
//    Label2->move(29,6);
//    Label1->setPixmap(pixmap);
//    Label1->setPalette(pal);
//    Label2->setText(tr("Slide Show"));

    QFont ft1 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft1.setFamily("SourceHanSansSC-Medium");
    ft1.setWeight(QFont::Medium);
    m_pSlideShowBtn->setFont(ft1);

    m_pSearchResultLabel = new DLabel();

    pHBoxLayout->addSpacing(5);
    pHBoxLayout->addWidget(m_pSlideShowBtn);
    pHBoxLayout->addSpacing(5);
    pHBoxLayout->addWidget(m_pSearchResultLabel);
    pHBoxLayout->addStretch(0);

    m_pThumbnailListView = new ThumbnailListView();

    m_pThumbnailListView->setFrameShape(QListView::NoFrame);

//    pSearchResultLayout->addSpacing(5);
    pSearchResultLayout->addWidget(pLabel1);
    pSearchResultLayout->addItem(pHBoxLayout);
    pSearchResultLayout->addWidget(m_pThumbnailListView);

    m_pSearchResultView->setLayout(pSearchResultLayout);
}

void SearchView::initMainStackWidget()
{
    m_stackWidget = new DStackedWidget();
    m_stackWidget->setContentsMargins(0, 0, 0, 0);
    m_stackWidget->addWidget(m_pNoSearchResultView);
    m_stackWidget->addWidget(m_pSearchResultView);

    QLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stackWidget);
}

void SearchView::improtSearchResultsIntoThumbnailView(QString s, QString album)
{
    using namespace utils::image;
    m_keywords = s;
    QList<ThumbnailListView::ItemInfo> thumbnaiItemList;
    DBImgInfoList infos;
    if(album.isEmpty())
    {
        infos = DBManager::instance()->getInfosForKeyword(s);
    }
    else if (COMMON_STR_TRASH == album) {
        infos = DBManager::instance()->getTrashInfosForKeyword(s);
    }
    else {
        infos = DBManager::instance()->getInfosForKeyword(album , s);
    }


    if (0 < infos.length())
    {
        for(auto info : infos)
        {
            ThumbnailListView::ItemInfo vi;
            vi.name = info.fileName;
            vi.path = info.filePath;
//            if (COMMON_STR_TRASH == album)
//            {
//                vi.image = dApp->m_imagetrashmap.value(info.filePath);//TODO_DS
//            }
//            else
//            {
//                vi.image = dApp->m_imagemap.value(info.filePath);//TODO_DS
//            }

            if (COMMON_STR_TRASH == album)
            {
                if (dApp->m_imagetrashmap.value(info.filePath).isNull())
                {
                    QSize imageSize = getImageQSize(vi.path);

                    vi.width = imageSize.width();
                    vi.height = imageSize.height();
                }
                else
                {
                    vi.width = dApp->m_imagetrashmap.value(info.filePath).width();
                    vi.height = dApp->m_imagetrashmap.value(info.filePath).height();
                }
            }
            else
            {
                if (dApp->m_imagemap.value(info.filePath).isNull())
                {
                    QSize imageSize = getImageQSize(vi.path);

                    vi.width = imageSize.width();
                    vi.height = imageSize.height();
                }
                else
                {
                    vi.width = dApp->m_imagemap.value(info.filePath).width();
                    vi.height = dApp->m_imagemap.value(info.filePath).height();
                }
            }

            thumbnaiItemList<<vi;
        }

        m_pThumbnailListView->insertThumbnails(thumbnaiItemList);


        QString searchStr = tr("%1 photo(s) found");
        QString str = QString::number(infos.length());
        m_searchPicNum = infos.length();
        m_pSearchResultLabel->setText(searchStr.arg(str));
        m_pSearchResultLabel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
        DPalette palette = DApplicationHelper::instance()->palette(m_pSearchResultLabel);
        QColor color_BT = palette.color(DPalette::BrightText);
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType)
        {
            color_BT.setAlphaF(0.5);
            palette.setBrush(DPalette::Text, color_BT);
            m_pSearchResultLabel->setForegroundRole(DPalette::Text);
            m_pSearchResultLabel->setPalette(palette);
        }
        else if (themeType == DGuiApplicationHelper::DarkType)
        {
            color_BT.setAlphaF(0.75);
            palette.setBrush(DPalette::Text, color_BT);
            m_pSearchResultLabel->setForegroundRole(DPalette::Text);
            m_pSearchResultLabel->setPalette(palette);
        }

        m_stackWidget->setCurrentIndex(1);
    }
    else
    {

        QString str = tr("No results for '%1', please try another word");
        m_pNoSearchResultLabel->setText(str.arg(s));
        m_pNoSearchResultLabel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T8));
        DPalette palette = DApplicationHelper::instance()->palette(m_pNoSearchResultLabel);
        QColor color_TTT = palette.color(DPalette::ToolTipText);
        DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
        if (themeType == DGuiApplicationHelper::LightType)
        {
            color_TTT.setAlphaF(0.3);
            palette.setBrush(DPalette::Text, color_TTT);
            m_pNoSearchResultLabel->setForegroundRole(DPalette::Text);
            m_pNoSearchResultLabel->setPalette(palette);
        }
        else if (themeType == DGuiApplicationHelper::DarkType)
        {
            color_TTT.setAlphaF(0.4);
            palette.setBrush(DPalette::Text, color_TTT);
            m_pNoSearchResultLabel->setForegroundRole(DPalette::Text);
            m_pNoSearchResultLabel->setPalette(palette);
        }

        m_searchPicNum = 0;
        m_stackWidget->setCurrentIndex(0);
    }
}

void SearchView::updateSearchResultsIntoThumbnailView()
{
    improtSearchResultsIntoThumbnailView(m_keywords, nullptr);
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
    if (themeType == DGuiApplicationHelper::LightType)
    {
        color_TTT.setAlphaF(0.3);
        pa.setBrush(DPalette::Text, color_TTT);
        pNoResult->setPalette(pa);
        m_pNoSearchResultLabel->setPalette(pa);

        color_BT.setAlphaF(0.5);
        pat.setBrush(DPalette::Text, color_BT);
        m_pSearchResultLabel->setPalette(pat);

        QPalette pal = m_pSlideShowBtn->palette();
        pal.setColor(QPalette::Light,QColor(253,94,94));
        pal.setColor(QPalette::Dark,QColor(237,86,86));
        pal.setColor(QPalette::ButtonText,QColor(255,255,255));
        m_pSlideShowBtn->setPalette(pal);
//        QGraphicsDropShadowEffect *shadow_effect = new QGraphicsDropShadowEffect(m_pSlideShowBtn);
//        shadow_effect->setOffset(0,4);
//        shadow_effect->setColor(QColor(248,44,71,102));
//        shadow_effect->setBlurRadius(6);
//        m_pSlideShowBtn->setGraphicsEffect(shadow_effect);
    }
    else if (themeType == DGuiApplicationHelper::DarkType)
    {
        color_TTT.setAlphaF(0.4);
        pa.setBrush(DPalette::Text, color_TTT);
        pNoResult->setPalette(pa);
        m_pNoSearchResultLabel->setPalette(pa);

        color_BT.setAlphaF(0.75);
        pat.setBrush(DPalette::Text, color_BT);
        m_pSearchResultLabel->setPalette(pat);

        QPalette pal = m_pSlideShowBtn->palette();
        pal.setColor(QPalette::Light,QColor(218,45,45));
        pal.setColor(QPalette::Dark,QColor(165,27,27));
        pal.setColor(QPalette::ButtonText,QColor(255,255,255));
        m_pSlideShowBtn->setPalette(pal);
//        QGraphicsDropShadowEffect *shadow_effect = new QGraphicsDropShadowEffect(m_pSlideShowBtn);
//        shadow_effect->setOffset(0,2);
//        shadow_effect->setColor(QColor(193,10,10,127));
//        shadow_effect->setBlurRadius(4);
//        m_pSlideShowBtn->setGraphicsEffect(shadow_effect);
    }

}

