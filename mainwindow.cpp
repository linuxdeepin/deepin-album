#include "mainwindow.h"
#include "controller/commandline.h"
#include "dialogs/albumcreatedialog.h"
#include "utils/snifferimageformat.h"
#include <DWidgetUtil>

namespace  {
const int VIEW_ALLPIC = 0;
const int VIEW_TIMELINE = 1;
const int VIEW_ALBUM = 2;
const int VIEW_SEARCH = 3;
const int VIEW_IMAGE = 4;

const QString TITLEBAR_NEWALBUM = "New albume";
const QString TITLEBAR_IMPORT = "Import";

}//namespace


MainWindow::MainWindow()
{
    m_allPicNum = DBManager::instance()->getImgsCount();
    m_iCurrentView = VIEW_ALLPIC;
    m_bTitleMenuImportClicked = false;

    initUI();
    initDB();
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
    connect(this, &MainWindow::sigTitleMenuImportClicked, this, &MainWindow::onImprotBtnClicked);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, [=]{
        if (0 < DBManager::instance()->getImgsCount())
        {
            m_pSearchEdit->setEnabled(true);
        }
        else
        {
            m_pSearchEdit->setEnabled(false);
        }
    });
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, [=]{
        if (0 < DBManager::instance()->getImgsCount())
        {
            m_pSearchEdit->setEnabled(true);
        }
        else
        {
            m_pSearchEdit->setEnabled(false);
        }
    });
	connect(dApp->signalM,&SignalManager::showImageView,this,[=](int index){
        m_backIndex = index;
        titlebar()->setFixedHeight(0);
        statusBar()->setFixedHeight(0);
        m_pCenterWidget->setCurrentIndex(VIEW_IMAGE);
    });
    connect(dApp->signalM,&SignalManager::hideImageView,this,[=](){
        emit dApp->signalM->hideExtensionPanel();

        titlebar()->setFixedHeight(50);
        statusBar()->setFixedHeight(30);
        m_pCenterWidget->setCurrentIndex(m_backIndex);
    });
    connect(dApp->signalM,&SignalManager::exportImage,this,[=](QStringList paths){
        Exporter::instance()->exportImage(paths);
    });
    connect(m_pSlider, &DSlider::valueChanged, dApp->signalM, &SignalManager::sigMainwindowSliderValueChg);
    connect(dApp->signalM, &SignalManager::showImageInfo, this, &MainWindow::onShowImageInfo);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &MainWindow::onUpdateAllpicsNumLabel);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &MainWindow::onUpdateAllpicsNumLabel);
}

void MainWindow::initUI()
{
//    resize(DEFAULT_WINDOWS_WIDTH, DEFAULT_WINDOWS_HEIGHT);
    setMinimumSize(MIX_WINDOWS_WIDTH, MIX_WINDOWS_HEIGHT);
}

void MainWindow::initDB()
{
    QStringList removePaths;

    auto infos = DBManager::instance()->getAllInfos();
    for(auto info : infos)
    {
        QString format = DetectImageFormat(info.filePath);
        if (format.isEmpty())
        {
            removePaths<<info.filePath;
        }
    }

    DBManager::instance()->removeImgInfosNoSignal(removePaths);
}

void MainWindow::initTitleBar()
{
    // TitleBar Img
    QLabel* pLabel = new QLabel();
    pLabel->setFixedSize(33, 32);

//    QPixmap pixmap;
//    pixmap = utils::base::renderSVG(":/resources/images/other/deepin-album.svg", QSize(33, 32));
    QIcon icon = QIcon::fromTheme("deepin-album");

    pLabel->setPixmap(icon.pixmap(QSize(30, 30)));

    // TitleBar Button
    m_titleBtnWidget = new QWidget();

    QHBoxLayout* pTitleBtnLayout = new QHBoxLayout();
    pTitleBtnLayout->setSpacing(5);

    m_pAllPicBtn = new DPushButton();
    m_pTimeLineBtn = new DPushButton();
    m_pAlbumBtn = new DPushButton();

    m_pAllPicBtn->setText("所有照片");
    m_pTimeLineBtn->setText("时间线");
    m_pAlbumBtn->setText("相册");

    pTitleBtnLayout->addWidget(m_pAllPicBtn);
    pTitleBtnLayout->addWidget(m_pTimeLineBtn);
    pTitleBtnLayout->addWidget(m_pAlbumBtn);

    m_titleBtnWidget->setLayout(pTitleBtnLayout);

    // TitleBar Search
    QWidget* m_titleSearchWidget = new QWidget();
    QHBoxLayout* pTitleSearchLayout = new QHBoxLayout();
    m_pSearchEdit = new DSearchEdit();
    m_pSearchEdit->setFixedSize(278, 26);
    if (0 < DBManager::instance()->getImgsCount())
    {
        m_pSearchEdit->setEnabled(true);
    }
    else
    {
        m_pSearchEdit->setEnabled(false);
    }

    pTitleSearchLayout->addWidget(m_pSearchEdit);
    m_titleSearchWidget->setLayout(pTitleSearchLayout);

    // TitleBar Menu
    m_pTitleBarMenu = new DMenu();
    QAction *pNewAlbum = new QAction();
    pNewAlbum->setText(TITLEBAR_NEWALBUM);
    m_pTitleBarMenu->addAction(pNewAlbum);

    QAction *pImport = new QAction();
    pImport->setText(TITLEBAR_IMPORT);
    m_pTitleBarMenu->addAction(pImport);
    m_pTitleBarMenu->addSeparator();

    titlebar()->addWidget(pLabel, Qt::AlignLeft);
    titlebar()->addWidget(m_titleBtnWidget, Qt::AlignLeft);
    titlebar()->addWidget(m_titleSearchWidget, Qt::AlignHCenter);
    titlebar()->setMenu(m_pTitleBarMenu);
}

void MainWindow::initCentralWidget()
{
    m_pCenterWidget = new QStackedWidget;
    m_pAlbumview = new AlbumView();
    m_pAllPicView = new AllPicView();
    m_pTimeLineView = new TimeLineView();
    m_pSearchView = new SearchView();

    m_pCenterWidget->addWidget(m_pAllPicView);
    m_pCenterWidget->addWidget(m_pTimeLineView);
    m_pCenterWidget->addWidget(m_pAlbumview);
    m_pCenterWidget->addWidget(m_pSearchView);
    m_commandLine = CommandLine::instance();
    m_commandLine->processOption();
    m_pCenterWidget->addWidget(m_commandLine);

    setCentralWidget(m_pCenterWidget);
}

void MainWindow::onUpdateCentralWidget()
{
    emit dApp->signalM->hideExtensionPanel();

    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
}

void MainWindow::initStatusBar()
{  
    m_pStatusBar = new DStatusBar(this);
    setStatusBar(m_pStatusBar);

    QString str = tr("%1张照片");

    m_pAllPicNumLabel = new DLabel();
    m_pAllPicNumLabel->setText(str.arg(QString::number(m_allPicNum)));
    m_pAllPicNumLabel->setFont(QFont("SourceHanSansSC-Normal"));
    m_pAllPicNumLabel->setAlignment(Qt::AlignCenter);
    m_pAllPicNumLabel->setFixedHeight(18);

    m_pSlider = new DSlider();
    m_pSlider->setFixedWidth(120);
    m_pSlider->setFixedHeight(24);
    m_pSlider->setMinimum(0);
    m_pSlider->setMaximum(4);
    m_pSlider->slider()->setSingleStep(1);
    m_pSlider->slider()->setTickInterval(1);
    m_pSlider->setValue(2);

//    QWidget *pWidget = new QWidget();
//    pWidget->setFixedWidth(160);
//    pWidget->setFixedHeight(30);

//    QHBoxLayout *pHBoxLayout = new QHBoxLayout();
//    pHBoxLayout->addWidget(m_pSlider, Qt::AlignLeft);

//    pWidget->setLayout(pHBoxLayout);

    statusBar()->addWidget(m_pAllPicNumLabel, 1);
    statusBar()->addWidget(m_pSlider, 1);
    statusBar()->setSizeGripEnabled(false);
    statusBar()->setFixedHeight(30);
}

void MainWindow::allPicBtnClicked()
{
    emit dApp->signalM->hideExtensionPanel();
    m_pSearchEdit->clear();

    m_iCurrentView = VIEW_ALLPIC;

    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
}

void MainWindow::timeLineBtnClicked()
{
    emit dApp->signalM->hideExtensionPanel();
    m_pSearchEdit->clear();

    m_iCurrentView = VIEW_TIMELINE;

    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
}

void MainWindow::albumBtnClicked()
{
    emit dApp->signalM->hideExtensionPanel();
    m_pSearchEdit->clear();

    m_iCurrentView = VIEW_ALBUM;

    m_pCenterWidget->setCurrentIndex(m_iCurrentView);
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
        m_bTitleMenuImportClicked = true;
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
        emit dApp->signalM->hideExtensionPanel();
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
    emit dApp->signalM->hideExtensionPanel();
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

void MainWindow::onUpdateAllpicsNumLabel()
{
    QString str = tr("%1张照片");

    m_allPicNum = DBManager::instance()->getImgsCount();
    m_pAllPicNumLabel->setText(str.arg(QString::number(m_allPicNum)));
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

    using namespace utils::image;

    for (auto imagePath : image_list)
    {
        if (! imageSupportRead(imagePath)) {
            continue;
        }

//        // Generate thumbnail and storage into cache dir
//        if (! utils::image::thumbnailExist(imagePath)) {
//            // Generate thumbnail failed, do not insert into DB
//            if (! utils::image::generateThumbnail(imagePath)) {
//                continue;
//            }
//        }

        QFileInfo fi(imagePath);
        DBImgInfo dbi;
        dbi.fileName = fi.fileName();
        dbi.filePath = imagePath;
        dbi.dirHash = utils::base::hash(QString());
        dbi.time = fi.birthTime();

        dbInfos << dbi;
    }

    if (! dbInfos.isEmpty())
    {
        DBManager::instance()->insertImgInfos(dbInfos);

        if (true == m_bTitleMenuImportClicked && VIEW_ALBUM == m_iCurrentView)
        {
            m_pAlbumview->picsIntoAlbum(image_list);
            m_bTitleMenuImportClicked = false;
        }
    }
}

void MainWindow::onShowImageInfo(const QString &path)
{
    ImgInfoDialog *dialog;
    if (m_propertyDialogs.contains(path)) {
        dialog = m_propertyDialogs.value(path);
        dialog->raise();
    } else {
        dialog = new ImgInfoDialog(path);
        m_propertyDialogs.insert(path, dialog);
        dialog->move((width() - dialog->width()) / 2 +
                   mapToGlobal(QPoint(0, 0)).x(),
                   (window()->height() - dialog->sizeHint().height()) / 2 +
                   mapToGlobal(QPoint(0, 0)).y());
        dialog->show();
        dialog->setWindowState(Qt::WindowActive);
        connect(dialog, &ImgInfoDialog::closed, this, [=] {
            dialog->deleteLater();
            m_propertyDialogs.remove(path);
        });
    }

}
