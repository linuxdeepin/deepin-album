#include "searchview.h"

namespace {
const QString RECENT_IMPORTED_ALBUM = "Recent imported";
const QString TRASH_ALBUM = "Trash";
const QString FAVORITES_ALBUM = "My favorite";
}  //namespace

SearchView::SearchView()
{
//    bool a = 0;
    bool a = 1;
    if ( a )
    {
        initImportFrame();
        initThumbnailListView();
        initMainStackWidget();
        initConnections();
    }
    else
    {
        removeDBAllInfos();
    }
}

void SearchView::initConnections()
{
    connect(m_pImportBtn, &DPushButton::clicked, this, &SearchView::improtBtnClicked);
    connect(this, &SearchView::sigImprotPicsIntoDB, DBManager::instance(), &DBManager::insertImgInfos);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &SearchView::improtPicsIntoThumbnailView);
    connect(dApp->signalM, &SignalManager::sigSendKeywordsIntoALLPic, this, &SearchView::improtSearchResultsIntoThumbnailView);
}

void SearchView::initImportFrame()
{
    m_pImportFrame = new DWidget();

    QVBoxLayout* pImportFrameLayout = new QVBoxLayout();

    DLabel* pLabel = new DLabel();
    pLabel->setFixedSize(128, 128);

    QPixmap pixmap;
    pixmap = utils::base::renderSVG(":/resources/images/logo/deepin-image-viewer.svg", QSize(128, 128));

    pLabel->setPixmap(pixmap);

    m_pImportBtn = new DPushButton();
    m_pImportBtn->setText("导入图片");
    m_pImportBtn->setFixedSize(142, 42);

    DLabel* pLabel2 = new DLabel();
    pLabel2->setFixedHeight(24);
    pLabel2->setText("您也可以拖拽或导入图片到时间线");

    pImportFrameLayout->addStretch();
    pImportFrameLayout->addWidget(pLabel, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(20);
    pImportFrameLayout->addWidget(m_pImportBtn, 0, Qt::AlignCenter);
    pImportFrameLayout->addSpacing(10);
    pImportFrameLayout->addWidget(pLabel2, 0, Qt::AlignCenter);
    pImportFrameLayout->addStretch();

    m_pImportFrame->setLayout(pImportFrameLayout);
}

void SearchView::initThumbnailListView()
{
    m_pThumbnailListView = new ThumbnailListView();

    if (0 < DBManager::instance()->getImgsCount())
    {
        QList<ThumbnailListView::ItemInfo> thumbnaiItemList;

        auto infos = DBManager::instance()->getAllInfos();
        for(auto info : infos)
        {
            ThumbnailListView::ItemInfo vi;
            vi.name = info.fileName;
            vi.path = info.filePath;

            thumbnaiItemList<<vi;
        }

        m_pThumbnailListView->insertThumbnails(thumbnaiItemList);
    }
}

void SearchView::initMainStackWidget()
{
    m_stackWidget = new DStackedWidget();
    m_stackWidget->setContentsMargins(0, 0, 0, 0);
    m_stackWidget->addWidget(m_pImportFrame);
    m_stackWidget->addWidget(m_pThumbnailListView);
    //show import frame if no images in database
    m_stackWidget->setCurrentIndex(DBManager::instance()->getImgsCount() > 0? 1 : 0);

    QLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stackWidget);
}

void SearchView::updateMainStackWidget()
{
    if (DBManager::instance()->getImgsCount() < 1)
    {
        m_stackWidget->setCurrentIndex(0);
    }
    else
    {
        m_stackWidget->setCurrentIndex(1);
    }
}

void SearchView::improtBtnClicked()
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

void SearchView::improtPicsIntoThumbnailView()
{
    QList<ThumbnailListView::ItemInfo> thumbnaiItemList;

    auto infos = DBManager::instance()->getAllInfos();
    for(auto info : infos)
    {
        ThumbnailListView::ItemInfo vi;
        vi.name = info.fileName;
        vi.path = info.filePath;

        thumbnaiItemList<<vi;
    }

    m_pThumbnailListView->insertThumbnails(thumbnaiItemList);

    updateMainStackWidget();
}

void SearchView::improtSearchResultsIntoThumbnailView(QString s)
{
    QList<ThumbnailListView::ItemInfo> thumbnaiItemList;
    auto infos = DBManager::instance()->getInfosForKeyword(s);

    for(auto info : infos)
    {
        ThumbnailListView::ItemInfo vi;
        vi.name = info.fileName;
        vi.path = info.filePath;
        thumbnaiItemList<<vi;
    }

    m_pThumbnailListView->insertThumbnails(thumbnaiItemList);

    updateMainStackWidget();
}

void SearchView::removeDBAllInfos()
{
    auto infos = DBManager::instance()->getAllInfos();

    QStringList paths;

    for(auto info : infos)
    {
        paths<<info.filePath;
    }


    DBManager::instance()->removeImgInfos(paths);

    auto albums = DBManager::instance()->getAllAlbumNames();

    for(auto album : albums)
    {
        DBManager::instance()->removeAlbum(album);
    }
}
