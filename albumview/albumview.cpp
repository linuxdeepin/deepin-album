#include "albumview.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "controller/exporter.h"
#include "dtkcore_global.h"

#include <DNotifySender>
#include <QMimeData>
#include <DTableView>
#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <DFontSizeManager>
#include "utils/snifferimageformat.h"

namespace {
const int ITEM_SPACING = 0;
const int LEFT_VIEW_WIDTH = 180;
const int LEFT_VIEW_LISTITEM_WIDTH = 160;
const int LEFT_VIEW_LISTITEM_HEIGHT = 40;
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
    m_vfsManager = new DGioVolumeManager;

    setAcceptDrops(true);
    initLeftView();
    initRightView();

    QHBoxLayout *pLayout = new QHBoxLayout();
    pLayout->setContentsMargins(0,0,0,0);
    pLayout->addWidget(m_pLeftWidget);
    pLayout->addWidget(m_pRightStackWidget);
    setLayout(pLayout);

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
    connect(dApp, &Application::sigFinishLoad, this, &AlbumView::updateRightView);
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

    connect(dApp->signalM, &SignalManager::sigUpdataAlbumRightTitle, this, &AlbumView::onUpdataAlbumRightTitle);
//    connect(m_vfsManager, &DGioVolumeManager::mountAdded, this, &AlbumView::onVfsMountChanged);
//    connect(m_vfsManager, &DGioVolumeManager::mountRemoved, this, &AlbumView::onVfsMountChanged);
}

void AlbumView::initLeftView()
{
    m_pLeftTabList = new DListWidget();

    m_pLeftTabList->setFixedWidth(162);
    m_pLeftTabList->setSpacing(ITEM_SPACING);
    m_pLeftTabList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_pLeftTabList->setFrameShape(DTableView::NoFrame);

    m_pLeftWidget = new DWidget();
    m_pLeftWidget->setFixedWidth(LEFT_VIEW_WIDTH);

    DPalette pa;
    pa.setColor(DPalette::Background,QColor(255, 255, 255));
    m_pLeftWidget->setAutoFillBackground(true);
    m_pLeftWidget->setPalette(pa);

    QHBoxLayout *pLeftLayout = new QHBoxLayout();
    pLeftLayout->setContentsMargins(0,0,0,0);
    pLeftLayout->addStretch();
    pLeftLayout->addWidget(m_pLeftTabList, Qt::AlignHCenter);

    m_pLeftWidget->setLayout(pLeftLayout);

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
        pAlbumLeftTabItem->setFixedWidth(LEFT_VIEW_LISTITEM_WIDTH);
        pAlbumLeftTabItem->setFixedHeight(LEFT_VIEW_LISTITEM_HEIGHT);
        if (RECENT_IMPORTED_ALBUM == albumName)
        {
            pListWidgetItem->setSelected(true);
        }
        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    }

    //init externalDevice
    const QList<QExplicitlySharedDataPointer<DGioMount> > mounts = getVfsMountList();
    updateExternalDevice(mounts);
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

    const QList<QExplicitlySharedDataPointer<DGioMount> > mounts = getVfsMountList();
    updateExternalDevice(mounts);
    updateRightNoTrashView();
}

void AlbumView::initRightView()
{
    m_pRightStackWidget = new DStackedWidget();

    // Import View
    m_pImportView = new ImportView();

    // Thumbnail View
    DWidget *pNoTrashWidget = new DWidget();
    QVBoxLayout *pNoTrashVBoxLayout = new QVBoxLayout();
    pNoTrashVBoxLayout->setContentsMargins(0,0,0,0);

    m_pRightTitle = new DLabel();
    m_pRightTitle->setText(RECENT_IMPORTED_ALBUM);

    QFont ft = DFontSizeManager::instance()->get(DFontSizeManager::T3);
    ft.setFamily("SourceHanSansSC");
    ft.setWeight(QFont::Medium);

    QFont ft1 = DFontSizeManager::instance()->get(DFontSizeManager::T6);
    ft1.setFamily("SourceHanSansSC");
    ft1.setWeight(QFont::Medium);

    m_pRightTitle->setFont(ft);

    DPalette pa = DApplicationHelper::instance()->palette(m_pRightTitle);
    pa.setBrush(DPalette::WindowText, pa.color(DPalette::ToolTipText));
    m_pRightTitle->setPalette(pa);

    m_pRightPicTotal = new DLabel();

    QString str = tr("%1张照片");
    m_pRightPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));
    m_pRightPicTotal->setFont(ft1);

    DPalette palette = DApplicationHelper::instance()->palette(m_pRightPicTotal);
//    palette.setBrush(DPalette::WindowText, palette.color(DPalette::WindowText));
    palette.setBrush(DPalette::WindowText, QColor(119,119,119));
    m_pRightPicTotal->setPalette(palette);


    m_pRightThumbnailList = new ThumbnailListView(RECENT_IMPORTED_ALBUM);
    m_pRightThumbnailList->setFrameShape(DTableView::NoFrame);

    pNoTrashVBoxLayout->addSpacing(5);
    pNoTrashVBoxLayout->addWidget(m_pRightTitle);
    pNoTrashVBoxLayout->addSpacing(9);
    pNoTrashVBoxLayout->addWidget(m_pRightPicTotal);
    pNoTrashVBoxLayout->addSpacing(7);
    pNoTrashVBoxLayout->setContentsMargins(10,0,0,0);

    QVBoxLayout *p_all = new QVBoxLayout();
    p_all->addLayout(pNoTrashVBoxLayout);
    p_all->addWidget(m_pRightThumbnailList);

    pNoTrashWidget->setLayout(p_all);

    // Trash View
    DWidget *pTrashWidget = new DWidget();
    QVBoxLayout *pMainVBoxLayout = new QVBoxLayout();
    QHBoxLayout *pTopHBoxLayout = new QHBoxLayout();

    QVBoxLayout *pTopLeftVBoxLayout = new QVBoxLayout();
    DLabel *pLabel1 = new DLabel();
    pLabel1->setText("最近删除");
    pLabel1->setFont(ft);
    pLabel1->setPalette(pa);

    DLabel *pLabel2 = new DLabel();
    pLabel2->setText("照片在删除前会显示剩余天数，之后将永久删除");
    pLabel2->setFont(ft1);
    pLabel2->setPalette(palette);

    pTopLeftVBoxLayout->addSpacing(5);
    pTopLeftVBoxLayout->addWidget(pLabel1);
    pTopLeftVBoxLayout->addSpacing(9);
    pTopLeftVBoxLayout->addWidget(pLabel2);
    pTopLeftVBoxLayout->addSpacing(7);
    pTopLeftVBoxLayout->setContentsMargins(10,0,0,0);

    QHBoxLayout *pTopRightVBoxLayout = new QHBoxLayout();
    m_pRecoveryBtn = new DPushButton();
    m_pRecoveryBtn->setText(BUTTON_STR_RECOVERY);
    m_pRecoveryBtn->setEnabled(false);
    m_pRecoveryBtn->setFixedSize(100,36);
//    DPalette pal = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
//    pal.setBrush(DPalette::Light, pal.color(DPalette::Text));
//    pal.setBrush(DPalette::Dark, pal.color(DPalette::Text));
//    pal.setBrush(DPalette::ButtonText, pal.color(DPalette::HighlightedText));
    DPalette pal = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
    pal.setBrush(DPalette::Light, QColor(100,100,100));
    pal.setBrush(DPalette::Dark, QColor(100,100,100));
    pal.setBrush(DPalette::ButtonText, pal.color(DPalette::HighlightedText));
    m_pRecoveryBtn->setPalette(pal);

    m_pDeleteBtn = new DPushButton();
    m_pDeleteBtn->setText(BUTTON_STR_DETELEALL);
    m_pDeleteBtn->setFixedSize(100,36);
//    DPalette dpa = DApplicationHelper::instance()->palette(m_pDeleteBtn);
//    dpa.setBrush(DPalette::Light, dpa.color(DPalette::Highlight));
//    dpa.setBrush(DPalette::Dark, dpa.color(DPalette::Highlight));
//    dpa.setBrush(DPalette::ButtonText, dpa.color(DPalette::HighlightedText));
    DPalette dpa = DApplicationHelper::instance()->palette(m_pDeleteBtn);
    dpa.setBrush(DPalette::Light, QColor(37,183,255));
    dpa.setBrush(DPalette::Dark, QColor(37,183,255));
    dpa.setBrush(DPalette::ButtonText, dpa.color(DPalette::HighlightedText));
    m_pDeleteBtn->setPalette(dpa);

    pTopRightVBoxLayout->addWidget(m_pRecoveryBtn);
    pTopRightVBoxLayout->addSpacing(10);
    pTopRightVBoxLayout->addWidget(m_pDeleteBtn);

    pTopHBoxLayout->addItem(pTopLeftVBoxLayout);
    pTopHBoxLayout->addStretch();
    pTopHBoxLayout->addItem(pTopRightVBoxLayout);
    pTopHBoxLayout->addSpacing(20);


    m_pRightTrashThumbnailList = new ThumbnailListView(TRASH_ALBUM);
    m_pRightTrashThumbnailList->setFrameShape(DTableView::NoFrame);

    pMainVBoxLayout->addItem(pTopHBoxLayout);
    pMainVBoxLayout->addWidget(m_pRightTrashThumbnailList);

    pTrashWidget->setLayout(pMainVBoxLayout);

    // Favorite View
    DWidget *pFavoriteWidget = new DWidget();
    QVBoxLayout *pFavoriteVBoxLayout = new QVBoxLayout();

    m_pFavoriteTitle = new DLabel();
    m_pFavoriteTitle->setText(FAVORITES_ALBUM);
    m_pFavoriteTitle->setFont(ft);
    m_pFavoriteTitle->setPalette(pa);

    m_pFavoritePicTotal = new DLabel();
    QString favoriteStr = tr("%1张照片");
    m_pFavoritePicTotal->setFont(ft1);
    m_pFavoritePicTotal->setPalette(palette);


    int favoritePicNum = DBManager::instance()->getImgsCountByAlbum(FAVORITES_ALBUM);
    m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(favoritePicNum)));

    m_pRightFavoriteThumbnailList = new ThumbnailListView(FAVORITES_ALBUM);
    m_pRightFavoriteThumbnailList->setFrameShape(DTableView::NoFrame);

    pFavoriteVBoxLayout->addSpacing(5);
    pFavoriteVBoxLayout->addWidget(m_pFavoriteTitle);
    pFavoriteVBoxLayout->addSpacing(9);
    pFavoriteVBoxLayout->addWidget(m_pFavoritePicTotal);
    pFavoriteVBoxLayout->addSpacing(7);

    pFavoriteVBoxLayout->setContentsMargins(10,0,0,0);

    QVBoxLayout *p_all1 = new QVBoxLayout();
    p_all1->addLayout(pFavoriteVBoxLayout);
    p_all1->addWidget(m_pRightFavoriteThumbnailList);

    pFavoriteWidget->setLayout(p_all1);

    //Search View
    m_pSearchView = new SearchView;

    // Add View
    m_pRightStackWidget->addWidget(m_pImportView);
    m_pRightStackWidget->addWidget(pNoTrashWidget);
    m_pRightStackWidget->addWidget(pTrashWidget);
    m_pRightStackWidget->addWidget(pFavoriteWidget);
    m_pRightStackWidget->addWidget(m_pSearchView);

    if (0 < DBManager::instance()->getImgsCount())
    {
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

//        m_pRightThumbnailList->insertThumbnails(thumbnaiItemList);
//        m_pRightThumbnailList->m_imageType = m_currentAlbum;
        m_pRightThumbnailList->setFrameShape(DTableView::NoFrame);
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

    //刷新外接设备中的图像文件，取currentItem()->data(Qt::UserRole)中的path，然后地柜遍历路径下的图片文件
    QListWidgetItem *pItem = m_pLeftTabList->currentItem();
    if (pItem) {
        QString strPath = m_pLeftTabList->currentItem()->data(Qt::UserRole).toString();
        if (!strPath.isEmpty())
        {
            findPictureFile(strPath, thumbnaiItemList);
            m_iAlubmPicsNum = thumbnaiItemList.size();
        }
    }

    for(auto info : infos)
    {
        ThumbnailListView::ItemInfo vi;
        vi.name = info.fileName;
        vi.path = info.filePath;
        vi.image = dApp->m_imagemap.value(info.filePath);

        thumbnaiItemList<<vi;
    }

    if (FAVORITES_ALBUM == m_currentAlbum)
    {
        QString favoriteStr = tr("%1张照片");
        m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(m_iAlubmPicsNum)));

        DPalette palette = DApplicationHelper::instance()->palette(m_pRightPicTotal);
        palette.setBrush(DPalette::WindowText, palette.color(DPalette::WindowText));
        m_pFavoritePicTotal->setPalette(palette);

        QFont ft = DFontSizeManager::instance()->get(DFontSizeManager::T6);
        ft.setFamily("SourceHanSansSC");
        ft.setWeight(QFont::Medium);
        m_pFavoritePicTotal->setFont(ft);

        m_pRightFavoriteThumbnailList->insertThumbnails(thumbnaiItemList);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_FAVORITE_LIST);
        setAcceptDrops(false);
    }
    else if (RECENT_IMPORTED_ALBUM == m_currentAlbum)
    {
        if (0 < DBManager::instance()->getImgsCount())
        {
            m_pRightTitle->setText(m_currentAlbum);
            QFont ft1 = DFontSizeManager::instance()->get(DFontSizeManager::T3);
            ft1.setFamily("SourceHanSansSC");
            ft1.setWeight(QFont::Medium);
            m_pRightTitle->setFont(ft1);

            QString str = tr("%1张照片");
            m_pRightPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));

            DPalette palette = DApplicationHelper::instance()->palette(m_pRightPicTotal);
            palette.setBrush(DPalette::WindowText, palette.color(DPalette::WindowText));
            m_pRightPicTotal->setPalette(palette);
            QFont ft = DFontSizeManager::instance()->get(DFontSizeManager::T6);
            ft.setFamily("SourceHanSansSC");
            ft.setWeight(QFont::Medium);
            m_pRightPicTotal->setFont(ft);

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

        QFont ft1 = DFontSizeManager::instance()->get(DFontSizeManager::T3);
        ft1.setFamily("SourceHanSansSC");
        ft1.setWeight(QFont::Medium);
        m_pRightTitle->setFont(ft1);

        QString str = tr("%1张照片");
        m_pRightPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));

        DPalette palette = DApplicationHelper::instance()->palette(m_pRightPicTotal);
        palette.setBrush(DPalette::WindowText, palette.color(DPalette::WindowText));
        m_pRightPicTotal->setPalette(palette);
        QFont ft = DFontSizeManager::instance()->get(DFontSizeManager::T6);
        ft.setFamily("SourceHanSansSC");
        ft.setWeight(QFont::Medium);
        m_pRightPicTotal->setFont(ft);

        m_pRightThumbnailList->insertThumbnails(thumbnaiItemList);
        m_pRightThumbnailList->m_imageType = m_currentAlbum;
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
        setAcceptDrops(true);
    }
}

void AlbumView::updateRightTrashView()
{
    int idaysec = 24*60*60;

    QList<ThumbnailListView::ItemInfo> thumbnaiItemList;

    DBImgInfoList infos;
    QStringList removepaths;

    infos = DBManager::instance()->getAllTrashInfos();

    for(auto info : infos)
    {
        QDateTime start = QDateTime::currentDateTime();
        QDateTime end = info.time;

        uint etime = start.toTime_t();
        uint stime = end.toTime_t();

        int Day = (etime - stime)/(idaysec) + ((etime - stime)%(idaysec)+(idaysec-1))/(idaysec) - 1;

        if (30 <= Day)
        {
            removepaths << info.filePath;
        }
        else
        {
            ThumbnailListView::ItemInfo vi;
            vi.name = info.fileName;
            vi.path = info.filePath;
            vi.image = dApp->m_imagetrashmap.value(info.filePath);
            vi.remainDays = QString::number(30-Day) + "天";

            thumbnaiItemList<<vi;
        }
    }

    if (0 < removepaths.length())
    {
        for(auto path : removepaths)
        {
            dApp->m_imagetrashmap.remove(path);
        }

        DBManager::instance()->removeTrashImgInfosNoSignal(removepaths);
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
        QFileInfo fi(info.filePath);
        info.time = fi.birthTime();
        infos<<info;

        dApp->m_imagetrashmap.remove(path);
    }

    dApp->m_imageloader->addImageLoader(paths);
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

    for(auto path : paths)
    {
        dApp->m_imagetrashmap.remove(path);
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
        QStringList paths;
        for(auto info : dbInfos)
        {
            paths<<info.filePath;
        }

        dApp->m_imageloader->addImageLoader(paths);
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

void AlbumView::onVfsMountChanged(QExplicitlySharedDataPointer<DGioMount> mount)
{
    updateLeftView();
}

const QList<QExplicitlySharedDataPointer<DGioMount> > AlbumView::getVfsMountList()
{
    QList<QExplicitlySharedDataPointer<DGioMount> > result;
    const QList<QExplicitlySharedDataPointer<DGioMount> > mounts = m_vfsManager->getMounts();

    for (auto mount : mounts) {
        result.append(mount);
    }

    return result;
}

bool AlbumView::findPictureFile(const QString &path, QList<ThumbnailListView::ItemInfo>& thumbnaiItemList)
{
    QDir dir(path);
    if (!dir.exists()) return false;

    dir.setFilter(QDir::Dirs | QDir::Files);
    dir.setSorting(QDir::DirsFirst);
    QFileInfoList list = dir.entryInfoList();
    if (list.size() == 0) return  false;
    int i=0;
    do {
        QFileInfo fileInfo = list.at(i);

        if (fileInfo.fileName()=="." | fileInfo.fileName()=="..")
        {
            i++;
            continue;
        }

        bool bisDir=fileInfo.isDir();
        if (bisDir)
        {
            findPictureFile(fileInfo.filePath(), thumbnaiItemList);
        }
        else {
            if (fileInfo.fileName().contains(".jpg")
                    || fileInfo.fileName().contains(".jpeg")) {
                QString strPicPath = QString("%1%2%3").arg(fileInfo.path(), "/", fileInfo.fileName());
                ThumbnailListView::ItemInfo vi;
                vi.name = fileInfo.fileName();
                vi.path = strPicPath;

                thumbnaiItemList<<vi;
            }
        }
        i++;
    } while(i<list.size());

    return true;
}

void AlbumView::updateExternalDevice(QList<QExplicitlySharedDataPointer<DGioMount> > mounts)
{
    for (auto mount : mounts) {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftTabList);
        //pListWidgetItem缓存文件挂载路径
        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
        QString strPath = LocationFile->path();
        pListWidgetItem->setData(Qt::UserRole, strPath);
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(mount->name(), "External Devices");
        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    }
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

void AlbumView::onUpdataAlbumRightTitle(QString titlename)
{
    m_currentAlbum = titlename;
    updateRightView();
}

void AlbumView::SearchReturnUpdate()
{
    if (4 == m_pRightStackWidget->currentIndex())
    {
        m_currentAlbum = RECENT_IMPORTED_ALBUM;
        updateRightView();
    }
}



