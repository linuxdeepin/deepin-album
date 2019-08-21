#include "mainwindow.h"

MainWindow::MainWindow()
{

    initUI();
    initTitleBar();
    initCentralWidget();
//    m_listWidget = new QListWidget();
//    m_listWidget->setViewMode(QListView::ListMode);

//    QListWidgetItem *item = new QListWidgetItem;
//    item->setText("已导入");
//    m_listWidget->addItem(item);

//    QListWidgetItem *item2 = new QListWidgetItem;
//    item2->setText("回收站");
//    m_listWidget->addItem(item2);

//    QListWidgetItem *item3 = new QListWidgetItem;
//    item3->setText("个人收藏");
//    m_listWidget->addItem(item3);

//    QVBoxLayout* pLayout = new QVBoxLayout();

//    pLayout->addWidget(m_listWidget);

//    pCenterFrame->setLayout(pLayout);

    initConnections();
}

MainWindow::~MainWindow()
{

}

void MainWindow::initConnections()
{
    connect(m_allPicBtn, &QPushButton::clicked, this, &MainWindow::allPicBtnClicked);
    connect(m_timeLineBtn, &QPushButton::clicked, this, &MainWindow::timeLineBtnClicked);
    connect(m_albumBtn, &QPushButton::clicked, this, &MainWindow::albumBtnClicked);
}

void MainWindow::initUI()
{
    resize(DEFAULT_WINDOWS_WIDTH, DEFAULT_WINDOWS_HEIGHT);
    setMinimumSize(MIX_WINDOWS_WIDTH, MIX_WINDOWS_HEIGHT);
}

void MainWindow::initTitleBar()
{
//    DSegmentedControl* pDSegmentedControl = new DSegmentedControl();
//    pDSegmentedControl->addSegmented("所有照片");
//    pDSegmentedControl->addSegmented("时间线");
//    pDSegmentedControl->addSegmented("相册");
//    titlebar()->setCustomWidget(pDSegmentedControl, true);

    m_titleBtnWidget = new QWidget();

    QHBoxLayout* pTitleBtnLayout = new QHBoxLayout();
    m_allPicBtn = new QPushButton();
    m_timeLineBtn = new QPushButton();
    m_albumBtn = new QPushButton();

    m_allPicBtn->setText("所有照片");
    m_timeLineBtn->setText("时间线");
    m_albumBtn->setText("相册");

    pTitleBtnLayout->addWidget(m_allPicBtn);
    pTitleBtnLayout->addWidget(m_timeLineBtn);
    pTitleBtnLayout->addWidget(m_albumBtn);
    pTitleBtnLayout->setSpacing(10);

    m_titleBtnWidget->setLayout(pTitleBtnLayout);

    titlebar()->setCustomWidget(m_titleBtnWidget, Qt::AlignLeft);
}

void MainWindow::initCentralWidget()
{
    m_pCenterWidget = new QStackedWidget;
    m_pAlbumview = new AlbumView();
    m_pAllPicView = new AllPicView();
    m_pTimeLineView = new TimeLineView();

    m_pCenterWidget->addWidget(m_pAllPicView);
    m_pCenterWidget->addWidget(m_pTimeLineView);
    m_pCenterWidget->addWidget(m_pAlbumview);

    setCentralWidget(m_pCenterWidget);
}

void MainWindow::initStatusBar()
{
    m_pStatusBar = statusBar();

}

void MainWindow::allPicBtnClicked()
{
    m_pCenterWidget->setCurrentIndex(0);
}

void MainWindow::timeLineBtnClicked()
{
    m_pCenterWidget->setCurrentIndex(1);
}

void MainWindow::albumBtnClicked()
{
    m_pCenterWidget->setCurrentIndex(2);
}
