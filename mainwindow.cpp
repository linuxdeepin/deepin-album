#include "mainwindow.h"
#include "dialogs/albumcreatedialog.h"
namespace  {
const QString TITLEBAR_NEWALBUM = "New albume";
const QString TITLEBAR_IMPORT = "Import";

}//namespace


MainWindow::MainWindow()
{
    m_allPicNum = DBManager::instance()->getImgsCount();

    initUI();
    initTitleBar();
    initCentralWidget();
    initStatusBar();

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
    connect(dApp->signalM, &SignalManager::createAlbum,this, &MainWindow::onCreateAlbum);
    connect(m_pSearchEdit, &DSearchEdit::editingFinished, this, &MainWindow::editingFinishedClicked);
    connect(m_pTitleBarMenu, &DMenu::triggered, this, &MainWindow::onTitleBarMenuClicked);
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

    m_pTitleBarMenu = new DMenu();
    QAction *pNewAlbum = new QAction();
    pNewAlbum->setText(TITLEBAR_NEWALBUM);
    m_pTitleBarMenu->addAction(pNewAlbum);

    QAction *pImport = new QAction();
    pImport->setText(TITLEBAR_IMPORT);
    m_pTitleBarMenu->addAction(pImport);
    m_pTitleBarMenu->addSeparator();

    titlebar()->addWidget(m_titleBtnWidget, Qt::AlignLeft);
    titlebar()->setMenu(m_pTitleBarMenu);
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

void MainWindow::onTitleBarMenuClicked(QAction *action)
{
    if(TITLEBAR_NEWALBUM == action->text())
    {
        dApp->signalM->createAlbum();
    }
}

void MainWindow::onCreateAlbum(QStringList imagepaths)
{
    if (m_pCenterWidget->currentWidget() == m_pAlbumview)
    {
//        m_pAlbumview->createAlbum();
    }
    else
    {
        showCreateDialog(imagepaths);
    }
}

void MainWindow::showCreateDialog(QStringList imgpaths)
{
    AlbumCreateDialog *d = new AlbumCreateDialog;
    d->showInCenter(window());

    connect(d, &AlbumCreateDialog::albumAdded, this, [=]{
        if (m_pCenterWidget->currentWidget() != m_pAllPicView &&
                m_pCenterWidget->currentWidget() != m_pTimeLineView)
        {
            m_pCenterWidget->setCurrentWidget(m_pAlbumview);
        }

        DBManager::instance()->insertIntoAlbum(d->getCreateAlbumName(), imgpaths.isEmpty()?QStringList(" "):imgpaths);
    });
}

void MainWindow::editingFinishedClicked()
{
    QString keywords = m_pSearchEdit->text();
    if (! keywords.isEmpty())
    {
        emit dApp->signalM->sigSendKeywordsIntoALLPic(keywords);
    }
}
