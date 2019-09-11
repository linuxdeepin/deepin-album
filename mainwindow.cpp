#include "mainwindow.h"
#include "controller/commandline.h"
#include "dialogs/albumcreatedialog.h"
namespace  {
const int VIEW_IMPORT = 0;
const int VIEW_ALLPIC = 1;
const int VIEW_TIMELINE = 2;
const int VIEW_ALBUM = 3;
const int VIEW_SEARCH = 4;
const int VIEW_IMAGE = 5;

const QString TITLEBAR_NEWALBUM = "New albume";
const QString TITLEBAR_IMPORT = "Import";

}//namespace


MainWindow::MainWindow()
{
    m_allPicNum = DBManager::instance()->getImgsCount();
    m_iCurrentView = VIEW_ALLPIC;

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
    connect(m_pSearchEdit, &DSearchEdit::editingFinished, this, &MainWindow::onSearchEditFinished);
    connect(m_pTitleBarMenu, &DMenu::triggered, this, &MainWindow::onTitleBarMenuClicked);
    connect(dApp->signalM, &SignalManager::sigUpdateAllpicsNumLabel, this, &MainWindow::onUpdateAllpicsNumLabel);
    connect(m_pImportView->m_pImportBtn, &DPushButton::clicked, this, &MainWindow::onImprotBtnClicked);
    connect(this, &MainWindow::sigTitleMenuImportClicked, this, &MainWindow::onImprotBtnClicked);
    connect(this, &MainWindow::sigImprotPicsIntoDB, DBManager::instance(), &DBManager::insertImgInfos);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &MainWindow::onUpdateCentralWidget);
	connect(dApp->signalM,&SignalManager::showImageView,this,[=](int index){
        m_backIndex = index;
        m_pCenterWidget->setCurrentIndex(VIEW_IMAGE);
    });
    connect(dApp->signalM,&SignalManager::hideImageView,this,[=](){
        m_pCenterWidget->setCurrentIndex(m_backIndex);
    });
    connect(dApp->signalM,&SignalManager::exportImage,this,[=](QStringList paths){
        Exporter::instance()->exportImage(paths);
    });
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
    pLabel->setFixedSize(33, 32);

    QPixmap pixmap;
    pixmap = utils::base::renderSVG(":/resources/images/other/deepin-photo-album.svg", QSize(33, 32));

    pLabel->setPixmap(pixmap);

    m_pAllPicBtn = new DPushButton();
    m_pTimeLineBtn = new DPushButton();
    m_pAlbumBtn = new DPushButton();

    m_pAllPicBtn->setText("所有照片");
    m_pTimeLineBtn->setText("时间线");
    m_pAlbumBtn->setText("相册");

    m_pSearchEdit = new DSearchEdit();
    m_pSearchEdit->setFixedSize(278, 26);

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
    m_pSearchView = new SearchView();
    m_pImportView = new ImportView();

    m_pCenterWidget->addWidget(m_pImportView);
    m_pCenterWidget->addWidget(m_pAllPicView);
    m_pCenterWidget->addWidget(m_pTimeLineView);
    m_pCenterWidget->addWidget(m_pAlbumview);
    m_pCenterWidget->addWidget(m_pSearchView);
    m_commandLine = CommandLine::instance();
    m_commandLine->processOption();
    m_pCenterWidget->addWidget(m_commandLine);
    m_pCenterWidget->setCurrentIndex(DBManager::instance()->getImgsCount()>0?VIEW_ALLPIC:VIEW_IMPORT);

    setCentralWidget(m_pCenterWidget);
}

void MainWindow::onUpdateCentralWidget()
{
    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
}

void MainWindow::initStatusBar()
{  
    m_pStatusBar = new DStatusBar(this);
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
    m_pSlider->setPageStep(20);
    m_pSlider->setTickInterval(1);
    m_pSlider->setValue(120);

    pHBoxLayout->addWidget(m_pAllPicNumLabel, Qt::AlignHCenter);
    pHBoxLayout->addWidget(m_pSlider, Qt::AlignRight);

    pWidget->setLayout(pHBoxLayout);

    statusBar()->addWidget(pWidget, 1);
    statusBar()->setSizeGripEnabled(false);
}

void MainWindow::allPicBtnClicked()
{
    if (VIEW_ALLPIC == m_iCurrentView)
    {
        return;
    }
    else
    {
        m_pSearchEdit->clear();

        int num = DBManager::instance()->getImgsCount();
        onUpdateAllpicsNumLabel(num);

        m_iCurrentView = VIEW_ALLPIC;

        if (0 != num)
        {
            m_pCenterWidget->setCurrentIndex(m_iCurrentView);
        }
    }
}

void MainWindow::timeLineBtnClicked()
{
    if (VIEW_TIMELINE == m_iCurrentView)
    {
        return;
    }
    else
    {
        m_pSearchEdit->clear();

        int num = DBManager::instance()->getImgsCount();
        onUpdateAllpicsNumLabel(num);

        m_iCurrentView = VIEW_TIMELINE;

        if (0 != num)
        {
            m_pCenterWidget->setCurrentIndex(m_iCurrentView);
        }
    }
}

void MainWindow::albumBtnClicked()
{
    if (VIEW_ALBUM == m_iCurrentView)
    {
        return;
    }
    else
    {
        m_pSearchEdit->clear();

        onUpdateAllpicsNumLabel(m_pAlbumview->m_iAlubmPicsNum);

        m_iCurrentView = VIEW_ALBUM;

        if (0 != m_pAlbumview->m_iAlubmPicsNum)
        {
            m_pCenterWidget->setCurrentIndex(m_iCurrentView);
        }
    }
}

void MainWindow::onTitleBarMenuClicked(QAction *action)
{
    if(TITLEBAR_NEWALBUM == action->text())
    {
        dApp->signalM->createAlbum();
    }
    else if (TITLEBAR_IMPORT == action->text())
    {
        emit sigTitleMenuImportClicked();
    }
    else
    {

    }
}

void MainWindow::onCreateAlbum(QStringList imagepaths)
{
    if (m_pCenterWidget->currentWidget() == m_pAlbumview)
    {
        m_pAlbumview->createNewAlbum();
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
        if (m_pCenterWidget->currentIndex() != VIEW_ALBUM)
        {
            m_iCurrentView = VIEW_ALBUM;
            m_pCenterWidget->setCurrentIndex(VIEW_ALBUM);
        }

        DBManager::instance()->insertIntoAlbum(d->getCreateAlbumName(), imgpaths.isEmpty()?QStringList(" "):imgpaths);
        emit dApp->signalM->sigCreateNewAlbumFromDialog();
    });
}

void MainWindow::onSearchEditFinished()
{
    QString keywords = m_pSearchEdit->text();
    if (keywords.isEmpty())
    {
        m_pCenterWidget->setCurrentIndex(m_iCurrentView);
    }
    else
    {
        emit dApp->signalM->sigSendKeywordsIntoALLPic(keywords);
        m_pCenterWidget->setCurrentIndex(VIEW_SEARCH);
    }
}

void MainWindow::onUpdateAllpicsNumLabel(int num)
{
    QString str = tr("%1张照片");

    m_pAllPicNumLabel->setText(str.arg(QString::number(num)));
}

void MainWindow::onImprotBtnClicked()
{
    static QStringList sList;

    for (const QByteArray &i : QImageReader::supportedImageFormats())
        sList << "*." + QString::fromLatin1(i);

    QString filter = tr("All images");

    filter.append('(');
    filter.append(sList.join(" "));
    filter.append(')');

    static QString cfgGroupName = QStringLiteral("General"), cfgLastOpenPath = QStringLiteral("LastOpenPath");
    QString pictureFolder = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QDir existChecker(pictureFolder);
    if (!existChecker.exists()) {
        pictureFolder = QDir::currentPath();
    }

    pictureFolder = dApp->setter->value(cfgGroupName, cfgLastOpenPath, pictureFolder).toString();

    const QStringList &image_list = QFileDialog::getOpenFileNames(this, tr("Open Image"),
                                                                  pictureFolder, filter, nullptr, QFileDialog::HideNameFilterDetails);

    if (image_list.isEmpty())
        return;

    QFileInfo firstFileInfo(image_list.first());
    dApp->setter->setValue(cfgGroupName, cfgLastOpenPath, firstFileInfo.path());


    DBImgInfoList dbInfos;
    QStringList paths;

    using namespace utils::image;

    for (auto imagePath : image_list)
    {
//        if (! imageSupportRead(imagePath)) {
//            continue;
//        }

        // Generate thumbnail and storage into cache dir
        if (! utils::image::thumbnailExist(imagePath)) {
            // Generate thumbnail failed, do not insert into DB
            if (! utils::image::generateThumbnail(imagePath)) {
                continue;
            }
        }

        QFileInfo fi(imagePath);
        DBImgInfo dbi;
        dbi.fileName = fi.fileName();
        dbi.filePath = imagePath;
        dbi.dirHash = utils::base::hash(QString());
        dbi.time = fi.birthTime();

        dbInfos << dbi;
        paths << imagePath;
    }

    if (! dbInfos.isEmpty())
    {
        emit sigImprotPicsIntoDB(dbInfos);
    }
}
