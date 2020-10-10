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
#include "utils/unionimage.h"
#include <QDirIterator>
#include <DComboBox>
#include "widgets/dialogs/imgdeletedialog.h"
#include "imageengine/imageengineapi.h"
#include <QShortcut>
#include <DWarningButton>
#include <QGraphicsOpacityEffect>
#include <DToast>
#include "dmessagemanager.h"
#include "dialogs/albumcreatedialog.h"
#include <QScrollBar> //add 3975
#include <QMutex>
#include <DMessageBox>

#include "imageengine/imageengineapi.h"
#include "imageengine/imageenginethread.h"

static QMutex m_mutex;

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
const int RIGHT_VIEW_WIDTH = 1119;
const int RIGHT_VIEW_IMPORT = 0;
const int RIGHT_VIEW_THUMBNAIL_LIST = 1;
const int RIGHT_VIEW_TRASH_LIST = 2;
const int RIGHT_VIEW_FAVORITE_LIST = 3;
const int RIGHT_VIEW_SEARCH = 4;
const int RIGHT_VIEW_PHONE = 5;
const int RIGHT_VIEW_TIMELINE_IMPORT = 6;
const int VIEW_MAINWINDOW_ALBUM = 2;

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

}  //namespace

using namespace utils::common;
Q_DECLARE_METATYPE(QExplicitlySharedDataPointer<DGioMount>)

AlbumViewList::AlbumViewList(QWidget *parent)
    : DListWidget(parent), m_scrollbartopdistance(130), m_scrollbarbottomdistance(27)
{
    connect(this->verticalScrollBar(), &QScrollBar::rangeChanged, this, [ = ](int min, int max) {
        Q_UNUSED(min);
        Q_UNUSED(max);
        QScrollBar *bar = this->verticalScrollBar();
        bar->setGeometry(bar->x(), /*bar->y() + */m_scrollbartopdistance, bar->width(), this->height() - m_scrollbartopdistance - m_scrollbarbottomdistance);
    });
}

void AlbumViewList::paintEvent(QPaintEvent *e)
{
    QListWidget::paintEvent(e);
    QScrollBar *bar = this->verticalScrollBar();
    bar->setGeometry(bar->x(), /*bar->y() + */m_scrollbartopdistance, bar->width(), this->height() - m_scrollbartopdistance - m_scrollbarbottomdistance);
}

AlbumView::AlbumView()
    : m_iAlubmPicsNum(0), m_currentAlbum(COMMON_STR_RECENT_IMPORTED)
    , m_currentType(COMMON_STR_RECENT_IMPORTED), m_selPicNum(0), m_itemClicked(false)
    , m_pRightStackWidget(nullptr), m_pLeftListView(nullptr), m_pStatusBar(nullptr)
    , m_pRightWidget(nullptr), m_pRightPhoneThumbnailList(nullptr), m_pwidget(nullptr)
    , m_pRightThumbnailList(nullptr), m_pRightTrashThumbnailList(nullptr), m_pRightFavoriteThumbnailList(nullptr)
    , pImportTimeLineWidget(nullptr), m_pTrashWidget(nullptr), m_pFavoriteWidget(nullptr), m_waitDeviceScandialog(nullptr)
    , m_pImportView(nullptr), m_pRecoveryBtn(nullptr), m_pDeleteBtn(nullptr)
    , m_pRightTitle(nullptr), m_pRightPicTotal(nullptr), m_pImportPicTotal(nullptr)
    , m_pFavoriteTitle(nullptr), m_pFavoritePicTotal(nullptr), m_pPhoneTitle(nullptr)
    , m_pPhonePicTotal(nullptr), m_pSearchView(nullptr), m_vfsManager(nullptr)
    , m_diskManager(nullptr), pLabel1(nullptr), pLabel2(nullptr)
    , m_pImpTimeLineWidget(nullptr), m_importByPhoneWidget(nullptr), m_importByPhoneComboBox(nullptr)
    , m_importAllByPhoneBtn(nullptr), m_importSelectByPhoneBtn(nullptr), m_mountPicNum(0)
    , m_mountloader(nullptr), m_LoadThread(nullptr), m_spinner(nullptr)
    , m_pImportTitle(nullptr), m_noTrashItem(nullptr), m_pNoTrashTitle(nullptr)
    , m_pNoTrashWidget(nullptr), m_FavoriteItem(nullptr), m_FavoriteTitle(nullptr)
    , m_TrashitemItem(nullptr), m_TrashTitle(nullptr), fatherwidget(nullptr)
    , pPhoneWidget(nullptr), phonetopwidget(nullptr)
    , isIgnore(true), m_waitDailog_timer(nullptr)
    , m_updateMountViewThread(nullptr), isMountThreadRunning(false), m_currentViewPictureCount(0)

{
    m_iAlubmPicsNum = DBManager::instance()->getImgsCount();
    m_vfsManager = new DGioVolumeManager;
    m_diskManager = new DDiskManager(this);
    m_diskManager->setWatchChanges(true);
    durlAndNameMap.clear();

    iniWaitDiolag();

    connect(dApp->signalM, &SignalManager::sigLoadMountImagesEnd, this, &AlbumView::onLoadMountImagesEnd);

    fatherwidget = new DWidget(this);
    fatherwidget->setFixedSize(this->size());
    setAcceptDrops(true);

    initLeftView();
    initRightView();

    DWidget *leftwidget = new DWidget;
    leftwidget->setFixedWidth(180);

    DWidget *lefttopwidget = new DWidget;
    lefttopwidget->setFixedHeight(45);

    QVBoxLayout *pvLayout = new QVBoxLayout();
    pvLayout->setContentsMargins(0, 0, 0, 0);
    leftwidget->setLayout(pvLayout);
    pvLayout->addWidget(lefttopwidget);
    pvLayout->addWidget(m_pLeftListView);

    QHBoxLayout *pLayout = new QHBoxLayout();
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->addWidget(leftwidget);
    pLayout->addWidget(m_pRightWidget);
    fatherwidget->setLayout(pLayout);

    initConnections();
    m_pwidget = new DWidget(this);
    m_pwidget->setAttribute(Qt::WA_TransparentForMouseEvents);

    m_spinner = new DSpinner(this);
    m_spinner->setFixedSize(40, 40);
    m_spinner->hide();
}

AlbumView::~AlbumView()
{
    emit dApp->signalM->sigDevStop("");
    m_pRightThumbnailList->stopLoadAndClear();
    m_pRightPhoneThumbnailList->stopLoadAndClear();
    m_pRightTrashThumbnailList->stopLoadAndClear();
    m_pRightFavoriteThumbnailList->stopLoadAndClear();

    ImageEngineImportObject::clearAndStopThread();
    ImageMountGetPathsObject::clearAndStopThread();
    ImageMountImportPathsObject::clearAndStopThread();
//    m_pAllPicture->stopLoadAndClear();
    //ImageEngineObject::clearAndStopThread();
    m_pImpTimeLineWidget->getFatherStatusBar(nullptr);
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
    qRegisterMetaType<DBImgInfoList>("DBImgInfoList &");
    connect(m_pRightFavoriteThumbnailList, &ThumbnailListView::needResize, this, [ = ](int h) {
        if (!m_pRightFavoriteThumbnailList->checkResizeNum())
            return ;
        if (isVisible()) {
            int mh = h;
            m_pRightFavoriteThumbnailList->setFixedHeight(mh + 27);
            m_FavoriteItem->setSizeHint(m_pRightFavoriteThumbnailList->size());
        }

    });
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::needResize, this, [ = ](int h) {
        if (!m_pRightTrashThumbnailList->checkResizeNum())
            return ;
        if (isVisible()) {
            int mh = h;
            m_pRightTrashThumbnailList->setFixedHeight(mh + 27);
            m_TrashitemItem->setSizeHint(m_pRightTrashThumbnailList->size());
        }

    });
    connect(m_pRightThumbnailList, &ThumbnailListView::needResize, this, [ = ](int h) {
        if (!m_pRightThumbnailList->checkResizeNum())
            return ;
        if (isVisible()) {
            int mh = h;
            m_pRightThumbnailList->setFixedHeight(mh + 27);
            m_noTrashItem->setSizeHint(m_pRightThumbnailList->size());
        }
    });
    connect(m_pLeftListView, &LeftListView::itemClicked, this, &AlbumView::leftTabClicked);
    connect(dApp->signalM, &SignalManager::sigCreateNewAlbumFromDialog, this, &AlbumView::onCreateNewAlbumFromDialog);
#if 1
    connect(dApp->signalM, &SignalManager::sigCreateNewAlbumFrom, this, &AlbumView::onCreateNewAlbumFrom);
    connect(m_pRightThumbnailList, &ThumbnailListView::sigMouseMove, this, [ = ] {
        updatePicNum();
    });
    connect(m_pRightFavoriteThumbnailList, &ThumbnailListView::sigMouseMove, this, [ = ] {
        updatePicNum();
    });
    //解决最近删除页面ctrl+all选择所有图片，恢复按钮为灰色的问题
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::sigSelectAll, this, [ = ] {
        m_pRecoveryBtn->setEnabled(true);
        updatePicNum();
    });
    connect(m_pRightFavoriteThumbnailList, &ThumbnailListView::sigSelectAll, this, &AlbumView::updatePicNum);
    connect(m_pRightThumbnailList, &ThumbnailListView::sigSelectAll, this, &AlbumView::updatePicNum);
    connect(m_pRightPhoneThumbnailList, &ThumbnailListView::sigSelectAll, this, &AlbumView::updatePicNum);
#endif
    connect(dApp->signalM, &SignalManager::sigLoadOnePhoto, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &AlbumView::updateRightView);
//    connect(dApp->signalM, &SignalManager::insertedIntoAlbum, this, &AlbumView::updateRightView);

    connect(dApp->signalM, &SignalManager::insertedIntoAlbum, this, [ = ](QString albumname, QStringList pathlist) {
        qDebug() << "添加到目的相册：" << albumname;
        Q_UNUSED(pathlist);
        if (m_currentType != albumname) {
            return;
        }
        if (m_currentType == COMMON_STR_CUSTOM || albumname == m_currentType) //如果需要更新的为当前界面
            updateRightView();
    });

    connect(dApp->signalM, &SignalManager::removedFromAlbum, this, &AlbumView::updateAlbumView);
    connect(dApp->signalM, &SignalManager::imagesTrashInserted, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::imagesTrashRemoved, this, &AlbumView::updateRightView);
    connect(dApp, &Application::sigFinishLoad, this, [ = ] {
        m_pImpTimeLineWidget->m_mainListWidget->update();
        m_pRightThumbnailList->update();
        m_pRightFavoriteThumbnailList->update();
        m_pRightTrashThumbnailList->update();
    });
    connect(m_pRecoveryBtn, &DPushButton::clicked, this, &AlbumView::onTrashRecoveryBtnClicked);
    connect(m_pDeleteBtn, &DPushButton::clicked, this, &AlbumView::onTrashDeleteBtnClicked);
    connect(m_pRightThumbnailList, &ThumbnailListView::openImage, this, &AlbumView::openImage);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::openImage, this, &AlbumView::openImage);
    connect(m_pRightFavoriteThumbnailList, &ThumbnailListView::openImage, this, &AlbumView::openImage);
    connect(m_pLeftListView, &LeftListView::menuOpenImage, this, &AlbumView::menuOpenImage);
    connect(m_pRightThumbnailList, &ThumbnailListView::menuOpenImage, this, &AlbumView::menuOpenImage);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::menuOpenImage, this, &AlbumView::menuOpenImage);
    connect(m_pRightFavoriteThumbnailList, &ThumbnailListView::menuOpenImage, this, &AlbumView::menuOpenImage);
    connect(dApp->signalM, &SignalManager::sigUpdataAlbumRightTitle, this, &AlbumView::onUpdataAlbumRightTitle);
    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &AlbumView::updateRightView);
//    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &AlbumView::updateRightImportViewColock);

    connect(dApp->signalM, &SignalManager::sigUpdateTrashImageLoader, this, &AlbumView::updateRightView);
    connect(m_vfsManager, &DGioVolumeManager::mountAdded, this, &AlbumView::onVfsMountChangedAdd);
    connect(m_vfsManager, &DGioVolumeManager::mountRemoved, this, &AlbumView::onVfsMountChangedRemove);
    connect(m_vfsManager, &DGioVolumeManager::volumeAdded, [](QExplicitlySharedDataPointer<DGioVolume> vol) {
        if (vol->volumeMonitorName().contains(QRegularExpression("(MTP|GPhoto2|Afc)$"))) {
            vol->mount();
        }
    });
    connect(m_diskManager, &DDiskManager::fileSystemAdded, this, [ = ](const QString & dbusPath) {
        DBlockDevice *blDev = DDiskManager::createBlockDevice(dbusPath);
        blDev->mount({});
    });
    connect(m_diskManager, &DDiskManager::blockDeviceAdded, this, [ = ](const QString & blks) {
//        qDebug() << "--------------blks:" << blks;
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
                udispname = QCoreApplication::translate("DeepinStorage", "%1 Encrypted").arg(formatSize(qint64(size)));
                goto runend;
                //            blk->mount({});
                //            return;
            }
            udispname = QCoreApplication::translate("DeepinStorage", "%1 Volume").arg(formatSize(qint64(size)));
//            udispname = QCoreApplication::translate("DeepinStorage", "%1 ").arg(formatSize(size));
//            udispname = QCoreApplication::translate("PathManager", "System Disk");
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
//        std::accumulate(qbl.first(), qbl.last(), mountPoint);
        QUrl qurl(mountPoint);
        durlAndNameMap[qurl] = udispname;
        return;
    });
    connect(m_importAllByPhoneBtn, &DPushButton::clicked, this, &AlbumView::importAllBtnClicked);
    connect(m_importSelectByPhoneBtn, &DPushButton::clicked, this, &AlbumView::importSelectBtnClicked);
    connect(m_pStatusBar->m_pSlider, &DSlider::valueChanged, dApp->signalM, &SignalManager::sigMainwindowSliderValueChg);
    //
    connect(m_pLeftListView->m_pCustomizeListView, &LeftListWidget::signalDropEvent, this, &AlbumView::onLeftListDropEvent);
    //
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [ = ](DGuiApplicationHelper::ColorType themeType) {
        DPalette ReBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
        ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
        m_pRecoveryBtn->setPalette(ReBtn);

        DPalette DeBtn = DApplicationHelper::instance()->palette(m_pDeleteBtn);
        DeBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
        m_pDeleteBtn->setPalette(DeBtn);
        //add start 3975
        DPalette palcolor = DApplicationHelper::instance()->palette(m_pNoTrashWidget);
        palcolor.setBrush(DPalette::Base, palcolor.color(DPalette::Window));
        m_pNoTrashWidget->setPalette(palcolor);

        DPalette ppal_light = DApplicationHelper::instance()->palette(m_pNoTrashTitle);
        ppal_light.setBrush(DPalette::Background, ppal_light.color(DPalette::Base));
        m_pNoTrashTitle->setPalette(ppal_light);

        DPalette palcolor2 = DApplicationHelper::instance()->palette(m_pFavoriteWidget);
        palcolor2.setBrush(DPalette::Base, palcolor2.color(DPalette::Window));
        m_pFavoriteWidget->setPalette(palcolor2);

        DPalette ppal_light2 = DApplicationHelper::instance()->palette(m_FavoriteTitle);
        ppal_light2.setBrush(DPalette::Background, ppal_light2.color(DPalette::Base));
        m_FavoriteTitle->setPalette(ppal_light2);

        DPalette palcolor3 = DApplicationHelper::instance()->palette(m_pTrashWidget);
        palcolor3.setBrush(DPalette::Base, palcolor3.color(DPalette::Window));
        m_pTrashWidget->setPalette(palcolor3);

        DPalette ppal_light3 = DApplicationHelper::instance()->palette(m_TrashTitle);
        ppal_light3.setBrush(DPalette::Background, ppal_light3.color(DPalette::Base));
        m_TrashTitle->setPalette(ppal_light3);
        //add end 3975

        DPalette pal = DApplicationHelper::instance()->palette(m_pFavoritePicTotal);
        QColor color_BT = pal.color(DPalette::BrightText);
        if (themeType == DGuiApplicationHelper::LightType) {
            color_BT.setAlphaF(0.5);
            pal.setBrush(DPalette::Text, color_BT);
        } else if (themeType == DGuiApplicationHelper::DarkType) {
            color_BT.setAlphaF(0.75);
            pal.setBrush(DPalette::Text, color_BT);
        }
        m_pFavoritePicTotal->setForegroundRole(DPalette::Text);
        m_pFavoritePicTotal->setPalette(pal);
        m_pRightPicTotal->setForegroundRole(DPalette::Text);
        m_pRightPicTotal->setPalette(pal);
        pLabel2->setForegroundRole(DPalette::Text);
        pLabel2->setPalette(pal);

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
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::sigSelectAll, this, &AlbumView::updatePicNum);
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
//    connect(dApp->signalM, &SignalManager::trashDelete, this, &AlbumView::onTrashDeleteUpdateClick);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::loadEnd, this, &AlbumView::onTrashListClicked);
    connect(m_pImportView->m_pImportBtn, &DPushButton::clicked, this, [ = ] {
//        m_spinner->show();
//        m_spinner->start();
        m_pRightTitle->setVisible(false);
        m_pRightPicTotal->setVisible(false);
//        m_pImportTitle->setVisible(false); //del 3975
//        if (COMMON_STR_RECENT_IMPORTED == m_currentType)
//        {
//            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TIMELINE_IMPORT);
//            m_pStatusBar->setVisible(true);
//        } else if (COMMON_STR_CUSTOM == m_currentType)
//        {
//            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
//            m_pStatusBar->setVisible(true);
//        }
        emit dApp->signalM->startImprot();
        m_pImportView->onImprotBtnClicked();
    });
    connect(dApp->signalM, &SignalManager::sigImportFailedToView, this, [ = ] {
        if (isVisible())
        {
            m_spinner->hide();
            m_spinner->stop();
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
            m_pStatusBar->setVisible(false);
        }
    });
    connect(m_importByPhoneComboBox, &DComboBox::currentTextChanged, this, &AlbumView::importComboBoxChange);
    connect(dApp->signalM, &SignalManager::updateFavoriteNum, this, [ = ] {
        m_iAlubmPicsNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum, AlbumDBType::Favourite);
        QString favoriteStr = tr("%1 photo(s)");
        m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(m_iAlubmPicsNum)));
    });
    connect(dApp->signalM, &SignalManager::sigShortcutKeyDelete, this, &AlbumView::onKeyDelete);
    connect(dApp->signalM, &SignalManager::sigShortcutKeyF2, this, &AlbumView::onKeyF2);
    //2020年03月26日15:12:23
    connect(dApp->signalM, &SignalManager::waitDevicescan, this, &AlbumView::importDialog);
    connect(m_waitDailog_timer, &QTimer::timeout, this, [ = ] {
        m_waitDeviceScandialog->m_ignoreDeviceScan->setEnabled(true);
        m_waitDeviceScandialog->m_closeDeviceScan->setEnabled(true);
        m_waitDailog_timer->stop();
    });
    //在外部绑定内部按钮事件
    connect(m_pRightPhoneThumbnailList, &ThumbnailListView::loadEnd, this, &AlbumView::onWaitDialogIgnore);
    connect(m_waitDeviceScandialog->m_closeDeviceScan, &DPushButton::clicked, this, &AlbumView::onWaitDialogClose);
    connect(m_waitDeviceScandialog->m_ignoreDeviceScan, &DPushButton::clicked, this, &AlbumView::onWaitDialogIgnore);
    connect(m_pLeftListView->m_pMountListWidget, &DListWidget::clicked, this, [ = ](const QModelIndex & index) {
        Q_UNUSED(index);
        m_pRightPhoneThumbnailList->stopLoadAndClear(false);
        m_pLeftListView->setFocus();
        updateRightView();
    });
    connect(m_waitDeviceScandialog, &Waitdevicedialog::closed, this, &AlbumView::onWaitDialogClose);

    connect(this, &AlbumView::sigReCalcTimeLineSizeIfNeed, m_pImpTimeLineWidget, &ImportTimeLineView::sigResizeTimelineBlock);

    //void sigPhonePath(QString PhoneName, QString pathName);

//    QTimer::singleShot(2000, this, [ = ] {
//        if (RIGHT_VIEW_PHONE == m_pRightStackWidget->currentIndex())
//        {
//            QString strPath;
//            if (m_pLeftListView->m_pMountListView->currentItem()) {
//                strPath = m_pLeftListView->m_pMountListView->currentItem()->data(Qt::UserRole).toString();
//            }
//            if (0 != m_currentViewPictureCount && m_currentViewPictureCount == m_pRightPhoneThumbnailList->model()->rowCount()) {
//                m_pRightPhoneThumbnailList->loadFilesFromLocal(m_phoneNameAndPathlist[strPath], false, false);
//            }
//        }

//    });
//    if (nullptr == m_pAllPicture ) {
//        m_pAllPicture = new ThumbnailListView(ThumbnailDelegate::AlbumViewPhoneType, ALBUM_PATHTYPE_BY_PHONE);
//    }
    //lmh手机加载图片边加载，边传输信息
    connect(dApp->signalM, &SignalManager::sigPhonePath, this, [ = ](QString PhoneName, QString pathName) {
        if (!m_phoneNameAndPathlist[PhoneName].contains(pathName)) {
            m_phoneNameAndPathlist[PhoneName] << pathName;
            emit dApp->signalM->sigDevStop(PhoneName);
            ImageEngineApi::instance()->loadImageDateToMemory(m_phoneNameAndPathlist[PhoneName], PhoneName);
        }
        QString strPath;
        if (m_pLeftListView->m_pMountListWidget->currentItem()) {
            strPath = m_pLeftListView->m_pMountListWidget->currentItem()->data(Qt::UserRole).toString();
        }
        //m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_PHONE);
        if (strPath == PhoneName) {
            //判断状态栏
            if (!m_pStatusBar->isVisible()) {
                m_pStatusBar->setVisible(true);

            }
            if (RIGHT_VIEW_PHONE != m_pRightStackWidget->currentIndex()) {
                m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_PHONE);
            }
            QString str1 = tr("%1 photo(s)");
            m_pStatusBar->m_pAllPicNumLabel->setText(str1.arg(QString::number(m_phoneNameAndPathlist[PhoneName].count())));
            QString str = tr("%1 photo(s)");
            m_pPhonePicTotal->setText(str.arg(QString::number(m_phoneNameAndPathlist[PhoneName].count())));
            if (m_currentViewPictureCount == m_pRightPhoneThumbnailList->model()->rowCount()) {
                m_currentViewPictureCount = m_phoneNameAndPathlist[PhoneName].count();
                if (!m_pRightPhoneThumbnailList->isLoading() && isIgnore) {
                    if (isVisible()) {
                        emit dApp->signalM->waitDevicescan();
                    }
                    m_pRightPhoneThumbnailList->loadFilesFromLocal(m_phoneNameAndPathlist[PhoneName], false, false);
                } else if (!isIgnore) {
                    m_pRightPhoneThumbnailList->loadFilesFromLocal(m_phoneNameAndPathlist[PhoneName], false, false);
                }
            }
            // updateRightMountView();
        }
    }, Qt::QueuedConnection);
}

void AlbumView::initLeftView()
{
    m_pLeftListView = new LeftListView(this);
    m_pLeftListView->m_pPhotoLibListView->setCurrentRow(0);

    //init externalDevice
    m_mounts = getVfsMountList();
    initExternalDevice();
}

void AlbumView::onCreateNewAlbumFromDialog(const QString newalbumname)
{
    int index = m_pLeftListView->m_pCustomizeListView->count();

    QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftListView->m_pCustomizeListView, ablumType);//hj add data to listwidgetitem to Distinguish item's type
    m_pLeftListView->m_pCustomizeListView->insertItem(index, pListWidgetItem);
    pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
    QString albumName = newalbumname;
    AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName, COMMON_STR_CREATEALBUM);

    m_pLeftListView->m_pCustomizeListView->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    m_pLeftListView->m_pCustomizeListView->setCurrentRow(index);
    m_pLeftListView->moveMountListWidget();
    //清除其他已选中的项
    QModelIndex index2;
    emit m_pLeftListView->m_pCustomizeListView->pressed(index2);
}

void AlbumView::onCreateNewAlbumFrom(QString albumname)
{
    int index = m_pLeftListView->m_pCustomizeListView->count();
    QListWidgetItem *pListWidgetItem = new QListWidgetItem();
    pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
    QString albumName = albumname;
    AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName);
    m_pLeftListView->m_pCustomizeListView->insertItem(index, pListWidgetItem);
    m_pLeftListView->m_pCustomizeListView->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    m_pLeftListView->moveMountListWidget();
}

//载入完毕或者发生错误
void AlbumView::onLoadMountImagesEnd(QString mountname)
{
    Q_UNUSED(mountname);
}

void AlbumView::iniWaitDiolag()
{
    m_waitDeviceScandialog = new Waitdevicedialog(this);
    m_waitDailog_timer = new QTimer(this);
    m_waitDeviceScandialog->m_closeDeviceScan = new DPushButton(tr("Cancel"));
    m_waitDeviceScandialog->m_ignoreDeviceScan = new DPushButton(tr("Ignore"));
    m_waitDeviceScandialog->waitTips = new DLabel(tr("Loading photos, please wait..."));
    m_waitDeviceScandialog->iniwaitdialog();
    if (!m_waitDeviceScandialog) {
        return;
    }
    m_waitDeviceScandialog->setFixedSize(QSize(422, 183));
    m_waitDeviceScandialog->move(749, 414);
}
void AlbumView::initRightView()
{
    m_pRightStackWidget = new DStackedWidget();
    m_pRightWidget = new DWidget();

    // Import View
    m_pImportView = new ImportView();
    QList<QLabel *> labelList = m_pImportView->findChildren<QLabel *>();
    labelList[1]->setFixedHeight(22);
    labelList[1]->setText(tr("Or drag photos here"));

    m_pNoTrashWidget = new DWidget(); //add 3975
    DPalette palcolor = DApplicationHelper::instance()->palette(m_pNoTrashWidget);
    palcolor.setBrush(DPalette::Base, palcolor.color(DPalette::Window));
    m_pNoTrashWidget->setPalette(palcolor);

    QVBoxLayout *pNoTrashVBoxLayout = new QVBoxLayout();
    pNoTrashVBoxLayout->setContentsMargins(0, 0, 0, 0);

    m_pRightTitle = new DLabel();
    DFontSizeManager::instance()->bind(m_pRightTitle, DFontSizeManager::T3, QFont::DemiBold);
    m_pRightTitle->setForegroundRole(DPalette::TextTitle);

    m_pRightPicTotal = new DLabel();
    DFontSizeManager::instance()->bind(m_pRightPicTotal, DFontSizeManager::T6, QFont::Medium);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    DPalette pal = DApplicationHelper::instance()->palette(m_pRightPicTotal);
    QColor color_BT = pal.color(DPalette::BrightText);
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
    }
    m_pRightPicTotal->setForegroundRole(DPalette::Text);
    m_pRightPicTotal->setPalette(pal);

//    m_pRightPicTotal->setForegroundRole(DPalette::TextTips);

    m_pRightThumbnailList = new ThumbnailListView(ThumbnailDelegate::AlbumViewType, COMMON_STR_RECENT_IMPORTED);
    m_pRightThumbnailList->setFrameShape(DTableView::NoFrame);

    pNoTrashVBoxLayout->addSpacing(5);
    pNoTrashVBoxLayout->addWidget(m_pRightTitle);
    pNoTrashVBoxLayout->addSpacing(2);
    pNoTrashVBoxLayout->addWidget(m_pRightPicTotal);
    pNoTrashVBoxLayout->addSpacing(-1);
    pNoTrashVBoxLayout->setContentsMargins(12, 0, 0, 0);  //Edit 3975
//del start 3975
//    QVBoxLayout *p_all = new QVBoxLayout();
//    p_all->setContentsMargins(2, 0, 2, 0);
//    p_all->addLayout(pNoTrashVBoxLayout);
//    p_all->addSpacing(2);
//    p_all->addWidget(m_pRightThumbnailList);

//    pNoTrashWidget->setLayout(p_all);
//del end 3975
    //add start 3975
//    DListWidget *lsitWidget = new DListWidget();
    AlbumViewList *lsitWidget = new AlbumViewList();
    lsitWidget->setContentsMargins(0, 0, 0, 0);
    lsitWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    lsitWidget->setResizeMode(QListWidget::Adjust);
    lsitWidget->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    lsitWidget->verticalScrollBar()->setSingleStep(20);

    lsitWidget->setFrameShape(DTableView::NoFrame);
    QVBoxLayout *p_all = new QVBoxLayout();
    p_all->setContentsMargins(0, 0, 0, 0);
    p_all->addWidget(lsitWidget);
    m_pNoTrashWidget->setLayout(p_all);

    DWidget *blankWidget = new DWidget();
    QListWidgetItem *item = new QListWidgetItem();

    item->setFlags(Qt::NoItemFlags);
    lsitWidget->insertItem(0, item);
    lsitWidget->setItemWidget(item, blankWidget);
    item->setSizeHint(QSize(width(), 83 + 50));

    m_noTrashItem = new QListWidgetItem();
    m_noTrashItem->setFlags(Qt::NoItemFlags);
//    m_pRightThumbnailList->setListWidgetItem(m_noTrashItem);

    lsitWidget->insertItem(1, m_noTrashItem);
    lsitWidget->setItemWidget(m_noTrashItem, m_pRightThumbnailList);

//    m_pRightThumbnailList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
//    m_pRightThumbnailList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_pRightThumbnailList->setViewportMargins(-6, 0, 0, 0);
    m_pRightThumbnailList->setContentsMargins(0, 0, 0, 0);
    m_pNoTrashTitle = new DWidget(m_pNoTrashWidget);
    m_pNoTrashTitle->setLayout(pNoTrashVBoxLayout);

    DPalette ppal_light = DApplicationHelper::instance()->palette(m_pNoTrashTitle);
    ppal_light.setBrush(DPalette::Background, ppal_light.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light = new QGraphicsOpacityEffect;
    opacityEffect_light->setOpacity(0.95);
    m_pNoTrashTitle->setPalette(ppal_light);
    m_pNoTrashTitle->setGraphicsEffect(opacityEffect_light);
    m_pNoTrashTitle->setAutoFillBackground(true);
    m_pNoTrashTitle->move(0, 50);
    m_pNoTrashTitle->setFixedSize(this->width() - LEFT_VIEW_WIDTH, 83);
//add end 3975

// Trash View
//    DWidget *pTrashWidget = new DWidget(); //del 3975
    m_pTrashWidget = new DWidget(); //add 3975
    m_pTrashWidget->setFixedWidth(RIGHT_VIEW_WIDTH);
//add start 3975
    DPalette palcolor3 = DApplicationHelper::instance()->palette(m_pTrashWidget);
    palcolor3.setBrush(DPalette::Base, palcolor3.color(DPalette::Window));
    m_pTrashWidget->setPalette(palcolor3);
//add end 3975

    //重新更改了最近删除的顶部布局   2020-4-17 xiaolong
    QVBoxLayout *pTopVBoxlayout = new QVBoxLayout();
    pTopVBoxlayout->setContentsMargins(17, 5, 0, 11);

    pLabel1 = new DLabel();
    DFontSizeManager::instance()->bind(pLabel1, DFontSizeManager::T3, QFont::DemiBold);
    pLabel1->setFixedHeight(36);
    pLabel1->setForegroundRole(DPalette::TextTitle);
    pLabel1->setText(tr("Trash"));
    pTopVBoxlayout->addWidget(pLabel1);

    QHBoxLayout *pTopButtonLayout  = new QHBoxLayout();
    pTopButtonLayout->setContentsMargins(0, 0, 20, 0);

    m_pRecoveryBtn = new DPushButton();
    m_pRecoveryBtn->setText(tr("Restore"));
    m_pRecoveryBtn->setEnabled(false);
    m_pRecoveryBtn->setFixedSize(100, 36);

    DPalette ReBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
    ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_pRecoveryBtn->setPalette(ReBtn);

    pTopButtonLayout->addWidget(m_pRecoveryBtn);
    pTopButtonLayout->addSpacing(5);

    m_pDeleteBtn = new DWarningButton();
    m_pDeleteBtn->setText(tr("Delete All"));
    m_pDeleteBtn->setFixedSize(100, 36);

    DPalette DeBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
    ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_pDeleteBtn->setPalette(ReBtn);
    pTopButtonLayout->addWidget(m_pDeleteBtn);

    pLabel2 = new DLabel();
    DFontSizeManager::instance()->bind(pLabel2, DFontSizeManager::T6, QFont::Medium);
    pal = DApplicationHelper::instance()->palette(pLabel2);
    color_BT = pal.color(DPalette::BrightText);
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
    }
    pLabel2->setForegroundRole(DPalette::Text);
    pLabel2->setPalette(pal);
//    pLabel2->setForegroundRole(DPalette::TextTips);
    pLabel2->setText(tr("The photos will be permanently deleted after the days shown on it"));
    pTopVBoxlayout->addSpacing(9);
    pTopVBoxlayout->addWidget(pLabel2);

    //重新对button布局
    QVBoxLayout *pVboxlayout = new QVBoxLayout();
    pVboxlayout->setContentsMargins(0, 0, 0, 0);
    pVboxlayout->addStretch(1);
    pVboxlayout->addLayout(pTopButtonLayout);
    pVboxlayout->addStretch(1);

    QHBoxLayout    *pTopHboxlayout = new QHBoxLayout();
    pTopHboxlayout->setContentsMargins(0, 0, 0, 0);
    pTopHboxlayout->addLayout(pTopVBoxlayout);
    pTopHboxlayout->addLayout(pVboxlayout);

    m_pRightTrashThumbnailList = new ThumbnailListView(ThumbnailDelegate::AlbumViewType, COMMON_STR_TRASH);
    m_pRightTrashThumbnailList->setFrameShape(DTableView::NoFrame);

    AlbumViewList *lsitWidget3 = new AlbumViewList();
    lsitWidget3->setContentsMargins(0, 0, 0, 0);
    lsitWidget3->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    lsitWidget3->setResizeMode(QListWidget::Adjust);
    lsitWidget3->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    lsitWidget3->verticalScrollBar()->setSingleStep(20);

    lsitWidget3->setFrameShape(DTableView::NoFrame);
    QVBoxLayout *p_Trash = new QVBoxLayout();
    p_Trash->setContentsMargins(0, 0, 0, 0);
    p_Trash->addWidget(lsitWidget3);
    m_pTrashWidget->setLayout(p_Trash);

    DWidget *blankWidget3 = new DWidget();
    QListWidgetItem *Trashitem = new QListWidgetItem();

    Trashitem->setFlags(Qt::NoItemFlags);
    lsitWidget3->insertItem(0, Trashitem);
    lsitWidget3->setItemWidget(Trashitem, blankWidget3);

    Trashitem->setSizeHint(QSize(width(), 83 + 50));

    m_TrashitemItem = new QListWidgetItem();
    m_TrashitemItem->setFlags(Qt::NoItemFlags);

    lsitWidget3->insertItem(1, m_TrashitemItem);
    lsitWidget3->setItemWidget(m_TrashitemItem, m_pRightTrashThumbnailList);

    m_pRightTrashThumbnailList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pRightTrashThumbnailList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_pRightTrashThumbnailList->setViewportMargins(-6, 0, 0, 0);
    m_pRightTrashThumbnailList->setContentsMargins(0, 0, 0, 0);
    m_pRightTrashThumbnailList->setFixedSize(m_pTrashWidget->size());
    m_TrashTitle = new DWidget(m_pTrashWidget);
    m_TrashTitle->setLayout(pTopHboxlayout);

    DPalette ppal_light3 = DApplicationHelper::instance()->palette(m_TrashTitle);
    ppal_light3.setBrush(DPalette::Background, ppal_light3.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light3 = new QGraphicsOpacityEffect;
    opacityEffect_light3->setOpacity(0.95);
    m_TrashTitle->setPalette(ppal_light3);
    m_TrashTitle->setGraphicsEffect(opacityEffect_light3);
    m_TrashTitle->setAutoFillBackground(true);
    m_TrashTitle->move(0, 50);
    m_TrashTitle->setFixedSize(this->width() - LEFT_VIEW_WIDTH, 81);
//add end 3975

// Favorite View
// DWidget *pFavoriteWidget = new DWidget(); //del 3975
    m_pFavoriteWidget = new DWidget(); //add 3975
//add start 3975
    DPalette palcolor2 = DApplicationHelper::instance()->palette(m_pFavoriteWidget);
    palcolor2.setBrush(DPalette::Base, palcolor2.color(DPalette::Window));
    m_pFavoriteWidget->setPalette(palcolor2);
//add end 3975
    QVBoxLayout *pFavoriteVBoxLayout = new QVBoxLayout();

    m_pFavoriteTitle = new DLabel();
    DFontSizeManager::instance()->bind(m_pFavoriteTitle, DFontSizeManager::T3, QFont::DemiBold);
    m_pFavoriteTitle->setForegroundRole(DPalette::TextTitle);
    m_pFavoriteTitle->setText(tr("Favorites"));

    m_pFavoritePicTotal = new DLabel();
    DFontSizeManager::instance()->bind(m_pFavoritePicTotal, DFontSizeManager::T6, QFont::Medium);
    m_pFavoritePicTotal->setForegroundRole(DPalette::TextTips);
    QString favoriteStr = tr("%1 photo(s)");

    int favoritePicNum = DBManager::instance()->getImgsCountByAlbum(COMMON_STR_FAVORITES, AlbumDBType::Favourite);
    m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(favoritePicNum)));

    pal = DApplicationHelper::instance()->palette(m_pFavoritePicTotal);
    color_BT = pal.color(DPalette::BrightText);
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
    }
    m_pFavoritePicTotal->setForegroundRole(DPalette::Text);
    m_pFavoritePicTotal->setPalette(pal);


    m_pRightFavoriteThumbnailList = new ThumbnailListView(ThumbnailDelegate::AlbumViewType, COMMON_STR_FAVORITES);
    m_pRightFavoriteThumbnailList->setFrameShape(DTableView::NoFrame);

    pFavoriteVBoxLayout->addSpacing(3);
    pFavoriteVBoxLayout->addWidget(m_pFavoriteTitle);
    pFavoriteVBoxLayout->addSpacing(2);
    pFavoriteVBoxLayout->addWidget(m_pFavoritePicTotal);
    pFavoriteVBoxLayout->addSpacing(-1);

    pFavoriteVBoxLayout->setContentsMargins(12, 0, 0, 0); //edit 3975

    AlbumViewList *lsitWidget2 = new AlbumViewList();
    lsitWidget2->setContentsMargins(0, 0, 0, 0);
    lsitWidget2->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    lsitWidget2->setResizeMode(QListWidget::Adjust);
    lsitWidget2->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    lsitWidget2->verticalScrollBar()->setSingleStep(20);

    lsitWidget2->setFrameShape(DTableView::NoFrame);
    QVBoxLayout *p_Favorite = new QVBoxLayout();
    p_Favorite->setContentsMargins(0, 0, 0, 0);
    p_Favorite->addWidget(lsitWidget2);
    m_pFavoriteWidget->setLayout(p_Favorite);

    DWidget *blankWidget2 = new DWidget();
    QListWidgetItem *Favoriteitem = new QListWidgetItem();

    Favoriteitem->setFlags(Qt::NoItemFlags);
    lsitWidget2->insertItem(0, Favoriteitem);
    lsitWidget2->setItemWidget(Favoriteitem, blankWidget2);
    Favoriteitem->setSizeHint(QSize(width(), 83 + 50));

    m_FavoriteItem = new QListWidgetItem();
    m_FavoriteItem->setFlags(Qt::NoItemFlags);
//    m_pRightFavoriteThumbnailList->setListWidgetItem(m_FavoriteItem);
    lsitWidget2->insertItem(1, m_FavoriteItem);
    lsitWidget2->setItemWidget(m_FavoriteItem, m_pRightFavoriteThumbnailList);
//    m_pRightFavoriteThumbnailList->resize(480, 5000);
    qDebug() << "m_pRightThumbnailList height" << m_pRightFavoriteThumbnailList->height() << endl;
    qDebug() << "listWidget2 height" << lsitWidget2->height() << endl;
//    m_FavoriteItem->setSizeHint(QSize(this->width() - 200, m_pRightFavoriteThumbnailList->getListViewHeight() + 8));

    m_pRightFavoriteThumbnailList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pRightFavoriteThumbnailList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_pRightFavoriteThumbnailList->setViewportMargins(-6, 0, 0, 0);
    m_pRightFavoriteThumbnailList->setContentsMargins(0, 0, 0, 0);

    m_FavoriteTitle = new DWidget(m_pFavoriteWidget);
    m_FavoriteTitle->setLayout(pFavoriteVBoxLayout);

    DPalette ppal_light2 = DApplicationHelper::instance()->palette(m_FavoriteTitle);
    ppal_light2.setBrush(DPalette::Background, ppal_light2.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light2 = new QGraphicsOpacityEffect;
    opacityEffect_light2->setOpacity(0.95);
    m_FavoriteTitle->setPalette(ppal_light2);
    m_FavoriteTitle->setGraphicsEffect(opacityEffect_light2);
    m_FavoriteTitle->setAutoFillBackground(true);
    m_FavoriteTitle->move(0, 50);
    m_FavoriteTitle->setFixedSize(this->width() - 200, 83);
//add end 3975

//Search View
    m_pSearchView = new SearchView;

// Phone View
    pPhoneWidget = new DWidget();
    pPhoneWidget->setBackgroundRole(DPalette::Window);

    QVBoxLayout *pPhoneVBoxLayout = new QVBoxLayout();
    pPhoneVBoxLayout->setContentsMargins(0, 0, 0, 0);

    m_pPhoneTitle = new DLabel();
    DFontSizeManager::instance()->bind(m_pPhoneTitle, DFontSizeManager::T3, QFont::DemiBold);
    m_pPhoneTitle->setForegroundRole(DPalette::TextTitle);

    m_pPhonePicTotal = new DLabel();
    DFontSizeManager::instance()->bind(m_pPhonePicTotal, DFontSizeManager::T6, QFont::Medium);
    m_pPhonePicTotal->setForegroundRole(DPalette::TextTips);

    m_pRightPhoneThumbnailList = new ThumbnailListView(ThumbnailDelegate::AlbumViewPhoneType, ALBUM_PATHTYPE_BY_PHONE);
    m_pRightPhoneThumbnailList->setListViewUseFor(ThumbnailListView::Mount);
    m_pRightPhoneThumbnailList->setFrameShape(DTableView::NoFrame);

    pPhoneVBoxLayout->addSpacing(3);
    pPhoneVBoxLayout->addWidget(m_pPhoneTitle);
    pPhoneVBoxLayout->addSpacing(4);
    pPhoneVBoxLayout->addWidget(m_pPhonePicTotal);
    pPhoneVBoxLayout->addSpacing(-1);
    pPhoneVBoxLayout->setContentsMargins(10, 0, 0, 0);

//手机相片导入窗体
    m_importByPhoneWidget = new DWidget;
//    DWidget *topwidget = new DWidget;
//    topwidget->setFixedHeight(50);
    QHBoxLayout *mainImportLayout = new QHBoxLayout;
    DLabel *importLabel = new DLabel();
    importLabel->setText(tr("Import to:"));
    DFontSizeManager::instance()->bind(importLabel, DFontSizeManager::T6, QFont::Medium);
    importLabel->setForegroundRole(DPalette::TextTitle);
    importLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_importByPhoneComboBox = new DComboBox;
//    m_importByPhoneComboBox->setMinimumSize(QSize(213, 36));
    m_importByPhoneComboBox->setFixedSize(QSize(213, 36));
    m_importByPhoneComboBox->setEnabled(false);


    m_importAllByPhoneBtn = new DPushButton(tr("Import All"));
    DFontSizeManager::instance()->bind(m_importAllByPhoneBtn, DFontSizeManager::T6);
//    m_importAllByPhoneBtn ->setMinimumSize(100, 36);
    m_importAllByPhoneBtn->setFixedSize(100, 36);
    DPalette importAllByPhoneBtnPa = DApplicationHelper::instance()->palette(m_importAllByPhoneBtn);
    importAllByPhoneBtnPa.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_importAllByPhoneBtn->setPalette(importAllByPhoneBtnPa);
    m_importAllByPhoneBtn->setEnabled(false);

    m_importSelectByPhoneBtn = new DSuggestButton(tr("Import"));
//    m_importSelectByPhoneBtn = new DSuggestButton(tr("Import Selected"));
//    m_importSelectByPhoneBtn = new DSuggestButton(tr("Import Selected"));
    DFontSizeManager::instance()->bind(m_importSelectByPhoneBtn, DFontSizeManager::T6);

//    m_importSelectByPhoneBtn->setMinimumSize(100, 36);
    m_importSelectByPhoneBtn->setFixedSize(QSize(100, 36));
//    DPalette importSelectByPhoneBtnPa = DApplicationHelper::instance()->palette(m_importSelectByPhoneBtn);
//    importSelectByPhoneBtnPa.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
//    m_importSelectByPhoneBtn->setPalette(importSelectByPhoneBtnPa);
    m_importSelectByPhoneBtn->setEnabled(false);
    mainImportLayout->addWidget(importLabel);
    mainImportLayout->addSpacing(8);
    mainImportLayout->addWidget(m_importByPhoneComboBox);
    mainImportLayout->addSpacing(15);
    mainImportLayout->addWidget(m_importAllByPhoneBtn);
    mainImportLayout->addSpacing(5);
    mainImportLayout->addWidget(m_importSelectByPhoneBtn);
    m_importByPhoneWidget->setLayout(mainImportLayout);

    QHBoxLayout *allHLayout = new QHBoxLayout;
    allHLayout->addLayout(pPhoneVBoxLayout, 1);
    allHLayout->addStretch();
    allHLayout->addWidget(m_importByPhoneWidget, 1);

    QVBoxLayout *p_all2 = new QVBoxLayout();
//    p_all2->addWidget(topwidget);
    p_all2->addLayout(allHLayout);
//    p_all2->addWidget(m_pRightPhoneThumbnailList);

    m_pRightPhoneThumbnailList->setParent(pPhoneWidget);
    phonetopwidget = new DWidget(pPhoneWidget);
    QGraphicsOpacityEffect *opacityEffect_lightphone = new QGraphicsOpacityEffect;
    opacityEffect_lightphone->setOpacity(0.95);
    phonetopwidget->setGraphicsEffect(opacityEffect_lightphone);

    phonetopwidget->setAutoFillBackground(true);
    phonetopwidget->setFixedSize(this->width() - LEFT_VIEW_WIDTH, 87);
    phonetopwidget->setLayout(p_all2);
    phonetopwidget->move(0, 50);
    phonetopwidget->raise();

//add start 3975
    m_pStatusBar = new StatusBar(this);
//    m_pStatusBar->setParent(this);
    m_pStatusBar->raise();
    m_pStatusBar->setFixedWidth(this->width() - m_pLeftListView->width());
    m_pStatusBar->move(m_pLeftListView->width(), this->height() - m_pStatusBar->height());

    pImportTimeLineWidget = new DWidget();
    pImportTimeLineWidget->setBackgroundRole(DPalette::Window);
    m_pImpTimeLineWidget = new ImportTimeLineView(pImportTimeLineWidget);
//    connect(m_pImpTimeLineWidget, &ImportTimeLineView::albumviewResize, this, [ = ]() {
//        this->setFixedSize(QSize(size().width() + 1, size().height()));
//        this->setFixedSize(QSize(size().width() - 1, size().height())); //触发resizeevent
//        this->setMinimumSize(0, 0);
//        this->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));  //触发后还原状态
//    });
    m_pImpTimeLineWidget->getFatherStatusBar(m_pStatusBar->m_pSlider);
    m_pImpTimeLineWidget->clearAndStartLayout();
    m_pImpTimeLineWidget->move(-6, 0);
//add end 3975
// Add View
    m_pRightStackWidget->addWidget(m_pImportView);
    m_pRightStackWidget->addWidget(m_pNoTrashWidget);  //已导入
    m_pRightStackWidget->addWidget(m_pTrashWidget);    //最近删除
    m_pRightStackWidget->addWidget(m_pFavoriteWidget);  //我的收藏
    m_pRightStackWidget->addWidget(m_pSearchView);
    m_pRightStackWidget->addWidget(pPhoneWidget);
    m_pRightStackWidget->addWidget(pImportTimeLineWidget);

// Statusbar


    QVBoxLayout *pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins(0, 0, 0, 0);
    pVBoxLayout->addWidget(m_pRightStackWidget);
//    pVBoxLayout->addWidget(m_pStatusBar);
    m_pRightWidget->setLayout(pVBoxLayout);

    if (0 < DBManager::instance()->getImgsCount()) {
        m_pRightThumbnailList->setFrameShape(DTableView::NoFrame);
//        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TIMELINE_IMPORT);
        m_pStatusBar->show();
    } else {
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
        // m_pStatusBar->show();
        m_pStatusBar->setVisible(false);

    }
//    updateRightView();
}

void AlbumView::updateRightView()
{
    if (!m_pRightTitle) {
        return;
    }
    m_pRightTitle->setVisible(true);
    m_pRightPicTotal->setVisible(true);
//    m_pImportTitle->setVisible(true);  //del 3975
    if (!m_spinner) {
        return;
    }
    m_spinner->hide();
    m_spinner->stop();

    m_curThumbnaiItemList_info.clear();
    m_curThumbnaiItemList_str.clear();

    if (COMMON_STR_RECENT_IMPORTED == m_currentType) {
        updateRightImportView();
    } else if (COMMON_STR_TRASH == m_currentType) {
        updateRightTrashView();
        onTrashListClicked();
        setAcceptDrops(false);
        emit sigSearchEditIsDisplay(false);
    } else if (COMMON_STR_FAVORITES == m_currentType) {
        updateRightMyFavoriteView();
    } else if (COMMON_STR_CUSTOM == m_currentType) {
        updateRightNoTrashView();
    } else if (ALBUM_PATHTYPE_BY_PHONE == m_currentType) {
        m_itemClicked = true;
        updateRightMountView();
        setAcceptDrops(false);
        emit sigSearchEditIsDisplay(false);
    }
    updatePicNum();
}

void AlbumView::updateAlbumView(const QString &album)
{
    if (m_currentType == album) {
        updateRightMyFavoriteView();
    }
}

// 更新已导入列表
void AlbumView::updateRightImportView()
{
    m_iAlubmPicsNum = DBManager::instance()->getImgsCount();

    if (0 < m_iAlubmPicsNum) {
//        m_pImpTimeLineWidget->updataLayout();
//        m_pImpTimeLineWidget->getFatherStatusBar(m_pStatusBar->m_pSlider);
        m_pImpTimeLineWidget->clearAndStartLayout();
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TIMELINE_IMPORT);
        m_pStatusBar->setVisible(true);
    } else {
//        m_pImpTimeLineWidget->updataLayout();
//        m_pImpTimeLineWidget->getFatherStatusBar(m_pStatusBar->m_pSlider);
        m_pImpTimeLineWidget->clearAndStartLayout();
        m_pImportView->setAlbumname(QString());
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
        m_pStatusBar->setVisible(false);
    }
//    m_pImpTimeLineWidget->resizeHand();    //重新计算已导入大小

    emit sigSearchEditIsDisplay(true);

    setAcceptDrops(true);
}

// 更新个人收藏列表
void AlbumView::updateRightMyFavoriteView()
{
    using namespace utils::image;
    DBImgInfoList infos;
    infos = DBManager::instance()->getInfosByAlbum(m_currentAlbum, AlbumDBType::Favourite);

//    bcurThumbnaiItemList_str = false;
//    m_curThumbnaiItemList_info.clear();
    m_curThumbnaiItemList_info << infos;
//    m_curThumbnaiItemList << infos;
//    for (auto info : infos) {
//        ThumbnailListView::ItemInfo vi;
//        vi.name = info.fileName;
//        vi.path = info.filePath;
////            vi.image = dApp->m_imagemap.value(info.filePath);
//        if (dApp->m_imagemap.value(info.filePath).isNull()) {
//            QSize imageSize = getImageQSize(vi.path);

//            vi.width = imageSize.width();
//            vi.height = imageSize.height();
//        } else {
//            vi.width = dApp->m_imagemap.value(info.filePath).width();
//            vi.height = dApp->m_imagemap.value(info.filePath).height();
//        }

//        m_curThumbnaiItemList << vi;
//    }
    m_pRightFavoriteThumbnailList->stopLoadAndClear();
    m_pRightFavoriteThumbnailList->loadFilesFromLocal(infos);
//    m_pRightFavoriteThumbnailList->insertThumbnails(m_curThumbnaiItemList);
//    m_pRightFavoriteThumbnailList->importFilesFromLocal(infos);
    m_iAlubmPicsNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum, AlbumDBType::Favourite);

    QString favoriteStr = tr("%1 photo(s)");
    m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(m_iAlubmPicsNum)));

    m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_FAVORITE_LIST);
    m_pStatusBar->setVisible(true);

    m_pRightFavoriteThumbnailList->resizeHand();

    emit sigSearchEditIsDisplay(true);
    setAcceptDrops(false);
}

// 更新外接设备右侧视图
void AlbumView::updateRightMountView()
{
    qDebug() << "zy------AlbumView::updateRightMountView";
    if (!isVisible()) {
        qDebug() << "提前退出更新右侧视图";
        return;
    }
    m_currentViewPictureCount = 0;
    isMountThreadRunning = true;
    if (!m_pLeftListView) {
        isMountThreadRunning = false;
        return;
    }
    //    qDebug() << m_phoneNameAndPathlist;
    QString strPath;
    QString phoneTitle;
    if (m_pLeftListView->m_pMountListWidget->currentItem()) {
        strPath = m_pLeftListView->m_pMountListWidget->currentItem()->data(Qt::UserRole).toString();
        if (!strPath.contains("/media/") && !strPath.contains("DCIM")) {
            findPicturePathByPhone(strPath);
        }
        phoneTitle = m_pLeftListView->m_pMountListWidget->currentItem()->data(Qt::UserRole + 1).toString();
    }
    qDebug() << "data(Qt::UserRole).toString()" << strPath;
    qDebug() << m_phoneNameAndPathlist.contains(strPath);
    qDebug() << m_phoneNameAndPathlist.value(strPath).length();

    QStringList filelist = m_phoneNameAndPathlist.value(strPath);
    m_currentViewPictureCount = filelist.count();
    if (m_phoneNameAndPathlist.contains(strPath) && 0 < m_currentViewPictureCount) {
//        QStringList filelist = m_phoneNameAndPathlist.value(strPath);
        if (!filelist.isEmpty()) {
            m_importByPhoneComboBox->setEnabled(true);
            m_importAllByPhoneBtn->setEnabled(true);
            updateImportComboBox();
            m_pRightPhoneThumbnailList->stopLoadAndClear();
            m_curThumbnaiItemList_str << filelist;
            m_curPhoneItemList_str.clear();
            m_curPhoneItemList_str << filelist;     //保存外部设备图片的路径
            m_iAlubmPicsNum = m_curThumbnaiItemList_str.size();
            m_mountPicNum = m_curThumbnaiItemList_str.size();
            qDebug() << "m_mountPicNum = " << m_mountPicNum;
            m_pPhoneTitle->setText(phoneTitle);
            QFontMetrics elideFont(m_pPhoneTitle->font());
            m_pPhoneTitle->setText(elideFont.elidedText(phoneTitle, Qt::ElideRight, 525));
            QString str = tr("%1 photo(s)");
            m_pPhonePicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));
            //LMH0424，下方显示和标题显示数字相同
            m_pStatusBar->m_pAllPicNumLabel->setText(str.arg(QString::number(m_iAlubmPicsNum)));
            m_pRightPhoneThumbnailList->m_imageType = ALBUM_PATHTYPE_BY_PHONE;
            if (!m_pRightPhoneThumbnailList->isLoading() && isIgnore) {
                if (isVisible()) {
                    emit dApp->signalM->waitDevicescan();
                }
                m_pRightPhoneThumbnailList->loadFilesFromLocal(filelist, false, false);
            } else if (!isIgnore) {
                m_pRightPhoneThumbnailList->loadFilesFromLocal(filelist, false, false);
            }
            QStringList paths = m_pRightPhoneThumbnailList->selectedPaths();
            if (0 < paths.length()) {
                m_importSelectByPhoneBtn->setEnabled(true);
            } else {
                m_importSelectByPhoneBtn->setEnabled(false);
            }
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_PHONE);
            m_pStatusBar->setVisible(true);
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
            //LMH0424，下方显示和标题显示数字相同
            m_pStatusBar->m_pAllPicNumLabel->setText(QString::number(0));
            m_pRightPhoneThumbnailList->m_imageType = ALBUM_PATHTYPE_BY_PHONE;
            m_pRightPhoneThumbnailList->stopLoadAndClear();
            if (m_curThumbnaiItemList_info.size() > 0) {
                m_pRightPhoneThumbnailList->loadFilesFromLocal(m_curThumbnaiItemList_info, false, false);
            } else {
                m_pRightPhoneThumbnailList->loadFilesFromLocal(m_curThumbnaiItemList_str, false, false);
            }
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_PHONE);
            m_pStatusBar->setVisible(true);
        }
        emit sigSearchEditIsDisplay(false);
        setAcceptDrops(false);
        isMountThreadRunning = false;
    }
}

// 更新新建相册列表
void AlbumView::updateRightNoTrashView()
{
    using namespace utils::image;
    DBImgInfoList infos;
    infos = DBManager::instance()->getInfosByAlbum(m_currentAlbum);
    m_curThumbnaiItemList_info << infos;
    m_iAlubmPicsNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);
    if (0 < m_iAlubmPicsNum) {
        m_pRightTitle->setText(m_currentAlbum);
        QFontMetrics elideFont(m_pRightTitle->font());
        m_pRightTitle->setText(elideFont.elidedText(m_currentAlbum, Qt::ElideRight, 525));
        QString str = tr("%1 photo(s)");
        m_pRightPicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));
        m_pRightThumbnailList->m_imageType = m_currentAlbum;
        m_pRightThumbnailList->stopLoadAndClear();
        m_pRightThumbnailList->loadFilesFromLocal(infos);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
        m_pStatusBar->show();
    } else {
        m_pRightThumbnailList->stopLoadAndClear();
        m_pRightThumbnailList->loadFilesFromLocal(infos);
        m_pImportView->setAlbumname(m_currentAlbum);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
        m_pStatusBar->setVisible(false);
    }

    emit sigSearchEditIsDisplay(true);
    setAcceptDrops(true);
}

void AlbumView::updateRightTrashView()
{
    using namespace utils::image;
    DBImgInfoList infos;
    infos = DBManager::instance()->getAllTrashInfos();
    for (auto pinfo : infos) {
        if (!QFileInfo(pinfo.filePath).exists()) {
            infos.removeOne(pinfo);
        }
    }
    m_curThumbnaiItemList_info << infos;
    if (0 < infos.length()) {
        m_pDeleteBtn->setEnabled(true);
    } else {
        m_pDeleteBtn->setText(tr("Delete All"));
        m_pRecoveryBtn->setEnabled(false);
        m_pDeleteBtn->setEnabled(false);
    }
    m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TRASH_LIST);
    m_pStatusBar->setVisible(true);
    m_pRightTrashThumbnailList->stopLoadAndClear();
    m_pRightTrashThumbnailList->loadFilesFromTrash(infos);
    m_pRightTrashThumbnailList->resizeHand();
}

void AlbumView::leftTabClicked()
{
    emit dApp->signalM->SearchEditClear();
    //若点击当前的item，则不做任何处理
    if (m_currentAlbum == m_pLeftListView->getItemCurrentName()
            && m_currentItemType == m_pLeftListView->getItemDataType()) {
        SearchReturnUpdate();
        return;
    }
    m_currentAlbum = m_pLeftListView->getItemCurrentName();
    m_currentType = m_pLeftListView->getItemCurrentType();
    m_currentItemType = static_cast<AblumType>(m_pLeftListView->getItemDataType());
    updateRightView();
}

bool AlbumView::imageGeted(QStringList &filelist, QString path)
{
    m_phoneNameAndPathlist[path] = filelist;
    m_pictrueallPathlist.clear();
    for (auto list : m_phoneNameAndPathlist) {
        m_pictrueallPathlist << list;
    }

    qDebug() << "zy------AlbumView::imageGeted loadImageDateToMemory";
    ImageEngineApi::instance()->loadImageDateToMemory(m_phoneNameAndPathlist[path], path);
    m_currentLoadingPictrue = m_pictrueallPathlist.count();
    if (m_itemClicked == true) {
        m_curThumbnaiItemList_str.clear();
        updateRightMountView();
    }
    return true;
}

void AlbumView::onTrashRecoveryBtnClicked()
{
    QStringList paths;
    paths = m_pRightTrashThumbnailList->selectedPaths();
    ImageEngineApi::instance()->recoveryImagesFromTrash(paths);
}

void AlbumView::onTrashDeleteBtnClicked()
{
    QStringList paths;

    if (tr("Delete") == m_pDeleteBtn->text()) {
        paths = m_pRightTrashThumbnailList->selectedPaths();
    } else {
        paths = DBManager::instance()->getAllTrashPaths();
    }
    QString str1 = tr("Delete All");
    QString str2 = m_pDeleteBtn->text();
    bool bstate = false;
    if (str1 == str2) {
        bstate = true;
    }
    ImgDeleteDialog *dialog = new ImgDeleteDialog(this, paths.count(), bstate);
    dialog->show();
    connect(dialog, &ImgDeleteDialog::imgdelete, this, [ = ] {
        ImageEngineApi::instance()->moveImagesToTrash(paths, true, false);
    });

    onTrashListClicked();
}

void AlbumView::openImage(int index)
{
    SignalManager::ViewInfo info;
    info.album = "";
    info.lastPanel = nullptr;
    if (m_curThumbnaiItemList_info.size() > 0) {
        for (auto image : m_curThumbnaiItemList_info) {
            info.paths << image.filePath;
        }
        info.path = m_curThumbnaiItemList_info[index].filePath;
    } else if (m_curThumbnaiItemList_str.size() > 0) {
        info.paths << m_curThumbnaiItemList_str;
        info.path = m_curThumbnaiItemList_str[index];
    } else {
        info.paths.clear();
    }
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

    QStringList imagelist;
    if (COMMON_STR_TRASH == m_currentAlbum) {
        imagelist = m_pRightTrashThumbnailList->getAllFileList();
    } else if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum) {
        imagelist = m_pRightThumbnailList->getAllFileList();
    } else if (COMMON_STR_FAVORITES == m_currentAlbum) {
        imagelist = m_pRightFavoriteThumbnailList->getAllFileList();
    } else if (m_currentType == COMMON_STR_CUSTOM) {
        imagelist = DBManager::instance()->getPathsByAlbum(m_currentAlbum);
    } else {
        imagelist = m_pRightThumbnailList->getAllFileList();
    }
    if (paths.size() > 1) {
        info.paths = paths;
    } else {
        if (imagelist.size() > 1) {
            for (auto image : imagelist) {
                info.paths << image;
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
        QStringList pathlist;
        pathlist.clear();
        for (auto path : info.paths) {
            if (QFileInfo(path).exists()) {
                pathlist << path;
            }
        }
        info.paths = pathlist;
        emit dApp->signalM->startSlideShow(info);
        emit dApp->signalM->showSlidePanel(VIEW_MAINWINDOW_ALBUM);
    } else {
        emit dApp->signalM->viewImage(info);
        emit dApp->signalM->showImageView(VIEW_MAINWINDOW_ALBUM);
    }
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
    const QMimeData *mimeData = e->mimeData();
    if (!utils::base::checkMimeData(mimeData)) {
        return;
    }
    e->setDropAction(Qt::CopyAction);
    e->accept();
}

void AlbumView::dropEvent(QDropEvent *event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (urls.isEmpty()) {
        return;
    }
    ImageEngineApi::instance()->ImportImagesFromUrlList(urls, m_currentAlbum, this);
    event->accept();
}

void AlbumView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void AlbumView::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_UNUSED(e);
}

void AlbumView::onKeyDelete()
{
    if (!isVisible()) return;
    if (RIGHT_VIEW_SEARCH == m_pRightStackWidget->currentIndex()) return;
    bool bMoveToTrash = false;
    QStringList paths;
    paths.clear();
    if (COMMON_STR_RECENT_IMPORTED == m_currentType) {
        paths = m_pImpTimeLineWidget->selectPaths();
        if (0 < paths.length()) {
            bMoveToTrash = true;
        }
    } else if (COMMON_STR_TRASH == m_currentType) {
        paths = m_pRightTrashThumbnailList->selectedPaths();
        if (0 < paths.length()) {
            ImgDeleteDialog *dialog = new ImgDeleteDialog(this, paths.length());
            dialog->show();
            connect(dialog, &ImgDeleteDialog::imgdelete, this, [ = ] {
                ImageEngineApi::instance()->moveImagesToTrash(paths, true);
                onTrashListClicked();
            });
        }
    } else if (COMMON_STR_FAVORITES == m_currentType) {
        paths = m_pRightFavoriteThumbnailList->selectedPaths();
        if (0 < paths.length()) {
            DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, paths, AlbumDBType::Favourite);
        }
    } else if (COMMON_STR_CUSTOM == m_currentType) {
        paths = m_pRightThumbnailList->selectedPaths();
        // 如果没有选中的照片,或相册中的照片数为0,则删除相册
        if (0 == paths.length() || 0 == DBManager::instance()->getImgsCountByAlbum(m_currentAlbum)) {
            QListWidgetItem *item = m_pLeftListView->m_pCustomizeListView->currentItem();
            AlbumLeftTabItem *pTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pCustomizeListView->itemWidget(item));
            m_deleteDialog = new AlbumDeleteDialog;
            connect(m_deleteDialog, &AlbumDeleteDialog::deleteAlbum, this, [ = ]() {
                QString str = pTabItem->m_albumNameStr;
                QStringList album_paths = DBManager::instance()->getPathsByAlbum(pTabItem->m_albumNameStr);
                ImageEngineApi::instance()->moveImagesToTrash(album_paths);
                DBManager::instance()->removeAlbum(pTabItem->m_albumNameStr);

                if (1 < m_pLeftListView->m_pCustomizeListView->count()) {
                    delete  item;
                    m_currentItemType = ablumType;
                } else {
                    m_pLeftListView->updateCustomizeListView();
                    m_pLeftListView->updatePhotoListView();
                    m_currentItemType = photosType;
                }
                //刷新右侧视图
                leftTabClicked();
                m_pLeftListView->moveMountListWidget();
                emit dApp->signalM->sigAlbDelToast(str);
            });
            m_deleteDialog->show();
        } else {
            bMoveToTrash = true;
        }
    } else if (ALBUM_PATHTYPE_BY_PHONE == m_currentType) {
        // 外部设备中的照片不删除
    } else {

    }
    // 删除选中照片
    if (bMoveToTrash) {
        ImageEngineApi::instance()->moveImagesToTrash(paths);
    }
}

void AlbumView::onKeyF2()
{
    if (COMMON_STR_CUSTOM != m_currentType) return;
    AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pCustomizeListView->itemWidget(m_pLeftListView->m_pCustomizeListView->currentItem()));
    item->m_opeMode = OPE_MODE_RENAMEALBUM;
    item->editAlbumEdit();
}

//挂载设备改变
void AlbumView::onVfsMountChangedAdd(QExplicitlySharedDataPointer<DGioMount> mount)
{
    qDebug() << "zy------onVfsMountChangedAdd activeThreadCount = " << QThreadPool::globalInstance()->activeThreadCount();
    qDebug() << "zy------AlbumView::onVfsMountChangedAdd() name:" << mount->name();
    //TODO:
    //Support android phone, iPhone, and usb devices. Not support ftp, smb mount, non removeable disk now
    QString uri = mount->getRootFile()->uri();
    QString scheme = QUrl(uri).scheme();
//    bool canEject = mount->canEject();
    if ((scheme == "file" /*&& mount->canEject()*/) ||  //usb device
            (scheme == "gphoto2") ||                //phone photo
            //(scheme == "afc") ||                  //iPhone document
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
        }
        QString rename = "";
        rename = durlAndNameMap[QUrl(mount->getRootFile()->uri())];
        if ("" == rename) {
            rename = mount->name();
        }
        //判断路径是否存在
        bool bFind = false;
        QDir dir(strPath);
        if (!dir.exists()) {
            qDebug() << strPath;
            qDebug() << "onLoadMountImagesStart() !dir.exists()";
            dApp->signalM->sigLoadMountImagesEnd(rename);
            bFind = false;
//            return;
        } else {
            bFind = true;
        }

        //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,再调用findPicturePathByPhone函数搜索DCIM文件目录
        if (!strPath.contains("/media/")) {
            bFind = findPicturePathByPhone(strPath);
//            if (!bFind) {
//                qDebug() << "onLoadMountImagesStart() !bFind";
//                dApp->signalM->sigLoadMountImagesEnd(rename);
//                return;
//            }
        }

        isIgnore = true;
        updateExternalDevice(mount, strPath);
        m_mounts = getVfsMountList();

        if (bFind) {
            qDebug() << "zy------AlbumView::onVfsMountChangedAdd ImageEngineApi getImageFilesFromMount";
//            this->clearAndStopThread();
            ImageEngineApi::instance()->getImageFilesFromMount(rename, strPath, this);//zy123
        }
        //emit dApp->signalM->waitDevicescan();
    }
}

//卸载外接设备
void AlbumView::onVfsMountChangedRemove(QExplicitlySharedDataPointer<DGioMount> mount)
{
    if (m_waitDeviceScandialog) {
        m_waitDeviceScandialog->close();
    }
    qDebug() << "zy------onVfsMountChangedRemove activeThreadCount = " << QThreadPool::globalInstance()->activeThreadCount();
    qDebug() << "zy------AlbumView::onVfsMountChangedRemove";
    QString uri = mount->getRootFile()->uri();
    QString strPath = mount->getDefaultLocationFile()->path();
    if (!strPath.contains("/media/")) {
        findPicturePathByPhone(strPath);
    }
//    for (auto mountLoop : m_mounts) {
//        QString uriLoop = mountLoop->getRootFile()->uri();
//        if (uri == uriLoop) {
//            auto it = m_phoneNameAndPathlist.begin();
//            while (it != m_phoneNameAndPathlist.end()) {
//                if (it.key().contains(strPath)) {
//                    emit dApp->signalM->sigDevStop(it.key());
//                    ImageEngineApi::instance()->removeImage(it.value());//从内存中删除
//                    m_phoneNameAndPathlist.erase(it);
//                    break;
//                }
//                it++;
//            }

//            m_mounts.removeOne(mountLoop);
//        }
//    }

    for (int i = 0; i < m_pLeftListView->m_pMountListWidget->count(); i++) {
        QListWidgetItem *pListWidgetItem = m_pLeftListView->m_pMountListWidget->item(i);
        AlbumLeftTabItem *pAlbumLeftTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pMountListWidget->itemWidget(pListWidgetItem));
        if (!pAlbumLeftTabItem || !pListWidgetItem)
            break;
        QString rename = "";
//        QString dpath = mount->getRootFile()->uri();
        QUrl qurl(mount->getRootFile()->uri());
        rename = durlAndNameMap[qurl];
        if ("" == rename) {
            rename = mount->name();
        }
        if (rename == pAlbumLeftTabItem->m_albumNameStr /*&&
                strPath == pListWidgetItem->data(Qt::UserRole).toString()*/
                /*pAlbumLeftTabItem->m_mountPath.contains(strPath)*/) {
            if (1 < m_pLeftListView->m_pMountListWidget->count()) {
                m_pLeftListView->m_pMountListWidget->takeItem(m_pLeftListView->m_pMountListWidget->row(pListWidgetItem));
                delete pListWidgetItem;
            } else {
                m_pLeftListView->m_pMountListWidget->clear();
                m_pLeftListView->updatePhotoListView();
            }
            emit dApp->signalM->sigDevStop(strPath);
            m_phoneNameAndPathlist.remove(strPath);
            durlAndNameMap.erase(durlAndNameMap.find(qurl));
            break;
        }
    }
    m_mounts = getVfsMountList();
//    QThread::sleep(1);
//    emit m_waitDeviceScandialog->m_closeDeviceScan->clicked();
}

void AlbumView::getAllDeviceName()
{
//    QStringList blDevList = m_diskManager->blockDevices();
    QStringList blDevList = DDiskManager::blockDevices(QVariantMap());
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
        }

        if (mps.contains(QByteArray("/\0", 2))) {
            udispname = tr("System Disk");
            goto runend1;
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
            }
            if (drv->opticalBlank()) {
                udispname = QCoreApplication::translate("DeepinStorage", "Blank %1 Disc").arg(opticalmediamap[drv->media()]);
                goto runend1;
            }
            if (pblk->isEncrypted() && !blk) {
                udispname = QCoreApplication::translate("DeepinStorage", "%1 Encrypted").arg(formatSize(qint64(size)));
                goto runend1;
            }
            udispname = QCoreApplication::translate("DeepinStorage", "%1 Volume").arg(formatSize(qint64(size)));
            goto runend1;
        }
        udispname = label;
    runend1:
        blk->mount({});
        QByteArrayList qbl = blk->mountPoints();
        QString mountPoint = "file://";
        for (QByteArray qb : qbl) {
            mountPoint += qb;
        }
//        std::accumulate(qbl.first(), qbl.last(), mountPoint);
        qDebug() << "mountPoint:" << mountPoint;
        QUrl qurl(mountPoint);
        durlAndNameMap[qurl] = udispname;
    }
}

//获取外部设备列表
const QList<QExplicitlySharedDataPointer<DGioMount>> AlbumView::getVfsMountList()
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
                //(scheme == "afc") ||                    //iPhone document
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
    filters << QString("*.jpeg") << QString("*.jpg")
            << QString("*.JPEG") << QString("*.JPG");
    //定义迭代器并设置过滤器
    QDirIterator dir_iterator(path, filters, QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    QStringList string_list;
    while (dir_iterator.hasNext()) {
        dir_iterator.next();
        QFileInfo fileInfo = dir_iterator.fileInfo();
        QImage tImg;
        QString format = UnionImage_NameSpace::detectImageFormat(fileInfo.filePath());
        if (format.isEmpty()) {
            QImageReader reader(fileInfo.filePath());
            reader.setAutoTransform(true);
            if (reader.canRead()) {
                tImg = reader.read();
            } else {
                tImg = QImage();
            }
        } else {
            QImageReader readerF(fileInfo.filePath(), format.toLatin1());
            readerF.setAutoTransform(true);
            if (readerF.canRead()) {
                tImg = readerF.read();
            } else {
                qDebug() << "can't read image:" << readerF.errorString() << format;
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

void AlbumView::importComboBoxChange(QString strText)
{
    Q_UNUSED(strText);
    if (1 == m_importByPhoneComboBox->currentIndex()) {
        AlbumCreateDialog *dialog = new AlbumCreateDialog(this);
        dialog->show();
        qDebug() << "xxxxxxxxxx" << window()->x();
        qDebug() << "xxxxxxxxxx" << window()->y();
        qDebug() << "xxxxxxxxxx" << dialog->width();
        qDebug() << "xxxxxxxxxx" << window()->width();
        dialog->move(window()->x() + (window()->width() - dialog->width()) / 2, window()->y() + (window()->height() - dialog->height()) / 2);
        connect(dialog, &AlbumCreateDialog::albumAdded, this, [ = ] {
            DBManager::instance()->insertIntoAlbum(dialog->getCreateAlbumName(), QStringList(" "));
            onCreateNewAlbumFrom(dialog->getCreateAlbumName());
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
    filters << QString("*.jpeg") << QString("*.jpg")
            << QString("*.JPEG") << QString("*.JPG");

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
#if 1
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(/*m_pLeftListView->m_pMountListWidget*/);
        //pListWidgetItem缓存文件挂载路径
        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
        QString strPath = LocationFile->path();
        QString rename = "";
        rename = durlAndNameMap[QUrl(mount->getRootFile()->uri())];
        if ("" == rename) {
            rename = mount->name();
        }
        //判断路径是否存在
        bool bFind = false;
        QDir dir(strPath);
        if (!dir.exists()) {
            delete pListWidgetItem;
            pListWidgetItem = nullptr;
            qDebug() << "onLoadMountImagesStart() !dir.exists()";
            dApp->signalM->sigLoadMountImagesEnd(rename);
//            return;
        } else {
            bFind = true;
        }
        //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
        if (!strPath.contains("/media/")) {
            bFind = findPicturePathByPhone(strPath);
            if (!bFind) {
                qDebug() << "onLoadMountImagesStart() !bFind";
                dApp->signalM->sigLoadMountImagesEnd(rename);
//                return;
            }
        }
        if (pListWidgetItem) {
            m_pLeftListView->m_pMountListWidget->addItem(pListWidgetItem);
            qDebug() << "strPath :" << strPath << endl;
            pListWidgetItem->setData(Qt::UserRole, strPath);
            pListWidgetItem->setData(Qt::UserRole + 1, rename);
            QVariant value = QVariant::fromValue(mount);
            pListWidgetItem->setData(Qt::UserRole + 2, value);
            pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
            AlbumLeftTabItem *pAlbumLeftTabItem;

            if (strPath.contains("/media/")) {
                pAlbumLeftTabItem = new AlbumLeftTabItem(rename, ALBUM_PATHTYPE_BY_U);
            } else {
                pAlbumLeftTabItem = new AlbumLeftTabItem(rename, ALBUM_PATHTYPE_BY_PHONE);
            }

            pAlbumLeftTabItem->setExternalDevicesMountPath(strPath);
            connect(pAlbumLeftTabItem, &AlbumLeftTabItem::unMountExternalDevices, this, &AlbumView::onUnMountSignal);
            if (!m_pLeftListView) {
                return;
            }
            m_pLeftListView->m_pMountListWidget->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
            if (m_itemClicked == true) {
                m_pLeftListView->m_pMountListWidget->setCurrentItem(pListWidgetItem);
            }
            if (bFind) {
                ImageEngineApi::instance()->getImageFilesFromMount(rename, strPath, this);
            }
        }
#endif
    }
}

void AlbumView::updateExternalDevice(QExplicitlySharedDataPointer<DGioMount> mount, QString strPath)
{
    QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftListView->m_pMountListWidget, devType);
    //pListWidgetItem缓存文件挂载路径
    if (pListWidgetItem) {
        pListWidgetItem->setData(Qt::UserRole, strPath);
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
    }
    AlbumLeftTabItem *pAlbumLeftTabItem;
    QString rename = "";
    rename = durlAndNameMap[QUrl(mount->getRootFile()->uri())];
    if ("" == rename) {
        rename = mount->name();
    }
    if (pListWidgetItem) {
        pListWidgetItem->setData(Qt::UserRole + 1, rename);
        QVariant value = QVariant::fromValue(mount);
        pListWidgetItem->setData(Qt::UserRole + 2, value);
    }
    if (strPath.contains("/media/")) {
        pAlbumLeftTabItem = new AlbumLeftTabItem(rename, ALBUM_PATHTYPE_BY_U);
    } else {
        pAlbumLeftTabItem = new AlbumLeftTabItem(rename, ALBUM_PATHTYPE_BY_PHONE);
    }
    pAlbumLeftTabItem->setExternalDevicesMountPath(strPath);
    pAlbumLeftTabItem->oriAlbumStatus();
    connect(pAlbumLeftTabItem, &AlbumLeftTabItem::unMountExternalDevices, this, &AlbumView::onUnMountSignal);
    if (!m_pLeftListView) {
        return;
    }
    m_itemClicked = true;
    if (pListWidgetItem) {
        m_pLeftListView->m_pMountListWidget->addItem(pListWidgetItem);
        m_pLeftListView->m_pMountListWidget->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
        m_pLeftListView->m_pMountListWidget->setCurrentItem(pListWidgetItem);
    }
    //右侧视图同时切换
    m_pRightPhoneThumbnailList->stopLoadAndClear(true);     //清除已有数据
    m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_PHONE);
    m_pStatusBar->setVisible(true);
    m_mounts = getVfsMountList();
}

void AlbumView::onUpdataAlbumRightTitle(QString titlename)
{
    m_currentAlbum = titlename;
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

            //更新搜索结果为空，清除搜索界面没有切换到初始状态   xiaolong 2020/05/22
            updateRightNoTrashView();
        }
        m_pStatusBar->setVisible(true);
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
        //针对ptp模式
        if (tempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0) {
            path = tempFileInfo.absoluteFilePath();
            return true;
        } else {        //针对MTP模式
            //  return true;
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
//            return false;
        }
    }
    return false;
}

void AlbumView::updateImportComboBox()
{
//    QMutexLocker locker(&m_mutex);
    m_importByPhoneComboBox->clear();
    m_importByPhoneComboBox->addItem(tr("Gallery"));
    m_importByPhoneComboBox->addItem(tr("New album"));
    QStringList allAlbumNames = DBManager::instance()->getAllAlbumNames();
    for (auto albumName : allAlbumNames) {
        m_importByPhoneComboBox->addItem(albumName);
    }
    m_importByPhoneComboBox->setCurrentText(tr("Gallery"));     //默认选中
}

//手机照片全部导入
void AlbumView::importAllBtnClicked()
{
    QStringList allPaths = m_pRightPhoneThumbnailList->getAllPaths();
    QString albumNameStr = m_importByPhoneComboBox->currentText();
    if (m_importByPhoneComboBox->currentIndex() == 0)
        albumNameStr = "";
    ImageEngineApi::instance()->importImageFilesFromMount(albumNameStr, allPaths, this);
    for (int i = 0; i < m_pLeftListView->m_pMountListWidget->count(); i++) {
        QListWidgetItem *pListWidgetItem = m_pLeftListView->m_pMountListWidget->item(i);
        AlbumLeftTabItem *pAlbumLeftTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pMountListWidget->itemWidget(pListWidgetItem));
        if (!pAlbumLeftTabItem) continue;
        if (albumNameStr == pAlbumLeftTabItem->m_albumNameStr) {
            m_pLeftListView->m_pMountListWidget->setCurrentRow(i);
            break;
        }
    }
}

//手机照片导入选中
void AlbumView::importSelectBtnClicked()
{
    QStringList selectPaths = m_pRightPhoneThumbnailList->selectedPaths();
    QString albumNameStr = m_importByPhoneComboBox->currentText();
    if (m_importByPhoneComboBox->currentIndex() == 0)
        albumNameStr = "";
    ImageEngineApi::instance()->importImageFilesFromMount(albumNameStr, selectPaths, this);
    for (int i = 0; i < m_pLeftListView->m_pMountListWidget->count(); i++) {
        QListWidgetItem *pListWidgetItem = m_pLeftListView->m_pMountListWidget->item(i);
        AlbumLeftTabItem *pAlbumLeftTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pMountListWidget->itemWidget(pListWidgetItem));
        if (!pAlbumLeftTabItem) continue;
        if (albumNameStr == pAlbumLeftTabItem->m_albumNameStr) {
            m_pLeftListView->m_pMountListWidget->setCurrentRow(i);
            break;
        }
    }
}

bool AlbumView::imageMountImported(QStringList &filelist)
{
    Q_UNUSED(filelist);
    emit dApp->signalM->closeWaitDialog();
    return true;
}

void AlbumView::needUnMount(QString path)
{
//    QStringList blDevList = m_diskManager->blockDevices();
    QStringList blDevList = DDiskManager::blockDevices(QVariantMap());
    qDebug() << "blDevList:" << blDevList;
    QSharedPointer<DBlockDevice> blkget;
    QString mountPoint = "";
    for (const QString &blks : blDevList) {
        QSharedPointer<DBlockDevice> blk(DDiskManager::createBlockDevice(blks));
        QScopedPointer<DDiskDevice> drv(DDiskManager::createDiskDevice(blk->drive()));
        if (!blk->hasFileSystem() && !drv->mediaCompatibility().join(" ").contains("optical") && !blk->isEncrypted()) {
            continue;
        }
        if ((blk->hintIgnore() && !blk->isEncrypted()) || blk->cryptoBackingDevice().length() > 1) {
            continue;
        }

        QByteArrayList qbl = blk->mountPoints();
        mountPoint = "file://";
        for (QByteArray qb : qbl) {
            mountPoint += qb;
        }
//        std::accumulate(qbl.first(), qbl.last(), mountPoint);
        if (mountPoint.contains(path, Qt::CaseSensitive)) {
            blkget = blk;
            break;
        } else {
            mountPoint = "";
        }
    }

//    if ("" == mountPoint) {
//        for (auto mount : m_mounts) {
//            //            QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
//            //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
//            QString strPath = mount->getDefaultLocationFile()->path();
//            if (!strPath.contains("/media/")) {
//                findPicturePathByPhone(strPath);
//            }
//            if (strPath.compare(path) == 0 && mount->canUnmount()) {
//                auto it = m_phoneNameAndPathlist.begin();
//                while (it != m_phoneNameAndPathlist.end()) {
//                    if (it.key().contains(strPath)) {
//                        emit dApp->signalM->sigDevStop(it.key());
//                        ImageEngineApi::instance()->removeImage(it.value());//从内存中删除
//                        m_phoneNameAndPathlist.erase(it);
//                        break;
//                    }
//                    it++;
//                }

//                m_mounts.removeOne(mount);
//                break;
//            }
//        }
//        //设备已卸载，未能在list上移除
//        QWidget *wdg  = m_pLeftListView->m_pMountListWidget->itemWidget(m_pLeftListView->m_pMountListWidget->currentItem());
//        m_pLeftListView->m_pMountListWidget->removeItemWidget(m_pLeftListView->m_pMountListWidget->currentItem());
//        wdg->deleteLater();
//        QListWidgetItem *pitem = m_pLeftListView->m_pMountListWidget->takeItem(m_pLeftListView->m_pMountListWidget->currentRow());
//        delete pitem;
//        pitem = nullptr;
//        //转到已导入界面
//        if (m_pLeftListView->m_pMountListWidget->count() == 0) {
//            m_pLeftListView->m_pPhotoLibListView->setCurrentRow(0);
//            QModelIndex index;
//            emit m_pLeftListView->m_pPhotoLibListView->pressed(index);
//            m_currentAlbum = COMMON_STR_RECENT_IMPORTED;
//            m_currentType = COMMON_STR_RECENT_IMPORTED;
//            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TIMELINE_IMPORT);
//            m_pStatusBar->setVisible(true);
//        }
//        return ;
//    }
//    for (auto mount : m_mounts) {
//        if (!blkget) {
//            continue;
//        }
//        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
//        if (LocationFile->path().compare(path) == 0 && mount->canUnmount()) {
//            QScopedPointer<DDiskDevice> drv(DDiskManager::createDiskDevice(blkget->drive()));
//            QScopedPointer<DBlockDevice> cbblk(DDiskManager::createBlockDevice(blkget->cryptoBackingDevice()));
//            bool err = false;
//            if (!blkget->mountPoints().empty()) {
//                blkget->unmount({});
//                err |= blkget->lastError().isValid();
//            }
//            if (blkget->cryptoBackingDevice().length() > 1) {
//                cbblk->lock({});
//                err |= cbblk->lastError().isValid();
//                drv.reset(DDiskManager::createDiskDevice(cbblk->drive()));
//            }
//            drv->powerOff({});
//            err |= drv->lastError().isValid();
//            if (err) {
//                DDialog msgbox(this);
//                msgbox.setFixedWidth(400);
//                msgbox.setIcon(DMessageBox::standardIcon(DMessageBox::Critical));
//                msgbox.setTextFormat(Qt::AutoText);
//                msgbox.setMessage(tr("Disk is busy, cannot eject now"));
//                msgbox.insertButton(1, tr("OK"), false, DDialog::ButtonNormal);
//                auto ret = msgbox.exec();
//                Q_UNUSED(ret);
//                return;
//            }
//            break;
//        }
//    }

    if (m_waitDeviceScandialog) {
        m_waitDeviceScandialog->close();
    }
    for (int i = 0; i < m_pLeftListView->m_pMountListWidget->count(); i++) {
        QListWidgetItem *pListWidgetItem = m_pLeftListView->m_pMountListWidget->item(i);
        if (!pListWidgetItem)
            continue;
        QExplicitlySharedDataPointer<DGioMount> mount =
            qvariant_cast<QExplicitlySharedDataPointer<DGioMount>>(pListWidgetItem->data(Qt::UserRole + 2));
        QString strPath = mount->getDefaultLocationFile()->path();
        if (!strPath.contains("/media/")) {
            findPicturePathByPhone(strPath);
        }
        if (strPath == path) {
            if (1 < m_pLeftListView->m_pMountListWidget->count()) {
                QWidget *wdg  = m_pLeftListView->m_pMountListWidget->itemWidget(pListWidgetItem);
                m_pLeftListView->m_pMountListWidget->removeItemWidget(pListWidgetItem);
                wdg->deleteLater();
                m_pLeftListView->m_pMountListWidget->takeItem(m_pLeftListView->m_pMountListWidget->row(pListWidgetItem));
                delete pListWidgetItem;
            } else {
                m_pLeftListView->m_pMountListWidget->clear();
                m_pLeftListView->updatePhotoListView();
            }
            emit dApp->signalM->sigDevStop(strPath);
            m_phoneNameAndPathlist.remove(strPath);
            QUrl qurl(mount->getRootFile()->uri());
            durlAndNameMap.erase(durlAndNameMap.find(qurl));

            QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
            if (LocationFile->path().compare(path) == 0 && mount->canUnmount()) {
                QScopedPointer<DDiskDevice> drv(DDiskManager::createDiskDevice(blkget->drive()));
                QScopedPointer<DBlockDevice> cbblk(DDiskManager::createBlockDevice(blkget->cryptoBackingDevice()));
                bool err = false;
                if (!blkget->mountPoints().empty()) {
                    blkget->unmount({});
                    err |= blkget->lastError().isValid();
                }
                if (blkget->cryptoBackingDevice().length() > 1) {
                    cbblk->lock({});
                    err |= cbblk->lastError().isValid();
                    drv.reset(DDiskManager::createDiskDevice(cbblk->drive()));
                }
                drv->powerOff({});
                err |= drv->lastError().isValid();
                if (err) {
                    DDialog msgbox(this);
                    msgbox.setFixedWidth(400);
                    msgbox.setIcon(DMessageBox::standardIcon(DMessageBox::Critical));
                    msgbox.setTextFormat(Qt::AutoText);
                    msgbox.setMessage(tr("Disk is busy, cannot eject now"));
                    msgbox.insertButton(1, tr("OK"), false, DDialog::ButtonNormal);
                    auto ret = msgbox.exec();
                    Q_UNUSED(ret);
                    return;
                }
                break;
            }
            break;
        }
//        mount->unmount();
//        mount->eject();
    }
    m_mounts = getVfsMountList();
}

//卸载外部设备
void AlbumView::onUnMountSignal(QString unMountPath)
{
    m_pRightPhoneThumbnailList->stopLoadAndClear();
    needUnMount(unMountPath);
}

void AlbumView::onLeftListDropEvent(QModelIndex dropIndex)
{

    qDebug() << "AlbumView::onLeftListDropEvent()";
    ThumbnailListView *currentViewList;
    QStringList dropItemPaths;
    AlbumLeftTabItem *item = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pCustomizeListView->itemWidget(m_pLeftListView->m_pCustomizeListView->item(dropIndex.row())));
    QString dropLeftTabListName = item->m_albumNameStr;
    qDebug() << "currentAlbum: " << m_currentAlbum << " ;dropLeftTabListName: " << dropLeftTabListName;

    //向自己的相册或“已导入”相册拖拽无效
    //“已导入”相册在leftlistwidget.cpp中也屏蔽过
    if (COMMON_STR_FAVORITES == m_currentAlbum) {
        currentViewList = m_pRightFavoriteThumbnailList;
        dropItemPaths = currentViewList->getDagItemPath();
    } else if (COMMON_STR_TRASH == m_currentAlbum) {
        currentViewList = m_pRightTrashThumbnailList;
        dropItemPaths = currentViewList->getDagItemPath();
    } else if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum) {
        dropItemPaths = m_pImpTimeLineWidget->selectPaths();
    } else {
        currentViewList = m_pRightThumbnailList;
        dropItemPaths = currentViewList->getDagItemPath();
    }
    qDebug() << "dropItemPaths: " << dropItemPaths;

    //向其他相册拖拽，动作添加
    DBManager::instance()->insertIntoAlbum(item->m_albumNameStr, dropItemPaths);
    //LMH0509,为了解决24887 【相册】【5.6.9.13】拖动已导入相册中的图片到新建相册，相册崩溃
    QModelIndex index;
    emit m_pLeftListView->m_pCustomizeListView->pressed(index);
    m_pLeftListView->m_pCustomizeListView->setCurrentRow(dropIndex.row());
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
            selPicNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum, AlbumDBType::Favourite);
        } else {
            if (5 == m_pRightStackWidget->currentIndex()) {
                selPicNum = m_mountPicNum;
            } else {
                //CUSTOM
                selPicNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);
            }
        }
    }
    if (selPicNum <= 0) {
        m_pStatusBar->setVisible(false);
    } else {
        m_pStatusBar->setVisible(true);
    }
    m_pStatusBar->m_pAllPicNumLabel->setText(str.arg(QString::number(selPicNum)));
}

MountLoader::MountLoader(AlbumView *parent)
    : m_parent(parent), bIsRunning(false), bneedunmountpath(false)
{

}

void MountLoader::onCopyPhotoFromPhone(QStringList phonepaths, QStringList systempaths)
{
    for (int i = 0; i < phonepaths.length(); i++) {
        if (QFile::copy(phonepaths[i], systempaths[i])) {
            qDebug() << "onCopyPhotoFromPhone()";
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

void AlbumView::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    if (nullptr != pPhoneWidget) {
        m_pRightPhoneThumbnailList->setFixedSize(pPhoneWidget->size());
        phonetopwidget->setFixedWidth(pPhoneWidget->size().width());
    }

}

void AlbumView::importDialog()
{
    //导入取消窗口
    m_waitDeviceScandialog->show();
    m_waitDailog_timer->start(2000);
    this->setDisabled(true);
    m_waitDeviceScandialog->setEnabled(true);
}

void AlbumView::onWaitDialogClose()
{
    qDebug() << "zy------onWaitDialogClose activeThreadCount1 = " << QThreadPool::globalInstance()->activeThreadCount();
    m_pRightPhoneThumbnailList->stopLoadAndClear(false);
    QListWidgetItem *item = m_pLeftListView->m_pMountListWidget->currentItem();
    if (item) {
        QString deviceName = item->data(Qt::UserRole).toString();
        emit dApp->signalM->sigDevStop(deviceName);
    }
    emit dApp->signalM->sigDevStop("");
    qDebug() << "zy------AlbumView::onWaitDialogClose empty";

    m_waitDeviceScandialog->close();
    this->setEnabled(true);
    qDebug() << "zy------onWaitDialogClose activeThreadCount2 = " << QThreadPool::globalInstance()->activeThreadCount();
}

void AlbumView::onWaitDialogIgnore()
{
    isIgnore = false;
    m_waitDeviceScandialog->hide();
    this->setEnabled(true);
}

void AlbumView::resizeEvent(QResizeEvent *e)
{
    m_spinner->move(width() / 2 + 60, (height() - 50) / 2 - 20);
    m_pImpTimeLineWidget->setFixedSize(width() - 181, height());
//    m_pImpTimeLineWidget->setFixedWidth(width() - 181);
////    m_pImpTimeLineWidget->setFixedHeight(height() - 35); //edit 3975
//    m_pImpTimeLineWidget->setFixedHeight(height());
////    m_pwidget->setFixedWidth(this->width() / 2 + 150);
////    m_pwidget->setFixedHeight(443);
////    m_pwidget->move(this->width() / 4, this->height() - 443 - 23);
//    m_pwidget->setFixedHeight(this->height() - 23);
//    m_pwidget->setFixedWidth(this->width());
    m_pwidget->setFixedSize(this->width(), this->height() - 23);
    m_pwidget->move(0, 0);

    //add start 3975
//    if (nullptr != m_pRightThumbnailList) {
//        m_pRightThumbnailList->setFixedSize(QSize(m_pRightTrashThumbnailList->size().width() + 1, m_pRightTrashThumbnailList->size().height()));
//        m_pRightThumbnailList->setFixedSize(QSize(m_pRightTrashThumbnailList->size().width() - 1, m_pRightTrashThumbnailList->size().height())); //触发resizeevent
//        m_pRightThumbnailList->setMinimumSize(0, 0);
//        m_pRightThumbnailList->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));  //触发后还原状态
//    }
//    if (nullptr != m_pRightFavoriteThumbnailList) {
//        m_pRightFavoriteThumbnailList->setFixedSize(QSize(m_pRightTrashThumbnailList->size().width() + 1, m_pRightTrashThumbnailList->size().height()));
//        m_pRightFavoriteThumbnailList->setFixedSize(QSize(m_pRightTrashThumbnailList->size().width() - 1, m_pRightTrashThumbnailList->size().height())); //触发resizeevent
//        m_pRightFavoriteThumbnailList->setMinimumSize(0, 0);
//        m_pRightFavoriteThumbnailList->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));  //触发后还原状态
//    }
//    if (nullptr != m_pRightTrashThumbnailList) {
//        m_pRightTrashThumbnailList->setFixedSize(QSize(m_pRightTrashThumbnailList->size().width() + 1, m_pRightTrashThumbnailList->size().height()));
//        m_pRightTrashThumbnailList->setFixedSize(QSize(m_pRightTrashThumbnailList->size().width() - 1, m_pRightTrashThumbnailList->size().height())); //触发resizeevent
//        m_pRightTrashThumbnailList->setMinimumSize(0, 0);
//        m_pRightTrashThumbnailList->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));  //触发后还原状态
//    }
    if (nullptr != m_noTrashItem) {
        m_noTrashItem->setSizeHint(QSize(this->width() - LEFT_VIEW_WIDTH, m_pRightThumbnailList->getListViewHeight() + 8 + 27));
    }
    if (nullptr != m_FavoriteItem) {
        m_FavoriteItem->setSizeHint(QSize(this->width() - LEFT_VIEW_WIDTH, m_pRightFavoriteThumbnailList->getListViewHeight() + 8 + 27));
    }
    if (nullptr != m_FavoriteItem) {
        m_TrashitemItem->setSizeHint(QSize(this->width() - LEFT_VIEW_WIDTH, m_pRightTrashThumbnailList->getListViewHeight() + 8 + 27));
    }
    if (nullptr != m_pNoTrashTitle) {
        m_pNoTrashTitle->setFixedSize(this->width() - LEFT_VIEW_WIDTH, 83);
    }
    if (nullptr != m_FavoriteTitle) {
        m_FavoriteTitle->setFixedSize(this->width() - LEFT_VIEW_WIDTH, 83);
    }
    if (nullptr != m_TrashTitle) {
        m_TrashTitle->setFixedSize(this->width() - LEFT_VIEW_WIDTH, 83);
        m_pRightTrashThumbnailList->setFixedWidth(this->width() - LEFT_VIEW_WIDTH);
        m_pTrashWidget->setFixedWidth(this->width() - LEFT_VIEW_WIDTH);
    }
    if (nullptr != pPhoneWidget) {
        m_pRightPhoneThumbnailList->setFixedSize(pPhoneWidget->size());
        phonetopwidget->setFixedWidth(pPhoneWidget->size().width());
    }
//    m_pStatusBar->move(this->width() / 4, this->height() - 27 - 81);

    //add end 3975
    m_pStatusBar->setFixedWidth(this->width() - m_pLeftListView->width());
    m_pStatusBar->move(m_pLeftListView->width(), this->height() - m_pStatusBar->height());
    fatherwidget->setFixedSize(this->size());
    QWidget::resizeEvent(e);
}
