#include "albumview.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "controller/exporter.h"
#include "dtkcore_global.h"

#include <DNotifySender>
#include <QMimeData>

#include "dgiovolumemanager.h"

namespace {
const int ITEM_SPACING = 5;
const int LEFT_VIEW_WIDTH = 250;
const int LEFT_VIEW_LISTITEM_WIDTH = 200;
const int LEFT_VIEW_LISTITEM_HEIGHT = 36;
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
const QString BUTTON_STR_RECOVERY = "恢复";
const QString BUTTON_STR_DETELE = "删除";
const QString BUTTON_STR_DETELEALL = "全部删除";
const int RIGHT_VIEW_IMPORT = 0;
const int RIGHT_VIEW_THUMBNAIL_LIST = 1;
const int RIGHT_VIEW_TRASH_LIST = 2;
const int RIGHT_VIEW_FAVORITE_LIST = 3;
const int VIEW_MAINWINDOW_ALBUM = 2;
}  //namespace

AlbumView::AlbumView()
{
    m_currentAlbum = RECENT_IMPORTED_ALBUM;
    m_iAlubmPicsNum = DBManager::instance()->getImgsCount();

    setAcceptDrops(true);
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
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::insertedIntoAlbum, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::removedFromAlbum, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::imagesTrashInserted, this, &AlbumView::updateRightView);
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

    connect(m_pRightTrashThumbnailList, &ThumbnailListView::clicked, this, &AlbumView::onTrashListClicked); 
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
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
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
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
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

    // Import View
    m_pImportView = new ImportView();

    // Thumbnail View
    DWidget *pNoTrashWidget = new DWidget();
    QVBoxLayout *pNoTrashVBoxLayout = new QVBoxLayout();

    m_pRightTitle = new DLabel();
    m_pRightTitle->setText(RECENT_IMPORTED_ALBUM);

    QFont ft;
    ft.setPixelSize(24);
    m_pRightTitle->setFont(ft);

    m_pRightPicTotal = new DLabel();

    QString str = tr("%1张照片");
    m_pRightPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));
    QFont ft1;
    ft1.setPixelSize(14);
    m_pRightPicTotal->setFont(ft1);


    m_pRightThumbnailList = new ThumbnailListView(RECENT_IMPORTED_ALBUM);

    pNoTrashVBoxLayout->addWidget(m_pRightTitle);
    pNoTrashVBoxLayout->addWidget(m_pRightPicTotal);
    pNoTrashVBoxLayout->addWidget(m_pRightThumbnailList);

    pNoTrashWidget->setLayout(pNoTrashVBoxLayout);

    // Trash View
    DWidget *pTrashWidget = new DWidget();
    QVBoxLayout *pMainVBoxLayout = new QVBoxLayout();
    QHBoxLayout *pTopHBoxLayout = new QHBoxLayout();

    QVBoxLayout *pTopLeftVBoxLayout = new QVBoxLayout();
    DLabel *pLabel1 = new DLabel();
    pLabel1->setText("最近删除");
    pLabel1->setFont(ft);

    DLabel *pLabel2 = new DLabel();
    pLabel2->setText("照片在删除前会显示剩余天数，之后将永久删除");
    pLabel2->setFont(ft1);

    pTopLeftVBoxLayout->addWidget(pLabel1);
    pTopLeftVBoxLayout->addWidget(pLabel2);

    QHBoxLayout *pTopRightVBoxLayout = new QHBoxLayout();
    m_pRecoveryBtn = new DPushButton();
    m_pRecoveryBtn->setText(BUTTON_STR_RECOVERY);
    m_pRecoveryBtn->setEnabled(false);

    m_pDeleteBtn = new DPushButton();
    m_pDeleteBtn->setText(BUTTON_STR_DETELEALL);

    pTopRightVBoxLayout->addWidget(m_pRecoveryBtn);
    pTopRightVBoxLayout->addWidget(m_pDeleteBtn);

    pTopHBoxLayout->addItem(pTopLeftVBoxLayout);
    pTopHBoxLayout->addItem(pTopRightVBoxLayout);

    m_pRightTrashThumbnailList = new ThumbnailListView(TRASH_ALBUM);

    pMainVBoxLayout->addItem(pTopHBoxLayout);
    pMainVBoxLayout->addWidget(m_pRightTrashThumbnailList);

    pTrashWidget->setLayout(pMainVBoxLayout);

    // Favorite View
    DWidget *pFavoriteWidget = new DWidget();
    QVBoxLayout *pFavoriteVBoxLayout = new QVBoxLayout();

    m_pFavoriteTitle = new DLabel();
    m_pFavoriteTitle->setText(FAVORITES_ALBUM);
    m_pFavoriteTitle->setFont(ft);

    m_pFavoritePicTotal = new DLabel();
    QString favoriteStr = tr("%1张照片");
    m_pFavoritePicTotal->setFont(ft1);


    int favoritePicNum = DBManager::instance()->getImgsCountByAlbum(FAVORITES_ALBUM);
    m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(favoritePicNum)));

    m_pRightFavoriteThumbnailList = new ThumbnailListView(FAVORITES_ALBUM);

    pFavoriteVBoxLayout->addWidget(m_pFavoriteTitle);
    pFavoriteVBoxLayout->addWidget(m_pFavoritePicTotal);
    pFavoriteVBoxLayout->addWidget(m_pRightFavoriteThumbnailList);

    pFavoriteWidget->setLayout(pFavoriteVBoxLayout);

    // Add View
    m_pRightStackWidget->addWidget(m_pImportView);
    m_pRightStackWidget->addWidget(pNoTrashWidget);
    m_pRightStackWidget->addWidget(pTrashWidget);
    m_pRightStackWidget->addWidget(pFavoriteWidget);

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
        m_pRightThumbnailList->m_imageType = m_currentAlbum;
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
    }
    else
    {
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
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
    }
    else
    {
        infos = DBManager::instance()->getInfosByAlbum(m_currentAlbum);

        m_iAlubmPicsNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);
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
        QString favoriteStr = tr("%1张照片");
        m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(m_iAlubmPicsNum)));

        m_pRightFavoriteThumbnailList->insertThumbnails(thumbnaiItemList);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_FAVORITE_LIST);
        setAcceptDrops(false);
    }
    else if (RECENT_IMPORTED_ALBUM == m_currentAlbum)
    {
        if (0 < DBManager::instance()->getImgsCount())
        {
            m_pRightTitle->setText(m_currentAlbum);

            QString str = tr("%1张照片");
            m_pRightPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));

            m_pRightThumbnailList->insertThumbnails(thumbnaiItemList);
            m_pRightThumbnailList->m_imageType = m_currentAlbum;
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
        }
        else
        {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
        }

        setAcceptDrops(true);
    }
    else
    {
        m_pRightTitle->setText(m_currentAlbum);

        QFontMetrics elideFont(m_pRightTitle->font());
        m_pRightTitle->setText(elideFont.elidedText(m_currentAlbum,Qt::ElideRight, 525));

        QFont ft;
        ft.setFamily("SourceHanSansSC-Medium");
        ft.setPixelSize(24);
        m_pRightTitle->setFont(ft);

        QString str = tr("%1张照片");
        m_pRightPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));

        m_pRightThumbnailList->insertThumbnails(thumbnaiItemList);
        m_pRightThumbnailList->m_imageType = m_currentAlbum;
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
        setAcceptDrops(true);
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

    if (0 < infos.length())
    {
        m_pDeleteBtn->setEnabled(true);
    }
    else
    {
        m_pDeleteBtn->setText(BUTTON_STR_DETELEALL);
        m_pRecoveryBtn->setEnabled(false);
        m_pDeleteBtn->setEnabled(false);
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
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TRASH_LIST);

        m_iAlubmPicsNum = DBManager::instance()->getTrashImgsCount();
        setAcceptDrops(false);
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

    AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());

    if (RECENT_IMPORTED_ALBUM == item->m_albumNameStr ||
        TRASH_ALBUM == item->m_albumNameStr ||
        FAVORITES_ALBUM == item->m_albumNameStr)
    {
        return;
    }

    m_pLeftMenu->clear();

    if (0 != DBManager::instance()->getImgsCountByAlbum(item->m_albumNameStr))
    {
        appendAction(LEFT_MENU_SLIDESHOW);
        m_pLeftMenu->addSeparator();
    }
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
        auto imagelist = DBManager::instance()->getInfosByAlbum(m_currentAlbum);
        QStringList paths;
        for(auto image : imagelist)
        {
            paths<<image.filePath;
        }

        const QString path = paths.first();

        emit m_pRightThumbnailList->menuOpenImage(path, paths, true, true);
    }
    else if (LEFT_MENU_NEWALBUM == str)
    {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem();
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));

        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(getNewAlbumName());

        m_pLeftTabList->insertItem(m_pLeftTabList->currentRow()+1, pListWidgetItem);
        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);

        m_pLeftTabList->setCurrentRow(m_pLeftTabList->currentRow()+1);

        AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
        item->m_opeMode = OPE_MODE_ADDNEWALBUM;
        item->editAlbumEdit();

        m_currentAlbum = item->m_albumNameStr;
        updateRightNoTrashView();
    }
    else if (LEFT_MENU_RENAMEALBUM == str)
    {
        AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
        item->m_opeMode = OPE_MODE_RENAMEALBUM;
        item->editAlbumEdit();
    }
    else if (LEFT_MENU_EXPORT == str)
    {
        QListWidgetItem *item = m_pLeftTabList->currentItem();
        AlbumLeftTabItem *pTabItem = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(item);
        Exporter::instance()->exportImage(DBManager::instance()->getPathsByAlbum(pTabItem->m_albumNameStr));
    }
    else if (LEFT_MENU_DELALBUM == str)
    {
        QString str;
        QListWidgetItem *item = m_pLeftTabList->currentItem();
        AlbumLeftTabItem *pTabItem = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(item);

        str = pTabItem->m_albumNameStr;
        DBManager::instance()->removeAlbum(pTabItem->m_albumNameStr);
        delete  item;

        QModelIndex index;
        emit m_pLeftTabList->clicked(index);

        QString str1 = "相册：“%1”，已经删除成功";
        DUtil::DNotifySender *pDNotifySender = new DUtil::DNotifySender("深度相册");
        pDNotifySender->appName("deepin-album");
        pDNotifySender->appBody(str1.arg(str));
        pDNotifySender->call();
    }
    else
    {
        // donothing
    }
}

void AlbumView::createNewAlbum()
{
    QListWidgetItem *pListWidgetItem = new QListWidgetItem();
    pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
    AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(getNewAlbumName());

    m_pLeftTabList->insertItem(m_pLeftTabList->count()+1, pListWidgetItem);
    m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);

    m_pLeftTabList->setCurrentRow(m_pLeftTabList->count()-1);

    AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
    item->m_opeMode = OPE_MODE_ADDNEWALBUM;
    item->editAlbumEdit();

    m_currentAlbum = item->m_albumNameStr;
    updateRightNoTrashView();
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

    if (BUTTON_STR_DETELE == m_pDeleteBtn->text())
    {
        paths = m_pRightTrashThumbnailList->selectedPaths();
    }
    else
    {
        paths = DBManager::instance()->getAllTrashPaths();
        m_pDeleteBtn->setEnabled(false);
    }

    DBManager::instance()->removeTrashImgInfos(paths);
}

void AlbumView::openImage(int index)
{
    SignalManager::ViewInfo info;
    info.album = "";
    info.lastPanel = nullptr;

    auto imagelist = DBManager::instance()->getInfosByAlbum(m_currentAlbum);
    if (TRASH_ALBUM == m_currentAlbum)
    {
        imagelist = DBManager::instance()->getAllTrashInfos();
    }
    else if(RECENT_IMPORTED_ALBUM == m_currentAlbum)
    {
        imagelist = DBManager::instance()->getAllInfos();
    }
    else
    {

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
    info.viewType = m_currentAlbum;

    emit dApp->signalM->viewImage(info);
    emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALBUM);
}

void AlbumView::menuOpenImage(QString path,QStringList paths,bool isFullScreen, bool isSlideShow)
{
    SignalManager::ViewInfo info;
    info.album = "";
    info.lastPanel = nullptr;
    auto imagelist = DBManager::instance()->getInfosByAlbum(m_currentAlbum);
    if (TRASH_ALBUM == m_currentAlbum)
    {
        imagelist = DBManager::instance()->getAllTrashInfos();
    }
    else if(RECENT_IMPORTED_ALBUM == m_currentAlbum)
    {
        imagelist = DBManager::instance()->getAllInfos();
    }
    else
    {

    }

    if (paths.size()>1)
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
    info.viewType = m_currentAlbum;
    emit dApp->signalM->viewImage(info);
    emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALBUM);
}

QString AlbumView::getNewAlbumName()
{
    const QString nan = tr("Unnamed");
       int num = 1;
       QString albumName = nan;
       while(DBManager::instance()->isAlbumExistInDB(albumName)) {
           num++;
           albumName = nan + QString::number(num);
       }
       return (const QString)(albumName);
}

void AlbumView::onTrashListClicked()
{
    QStringList paths = m_pRightTrashThumbnailList->selectedPaths();
    paths.removeAll(QString(""));

    if (0 < paths.length())
    {
        m_pRecoveryBtn->setEnabled(true);
        m_pDeleteBtn->setText(BUTTON_STR_DETELE);
    }
    else
    {
        m_pRecoveryBtn->setEnabled(false);
        m_pDeleteBtn->setText(BUTTON_STR_DETELEALL);
    }
}

void AlbumView::dragEnterEvent(QDragEnterEvent *e)
{
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void AlbumView::dropEvent(QDropEvent *event)
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
        DBManager::instance()->insertImgInfos(dbInfos);
        picsIntoAlbum(paths);
    }

    event->accept();
}

void AlbumView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void AlbumView::dragLeaveEvent(QDragLeaveEvent *e)
{

}

void AlbumView::picsIntoAlbum(QStringList paths)
{
    if (RECENT_IMPORTED_ALBUM != m_currentAlbum
        && TRASH_ALBUM != m_currentAlbum
        && FAVORITES_ALBUM != m_currentAlbum)
    {
        DBManager::instance()->insertIntoAlbum(m_currentAlbum, paths);
    }
}

//AlbumView::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
//{
//    // TODO: Add your specialized code here and/or call the base class
//    DWORD ThreadId;
//    bool allzero=true;
//    size_t i;

//    if(message!=WM_DEVICECHANGE)
//    {
//        return CDialog::WindowProc(message, wParam, lParam);
//    }

//    if(wParam==DBT_DEVICEARRIVAL)//有新设备插入系统
//    {
//        DEV_BROADCAST_HDR* pDev=(DEV_BROADCAST_HDR*)lParam;
//        if(pDev->dbch_devicetype!=DBT_DEVTYP_VOLUME )//移动存储设备
//        {
//            return CDialog::WindowProc(message, wParam, Param);
//        }

//        DEV_BROADCAST_VOLUME* pDisk=(DEV_BROADCAST_VOLUME*)lParam;
//        DWORD mask=pDisk->dbcv_unitmask;

//        TCHAR diskname[MAX_PATH];
//        for(i=0;i<32;i++)
//        {
//            if((mask>>i)==1)
//            {//获取盘符
//                diskname[0]='A'+i;
//                diskname[1]='\0';
//                _tcscat_s(diskname,TEXT(":\\"));
//                break;
//            }
//        }
//    }
//}
