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
    initUI();

    initLeftView();
    initRightView();

    addWidget(m_pLeftMenuList);
    addWidget(m_pRightThumbnailList);

    setChildrenCollapsible(false);
}

void AlbumView::initUI()
{
//    QString album3 = RECENT_IMPORTED_ALBUM;
//    QStringList paths3;
//    paths3.clear();

//    DBManager::instance()->insertIntoAlbum(album3, paths3);

//    QString album2 = TRASH_ALBUM;
//    QStringList paths2;
//    paths2.clear();

//    DBManager::instance()->insertIntoAlbum(album2, paths2);

//    QString album = FAVORITES_ALBUM;
//    QStringList paths;
//    paths.clear();

//    DBManager::instance()->insertIntoAlbum(album, paths);
}

void AlbumView::initLeftView()
{
    m_pLeftMenuList = new DListWidget();
    m_pLeftMenuList->setFixedWidth(LEFT_VIEW_WIDTH);
    m_pLeftMenuList->setSpacing(ITEM_SPACING);

    QStringList allAlbumNames;
    allAlbumNames = DBManager::instance()->getAllAlbumNames();

    QListWidgetItem* pListWidgetItem1 = new QListWidgetItem();
    pListWidgetItem1->setText(RECENT_IMPORTED_ALBUM);
    m_pLeftMenuList->addItem(pListWidgetItem1);

    QListWidgetItem* pListWidgetItem2 = new QListWidgetItem();
    pListWidgetItem2->setText(TRASH_ALBUM);
    m_pLeftMenuList->addItem(pListWidgetItem2);

    QListWidgetItem* pListWidgetItem3 = new QListWidgetItem();
    pListWidgetItem3->setText(FAVORITES_ALBUM);
    m_pLeftMenuList->addItem(pListWidgetItem3);

    for(auto albumName : allAlbumNames)
    {
        if (RECENT_IMPORTED_ALBUM == albumName || TRASH_ALBUM == albumName || FAVORITES_ALBUM == albumName)
        {
            continue;
        }

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

    updateRightView();
}

void AlbumView::updateRightView()
{
    QList<ThumbnailListView::ItemInfo> thumbnaiItemList;

    auto infos = DBManager::instance()->getInfosByAlbum(m_currentAlbum);
    for(auto info : infos)
    {
        ThumbnailListView::ItemInfo vi;
        vi.name = info.fileName;
        vi.path = info.filePath;

        thumbnaiItemList<<vi;
    }

    m_pRightThumbnailList->insertThumbnails(thumbnaiItemList);
}
