#include "albumview.h"
#include "utils/baseutils.h"

namespace {
const int ITEM_SPACING = 5;
const int LEFT_VIEW_WIDTH = 250;
const QString RECENT_IMPORTED_ALBUM = "Recent imported";
const QString TRASH_ALBUM = "Trash";
const QString FAVORITES_ALBUM = "My favorite";

}  //namespace

AlbumView::AlbumView()
{
    m_currentAlbum = RECENT_IMPORTED_ALBUM;

    m_allAlbumNames<<RECENT_IMPORTED_ALBUM;
    m_allAlbumNames<<TRASH_ALBUM;
    m_allAlbumNames<<FAVORITES_ALBUM;

    initLeftView();
    initRightView();

    addWidget(m_pLeftMenuList);
    addWidget(m_pRightThumbnailList);

    setChildrenCollapsible(false);

    initConnections();
}

void AlbumView::initConnections()
{
    connect(m_pLeftMenuList, &DListWidget::clicked, this, &AlbumView::leftMenuClicked);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &AlbumView::updateRightView);
}

void AlbumView::initLeftView()
{
    m_pLeftMenuList = new DListWidget();
    m_pLeftMenuList->setFixedWidth(LEFT_VIEW_WIDTH);
    m_pLeftMenuList->setSpacing(ITEM_SPACING);
    updateLeftView();
}

void AlbumView::updateLeftView()
{
    m_pLeftMenuList->clear();
    m_allAlbumNames.clear();

    m_allAlbumNames<<RECENT_IMPORTED_ALBUM;
    m_allAlbumNames<<TRASH_ALBUM;
    m_allAlbumNames<<FAVORITES_ALBUM;

    QStringList allAlbumNames = DBManager::instance()->getAllAlbumNames();
    for(auto albumName : allAlbumNames)
    {
        if (TRASH_ALBUM == albumName || FAVORITES_ALBUM == albumName)
        {
            continue;
        }

        m_allAlbumNames<<albumName;
    }

    for(auto albumName : m_allAlbumNames)
    {
        QListWidgetItem* pListWidgetItem = new QListWidgetItem();
        pListWidgetItem->setText(albumName);
        m_pLeftMenuList->addItem(pListWidgetItem);
    }
}

void AlbumView::initRightView()
{
    m_pRightThumbnailList = new ThumbnailListView();
    QSizePolicy sp = m_pRightThumbnailList->sizePolicy();
    sp.setHorizontalStretch(1);
    m_pRightThumbnailList->setSizePolicy(sp);

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

        m_pRightThumbnailList->insertThumbnails(thumbnaiItemList);
    }
}

void AlbumView::updateRightView()
{
    QList<ThumbnailListView::ItemInfo> thumbnaiItemList;

    DBImgInfoList infos;

    if (RECENT_IMPORTED_ALBUM == m_currentAlbum)
    {
        infos = DBManager::instance()->getAllInfos();
    }
    else
    {
        infos = DBManager::instance()->getInfosByAlbum(m_currentAlbum);
    }

    for(auto info : infos)
    {
        ThumbnailListView::ItemInfo vi;
        vi.name = info.fileName;
        vi.path = info.filePath;

        thumbnaiItemList<<vi;
    }

    m_pRightThumbnailList->insertThumbnails(thumbnaiItemList);
}

void AlbumView::leftMenuClicked(const QModelIndex &index)
{
    int num = index.row();
    if (m_currentAlbum == m_allAlbumNames[num])
    {
        // donothing
    }
    else
    {
        m_currentAlbum = m_allAlbumNames[num];
        updateRightView();
    }
}
