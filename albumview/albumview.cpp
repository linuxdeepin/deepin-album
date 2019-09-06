#include "albumview.h"
#include "utils/baseutils.h"

namespace {
const int ITEM_SPACING = 5;
const int LEFT_VIEW_WIDTH = 250;
const QString RECENT_IMPORTED_ALBUM = "Recent imported";
const QString TRASH_ALBUM = "Trash";
const QString FAVORITES_ALBUM = "My favorite";
const QString LEFT_MENU_SLIDESHOW = "Slide show";
const QString LEFT_MENU_NEWALBUM = "New album";
const QString LEFT_MENU_RENAMEALBUM = "Rename album";
const QString LEFT_MENU_EXPORT = "Export";
const QString LEFT_MENU_DELALBUM = "Delete album";
}  //namespace

AlbumView::AlbumView()
{
    m_currentAlbum = RECENT_IMPORTED_ALBUM;

    m_allAlbumNames<<RECENT_IMPORTED_ALBUM;
    m_allAlbumNames<<TRASH_ALBUM;
    m_allAlbumNames<<FAVORITES_ALBUM;

    initLeftView();
    initRightView();

    addWidget(m_pLeftTabList);
    addWidget(m_pRightThumbnailList);

    setChildrenCollapsible(false);

    initConnections();
}

void AlbumView::initConnections()
{
    connect(m_pLeftTabList, &DListWidget::clicked, this, &AlbumView::leftTabClicked);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &AlbumView::updateRightView);
    connect(m_pLeftTabList, &QListView::customContextMenuRequested, this, &AlbumView::showLeftMenu);
    connect(m_pLeftMenu, &DMenu::triggered, this, &AlbumView::onLeftMenuClicked);
}

void AlbumView::initLeftView()
{
    m_pLeftTabList = new DListWidget();
    m_pLeftTabList->setFixedWidth(LEFT_VIEW_WIDTH);
    m_pLeftTabList->setSpacing(ITEM_SPACING);
    m_pLeftTabList->setContextMenuPolicy(Qt::CustomContextMenu);

    m_pLeftMenu = new DMenu();

    updateLeftView();
}

void AlbumView::updateLeftView()
{
    m_pLeftTabList->clear();
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
        m_pLeftTabList->addItem(pListWidgetItem);
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

void AlbumView::leftTabClicked(const QModelIndex &index)
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

void AlbumView::showLeftMenu(const QPoint &pos)
{
    if (!m_pLeftTabList->indexAt(pos).isValid())
    {
        return;
    }

    int num = m_pLeftTabList->indexAt(pos).row();

    m_allAlbumNames<<RECENT_IMPORTED_ALBUM;
    m_allAlbumNames<<TRASH_ALBUM;
    m_allAlbumNames<<FAVORITES_ALBUM;
    if (RECENT_IMPORTED_ALBUM == m_allAlbumNames[num] ||
        TRASH_ALBUM == m_allAlbumNames[num] ||
        FAVORITES_ALBUM == m_allAlbumNames[num])
    {
        return;
    }

    m_pLeftMenu->clear();

    appendAction(LEFT_MENU_SLIDESHOW);
    m_pLeftMenu->addSeparator();
    appendAction(LEFT_MENU_NEWALBUM);
    m_pLeftMenu->addSeparator();
    appendAction(LEFT_MENU_RENAMEALBUM);
    m_pLeftMenu->addSeparator();
    appendAction(LEFT_MENU_EXPORT);
    m_pLeftMenu->addSeparator();
    appendAction(LEFT_MENU_DELALBUM);

    m_pLeftMenu->popup(QCursor::pos());
}

void AlbumView::appendAction(const QString &text)
{
    QAction *ac = new QAction(m_pLeftMenu);
    addAction(ac);
    ac->setText(text);
    m_pLeftMenu->addAction(ac);
}

void AlbumView::onLeftMenuClicked(QAction *action)
{
    QString str = action->text();

    if (LEFT_MENU_SLIDESHOW == str)
    {

    }
    else if (LEFT_MENU_NEWALBUM == str)
    {

    }
    else if (LEFT_MENU_RENAMEALBUM == str)
    {

    }
    else if (LEFT_MENU_EXPORT == str)
    {

    }
    else if (LEFT_MENU_DELALBUM == str)
    {

    }
    else
    {
        // donothing
    }
}
