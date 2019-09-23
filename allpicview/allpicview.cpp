#include "allpicview.h"
#include <QMimeData>

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
        initThumbnailListView();
        
        m_pImportView = new ImportView();
        addWidget(m_pImportView);
        addWidget(m_pThumbnailListView);
        
        updateStackedWidget();
        
        initConnections();
    }
    else
    {
        removeDBAllInfos();
    }
}

void AllPicView::initConnections()
{
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &AllPicView::updatePicsIntoThumbnailView);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &AllPicView::updatePicsIntoThumbnailView);

    connect(m_pThumbnailListView,&ThumbnailListView::openImage,this,[=](int index){
        SignalManager::ViewInfo info;
        info.album = "";
        info.lastPanel = nullptr;
        auto imagelist = DBManager::instance()->getAllInfos();
        if(imagelist.size()>1){
            for(auto image : imagelist)
            {
                info.paths<<image.filePath;
            }
        }else {
          info.paths.clear();
         }
        info.path = imagelist[index].filePath;
//        info.fullScreen = true;
        emit dApp->signalM->viewImage(info);
        emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALLPIC);
    });
    connect(m_pThumbnailListView,&ThumbnailListView::menuOpenImage,this,[=](QString path,QStringList paths,bool isFullScreen, bool isSlideShow){
        SignalManager::ViewInfo info;
        info.album = "";
        info.lastPanel = nullptr;
        auto imagelist = DBManager::instance()->getAllInfos();
        if(paths.size()>1){
            info.paths = paths;
        }else
        {
            if(imagelist.size()>1){
                for(auto image : imagelist)
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
        emit dApp->signalM->viewImage(info);
        emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALLPIC);
    });
    connect(dApp->signalM, &SignalManager::sigPixMapRotate, this, &AllPicView::updatePicsIntoThumbnailView);
}

void AllPicView::initThumbnailListView()
{
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
//        if (! imageSupportRead(imagePath)) {
//            continue;
//        }

        // Generate thumbnail and storage into cache dir
        if (! utils::image::thumbnailExist(path)) {
            // Generate thumbnail failed, do not insert into DB
            if (! utils::image::generateThumbnail(path)) {
                continue;
            }
        }

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
