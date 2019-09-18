#include "timelineview.h"
#include <QScrollBar>
#include <QScroller>
#include <DPushButton>

void TimeLineView::initUI()
{
    m_index = 0;
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainListWidget = new TimelineList;
    m_mainLayout->addWidget(m_mainListWidget);
    m_mainListWidget->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    m_mainListWidget->verticalScrollBar()->setSingleStep(5);

    //添加悬浮title
    m_dateItem = new QWidget(this);
    QVBoxLayout *TitleViewLayout = new QVBoxLayout();
    m_dateItem->setLayout(TitleViewLayout);
    DLabel* pDate = new DLabel();
    pDate->setFixedHeight(24);

    DLabel* pNum = new DLabel();
    pNum->setFixedHeight(24);
    TitleViewLayout->addWidget(pDate);
    TitleViewLayout->addWidget(pNum);

    QHBoxLayout *Layout = new QHBoxLayout();
    pSuspensionChose = new DPushButton();
    pSuspensionChose->setText("选择");
    pSuspensionChose->setFixedHeight(24);
    pNum->setLayout(Layout);
    Layout->addStretch(1);
    Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    Layout->setContentsMargins(0,0,0,0);
    Layout->addWidget(pSuspensionChose);
    connect(pSuspensionChose, &DPushButton::clicked, this, [=]{
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
    ppal.setColor(QPalette::Background,  QColor(0xff,0xff,0xff,0xf0));
    m_dateItem->setAutoFillBackground(true);
    m_dateItem->setPalette(ppal);

    m_dateItem->setFixedSize(this->width(),50);
    m_dateItem->setContentsMargins(0,0,0,0);
    m_dateItem->move(0,0);
    m_dateItem->show();
    m_dateItem->setVisible(false);
}

void TimeLineView::updataLayout(){
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
        listItem->m_sdate=QString("%1 年 %2 月 %3 日").arg(datelist[0]).arg(datelist[1]).arg(datelist[2]);
        pDate->setText(listItem->m_sdate);
        listItem->m_date=pDate;

        DLabel* pNum = new DLabel();
        pNum->setFixedHeight(24);
        listItem->m_snum = QString("%1 张").arg(ImgInfoList.size());
        pNum->setText(listItem->m_snum);

        QHBoxLayout *Layout = new QHBoxLayout();
        DPushButton *pChose = new DPushButton();
        pChose->setText("选择");
        pChose->setFixedHeight(24);
        pNum->setLayout(Layout);
        Layout->addStretch(1);
        Layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        Layout->setContentsMargins(0,0,0,0);
        Layout->addWidget(pChose);



        listItem->m_num=pNum;
        TitleViewLayout->addWidget(pDate);
        TitleViewLayout->addWidget(pNum);
        TitleView->setFixedHeight(50);
        listItem->m_title = TitleView;

        //添加图片
        ThumbnailListView *pThumbnailListView = new ThumbnailListView();
        pThumbnailListView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        pThumbnailListView->setContextMenuPolicy(Qt::CustomContextMenu);
        pThumbnailListView->setContentsMargins(0,0,0,0);

        QList<ThumbnailListView::ItemInfo> thumbnaiItemList;
        for(int j = 0; j < ImgInfoList.size(); j++){
            ThumbnailListView::ItemInfo vi;
            vi.name = ImgInfoList.at(j).fileName;
            vi.path = ImgInfoList.at(j).filePath;
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
           emit dApp->signalM->viewImage(info);
           emit dApp->signalM->showImageView(2);
       });
       connect(pThumbnailListView,&ThumbnailListView::menuOpenImage,this,[=](QString path,QStringList paths,bool isFullScreen){
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
           emit dApp->signalM->viewImage(info);
           emit dApp->signalM->showImageView(2);
       });
       connect(pChose, &DPushButton::clicked, this, [=]{
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

}
void TimeLineView::initConnections(){
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &TimeLineView::updataLayout);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &TimeLineView::updataLayout);
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
}
TimeLineView::TimeLineView()
{
    initUI();
    updataLayout();
    initConnections();
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
    m_dateItem->setFixedSize(width(),50);
}
