#include "timelineview.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "utils/snifferimageformat.h"

#include <QScrollBar>
#include <QScroller>
#include <DPushButton>
#include <QMimeData>
#include <DTableView>
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

    pTimeLineViewWidget = new DWidget();
    pImportView = new ImportView();
    pSearchView = new SearchView();

    addWidget(pImportView);
    addWidget(pTimeLineViewWidget);
    addWidget(pSearchView);

    initTimeLineViewWidget();

    updataLayout();

    initConnections();
}

void TimeLineView::initConnections(){
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &TimeLineView::updataLayout);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &TimeLineView::updataLayout);
    connect(dApp, &Application::sigFinishLoad, this, &TimeLineView::updataLayout);
    connect(m_mainListWidget,&TimelineList::sigNewTime,this,[=](QString date,QString num,int index){
        m_index = index;
        on_AddLabel(date,num);
    });

    connect(m_mainListWidget,&TimelineList::sigDelTime,this,[=](){
        on_DelLabel();
    });

    connect(m_mainListWidget,&TimelineList::sigMoveTime,this,[=](int y){
        on_MoveLabel(y);
    });

    connect(dApp->signalM, &SignalManager::sigPixMapRotate, this, &TimeLineView::onPixMapRotate);
}

void TimeLineView::initTimeLineViewWidget()
{
    m_mainLayout = new QVBoxLayout();
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    pTimeLineViewWidget->setLayout(m_mainLayout);

    m_mainListWidget = new TimelineList;
    m_mainListWidget->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    m_mainListWidget->verticalScrollBar()->setSingleStep(5);
    m_mainLayout->addWidget(m_mainListWidget);

    //添加悬浮title
    m_dateItem = new QWidget(pTimeLineViewWidget);
    QVBoxLayout *TitleViewLayout = new QVBoxLayout();
    m_dateItem->setLayout(TitleViewLayout);
    DLabel* pDate = new DLabel();
    pDate->setFixedHeight(24);

    QFont ft3 = DFontSizeManager::instance()->get(DFontSizeManager::T3);
    ft3.setFamily("SourceHanSansSC");
    ft3.setWeight(QFont::Medium);

    DPalette color = DApplicationHelper::instance()->palette(pDate);
    color.setBrush(DPalette::WindowText, color.color(DPalette::ToolTipText));

    pDate->setFont(ft3);
    pDate->setPalette(color);

    DLabel* pNum = new DLabel();
    pNum->setFixedHeight(24);
    QFont ft6 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft6.setFamily("SourceHanSansSC");
    ft6.setWeight(QFont::Medium);
    pNum->setFont(ft6);
    DPalette pal = DApplicationHelper::instance()->palette(pNum);
    pal.setBrush(DPalette::WindowText, QColor(119,119,119));
    pNum->setPalette(pal);

    TitleViewLayout->addWidget(pDate);
    TitleViewLayout->addWidget(pNum);

    QHBoxLayout *Layout = new QHBoxLayout();
    pSuspensionChose = new DCommandLinkButton("选择");

//    QFont ftt;
//    ftt.setPixelSize(18);
//    pSuspensionChose->setFont(ftt);
    pSuspensionChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
    pSuspensionChose->setFixedHeight(24);
    pSuspensionChose->resize(36,27);

    pNum->setLayout(Layout);
    Layout->addStretch(1);
    Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    Layout->setContentsMargins(0,0,22,0);
    Layout->addWidget(pSuspensionChose);
    connect(pSuspensionChose, &DCommandLinkButton::clicked, this, [=]{
        if ("选择" == pSuspensionChose->text())
        {
            pSuspensionChose->setText("取消选择");
            QList<ThumbnailListView*> p = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<ThumbnailListView*>();
            p[0]->selectAll();
        }
        else
        {
            pSuspensionChose->setText("选择");
            QList<ThumbnailListView*> p = m_mainListWidget->itemWidget(m_mainListWidget->item(m_index))->findChildren<ThumbnailListView*>();
            p[0]->clearSelection();
        }
    });
    QPalette ppal(m_dateItem->palette());
    ppal.setColor(QPalette::Background,  QColor(0xff,0xff,0xff,0xf9));
    m_dateItem->setAutoFillBackground(true);
    m_dateItem->setPalette(ppal);

    m_dateItem->setFixedSize(this->width(),87);
    m_dateItem->setContentsMargins(10,0,0,0);
    m_dateItem->move(0,0);
    m_dateItem->show();
    m_dateItem->setVisible(false);
}

void TimeLineView::updateStackedWidget()
{
    if (0 < DBManager::instance()->getImgsCount())
    {
        setCurrentIndex(VIEW_TIMELINE);
    }
    else
    {
        setCurrentIndex(VIEW_IMPORT);
    }
}

void TimeLineView::updataLayout()
{
    //获取所有时间线
    m_mainListWidget->clear();
    m_timelines = DBManager::instance()->getAllTimelines();
    for(int i = 0; i < m_timelines.size(); i++)
    {
        //获取当前时间图片
        DBImgInfoList ImgInfoList= DBManager::instance()->getInfosByTimeline(m_timelines.at(i));

        QListWidgetItem *item = new QListWidgetItem;
        TimelineItem *listItem = new TimelineItem;
        QVBoxLayout *listItemlayout=new QVBoxLayout();
        listItem->setLayout(listItemlayout);
        listItemlayout->setMargin(0);
        listItemlayout->setSpacing(0);
        listItemlayout->setContentsMargins(0,0,0,0);

        //添加title
        QWidget *TitleView = new QWidget;
        QVBoxLayout *TitleViewLayout = new QVBoxLayout();
        TitleView->setLayout(TitleViewLayout);
        DLabel* pDate = new DLabel();
        pDate->setFixedHeight(24);
        QStringList datelist = m_timelines.at(i).split(".");
        listItem->m_sdate=QString("%1年%2月%3日").arg(datelist[0]).arg(datelist[1]).arg(datelist[2]);
        pDate->setText(listItem->m_sdate);

        DPalette color = DApplicationHelper::instance()->palette(pDate);
        color.setBrush(DPalette::Text, color.color(DPalette::ToolTipText));

        QFont ft3 = DFontSizeManager::instance()->get(DFontSizeManager::T3);
        ft3.setFamily("SourceHanSansSC");
        ft3.setWeight(QFont::Medium);

        pDate->setFont(ft3);
        pDate->setPalette(color);

        listItem->m_date=pDate;

        DLabel* pNum = new DLabel();
        pNum->setFixedHeight(24);
        listItem->m_snum = QString("%1张照片").arg(ImgInfoList.size());
        pNum->setText(listItem->m_snum);
        QFont ft6 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
        ft6.setFamily("SourceHanSansSC");
        ft6.setWeight(QFont::Medium);
        pNum->setFont(ft6);
        DPalette pal = DApplicationHelper::instance()->palette(pNum);
        pal.setBrush(DPalette::Text, QColor(119,119,119));
        pNum->setPalette(pal);

        QHBoxLayout *Layout = new QHBoxLayout();
        DCommandLinkButton *pChose = new DCommandLinkButton("选择");

        pChose->setFont(DFontSizeManager::instance()->get(DFontSizeManager::T5));
//        QFont fttt;
//        fttt.setPixelSize(18);
//        pChose->setFont(fttt);

        pChose->setFixedHeight(24);
        pChose->resize(36,27);

        pNum->setLayout(Layout);
        Layout->addStretch(1);
        Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        Layout->setContentsMargins(0,0,0,0);
        Layout->addWidget(pChose);

        listItem->m_num=pNum;
        TitleViewLayout->addWidget(pDate);
        TitleViewLayout->addWidget(pNum);
        TitleView->setFixedHeight(87);
        listItem->m_title = TitleView;

        //添加图片
        ThumbnailListView *pThumbnailListView = new ThumbnailListView();
        pThumbnailListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        pThumbnailListView->setContextMenuPolicy(Qt::CustomContextMenu);
        pThumbnailListView->setContentsMargins(0,0,0,0);
        pThumbnailListView->setFrameShape(DTableView::NoFrame);


        QList<ThumbnailListView::ItemInfo> thumbnaiItemList;
        for(int j = 0; j < ImgInfoList.size(); j++){
            ThumbnailListView::ItemInfo vi;
            vi.name = ImgInfoList.at(j).fileName;
            vi.path = ImgInfoList.at(j).filePath;
            vi.image = dApp->m_imagemap.value(ImgInfoList.at(j).filePath);
            thumbnaiItemList.append(vi);
        }
        //保存当前时间图片
       pThumbnailListView->insertThumbnails(thumbnaiItemList);

       listItemlayout->addWidget(TitleView);
       listItemlayout->addWidget(pThumbnailListView);
       item->setFlags(Qt::NoItemFlags);
       m_mainListWidget->addItemForWidget(item);
       m_mainListWidget->setItemWidget(item,listItem);
       connect(pThumbnailListView,&ThumbnailListView::loadend,this,[=](int h){
           pThumbnailListView->setFixedHeight(h);
           listItem->setFixedHeight(TitleView->height()+h);
           item->setSizeHint(listItem->rect().size());
       });
       connect(pThumbnailListView,&ThumbnailListView::openImage,this,[=](int index){
           SignalManager::ViewInfo info;
           info.album = "";
           info.lastPanel = nullptr;
           if(ImgInfoList.size()<=1){
               info.paths.clear();
           }else {
               for(auto image : ImgInfoList)
               {
                   info.paths<<image.filePath;
               }
            }
           info.path = ImgInfoList[index].filePath;
           info.viewType = utils::common::VIEW_TIMELINE_SRN;

           emit dApp->signalM->viewImage(info);
           emit dApp->signalM->showImageView(VIEW_MAINWINDOW_TIMELINE);
       });
       connect(pThumbnailListView,&ThumbnailListView::menuOpenImage,this,[=](QString path,QStringList paths,bool isFullScreen, bool isSlideShow){
           SignalManager::ViewInfo info;
           info.album = "";
           info.lastPanel = nullptr;
           if(paths.size()>1){
               info.paths = paths;
           }else
           {
               if(ImgInfoList.size()>1){
                   for(auto image : ImgInfoList)
                   {
                       info.paths<<image.filePath;
                   }
               }else {
                 info.paths.clear();
                }
           }
           info.path = path;
           info.fullScreen = isFullScreen;
           info.slideShow = isSlideShow;
           info.viewType = utils::common::VIEW_TIMELINE_SRN;

           emit dApp->signalM->viewImage(info);
           emit dApp->signalM->showImageView(VIEW_MAINWINDOW_TIMELINE);
       });
       connect(pChose, &DCommandLinkButton::clicked, this, [=]{
           if ("选择" == pChose->text())
           {
               pChose->setText("取消选择");
               pThumbnailListView->selectAll();
           }
           else
           {
               pChose->setText("选择");
               pThumbnailListView->clearSelection();
           }
       });
       connect(pThumbnailListView,&ThumbnailListView::clicked,this,[=]{
            QStringList paths = pThumbnailListView->selectedPaths();
            if (pThumbnailListView->model()->rowCount() == paths.length() && "选择" == pChose->text())
            {
                pChose->setText("取消选择");
            }

            if (pThumbnailListView->model()->rowCount() != paths.length() && "取消选择" == pChose->text())
            {
                pChose->setText("选择");
            }
       });
    }

    if(VIEW_SEARCH == currentIndex())
    {
        // donothing
    }
    else
    {
        updateStackedWidget();
    }
}

void TimeLineView::onPixMapRotate(QStringList paths)
{
    dApp->m_imageloader->updateImageLoader(paths);

    updataLayout();
}

void TimeLineView::on_AddLabel(QString date,QString num)
{
    if((nullptr != m_dateItem)&&(nullptr != m_mainListWidget))
    {
        QList<QLabel*> labelList = m_dateItem->findChildren<QLabel*>();
        labelList[0]->setText(date);
        labelList[1]->setText(num);
        m_dateItem->setVisible(true);
        m_dateItem->move(0,0);
    }
}

void TimeLineView::on_DelLabel()
{
    if(nullptr != m_dateItem)
    {
        m_dateItem->setVisible(false);
    }
}

void TimeLineView::on_MoveLabel(int y)
{
    if((nullptr != m_dateItem)&&(nullptr != m_mainListWidget))
    {
        m_dateItem->move(0,y + 1);
    }
}

void TimeLineView::resizeEvent(QResizeEvent *ev){
    m_dateItem->setFixedSize(width(),87);
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

    if (paths.isEmpty())
    {
        return;
    }

    DBImgInfoList dbInfos;

    using namespace utils::image;

    for (auto path : paths)
    {
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

    if (! dbInfos.isEmpty())
    {
        QStringList paths;
        for(auto info : dbInfos)
        {
            paths<<info.filePath;
        }

        dApp->m_imageloader->addImageLoader(paths);
        DBManager::instance()->insertImgInfos(dbInfos);
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
