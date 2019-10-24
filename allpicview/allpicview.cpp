#include "allpicview.h"
#include <QMimeData>
#include "utils/snifferimageformat.h"

namespace {
const QString RECENT_IMPORTED_ALBUM = "Recent imported";
const QString TRASH_ALBUM = "Trash";
const QString FAVORITES_ALBUM = "My favorite";
const int VIEW_IMPORT = 0;
const int VIEW_ALLPICS = 1;
const int VIEW_MAINWINDOW_ALLPIC = 0;
}  //namespace

AllPicView::AllPicView()
{
    setAcceptDrops(true);
//    bool a = 0;
    bool a = 1;
    if ( a )
    {
        m_pThumbnailListView = new ThumbnailListView();
//        initThumbnailListView();
        
        m_pImportView = new ImportView();
        addWidget(m_pImportView);
        addWidget(m_pThumbnailListView);
        
        m_pSearchView = new SearchView();
        addWidget(m_pSearchView);


        updateStackedWidget();
        
        initConnections();
    }
    else
    {
        removeDBAllInfos();
    }

    m_spinner = new DSpinner(this);
    m_spinner->setFixedSize(40, 40);
    m_spinner->hide();

    if (0 < DBManager::instance()->getImgsCount())
    {
        m_spinner->show();
        m_spinner->start();
    }
}

void AllPicView::initConnections()
{
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &AllPicView::updatePicsIntoThumbnailView);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &AllPicView::updatePicsIntoThumbnailView);
    connect(dApp, &Application::sigFinishLoad, this, &AllPicView::updatePicsIntoThumbnailView);

    connect(m_pThumbnailListView,&ThumbnailListView::openImage,this,[=](int index){
        SignalManager::ViewInfo info;
        info.album = "";
        info.lastPanel = nullptr;
        auto imagelist = DBManager::instance()->getAllInfos();
        if(imagelist.size()>1)
        {
            for(auto image : imagelist)
            {
                info.paths<<image.filePath;
            }
        }
        else
        {
          info.paths.clear();
        }
        info.path = imagelist[index].filePath;
        info.viewType = utils::common::VIEW_ALLPIC_SRN;
        emit dApp->signalM->viewImage(info);
        emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALLPIC);
    });
    connect(m_pThumbnailListView,&ThumbnailListView::menuOpenImage,this,[=](QString path,QStringList paths,bool isFullScreen, bool isSlideShow){
        SignalManager::ViewInfo info;
        info.album = "";
        info.lastPanel = nullptr;
        auto imagelist = DBManager::instance()->getAllInfos();
        if(paths.size()>1)
        {
            info.paths = paths;
        }
        else
        {
            if(imagelist.size()>1)
            {
                for(auto image : imagelist)
                {
                    info.paths<<image.filePath;
                }
            }
            else
            {
              info.paths.clear();
            }
        }
        info.path = path;
        info.fullScreen = isFullScreen;
        info.slideShow = isSlideShow;
        info.viewType = utils::common::VIEW_ALLPIC_SRN;

        emit dApp->signalM->viewImage(info);
        emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALLPIC);
    });
    connect(dApp->signalM, &SignalManager::sigPixMapRotate, this, &AllPicView::updatePicsIntoThumbnailView);
}

//void AllPicView::initThumbnailListView()
//{
//    if (0 < DBManager::instance()->getImgsCount())
//    {
//        QList<ThumbnailListView::ItemInfo> thumbnaiItemList;

//        auto infos = DBManager::instance()->getAllInfos();
//        for(auto info : infos)
//        {
//            ThumbnailListView::ItemInfo vi;
//            vi.name = info.fileName;
//            QImage tImg;

//            QString format = DetectImageFormat(info.filePath);
//            if (format.isEmpty()) {
//                QImageReader reader(info.filePath);
//                reader.setAutoTransform(true);
//                if (reader.canRead()) {
//                    tImg = reader.read();
//                }
//            } else {
//                QImageReader readerF(info.filePath, format.toLatin1());
//                readerF.setAutoTransform(true);
//                if (readerF.canRead()) {
//                    tImg = readerF.read();
//                } else {
//                    qWarning() << "can't read image:" << readerF.errorString()
//                               << format;

//                    tImg = QImage(info.filePath);
//                }
//            }

//            vi.image = QPixmap::fromImage(tImg);

//            thumbnaiItemList<<vi;
//        }

//        m_pThumbnailListView->insertThumbnails(thumbnaiItemList);
//    }
//}

void AllPicView::updateStackedWidget()
{
    if (0 < DBManager::instance()->getImgsCount())
    {
        setCurrentIndex(VIEW_ALLPICS);
    }
    else
    {
        setCurrentIndex(VIEW_IMPORT);
    }
}

void AllPicView::updatePicsIntoThumbnailView()
{
    m_spinner->hide();
    QList<ThumbnailListView::ItemInfo> thumbnaiItemList;

    auto infos = DBManager::instance()->getAllInfos();
    for(auto info : infos)
    {
        ThumbnailListView::ItemInfo vi;
        vi.name = info.fileName;
        vi.path = info.filePath;
        vi.image = dApp->m_imagemap.value(info.filePath);

        thumbnaiItemList<<vi;
    }

    m_pThumbnailListView->insertThumbnails(thumbnaiItemList);

    updateStackedWidget();
}

void AllPicView::dragEnterEvent(QDragEnterEvent *e)
{
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void AllPicView::dropEvent(QDropEvent *event)
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

void AllPicView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void AllPicView::dragLeaveEvent(QDragLeaveEvent *e)
{

}

void AllPicView::resizeEvent(QResizeEvent *e)
{
    m_spinner->move(width()/2 - 20, (height()-50)/2 - 20);
}

void AllPicView::removeDBAllInfos()
{
    auto infos = DBManager::instance()->getAllInfos();

    QStringList paths;

    for(auto info : infos)
    {
        paths<<info.filePath;
    }


    DBManager::instance()->removeImgInfos(paths);

    //
    auto albums = DBManager::instance()->getAllAlbumNames();

    for(auto album : albums)
    {
        DBManager::instance()->removeAlbum(album);
    }

    //
    auto infos1 = DBManager::instance()->getAllTrashInfos();

    QStringList paths1;

    for(auto info : infos1)
    {
        paths1<<info.filePath;
    }


    DBManager::instance()->removeTrashImgInfos(paths1);

}
