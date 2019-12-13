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
#include <dgiovolume.h>
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
#include "dialogs/albumcreatedialog.h"

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
const int RIGHT_VIEW_PHONE = 5;
const int RIGHT_VIEW_TIMELINE_IMPORT = 6;
const int VIEW_MAINWINDOW_ALBUM = 2;
const QString SHORTCUTVIEW_GROUP = "SHORTCUTVIEW";

static QMap<QString, const char *> i18nMap {
    {"data", "Data Disk"}
};
const QString ddeI18nSym = QStringLiteral("_dde_");

static std::initializer_list<std::pair<QString, QString>> opticalmediakeys {
    {"optical",                "Optical"},
    {"optical_cd",             "CD-ROM"},
    {"optical_cd_r",           "CD-R"},
    {"optical_cd_rw",          "CD-RW"},
    {"optical_dvd",            "DVD-ROM"},
    {"optical_dvd_r",          "DVD-R"},
    {"optical_dvd_rw",         "DVD-RW"},
    {"optical_dvd_ram",        "DVD-RAM"},
    {"optical_dvd_plus_r",     "DVD+R"},
    {"optical_dvd_plus_rw",    "DVD+RW"},
    {"optical_dvd_plus_r_dl",  "DVD+R/DL"},
    {"optical_dvd_plus_rw_dl", "DVD+RW/DL"},
    {"optical_bd",             "BD-ROM"},
    {"optical_bd_r",           "BD-R"},
    {"optical_bd_re",          "BD-RE"},
    {"optical_hddvd",          "HD DVD-ROM"},
    {"optical_hddvd_r",        "HD DVD-R"},
    {"optical_hddvd_rw",       "HD DVD-RW"},
    {"optical_mo",             "MO"}
};
static QVector<std::pair<QString, QString>> opticalmediakv(opticalmediakeys);
static QMap<QString, QString> opticalmediamap(opticalmediakeys);


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
    m_diskManager = new DDiskManager(this);
    m_diskManager->setWatchChanges(true);
    m_curListWidgetItem = nullptr;
    m_mountPicNum = 0;
    durlAndNameMap.clear();

    auto infos = DBManager::instance()->getAllInfos();
    QStringList pathlist;
    foreach (auto info, infos) {
        pathlist.append(info.filePath);
    }

    auto infostrash = DBManager::instance()->getAllTrashInfos();
    QStringList pathlisttrash;
    foreach (auto info, infostrash) {
        pathlisttrash.append(info.filePath);
    }

    connect(dApp->signalM, &SignalManager::sigLoadMountImagesEnd, this, &AlbumView::onLoadMountImagesEnd);

    setAcceptDrops(true);
    initLeftView();
    initRightView();
    initLeftMenu();

    QHBoxLayout *pLayout = new QHBoxLayout();
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->addWidget(m_pLeftWidget);
    pLayout->addWidget(m_pWidget);
    setLayout(pLayout);

    initConnections();
    m_pwidget = new QWidget(this);
}

AlbumView::~AlbumView()
{
    if (m_vfsManager) {
        delete  m_vfsManager;
        m_vfsManager = nullptr;
    }
}

QString sizeString(const QString &str)
{
    int begin_pos = str.indexOf('.');

    if (begin_pos < 0)
        return str;

    QString size = str;

    while (size.count() - 1 > begin_pos) {
        if (!size.endsWith('0'))
            return size;

        size = size.left(size.count() - 1);
    }

    return size.left(size.count() - 1);
}

QString formatSize(qint64 num, bool withUnitVisible = true, int precision = 1, int forceUnit = -1, QStringList unitList = QStringList())
{
    if (num < 0) {
        qWarning() << "Negative number passed to formatSize():" << num;
        num = 0;
    }

    bool isForceUnit = (forceUnit >= 0);
    QStringList list;
    qreal fileSize(num);

    if (unitList.size() == 0) {
        list << " B" << " KB" << " MB" << " GB" << " TB"; // should we use KiB since we use 1024 here?
    } else {
        list = unitList;
    }

    QStringListIterator i(list);
    QString unit = i.hasNext() ? i.next() : QStringLiteral(" B");

    int index = 0;
    while (i.hasNext()) {
        if (fileSize < 1024 && !isForceUnit) {
            break;
        }

        if (isForceUnit && index == forceUnit) {
            break;
        }

        unit = i.next();
        fileSize /= 1024;
        index++;
    }
    QString unitString = withUnitVisible ? unit : QString();
    return QString("%1%2").arg(sizeString(QString::number(fileSize, 'f', precision)), unitString);
}

void AlbumView::initConnections()
{
    connect(m_pLeftTabList, &DListWidget::clicked, this, &AlbumView::leftTabClicked);
    connect(dApp->signalM, &SignalManager::sigCreateNewAlbumFromDialog, this, &AlbumView::onCreateNewAlbumFromDialog);
#if 1
    connect(dApp->signalM, &SignalManager::sigCreateNewAlbumFrom, this, &AlbumView::onCreateNewAlbumFrom);
    connect(m_pRightThumbnailList, &ThumbnailListView::sigMouseMove, this, [ = ] {
        updatePicNum();
    });
    connect(m_pRightFavoriteThumbnailList, &ThumbnailListView::sigMouseMove, this, [ = ] {
        updatePicNum();
    });    
    connect(m_pRightFavoriteThumbnailList,&ThumbnailListView::sigSelectAll,this,&AlbumView::updatePicNum);
    connect(m_pRightThumbnailList,&ThumbnailListView::sigSelectAll,this,&AlbumView::updatePicNum);
    connect(m_pRightPhoneThumbnailList,&ThumbnailListView::sigSelectAll,this,&AlbumView::updatePicNum);
    connect(m_pRightTrashThumbnailList,&ThumbnailListView::sigSelectAll,this,&AlbumView::updatePicNum);

#endif
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::insertedIntoAlbum, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::removedFromAlbum, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::imagesTrashInserted, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::imagesTrashRemoved, this, &AlbumView::updateRightView);
    connect(dApp, &Application::sigFinishLoad, this, [ = ] {
        m_pImpTimeLineWidget->m_mainListWidget->update();
        m_pRightThumbnailList->update();
        m_pRightFavoriteThumbnailList->update();
        m_pRightTrashThumbnailList->update();
    });
    connect(m_pLeftTabList, &QListView::customContextMenuRequested, this, &AlbumView::showLeftMenu);
    connect(m_pLeftMenu, &DMenu::triggered, this, &AlbumView::onLeftMenuClicked);
    connect(m_pRecoveryBtn, &DPushButton::clicked, this, &AlbumView::onTrashRecoveryBtnClicked);
    connect(m_pDeleteBtn, &DPushButton::clicked, this, &AlbumView::onTrashDeleteBtnClicked);
    connect(m_pRightThumbnailList, &ThumbnailListView::openImage, this, &AlbumView::openImage);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::openImage, this, &AlbumView::openImage);
    connect(m_pRightFavoriteThumbnailList, &ThumbnailListView::openImage, this, &AlbumView::openImage);
    connect(m_pRightThumbnailList, &ThumbnailListView::menuOpenImage, this, &AlbumView::menuOpenImage);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::menuOpenImage, this, &AlbumView::menuOpenImage);
    connect(m_pRightFavoriteThumbnailList, &ThumbnailListView::menuOpenImage, this, &AlbumView::menuOpenImage);
    connect(dApp->signalM, &SignalManager::sigUpdataAlbumRightTitle, this, &AlbumView::onUpdataAlbumRightTitle);
    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::sigUpdateTrashImageLoader, this, &AlbumView::updateRightView);
    connect(m_vfsManager, &DGioVolumeManager::mountAdded, this, &AlbumView::onVfsMountChangedAdd);
    connect(m_vfsManager, &DGioVolumeManager::mountRemoved, this, &AlbumView::onVfsMountChangedRemove);
    connect(m_diskManager, &DDiskManager::blockDeviceAdded, this, [ = ](const QString & blks) {
        qDebug() << "--------------blks:" << blks;
        QSharedPointer<DBlockDevice> blk(DDiskManager::createBlockDevice(blks));
        QScopedPointer<DDiskDevice> drv(DDiskManager::createDiskDevice(blk->drive()));

        if (!blk->hasFileSystem() && !drv->mediaCompatibility().join(" ").contains("optical") && !blk->isEncrypted()) {
            return;
        }
        if ((blk->hintIgnore() && !blk->isEncrypted()) || blk->cryptoBackingDevice().length() > 1) {
            return;
        }
        DBlockDevice *pblk = blk.data();
        QByteArrayList mps = blk->mountPoints();
        qulonglong size = blk->size();
        QString label = blk->idLabel();
        QString fs = blk->idType();
        QString udispname = "";
        if (label.startsWith(ddeI18nSym)) {
            QString i18nKey = label.mid(ddeI18nSym.size(), label.size() - ddeI18nSym.size());
            udispname = qApp->translate("DeepinStorage", i18nMap.value(i18nKey, i18nKey.toUtf8().constData()));
            goto runend;
//            blk->mount({});
//            return;
        }

        if (mps.contains(QByteArray("/\0", 2))) {
            udispname = QCoreApplication::translate("PathManager", "System Disk");
            goto runend;
//            blk->mount({});
//            return;
        }
        if (label.length() == 0) {
            QScopedPointer<DDiskDevice> drv(DDiskManager::createDiskDevice(pblk->drive()));
            if (!drv->mediaAvailable() && drv->mediaCompatibility().join(" ").contains("optical")) {
                QString maxmediacompat;
                for (auto i = opticalmediakv.rbegin(); i != opticalmediakv.rend(); ++i) {
                    if (drv->mediaCompatibility().contains(i->first)) {
                        maxmediacompat = i->second;
                        break;
                    }
                }
                udispname = QCoreApplication::translate("DeepinStorage", "%1 Drive").arg(maxmediacompat);
                goto runend;
                //            blk->mount({});
                //            return;
            }
            if (drv->opticalBlank()) {
                udispname = QCoreApplication::translate("DeepinStorage", "Blank %1 Disc").arg(opticalmediamap[drv->media()]);
                goto runend;
                //            blk->mount({});
                //            return;
            }
            if (pblk->isEncrypted() && !blk) {
                udispname = QCoreApplication::translate("DeepinStorage", "%1 Encrypted").arg(formatSize(size));
                goto runend;
                //            blk->mount({});
                //            return;
            }
//            udispname = QCoreApplication::translate("DeepinStorage", "%1 Volume").arg(formatSize(size));
            udispname = QCoreApplication::translate("DeepinStorage", "%1 ").arg(formatSize(size));
            udispname += tr("卷");
            goto runend;
//            blk->mount({});
//            return;
        }
        udispname = label;

runend:
        blk->mount({});
        QByteArrayList qbl = blk->mountPoints();
        QString mountPoint = "file://";
        for (QByteArray qb : qbl) {
            mountPoint += qb;
        }
        QUrl qurl(mountPoint);
        durlAndNameMap[qurl] = udispname;
        return;
    });
    connect(m_importAllByPhoneBtn, &DPushButton::clicked, this, &AlbumView::importAllBtnClicked);
    connect(m_importSelectByPhoneBtn, &DPushButton::clicked, this, &AlbumView::importSelectBtnClicked);
    connect(m_pStatusBar->m_pSlider, &DSlider::valueChanged, dApp->signalM, &SignalManager::sigMainwindowSliderValueChg);
    connect(m_pLeftTabList, &LeftListWidget::signalDropEvent, this, &AlbumView::onLeftListDropEvent);
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
    this, [ = ] {
        DPalette ReBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
        ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
        m_pRecoveryBtn->setPalette(ReBtn);

        DPalette DeBtn = DApplicationHelper::instance()->palette(m_pDeleteBtn);
        DeBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
        m_pDeleteBtn->setPalette(DeBtn);

    });
#if 1
    connect(m_pRightThumbnailList, &ThumbnailListView::customContextMenuRequested, this, &AlbumView::updatePicNum);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::customContextMenuRequested, this, &AlbumView::updatePicNum);
    connect(m_pRightFavoriteThumbnailList, &ThumbnailListView::customContextMenuRequested, this, &AlbumView::updatePicNum);
    connect(m_pRightPhoneThumbnailList, &ThumbnailListView::customContextMenuRequested, this, [ = ] {
        QStringList paths = m_pRightPhoneThumbnailList->selectedPaths();
        if (0 < paths.length())
        {
            m_importSelectByPhoneBtn->setEnabled(true);
        } else
        {
            m_importSelectByPhoneBtn->setEnabled(false);
        }

        updatePicNum();
    });

    connect(m_pRightThumbnailList, &ThumbnailListView::sigMouseRelease, this, &AlbumView::updatePicNum);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::sigMouseRelease, this, &AlbumView::updatePicNum);
    connect(m_pRightFavoriteThumbnailList, &ThumbnailListView::sigMouseRelease, this, &AlbumView::updatePicNum);
    connect(m_pRightPhoneThumbnailList, &ThumbnailListView::sigMouseRelease, this, [ = ] {
        QStringList paths = m_pRightPhoneThumbnailList->selectedPaths();
        if (0 < paths.length())
        {
            m_importSelectByPhoneBtn->setEnabled(true);
        } else
        {
            m_importSelectByPhoneBtn->setEnabled(false);
        }

        updatePicNum();
    });
    connect(m_pImpTimeLineWidget, &ImportTimeLineView::sigUpdatePicNum, this, &AlbumView::updatePicNum);

    connect(m_pRightTrashThumbnailList, &ThumbnailListView::customContextMenuRequested, this, &AlbumView::onTrashListClicked);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::sigMouseRelease, this, &AlbumView::onTrashListClicked);
#endif
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::trashRecovery, this, &AlbumView::onTrashRecoveryBtnClicked);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::trashDelete, this, &AlbumView::onTrashListClicked);

    connect(DApplicationHelper::instance(), &DApplicationHelper::themeTypeChanged, m_pLeftTabList, [ = ] {
        AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
        item->newAlbumStatus();
    });
    connect(m_pLeftTabList, &DListWidget::currentItemChanged, this, [ = ] {
        if (0 < m_pLeftTabList->count())
        {
            for (int i = 0; i < m_pLeftTabList->count(); i++) {
                AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->item(i));
                item->oriAlbumStatus();
            }
            AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
            item->newAlbumStatus();
            if (COMMON_STR_TRASH == item->m_albumNameStr) {
                m_currentAlbum = item->m_albumNameStr;
                updateRightView();
                m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TRASH_LIST);

                m_iAlubmPicsNum = DBManager::instance()->getTrashImgsCount();
                emit sigSearchEditIsDisplay(false);
                setAcceptDrops(false);
            } else {
                m_currentAlbum = item->m_albumNameStr;
                updateRightView();
            }
        }
    });

    connect(m_pImportView->m_pImportBtn, &DPushButton::clicked, this, [ = ] {
        m_pImportView->onImprotBtnClicked();
    });
    connect(m_pImportView, &ImportView::importFailedToView, this, [ = ] {
    });
    connect(m_importByPhoneComboBox, &DComboBox::currentTextChanged, this, &AlbumView::importComboBoxChange);
    connect(dApp->signalM, &SignalManager::updateFavoriteNum, this, [=]{
        m_iAlubmPicsNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);
        QString favoriteStr = tr("%1 photo(s)");
        m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(m_iAlubmPicsNum)));
    });
}

void AlbumView::initLeftView()
{
    m_pLeftTabList = new LeftListWidget();

    DStyledItemDelegate *itemDelegate = new DStyledItemDelegate(m_pLeftTabList);
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
    pLeftLayout->setContentsMargins(0, 0, 0, 0);
    pLeftLayout->addStretch();
    pLeftLayout->addWidget(m_pLeftTabList, Qt::AlignHCenter);
    pLeftLayout->addStretch();

    m_pLeftWidget->setLayout(pLeftLayout);

    m_pLeftMenu = new DMenu();

    m_allAlbumNames << COMMON_STR_RECENT_IMPORTED;
    m_allAlbumNames << COMMON_STR_TRASH;
    m_allAlbumNames << COMMON_STR_FAVORITES;

    QStringList allAlbumNames = DBManager::instance()->getAllAlbumNames();
    for (auto albumName : allAlbumNames) {
        if (COMMON_STR_FAVORITES == albumName || COMMON_STR_RECENT_IMPORTED == albumName || COMMON_STR_TRASH == albumName) {
            continue;
        }

        m_allAlbumNames << albumName;
//        m_customAlbumNames << albumName;
    }

    for (auto albumName : m_allAlbumNames) {
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

    AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->item(0));
    item->newAlbumStatus();
}

void AlbumView::onCreateNewAlbumFromDialog(QString newalbumname)
{
    //新建相册需要在外接设备节点上面，此处调用getNewAlbumItemIndex函数，获取新建相册的index
    int index = getNewAlbumItemIndex();

    QListWidgetItem *pListWidgetItem = new QListWidgetItem();
    m_pLeftTabList->insertItem(index, pListWidgetItem);
    pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
    QString albumName = newalbumname;
    AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName, m_pLeftTabList, pListWidgetItem);
//    m_customAlbumNames << albumName;

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
//    m_customAlbumNames << albumName;

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
    QList<QLabel *> labelList = m_pImportView->findChildren<QLabel *>();
    labelList[1]->setText(tr("Or drag photos here"));

    // Thumbnail View
    DWidget *pNoTrashWidget = new DWidget();
    pNoTrashWidget->setBackgroundRole(DPalette::Window);

    QVBoxLayout *pNoTrashVBoxLayout = new QVBoxLayout();
    pNoTrashVBoxLayout->setContentsMargins(0, 0, 0, 0);

    m_pRightTitle = new DLabel();
    m_pRightTitle->setText(tr("Import"));
    DFontSizeManager::instance()->bind(m_pRightTitle, DFontSizeManager::T3, QFont::DemiBold);
    m_pRightTitle->setForegroundRole(DPalette::TextTitle);

    m_pRightPicTotal = new DLabel();
    QString str = tr("%1 photo(s)");
    m_pRightPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));
    DFontSizeManager::instance()->bind(m_pRightPicTotal, DFontSizeManager::T6, QFont::Medium);
    m_pRightPicTotal->setForegroundRole(DPalette::TextTips);

    m_pRightThumbnailList = new ThumbnailListView(COMMON_STR_RECENT_IMPORTED);
    m_pRightThumbnailList->setFrameShape(DTableView::NoFrame);

    pNoTrashVBoxLayout->addSpacing(3);
    pNoTrashVBoxLayout->addWidget(m_pRightTitle);
    pNoTrashVBoxLayout->addSpacing(4);
    pNoTrashVBoxLayout->addWidget(m_pRightPicTotal);
    pNoTrashVBoxLayout->addSpacing(-1);
    pNoTrashVBoxLayout->setContentsMargins(10, 0, 0, 0);

    QVBoxLayout *p_all = new QVBoxLayout();
    p_all->setContentsMargins(0, 0, 2, 0);
    p_all->addLayout(pNoTrashVBoxLayout);
    p_all->addSpacing(2);
    p_all->addWidget(m_pRightThumbnailList);

    pNoTrashWidget->setLayout(p_all);

    // Trash View
    DWidget *pTrashWidget = new DWidget();
    QVBoxLayout *pMainVBoxLayout = new QVBoxLayout();
    QHBoxLayout *pTopHBoxLayout = new QHBoxLayout();

    QVBoxLayout *pTopLeftVBoxLayout = new QVBoxLayout();

    pLabel1 = new DLabel();
    DFontSizeManager::instance()->bind(pLabel1, DFontSizeManager::T3, QFont::DemiBold);
    pLabel1->setForegroundRole(DPalette::TextTitle);
    pLabel1->setText(tr("Trash"));

    pLabel2 = new DLabel();
    DFontSizeManager::instance()->bind(pLabel2, DFontSizeManager::T6, QFont::Medium);
    pLabel2->setForegroundRole(DPalette::TextTips);
    pLabel2->setText(tr("The photos will be permanently deleted after the days shown on it"));

    pTopLeftVBoxLayout->addSpacing(3);
    pTopLeftVBoxLayout->addWidget(pLabel1);
    pTopLeftVBoxLayout->addSpacing(9);
    pTopLeftVBoxLayout->addWidget(pLabel2);
    pTopLeftVBoxLayout->addSpacing(-1);
    pTopLeftVBoxLayout->setContentsMargins(10, 0, 0, 0);

    QHBoxLayout *pTopRightVBoxLayout = new QHBoxLayout();
    m_pRecoveryBtn = new DPushButton();

    m_pRecoveryBtn->setText(tr("Restore"));
    m_pRecoveryBtn->setEnabled(false);
    m_pRecoveryBtn->setFixedSize(100, 36);

    DPalette ReBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
    ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_pRecoveryBtn->setPalette(ReBtn);

    m_pDeleteBtn = new DWarningButton();

    m_pDeleteBtn->setText(tr("Delete All"));
    m_pDeleteBtn->setFixedSize(100, 36);

    DPalette DeBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
    ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
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

    pMainVBoxLayout->setMargin(2);
    pMainVBoxLayout->addItem(pTopHBoxLayout);
    pMainVBoxLayout->addSpacing(2);
    pMainVBoxLayout->addWidget(m_pRightTrashThumbnailList);

    pTrashWidget->setLayout(pMainVBoxLayout);

    // Favorite View
    DWidget *pFavoriteWidget = new DWidget();
    QVBoxLayout *pFavoriteVBoxLayout = new QVBoxLayout();

    m_pFavoriteTitle = new DLabel();
    DFontSizeManager::instance()->bind(m_pFavoriteTitle, DFontSizeManager::T3, QFont::DemiBold);
    m_pFavoriteTitle->setForegroundRole(DPalette::TextTitle);
    m_pFavoriteTitle->setText(tr("Favorites"));

    m_pFavoritePicTotal = new DLabel();
    DFontSizeManager::instance()->bind(m_pFavoritePicTotal, DFontSizeManager::T6, QFont::Medium);
    m_pFavoritePicTotal->setForegroundRole(DPalette::TextTips);
    QString favoriteStr = tr("%1 photo(s)");

    int favoritePicNum = DBManager::instance()->getImgsCountByAlbum(COMMON_STR_FAVORITES);
    m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(favoritePicNum)));

    m_pRightFavoriteThumbnailList = new ThumbnailListView(COMMON_STR_FAVORITES);
    m_pRightFavoriteThumbnailList->setFrameShape(DTableView::NoFrame);

    pFavoriteVBoxLayout->addSpacing(3);
    pFavoriteVBoxLayout->addWidget(m_pFavoriteTitle);
    pFavoriteVBoxLayout->addSpacing(4);
    pFavoriteVBoxLayout->addWidget(m_pFavoritePicTotal);
    pFavoriteVBoxLayout->addSpacing(-1);

    pFavoriteVBoxLayout->setContentsMargins(10, 0, 0, 0);

    QVBoxLayout *p_all1 = new QVBoxLayout();

    p_all1->setMargin(2);
    p_all1->addLayout(pFavoriteVBoxLayout);
    p_all1->addSpacing(2);
    p_all1->addWidget(m_pRightFavoriteThumbnailList);

    pFavoriteWidget->setLayout(p_all1);

    //Search View
    m_pSearchView = new SearchView;

    // Phone View
    DWidget *pPhoneWidget = new DWidget();
    pPhoneWidget->setBackgroundRole(DPalette::Window);

    QVBoxLayout *pPhoneVBoxLayout = new QVBoxLayout();
    pPhoneVBoxLayout->setContentsMargins(0, 0, 0, 0);

    m_pPhoneTitle = new DLabel();
    DFontSizeManager::instance()->bind(m_pPhoneTitle, DFontSizeManager::T3, QFont::DemiBold);
    m_pPhoneTitle->setForegroundRole(DPalette::TextTitle);

    m_pPhonePicTotal = new DLabel();
    DFontSizeManager::instance()->bind(m_pPhonePicTotal, DFontSizeManager::T6, QFont::Medium);
    m_pPhonePicTotal->setForegroundRole(DPalette::TextTips);

    m_pRightPhoneThumbnailList = new ThumbnailListView(ALBUM_PATHTYPE_BY_PHONE);
    m_pRightPhoneThumbnailList->setFrameShape(DTableView::NoFrame);

    pPhoneVBoxLayout->addSpacing(3);
    pPhoneVBoxLayout->addWidget(m_pPhoneTitle);
    pPhoneVBoxLayout->addSpacing(4);
    pPhoneVBoxLayout->addWidget(m_pPhonePicTotal);
    pPhoneVBoxLayout->addSpacing(-1);
    pPhoneVBoxLayout->setContentsMargins(10, 0, 0, 0);

    //手机相片导入窗体
    m_importByPhoneWidget = new DWidget;
    QHBoxLayout *mainImportLayout = new QHBoxLayout;
    DLabel *importLabel = new DLabel();
    importLabel->setText(tr("Import to:"));
    DFontSizeManager::instance()->bind(importLabel, DFontSizeManager::T6, QFont::Medium);
    importLabel->setForegroundRole(DPalette::TextTips);
    importLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_importByPhoneComboBox = new DComboBox;
    m_importByPhoneComboBox->setMinimumSize(QSize(213, 36));
    m_importByPhoneComboBox->setEnabled(false);

    m_importAllByPhoneBtn = new DPushButton(tr("Import All"));
    m_importAllByPhoneBtn ->setMinimumSize(100, 36);
    DPalette importAllByPhoneBtnPa = DApplicationHelper::instance()->palette(m_importAllByPhoneBtn);
    importAllByPhoneBtnPa.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_importAllByPhoneBtn->setPalette(importAllByPhoneBtnPa);
    m_importAllByPhoneBtn->setEnabled(false);

    m_importSelectByPhoneBtn = new DSuggestButton(tr("Import Selected"));
    m_importSelectByPhoneBtn->setMinimumSize(100, 36);
    DPalette importSelectByPhoneBtnPa = DApplicationHelper::instance()->palette(m_importSelectByPhoneBtn);
    importSelectByPhoneBtnPa.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_importSelectByPhoneBtn->setPalette(importSelectByPhoneBtnPa);
    m_importSelectByPhoneBtn->setEnabled(false);

    mainImportLayout->addWidget(importLabel);
    mainImportLayout->addSpacing(11);
    mainImportLayout->addWidget(m_importByPhoneComboBox);
    mainImportLayout->addSpacing(30);
    mainImportLayout->addWidget(m_importAllByPhoneBtn);
    mainImportLayout->addSpacing(10);
    mainImportLayout->addWidget(m_importSelectByPhoneBtn);
    m_importByPhoneWidget->setLayout(mainImportLayout);

    QHBoxLayout *allHLayout = new QHBoxLayout;
    allHLayout->addLayout(pPhoneVBoxLayout, 1);
    allHLayout->addStretch();
    allHLayout->addWidget(m_importByPhoneWidget, 1);

    QVBoxLayout *p_all2 = new QVBoxLayout();
    p_all2->addLayout(allHLayout);
    p_all2->addWidget(m_pRightPhoneThumbnailList);

    pPhoneWidget->setLayout(p_all2);

    // 导入图片,按导入时间排列
    pImportTimeLineWidget = new DWidget();
//    pImportTimeLineWidget->setStyleSheet("background:red");
    pImportTimeLineWidget->setBackgroundRole(DPalette::Window);

    QVBoxLayout *pImpTimeLineVBoxLayout = new QVBoxLayout();
    pImpTimeLineVBoxLayout->setContentsMargins(0, 0, 0, 0);

    DLabel *pImportTitle = new DLabel();
    pImportTitle->setText(tr("Import"));
    DFontSizeManager::instance()->bind(pImportTitle, DFontSizeManager::T3, QFont::DemiBold);
    pImportTitle->setForegroundRole(DPalette::TextTitle);

//    m_pImportPicTotal = new DLabel();
//    QString strTitle = tr("%1 photo(s)");
//    m_pImportPicTotal->setText(strTitle.arg(QString::number(m_iAlubmPicsNum)));
//    DFontSizeManager::instance()->bind(m_pImportPicTotal, DFontSizeManager::T6, QFont::Medium);
//    m_pImportPicTotal->setForegroundRole(DPalette::TextTips);

    m_pImpTimeLineWidget = new ImportTimeLineView(pImportTimeLineWidget);
    m_pImpTimeLineWidget->move(-6, 40);

    pImpTimeLineVBoxLayout->addSpacing(5);
    pImpTimeLineVBoxLayout->addWidget(pImportTitle);
//    pImpTimeLineVBoxLayout->addSpacing(4);
//    pImpTimeLineVBoxLayout->addWidget(m_pImportPicTotal);
    pImpTimeLineVBoxLayout->addSpacing(-6);

    QHBoxLayout *pImpTimeLineHLayout = new QHBoxLayout;
    pImpTimeLineHLayout->addSpacing(10);
    pImpTimeLineHLayout->addLayout(pImpTimeLineVBoxLayout);
//    pImpTimeLineHLayout->addStretch();

    QVBoxLayout *pImportAllV = new QVBoxLayout();
    pImportAllV->setContentsMargins(0, 0, 2, 0);
    pImportAllV->addLayout(pImpTimeLineHLayout);
    pImportAllV->addStretch();
//    pImportAllV->addWidget(m_pImpTimeLineWidget);
    pImportTimeLineWidget->setLayout(pImportAllV);

    // Add View
    m_pRightStackWidget->addWidget(m_pImportView);
    m_pRightStackWidget->addWidget(pNoTrashWidget);
    m_pRightStackWidget->addWidget(pTrashWidget);
    m_pRightStackWidget->addWidget(pFavoriteWidget);
    m_pRightStackWidget->addWidget(m_pSearchView);
    m_pRightStackWidget->addWidget(pPhoneWidget);
    m_pRightStackWidget->addWidget(pImportTimeLineWidget);

    // Statusbar
    m_pStatusBar = new StatusBar();
    m_pStatusBar->setParent(this);

    QVBoxLayout *pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins(0, 0, 0, 0);
    pVBoxLayout->addWidget(m_pRightStackWidget);
    pVBoxLayout->addWidget(m_pStatusBar);
    m_pWidget->setLayout(pVBoxLayout);

//    if (0 < DBManager::instance()->getImgsCount())
//    {
//        m_pRightThumbnailList->setFrameShape(DTableView::NoFrame);
//        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
//        m_pStatusBar->show();
//    } else {
//        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
//        m_pStatusBar->show();

//    }
    updateRightView();
}

void AlbumView::updateRightView()
{
    if (COMMON_STR_TRASH == m_currentAlbum) {
        updateRightTrashView();
        onTrashListClicked();
        emit sigSearchEditIsDisplay(false);
    } else {
        updateRightNoTrashView();
    }

    updatePicNum();
}

void AlbumView::updateRightNoTrashView()
{
    using namespace utils::image;
//    m_pSpinner->stop();
    m_curThumbnaiItemList.clear();

    DBImgInfoList infos;

    // 已导入
    if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum) {
//        infos = DBManager::instance()->getAllInfos();

//        for (auto info : infos) {
//            ThumbnailListView::ItemInfo vi;
//            vi.name = info.fileName;
//            vi.path = info.filePath;
////            vi.image = dApp->m_imagemap.value(info.filePath);
//            if (dApp->m_imagemap.value(info.filePath).isNull())
//            {
//                QSize imageSize = getImageQSize(vi.path);

//                vi.width = imageSize.width();
//                vi.height = imageSize.height();
//            }
//            else
//            {
//                vi.width = dApp->m_imagemap.value(info.filePath).width();
//                vi.height = dApp->m_imagemap.value(info.filePath).height();
//            }

//            m_curThumbnaiItemList << vi;
//        }

        m_iAlubmPicsNum = DBManager::instance()->getImgsCount();

        if (0 < m_iAlubmPicsNum) {
//            QString str = tr("%1 photo(s)");
//            m_pImportPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));

            m_pImpTimeLineWidget->updataLayout();

            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TIMELINE_IMPORT);
            m_pStatusBar->show();
        } else {

            m_pImportView->setAlbumname(QString());
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
            m_pStatusBar->show();
        }

        emit sigSearchEditIsDisplay(true);

        setAcceptDrops(true);
    } else if (COMMON_STR_FAVORITES == m_currentAlbum) { //个人收藏
        infos = DBManager::instance()->getInfosByAlbum(m_currentAlbum);

        for (auto info : infos) {
            ThumbnailListView::ItemInfo vi;
            vi.name = info.fileName;
            vi.path = info.filePath;
//            vi.image = dApp->m_imagemap.value(info.filePath);
            if (dApp->m_imagemap.value(info.filePath).isNull()) {
                QSize imageSize = getImageQSize(vi.path);

                vi.width = imageSize.width();
                vi.height = imageSize.height();
            } else {
                vi.width = dApp->m_imagemap.value(info.filePath).width();
                vi.height = dApp->m_imagemap.value(info.filePath).height();
            }

            m_curThumbnaiItemList << vi;
        }

        m_iAlubmPicsNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);

        QString favoriteStr = tr("%1 photo(s)");
        m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(m_iAlubmPicsNum)));

        m_pRightFavoriteThumbnailList->insertThumbnails(m_curThumbnaiItemList);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_FAVORITE_LIST);
        emit sigSearchEditIsDisplay(true);
        setAcceptDrops(false);
    } else {
        AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());

        qDebug() << item->m_albumTypeStr;
        if ( ALBUM_PATHTYPE_BY_PHONE == item->m_albumTypeStr || ALBUM_PATHTYPE_BY_U == item->m_albumTypeStr) {
            // 手机
            qDebug() << item->m_albumNameStr;
            qDebug() << m_phoneNameAndPathlist;
            QString strPath = m_pLeftTabList->currentItem()->data(Qt::UserRole).toString();
            qDebug() << "data(Qt::UserRole).toString()" << strPath;
            qDebug() << m_phoneNameAndPathlist.contains(strPath);
            qDebug() << m_phoneNameAndPathlist.value(strPath).length();

            if (true == m_phoneNameAndPathlist.contains(strPath) && 0 < m_phoneNameAndPathlist.value(strPath).length()) {
                m_importByPhoneComboBox->setEnabled(true);
                m_importAllByPhoneBtn->setEnabled(true);
                updateImportComboBox();

                for (auto path : m_phoneNameAndPathlist.value(strPath)) {
                    ThumbnailListView::ItemInfo vi;
                    vi.path = path;
                    vi.image = m_phonePathAndImage.value(path);
                    vi.width = vi.image.width();
                    vi.height = vi.image.height();
                    m_curThumbnaiItemList << vi;
                }

                m_iAlubmPicsNum = m_curThumbnaiItemList.size();
                m_mountPicNum = m_curThumbnaiItemList.size();
                qDebug() << "m_mountPicNum = " << m_mountPicNum;
                m_pPhoneTitle->setText(m_currentAlbum);

                QFontMetrics elideFont(m_pPhoneTitle->font());
                m_pPhoneTitle->setText(elideFont.elidedText(m_currentAlbum, Qt::ElideRight, 525));

                QString str = tr("%1 photo(s)");
                m_pPhonePicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));

                //保存更新之前的选择状态
                QModelIndexList mlist = m_pRightPhoneThumbnailList->getSelectedIndexes();
                QModelIndexList::iterator i;
                struct Listolditem {
                    int row;
                    int column;
                };
                QList<Listolditem> items;
                for (i = mlist.begin(); i != mlist.end(); ++i) {
                    Listolditem item;
                    item.row = (*i).row();
                    item.column = (*i).column();
                    items.append(item);
                }

                m_pRightPhoneThumbnailList->m_imageType = ALBUM_PATHTYPE_BY_PHONE;
                m_pRightPhoneThumbnailList->insertThumbnails(m_curThumbnaiItemList);

                //设置更新之前的选择状态
                QList<Listolditem>::iterator j;
                for (j = items.begin(); j != items.end(); ++j) {
                    if ((*j).row < m_pRightPhoneThumbnailList->m_model->rowCount()
                            && (*j).column < m_pRightPhoneThumbnailList->m_model->columnCount()) {
                        QModelIndex qindex = m_pRightPhoneThumbnailList->m_model->index((*j).row, (*j).column);
                        m_pRightPhoneThumbnailList->selectionModel()->select(qindex, QItemSelectionModel::Select);
                    }
                }

                QStringList paths = m_pRightPhoneThumbnailList->selectedPaths();
                if (0 < paths.length()) {
                    m_importSelectByPhoneBtn->setEnabled(true);
                } else {
                    m_importSelectByPhoneBtn->setEnabled(false);
                }

                m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_PHONE);

            } else {
                qDebug() << "phone zero";
                m_importByPhoneComboBox->setEnabled(false);
                m_importAllByPhoneBtn->setEnabled(false);
                m_importSelectByPhoneBtn->setEnabled(false);
                m_mountPicNum = 0;
                m_pPhoneTitle->setText(m_currentAlbum);

                QFontMetrics elideFont(m_pPhoneTitle->font());
                m_pPhoneTitle->setText(elideFont.elidedText(m_currentAlbum, Qt::ElideRight, 525));

                QString str = tr("%1 photo(s)");
                m_pPhonePicTotal->setText(str.arg(QString::number(0)));

                m_pRightPhoneThumbnailList->m_imageType = ALBUM_PATHTYPE_BY_PHONE;
                m_pRightPhoneThumbnailList->insertThumbnails(m_curThumbnaiItemList);

                m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_PHONE);
            }

            emit sigSearchEditIsDisplay(false);
            setAcceptDrops(false);
        } else {
            // 新建相册
            infos = DBManager::instance()->getInfosByAlbum(m_currentAlbum);

            for (auto info : infos) {
                ThumbnailListView::ItemInfo vi;
                vi.name = info.fileName;
                vi.path = info.filePath;
//                vi.image = dApp->m_imagemap.value(info.filePath);
                if (dApp->m_imagemap.value(info.filePath).isNull()) {
                    QSize imageSize = getImageQSize(vi.path);

                    vi.width = imageSize.width();
                    vi.height = imageSize.height();
                } else {
                    vi.width = dApp->m_imagemap.value(info.filePath).width();
                    vi.height = dApp->m_imagemap.value(info.filePath).height();
                }

                m_curThumbnaiItemList << vi;
            }

            m_iAlubmPicsNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);

            if (0 < m_iAlubmPicsNum) {
                m_pRightTitle->setText(m_currentAlbum);

                QFontMetrics elideFont(m_pRightTitle->font());
                m_pRightTitle->setText(elideFont.elidedText(m_currentAlbum, Qt::ElideRight, 525));

                QString str = tr("%1 photo(s)");
                m_pRightPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));

                m_pRightThumbnailList->m_imageType = m_currentAlbum;
                m_pRightThumbnailList->insertThumbnails(m_curThumbnaiItemList);

                m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
                m_pStatusBar->show();
            } else {
                m_pImportView->setAlbumname(m_currentAlbum);
                m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
                m_pStatusBar->show();
            }

            emit sigSearchEditIsDisplay(true);
            setAcceptDrops(true);
        }
    }
}

void AlbumView::updateRightTrashView()
{
    using namespace utils::image;
    int idaysec = 24 * 60 * 60;
    m_curThumbnaiItemList.clear();

    DBImgInfoList infos;
    QStringList removepaths;

    infos = DBManager::instance()->getAllTrashInfos();

    for (auto info : infos) {
        QDateTime start = QDateTime::currentDateTime();
        QDateTime end = info.changeTime;

        uint etime = start.toTime_t();
        uint stime = end.toTime_t();

        int Day = (etime - stime) / (idaysec) + ((etime - stime) % (idaysec) + (idaysec - 1)) / (idaysec) - 1;

        if (30 <= Day) {
            removepaths << info.filePath;
        } else {
            ThumbnailListView::ItemInfo vi;
            vi.name = info.fileName;
            vi.path = info.filePath;
//            vi.image = dApp->m_imagetrashmap.value(info.filePath);
            if (dApp->m_imagetrashmap.value(info.filePath).isNull()) {
                QSize imageSize = getImageQSize(vi.path);

                vi.width = imageSize.width();
                vi.height = imageSize.height();
            } else {
                vi.width = dApp->m_imagetrashmap.value(info.filePath).width();
                vi.height = dApp->m_imagetrashmap.value(info.filePath).height();
            }
            vi.remainDays = QString::number(30 - Day) + tr(" D");

            m_curThumbnaiItemList << vi;
        }
    }

    if (0 < removepaths.length()) {
//        for (auto path : removepaths) {
//            dApp->m_imagetrashmap.remove(path);
//        }

        DBManager::instance()->removeTrashImgInfosNoSignal(removepaths);
    }

    if (0 < infos.length()) {
        m_pDeleteBtn->setEnabled(true);
    } else {
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

    if (m_curListWidgetItem == m_pLeftTabList->currentItem()) {
        SearchReturnUpdate();
        return;
    }

    m_curListWidgetItem = m_pLeftTabList->currentItem();

    AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());

    if (COMMON_STR_TRASH == item->m_albumNameStr) {
        m_currentAlbum = item->m_albumNameStr;
        updateRightTrashView();
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TRASH_LIST);

        m_iAlubmPicsNum = DBManager::instance()->getTrashImgsCount();
        setAcceptDrops(false);
    } else {
        m_currentAlbum = item->m_albumNameStr;
        updateRightNoTrashView();
    }
}

void AlbumView::showLeftMenu(const QPoint &pos)
{
    if (!m_pLeftTabList->indexAt(pos).isValid()) {
        return;
    }

    AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());

    if (COMMON_STR_RECENT_IMPORTED == item->m_albumNameStr
            || COMMON_STR_TRASH == item->m_albumNameStr
            || COMMON_STR_FAVORITES == item->m_albumNameStr
            || ALBUM_PATHTYPE_BY_PHONE == item->m_albumTypeStr
            || ALBUM_PATHTYPE_BY_U == item->m_albumTypeStr) {
        return;
    }

    m_pLeftMenu->setVisible(true);
    foreach (QAction *action, m_MenuActionMap.values()) {
        action->setVisible(true);
    }

    if (0 == DBManager::instance()->getImgsCountByAlbum(item->m_albumNameStr)) {
        m_MenuActionMap.value(tr("Slide show"))->setVisible(false);
    }

    if (0 == DBManager::instance()->getImgsCountByAlbum(item->m_albumNameStr)) {
        m_MenuActionMap.value(tr("Export"))->setVisible(false);
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
    switch (MenuItemId(id)) {
    case IdStartSlideShow: {
        auto imagelist = DBManager::instance()->getInfosByAlbum(m_currentAlbum);
        QStringList paths;
        for (auto image : imagelist) {
            paths << image.filePath;
        }

        const QString path = paths.first();

        emit m_pRightThumbnailList->menuOpenImage(path, paths, true, true);
    }
    break;
    case IdCreateAlbum: {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem();
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));

        m_pLeftTabList->insertItem(m_pLeftTabList->currentRow() + 1, pListWidgetItem);
        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(getNewAlbumName(), m_pLeftTabList, pListWidgetItem);


        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);

        m_pLeftTabList->setCurrentRow(m_pLeftTabList->currentRow() + 1);


        AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
        item->m_opeMode = OPE_MODE_ADDNEWALBUM;
        item->editAlbumEdit();

        m_curListWidgetItem = m_pLeftTabList->currentItem();

        m_currentAlbum = item->m_albumNameStr;
        updateRightView();
    }
    break;
    case IdRenameAlbum: {
        AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());

        if (COMMON_STR_RECENT_IMPORTED == item->m_albumNameStr
                || COMMON_STR_TRASH == item->m_albumNameStr
                || COMMON_STR_FAVORITES == item->m_albumNameStr
                || ALBUM_PATHTYPE_BY_PHONE == item->m_albumTypeStr
                || ALBUM_PATHTYPE_BY_U == item->m_albumTypeStr) {
            return;
        }

        item->m_opeMode = OPE_MODE_RENAMEALBUM;
        item->editAlbumEdit();
    }
    break;
    case IdExport: {
        QListWidgetItem *item = m_pLeftTabList->currentItem();
        AlbumLeftTabItem *pTabItem = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(item);
        Exporter::instance()->exportAlbum(DBManager::instance()->getPathsByAlbum(pTabItem->m_albumNameStr), pTabItem->m_albumNameStr);
    }
    break;
    case IdDeleteAlbum: {
//        AlbumDeleteDialog *dialog = new AlbumDeleteDialog();
//        dialog->showInCenter(window());

//        connect(dialog,&AlbumDeleteDialog::deleteAlbum,this,[=]{
        QString str;
        QListWidgetItem *item = m_pLeftTabList->currentItem();
        AlbumLeftTabItem *pTabItem = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(item);

        str = pTabItem->m_albumNameStr;
        DBManager::instance()->removeAlbum(pTabItem->m_albumNameStr);
        delete  item;

//        m_customAlbumNames.removeOne(str);
        updateImportComboBox();

        QModelIndex index;
        emit m_pLeftTabList->clicked(index);

//        QString str1 = "相册：“%1”，已经删除成功";
//        DUtil::DNotifySender *pDNotifySender = new DUtil::DNotifySender("深度相册");
//        pDNotifySender->appName("deepin-album");
//        pDNotifySender->appBody(str1.arg(str));
//        pDNotifySender->call();
//        });

        emit dApp->signalM->sigAlbDelToast(str);

        AlbumLeftTabItem *currentItem = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
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

    QListWidgetItem *pListWidgetItem = new QListWidgetItem();
    m_pLeftTabList->insertItem(index, pListWidgetItem);
    pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
    QString albumName = getNewAlbumName();

    if (QStringList(" ") != imagepaths) {
        DBManager::instance()->insertIntoAlbum(albumName, imagepaths);
    }

    AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName, m_pLeftTabList, pListWidgetItem);
//    m_customAlbumNames << albumName;

    m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);

    m_pLeftTabList->setCurrentRow(index);

    AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
    if (QStringList(" ") != imagepaths) {
        item->m_opeMode = OPE_MODE_ADDRENAMEALBUM;
    } else {
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
    for (auto path : paths) {
        DBImgInfo info;
        info = DBManager::instance()->getTrashInfoByPath(path);
        QFileInfo fi(info.filePath);
        info.changeTime = QDateTime::currentDateTime();
        infos << info;

//        dApp->m_imagetrashmap.remove(path);
    }

    dApp->m_imageloader->addImageLoader(paths);
    DBManager::instance()->insertImgInfos(infos);

    for (auto path : paths) {
        DBImgInfo info;
        info = DBManager::instance()->getTrashInfoByPath(path);
        QStringList namelist = info.albumname.split(",");
        for (auto eachname : namelist) {
            if (DBManager::instance()->isAlbumExistInDB(eachname)) {
                DBManager::instance()->insertIntoAlbum(eachname, QStringList(path));
            }
        }
    }

    DBManager::instance()->removeTrashImgInfos(paths);

    onTrashListClicked();
}

void AlbumView::onTrashDeleteBtnClicked()
{
    QStringList paths;

    if (tr("Delete") == m_pDeleteBtn->text()) {
        paths = m_pRightTrashThumbnailList->selectedPaths();
    } else {
        paths = DBManager::instance()->getAllTrashPaths();
//        m_pDeleteBtn->setEnabled(false);
    }
    ImgDeleteDialog *dialog = new ImgDeleteDialog(this, paths.count());
    dialog->show();
    connect(dialog, &ImgDeleteDialog::imgdelete, this, [ = ] {
//        for (auto path : paths)
//        {
//            dApp->m_imagetrashmap.remove(path);
//        }
        emit dApp->signalM->sigDeletePhotos(paths.length());
        DBManager::instance()->removeTrashImgInfos(paths);
    });

    onTrashListClicked();
}

void AlbumView::openImage(int index)
{
    SignalManager::ViewInfo info;
    info.album = "";
    info.lastPanel = nullptr;

    if (m_curThumbnaiItemList.size() > 1) {
        for (auto image : m_curThumbnaiItemList) {
            info.paths << image.path;
        }
    } else {
        info.paths.clear();
    }
    info.path = m_curThumbnaiItemList[index].path;
    info.viewType = m_currentAlbum;
    info.viewMainWindowID = VIEW_MAINWINDOW_ALBUM;
    emit dApp->signalM->viewImage(info);
    emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALBUM);
}

void AlbumView::menuOpenImage(QString path, QStringList paths, bool isFullScreen, bool isSlideShow)
{
    SignalManager::ViewInfo info;
    info.album = "";
    info.lastPanel = nullptr;
    auto imagelist = DBManager::instance()->getInfosByAlbum(m_currentAlbum);
    if (COMMON_STR_TRASH == m_currentAlbum) {
        imagelist = DBManager::instance()->getAllTrashInfos();
    } else if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum) {
        imagelist = DBManager::instance()->getAllInfos();
    } else {

    }

    if (paths.size() > 1) {
        info.paths = paths;
    } else {
        if (imagelist.size() > 1) {
            for (auto image : imagelist) {
                info.paths << image.filePath;
            }
        } else {
            info.paths.clear();
        }
    }

    info.path = path;
    info.fullScreen = isFullScreen;
    info.slideShow = isSlideShow;
    info.viewType = m_currentAlbum;
    info.viewMainWindowID = VIEW_MAINWINDOW_ALBUM;
    if (info.slideShow) {
        if (imagelist.count() == 1) {
            info.paths = paths;
        }
        emit dApp->signalM->startSlideShow(info);
        emit dApp->signalM->showSlidePanel(VIEW_MAINWINDOW_ALBUM);
    } else {
        emit dApp->signalM->viewImage(info);
        emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALBUM);
    }
}

QString AlbumView::getNewAlbumName()
{
    const QString nan = tr("Unnamed");
    int num = 1;
    QString albumName = nan + QString::number(num);
    while (DBManager::instance()->isAlbumExistInDB(albumName)) {
        num++;
        albumName = nan + QString::number(num);
    }
    return (const QString)(albumName);
}

void AlbumView::onTrashListClicked()
{
    QStringList paths = m_pRightTrashThumbnailList->selectedPaths();
    paths.removeAll(QString(""));

    if (0 < paths.length()) {
        m_pRecoveryBtn->setEnabled(true);
        DPalette ReBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
        ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
        m_pRecoveryBtn->setPalette(ReBtn);

        m_pDeleteBtn->setText(tr("Delete"));
    } else {
        m_pRecoveryBtn->setEnabled(false);
        DPalette ReBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
        ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
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

    if (paths.isEmpty()) {
        return;
    }

    // 判断当前导入路径是否为外接设备
    int isMountFlag = 0;
    DGioVolumeManager *pvfsManager = new DGioVolumeManager;
    QList<QExplicitlySharedDataPointer<DGioMount>> mounts = pvfsManager->getMounts();
    for(auto mount : mounts)
    {
        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
        QString strPath = LocationFile->path();
        if (0 == paths.first().compare(strPath))
        {
            isMountFlag = 1;
            break;
        }
    }

    // 当前导入路径
    if(isMountFlag)
    {
        QString strHomePath = QDir::homePath();
        //获取系统现在的时间
        QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
        QDir dir;
        if (!dir.exists(basePath))
        {
            dir.mkpath(basePath);
        }

        QStringList newImagePaths;
        foreach (QString strPath, paths)
        {
            //取出文件名称
            QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
            QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
            QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(), QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

            newImagePaths << strNewPath;
            //判断新路径下是否存在目标文件，若存在，下一次张
            if (dir.exists(strNewPath))
            {
                continue;
            }

            // 外接设备图片拷贝到系统
            if (QFile::copy(strPath, strNewPath))
            {

            }
        }

        paths.clear();
        paths = newImagePaths;
    }

    DBImgInfoList dbInfos;

    using namespace utils::image;

    for (auto path : paths) {
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
        if(fi.birthTime().isValid())
        {
            dbi.time = fi.birthTime();
        }
        else if (fi.metadataChangeTime().isValid())
        {
            dbi.time = fi.metadataChangeTime();
        }
        else
        {
            dbi.time = QDateTime::currentDateTime();
        }
        dbi.changeTime = QDateTime::currentDateTime();

        dbInfos << dbi;
    }

    if (! dbInfos.isEmpty()) {
        dApp->m_imageloader->ImportImageLoader(dbInfos, m_currentAlbum);
    } else {
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
    if (event->key() == Qt::Key_Delete) {
        QStringList paths = m_pRightThumbnailList->selectedPaths();

        if (0 == paths.length() || 0 == DBManager::instance()->getImgsCountByAlbum(m_currentAlbum)) {
            if (COMMON_STR_RECENT_IMPORTED != m_currentAlbum
                    && COMMON_STR_TRASH != m_currentAlbum
                    && COMMON_STR_FAVORITES != m_currentAlbum
                    && 5 != m_pRightStackWidget->currentIndex()) {
                QString str;
                QListWidgetItem *item = m_pLeftTabList->currentItem();
                AlbumLeftTabItem *pTabItem = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(item);

                str = pTabItem->m_albumNameStr;
                DBManager::instance()->removeAlbum(pTabItem->m_albumNameStr);
                delete  item;

                QModelIndex index;
                emit m_pLeftTabList->clicked(index);

                emit dApp->signalM->sigAlbDelToast(str);

                AlbumLeftTabItem *currentItem = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
                m_currentAlbum = currentItem->m_albumNameStr;

                updateRightView();
            }
        }
#if 1
    } else if (event->key() == Qt::Key_F2) {
        AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
        if (COMMON_STR_RECENT_IMPORTED == item->m_albumNameStr
                || COMMON_STR_TRASH == item->m_albumNameStr
                || COMMON_STR_FAVORITES == item->m_albumNameStr
                || ALBUM_PATHTYPE_BY_PHONE == item->m_albumTypeStr
                || ALBUM_PATHTYPE_BY_U == item->m_albumTypeStr) {
            return;
        }
        item->m_opeMode = OPE_MODE_RENAMEALBUM;
        item->editAlbumEdit();
    }
#endif
}

void AlbumView::onVfsMountChangedAdd(QExplicitlySharedDataPointer<DGioMount> mount)
{
    qDebug() << "onVfsMountChangedAdd() name:" << mount->name();
    Q_UNUSED(mount);
    //TODO:
    //Support android phone, iPhone, and usb devices. Not support ftp, smb mount, non removeable disk now
    QString uri = mount->getRootFile()->uri();
    QString scheme = QUrl(uri).scheme();

    if ((scheme == "file" && mount->canEject()) ||  //usb device
            (scheme == "gphoto2") ||                //phone photo
//            (scheme == "afc") ||                    //iPhone document
            (scheme == "mtp")) {                    //android file
        qDebug() << "mount.name" << mount->name() << " scheme type:" << scheme;

        for (auto mountLoop : m_mounts) {
            QString uriLoop = mountLoop->getRootFile()->uri();
            if (uri == uriLoop) {
                qDebug() << "Already has this device in mount list. uri:" << uriLoop;
                return;
            }
        }

        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
        QString strPath = LocationFile->path();
        if (strPath.isEmpty()) {
            qDebug() << "onVfsMountChangedAdd() strPath.isEmpty()";
            return;
        }

        MountLoader *pMountloader = new MountLoader(this);
        QThread *pLoadThread = new QThread();

        pMountloader->moveToThread(pLoadThread);
        pLoadThread->start();

        connect(pMountloader, SIGNAL(sigLoadMountImagesStart(QString, QString)), pMountloader, SLOT(onLoadMountImagesStart(QString, QString)));

        qDebug() << "onVfsMountChangedAdd() emit pMountloader->sigLoadMountImagesStart()";
//        emit pMountloader->sigLoadMountImagesStart(mount->name(), strPath);

//        m_mountLoaderList.insert(mount->name(), pMountloader);
//        m_loadThreadList.insert(mount->name(), pLoadThread);
        QString rename = "";
        rename = durlAndNameMap[QUrl(mount->getRootFile()->uri())];
        if ("" == rename) {
            rename = mount->name();
        }

        emit pMountloader->sigLoadMountImagesStart(rename, strPath);

        m_mountLoaderList.insert(rename, pMountloader);
        m_loadThreadList.insert(rename, pLoadThread);

        updateExternalDevice(mount);
    }
}

void AlbumView::onVfsMountChangedRemove(QExplicitlySharedDataPointer<DGioMount> mount)
{
    qDebug() << "onVfsMountChangedRemove() mountname" << mount->name();
    Q_UNUSED(mount);

    QString uri = mount->getRootFile()->uri();
    for (auto mountLoop : m_mounts) {
        QString uriLoop = mountLoop->getRootFile()->uri();
        if (uri == uriLoop) {
            m_mounts.removeOne(mountLoop);
        }
    }

    for (int i = 0; i < m_pLeftTabList->count(); i++) {
        QListWidgetItem *pListWidgetItem = m_pLeftTabList->item(i);
        AlbumLeftTabItem *pAlbumLeftTabItem = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(pListWidgetItem);
//        if (mount->name() == pAlbumLeftTabItem->m_albumNameStr) {
        QString rename = "";
        rename = durlAndNameMap[QUrl(mount->getRootFile()->uri())];
        if ("" == rename) {
            rename = mount->name();
        }
        if (rename == pAlbumLeftTabItem->m_albumNameStr) {
            delete pListWidgetItem;
            AlbumLeftTabItem *currentItem = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->currentItem());
            m_currentAlbum = currentItem->m_albumNameStr;

            updateRightView();

            break;
        }
    }
}

void AlbumView::getAllDeviceName()
{
    QStringList blDevList = m_diskManager->blockDevices();
    qDebug() << "blDevList:" << blDevList;
    for (const QString &blks : blDevList) {
        QSharedPointer<DBlockDevice> blk(DDiskManager::createBlockDevice(blks));
        QScopedPointer<DDiskDevice> drv(DDiskManager::createDiskDevice(blk->drive()));

        if (!blk->hasFileSystem() && !drv->mediaCompatibility().join(" ").contains("optical") && !blk->isEncrypted()) {
            continue;
        }
        if ((blk->hintIgnore() && !blk->isEncrypted()) || blk->cryptoBackingDevice().length() > 1) {
            continue;
        }
        DBlockDevice *pblk = blk.data();
        QByteArrayList mps = blk->mountPoints();
        qulonglong size = blk->size();
        QString label = blk->idLabel();
        QString fs = blk->idType();
        QString udispname = "";

        if (label.startsWith(ddeI18nSym)) {
            QString i18nKey = label.mid(ddeI18nSym.size(), label.size() - ddeI18nSym.size());
            udispname = qApp->translate("DeepinStorage", i18nMap.value(i18nKey, i18nKey.toUtf8().constData()));
            goto runend1;
//            blk->mount({});
//            return;
        }

        if (mps.contains(QByteArray("/\0", 2))) {
            udispname = QCoreApplication::translate("PathManager", "System Disk");
            goto runend1;
//            blk->mount({});
//            return;
        }
        if (label.length() == 0) {
            QScopedPointer<DDiskDevice> drv(DDiskManager::createDiskDevice(pblk->drive()));
            if (!drv->mediaAvailable() && drv->mediaCompatibility().join(" ").contains("optical")) {
                QString maxmediacompat;
                for (auto i = opticalmediakv.rbegin(); i != opticalmediakv.rend(); ++i) {
                    if (drv->mediaCompatibility().contains(i->first)) {
                        maxmediacompat = i->second;
                        break;
                    }
                }
                udispname = QCoreApplication::translate("DeepinStorage", "%1 Drive").arg(maxmediacompat);
                goto runend1;
                //            blk->mount({});
                //            return;
            }
            if (drv->opticalBlank()) {
                udispname = QCoreApplication::translate("DeepinStorage", "Blank %1 Disc").arg(opticalmediamap[drv->media()]);
                goto runend1;
                //            blk->mount({});
                //            return;
            }
            if (pblk->isEncrypted() && !blk) {
                udispname = QCoreApplication::translate("DeepinStorage", "%1 Encrypted").arg(formatSize(size));
                goto runend1;
                //            blk->mount({});
                //            return;
            }
//            udispname = QCoreApplication::translate("DeepinStorage", "%1 Volume").arg(formatSize(size));
            udispname = QCoreApplication::translate("DeepinStorage", "%1 ").arg(formatSize(size));
            udispname += tr("卷");
            goto runend1;
//            blk->mount({});
//            return;
        }
        udispname = label;

runend1:
        blk->mount({});
        QByteArrayList qbl = blk->mountPoints();
        QString mountPoint = "file://";
        for (QByteArray qb : qbl) {
            mountPoint += qb;
        }
        qDebug() << "mountPoint:" << mountPoint;
        QUrl qurl(mountPoint);
        durlAndNameMap[qurl] = udispname;
    }
}

const QList<QExplicitlySharedDataPointer<DGioMount> > AlbumView::getVfsMountList()
{
    getAllDeviceName();
    QList<QExplicitlySharedDataPointer<DGioMount> > result;
    const QList<QExplicitlySharedDataPointer<DGioMount> > mounts = m_vfsManager->getMounts();

    for (auto mount : mounts) {

        //TODO:
        //Support android phone, iPhone, and usb devices. Not support ftp, smb, non removeable disk now
        QString scheme = QUrl(mount->getRootFile()->uri()).scheme();

        if ((scheme == "file" && mount->canEject()) ||  //usb device
                (scheme == "gphoto2") ||                //phone photo
                //            (scheme == "afc") ||                    //iPhone document
                (scheme == "mtp")) {                    //android file
            qDebug() << "getVfsMountList() mount.name" << mount->name() << " scheme type:" << scheme;
            result.append(mount);
        } else {
            qDebug() <<  mount->name() << " scheme type:" << scheme << "is not supported by album.";
        }
    }

    return result;
}

void AlbumView::loadMountPicture(QString path)
{
    //判断路径是否存在
    QDir dir(path);
    if (!dir.exists()) return;

    //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
    if (!path.contains("/media/")) {
        bool bFind = findPicturePathByPhone(path);
        if (!bFind) return;
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
            } else if (path.contains(".tga")) {
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
        if (pixmap.isNull()) {
            pixmap = QPixmap(":/resources/images/other/deepin-album.svg");
        }

        pixmap.scaledToHeight(100,  Qt::FastTransformation);
        if (pixmap.isNull()) {
            pixmap = QPixmap::fromImage(tImg);
        }

        m_phonePicMap.insert(fileInfo.filePath(), pixmap);
    }
}

void AlbumView::initLeftMenu()
{
    m_MenuActionMap.clear();


    appendAction(IdStartSlideShow, tr("Slide show"), ss(""));
    m_pLeftMenu->addSeparator();

    appendAction(IdCreateAlbum, tr("New Album"), ss(""));
    m_pLeftMenu->addSeparator();

    appendAction(IdRenameAlbum, tr("Rename"), ss("COMMON_STR_RENAMEALBUM"));
    m_pLeftMenu->addSeparator();

    appendAction(IdExport, tr("Export"), ss(""));
    m_pLeftMenu->addSeparator();

    appendAction(IdDeleteAlbum, tr("Delete"), ss(""));
}

void AlbumView::importComboBoxChange(QString strText)
{
    if (1 == m_importByPhoneComboBox->currentIndex()) {
        AlbumCreateDialog *dialog = new AlbumCreateDialog;
        dialog->show();
        qDebug() << "xxxxxxxxxx" << window()->x();
        qDebug() << "xxxxxxxxxx" << window()->y();
        qDebug() << "xxxxxxxxxx" << dialog->width();
        qDebug() << "xxxxxxxxxx" << window()->width();
        dialog->move(window()->x() + (window()->width() - dialog->width()) / 2, window()->y() + (window()->height() - dialog->height()) / 2);
        connect(dialog, &AlbumCreateDialog::albumAdded, this, [ = ] {
            DBManager::instance()->insertIntoAlbum(dialog->getCreateAlbumName(), QStringList(" "));
            int index = getNewAlbumItemIndex();
            QListWidgetItem *pListWidgetItem = new QListWidgetItem();
            m_pLeftTabList->insertItem(index, pListWidgetItem);
            pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
            QString albumName = dialog->getCreateAlbumName();
            AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName, m_pLeftTabList, pListWidgetItem);
            pAlbumLeftTabItem->oriAlbumStatus();
            m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
//            m_customAlbumNames << albumName;
            updateImportComboBox();
            m_importByPhoneComboBox->setCurrentIndex(m_importByPhoneComboBox->count() - 1);
        });

        connect(dialog, &AlbumCreateDialog::sigClose, this, [ = ] {
            m_importByPhoneComboBox->setCurrentIndex(0);
        });
    }
}

bool AlbumView::findPictureFile(QString &path, QList<ThumbnailListView::ItemInfo> &thumbnaiItemList)
{
    //判断路径是否存在
    QDir dir(path);
    if (!dir.exists()) return false;

    //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
    if (!path.contains("/media/")) {
        bool bFind = findPicturePathByPhone(path);
        if (!bFind) return  false;
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
    for (auto mount : m_mounts) {
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftTabList);
        //pListWidgetItem缓存文件挂载路径
        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
        QString strPath = LocationFile->path();
        pListWidgetItem->setData(Qt::UserRole, strPath);
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
        AlbumLeftTabItem *pAlbumLeftTabItem;
        QString rename = "";
        rename = durlAndNameMap[QUrl(mount->getRootFile()->uri())];
        if ("" == rename) {
            rename = mount->name();
        }
        if (strPath.contains("/media/")) {
//            pAlbumLeftTabItem = new AlbumLeftTabItem(mount->name(), m_pLeftTabList, pListWidgetItem, ALBUM_PATHTYPE_BY_U);
            pAlbumLeftTabItem = new AlbumLeftTabItem(rename, m_pLeftTabList, pListWidgetItem, ALBUM_PATHTYPE_BY_U);
        } else {
//            pAlbumLeftTabItem = new AlbumLeftTabItem(mount->name(), m_pLeftTabList, pListWidgetItem, ALBUM_PATHTYPE_BY_PHONE);
            pAlbumLeftTabItem = new AlbumLeftTabItem(rename, m_pLeftTabList, pListWidgetItem, ALBUM_PATHTYPE_BY_PHONE);
        }

        pAlbumLeftTabItem->setExternalDevicesMountPath(strPath);
        connect(pAlbumLeftTabItem, &AlbumLeftTabItem::unMountExternalDevices, this, &AlbumView::onUnMountSignal);
        m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);

        MountLoader *pMountloader = new MountLoader(this);
        QThread *pLoadThread = new QThread();

        pMountloader->moveToThread(pLoadThread);
        pLoadThread->start();

        connect(pMountloader, SIGNAL(sigLoadMountImagesStart(QString, QString)), pMountloader, SLOT(onLoadMountImagesStart(QString, QString)));
//        emit pMountloader->sigLoadMountImagesStart(mount->name(), strPath);
        emit pMountloader->sigLoadMountImagesStart(rename, strPath);


//        m_mountLoaderList.insert(mount->name(), pMountloader);
//        m_loadThreadList.insert(mount->name(), pLoadThread);
        m_mountLoaderList.insert(rename, pMountloader);
        m_loadThreadList.insert(rename, pLoadThread);
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
    QString rename = "";
    rename = durlAndNameMap[QUrl(mount->getRootFile()->uri())];
    if ("" == rename) {
        rename = mount->name();
    }
    if (strPath.contains("/media/")) {
//        pAlbumLeftTabItem = new AlbumLeftTabItem(mount->name(), m_pLeftTabList, pListWidgetItem, ALBUM_PATHTYPE_BY_U);
        pAlbumLeftTabItem = new AlbumLeftTabItem(rename, m_pLeftTabList, pListWidgetItem, ALBUM_PATHTYPE_BY_U);
    } else {
//        pAlbumLeftTabItem = new AlbumLeftTabItem(mount->name(), m_pLeftTabList, pListWidgetItem, ALBUM_PATHTYPE_BY_PHONE);
        pAlbumLeftTabItem = new AlbumLeftTabItem(rename, m_pLeftTabList, pListWidgetItem, ALBUM_PATHTYPE_BY_PHONE);
    }

    pAlbumLeftTabItem->setExternalDevicesMountPath(strPath);
    connect(pAlbumLeftTabItem, &AlbumLeftTabItem::unMountExternalDevices, this, &AlbumView::onUnMountSignal);
    m_pLeftTabList->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    m_mounts.append(mount);
}

void AlbumView::onUpdataAlbumRightTitle(QString titlename)
{
//    int index = m_customAlbumNames.indexOf(m_currentAlbum);
    m_currentAlbum = titlename;
//    m_customAlbumNames.replace(index, m_currentAlbum);
    updateRightView();
}

void AlbumView::SearchReturnUpdate()
{
    if (RIGHT_VIEW_SEARCH == m_pRightStackWidget->currentIndex()) {
        if (COMMON_STR_TRASH == m_currentAlbum) {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TRASH_LIST);
        } else if (COMMON_STR_FAVORITES == m_currentAlbum) {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_FAVORITE_LIST);
        } else if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum) {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TIMELINE_IMPORT);
        } else {
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
        if (tempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0) {
            path = tempFileInfo.absoluteFilePath();
            return true;
        } else {
            QDir subDir;
            subDir.setPath(tempFileInfo.absoluteFilePath());

            QFileInfoList subFileInfoList = subDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            QFileInfo subTempFileInfo;
            foreach (subTempFileInfo, subFileInfoList) {
                if (subTempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0) {
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
    m_importByPhoneComboBox->addItem(tr("Album Gallery"));
    m_importByPhoneComboBox->addItem(tr("New Album"));
    QStringList allAlbumNames = DBManager::instance()->getAllAlbumNames();
    qDebug() << "updateImportComboBox()" << allAlbumNames;
    for (auto albumName : allAlbumNames) {
        if (COMMON_STR_FAVORITES == albumName || COMMON_STR_RECENT_IMPORTED == albumName || COMMON_STR_TRASH == albumName) {
            continue;
        }

        m_importByPhoneComboBox->addItem(albumName);
    }
}

//手机照片全部导入
void AlbumView::importAllBtnClicked()
{
    QList<ThumbnailListView::ItemInfo> allPaths = m_pRightPhoneThumbnailList->getAllPaths();
    QString albumNameStr = m_importByPhoneComboBox->currentText();
    QStringList picPathList;
    QStringList newPathList;
    DBImgInfoList dbInfos;
    QString strHomePath = QDir::homePath();
    //获取系统现在的时间
    QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
    QDir dir;
    if (!dir.exists(basePath)) {
        dir.mkpath(basePath);
    }

    foreach (ThumbnailListView::ItemInfo info, allPaths) {
        QString strPath = info.path;
        QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
        QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
        QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(), QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

        //判断新路径下是否存在目标文件，若存在，先删除掉
        if (dir.exists(strNewPath)) {
            dir.remove(strNewPath);
        }

//        if (QFile::copy(strPath, strNewPath)) {
        picPathList << strPath;
        newPathList << strNewPath;
        QFileInfo fi(strPath);
        DBImgInfo dbi;
        dbi.fileName = fi.fileName();
        dbi.filePath = strNewPath;
        dbi.dirHash = utils::base::hash(QString());
        if(fi.birthTime().isValid())
        {
            dbi.time = fi.birthTime();
        }
        else if (fi.metadataChangeTime().isValid())
        {
            dbi.time = fi.metadataChangeTime();
        }
        else
        {
            dbi.time = QDateTime::currentDateTime();
        }
        dbi.changeTime = QDateTime::currentDateTime();

        dbInfos << dbi;
//        }
    }

    MountLoader *pMountloader = new MountLoader(this);
    QThread *pLoadThread = new QThread();

    pMountloader->moveToThread(pLoadThread);
    pLoadThread->start();

    connect(pMountloader, SIGNAL(sigCopyPhotoFromPhone(QStringList, QStringList)), pMountloader, SLOT(onCopyPhotoFromPhone(QStringList, QStringList)));
    emit pMountloader->sigCopyPhotoFromPhone(picPathList, newPathList);

    if (!dbInfos.isEmpty()) {
        DBImgInfoList dbInfoList;
        QStringList pathslist;

        for (int i = 0; i < dbInfos.length(); i++) {
            if (m_phonePathAndImage.value(picPathList[i]).isNull()) {
                continue;
            }

            dApp->m_imagemap.insert(dbInfos[i].filePath, m_phonePathAndImage.value(picPathList[i]));

            pathslist << dbInfos[i].filePath;
            dbInfoList << dbInfos[i];
        }

        if (albumNameStr.length() > 0) {
            if (COMMON_STR_RECENT_IMPORTED != albumNameStr
                    && COMMON_STR_TRASH != albumNameStr
                    && COMMON_STR_FAVORITES != albumNameStr
                    && ALBUM_PATHTYPE_BY_PHONE != albumNameStr
                    && 0 != albumNameStr.compare(tr("Album Gallery"))) {
                DBManager::instance()->insertIntoAlbumNoSignal(albumNameStr, pathslist);
            }
        }

        DBManager::instance()->insertImgInfos(dbInfoList);

        if (dbInfoList.length() != allPaths.length()) {
            emit dApp->signalM->ImportSomeFailed();
        } else {
            emit dApp->signalM->ImportSuccess();
        }
    } else {
        emit dApp->signalM->ImportFailed();
    }

    for (int i = 0; i < m_pLeftTabList->count(); i++) {
        QListWidgetItem *pListWidgetItem = m_pLeftTabList->item(i);
        AlbumLeftTabItem *pAlbumLeftTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftTabList->itemWidget(pListWidgetItem));
        if (!pAlbumLeftTabItem) continue;
        if (albumNameStr == pAlbumLeftTabItem->m_albumNameStr) {
            m_pLeftTabList->setCurrentRow(i);
            break;
        }
    }
}

//手机照片导入选中
void AlbumView::importSelectBtnClicked()
{
    QStringList selectPaths = m_pRightPhoneThumbnailList->selectedPaths();
    QString albumNameStr = m_importByPhoneComboBox->currentText();
    QStringList picPathList;
    QStringList newPathList;
    DBImgInfoList dbInfos;
    QString strHomePath = QDir::homePath();
    //获取系统现在的时间
    QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
    QDir dir;
    if (!dir.exists(basePath)) {
        dir.mkpath(basePath);
    }

    foreach (QString strPath, selectPaths) {
        //取出文件名称
        QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
        QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
        QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(), QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

        //判断新路径下是否存在目标文件，若存在，先删除掉
        if (dir.exists(strNewPath)) {
            dir.remove(strNewPath);
        }

//        if (QFile::copy(strPath, strNewPath)) {
        picPathList << strPath;
        newPathList << strNewPath;

        QFileInfo fi(strNewPath);
        DBImgInfo dbi;
        dbi.fileName = fi.fileName();
        dbi.filePath = strNewPath;
        dbi.dirHash = utils::base::hash(QString());
        if(fi.birthTime().isValid())
        {
            dbi.time = fi.birthTime();
        }
        else if (fi.metadataChangeTime().isValid())
        {
            dbi.time = fi.metadataChangeTime();
        }
        else
        {
            dbi.time = QDateTime::currentDateTime();
        }

        dbi.changeTime = QDateTime::currentDateTime();

        dbInfos << dbi;
//        }
    }

    MountLoader *pMountloader = new MountLoader(this);
    QThread *pLoadThread = new QThread();

    pMountloader->moveToThread(pLoadThread);
    pLoadThread->start();

    connect(pMountloader, SIGNAL(sigCopyPhotoFromPhone(QStringList, QStringList)), pMountloader, SLOT(onCopyPhotoFromPhone(QStringList, QStringList)));
    emit pMountloader->sigCopyPhotoFromPhone(picPathList, newPathList);

    if (!dbInfos.isEmpty()) {
        DBImgInfoList dbInfoList;
        QStringList pathslist;

        for (int i = 0; i < dbInfos.length(); i++) {
            if (m_phonePathAndImage.value(picPathList[i]).isNull()) {
                continue;
            }

            dApp->m_imagemap.insert(dbInfos[i].filePath, m_phonePathAndImage.value(picPathList[i]));

            pathslist << dbInfos[i].filePath;
            dbInfoList << dbInfos[i];
        }

        if (albumNameStr.length() > 0) {
            if (COMMON_STR_RECENT_IMPORTED != albumNameStr
                    && COMMON_STR_TRASH != albumNameStr
                    && COMMON_STR_FAVORITES != albumNameStr
                    && ALBUM_PATHTYPE_BY_PHONE != albumNameStr
                    && 0 != albumNameStr.compare(tr("Album Gallery"))) {
                DBManager::instance()->insertIntoAlbumNoSignal(albumNameStr, pathslist);
            }
        }

        DBManager::instance()->insertImgInfos(dbInfoList);

        if (dbInfoList.length() != selectPaths.length()) {
            emit dApp->signalM->ImportSomeFailed();
        } else {
            emit dApp->signalM->ImportSuccess();
        }
    } else {
        emit dApp->signalM->ImportFailed();
    }

    for (int i = 0; i < m_pLeftTabList->count(); i++) {
        QListWidgetItem *pListWidgetItem = m_pLeftTabList->item(i);
        AlbumLeftTabItem *pAlbumLeftTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftTabList->itemWidget(pListWidgetItem));
        if (!pAlbumLeftTabItem) continue;
        if (albumNameStr == pAlbumLeftTabItem->m_albumNameStr) {
            m_pLeftTabList->setCurrentRow(i);
            break;
        }
    }
}

int AlbumView::getNewAlbumItemIndex()
{
    int count = m_pLeftTabList->count();
    for (int i = 0; i < count; ++i) {
        QString strPath = m_pLeftTabList->item(i)->data(Qt::UserRole).toString();
        if (!strPath.isEmpty()) {
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
        if (LocationFile->path().compare(unMountPath) == 0 && mount->canUnmount()) {
            mount->unmount(true);
            durlAndNameMap.erase(durlAndNameMap.find(QUrl(mount->getRootFile()->uri())));
            m_mounts.removeOne(mount);
            break;
        }
    }
}

void AlbumView::onLoadMountImagesEnd(QString mountname)
{
    qDebug() << "onLoadMountImagesEnd() mountname: " << mountname;
    qDebug() << "onLoadMountImagesEnd() m_currentAlbum: " << m_currentAlbum;

    if (mountname == m_currentAlbum) {
        updateRightView();
    }
}

void AlbumView::onLeftListDropEvent(QModelIndex dropIndex)
{
    qDebug() << "AlbumView::onLeftListDropEvent()";
    ThumbnailListView *currentViewList;
    QStringList dropItemPaths;

    AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftTabList->itemWidget(m_pLeftTabList->item(dropIndex.row()));
    QString dropLeftTabListName = item->m_albumNameStr;
    qDebug() << "currentAlbum: " << m_currentAlbum << " ;dropLeftTabListName: " << dropLeftTabListName;

    //向自己的相册或“已导入”相册拖拽无效
    //“已导入”相册在leftlistwidget.cpp中也屏蔽过
    if ((m_currentAlbum == dropLeftTabListName) /*|| (COMMON_STR_RECENT_IMPORTED == dropLeftTabListName)*/ || 5 == m_pRightStackWidget->currentIndex()) {
        qDebug() << "Can not drop!";
        return;
    }

    if (COMMON_STR_FAVORITES == m_currentAlbum)
    {
        currentViewList = m_pRightFavoriteThumbnailList;
        dropItemPaths = currentViewList->getDagItemPath();
    }
    else if (COMMON_STR_TRASH == m_currentAlbum)
    {
        currentViewList = m_pRightTrashThumbnailList;
        dropItemPaths = currentViewList->getDagItemPath();
    }
    else if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum)
    {
        dropItemPaths = m_pImpTimeLineWidget->selectPaths();
    }
    else
    {
        currentViewList = m_pRightThumbnailList;
        dropItemPaths = currentViewList->getDagItemPath();
    }

    qDebug() << "dropItemPaths: " << dropItemPaths;

    if (COMMON_STR_TRASH == dropLeftTabListName) {
        //向回收站拖拽，动作删除
        //回收站在leftlistwidget.cpp中屏蔽掉了
    } else {
        //向其他相册拖拽，动作添加
        DBManager::instance()->insertIntoAlbum(item->m_albumNameStr, dropItemPaths);
    }

}

void AlbumView::updatePicNum()
{
    QString str = tr("%1 photo(s) selected");
    int selPicNum = 0;

    if (4 == m_pRightStackWidget->currentIndex()) {
        QStringList paths = m_pSearchView->m_pThumbnailListView->selectedPaths();
        selPicNum = paths.length();
    } else {
        if (m_currentAlbum == COMMON_STR_TRASH) {
            QStringList paths = m_pRightTrashThumbnailList->selectedPaths();
            selPicNum = paths.length();
        } else if (m_currentAlbum == COMMON_STR_FAVORITES) {
            QStringList paths = m_pRightFavoriteThumbnailList->selectedPaths();
            selPicNum = paths.length();
        } else if (m_currentAlbum == COMMON_STR_RECENT_IMPORTED) {
            QStringList paths = m_pImpTimeLineWidget->selectPaths();
            selPicNum = paths.length();
        } else if (5 == m_pRightStackWidget->currentIndex()) {
            QStringList paths = m_pRightPhoneThumbnailList->selectedPaths();
            selPicNum = paths.length();
        } else {
            QStringList paths = m_pRightThumbnailList->selectedPaths();
            selPicNum = paths.length();
        }
    }

    if (0 < selPicNum) {
        m_pStatusBar->m_pAllPicNumLabel->setText(str.arg(QString::number(selPicNum)));
    } else {
        restorePicNum();
    }
}

void AlbumView::restorePicNum()
{
    QString str = tr("%1 photo(s)");
    int selPicNum = 0;

    if (4 == m_pRightStackWidget->currentIndex()) {
        selPicNum = m_pSearchView->m_searchPicNum;
    } else {
        if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum) {
            selPicNum = DBManager::instance()->getImgsCount();
        } else if (COMMON_STR_TRASH == m_currentAlbum) {
            selPicNum = DBManager::instance()->getTrashImgsCount();
        } else if (COMMON_STR_FAVORITES == m_currentAlbum) {
            selPicNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);
        } else {
            if (5 == m_pRightStackWidget->currentIndex()) {
                selPicNum = m_mountPicNum;
            } else {
                selPicNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);
            }
        }
    }

    m_pStatusBar->m_pAllPicNumLabel->setText(str.arg(QString::number(selPicNum)));
}

MountLoader::MountLoader(AlbumView *parent)
{
    m_parent = parent;
}

void MountLoader::onLoadMountImagesStart(QString mountName, QString path)
{
    qDebug() << "onLoadMountImagesStart() mountName: " << mountName;
    qDebug() << "onLoadMountImagesStart() path: " << path;
    QString strPath = path;
    //判断路径是否存在
    QDir dir(path);
    if (!dir.exists()) {
        qDebug() << "onLoadMountImagesStart() !dir.exists()";
        dApp->signalM->sigLoadMountImagesEnd(mountName);
        return;
    }

    //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
    if (!path.contains("/media/")) {
        bool bFind = findPicturePathByPhone(path);
        if (!bFind) {
            qDebug() << "onLoadMountImagesStart() !bFind";
            dApp->signalM->sigLoadMountImagesEnd(mountName);
            return;
        }
    }

    //获取所选文件类型过滤器
    QStringList filters;
    filters << QString("*.jpeg") << QString("*.jpg")
            << QString("*.bmp") << QString("*.png")
            << QString("*.ppm") << QString("*.xbm")
            << QString("*.xpm") << QString("*.gif")
            ;

    //定义迭代器并设置过滤器
    QDirIterator dir_iterator(path,
                              filters,
                              QDir::Files | QDir::NoSymLinks,
                              QDirIterator::Subdirectories);

    m_phoneImgPathList.clear();

    qDebug() << "onLoadMountImagesStart() while (dir_iterator.hasNext())";
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
            } else if (path.contains(".tga")) {
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
        if (pixmap.isNull()) {
            qDebug() << "pixmap.isNull()";
            continue;
        }

        pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
        if (pixmap.isNull()) {
            pixmap = QPixmap::fromImage(tImg);
        }

        m_phonePathImage.insert(fileInfo.filePath(), pixmap);

        m_phoneImgPathList << fileInfo.filePath();

        if (0 == m_phoneImgPathList.length() % 50) {
            m_parent->m_phonePathAndImage = m_phonePathImage;
            m_parent->m_phoneNameAndPathlist.insert(strPath, m_phoneImgPathList);
            dApp->signalM->sigLoadMountImagesEnd(mountName);
        }
    }

    qDebug() << "onLoadMountImagesStart() m_phoneImgPathList.length()" << m_phoneImgPathList.length();
    if (0 < m_phoneImgPathList.length()) {
        m_parent->m_phonePathAndImage = m_phonePathImage;
        m_parent->m_phoneNameAndPathlist.insert(strPath, m_phoneImgPathList);
        qDebug() << "onLoadMountImagesStart() strPath:" << strPath;
    }

    dApp->signalM->sigLoadMountImagesEnd(mountName);
}

void MountLoader::onCopyPhotoFromPhone(QStringList phonepaths, QStringList systempaths)
{
    for(int i = 0; i< phonepaths.length(); i++)
    {
        if(QFile::copy(phonepaths[i], systempaths[i]))
        {
            qDebug()<<"onCopyPhotoFromPhone()";
        }
    }
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
        if (tempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0) {
            path = tempFileInfo.absoluteFilePath();
            return true;
        } else {
            QDir subDir;
            subDir.setPath(tempFileInfo.absoluteFilePath());

            QFileInfoList subFileInfoList = subDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            QFileInfo subTempFileInfo;
            foreach (subTempFileInfo, subFileInfoList) {
                if (subTempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0) {
                    path = subTempFileInfo.absoluteFilePath();
                    return true;
                }
            }
            return false;
        }
    }

    return false;
}

void AlbumView::resizeEvent(QResizeEvent *e)
{
    m_pImpTimeLineWidget->setFixedWidth(width() - 181);
    m_pImpTimeLineWidget->setFixedHeight(height() - 75);
    m_pwidget->setFixedWidth(160);
    m_pwidget->setFixedHeight(54);
    m_pwidget->move(this->width() / 2 - 80, this->height() - 81);
}
