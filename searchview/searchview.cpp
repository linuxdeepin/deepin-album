#include "searchview.h"

namespace {
const QString RECENT_IMPORTED_ALBUM = "Recent imported";
const QString TRASH_ALBUM = "Trash";
const QString FAVORITES_ALBUM = "My favorite";
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

        emit dApp->signalM->viewImage(info);
        emit dApp->signalM->showImageView(VIEW_MAINWINDOW_SEARCH);
    });
}

void SearchView::initNoSearchResultView()
{
    m_pNoSearchResultView = new DWidget();

    QVBoxLayout* pNoSearchResultLayout = new QVBoxLayout();

    DLabel* pLabel1 = new DLabel();
    pLabel1->setText("无结果");

    m_pNoSearchResultLabel = new DLabel();

    pNoSearchResultLayout->addStretch();
    pNoSearchResultLayout->addWidget(pLabel1, 0, Qt::AlignCenter);
    pNoSearchResultLayout->addSpacing(20);
    pNoSearchResultLayout->addWidget(m_pNoSearchResultLabel, 0, Qt::AlignCenter);
    pNoSearchResultLayout->addStretch();

    m_pNoSearchResultView->setLayout(pNoSearchResultLayout);
}

void SearchView::initSearchResultView()
{
    m_pSearchResultView = new DWidget();

    QVBoxLayout* pSearchResultLayout = new QVBoxLayout();
    pSearchResultLayout->setSpacing(10);

    DLabel* pLabel1 = new DLabel();
    pLabel1->setText("搜索结果");

    QHBoxLayout* pHBoxLayout = new QHBoxLayout();
    pHBoxLayout->setSpacing(5);

    m_pSlideShowBtn = new DPushButton();
    m_pSlideShowBtn->setFixedSize(142, 42);
    m_pSlideShowBtn->setText("播放幻灯片");

    m_pSearchResultLabel = new DLabel();

    pHBoxLayout->addWidget(m_pSlideShowBtn);
    pHBoxLayout->addWidget(m_pSearchResultLabel);

    m_pThumbnailListView = new ThumbnailListView();

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
            thumbnaiItemList<<vi;
        }

        m_pThumbnailListView->insertThumbnails(thumbnaiItemList);

        QString searchStr = tr("共搜到%1张照片");
        QString str = QString::number(infos.length());
        m_pSearchResultLabel->setText(searchStr.arg(str));

        m_stackWidget->setCurrentIndex(1);
    }
    else
    {
        QString str = tr("没有“%1”的结果，请尝试搜索新词。");
        m_pNoSearchResultLabel->setText(str.arg(s));

        m_stackWidget->setCurrentIndex(0);
    }
}
