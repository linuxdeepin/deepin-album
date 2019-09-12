#include "allpicview.h"

namespace {
const QString RECENT_IMPORTED_ALBUM = "Recent imported";
const QString TRASH_ALBUM = "Trash";
const QString FAVORITES_ALBUM = "My favorite";
}  //namespace

AllPicView::AllPicView()
{
//    bool a = 0;
    bool a = 1;
    if ( a )
    {
        initThumbnailListView();
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
    connect(this,&ThumbnailListView::openImage,this,[=](int index){
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
        emit dApp->signalM->showImageView(1);
    });
    connect(this,&ThumbnailListView::menuOpenImage,this,[=](QString path,QStringList paths,bool isFullScreen){
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
        emit dApp->signalM->viewImage(info);
        emit dApp->signalM->showImageView(1);
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

        insertThumbnails(thumbnaiItemList);
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

    insertThumbnails(thumbnaiItemList);
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

    auto albums = DBManager::instance()->getAllAlbumNames();

    for(auto album : albums)
    {
        DBManager::instance()->removeAlbum(album);
    }
}
