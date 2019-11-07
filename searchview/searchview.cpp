#include "searchview.h"
#include <DApplicationHelper>
#include "utils/snifferimageformat.h"

namespace {
const int VIEW_MAINWINDOW_SEARCH = 3;
}  //namespace

SearchView::SearchView()
{
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
}

void SearchView::initNoSearchResultView()
{
    m_pNoSearchResultView = new DWidget();
    QVBoxLayout* pNoSearchResultLayout = new QVBoxLayout();
    pNoResult = new DLabel();
    pNoResult->setText("无结果");
    pNoResult->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T4));

    DPalette pa = DApplicationHelper::instance()->palette(pNoResult);
    pa.setBrush(DPalette::WindowText, pa.color(DPalette::Text));
    pNoResult->setPalette(pa);

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
    pLabel1->setText("搜索结果");
    pLabel1->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T3));
    DPalette pa = DApplicationHelper::instance()->palette(pLabel1);
    pa.setBrush(DPalette::WindowText, pa.color(DPalette::ToolTipText));
    pLabel1->setPalette(pa);
    pLabel1->setContentsMargins(13,0,0,0);

    QHBoxLayout* pHBoxLayout = new QHBoxLayout();
    pHBoxLayout->setSpacing(5);
    pHBoxLayout->setContentsMargins(8,0,0,0);

    m_pSlideShowBtn = new DPushButton();
    m_pSlideShowBtn ->setFocusPolicy(Qt::NoFocus);
    m_pSlideShowBtn->setFixedSize(105, 31);

    QPalette pal = m_pSlideShowBtn->palette();
    pal.setColor(QPalette::Light,QColor(253,94,94));
    pal.setColor(QPalette::Dark,QColor(237,86,86));
    pal.setColor(QPalette::ButtonText,QColor(255,255,255));
    m_pSlideShowBtn->setPalette(pal);

    QPixmap pixmap;
    pixmap = utils::base::renderSVG(":/resources/images/other/play all_normal.svg", QSize(18, 17));

    DLabel* Label1 = new DLabel(m_pSlideShowBtn);
    Label1->move(6,7);
    DLabel* Label2 = new DLabel(m_pSlideShowBtn);
    Label2->setFixedSize(70,18);
    Label2->move(29,6);

    Label1->setPixmap(pixmap);
    Label1->setPalette(pal);

    Label2->setText("幻灯片放映");

    QFont ft1 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft1.setFamily("SourceHanSansSC-Medium");
    ft1.setWeight(QFont::Medium);

    Label2->setFont(ft1);
    Label2->setPalette(pal);

    m_pSearchResultLabel = new DLabel();

    pHBoxLayout->addSpacing(5);
    pHBoxLayout->addWidget(m_pSlideShowBtn);
    pHBoxLayout->addSpacing(5);
    pHBoxLayout->addWidget(m_pSearchResultLabel);

    m_pThumbnailListView = new ThumbnailListView();

    m_pThumbnailListView->setStyleSheet("background-color:rgb(248, 248, 248)");
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

void SearchView::improtSearchResultsIntoThumbnailView(QString s)
{
    m_keywords = s;
    QList<ThumbnailListView::ItemInfo> thumbnaiItemList;
    auto infos = DBManager::instance()->getInfosForKeyword(s);

    if (0 < infos.length())
    {
        for(auto info : infos)
        {
            ThumbnailListView::ItemInfo vi;
            vi.name = info.fileName;
            vi.path = info.filePath;
            vi.image = dApp->m_imagemap.value(info.filePath);//TODO_DS
            thumbnaiItemList<<vi;
        }

        m_pThumbnailListView->insertThumbnails(thumbnaiItemList);

        QString searchStr = tr("共搜到%1张照片");
        QString str = QString::number(infos.length());
        m_pSearchResultLabel->setText(searchStr.arg(str));
        m_pSearchResultLabel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T6));
        DPalette pa = DApplicationHelper::instance()->palette(m_pSearchResultLabel);
        pa.setBrush(DPalette::WindowText, pa.color(DPalette::Text));
        m_pSearchResultLabel->setPalette(pa);

        m_stackWidget->setCurrentIndex(1);
    }
    else
    {
        QString str = tr("没有“%1”的结果，请尝试搜索新词。");
        m_pNoSearchResultLabel->setText(str.arg(s));
        m_pNoSearchResultLabel->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T8));
        DPalette pa = DApplicationHelper::instance()->palette(m_pNoSearchResultLabel);
        pa.setBrush(DPalette::WindowText, pa.color(DPalette::TextTips));
        m_pNoSearchResultLabel->setPalette(pa);

        m_stackWidget->setCurrentIndex(0);
    }
}

void SearchView::updateSearchResultsIntoThumbnailView()
{
    improtSearchResultsIntoThumbnailView(m_keywords);
}

void SearchView::changeTheme()
{
    //无结果
    DPalette pa = DApplicationHelper::instance()->palette(pNoResult);
    pa.setBrush(DPalette::WindowText, pa.color(DPalette::Text));
    pNoResult->setPalette(pa);
    //尝试新搜索词
    DPalette pal = DApplicationHelper::instance()->palette(m_pNoSearchResultLabel);
    pal.setBrush(DPalette::WindowText, pal.color(DPalette::TextTips));
    m_pNoSearchResultLabel->setPalette(pal);
    //"搜索结果"
    DPalette pale = DApplicationHelper::instance()->palette(pLabel1);
    pale.setBrush(DPalette::WindowText, pale.color(DPalette::ToolTipText));
    pLabel1->setPalette(pale);

}

