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
const int OPE_MODE_ADDNEWALBUM = 0;
const int OPE_MODE_RENAMEALBUM = 1;
}  //namespace

AlbumView::AlbumView()
{
    m_currentAlbum = RECENT_IMPORTED_ALBUM;
    m_iAlubmPicsNum = DBManager::instance()->getImgsCount();

    initLeftView();
    initRightView();

    addWidget(m_pLeftTabList);
    addWidget(m_pRightStackWidget);

    setChildrenCollapsible(false);

    initConnections();
}

void AlbumView::initConnections()
{
    connect(m_pLeftTabList, &DListWidget::clicked, this, &AlbumView::leftTabClicked);
    connect(dApp->signalM, &SignalManager::sigCreateNewAlbumFromDialog, this, &AlbumView::updateLeftView);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &AlbumView::updateRightNoTrashView);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::imagesTrashRemoved, this, &AlbumView::updateRightView);
    connect(m_pLeftTabList, &QListView::customContextMenuRequested, this, &AlbumView::showLeftMenu);
    connect(m_pLeftMenu, &DMenu::triggered, this, &AlbumView::onLeftMenuClicked);
    connect(m_pRecoveryBtn, &DPushButton::clicked, this, &AlbumView::onTrashRecoveryBtnClicked);
    connect(m_pDeleteBtn, &DPushButton::clicked, this, &AlbumView::onTrashDeleteBtnClicked);

    connect(m_pRightThumbnailList,&ThumbnailListView::openImage,this,&AlbumView::openImage);
    connect(m_pRightTrashThumbnailList,&ThumbnailListView::openImage,this,&AlbumView::openImage);
    connect(m_pRightFavoriteThumbnailList,&ThumbnailListView::openImage,this,&AlbumView::openImage);

    connect(m_pRightThumbnailList,&ThumbnailListView::menuOpenImage,this,&AlbumView::menuOpenImage);
    connect(m_pRightTrashThumbnailList,&ThumbnailListView::menuOpenImage,this,&AlbumView::menuOpenImage);
    connect(m_pRightFavoriteThumbnailList,&ThumbnailListView::menuOpenImage,this,&AlbumView::menuOpenImage);
}

void AlbumView::initLeftView()
{
    m_pLeftTabList = new DListWidget();
    m_pLeftTabList->setFixedWidth(LEFT_VIEW_WIDTH);
    m_pLeftTabList->setSpacing(ITEM_SPACING);
    m_pLeftTabList->setContextMenuPolicy(Qt::CustomContextMenu);

    m_pLeftMenu = new DMenu();

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
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftTabList);
        pListWidgetItem->setSizeHint(QSize(200, 36));
        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName);
        if (RECENT_IMPORTED_ALBUM == albumName)
        {
            pListWidgetItem->setSelected(true);
        }
        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    }
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

    for(int i = 0; i < m_allAlbumNames.length(); i++)
    {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftTabList);
        pListWidgetItem->setSizeHint(QSize(200, 36));
        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(m_allAlbumNames[i]);
        if ((m_allAlbumNames.length() - 1) == i)
        {
            m_currentAlbum = m_allAlbumNames[i];
            pListWidgetItem->setSelected(true);
        }
        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    }

    updateRightNoTrashView();
}

void AlbumView::initRightView()
{
    m_pRightStackWidget = new DStackedWidget();
    QSizePolicy sp = m_pRightStackWidget->sizePolicy();
    sp.setHorizontalStretch(1);
    m_pRightStackWidget->setSizePolicy(sp);

    m_pRightThumbnailList = new ThumbnailListView();

    DWidget *pTrashWidget = new DWidget();
    QVBoxLayout *pMainVBoxLayout = new QVBoxLayout();
    QHBoxLayout *pTopHBoxLayout = new QHBoxLayout();

    QVBoxLayout *pTopLeftVBoxLayout = new QVBoxLayout();
    DLabel *pLabel1 = new DLabel();
    pLabel1->setText("最近删除");

    DLabel *pLabel2 = new DLabel();
    pLabel2->setText("照片在删除前会显示剩余天数，之后将永久删除");

    pTopLeftVBoxLayout->addWidget(pLabel1);
    pTopLeftVBoxLayout->addWidget(pLabel2);

    QHBoxLayout *pTopRightVBoxLayout = new QHBoxLayout();
    m_pRecoveryBtn = new DPushButton();
    m_pRecoveryBtn->setText("恢复");

    m_pDeleteBtn = new DPushButton();
    m_pDeleteBtn->setText("删除");

    pTopRightVBoxLayout->addWidget(m_pRecoveryBtn);
    pTopRightVBoxLayout->addWidget(m_pDeleteBtn);

    pTopHBoxLayout->addItem(pTopLeftVBoxLayout);
    pTopHBoxLayout->addItem(pTopRightVBoxLayout);

    m_pRightTrashThumbnailList = new ThumbnailListView(TRASH_ALBUM);

    pMainVBoxLayout->addItem(pTopHBoxLayout);
    pMainVBoxLayout->addWidget(m_pRightTrashThumbnailList);

    pTrashWidget->setLayout(pMainVBoxLayout);

    m_pRightFavoriteThumbnailList = new ThumbnailListView(FAVORITES_ALBUM);

    m_pRightStackWidget->addWidget(m_pRightThumbnailList);
    m_pRightStackWidget->addWidget(pTrashWidget);
    m_pRightStackWidget->addWidget(m_pRightFavoriteThumbnailList);

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
    if (TRASH_ALBUM == m_currentAlbum)
    {
        updateRightTrashView();
    }
    else
    {
        updateRightNoTrashView();
    }
}

void AlbumView::updateRightNoTrashView()
{
    QList<ThumbnailListView::ItemInfo> thumbnaiItemList;

    DBImgInfoList infos;

    if (RECENT_IMPORTED_ALBUM == m_currentAlbum)
    {
        infos = DBManager::instance()->getAllInfos();

        m_iAlubmPicsNum = DBManager::instance()->getImgsCount();
        emit dApp->signalM->sigUpdateAllpicsNumLabel(m_iAlubmPicsNum);
    }
    else
    {
        infos = DBManager::instance()->getInfosByAlbum(m_currentAlbum);

        m_iAlubmPicsNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);
        emit dApp->signalM->sigUpdateAllpicsNumLabel(m_iAlubmPicsNum);
    }

    for(auto info : infos)
    {
        ThumbnailListView::ItemInfo vi;
        vi.name = info.fileName;
        vi.path = info.filePath;

        thumbnaiItemList<<vi;
    }

    if (FAVORITES_ALBUM == m_currentAlbum)
    {
        m_pRightFavoriteThumbnailList->insertThumbnails(thumbnaiItemList);
        m_pRightStackWidget->setCurrentIndex(2);
    }
    else
    {
        m_pRightThumbnailList->insertThumbnails(thumbnaiItemList);
        m_pRightStackWidget->setCurrentIndex(0);
    }
}

void AlbumView::updateRightTrashView()
{
    QList<ThumbnailListView::ItemInfo> thumbnaiItemList;

    DBImgInfoList infos;

    infos = DBManager::instance()->getAllTrashInfos();

    for(auto info : infos)
    {
        ThumbnailListView::ItemInfo vi;
        vi.name = info.fileName;
        vi.path = info.filePath;

        thumbnaiItemList<<vi;
    }

    m_pRightTrashThumbnailList->insertThumbnails(thumbnaiItemList);
}

void AlbumView::leftTabClicked(const QModelIndex &index)
{
    AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());

    if (m_currentAlbum == item->m_albumNameStr)
    {
        // donothing
    }
    else if (TRASH_ALBUM == item->m_albumNameStr)
    {
        m_currentAlbum = item->m_albumNameStr;
        updateRightTrashView();
        m_pRightStackWidget->setCurrentIndex(1);

        m_iAlubmPicsNum = DBManager::instance()->getTrashImgsCount();
        emit dApp->signalM->sigUpdateAllpicsNumLabel(m_iAlubmPicsNum);
    }
    else
    {
        m_currentAlbum = item->m_albumNameStr;
        updateRightNoTrashView();
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

    m_iTabListRMouseBtnNum = num;

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
        QListWidgetItem *pListWidgetItem = new QListWidgetItem();
        pListWidgetItem->setSizeHint(QSize(200, 36));
        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem("新建相册");
        pAlbumLeftTabItem->m_opeMode = OPE_MODE_ADDNEWALBUM;
        pAlbumLeftTabItem->editAlbumEdit();

        m_pLeftTabList->insertItem(m_pLeftTabList->currentRow()+1, pListWidgetItem);
        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    }
    else if (LEFT_MENU_RENAMEALBUM == str)
    {
        AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
        item->m_opeMode = OPE_MODE_RENAMEALBUM;
        item->editAlbumEdit();
    }
    else if (LEFT_MENU_EXPORT == str)
    {

    }
    else if (LEFT_MENU_DELALBUM == str)
    {
        QListWidgetItem *item = m_pLeftTabList->currentItem();
        AlbumLeftTabItem *pTabItem = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(item);

        DBManager::instance()->removeAlbum(pTabItem->m_albumNameStr);
        delete  item;

    }
    else
    {
        // donothing
    }
}

void AlbumView::createNewAlbum()
{
    QListWidgetItem *pListWidgetItem = new QListWidgetItem();
    pListWidgetItem->setSizeHint(QSize(200, 36));
    AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem("新建相册");
    pAlbumLeftTabItem->m_opeMode = OPE_MODE_ADDNEWALBUM;
    pAlbumLeftTabItem->editAlbumEdit();

    m_pLeftTabList->insertItem(m_pLeftTabList->count()+1, pListWidgetItem);
    m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
}

void AlbumView::onTrashRecoveryBtnClicked()
{
    QStringList paths;
    paths = m_pRightTrashThumbnailList->selectedPaths();

    DBImgInfoList infos;
    for(auto path : paths)
    {
        DBImgInfo info;
        info = DBManager::instance()->getTrashInfoByPath(path);
        infos<<info;
    }

    DBManager::instance()->insertImgInfos(infos);
    DBManager::instance()->removeTrashImgInfos(paths);
}

void AlbumView::onTrashDeleteBtnClicked()
{
    QStringList paths;
    paths = m_pRightTrashThumbnailList->selectedPaths();

    DBManager::instance()->removeTrashImgInfos(paths);
}

void AlbumView::openImage(int index)
{
    SignalManager::ViewInfo info;
    info.album = "";
    info.lastPanel = nullptr;
    auto imagelist = DBManager::instance()->getAllInfos();
    if (TRASH_ALBUM == m_currentAlbum)
    {
        imagelist = DBManager::instance()->getAllTrashInfos();
    }
    else if(FAVORITES_ALBUM == m_currentAlbum)
    {
        imagelist = DBManager::instance()->getInfosByAlbum(m_currentAlbum);
    }

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
    emit dApp->signalM->showImageView(3);
}

void AlbumView::menuOpenImage(QString path,QStringList paths,bool isFullScreen)
{
    SignalManager::ViewInfo info;
    info.album = "";
    info.lastPanel = nullptr;
    auto imagelist = DBManager::instance()->getAllInfos();
    if (TRASH_ALBUM == m_currentAlbum)
    {
        imagelist = DBManager::instance()->getAllTrashInfos();
    }
    else if(FAVORITES_ALBUM == m_currentAlbum)
    {
        imagelist = DBManager::instance()->getInfosByAlbum(m_currentAlbum);
    }

    for(auto image : imagelist)
    {
        info.paths<<image.filePath;
    }
    if(paths.size()>1){
        info.paths = paths;
    }else {
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
    emit dApp->signalM->showImageView(3);
}
