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

namespace {
const int ITEM_SPACING = 0;
const int LEFT_VIEW_WIDTH = 180;
const int LEFT_VIEW_LISTITEM_WIDTH = 160;
const int LEFT_VIEW_LISTITEM_HEIGHT = 40;
const int OPE_MODE_ADDNEWALBUM = 0;
const int OPE_MODE_RENAMEALBUM = 1;
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

    setAcceptDrops(true);
    initLeftView();
    initRightView();
    initLeftMenu();

    QHBoxLayout *pLayout = new QHBoxLayout();
    pLayout->setContentsMargins(0,0,0,0);
    pLayout->addWidget(m_pLeftWidget);
//    pLayout->addWidget(m_pRightStackWidget);
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
    connect(m_pLeftTabList, &DListWidget::pressed, this, &AlbumView::leftTabClicked);
    connect(dApp->signalM, &SignalManager::sigCreateNewAlbumFromDialog, this, &AlbumView::onCreateNewAlbumFromDialog);
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
    connect(dApp->signalM, &SignalManager::sigPixMapRotate, this, &AlbumView::onPixMapRotate);
    connect(m_vfsManager, &DGioVolumeManager::mountAdded, this, &AlbumView::onVfsMountChangedAdd);
    connect(m_vfsManager, &DGioVolumeManager::mountRemoved, this, &AlbumView::onVfsMountChangedRemove);

    connect(m_importAllByPhoneBtn, &DPushButton::clicked, this, &AlbumView::importAllBtnClicked);
    connect(m_importSelectByPhoneBtn, &DPushButton::clicked, this, &AlbumView::importSelectBtnClicked);
    connect(m_pStatusBar->m_pSlider, &DSlider::valueChanged, dApp->signalM, &SignalManager::sigMainwindowSliderValueChg);

    connect(dApp->signalM, &SignalManager::sigTrashViewBlankArea, this, [=]{
        m_pRecoveryBtn->setEnabled(false);
        DPalette pal = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
        pal.setBrush(DPalette::Light, QColor(100,100,100));
        pal.setBrush(DPalette::Dark, QColor(92,92,92));
        pal.setBrush(DPalette::ButtonText, pal.color(DPalette::HighlightedText));
        m_pRecoveryBtn->setPalette(pal);

        m_pDeleteBtn->setText(BUTTON_STR_DETELEALL);
            });
    connect(dApp->signalM, &SignalManager::sigBoxToChoose, this, &AlbumView::onTrashListClicked);
    connect(dApp->signalM, &SignalManager::sigLoadMountImagesEnd, this, &AlbumView::onLoadMountImagesEnd);
}

void AlbumView::initLeftView()
{
    m_pLeftTabList = new DListWidget();

    m_pLeftTabList->setFixedWidth(162);
    m_pLeftTabList->setSpacing(ITEM_SPACING);
    m_pLeftTabList->setContextMenuPolicy(Qt::CustomContextMenu);
    m_pLeftTabList->setFrameShape(DTableView::NoFrame);
//    m_pLeftTabList->setFocusPolicy(Qt::NoFocus);
//    m_pLeftTabList->setBackgroundRole(DPalette::Base);

    m_pLeftWidget = new DWidget();
    m_pLeftWidget->setFixedWidth(LEFT_VIEW_WIDTH);

//    DPalette palcor = DApplicationHelper::instance()->palette(m_pLeftWidget);
//    palcor.setBrush(DPalette::Background, palcor.color(DPalette::Base));
//    m_pLeftWidget->setAutoFillBackground(true);
//    m_pLeftWidget->setPalette(palcor);

    m_pLeftWidget->setBackgroundRole(DPalette::Base);
    m_pLeftWidget->setAutoFillBackground(true);

//    DPalette pa;
//    pa.setColor(DPalette::Background,QColor(255, 255, 255));
//    m_pLeftWidget->setAutoFillBackground(true);
//    m_pLeftWidget->setPalette(pa);

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
        if (COMMON_STR_FAVORITES == albumName)
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

        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName);
        pAlbumLeftTabItem->setFixedWidth(LEFT_VIEW_LISTITEM_WIDTH);
        pAlbumLeftTabItem->setFixedHeight(LEFT_VIEW_LISTITEM_HEIGHT);
//        if (COMMON_STR_RECENT_IMPORTED == albumName)
//        {
//            pListWidgetItem->setSelected(true);
//        }
        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    }

    m_pLeftTabList->setCurrentRow(0);
    //init externalDevice
    m_mounts = getVfsMountList();
    updateExternalDevice();

    AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->item(0));
    item->newAlbumStatus();
}

void AlbumView::updateLeftView()
{
    int row = m_pLeftTabList->currentRow();
    m_pLeftTabList->clear();
    m_allAlbumNames.clear();
    m_customAlbumNames.clear();

    m_allAlbumNames<<COMMON_STR_RECENT_IMPORTED;
    m_allAlbumNames<<COMMON_STR_TRASH;
    m_allAlbumNames<<COMMON_STR_FAVORITES;

    QStringList allAlbumNames = DBManager::instance()->getAllAlbumNames();
    for(auto albumName : allAlbumNames)
    {
        if (COMMON_STR_FAVORITES == albumName)
        {
            continue;
        }

        m_allAlbumNames<<albumName;
        m_customAlbumNames << albumName;
    }

    for(int i = 0; i < m_allAlbumNames.length(); i++)
    {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftTabList);
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(m_allAlbumNames[i]);
        pAlbumLeftTabItem->oriAlbumStatus();
        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    }

    m_pLeftTabList->setCurrentRow(row);
    AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->item(row));
    item->newAlbumStatus();

    m_mounts = getVfsMountList();
    updateExternalDevice();
    updateRightNoTrashView();
}

void AlbumView::onCreateNewAlbumFromDialog()
{
    m_pLeftTabList->clear();
    m_allAlbumNames.clear();
    m_customAlbumNames.clear();

    m_allAlbumNames<<COMMON_STR_RECENT_IMPORTED;
    m_allAlbumNames<<COMMON_STR_TRASH;
    m_allAlbumNames<<COMMON_STR_FAVORITES;

    QStringList allAlbumNames = DBManager::instance()->getAllAlbumNames();
    for(auto albumName : allAlbumNames)
    {
        if (COMMON_STR_FAVORITES == albumName)
        {
            continue;
        }

        m_allAlbumNames<<albumName;
        m_customAlbumNames << albumName;
    }

    for(int i = 0; i < m_allAlbumNames.length(); i++)
    {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftTabList);
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(m_allAlbumNames[i]);
        pAlbumLeftTabItem->oriAlbumStatus();

        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    }

    m_pLeftTabList->setCurrentRow((m_allAlbumNames.length() - 1));
    AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->item((m_allAlbumNames.length() - 1)));
    item->newAlbumStatus();

    m_mounts = getVfsMountList();
    updateExternalDevice();
    updateRightNoTrashView();
}

void AlbumView::initRightView()
{
    m_pRightStackWidget = new DStackedWidget();
    m_pWidget = new DWidget();

    // Import View
    m_pImportView = new ImportView();

    // Thumbnail View
    DWidget *pNoTrashWidget = new DWidget();
    pNoTrashWidget->setBackgroundRole(DPalette::Window);
//    pNoTrashWidget->setAutoFillBackground(true);
    QVBoxLayout *pNoTrashVBoxLayout = new QVBoxLayout();
    pNoTrashVBoxLayout->setContentsMargins(0,0,0,0);

    m_pRightTitle = new DLabel();
    m_pRightTitle->setText(tr(COMMON_STR_RECENT_IMPORTED));

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


    m_pRightThumbnailList = new ThumbnailListView(COMMON_STR_RECENT_IMPORTED);
    m_pRightThumbnailList->setFrameShape(DTableView::NoFrame);

    pNoTrashVBoxLayout->addSpacing(5);
    pNoTrashVBoxLayout->addWidget(m_pRightTitle);
    pNoTrashVBoxLayout->addSpacing(9);
    pNoTrashVBoxLayout->addWidget(m_pRightPicTotal);
    pNoTrashVBoxLayout->addSpacing(7);
    pNoTrashVBoxLayout->setContentsMargins(10,0,0,0);

    //手机相片导入窗体
    m_importByPhoneWidget = new DWidget;
    QHBoxLayout *mainImportLayout = new QHBoxLayout;
    DLabel *importLabel = new DLabel();
    importLabel->setText(tr("导入到："));
    m_importByPhoneComboBox = new DComboBox;
    m_importAllByPhoneBtn = new DPushButton(tr("全部导入"));
    m_importSelectByPhoneBtn = new DPushButton(tr("导入所选"));
    mainImportLayout->addWidget(importLabel, 2);
    mainImportLayout->addWidget(m_importByPhoneComboBox, 6);
    mainImportLayout->addWidget(m_importAllByPhoneBtn, 3);
    mainImportLayout->addWidget(m_importSelectByPhoneBtn, 3);
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
    pal.setBrush(DPalette::Dark, QColor(92,92,92));
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
    dpa.setBrush(DPalette::Dark, QColor(0,152,255));
    dpa.setBrush(DPalette::ButtonText, dpa.color(DPalette::HighlightedText));
    m_pDeleteBtn->setPalette(dpa);

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
    m_pFavoriteTitle->setText(COMMON_STR_FAVORITES);
    m_pFavoriteTitle->setFont(ft);
    m_pFavoriteTitle->setPalette(pa);

    m_pFavoritePicTotal = new DLabel();
    QString favoriteStr = tr("%1张照片");
    m_pFavoritePicTotal->setFont(ft1);
    m_pFavoritePicTotal->setPalette(palette);


    int favoritePicNum = DBManager::instance()->getImgsCountByAlbum(COMMON_STR_FAVORITES);
    m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(favoritePicNum)));

    m_pRightFavoriteThumbnailList = new ThumbnailListView(COMMON_STR_FAVORITES);
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
        m_pStatusBar->show();
    }
    else
    {
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
        m_pStatusBar->hide();

    }
}

void AlbumView::updateRightView()
{
    if (COMMON_STR_TRASH == m_currentAlbum)
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

            m_pRightThumbnailList->insertThumbnails(m_curThumbnaiItemList);
            m_pRightThumbnailList->m_imageType = m_currentAlbum;
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
            m_pStatusBar->show();
        }
        else
        {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
            m_pStatusBar->hide();
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

        QString favoriteStr = tr("%1张照片");
        m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(m_iAlubmPicsNum)));

        DPalette palette = DApplicationHelper::instance()->palette(m_pRightPicTotal);
        palette.setBrush(DPalette::WindowText, palette.color(DPalette::WindowText));
        m_pFavoritePicTotal->setPalette(palette);

        QFont ft = DFontSizeManager::instance()->get(DFontSizeManager::T6);
        ft.setFamily("SourceHanSansSC");
        ft.setWeight(QFont::Medium);
        m_pFavoritePicTotal->setFont(ft);

        m_pRightFavoriteThumbnailList->insertThumbnails(m_curThumbnaiItemList);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_FAVORITE_LIST);
        setAcceptDrops(false);
    }
    else
    {
        m_importByPhoneWidget->setVisible(false);
        AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());

        qDebug()<<item->m_albumTypeStr;
        if ( ALBUM_PATHTYPE_BY_PHONE == item->m_albumTypeStr)
        {
            // 手机
            qDebug()<<item->m_albumNameStr;
            qDebug()<<dApp->m_phoneNameAndPathlist;
            if (true == dApp->m_phoneNameAndPathlist.contains(item->m_albumNameStr))
            {
                updateImportComboBox();
                m_importByPhoneWidget->setVisible(true);

                for(auto path : dApp->m_phoneNameAndPathlist.value(item->m_albumNameStr))
                {
                    ThumbnailListView::ItemInfo vi;
                    vi.path = path;
                    vi.image = dApp->m_phonePathAndImage.value(path);
                    m_curThumbnaiItemList<<vi;
                }

                m_iAlubmPicsNum = m_curThumbnaiItemList.size();

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

                m_pRightThumbnailList->insertThumbnails(m_curThumbnaiItemList);
                m_pRightThumbnailList->m_imageType = ALBUM_PATHTYPE_BY_PHONE;

                m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
            }
            else
            {
                m_pSpinner->start();
                m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_SPINNER);
            }
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

            m_pRightThumbnailList->insertThumbnails(m_curThumbnaiItemList);
            m_pRightThumbnailList->m_imageType = m_currentAlbum;

            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
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
            vi.remainDays = QString::number(30-Day) + "天";

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
        m_pDeleteBtn->setText(BUTTON_STR_DETELEALL);
        m_pRecoveryBtn->setEnabled(false);
        m_pDeleteBtn->setEnabled(false);
    }

    m_pRightTrashThumbnailList->insertThumbnails(m_curThumbnaiItemList);
}

void AlbumView::leftTabClicked(const QModelIndex &index)
{
    //若点击当前的item，则不做任何处理
    if(m_curListWidgetItem == m_pLeftTabList->currentItem()) return;
    m_curListWidgetItem = m_pLeftTabList->currentItem();
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

    m_pLeftMenu->setVisible(true);
    foreach (QAction* action , m_MenuActionMap.values()) {
        action->setVisible(true);
    }

    AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());

    if (COMMON_STR_RECENT_IMPORTED == item->m_albumNameStr
        || COMMON_STR_TRASH == item->m_albumNameStr
        || COMMON_STR_FAVORITES == item->m_albumNameStr
        || ALBUM_PATHTYPE_BY_PHONE == item->m_albumTypeStr)
    {
        return;
    }

    if (0 == DBManager::instance()->getImgsCountByAlbum(item->m_albumNameStr))
    {
        m_MenuActionMap.value(tr("幻灯片放映"))->setVisible(false);
    }

    if (0 == DBManager::instance()->getImgsCountByAlbum(item->m_albumNameStr))
    {
        m_MenuActionMap.value(tr("导出"))->setVisible(false);
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

        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(getNewAlbumName());

        m_pLeftTabList->insertItem(m_pLeftTabList->currentRow()+1, pListWidgetItem);
        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);

        m_pLeftTabList->setCurrentRow(m_pLeftTabList->currentRow()+1);

        for(int i = 0; i < m_pLeftTabList->count(); i++)
        {
            AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->item(i));
            item->oriAlbumStatus();
        }

        AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
        item->newAlbumStatus();
        item->m_opeMode = OPE_MODE_ADDNEWALBUM;
        item->editAlbumEdit();

        m_currentAlbum = item->m_albumNameStr;
        updateRightNoTrashView();
    }
        break;
    case IdRenameAlbum:
    {
        AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
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

        QString str1 = "相册：“%1”，已经删除成功";
        DUtil::DNotifySender *pDNotifySender = new DUtil::DNotifySender("深度相册");
        pDNotifySender->appName("deepin-album");
        pDNotifySender->appBody(str1.arg(str));
        pDNotifySender->call();
//        });
        for(int i = 0; i < m_pLeftTabList->count(); i++)
        {
            AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->item(i));
            item->oriAlbumStatus();
        }

        AlbumLeftTabItem *currentItem = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
        currentItem->newAlbumStatus();

        m_currentAlbum = currentItem->m_albumNameStr;

        updateRightView();
    }
        break;
    default:
        break;
    }
}

void AlbumView::createNewAlbum()
{
    QListWidgetItem *pListWidgetItem = new QListWidgetItem();
    pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
    QString albumName = getNewAlbumName();
    AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName);
    m_customAlbumNames << albumName;
    //新建相册需要在外接设备节点上面，此处调用getNewAlbumItemIndex函数，获取新建相册的index
    int index = getNewAlbumItemIndex();
    m_pLeftTabList->insertItem(index, pListWidgetItem);
    m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);

    m_pLeftTabList->setCurrentRow(index);

    for(int i = 0; i < m_pLeftTabList->count(); i++)
    {
        AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->item(i));
        item->oriAlbumStatus();
    }

    AlbumLeftTabItem *item = (AlbumLeftTabItem*)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
    item->newAlbumStatus();
    item->m_opeMode = OPE_MODE_ADDNEWALBUM;
    item->editAlbumEdit();

    m_currentAlbum = albumName;
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
    if(info.slideShow)
    {
        emit dApp->signalM->startSlideShow(info);
    }
    else {
        emit dApp->signalM->viewImage(info);
    }
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
        DPalette dpa = DApplicationHelper::instance()->palette(m_pDeleteBtn);
        dpa.setBrush(DPalette::Light, QColor(37,183,255));
        dpa.setBrush(DPalette::Dark, QColor(0,152,255));
        dpa.setBrush(DPalette::ButtonText, dpa.color(DPalette::HighlightedText));
        m_pRecoveryBtn->setPalette(dpa);

        m_pDeleteBtn->setText(BUTTON_STR_DETELE);
    }
    else
    {
        m_pRecoveryBtn->setEnabled(false);
        DPalette pal = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
        pal.setBrush(DPalette::Light, QColor(100,100,100));
        pal.setBrush(DPalette::Dark, QColor(92,92,92));
        pal.setBrush(DPalette::ButtonText, pal.color(DPalette::HighlightedText));
        m_pRecoveryBtn->setPalette(pal);

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

void AlbumView::onVfsMountChangedAdd(QExplicitlySharedDataPointer<DGioMount> mount)
{
    qDebug()<<"onVfsMountChangedAdd()";
    Q_UNUSED(mount);

    QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
    QString strPath = LocationFile->path();
    if (strPath.isEmpty())
    {
        qDebug()<<"onVfsMountChangedAdd() strPath.isEmpty()";
        return;
    }

    m_loadMountMap.insert(mount, 0);

    if (0 == m_loadMountFlag)
    {
        m_loadMountFlag = 1;
        qDebug()<<"onVfsMountChangedAdd() emit dApp->sigLoadMountImagesStart()"<<mount->name();
        emit dApp->sigLoadMountImagesStart(mount->name(), strPath);
    }

    qDebug()<<"onVfsMountChangedAdd() updateLeftView()";
    updateLeftView();
}

void AlbumView::onVfsMountChangedRemove(QExplicitlySharedDataPointer<DGioMount> mount)
{
    qDebug()<<"onVfsMountChangedRemove() mountname"<<mount->name();
    Q_UNUSED(mount);

    m_loadMountMap.remove(mount);

    qDebug()<<"onVfsMountChangedRemove() updateLeftView()";
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

    appendAction(IdStartSlideShow, tr(COMMON_STR_SLIDESHOW), ss(""));
    m_pLeftMenu->addSeparator();
    appendAction(IdCreateAlbum, tr(COMMON_STR_CREATEALBUM), ss(""));
    m_pLeftMenu->addSeparator();
    appendAction(IdRenameAlbum, tr(COMMON_STR_RENAMEALBUM), ss(COMMON_STR_RENAMEALBUM));
    m_pLeftMenu->addSeparator();
    appendAction(IdExport, tr(COMMON_STR_EXPORT), ss(""));
    m_pLeftMenu->addSeparator();
    appendAction(IdDeleteAlbum, tr("删除相册"), ss(""));
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

void AlbumView::updateExternalDevice()
{
    for (auto mount : m_mounts) {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftTabList);
        //pListWidgetItem缓存文件挂载路径
        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
        QString strPath = LocationFile->path();
        pListWidgetItem->setData(Qt::UserRole, strPath);
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(mount->name(), ALBUM_PATHTYPE_BY_PHONE);
        pAlbumLeftTabItem->setExternalDevicesMountPath(strPath);
        connect(pAlbumLeftTabItem, &AlbumLeftTabItem::unMountExternalDevices, this, &AlbumView::onUnMountSignal);
        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    }
}

void AlbumView::picsIntoAlbum(QStringList paths)
{
    if (COMMON_STR_RECENT_IMPORTED != m_currentAlbum
        && COMMON_STR_TRASH != m_currentAlbum
        && COMMON_STR_FAVORITES != m_currentAlbum)
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
    if (RIGHT_VIEW_SEARCH == m_pRightStackWidget->currentIndex())
    {
        m_currentAlbum = COMMON_STR_RECENT_IMPORTED;
        updateRightView();
    }
}

void AlbumView::onPixMapRotate(QStringList paths)
{
    dApp->m_imageloader->updateImageLoader(paths);

    updateRightView();
}

//搜索手机中存储相机图片文件的路径，采用两级文件目录深度，找"DCIM"文件目录
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
    QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/图片/", strDate);
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
    QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/图片/", strDate);
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
            break;
        }
    }
}

void AlbumView::onLoadMountImagesEnd(QString mountname)
{
    qDebug()<<"onLoadMountImagesEnd() mountname"<<mountname;
    qDebug()<<dApp->m_phoneNameAndPathlist;
    for(auto mount : m_loadMountMap.keys())
    {
        if (mount->name() == mountname)
        {
            m_loadMountMap[mount] = 1;
            qDebug()<<"onLoadMountImagesEnd() pdateRightView()";
            updateRightView();
            break;
        }
    }

    int iloadEndFlag = 0;

    for(auto mount : m_loadMountMap.keys())
    {
        if(0 == m_loadMountMap.value(mount))
        {
            QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
            QString strPath = LocationFile->path();
            if (strPath.isEmpty())
            {
                continue;
            }
            else
            {
                iloadEndFlag = 1;
                m_loadMountMap.insert(mount, 0);
                qDebug()<<"onLoadMountImagesEnd() emit dApp->sigLoadMountImagesStart()";
                emit dApp->sigLoadMountImagesStart(mount->name(), strPath);
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
