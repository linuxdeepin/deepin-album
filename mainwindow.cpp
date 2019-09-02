#include "mainwindow.h"

MainWindow::MainWindow()
{
    m_allPicNum = DBManager::instance()->getImgsCount();

    initUI();
    initTitleBar();
    initCentralWidget();
    initStatusBar();
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
    connect(m_pAllPicBtn, &DPushButton::clicked, this, &MainWindow::allPicBtnClicked);
    connect(m_pTimeLineBtn, &DPushButton::clicked, this, &MainWindow::timeLineBtnClicked);
    connect(m_pAlbumBtn, &DPushButton::clicked, this, &MainWindow::albumBtnClicked);
}

void MainWindow::initUI()
{
    resize(DEFAULT_WINDOWS_WIDTH, DEFAULT_WINDOWS_HEIGHT);
    setMinimumSize(MIX_WINDOWS_WIDTH, MIX_WINDOWS_HEIGHT);
}

void MainWindow::initTitleBar()
{
    m_titleBtnWidget = new QWidget();

    QHBoxLayout* pTitleBtnLayout = new QHBoxLayout();

    QLabel* pLabel = new QLabel();
    pLabel->setFixedSize(22, 22);

    QPixmap pixmap;
    pixmap = utils::base::renderSVG(":/resources/images/logo/deepin-image-viewer.svg", QSize(22, 22));

    pLabel->setPixmap(pixmap);

    m_pAllPicBtn = new DPushButton();
    m_pTimeLineBtn = new DPushButton();
    m_pAlbumBtn = new DPushButton();

    m_pAllPicBtn->setText("所有照片");
    m_pTimeLineBtn->setText("时间线");
    m_pAlbumBtn->setText("相册");

    m_pSearchEdit = new DSearchEdit();
    m_pSearchEdit->setFixedSize(278, 26);
    m_pSearchEdit->setPlaceHolder(tr("Search"));

    pTitleBtnLayout->addStretch();
    pTitleBtnLayout->addWidget(pLabel);
    pTitleBtnLayout->addSpacing(15);
    pTitleBtnLayout->addWidget(m_pAllPicBtn);
    pTitleBtnLayout->addSpacing(5);
    pTitleBtnLayout->addWidget(m_pTimeLineBtn);
    pTitleBtnLayout->addSpacing(5);
    pTitleBtnLayout->addWidget(m_pAlbumBtn);
    pTitleBtnLayout->addSpacing(50);
    pTitleBtnLayout->addWidget(m_pSearchEdit);
    pTitleBtnLayout->addStretch();

    m_titleBtnWidget->setLayout(pTitleBtnLayout);

    titlebar()->addWidget(m_titleBtnWidget, Qt::AlignLeft);
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
    m_pStatusBar = new QStatusBar(this);
    setStatusBar(m_pStatusBar);

    QWidget* pWidget = new QWidget();

    QString str = tr("%1张照片");

    QHBoxLayout* pHBoxLayout = new QHBoxLayout();

    m_pAllPicNumLabel = new DLabel();
    m_pAllPicNumLabel->setText(str.arg(QString::number(m_allPicNum)));
    m_pAllPicNumLabel->setAlignment(Qt::AlignCenter);

    m_pSlider = new DSlider();
    m_pSlider->setOrientation(Qt::Horizontal);
    m_pSlider->adjustSize();
    m_pSlider->setFixedWidth(120);

    pHBoxLayout->addWidget(m_pAllPicNumLabel, Qt::AlignHCenter);
    pHBoxLayout->addWidget(m_pSlider, Qt::AlignRight);

    pWidget->setLayout(pHBoxLayout);

    statusBar()->addWidget(pWidget, 1);
    statusBar()->setSizeGripEnabled(false);
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

void MainWindow::resizeEvent(QResizeEvent *event)
{
    int wid = width();
}
