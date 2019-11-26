#include "albumview.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "controller/exporter.h"
#include "dtkcore_global.h"
#include "dialogs/albumdeletedialog.h"
#include "dbmanager/dbmanager.h"
#include <DNotifySender>
#include <QMimeData>
#include <DTableView>
#include <dgiovolumemanager.h>
#include <dgiofile.h>
#include <dgiofileinfo.h>
#include <DFontSizeManager>
#include "utils/snifferimageformat.h"
#include <QDirIterator>
#include <DComboBox>
#include "widgets/dialogs/imgdeletedialog.h"
#include <QShortcut>
#include <DWarningButton>
#include <QGraphicsOpacityEffect>
#include <DToast>
#include "dmessagemanager.h"

namespace {
const int ITEM_SPACING = 0;
const int LEFT_VIEW_WIDTH = 180;
const int LEFT_VIEW_LISTITEM_WIDTH = 160;
const int LEFT_VIEW_LISTITEM_HEIGHT = 40;
const int OPE_MODE_ADDNEWALBUM = 0;
const int OPE_MODE_RENAMEALBUM = 1;
const int OPE_MODE_ADDRENAMEALBUM = 2;
const QString BUTTON_STR_RECOVERY = "恢复";
const QString BUTTON_STR_DETELE = "删除";
const QString BUTTON_STR_DETELEALL = "全部删除";
const int RIGHT_VIEW_IMPORT = 0;
const int RIGHT_VIEW_THUMBNAIL_LIST = 1;
const int RIGHT_VIEW_TRASH_LIST = 2;
const int RIGHT_VIEW_FAVORITE_LIST = 3;
const int RIGHT_VIEW_SEARCH = 4;
const int RIGHT_VIEW_SPINNER = 5;
const int VIEW_MAINWINDOW_ALBUM = 2;
const QString SHORTCUTVIEW_GROUP = "SHORTCUTVIEW";

QString ss(const QString &text)
{
    QString str = dApp->setter->value(SHORTCUTVIEW_GROUP, text).toString();
    str.replace(" ", "");
    return str;
}
}  //namespace

using namespace utils::common;

AlbumView::AlbumView()
{
    m_currentAlbum = COMMON_STR_RECENT_IMPORTED;
    m_iAlubmPicsNum = DBManager::instance()->getImgsCount();
    m_vfsManager = new DGioVolumeManager;
    m_curListWidgetItem = nullptr;
    m_loadMountFlag = 0;
    m_mountPicNum = 0;
	
    auto infos = DBManager::instance()->getAllInfos();
    QStringList pathlist;
    foreach(auto info, infos)
    {
        pathlist.append(info.filePath);
    }

    auto infostrash = DBManager::instance()->getAllTrashInfos();
    QStringList pathlisttrash;
    foreach(auto info, infostrash)
    {
        pathlisttrash.append(info.filePath);
    }
    m_mountloader= new MountLoader(this);
    m_LoadThread = new QThread();

    m_mountloader->moveToThread(m_LoadThread);
    m_LoadThread->start();

    connect(this, SIGNAL(sigLoadMountImagesStart(QString, QString)),
            m_mountloader, SLOT(onLoadMountImagesStart(QString, QString)));

    setAcceptDrops(true);
    initLeftView();
    initRightView();
    initLeftMenu();

    QHBoxLayout *pLayout = new QHBoxLayout();
    pLayout->setContentsMargins(0,0,0,0);
    pLayout->addWidget(m_pLeftWidget);
    pLayout->addWidget(m_pWidget);
    setLayout(pLayout);

    initConnections();
}

AlbumView::~AlbumView()
{
    if (m_vfsManager) {
        delete  m_vfsManager;
        m_vfsManager = nullptr;
    }
}

void AlbumView::initConnections()
{
    connect(m_pLeftTabList, &DListWidget::clicked, this, &AlbumView::leftTabClicked);
    connect(dApp->signalM, &SignalManager::sigCreateNewAlbumFromDialog, this, &AlbumView::onCreateNewAlbumFromDialog);
#if 1
    connect(dApp->signalM, &SignalManager::sigCreateNewAlbumFrom, this, &AlbumView::onCreateNewAlbumFrom);
#endif
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
    connect(dApp->signalM, &SignalManager::sigUpdataAlbumRightTitle, this, &AlbumView::onUpdataAlbumRightTitle);    
    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::sigUpdateTrashImageLoader, this, &AlbumView::updateRightView);
    connect(m_vfsManager, &DGioVolumeManager::mountAdded, this, &AlbumView::onVfsMountChangedAdd);
    connect(m_vfsManager, &DGioVolumeManager::mountRemoved, this, &AlbumView::onVfsMountChangedRemove);
    connect(m_importAllByPhoneBtn, &DPushButton::clicked, this, &AlbumView::importAllBtnClicked);
    connect(m_importSelectByPhoneBtn, &DPushButton::clicked, this, &AlbumView::importSelectBtnClicked);
    connect(m_pStatusBar->m_pSlider, &DSlider::valueChanged, dApp->signalM, &SignalManager::sigMainwindowSliderValueChg);
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
                        this, [=]
    {
        DPalette ReBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
        ReBtn.setBrush(DPalette::Highlight, QColor(0,0,0,0));
        m_pRecoveryBtn->setPalette(ReBtn);

        DPalette DeBtn = DApplicationHelper::instance()->palette(m_pDeleteBtn);
        DeBtn.setBrush(DPalette::Highlight, QColor(0,0,0,0));
        m_pDeleteBtn->setPalette(DeBtn);

    });
    connect(dApp->signalM, &SignalManager::sigLoadMountImagesEnd, this, &AlbumView::onLoadMountImagesEnd);
#if 1
    connect(m_pRightThumbnailList, &ThumbnailListView::customContextMenuRequested, this, &AlbumView::updatePicNum);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::customContextMenuRequested, this, &AlbumView::updatePicNum);
    connect(m_pRightFavoriteThumbnailList, &ThumbnailListView::customContextMenuRequested, this, &AlbumView::updatePicNum);

    connect(m_pRightThumbnailList, &ThumbnailListView::sigMouseRelease, this, &AlbumView::updatePicNum);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::sigMouseRelease, this, &AlbumView::updatePicNum);
    connect(m_pRightFavoriteThumbnailList, &ThumbnailListView::sigMouseRelease, this, &AlbumView::updatePicNum);

    connect(m_pRightTrashThumbnailList, &ThumbnailListView::customContextMenuRequested, this, &AlbumView::onTrashListClicked);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::sigMouseRelease, this, &AlbumView::onTrashListClicked);
#endif
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::trashRecovery, this, &AlbumView::onTrashRecoveryBtnClicked);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::trashDelete, this, &AlbumView::onTrashListClicked);

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, m_pLeftTabList,[=]{
        AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
        item->newAlbumStatus();
    });
    connect(m_pLeftTabList, &DListWidget::currentItemChanged, this, [=]{
        if(0 < m_pLeftTabList->count())
        {
            for(int i = 0; i < m_pLeftTabList->count(); i++)
            {
                AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->item(i));
                item->oriAlbumStatus();
            }
            AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
            item->newAlbumStatus();
            if (COMMON_STR_TRASH == item->m_albumNameStr)
            {
                m_currentAlbum = item->m_albumNameStr;
                updateRightView();
                m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TRASH_LIST);

                m_iAlubmPicsNum = DBManager::instance()->getTrashImgsCount();
                setAcceptDrops(false);
            }
            else
            {
                m_currentAlbum = item->m_albumNameStr;
                updateRightView();
            }
        }
    });

    connect(m_pImportView->m_pImportBtn, &DPushButton::clicked, this, [=]{
        m_pImportView->onImprotBtnClicked();
    });
    connect(m_pImportView, &ImportView::importFailedToView, this, [=]{
    });
}

void AlbumView::initLeftView()
{
    m_pLeftTabList = new LeftListWidget();

    DStyledItemDelegate* itemDelegate = new DStyledItemDelegate(m_pLeftTabList);
    itemDelegate->setBackgroundType(DStyledItemDelegate::NoBackground);
    m_pLeftTabList->setItemDelegate(itemDelegate);

    m_pLeftTabList->setFixedWidth(LEFT_VIEW_WIDTH);
    m_pLeftTabList->setSpacing(ITEM_SPACING);
    m_pLeftTabList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_pLeftTabList->setFrameShape(DTableView::NoFrame);

    m_pLeftWidget = new DWidget();
    m_pLeftWidget->setFixedWidth(LEFT_VIEW_WIDTH);
    m_pLeftWidget->setBackgroundRole(DPalette::Base);
    m_pLeftWidget->setAutoFillBackground(true);

    QHBoxLayout *pLeftLayout = new QHBoxLayout();
    pLeftLayout->setContentsMargins(0,0,0,0);
    pLeftLayout->addStretch();
    pLeftLayout->addWidget(m_pLeftTabList, Qt::AlignHCenter);
    pLeftLayout->addStretch();

    m_pLeftWidget->setLayout(pLeftLayout);

    m_pLeftMenu = new DMenu();

    m_allAlbumNames<<COMMON_STR_RECENT_IMPORTED;
    m_allAlbumNames<<COMMON_STR_TRASH;
    m_allAlbumNames<<COMMON_STR_FAVORITES;

    QStringList allAlbumNames = DBManager::instance()->getAllAlbumNames();
    for(auto albumName : allAlbumNames)
    {
        if (COMMON_STR_FAVORITES == albumName || COMMON_STR_RECENT_IMPORTED == albumName || COMMON_STR_TRASH == albumName)
        {
            continue;
        }

        m_allAlbumNames<<albumName;
        m_customAlbumNames << albumName;
    }

    for(auto albumName : m_allAlbumNames)
    {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftTabList);
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));

        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName, m_pLeftTabList, pListWidgetItem);
        pAlbumLeftTabItem->setFixedWidth(LEFT_VIEW_LISTITEM_WIDTH);
        pAlbumLeftTabItem->setFixedHeight(LEFT_VIEW_LISTITEM_HEIGHT);
        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    }

    m_pLeftTabList->setCurrentRow(0);

    //init externalDevice
    m_mounts = getVfsMountList();
    initExternalDevice();

    AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->item(0));
    item->newAlbumStatus();
}

void AlbumView::onCreateNewAlbumFromDialog(QString newalbumname)
{
    //新建相册需要在外接设备节点上面，此处调用getNewAlbumItemIndex函数，获取新建相册的index
    int index = getNewAlbumItemIndex();

    QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftTabList);
    pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
    QString albumName = newalbumname;
    AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName, m_pLeftTabList, pListWidgetItem);
    m_customAlbumNames << albumName;

    m_pLeftTabList->insertItem(index, pListWidgetItem);
    m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    m_pLeftTabList->setCurrentRow(index);
    m_curListWidgetItem = m_pLeftTabList->currentItem();
    m_currentAlbum = albumName;
    updateRightView();
}

#if 1
void AlbumView::onCreateNewAlbumFrom(QString albumname)
{
    int index = getNewAlbumItemIndex();

    QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftTabList);
    pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
    QString albumName = albumname;
    AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName, m_pLeftTabList, pListWidgetItem);
    m_customAlbumNames << albumName;

    m_pLeftTabList->insertItem(index, pListWidgetItem);
    m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
}
#endif

void AlbumView::initRightView()
{
    m_pRightStackWidget = new DStackedWidget();
    m_pWidget = new DWidget();

    // Import View
    m_pImportView = new ImportView();
    QList<QLabel*> labelList = m_pImportView->findChildren<QLabel*>();
    labelList[1]->setText(tr("You can also drag it here"));

    // Thumbnail View
    DWidget *pNoTrashWidget = new DWidget();
    pNoTrashWidget->setBackgroundRole(DPalette::Window);

    QVBoxLayout *pNoTrashVBoxLayout = new QVBoxLayout();
    pNoTrashVBoxLayout->setContentsMargins(0,0,0,0);

    m_pRightTitle = new DLabel();
    m_pRightTitle->setText(tr("Imported"));
    DFontSizeManager::instance()->bind(m_pRightTitle, DFontSizeManager::T3, QFont::Medium);
    m_pRightTitle->setForegroundRole(DPalette::TextTitle);

    m_pRightPicTotal = new DLabel();

    QString str = tr("%1 Photos");
    m_pRightPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));
    DFontSizeManager::instance()->bind(m_pRightPicTotal, DFontSizeManager::T6, QFont::Medium);
    m_pRightPicTotal->setForegroundRole(DPalette::TextTips);

    m_pRightThumbnailList = new ThumbnailListView(COMMON_STR_RECENT_IMPORTED);
    m_pRightThumbnailList->setFrameShape(DTableView::NoFrame);

    pNoTrashVBoxLayout->addSpacing(5);
    pNoTrashVBoxLayout->addWidget(m_pRightTitle);
    pNoTrashVBoxLayout->addSpacing(9);
    pNoTrashVBoxLayout->addWidget(m_pRightPicTotal);
    pNoTrashVBoxLayout->addSpacing(-6);
    pNoTrashVBoxLayout->setContentsMargins(10,0,0,0);

    //手机相片导入窗体
    m_importByPhoneWidget = new DWidget;
    QHBoxLayout *mainImportLayout = new QHBoxLayout;
    DLabel *importLabel = new DLabel();
    importLabel->setText(tr("Imported into："));

    m_importByPhoneComboBox = new DComboBox;
    m_importByPhoneComboBox->setMinimumSize(QSize(213, 36));

    m_importAllByPhoneBtn = new DPushButton(tr("All Import"));
    DPalette importAllByPhoneBtnPa = DApplicationHelper::instance()->palette(m_importAllByPhoneBtn);
    importAllByPhoneBtnPa.setBrush(DPalette::Highlight, QColor(0,0,0,0));
    m_importAllByPhoneBtn->setPalette(importAllByPhoneBtnPa);

    m_importSelectByPhoneBtn = new DSuggestButton(tr("Selected Import"));
    DPalette importSelectByPhoneBtnPa = DApplicationHelper::instance()->palette(m_importSelectByPhoneBtn);
    importSelectByPhoneBtnPa.setBrush(DPalette::Highlight, QColor(0,0,0,0));
    m_importSelectByPhoneBtn->setPalette(importSelectByPhoneBtnPa);

    mainImportLayout->addWidget(importLabel);
    mainImportLayout->addSpacing(11);
    mainImportLayout->addWidget(m_importByPhoneComboBox);
    mainImportLayout->addSpacing(30);
    mainImportLayout->addWidget(m_importAllByPhoneBtn);
    mainImportLayout->addSpacing(10);
    mainImportLayout->addWidget(m_importSelectByPhoneBtn);
    m_importByPhoneWidget->setLayout(mainImportLayout);
    m_importByPhoneWidget->setVisible(false);

    QHBoxLayout *allHLayout = new QHBoxLayout;
    allHLayout->addLayout(pNoTrashVBoxLayout, 1);
    allHLayout->addStretch();
    allHLayout->addWidget(m_importByPhoneWidget, 1);

    QVBoxLayout *p_all = new QVBoxLayout();
    p_all->addLayout(allHLayout);
    p_all->addWidget(m_pRightThumbnailList);

    pNoTrashWidget->setLayout(p_all);

    // Trash View
    DWidget *pTrashWidget = new DWidget();
    QVBoxLayout *pMainVBoxLayout = new QVBoxLayout();
    QHBoxLayout *pTopHBoxLayout = new QHBoxLayout();

    QVBoxLayout *pTopLeftVBoxLayout = new QVBoxLayout();

    pLabel1 = new DLabel();
    DFontSizeManager::instance()->bind(pLabel1, DFontSizeManager::T3, QFont::Medium);
    pLabel1->setForegroundRole(DPalette::TextTitle);
    pLabel1->setText(tr("Trash"));

    pLabel2 = new DLabel();
    DFontSizeManager::instance()->bind(pLabel2, DFontSizeManager::T6, QFont::Medium);
    pLabel2->setForegroundRole(DPalette::TextTips);
    pLabel2->setText(tr("The photos shows the number of days left before it is deleted, and then it is permanently deleted"));

    pTopLeftVBoxLayout->addSpacing(5);
    pTopLeftVBoxLayout->addWidget(pLabel1);
    pTopLeftVBoxLayout->addSpacing(9);
    pTopLeftVBoxLayout->addWidget(pLabel2);
    pTopLeftVBoxLayout->addSpacing(-4);
    pTopLeftVBoxLayout->setContentsMargins(10,0,0,0);

    QHBoxLayout *pTopRightVBoxLayout = new QHBoxLayout();
    m_pRecoveryBtn = new DPushButton();

    m_pRecoveryBtn->setText(tr("Recovery"));
    m_pRecoveryBtn->setEnabled(false);
    m_pRecoveryBtn->setFixedSize(100,36);

    DPalette ReBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
    ReBtn.setBrush(DPalette::Highlight, QColor(0,0,0,0));
    m_pRecoveryBtn->setPalette(ReBtn);

    m_pDeleteBtn = new DWarningButton();

    m_pDeleteBtn->setText(tr("Delete All"));
    m_pDeleteBtn->setFixedSize(100,36);

    DPalette DeBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
    ReBtn.setBrush(DPalette::Highlight, QColor(0,0,0,0));
    m_pDeleteBtn->setPalette(ReBtn);

    pTopRightVBoxLayout->addWidget(m_pRecoveryBtn);
    pTopRightVBoxLayout->addSpacing(10);
    pTopRightVBoxLayout->addWidget(m_pDeleteBtn);

    pTopHBoxLayout->addItem(pTopLeftVBoxLayout);
    pTopHBoxLayout->addStretch();
    pTopHBoxLayout->addItem(pTopRightVBoxLayout);
    pTopHBoxLayout->addSpacing(20);


    m_pRightTrashThumbnailList = new ThumbnailListView(COMMON_STR_TRASH);
    m_pRightTrashThumbnailList->setFrameShape(DTableView::NoFrame);

    pMainVBoxLayout->addItem(pTopHBoxLayout);
    pMainVBoxLayout->addWidget(m_pRightTrashThumbnailList);

    pTrashWidget->setLayout(pMainVBoxLayout);

    // Favorite View
    DWidget *pFavoriteWidget = new DWidget();
    QVBoxLayout *pFavoriteVBoxLayout = new QVBoxLayout();

    m_pFavoriteTitle = new DLabel();
    DFontSizeManager::instance()->bind(m_pFavoriteTitle, DFontSizeManager::T3, QFont::Medium);
    m_pFavoriteTitle->setForegroundRole(DPalette::TextTitle);
    m_pFavoriteTitle->setText(tr("Favorites"));

    m_pFavoritePicTotal = new DLabel();
    DFontSizeManager::instance()->bind(m_pFavoritePicTotal, DFontSizeManager::T6, QFont::Medium);
    m_pFavoritePicTotal->setForegroundRole(DPalette::TextTips);
    QString favoriteStr = tr("%1 Photos");

    int favoritePicNum = DBManager::instance()->getImgsCountByAlbum(COMMON_STR_FAVORITES);
    m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(favoritePicNum)));

    m_pRightFavoriteThumbnailList = new ThumbnailListView(COMMON_STR_FAVORITES);
    m_pRightFavoriteThumbnailList->setFrameShape(DTableView::NoFrame);

    pFavoriteVBoxLayout->addSpacing(5);
    pFavoriteVBoxLayout->addWidget(m_pFavoriteTitle);
    pFavoriteVBoxLayout->addSpacing(9);
    pFavoriteVBoxLayout->addWidget(m_pFavoritePicTotal);
    pFavoriteVBoxLayout->addSpacing(-6);

    pFavoriteVBoxLayout->setContentsMargins(10,0,0,0);

    QVBoxLayout *p_all1 = new QVBoxLayout();
    p_all1->addLayout(pFavoriteVBoxLayout);
    p_all1->addWidget(m_pRightFavoriteThumbnailList);

    pFavoriteWidget->setLayout(p_all1);

    //Search View
    m_pSearchView = new SearchView;

    //Spinner View
    DWidget *pSpinnerWidget = new DWidget();
    QHBoxLayout *pSpinnerLayout = new QHBoxLayout();
    m_pSpinner = new DSpinner();
    m_pSpinner->setFixedSize(40, 40);

    pSpinnerLayout->addWidget(m_pSpinner, Qt::AlignCenter);
    pSpinnerWidget->setLayout(pSpinnerLayout);

    // Add View
    m_pRightStackWidget->addWidget(m_pImportView);
    m_pRightStackWidget->addWidget(pNoTrashWidget);
    m_pRightStackWidget->addWidget(pTrashWidget);
    m_pRightStackWidget->addWidget(pFavoriteWidget);
    m_pRightStackWidget->addWidget(m_pSearchView);
    m_pRightStackWidget->addWidget(pSpinnerWidget);

    m_pStatusBar = new StatusBar();
    m_pStatusBar->setParent(this);

    QVBoxLayout* pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins(0,0,0,0);
    pVBoxLayout->addWidget(m_pRightStackWidget);
    pVBoxLayout->addWidget(m_pStatusBar);
    m_pWidget->setLayout(pVBoxLayout);

    if (0 < DBManager::instance()->getImgsCount())
    {
        m_pRightThumbnailList->setFrameShape(DTableView::NoFrame);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
        m_pStatusBar->show();
    }
    else
    {
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
        m_pStatusBar->show();

    }
}

void AlbumView::updateRightView()
{
    if (COMMON_STR_TRASH == m_currentAlbum)
    {
        onTrashListClicked();
        updateRightTrashView();
    }
    else
    {
        updateRightNoTrashView();
    }

    restorePicNum();
}

void AlbumView::updateRightNoTrashView()
{
    m_pSpinner->stop();
    m_curThumbnaiItemList.clear();

    DBImgInfoList infos;

    // 已导入
    if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum)
    {
        infos = DBManager::instance()->getAllInfos();

        for(auto info : infos)
        {
            ThumbnailListView::ItemInfo vi;
            vi.name = info.fileName;
            vi.path = info.filePath;
            vi.image = dApp->m_imagemap.value(info.filePath);

            m_curThumbnaiItemList<<vi;
        }

        m_iAlubmPicsNum = DBManager::instance()->getImgsCount();

        if (0 < m_iAlubmPicsNum)
        {
            m_pRightTitle->setText(tr("Imported"));

            QString str = tr("%1 Photos");
            m_pRightPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));

            m_pRightThumbnailList->insertThumbnails(m_curThumbnaiItemList);
            m_pRightThumbnailList->m_imageType = m_currentAlbum;
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
            m_pStatusBar->show();
        }
        else
        {

            m_pImportView->setAlbumname(QString());
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
            m_pStatusBar->show();
        }

        setAcceptDrops(true);
    }
    else if (COMMON_STR_FAVORITES == m_currentAlbum)    //个人收藏
    {
        infos = DBManager::instance()->getInfosByAlbum(m_currentAlbum);

        for(auto info : infos)
        {
            ThumbnailListView::ItemInfo vi;
            vi.name = info.fileName;
            vi.path = info.filePath;
            vi.image = dApp->m_imagemap.value(info.filePath);

            m_curThumbnaiItemList<<vi;
        }

        m_iAlubmPicsNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);

        QString favoriteStr = tr("%1 Photos");
        m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(m_iAlubmPicsNum)));

        m_pRightFavoriteThumbnailList->insertThumbnails(m_curThumbnaiItemList);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_FAVORITE_LIST);
        setAcceptDrops(false);
    }
    else
    {
        m_importByPhoneWidget->setVisible(false);
        AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());

        qDebug()<<item->m_albumTypeStr;
        if ( ALBUM_PATHTYPE_BY_PHONE == item->m_albumTypeStr || ALBUM_PATHTYPE_BY_U == item->m_albumTypeStr)
        {
            // 手机
            qDebug()<<item->m_albumNameStr;
            qDebug()<<m_phoneNameAndPathlist;
            if (true == m_phoneNameAndPathlist.contains(item->m_albumNameStr))
            {
                updateImportComboBox();
                m_importByPhoneWidget->setVisible(true);

                for(auto path : m_phoneNameAndPathlist.value(item->m_albumNameStr))
                {
                    ThumbnailListView::ItemInfo vi;
                    vi.path = path;
                    vi.image = m_phonePathAndImage.value(path);
                    m_curThumbnaiItemList<<vi;
                }

                m_iAlubmPicsNum = m_curThumbnaiItemList.size();
                m_mountPicNum = m_curThumbnaiItemList.size();

                m_pRightTitle->setText(m_currentAlbum);

                QFontMetrics elideFont(m_pRightTitle->font());
                m_pRightTitle->setText(elideFont.elidedText(m_currentAlbum,Qt::ElideRight, 525));

                QString str = tr("%1 Photos");
                m_pRightPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));

                m_pRightThumbnailList->insertThumbnails(m_curThumbnaiItemList);
                m_pRightThumbnailList->m_imageType = ALBUM_PATHTYPE_BY_PHONE;

                m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
            }
            else
            {
                m_mountPicNum = 0;
                m_pSpinner->start();
                m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_SPINNER);
            }

            setAcceptDrops(false);
        }
        else
        {
            // 新建相册
            infos = DBManager::instance()->getInfosByAlbum(m_currentAlbum);

            for(auto info : infos)
            {
                ThumbnailListView::ItemInfo vi;
                vi.name = info.fileName;
                vi.path = info.filePath;
                vi.image = dApp->m_imagemap.value(info.filePath);

                m_curThumbnaiItemList<<vi;
            }

            m_iAlubmPicsNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);

            if (0 < m_iAlubmPicsNum)
            {
                m_pRightTitle->setText(m_currentAlbum);

                QFontMetrics elideFont(m_pRightTitle->font());
                m_pRightTitle->setText(elideFont.elidedText(m_currentAlbum,Qt::ElideRight, 525));

                QString str = tr("%1 Photos");
                m_pRightPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));

                m_pRightThumbnailList->insertThumbnails(m_curThumbnaiItemList);
                m_pRightThumbnailList->m_imageType = m_currentAlbum;

                m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
                m_pStatusBar->show();
            }
            else {
                m_pImportView->setAlbumname(m_currentAlbum);
                m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
                m_pStatusBar->show();
            }

            setAcceptDrops(true);
        }
    }
}

void AlbumView::updateRightTrashView()
{
    int idaysec = 24*60*60;
    m_curThumbnaiItemList.clear();

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
            vi.remainDays = QString::number(30-Day) + tr("Days");

            m_curThumbnaiItemList<<vi;
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
        m_pDeleteBtn->setText(tr("Delete All"));
        m_pRecoveryBtn->setEnabled(false);
        m_pDeleteBtn->setEnabled(false);
    }

    m_pRightTrashThumbnailList->insertThumbnails(m_curThumbnaiItemList);
}

void AlbumView::leftTabClicked(const QModelIndex &index)
{
    emit dApp->signalM->SearchEditClear();
    //若点击当前的item，则不做任何处理

    if(m_curListWidgetItem == m_pLeftTabList->currentItem())
    {
        SearchReturnUpdate();
        return;
    }

    m_curListWidgetItem = m_pLeftTabList->currentItem();

    AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());

    if (COMMON_STR_TRASH == item->m_albumNameStr)
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

    if (COMMON_STR_RECENT_IMPORTED == item->m_albumNameStr
        || COMMON_STR_TRASH == item->m_albumNameStr
        || COMMON_STR_FAVORITES == item->m_albumNameStr
        || ALBUM_PATHTYPE_BY_PHONE == item->m_albumTypeStr
        || ALBUM_PATHTYPE_BY_U == item->m_albumTypeStr)
    {
        return;
    }

    m_pLeftMenu->setVisible(true);
    foreach (QAction* action , m_MenuActionMap.values()) {
        action->setVisible(true);
    }

    if (0 == DBManager::instance()->getImgsCountByAlbum(item->m_albumNameStr))
    {
        m_MenuActionMap.value(tr("Slide show"))->setVisible(false);
    }

    if (0 == DBManager::instance()->getImgsCountByAlbum(item->m_albumNameStr))
    {
        m_MenuActionMap.value(tr("Export album"))->setVisible(false);
    }
    m_pLeftMenu->popup(QCursor::pos());
}

void AlbumView::appendAction(int id, const QString &text, const QString &shortcut)
{
    QAction *ac = new QAction(m_pLeftMenu);
    addAction(ac);
    ac->setText(text);
    ac->setProperty("MenuID", id);
    ac->setShortcut(QKeySequence(shortcut));
    m_MenuActionMap.insert(text, ac);
    m_pLeftMenu->addAction(ac);
}

void AlbumView::onLeftMenuClicked(QAction *action)
{
    const int id = action->property("MenuID").toInt();
    switch (MenuItemId(id))
    {
    case IdStartSlideShow:
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
        break;
    case IdCreateAlbum:
    {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem();
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));

        m_pLeftTabList->insertItem(m_pLeftTabList->currentRow()+1, pListWidgetItem);
        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(getNewAlbumName(), m_pLeftTabList, pListWidgetItem);


        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);

        m_pLeftTabList->setCurrentRow(m_pLeftTabList->currentRow()+1);


        AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
        item->m_opeMode = OPE_MODE_ADDNEWALBUM;
        item->editAlbumEdit();

        m_curListWidgetItem = m_pLeftTabList->currentItem();

        m_currentAlbum = item->m_albumNameStr;
        updateRightView();
    }
        break;
    case IdRenameAlbum:
    {
        AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());

        if (COMMON_STR_RECENT_IMPORTED == item->m_albumNameStr
            || COMMON_STR_TRASH == item->m_albumNameStr
            || COMMON_STR_FAVORITES == item->m_albumNameStr
            || ALBUM_PATHTYPE_BY_PHONE == item->m_albumTypeStr
            || ALBUM_PATHTYPE_BY_U == item->m_albumTypeStr)
        {
            return;
        }

        item->m_opeMode = OPE_MODE_RENAMEALBUM;
        item->editAlbumEdit();
    }
        break;
    case IdExport:
    {
        QListWidgetItem *item = m_pLeftTabList->currentItem();
        AlbumLeftTabItem *pTabItem = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(item);
        Exporter::instance()->exportAlbum(DBManager::instance()->getPathsByAlbum(pTabItem->m_albumNameStr), pTabItem->m_albumNameStr);
    }
        break;
    case IdDeleteAlbum:
    {
//        AlbumDeleteDialog *dialog = new AlbumDeleteDialog();
//        dialog->showInCenter(window());

//        connect(dialog,&AlbumDeleteDialog::deleteAlbum,this,[=]{
            QString str;
            QListWidgetItem *item = m_pLeftTabList->currentItem();
            AlbumLeftTabItem *pTabItem = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(item);

        str = pTabItem->m_albumNameStr;
        DBManager::instance()->removeAlbum(pTabItem->m_albumNameStr);
        delete  item;

        QModelIndex index;
        emit m_pLeftTabList->clicked(index);

//        QString str1 = "相册：“%1”，已经删除成功";
//        DUtil::DNotifySender *pDNotifySender = new DUtil::DNotifySender("深度相册");
//        pDNotifySender->appName("deepin-album");
//        pDNotifySender->appBody(str1.arg(str));
//        pDNotifySender->call();
//        });

        emit dApp->signalM->sigAlbDelToast(str);

        AlbumLeftTabItem *currentItem = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
        m_currentAlbum = currentItem->m_albumNameStr;

        updateRightView();
    }
        break;
    default:
        break;
    }
}

void AlbumView::createNewAlbum(QStringList imagepaths)
{
    //新建相册需要在外接设备节点上面，此处调用getNewAlbumItemIndex函数，获取新建相册的index
    int index = getNewAlbumItemIndex();

    QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftTabList);
    pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
    QString albumName = getNewAlbumName();

    if (QStringList(" ") != imagepaths)
    {
        DBManager::instance()->insertIntoAlbum(albumName, imagepaths);
    }

    AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName, m_pLeftTabList, pListWidgetItem);
    m_customAlbumNames << albumName;

    m_pLeftTabList->insertItem(index, pListWidgetItem);
    m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);

    m_pLeftTabList->setCurrentRow(index);

    AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
    if (QStringList(" ") != imagepaths)
    {
        item->m_opeMode = OPE_MODE_ADDRENAMEALBUM;
    }
    else
    {
        item->m_opeMode = OPE_MODE_ADDNEWALBUM;
    }

    item->editAlbumEdit();

    m_curListWidgetItem = m_pLeftTabList->currentItem();
    m_currentAlbum = albumName;

    updateRightView();
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

    onTrashListClicked();
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
//        m_pDeleteBtn->setEnabled(false);
    }
    ImgDeleteDialog *dialog = new ImgDeleteDialog(paths.count());
    dialog->show();
    connect(dialog,&ImgDeleteDialog::imgdelete,this,[=]{
        for(auto path : paths)
        {
            dApp->m_imagetrashmap.remove(path);
        }

        DBManager::instance()->removeTrashImgInfos(paths);
    });

    onTrashListClicked();
}

void AlbumView::openImage(int index)
{
    SignalManager::ViewInfo info;
    info.album = "";
    info.lastPanel = nullptr;

    if(m_curThumbnaiItemList.size()>1){
        for(auto image : m_curThumbnaiItemList)
        {
            info.paths<<image.path;
        }
    }else {
      info.paths.clear();
     }
    info.path = m_curThumbnaiItemList[index].path;
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
    if (COMMON_STR_TRASH == m_currentAlbum)
    {
        imagelist = DBManager::instance()->getAllTrashInfos();
    }
    else if(COMMON_STR_RECENT_IMPORTED == m_currentAlbum)
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
    info.viewMainWindowID = VIEW_MAINWINDOW_ALBUM;
    if(info.slideShow)
    {
        if(imagelist.count() == 1)
        {
            info.paths = paths;
        }
        emit dApp->signalM->startSlideShow(info);
    }
    else {
        emit dApp->signalM->viewImage(info);
        emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALBUM);
    }
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
        DPalette ReBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
        ReBtn.setBrush(DPalette::Highlight, QColor(0,0,0,0));
        m_pRecoveryBtn->setPalette(ReBtn);

       m_pDeleteBtn->setText(tr("Delete"));
    }
    else
    {
        m_pRecoveryBtn->setEnabled(false);
        DPalette ReBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
        ReBtn.setBrush(DPalette::Highlight, QColor(0,0,0,0));
        m_pRecoveryBtn->setPalette(ReBtn);

        m_pDeleteBtn->setText(tr("Delete All"));
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
        dApp->m_imageloader->ImportImageLoader(dbInfos, m_currentAlbum);
    }
    else
    {
        emit dApp->signalM->ImportFailed();
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

void AlbumView::keyPressEvent(QKeyEvent *event)
{
    QWidget::keyPressEvent(event);
    if(event->key() == Qt::Key_Delete){
        QStringList paths = m_pRightThumbnailList->selectedPaths();

        if (0 == paths.length())
        {
            if (COMMON_STR_RECENT_IMPORTED != m_currentAlbum
                && COMMON_STR_TRASH != m_currentAlbum
                && COMMON_STR_FAVORITES != m_currentAlbum
                && ALBUM_PATHTYPE_BY_PHONE != m_pRightThumbnailList->m_imageType)
            {
                QString str;
                QListWidgetItem *item = m_pLeftTabList->currentItem();
                AlbumLeftTabItem *pTabItem = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(item);

                str = pTabItem->m_albumNameStr;
                DBManager::instance()->removeAlbum(pTabItem->m_albumNameStr);
                delete  item;

                QModelIndex index;
                emit m_pLeftTabList->clicked(index);

                emit dApp->signalM->sigAlbDelToast(str);

                AlbumLeftTabItem *currentItem = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
                m_currentAlbum = currentItem->m_albumNameStr;

                updateRightView();
            }
        }
#if 1
    }else if(event->key() == Qt::Key_F2){
        AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
        if (COMMON_STR_RECENT_IMPORTED == item->m_albumNameStr
            || COMMON_STR_TRASH == item->m_albumNameStr
            || COMMON_STR_FAVORITES == item->m_albumNameStr
            || ALBUM_PATHTYPE_BY_PHONE == item->m_albumTypeStr
            || ALBUM_PATHTYPE_BY_U == item->m_albumTypeStr)
        {
            return;
        }
        item->m_opeMode = OPE_MODE_RENAMEALBUM;
        item->editAlbumEdit();
    }
#endif
}

void AlbumView::onVfsMountChangedAdd(QExplicitlySharedDataPointer<DGioMount> mount)
{
    qDebug()<<"onVfsMountChangedAdd()"<<mount->name();
    Q_UNUSED(mount);

    QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
    QString strPath = LocationFile->path();
    if (strPath.isEmpty())
    {
        qDebug()<<"onVfsMountChangedAdd() strPath.isEmpty()";
        return;
    }

    if (m_loadMountMap.keys().contains(strPath)) return;

    m_loadMountMap.insert(mount->getDefaultLocationFile()->path(), 0);

    if (0 == m_loadMountFlag)
    {
        m_loadMountFlag = 1;
        qDebug()<<"onVfsMountChangedAdd() emit dApp->sigLoadMountImagesStart()"<<mount->name();
        emit sigLoadMountImagesStart(mount->name(), strPath);
    }

    updateExternalDevice(mount);
}

void AlbumView::onVfsMountChangedRemove(QExplicitlySharedDataPointer<DGioMount> mount)
{
    qDebug()<<"onVfsMountChangedRemove() mountname"<<mount->name();
    Q_UNUSED(mount);

    m_loadMountMap.remove(mount->getDefaultLocationFile()->path());
    m_mounts.removeOne(mount);

    for(int i = 0; i < m_pLeftTabList->count(); i++)
    {
        QListWidgetItem *pListWidgetItem = m_pLeftTabList->item(i);
        AlbumLeftTabItem *pAlbumLeftTabItem = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(pListWidgetItem);
        if (mount->name() == pAlbumLeftTabItem->m_albumNameStr)
        {
            delete pListWidgetItem;
            AlbumLeftTabItem *currentItem = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
            m_currentAlbum = currentItem->m_albumNameStr;

            updateRightView();

            break;
        }
    }
}

const QList<QExplicitlySharedDataPointer<DGioMount> > AlbumView::getVfsMountList()
{
    QList<QExplicitlySharedDataPointer<DGioMount> > result;
    const QList<QExplicitlySharedDataPointer<DGioMount> > mounts = m_vfsManager->getMounts();

    for (auto mount : mounts) {
        qDebug()<<"getVfsMountList() mount.name"<<mount->name();
        result.append(mount);
    }

    return result;
}

void AlbumView::loadMountPicture(QString path)
{
    //判断路径是否存在
    QDir dir(path);
    if (!dir.exists()) return;

    //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
    if(!path.contains("/media/")) {
        bool bFind = findPicturePathByPhone(path);
        if(!bFind) return;
    }

    //获取所选文件类型过滤器
    QStringList filters;
    filters << QString("*.jpeg") << QString("*.jpg");

    //定义迭代器并设置过滤器
    QDirIterator dir_iterator(path,
                              filters,
                              QDir::Files | QDir::NoSymLinks,
                              QDirIterator::Subdirectories);
    QStringList string_list;
    while (dir_iterator.hasNext()) {
        dir_iterator.next();
        QFileInfo fileInfo = dir_iterator.fileInfo();

        QImage tImg;

        QString format = DetectImageFormat(fileInfo.filePath());
        if (format.isEmpty()) {
            QImageReader reader(fileInfo.filePath());
            reader.setAutoTransform(true);
            if (reader.canRead()) {
                tImg = reader.read();
            }
            else if (path.contains(".tga")) {
                bool ret = false;
                tImg = utils::image::loadTga(path, ret);
            }
        } else {
            QImageReader readerF(fileInfo.filePath(), format.toLatin1());
            readerF.setAutoTransform(true);
            if (readerF.canRead()) {
                tImg = readerF.read();
            } else {
                qWarning() << "can't read image:" << readerF.errorString()
                           << format;

                tImg = QImage(fileInfo.filePath());
            }
        }
        QPixmap pixmap = QPixmap::fromImage(tImg);

        m_phonePicMap.insert(fileInfo.filePath(), pixmap.scaledToHeight(100,  Qt::FastTransformation));
    }
}

void AlbumView::initLeftMenu()
{
    m_MenuActionMap.clear();


    appendAction(IdStartSlideShow, tr("Slide show"), ss(""));
    m_pLeftMenu->addSeparator();

    appendAction(IdCreateAlbum, tr("Creat album"), ss(""));
    m_pLeftMenu->addSeparator();

    appendAction(IdRenameAlbum, tr("Rename album"), ss("COMMON_STR_RENAMEALBUM"));
    m_pLeftMenu->addSeparator();

    appendAction(IdExport, tr("Export album"), ss(""));
    m_pLeftMenu->addSeparator();

    appendAction(IdDeleteAlbum, tr("Remove album"), ss(""));
}

bool AlbumView::findPictureFile(QString &path, QList<ThumbnailListView::ItemInfo>& thumbnaiItemList)
{
    //判断路径是否存在
    QDir dir(path);
    if (!dir.exists()) return false;

    //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
    if(!path.contains("/media/")) {
        bool bFind = findPicturePathByPhone(path);
        if(!bFind) return  false;
    }

    //获取所选文件类型过滤器
    QStringList filters;
    filters << QString("*.jpeg") << QString("*.jpg");

    //定义迭代器并设置过滤器
    QDirIterator dir_iterator(path,
                              filters,
                              QDir::Files | QDir::NoSymLinks,
                              QDirIterator::Subdirectories);
    QStringList string_list;
    while (dir_iterator.hasNext()) {
        dir_iterator.next();
        QFileInfo fileInfo = dir_iterator.fileInfo();
        ThumbnailListView::ItemInfo vi;
        vi.name = fileInfo.fileName();
        vi.path = fileInfo.filePath();
        vi.image = m_phonePicMap.value(fileInfo.filePath());
        thumbnaiItemList << vi;
    }

    return true;
}

void AlbumView::initExternalDevice()
{
    for(auto mount : m_mounts)
    {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftTabList);
        //pListWidgetItem缓存文件挂载路径
        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
        QString strPath = LocationFile->path();
        pListWidgetItem->setData(Qt::UserRole, strPath);
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
        AlbumLeftTabItem *pAlbumLeftTabItem;
        if (strPath.contains("/media/"))
        {
            pAlbumLeftTabItem = new AlbumLeftTabItem(mount->name(), m_pLeftTabList, pListWidgetItem, ALBUM_PATHTYPE_BY_U);
        }
        else
        {
            pAlbumLeftTabItem = new AlbumLeftTabItem(mount->name(), m_pLeftTabList, pListWidgetItem, ALBUM_PATHTYPE_BY_PHONE);
        }

        pAlbumLeftTabItem->setExternalDevicesMountPath(strPath);
        connect(pAlbumLeftTabItem, &AlbumLeftTabItem::unMountExternalDevices, this, &AlbumView::onUnMountSignal);
        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);

        m_loadMountMap.insert(mount->getDefaultLocationFile()->path(), 0);
        if (0 == m_loadMountFlag)
        {
            m_loadMountFlag = 1;
            qDebug()<<"emit dApp->sigLoadMountImagesStart()"<<mount->name() << strPath;
            emit sigLoadMountImagesStart(mount->name(), strPath);
        }
    }
}

void AlbumView::updateExternalDevice(QExplicitlySharedDataPointer<DGioMount> mount)
{
    QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftTabList);
    //pListWidgetItem缓存文件挂载路径
    QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
    QString strPath = LocationFile->path();
    pListWidgetItem->setData(Qt::UserRole, strPath);
    pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));

    AlbumLeftTabItem *pAlbumLeftTabItem;
    if (strPath.contains("/media/"))
    {
        pAlbumLeftTabItem = new AlbumLeftTabItem(mount->name(), m_pLeftTabList, pListWidgetItem, ALBUM_PATHTYPE_BY_U);
    }
    else
    {
        pAlbumLeftTabItem = new AlbumLeftTabItem(mount->name(), m_pLeftTabList, pListWidgetItem, ALBUM_PATHTYPE_BY_PHONE);
    }

    pAlbumLeftTabItem->setExternalDevicesMountPath(strPath);
    connect(pAlbumLeftTabItem, &AlbumLeftTabItem::unMountExternalDevices, this, &AlbumView::onUnMountSignal);
    m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    m_mounts.append(mount);
}

void AlbumView::onUpdataAlbumRightTitle(QString titlename)
{
    m_currentAlbum = titlename;
    updateRightView();
}

void AlbumView::SearchReturnUpdate()
{
    if (RIGHT_VIEW_SEARCH == m_pRightStackWidget->currentIndex())
    {
        if (COMMON_STR_TRASH == m_currentAlbum)
        {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TRASH_LIST);
        }
        else if (COMMON_STR_FAVORITES == m_currentAlbum)
        {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_FAVORITE_LIST);
        }
        else
        {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
        }
    }
}

//搜索手机中存储相机照片文件的路径，采用两级文件目录深度，找"DCIM"文件目录
//经过调研，安卓手机在path/外部存储设备/DCIM下，iPhone在patn/DCIM下
bool AlbumView::findPicturePathByPhone(QString &path)
{
    QDir dir(path);
    if (!dir.exists()) return false;

    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfo tempFileInfo;
    foreach (tempFileInfo, fileInfoList) {
        if (tempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0)
        {
            path = tempFileInfo.absoluteFilePath();
            return true;
        } else {
            QDir subDir;
            subDir.setPath(tempFileInfo.absoluteFilePath());

            QFileInfoList subFileInfoList = subDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            QFileInfo subTempFileInfo;
            foreach (subTempFileInfo, subFileInfoList) {
                if (subTempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0)
                {
                    path = subTempFileInfo.absoluteFilePath();
                    return true;
                }
            }
            return false;
        }
    }

    return false;
}

void AlbumView::updateImportComboBox()
{
    m_importByPhoneComboBox->clear();
    m_importByPhoneComboBox->addItems(m_customAlbumNames);
}

//手机照片全部导入
void AlbumView::importAllBtnClicked()
{
    QList<ThumbnailListView::ItemInfo> allPaths = m_pRightThumbnailList->getAllPaths();
    QString albumName = m_importByPhoneComboBox->currentText();
    QStringList picPathList;
    DBImgInfoList dbInfos;
    QString strHomePath = QDir::homePath();
    //获取系统现在的时间
    QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
    QDir dir;
    if (!dir.exists(basePath)) {
        dir.mkpath(basePath);
    }

    foreach (ThumbnailListView::ItemInfo info , allPaths) {
        QString strPath = info.path;
        QString strNewPath = QString("%1%2%3").arg(basePath, "/", info.name);

        //判断新路径下是否存在目标文件，若存在，先删除掉
        if (dir.exists(strNewPath)) {
            dir.remove(strNewPath);
        }

        if (QFile::copy(strPath, strNewPath)) {
            picPathList << strNewPath;

            QFileInfo fi(strNewPath);
            DBImgInfo dbi;
            dbi.fileName = fi.fileName();
            dbi.filePath = strNewPath;
            dbi.dirHash = utils::base::hash(QString());
            dbi.time = fi.birthTime();

            dbInfos << dbi;
        }
    }

    if (!picPathList.isEmpty()) {
        DBManager::instance()->insertIntoAlbum(albumName, picPathList);
    }

    if (! dbInfos.isEmpty())
    {
        DBManager::instance()->insertImgInfos(dbInfos);
    }
}

//手机照片导入选中
void AlbumView::importSelectBtnClicked()
{
    QStringList selectPaths = m_pRightThumbnailList->selectedPaths();
    QString albumName = m_importByPhoneComboBox->currentText();
    QStringList picPathList;
    DBImgInfoList dbInfos;
    QString strHomePath = QDir::homePath();
    //获取系统现在的时间
    QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
    QDir dir;
    if (!dir.exists(basePath)) {
        dir.mkpath(basePath);
    }

    foreach (QString path , selectPaths) {
        //取出文件名称
        QStringList pathList = path.split("/", QString::SkipEmptyParts);
        QString strNewPath = QString("%1%2%3").arg(basePath, "/", pathList.last());

        //判断新路径下是否存在目标文件，若存在，先删除掉
        if (dir.exists(strNewPath)) {
            dir.remove(strNewPath);
        }

        if (QFile::copy(path, strNewPath)) {
            picPathList << strNewPath;

            QFileInfo fi(strNewPath);
            DBImgInfo dbi;
            dbi.fileName = fi.fileName();
            dbi.filePath = strNewPath;
            dbi.dirHash = utils::base::hash(QString());
            dbi.time = fi.birthTime();

            dbInfos << dbi;
        }
    }

    if (!picPathList.isEmpty()) {
        DBManager::instance()->insertIntoAlbum(albumName, picPathList);
    }

    if (!dbInfos.isEmpty()) {
        DBManager::instance()->insertImgInfos(dbInfos);
    }
}

int AlbumView::getNewAlbumItemIndex()
{
    int count = m_pLeftTabList->count();
    for(int i = 0; i < count; ++i) {
        QString strPath = m_pLeftTabList->item(i)->data(Qt::UserRole).toString();
        if(!strPath.isEmpty())
        {
            return i;
        }
    }
    return count;
}

//卸载外部设备
void AlbumView::onUnMountSignal(QString unMountPath)
{
    for (auto mount : m_mounts) {
        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
        if(LocationFile->path().compare(unMountPath) == 0 && mount->canUnmount()) {
            mount->unmount(true);
            m_mounts.removeOne(mount);
            break;
        }
    }
}

void AlbumView::onLoadMountImagesEnd(QString mountname)
{
    qDebug()<<"onLoadMountImagesEnd() mountname"<<mountname;
    qDebug()<<m_phoneNameAndPathlist;
    for(auto path : m_loadMountMap.keys())
    {
        QStringList pathList = path.split("/");
        if (pathList.last() == mountname)
        {
            m_loadMountMap[path] = 1;
            qDebug()<<"onLoadMountImagesEnd() updateRightView()";
            updateRightView();
            break;
        }
    }

    int iloadEndFlag = 0;

    for(auto path : m_loadMountMap.keys())
    {
        if(0 == m_loadMountMap.value(path))
        {
            if (path.isEmpty())
            {
                continue;
            }
            else
            {
                iloadEndFlag = 1;
                m_loadMountMap.insert(path, 0);
                QStringList pathList = path.split("/");

                qDebug()<<"onLoadMountImagesEnd() emit dApp->sigLoadMountImagesStart()";
                emit sigLoadMountImagesStart(pathList.last(), path);
                break;
            }
        }
    }

    if (0 == iloadEndFlag)
    {
        qDebug()<<"onLoadMountImagesEnd() m_loadMountFlag = 0";
        m_loadMountFlag = 0;
    }
}

void AlbumView::updatePicNum()
{
    QString str = tr("Selected %1 photos");
    int selPicNum = 0;

    if(4 == m_pRightStackWidget->currentIndex())
    {
        QStringList paths = m_pSearchView->m_pThumbnailListView->selectedPaths();
        selPicNum = paths.length();
    }
    else
    {
        if(m_currentAlbum == COMMON_STR_TRASH)
        {
            QStringList paths = m_pRightTrashThumbnailList->selectedPaths();
            selPicNum = paths.length();
        }
        else if(m_currentAlbum == COMMON_STR_FAVORITES)
        {
            QStringList paths = m_pRightFavoriteThumbnailList->selectedPaths();
            selPicNum = paths.length();
        }
        else
        {
            QStringList paths = m_pRightThumbnailList->selectedPaths();
            selPicNum = paths.length();
        }
    }

    if (0 < selPicNum)
    {
        m_pStatusBar->m_pAllPicNumLabel->setText(str.arg(QString::number(selPicNum)));
    }
    else
    {
        restorePicNum();
    }
}

void AlbumView::restorePicNum()
{
    QString str = tr("%1 photos");
    int selPicNum = 0;

    if(4 == m_pRightStackWidget->currentIndex())
    {
        selPicNum = m_pSearchView->m_searchPicNum;
    }
    else
    {
        if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum)
        {
            selPicNum = DBManager::instance()->getImgsCount();
        }
        else if (COMMON_STR_TRASH == m_currentAlbum)
        {
            selPicNum = DBManager::instance()->getTrashImgsCount();
        }
        else if (COMMON_STR_FAVORITES == m_currentAlbum)
        {
            selPicNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);
        }
        else
        {
            if (ALBUM_PATHTYPE_BY_PHONE == m_pRightThumbnailList->m_imageType)
            {
                selPicNum = m_mountPicNum;
            }
            else
            {
                selPicNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);
            }
        }
    }

    m_pStatusBar->m_pAllPicNumLabel->setText(str.arg(QString::number(selPicNum)));
}

MountLoader::MountLoader(AlbumView* parent)
{
    m_parent = parent;
}

void MountLoader::onLoadMountImagesStart(QString mountName, QString path)
{
    qDebug()<<"onLoadMountImagesStart()";
    //判断路径是否存在
    QDir dir(path);
    if (!dir.exists())
    {
        qDebug()<<"onLoadMountImagesStart() !dir.exists()";
        dApp->signalM->sigLoadMountImagesEnd(mountName);
        return;
    }

    //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
    if(!path.contains("/media/"))
    {
        bool bFind = findPicturePathByPhone(path);
        if(!bFind)
        {
            qDebug()<<"onLoadMountImagesStart() !bFind";
            dApp->signalM->sigLoadMountImagesEnd(mountName);
            return;
        }
    }

    //获取所选文件类型过滤器
    QStringList filters;
    filters << QString("*.jpeg") << QString("*.jpg");

    //定义迭代器并设置过滤器
    QDirIterator dir_iterator(path,
                              filters,
                              QDir::Files | QDir::NoSymLinks,
                              QDirIterator::Subdirectories);

    m_phoneImgPathList.clear();

    while (dir_iterator.hasNext()) {
        dir_iterator.next();
        QFileInfo fileInfo = dir_iterator.fileInfo();

        QImage tImg;

        QString format = DetectImageFormat(fileInfo.filePath());
        if (format.isEmpty()) {
            QImageReader reader(fileInfo.filePath());
            reader.setAutoTransform(true);
            if (reader.canRead()) {
                tImg = reader.read();
            }
            else if (path.contains(".tga")) {
                bool ret = false;
                tImg = utils::image::loadTga(path, ret);
            }
        } else {
            QImageReader readerF(fileInfo.filePath(), format.toLatin1());
            readerF.setAutoTransform(true);
            if (readerF.canRead()) {
                tImg = readerF.read();
            } else {
                qWarning() << "can't read image:" << readerF.errorString()
                           << format;

                tImg = QImage(fileInfo.filePath());
            }
        }

        QPixmap pixmap = QPixmap::fromImage(tImg);
        pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
        if(800 < pixmap.width())
        {
            pixmap = pixmap.scaledToWidth(800,  Qt::FastTransformation);
        }

        m_parent->m_phonePathAndImage.insert(fileInfo.filePath(), pixmap);

        m_phoneImgPathList<<fileInfo.filePath();
    }

    qDebug()<<"onLoadMountImagesStart() m_phoneImgPathList.length()"<<m_phoneImgPathList.length();
    if (0 < m_phoneImgPathList.length())
    {
        m_parent->m_phoneNameAndPathlist.insert(mountName, m_phoneImgPathList);
    }

    dApp->signalM->sigLoadMountImagesEnd(mountName);
}

//搜索手机中存储相机照片文件的路径，采用两级文件目录深度，找"DCIM"文件目录
//经过调研，安卓手机在path/外部存储设备/DCIM下，iPhone在patn/DCIM下
bool MountLoader::findPicturePathByPhone(QString &path)
{
    QDir dir(path);
    if (!dir.exists()) return false;

    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfo tempFileInfo;
    foreach (tempFileInfo, fileInfoList) {
        if (tempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0)
        {
            path = tempFileInfo.absoluteFilePath();
            return true;
        } else {
            QDir subDir;
            subDir.setPath(tempFileInfo.absoluteFilePath());

            QFileInfoList subFileInfoList = subDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            QFileInfo subTempFileInfo;
            foreach (subTempFileInfo, subFileInfoList) {
                if (subTempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0)
                {
                    path = subTempFileInfo.absoluteFilePath();
                    return true;
                }
            }
            return false;
        }
    }

    return false;
}

