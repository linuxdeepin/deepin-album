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

static QMutex m_mutex;


namespace {
const int ITEM_SPACING = 0;
const int LEFT_VIEW_WIDTH = 180;
const int LEFT_VIEW_LISTITEM_WIDTH = 140;
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



//ThreadRenderImage::ThreadRenderImage()
//{
//    //    connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
////    m_page = nullptr;
////    restart = false;
////    m_width = 0;
////    m_height = 0;
////    b_running = false;
//    setAutoDelete(true);
//}

////void ThreadRenderImage::setRestart()
////{
////    restart = true;
////}

//void ThreadRenderImage::setData(QFileInfo fileinfo, QString path, QMap<QString, QPixmap> *map, QStringList *list)
//{
////    qDebug() << "ThreadRenderImage::setPage" << width << height;
//    m_fileinfo = fileinfo;
//    m_path = path;
//    m_map = map;
//    m_pathlist = list;
//}

////bool ThreadRenderImage::isRunning()
////{
////    return b_running;
////}

////void ThreadRenderImage::setRunningTrue()
////{
////    b_running = true;
////}

//void ThreadRenderImage::run()
//{
//    QImage tImg;

//    QString format = DetectImageFormat(m_fileinfo.filePath());
//    if (format.isEmpty()) {
//        QImageReader reader(m_fileinfo.filePath());
//        reader.setAutoTransform(true);
//        if (reader.canRead()) {
//            tImg = reader.read();
//        } else if (m_path.contains(".tga")) {
//            bool ret = false;
//            tImg = utils::image::loadTga(m_path, ret);
//        }
//    } else {
//        QImageReader readerF(m_fileinfo.filePath(), format.toLatin1());
//        readerF.setAutoTransform(true);
//        if (readerF.canRead()) {
//            tImg = readerF.read();
//        } else {
//            qWarning() << "can't read image:" << readerF.errorString()
//                       << format;

//            tImg = QImage(m_fileinfo.filePath());
//        }
//    }

//    QPixmap pixmap = QPixmap::fromImage(tImg);
//    if (pixmap.isNull()) {
//        qDebug() << "pixmap.isNull()";
//        return;
//    }

//    pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
//    if (pixmap.isNull()) {
//        pixmap = QPixmap::fromImage(tImg);
//    }

//    QMutexLocker mutex(&m_mutex);
//    if (m_map)
//        m_map->insert(m_fileinfo.filePath(), pixmap);

//    if (m_pathlist)
//        *m_pathlist << m_fileinfo.filePath();
////    emit signal_RenderFinish(/*pixmap,*/ /*m_fileinfo.filePath()*/);
//}

AlbumViewList::AlbumViewList(QWidget *parent) : DListWidget(parent)
{
//    setContentsMargins(0, 0, 0, 0);
//    setResizeMode(QListView::Adjust);
//    setViewMode(QListView::ListMode);
//    setFlow(QListView::TopToBottom);
//    setSpacing(0);
//    setDragEnabled(false);
    connect(this->verticalScrollBar(), &QScrollBar::rangeChanged, this, [ = ](int min, int max) {
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
{
    m_pNoTrashTitle = nullptr; //add 3975
    m_FavoriteTitle = nullptr; //add 3975
    m_TrashTitle = nullptr; //add 3975

    m_currentAlbum = COMMON_STR_RECENT_IMPORTED;
    m_currentType = COMMON_STR_RECENT_IMPORTED;
    m_iAlubmPicsNum = DBManager::instance()->getImgsCount();
    m_vfsManager = new DGioVolumeManager;
    m_diskManager = new DDiskManager(this);
    m_diskManager->setWatchChanges(true);
    m_curListWidgetItem = nullptr;
    m_mountPicNum = 0;
    durlAndNameMap.clear();

    iniWaitDiolag();

//    m_waitDeviceScanMessage = new DMessageBox(DMessageBox::Warning, "", "");
//    m_waitDeviceScanMessage->addButton(m_closeDeviceScan, DMessageBox::AcceptRole);
//    m_waitDeviceScanMessage->addButton(m_ignoreDeviceScan, DMessageBox::RejectRole);

    connect(dApp->signalM, &SignalManager::sigLoadMountImagesEnd, this, &AlbumView::onLoadMountImagesEnd);

    fatherwidget = new DWidget(this);
    fatherwidget->setFixedSize(this->size());
    setAcceptDrops(true);
    initRightView();
    initLeftView();

    DWidget *leftwidget = new DWidget;
    leftwidget->setFixedWidth(180);

    DWidget *lefttopwidget = new DWidget;
    lefttopwidget->setFixedHeight(45);
    DWidget *leftbottomwidget = new DWidget;
    leftbottomwidget->setFixedHeight(22);

    QVBoxLayout *pvLayout = new QVBoxLayout();
    leftwidget->setLayout(pvLayout);
    pvLayout->addWidget(lefttopwidget);
    pvLayout->addWidget(m_pLeftListView);
    pvLayout->addWidget(leftbottomwidget);

    QHBoxLayout *pLayout = new QHBoxLayout();
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->addWidget(leftwidget);
    pLayout->addWidget(m_pRightWidget);
    fatherwidget->setLayout(pLayout);




//    QHBoxLayout *whLayout = new QHBoxLayout();
//    QHBoxLayout *tipLayout = new QHBoxLayout;
//    QVBoxLayout *wvLayout = new QVBoxLayout();
//    whLayout->addWidget(m_closeDeviceScan);
//    whLayout->addWidget(m_ignoreDeviceScan);
//    DLabel *waitTips = new DLabel(tr("loading images，please wait..."));
//    waitTips->setAlignment(Qt::AlignCenter);
//    tipLayout->addWidget(waitTips);
//    wvLayout->addLayout(tipLayout);
//    wvLayout->addLayout(whLayout);
//    m_waitDeviceScandailog->setLayout(wvLayout);

    initConnections();
    m_pwidget = new DWidget(this);
    m_pwidget->setAttribute(Qt::WA_TransparentForMouseEvents);

    m_spinner = new DSpinner(this);
    m_spinner->setFixedSize(40, 40);
    m_spinner->hide();
}

AlbumView::~AlbumView()
{
//    for (auto mount : m_vfsManager->getMounts()) {
//        QString uri = mount->getRootFile()->uri();
//        for (auto mountLoop : m_mounts) {
//            QString uriLoop = mountLoop->getRootFile()->uri();
//            if (uri == uriLoop) {
//                m_mounts.removeOne(mountLoop);
//            }
//        }
//        //QThread::msleep(100);
//    }
//    QMap<QString, QStringList>::iterator iter;
//    QString key;
//    for (iter = m_phoneNameAndPathlist.begin(); iter !=  m_phoneNameAndPathlist.end();) {
//        key = iter.key();
//        iter++;
//        m_phoneNameAndPathlist.remove(key);
//        needUnMount(key);
//    }
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
    m_itemClicked = false;
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
    connect(m_pRightFavoriteThumbnailList, &ThumbnailListView::sigSelectAll, this, &AlbumView::updatePicNum);
    connect(m_pRightThumbnailList, &ThumbnailListView::sigSelectAll, this, &AlbumView::updatePicNum);
    connect(m_pRightPhoneThumbnailList, &ThumbnailListView::sigSelectAll, this, &AlbumView::updatePicNum);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::sigSelectAll, this, &AlbumView::updatePicNum);

#endif
    connect(dApp->signalM, &SignalManager::sigLoadOnePhoto, this, &AlbumView::updateRightView);
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
                udispname = QCoreApplication::translate("DeepinStorage", "%1 Encrypted").arg(formatSize(size));
                goto runend;
                //            blk->mount({});
                //            return;
            }
            udispname = QCoreApplication::translate("DeepinStorage", "%1 Volume").arg(formatSize(size));
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
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged,
    this, [ = ] {
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
//    connect(dApp->signalM, &SignalManager::trashDelete, this, &AlbumView::onTrashDeleteUpdateClick);
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::loadEnd, this, &AlbumView::onTrashListClicked);
    connect(m_pImportView->m_pImportBtn, &DPushButton::clicked, this, [ = ] {
//        m_spinner->show();
//        m_spinner->start();
        m_pRightTitle->setVisible(false);
        m_pRightPicTotal->setVisible(false);
//        m_pImportTitle->setVisible(false); //del 3975
        if (COMMON_STR_RECENT_IMPORTED == m_currentType)
        {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TIMELINE_IMPORT);
        } else if (COMMON_STR_CUSTOM == m_currentType)
        {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
        }
        emit dApp->signalM->startImprot();
        m_pImportView->onImprotBtnClicked();
    });
    connect(dApp->signalM, &SignalManager::sigImportFailedToView, this, [ = ] {
        if (isVisible())
        {
            m_spinner->hide();
            m_spinner->stop();
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
        }
    });
    connect(m_importByPhoneComboBox, &DComboBox::currentTextChanged, this, &AlbumView::importComboBoxChange);
    connect(dApp->signalM, &SignalManager::updateFavoriteNum, this, [ = ] {
        m_iAlubmPicsNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);
        QString favoriteStr = tr("%1 photo(s)");
        m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(m_iAlubmPicsNum)));
    });
//    connect(m_pLeftListView, &LeftListView::sigKeyDelete, this, &AlbumView::onKeyDelete);
//    connect(m_pLeftListView, &LeftListView::sigKeyF2, this, &AlbumView::onKeyF2);
    connect(dApp->signalM, &SignalManager::sigShortcutKeyDelete, this, &AlbumView::onKeyDelete);
    connect(dApp->signalM, &SignalManager::sigShortcutKeyF2, this, &AlbumView::onKeyF2);

    connect(dApp->signalM, &SignalManager::updateThumbnailViewSize, this, &AlbumView::onUpdateThumbnailViewSize);

    //2020年03月26日15:12:23
    connect(dApp->signalM, &SignalManager::waitDevicescan, this, &AlbumView::importDialog);
    connect(m_waitDailog_timer, &QTimer::timeout, this, [ = ] {
        m_ignoreDeviceScan->setEnabled(true);
        m_closeDeviceScan->setEnabled(true);
        m_waitDailog_timer->stop();
    });

    connect(m_pRightPhoneThumbnailList, &ThumbnailListView::loadEnd, this, &AlbumView::onWaitDialogIgnore);
    connect(m_closeDeviceScan, &DPushButton::clicked, this, &AlbumView::onWaitDialogClose);
    connect(m_ignoreDeviceScan, &DPushButton::clicked, this, &AlbumView::onWaitDialogIgnore);
}

void AlbumView::initLeftView()
{
    m_pLeftListView = new LeftListView(this);
    m_pLeftListView->m_pPhotoLibListView->setCurrentRow(0);

    //init externalDevice
    m_mounts = getVfsMountList();
    initExternalDevice();
}

void AlbumView::onCreateNewAlbumFromDialog(QString newalbumname)
{
    int index = m_pLeftListView->m_pCustomizeListView->count();

    QListWidgetItem *pListWidgetItem = new QListWidgetItem();
    m_pLeftListView->m_pCustomizeListView->insertItem(index, pListWidgetItem);
    pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));

    QString albumName = newalbumname;
    AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName);

    m_pLeftListView->m_pCustomizeListView->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    m_pLeftListView->m_pCustomizeListView->setCurrentRow(index);

    m_pLeftListView->moveMountListWidget();

    //清除其他已选中的项
    m_pLeftListView->m_pPhotoLibListView->clearFocus();

}

#if 1
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

void AlbumView::onLoadMountImagesEnd(QString mountname)
{

}
#endif

void AlbumView::iniWaitDiolag()
{
    m_waitDeviceScandailog = new DDialog();
    m_waitDailog_timer = new QTimer(this);
    m_closeDeviceScan = new DPushButton(tr("Cancel"));
    m_ignoreDeviceScan = new DPushButton(tr("Ignore"));
    QPixmap iconImage = QPixmap(":/icons/deepin/builtin/icons/Bullet_window_warning.svg");
    QPixmap iconI = iconImage.scaled(30, 30);
    QIcon icon(iconImage);
    m_waitDeviceScandailog->setIcon(icon);

    if (!m_waitDeviceScandailog) {
        return;
    }
    //m_waitDeviceScandailog->setWindowFlag(Qt::WindowTitleHint);
    m_waitDeviceScandailog->setFixedSize(QSize(422, 183));
    m_waitDeviceScandailog->move(749, 414);
    DLabel *waitTips = new DLabel(tr("loading images，please wait..."));
    waitTips->setAlignment(Qt::AlignCenter);
    m_waitDeviceScandailog->insertContent(0, waitTips);
    m_waitDeviceScandailog->insertButton(1, m_closeDeviceScan);
    m_waitDeviceScandailog->insertButton(2, m_ignoreDeviceScan);

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

    // Thumbnail View
    //    DWidget *pNoTrashWidget = new DWidget();  //del 3975
    m_pNoTrashWidget = new DWidget(); //add 3975
    //    pNoTrashWidget->setBackgroundRole(DPalette::Window);  //del 3975
    //add start 3975
    DPalette palcolor = DApplicationHelper::instance()->palette(m_pNoTrashWidget);
    palcolor.setBrush(DPalette::Base, palcolor.color(DPalette::Window));
    m_pNoTrashWidget->setPalette(palcolor);
    //add end 3975

    QVBoxLayout *pNoTrashVBoxLayout = new QVBoxLayout();
    pNoTrashVBoxLayout->setContentsMargins(0, 0, 0, 0);

    m_pRightTitle = new DLabel();
    DFontSizeManager::instance()->bind(m_pRightTitle, DFontSizeManager::T3, QFont::DemiBold);
    m_pRightTitle->setForegroundRole(DPalette::TextTitle);

    m_pRightPicTotal = new DLabel();
    DFontSizeManager::instance()->bind(m_pRightPicTotal, DFontSizeManager::T6, QFont::Medium);
    m_pRightPicTotal->setForegroundRole(DPalette::TextTips);

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
    lsitWidget->verticalScrollBar()->setSingleStep(5);

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

    m_pRightThumbnailList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pRightThumbnailList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

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
    m_pNoTrashTitle->setFixedSize(this->width() - 200, 83);
//add end 3975

// Trash View
//    DWidget *pTrashWidget = new DWidget(); //del 3975
    m_pTrashWidget = new DWidget(); //add 3975
//add start 3975
    DPalette palcolor3 = DApplicationHelper::instance()->palette(m_pTrashWidget);
    palcolor3.setBrush(DPalette::Base, palcolor3.color(DPalette::Window));
    m_pTrashWidget->setPalette(palcolor3);
//add end 3975
//    QVBoxLayout *pMainVBoxLayout = new QVBoxLayout();
//    QHBoxLayout *pTopHBoxLayout = new QHBoxLayout();

//    QVBoxLayout *pTopLeftVBoxLayout = new QVBoxLayout();

//    pLabel1 = new DLabel();
//    DFontSizeManager::instance()->bind(pLabel1, DFontSizeManager::T3, QFont::DemiBold);
//    pLabel1->setFixedHeight(32);
//    pLabel1->setForegroundRole(DPalette::TextTitle);
//    pLabel1->setText(tr("Trash"));

//    pLabel2 = new DLabel();
//    DFontSizeManager::instance()->bind(pLabel2, DFontSizeManager::T6, QFont::Medium);
//    pLabel2->setForegroundRole(DPalette::TextTips);
//    pLabel2->setText(tr("The photos will be permanently deleted after the days shown on it"));

//    pTopLeftVBoxLayout->addSpacing(3);
//    pTopLeftVBoxLayout->addWidget(pLabel1);
//    pTopLeftVBoxLayout->addSpacing(10);
//    pTopLeftVBoxLayout->addWidget(pLabel2);
//    pTopLeftVBoxLayout->addSpacing(-1);
//    pTopLeftVBoxLayout->setContentsMargins(3, 0, 0, 0); //edit 3975

//    QHBoxLayout *pTopRightVBoxLayout = new QHBoxLayout();
//    m_pRecoveryBtn = new DPushButton();

//    m_pRecoveryBtn->setText(tr("Restore"));
//    m_pRecoveryBtn->setEnabled(false);
//    m_pRecoveryBtn->setFixedSize(120, 36);

//    DPalette ReBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
//    ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
//    m_pRecoveryBtn->setPalette(ReBtn);

//    m_pDeleteBtn = new DWarningButton();

//    m_pDeleteBtn->setText(tr("Delete All"));
//    m_pDeleteBtn->setFixedSize(120, 36);

//    DPalette DeBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
//    ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
//    m_pDeleteBtn->setPalette(ReBtn);

//    pTopRightVBoxLayout->addWidget(m_pRecoveryBtn);
//    pTopRightVBoxLayout->addSpacing(10);
//    pTopRightVBoxLayout->addWidget(m_pDeleteBtn);

//    pTopHBoxLayout->addItem(pTopLeftVBoxLayout);
//    pTopHBoxLayout->addStretch();
//    pTopHBoxLayout->addItem(pTopRightVBoxLayout);
//    pTopHBoxLayout->addSpacing(20);

    QHBoxLayout *pTopHBoxLayout = new QHBoxLayout();

    pLabel1 = new DLabel();
    DFontSizeManager::instance()->bind(pLabel1, DFontSizeManager::T3, QFont::DemiBold);
    pLabel1->setFixedHeight(32);
    pLabel1->setForegroundRole(DPalette::TextTitle);
    pLabel1->setText(tr("Trash"));
    pTopHBoxLayout->addWidget(pLabel1);

    QHBoxLayout *pTopButtonLayout  = new QHBoxLayout();

    m_pRecoveryBtn = new DPushButton();
    m_pRecoveryBtn->setText(tr("Restore"));
    m_pRecoveryBtn->setEnabled(false);
    m_pRecoveryBtn->setFixedSize(120, 36);

    DPalette ReBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
    ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_pRecoveryBtn->setPalette(ReBtn);

    pTopButtonLayout->addWidget(m_pRecoveryBtn);
    pTopButtonLayout->addSpacing(10);

    m_pDeleteBtn = new DWarningButton();
    m_pDeleteBtn->setText(tr("Delete All"));
    m_pDeleteBtn->setFixedSize(120, 36);

    DPalette DeBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
    ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_pDeleteBtn->setPalette(ReBtn);
    pTopButtonLayout->addWidget(m_pDeleteBtn);

    pTopHBoxLayout->addLayout(pTopButtonLayout);
    pTopHBoxLayout->addSpacing(10);

    QVBoxLayout *pTopVBoxLayout = new QVBoxLayout();
    pTopVBoxLayout->addLayout(pTopHBoxLayout);

    pLabel2 = new DLabel();
    DFontSizeManager::instance()->bind(pLabel2, DFontSizeManager::T6, QFont::Medium);
    pLabel2->setForegroundRole(DPalette::TextTips);
    pLabel2->setText(tr("The photos will be permanently deleted after the days shown on it"));

    pTopVBoxLayout->addWidget(pLabel2);

    m_pRightTrashThumbnailList = new ThumbnailListView(ThumbnailDelegate::AlbumViewType, COMMON_STR_TRASH);
    m_pRightTrashThumbnailList->setFrameShape(DTableView::NoFrame);
//del start 3975
//    pMainVBoxLayout->setMargin(2);
//    pMainVBoxLayout->addItem(pTopHBoxLayout);
//    pMainVBoxLayout->addSpacing(2);
//    pMainVBoxLayout->addWidget(m_pRightTrashThumbnailList);

//    pTrashWidget->setLayout(pMainVBoxLayout);
//del end 3975
//add start 3975
//    DListWidget *lsitWidget3 = new DListWidget();
    AlbumViewList *lsitWidget3 = new AlbumViewList();
    lsitWidget3->setContentsMargins(0, 0, 0, 0);
    lsitWidget3->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    lsitWidget3->setResizeMode(QListWidget::Adjust);
    lsitWidget3->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    lsitWidget3->verticalScrollBar()->setSingleStep(5);

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
//    m_pRightTrashThumbnailList->setListWidgetItem(m_TrashitemItem);

    lsitWidget3->insertItem(1, m_TrashitemItem);
    lsitWidget3->setItemWidget(m_TrashitemItem, m_pRightTrashThumbnailList);

    m_pRightTrashThumbnailList->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pRightTrashThumbnailList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_pRightTrashThumbnailList->setViewportMargins(-6, 0, 0, 0);
    m_pRightTrashThumbnailList->setContentsMargins(0, 0, 0, 0);
    m_TrashTitle = new DWidget(m_pTrashWidget);
    m_TrashTitle->setLayout(pTopVBoxLayout);

    DPalette ppal_light3 = DApplicationHelper::instance()->palette(m_TrashTitle);
    ppal_light3.setBrush(DPalette::Background, ppal_light3.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light3 = new QGraphicsOpacityEffect;
    opacityEffect_light3->setOpacity(0.95);
    m_TrashTitle->setPalette(ppal_light3);
    m_TrashTitle->setGraphicsEffect(opacityEffect_light3);
    m_TrashTitle->setAutoFillBackground(true);
    m_TrashTitle->move(0, 50);
    m_TrashTitle->setFixedSize(this->width() - 200, 83);
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

    int favoritePicNum = DBManager::instance()->getImgsCountByAlbum(COMMON_STR_FAVORITES);
    m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(favoritePicNum)));

    m_pRightFavoriteThumbnailList = new ThumbnailListView(ThumbnailDelegate::AlbumViewType, COMMON_STR_FAVORITES);
    m_pRightFavoriteThumbnailList->setFrameShape(DTableView::NoFrame);

    pFavoriteVBoxLayout->addSpacing(3);
    pFavoriteVBoxLayout->addWidget(m_pFavoriteTitle);
    pFavoriteVBoxLayout->addSpacing(2);
    pFavoriteVBoxLayout->addWidget(m_pFavoritePicTotal);
    pFavoriteVBoxLayout->addSpacing(-1);

    pFavoriteVBoxLayout->setContentsMargins(12, 0, 0, 0); //edit 3975
//del start 3975
//    QVBoxLayout *p_all1 = new QVBoxLayout();

//    p_all1->setMargin(2);
//    p_all1->addLayout(pFavoriteVBoxLayout);
//    p_all1->addSpacing(2);
//    p_all1->addWidget(m_pRightFavoriteThumbnailList);

//    pFavoriteWidget->setLayout(p_all1);
//del end 3975
//add start 3975
//    DListWidget *lsitWidget2 = new DListWidget();
    AlbumViewList *lsitWidget2 = new AlbumViewList();
    lsitWidget2->setContentsMargins(0, 0, 0, 0);
    lsitWidget2->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    lsitWidget2->setResizeMode(QListWidget::Adjust);
    lsitWidget2->setVerticalScrollMode(QListWidget::ScrollPerPixel);
    lsitWidget2->verticalScrollBar()->setSingleStep(5);

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
    importLabel->setForegroundRole(DPalette::TextTips);
    importLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_importByPhoneComboBox = new DComboBox;
    m_importByPhoneComboBox->setMinimumSize(QSize(190, 36));
    m_importByPhoneComboBox->setEnabled(false);

    m_importAllByPhoneBtn = new DPushButton(tr("Import All"));
    DFontSizeManager::instance()->bind(m_importAllByPhoneBtn, DFontSizeManager::T6);
    m_importAllByPhoneBtn ->setMinimumSize(110, 36);
    DPalette importAllByPhoneBtnPa = DApplicationHelper::instance()->palette(m_importAllByPhoneBtn);
    importAllByPhoneBtnPa.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_importAllByPhoneBtn->setPalette(importAllByPhoneBtnPa);
    m_importAllByPhoneBtn->setEnabled(false);

    m_importSelectByPhoneBtn = new DSuggestButton(tr("Import"));
//    m_importSelectByPhoneBtn = new DSuggestButton(tr("Import Selected"));
//    m_importSelectByPhoneBtn = new DSuggestButton(tr("Import Selected"));
    DFontSizeManager::instance()->bind(m_importSelectByPhoneBtn, DFontSizeManager::T6);

    m_importSelectByPhoneBtn->setMinimumSize(110, 36);
//    DPalette importSelectByPhoneBtnPa = DApplicationHelper::instance()->palette(m_importSelectByPhoneBtn);
//    importSelectByPhoneBtnPa.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
//    m_importSelectByPhoneBtn->setPalette(importSelectByPhoneBtnPa);
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
//    p_all2->addWidget(topwidget);
    p_all2->addLayout(allHLayout);
//    p_all2->addWidget(m_pRightPhoneThumbnailList);

    m_pRightPhoneThumbnailList->setParent(pPhoneWidget);
    phonetopwidget = new DBlurEffectWidget(pPhoneWidget);
    phonetopwidget->setFixedHeight(80);
    phonetopwidget->setLayout(p_all2);
    phonetopwidget->move(0, 50);
    phonetopwidget->raise();

//    pPhoneWidget->setLayout(p_all2);

// 导入图片列表,按导入时间排列
//del start 3975
//    pImportTimeLineWidget = new DWidget();
////    pImportTimeLineWidget->setStyleSheet("background:red");
//    pImportTimeLineWidget->setBackgroundRole(DPalette::Window);

//    QVBoxLayout *pImpTimeLineVBoxLayout = new QVBoxLayout();
//    pImpTimeLineVBoxLayout->setContentsMargins(0, 0, 0, 0);

//    m_pImportTitle = new DLabel();
//    m_pImportTitle->setText(tr("Import"));
//    DFontSizeManager::instance()->bind(m_pImportTitle, DFontSizeManager::T3, QFont::DemiBold);
//    m_pImportTitle->setForegroundRole(DPalette::TextTitle);

////    m_pImportPicTotal = new DLabel();
////    QString strTitle = tr("%1 photo(s)");
////    m_pImportPicTotal->setText(strTitle.arg(QString::number(m_iAlubmPicsNum)));
////    DFontSizeManager::instance()->bind(m_pImportPicTotal, DFontSizeManager::T6, QFont::Medium);
////    m_pImportPicTotal->setForegroundRole(DPalette::TextTips);

//    m_pImpTimeLineWidget = new ImportTimeLineView(pImportTimeLineWidget);
//    m_pImpTimeLineWidget->move(-6, 40);

//    pImpTimeLineVBoxLayout->addSpacing(5);
//    pImpTimeLineVBoxLayout->addWidget(m_pImportTitle);
////    pImpTimeLineVBoxLayout->addSpacing(4);
////    pImpTimeLineVBoxLayout->addWidget(m_pImportPicTotal);
//    pImpTimeLineVBoxLayout->addSpacing(-6);

//    QHBoxLayout *pImpTimeLineHLayout = new QHBoxLayout;
//    pImpTimeLineHLayout->addSpacing(10);
//    pImpTimeLineHLayout->addLayout(pImpTimeLineVBoxLayout);
////    pImpTimeLineHLayout->addStretch();

//    QVBoxLayout *pImportAllV = new QVBoxLayout();
//    pImportAllV->setContentsMargins(0, 0, 2, 0);
//    pImportAllV->addLayout(pImpTimeLineHLayout);
//    pImportAllV->addStretch();
////    pImportAllV->addWidget(m_pImpTimeLineWidget);
//    pImportTimeLineWidget->setLayout(pImportAllV);
//del end 3975
//add start 3975
    m_pStatusBar = new StatusBar(this);
//    m_pStatusBar->setParent(this);
    m_pStatusBar->raise();
    m_pStatusBar->setFixedWidth(this->width());
    m_pStatusBar->move(0, this->height() - m_pStatusBar->height());

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
    m_pRightStackWidget->addWidget(m_pNoTrashWidget);  //edit 3975
    m_pRightStackWidget->addWidget(m_pTrashWidget);    //edit 3975
    m_pRightStackWidget->addWidget(m_pFavoriteWidget);  //edit 3975
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
        m_pStatusBar->show();

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
//    m_curThumbnaiItemList.clear();
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
    } else {

    }

    updatePicNum();
    qDebug() << "";
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
    } else {
//        m_pImpTimeLineWidget->updataLayout();
//        m_pImpTimeLineWidget->getFatherStatusBar(m_pStatusBar->m_pSlider);
        m_pImpTimeLineWidget->clearAndStartLayout();
        m_pImportView->setAlbumname(QString());
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
    }

    emit sigSearchEditIsDisplay(true);

    setAcceptDrops(true);
}

// 更新个人收藏列表
void AlbumView::updateRightMyFavoriteView()
{
    using namespace utils::image;
    DBImgInfoList infos;
    infos = DBManager::instance()->getInfosByAlbum(m_currentAlbum);

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
    m_iAlubmPicsNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);

    QString favoriteStr = tr("%1 photo(s)");
    m_pFavoritePicTotal->setText(favoriteStr.arg(QString::number(m_iAlubmPicsNum)));

    m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_FAVORITE_LIST);
    emit sigSearchEditIsDisplay(true);
    setAcceptDrops(false);
}

// 更新外接设备右侧视图
void AlbumView::updateRightMountView()
{
    if (!m_pLeftListView) {
        return;
    }
//    qDebug() << m_phoneNameAndPathlist;
    QString strPath = m_pLeftListView->m_pMountListView->currentItem()->data(Qt::UserRole).toString();
    qDebug() << "data(Qt::UserRole).toString()" << strPath;
    qDebug() << m_phoneNameAndPathlist.contains(strPath);
    qDebug() << m_phoneNameAndPathlist.value(strPath).length();

    //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
    if (!strPath.contains("/media/")) {
        bool bFind = findPicturePathByPhone(strPath);
        if (!bFind) {
//            qDebug() << "onLoadMountImagesStart() !bFind";
//            dApp->signalM->sigLoadMountImagesEnd(m_mountname);
            return;
        }
        //emit dApp->signalM->waitDevicescan();
    }
    QStringList filelist = m_phoneNameAndPathlist.value(strPath);
    if (true == m_phoneNameAndPathlist.contains(strPath) && 0 < filelist.length()) {
        m_importByPhoneComboBox->setEnabled(true);
        m_importAllByPhoneBtn->setEnabled(true);
        updateImportComboBox();
//        bcurThumbnaiItemList_str = true;
        m_curThumbnaiItemList_str.clear();      //确保每次使用，统计的数量都清除上次的，（防止遗漏）
        m_curThumbnaiItemList_str << filelist;
//        for (auto path : m_phoneNameAndPathlist.value(strPath)) {
//            ThumbnailListView::ItemInfo vi;
//            vi.path = path;
//            vi.image = m_phonePathAndImage.value(path);
//            vi.width = vi.image.width();
//            vi.height = vi.image.height();
//            m_curThumbnaiItemList << vi;
//        }

//        m_iAlubmPicsNum = m_curThumbnaiItemList.size();
//        m_mountPicNum = m_curThumbnaiItemList.size();
//        m_iAlubmPicsNum = m_pRightPhoneThumbnailList->getAllFileList().size();
//        m_mountPicNum = m_pRightPhoneThumbnailList->getAllFileList().size();
        m_iAlubmPicsNum = m_curThumbnaiItemList_str.size();
        m_mountPicNum = m_curThumbnaiItemList_str.size();
        qDebug() << "m_mountPicNum = " << m_mountPicNum;
        m_pPhoneTitle->setText(m_currentAlbum);

        QFontMetrics elideFont(m_pPhoneTitle->font());
        m_pPhoneTitle->setText(elideFont.elidedText(m_currentAlbum, Qt::ElideRight, 525));

        QString str = tr("%1 photo(s)");
        m_pPhonePicTotal->setText(str.arg(QString::number(m_iAlubmPicsNum)));

//        //保存更新之前的选择状态
//        QModelIndexList mlist = m_pRightPhoneThumbnailList->getSelectedIndexes();
//        QModelIndexList::iterator i;
//        struct Listolditem {
//            int row;
//            int column;
//        };
//        QList<Listolditem> items;
//        for (i = mlist.begin(); i != mlist.end(); ++i) {
//            Listolditem item;
//            item.row = (*i).row();
//            item.column = (*i).column();
//            items.append(item);
//        }

        m_pRightPhoneThumbnailList->m_imageType = ALBUM_PATHTYPE_BY_PHONE;
//        m_pRightPhoneThumbnailList->importFilesFromLocal(m_phoneNameAndPathlist.value(strPath));
//        m_pRightPhoneThumbnailList->insertThumbnails(m_curThumbnaiItemList);
        QThread::msleep(50);
        if (!m_pRightPhoneThumbnailList->isLoading()) {
            emit dApp->signalM->waitDevicescan();
        }
        m_pRightPhoneThumbnailList->stopLoadAndClear();
        m_pRightPhoneThumbnailList->loadFilesFromLocal(filelist, false, false);


//        //设置更新之前的选择状态
//        QList<Listolditem>::iterator j;
//        for (j = items.begin(); j != items.end(); ++j) {
//            if ((*j).row < m_pRightPhoneThumbnailList->m_model->rowCount()
//                    && (*j).column < m_pRightPhoneThumbnailList->m_model->columnCount()) {
//                QModelIndex qindex = m_pRightPhoneThumbnailList->m_model->index((*j).row, (*j).column);
//                m_pRightPhoneThumbnailList->selectionModel()->select(qindex, QItemSelectionModel::Select);
//            }
//        }

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

        m_pRightPhoneThumbnailList->stopLoadAndClear();
        if (m_curThumbnaiItemList_info.size() > 0) {
            m_pRightPhoneThumbnailList->loadFilesFromLocal(m_curThumbnaiItemList_info, false, false);
        } else {
            m_pRightPhoneThumbnailList->loadFilesFromLocal(m_curThumbnaiItemList_str, false, false);
        }
//        m_pRightPhoneThumbnailList->insertThumbnails(m_curThumbnaiItemList);

        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_PHONE);
    }

    emit sigSearchEditIsDisplay(false);
    setAcceptDrops(false);
}

// 更新新建相册列表
void AlbumView::updateRightNoTrashView()
{
    using namespace utils::image;

    DBImgInfoList infos;
    infos = DBManager::instance()->getInfosByAlbum(m_currentAlbum);

//    bcurThumbnaiItemList_str = false;
//    m_curThumbnaiItemList_info.clear();
    m_curThumbnaiItemList_info << infos;
//    m_curThumbnaiItemList << infos;
//    for (auto info : infos) {
//        ThumbnailListView::ItemInfo vi;
//        vi.name = info.fileName;
//        vi.path = info.filePath;
////                vi.image = dApp->m_imagemap.value(info.filePath);
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
//        m_pRightThumbnailList->insertThumbnails(m_curThumbnaiItemList);

        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
        m_pStatusBar->show();
    } else {
//        m_pRightThumbnailList->insertThumbnails(m_curThumbnaiItemList);
        m_pRightThumbnailList->stopLoadAndClear();
        m_pRightThumbnailList->loadFilesFromLocal(infos);
        m_pImportView->setAlbumname(m_currentAlbum);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
        m_pStatusBar->show();
    }

    emit sigSearchEditIsDisplay(true);
    setAcceptDrops(true);
    //add start 3975
//    m_noTrashItem->setSizeHint(QSize(this->width() - 200, m_pRightThumbnailList->getListViewHeight() + 8));
//    m_FavoriteItem->setSizeHint(QSize(this->width() - 200, m_pRightFavoriteThumbnailList->getListViewHeight() + 8));
    //add end 3975
}

void AlbumView::updateRightTrashView()
{
    using namespace utils::image;
//    int idaysec = 24 * 60 * 60;
//    m_curThumbnaiItemList.clear();

    DBImgInfoList infos;
//    QStringList removepaths;

    infos = DBManager::instance()->getAllTrashInfos();
//    m_curThumbnaiItemList << infos;

//    bcurThumbnaiItemList_str = false;
//    m_curThumbnaiItemList_info.clear();
    m_curThumbnaiItemList_info << infos;
//    for (auto info : infos) {
//        QDateTime start = QDateTime::currentDateTime();
//        QDateTime end = info.changeTime;

//        uint etime = start.toTime_t();
//        uint stime = end.toTime_t();

//        int Day = (etime - stime) / (idaysec) + ((etime - stime) % (idaysec) + (idaysec - 1)) / (idaysec) - 1;

//        if (30 <= Day) {
//            removepaths << info.filePath;
//        } else {
//            ThumbnailListView::ItemInfo vi;
//            vi.name = info.fileName;
//            vi.path = info.filePath;
////            vi.image = dApp->m_imagetrashmap.value(info.filePath);
//            if (dApp->m_imagetrashmap.value(info.filePath).isNull()) {
//                QSize imageSize = getImageQSize(vi.path);

//                vi.width = imageSize.width();
//                vi.height = imageSize.height();
//            } else {
//                vi.width = dApp->m_imagetrashmap.value(info.filePath).width();
//                vi.height = dApp->m_imagetrashmap.value(info.filePath).height();
//            }
//            vi.remainDays = QString::number(30 - Day) + tr("days");

//            m_curThumbnaiItemList << vi;
//        }
//    }

//    if (0 < removepaths.length()) {
////        for (auto path : removepaths) {
////            dApp->m_imagetrashmap.remove(path);
////        }

//        DBManager::instance()->removeTrashImgInfosNoSignal(removepaths);
//    }

    if (0 < infos.length()) {
        m_pDeleteBtn->setEnabled(true);
    } else {
        m_pDeleteBtn->setText(tr("Delete All"));
        m_pRecoveryBtn->setEnabled(false);
        m_pDeleteBtn->setEnabled(false);
    }

//    m_pRightTrashThumbnailList->insertThumbnails(m_curThumbnaiItemList);
    m_pRightTrashThumbnailList->stopLoadAndClear();
    m_pRightTrashThumbnailList->loadFilesFromTrash(infos);
    m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TRASH_LIST);
//    m_TrashitemItem->setSizeHint(QSize(this->width() - 200, m_pRightTrashThumbnailList->getListViewHeight() + 8)); //add 3975
}

void AlbumView::leftTabClicked()
{
    emit dApp->signalM->SearchEditClear();
    //若点击当前的item，则不做任何处理
    if (m_currentAlbum == m_pLeftListView->getItemCurrentName()) {
//        if (m_currentAlbum != COMMON_STR_FAVORITES) {
        SearchReturnUpdate();
        return;
//        }
    }

    m_currentAlbum = m_pLeftListView->getItemCurrentName();
    m_currentType = m_pLeftListView->getItemCurrentType();

    updateRightView();
}

bool AlbumView::imageGeted(QStringList &filelist, QString path)
{
    m_phoneNameAndPathlist[path] = filelist;
    if (m_itemClicked == true) {
        updateRightMountView();
    }
    return true;
}

void AlbumView::createNewAlbum(QStringList imagepaths)
{
    int index = m_pLeftListView->m_pCustomizeListView->count();

    QListWidgetItem *pListWidgetItem = new QListWidgetItem();
    m_pLeftListView->m_pCustomizeListView->insertItem(index, pListWidgetItem);
    pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
    QString albumName = getNewAlbumName();

    if (QStringList(" ") != imagepaths) {
        DBManager::instance()->insertIntoAlbum(albumName, imagepaths);
    }

    AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName);

    m_pLeftListView->m_pCustomizeListView->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);

    m_pLeftListView->m_pCustomizeListView->setCurrentRow(index);

    AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftListView->m_pCustomizeListView->itemWidget(m_pLeftListView->m_pCustomizeListView->currentItem());
    if (QStringList(" ") != imagepaths) {
        item->m_opeMode = OPE_MODE_ADDRENAMEALBUM;
    } else {
        item->m_opeMode = OPE_MODE_ADDNEWALBUM;
    }

    item->editAlbumEdit();
}

void AlbumView::onTrashRecoveryBtnClicked()
{
    QStringList paths;
    paths = m_pRightTrashThumbnailList->selectedPaths();

//    DBImgInfoList infos;
//    for (auto path : paths) {
//        DBImgInfo info;
//        info = DBManager::instance()->getTrashInfoByPath(path);
//        QFileInfo fi(info.filePath);
//        info.changeTime = QDateTime::currentDateTime();
//        infos << info;

////        dApp->m_imagetrashmap.remove(path);
//    }

////    dApp->m_imageloader->addImageLoader(paths);
//    DBManager::instance()->insertImgInfos(infos);

//    for (auto path : paths) {
//        DBImgInfo info;
//        info = DBManager::instance()->getTrashInfoByPath(path);
//        QStringList namelist = info.albumname.split(",");
//        for (auto eachname : namelist) {
//            if (DBManager::instance()->isAlbumExistInDB(eachname)) {
//                DBManager::instance()->insertIntoAlbum(eachname, QStringList(path));
//            }
//        }
//    }

//    DBManager::instance()->removeTrashImgInfos(paths);

//    onTrashListClicked();
    ImageEngineApi::instance()->recoveryImagesFromTrash(paths);
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
//        DBManager::instance()->removeTrashImgInfos(paths);
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
//    info.path = m_curThumbnaiItemList[index].path;
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

//    auto imagelist = DBManager::instance()->getInfosByAlbum(m_currentAlbum);
//    if (COMMON_STR_TRASH == m_currentAlbum) {
//        imagelist = DBManager::instance()->getAllTrashInfos();
//    } else if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum) {
//        imagelist = DBManager::instance()->getAllInfos();
//    } else {

//    }

    auto imagelist = m_pRightThumbnailList->getAllFileList();
    if (COMMON_STR_TRASH == m_currentAlbum) {
        imagelist = m_pRightTrashThumbnailList->getAllFileList();
    } else if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum) {
        imagelist = m_pRightFavoriteThumbnailList->getAllFileList();
    } else {

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
//        qDebug() << "--------Delete";
    } else {
        m_pRecoveryBtn->setEnabled(false);
        DPalette ReBtn = DApplicationHelper::instance()->palette(m_pRecoveryBtn);
        ReBtn.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
        m_pRecoveryBtn->setPalette(ReBtn);

        m_pDeleteBtn->setText(tr("Delete All"));
//        qDebug() << "--------Delete All";
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
//    using namespace utils::image;
//    QStringList paths;
//    for (QUrl url : urls) {
//        const QString path = url.toLocalFile();
//        if (QFileInfo(path).isDir()) {
//            auto finfos =  getImagesInfo(path, false);
//            for (auto finfo : finfos) {
//                if (imageSupportRead(finfo.absoluteFilePath())) {
//                    paths << finfo.absoluteFilePath();
//                }
//            }
//        } else if (imageSupportRead(path)) {
//            paths << path;
//        }
//    }

//    if (paths.isEmpty()) {
//        return;
//    }

//    // 判断当前导入路径是否为外接设备
//    int isMountFlag = 0;
//    DGioVolumeManager *pvfsManager = new DGioVolumeManager;
//    QList<QExplicitlySharedDataPointer<DGioMount>> mounts = pvfsManager->getMounts();
//    for (auto mount : mounts) {
//        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
//        QString strPath = LocationFile->path();
//        if (0 == paths.first().compare(strPath)) {
//            isMountFlag = 1;
//            break;
//        }
//    }

//    // 当前导入路径
//    if (isMountFlag) {
//        QString strHomePath = QDir::homePath();
//        //获取系统现在的时间
//        QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
//        QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
//        QDir dir;
//        if (!dir.exists(basePath)) {
//            dir.mkpath(basePath);
//        }

//        QStringList newImagePaths;
//        foreach (QString strPath, paths) {
//            //取出文件名称
//            QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
//            QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
//            QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(), QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

//            newImagePaths << strNewPath;
//            //判断新路径下是否存在目标文件，若存在，下一次张
//            if (dir.exists(strNewPath)) {
//                continue;
//            }

//            // 外接设备图片拷贝到系统
//            if (QFile::copy(strPath, strNewPath)) {

//            }
//        }

//        paths.clear();
//        paths = newImagePaths;
//    }

//    DBImgInfoList dbInfos;

//    using namespace utils::image;

//    for (auto path : paths) {
//        if (! imageSupportRead(path)) {
//            continue;
//        }

////        // Generate thumbnail and storage into cache dir
////        if (! utils::image::thumbnailExist(path)) {
////            // Generate thumbnail failed, do not insert into DB
////            if (! utils::image::generateThumbnail(path)) {
////                continue;
////            }
////        }

//        QFileInfo fi(path);
//        using namespace utils::image;
//        using namespace utils::base;
//        auto mds = getAllMetaData(path);
//        QString value = mds.value("DateTimeOriginal");
////        qDebug() << value;
//        DBImgInfo dbi;
//        dbi.fileName = fi.fileName();
//        dbi.filePath = path;
//        dbi.dirHash = utils::base::hash(QString());
//        if ("" != value) {
//            dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm:ss");
//        } else if (fi.birthTime().isValid()) {
//            dbi.time = fi.birthTime();
//        } else if (fi.metadataChangeTime().isValid()) {
//            dbi.time = fi.metadataChangeTime();
//        } else {
//            dbi.time = QDateTime::currentDateTime();
//        }
//        dbi.changeTime = QDateTime::currentDateTime();

//        dbInfos << dbi;
//    }

//    if (! dbInfos.isEmpty()) {
//        dApp->m_imageloader->ImportImageLoader(dbInfos, m_currentAlbum);
//    } else {
//        emit dApp->signalM->ImportFailed();
//    }

    event->accept();
}

void AlbumView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void AlbumView::dragLeaveEvent(QDragLeaveEvent *e)
{

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
//                emit dApp->signalM->sigDeletePhotos(paths.length());
                ImageEngineApi::instance()->moveImagesToTrash(paths, true);
//                DBManager::instance()->removeTrashImgInfos(paths);
                onTrashListClicked();
            });
        }
    } else if (COMMON_STR_FAVORITES == m_currentType) {
        paths = m_pRightFavoriteThumbnailList->selectedPaths();
        if (0 < paths.length()) {
            DBManager::instance()->removeFromAlbum(COMMON_STR_FAVORITES, paths);
        }
    } else if (COMMON_STR_CUSTOM == m_currentType) {
        paths = m_pRightThumbnailList->selectedPaths();
        // 如果没有选中的照片,或相册中的照片数为0,则删除相册
        if (0 == paths.length() || 0 == DBManager::instance()->getImgsCountByAlbum(m_currentAlbum)) {
            QString str;
            QListWidgetItem *item = m_pLeftListView->m_pCustomizeListView->currentItem();
            AlbumLeftTabItem *pTabItem = (AlbumLeftTabItem *)m_pLeftListView->m_pCustomizeListView->itemWidget(item);

            str = pTabItem->m_albumNameStr;
            DBManager::instance()->removeAlbum(pTabItem->m_albumNameStr);

            if (1 < m_pLeftListView->m_pCustomizeListView->count()) {
                delete  item;
            } else {
                m_pLeftListView->updateCustomizeListView();
                m_pLeftListView->updatePhotoListView();
            }

            m_pLeftListView->moveMountListWidget();
            emit dApp->signalM->sigAlbDelToast(str);
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
//        DBImgInfoList infos;
//        for (auto path : paths) {
//            DBImgInfo info;
//            info = DBManager::instance()->getInfoByPath(path);
//            info.changeTime = QDateTime::currentDateTime();

//            QStringList allalbumnames = DBManager::instance()->getAllAlbumNames();
//            for (auto eachname : allalbumnames) {
//                if (DBManager::instance()->isImgExistInAlbum(eachname, path)) {
//                    info.albumname += (eachname + ",");
//                }
//            }
//            infos << info;
//        }

////        dApp->m_imageloader->addTrashImageLoader(paths);
//        DBManager::instance()->insertTrashImgInfos(infos);
//        DBManager::instance()->removeImgInfos(paths);
    }
}

void AlbumView::onKeyF2()
{
    if (COMMON_STR_CUSTOM != m_currentType) return;
    AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftListView->m_pCustomizeListView->itemWidget(m_pLeftListView->m_pCustomizeListView->currentItem());

    item->m_opeMode = OPE_MODE_RENAMEALBUM;
    item->editAlbumEdit();
}

//挂载设备改变
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
            //(scheme == "afc") ||                    //iPhone document
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

//        MountLoader *pMountloader = new MountLoader(this);
//        QThread *pLoadThread = new QThread();


//        connect(pMountloader, SIGNAL(needUnMount(QString)), this, SLOT(needUnMount(QString)));
//        pMountloader->moveToThread(pLoadThread);
//        pLoadThread->start();

//        connect(pMountloader, SIGNAL(sigLoadMountImagesStart(QString, QString)), pMountloader, SLOT(onLoadMountImagesStart(QString, QString)));

        QString rename = "";
        rename = durlAndNameMap[QUrl(mount->getRootFile()->uri())];
        if ("" == rename) {
            rename = mount->name();
        }

//        emit pMountloader->sigLoadMountImagesStart(rename, strPath);
        //判断路径是否存在
        QDir dir(strPath);
        if (!dir.exists()) {
            qDebug() << "onLoadMountImagesStart() !dir.exists()";
            dApp->signalM->sigLoadMountImagesEnd(rename);
            return;
        }

        //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,再调用findPicturePathByPhone函数搜索DCIM文件目录
        if (!strPath.contains("/media/")) {
            bool bFind = findPicturePathByPhone(strPath);
            if (!bFind) {
                qDebug() << "onLoadMountImagesStart() !bFind";
                dApp->signalM->sigLoadMountImagesEnd(rename);
                return;
            }
        }


        updateExternalDevice(mount);
        ImageEngineApi::instance()->getImageFilesFromMount(rename, strPath, this);
        //emit dApp->signalM->waitDevicescan();
    }
}

//卸载外接设备
void AlbumView::onVfsMountChangedRemove(QExplicitlySharedDataPointer<DGioMount> mount)
{
    Q_UNUSED(mount);

    QString uri = mount->getRootFile()->uri();
    for (auto mountLoop : m_mounts) {
        QString uriLoop = mountLoop->getRootFile()->uri();
        if (uri == uriLoop) {
            m_mounts.removeOne(mountLoop);
        }
    }

    for (int i = 0; i < m_pLeftListView->m_pMountListView->count(); i++) {
        QListWidgetItem *pListWidgetItem = m_pLeftListView->m_pMountListView->item(i);
        AlbumLeftTabItem *pAlbumLeftTabItem = (AlbumLeftTabItem *)m_pLeftListView->m_pMountListView->itemWidget(pListWidgetItem);

//        if (mount->name() == pAlbumLeftTabItem->m_albumNameStr) {
        QString rename = "";
        QString dpath = mount->getRootFile()->uri();
        QUrl qurl(mount->getRootFile()->uri());
        rename = durlAndNameMap[qurl];
        if ("" == rename) {
            rename = mount->name();
        }

        if (rename == pAlbumLeftTabItem->m_albumNameStr &&  mount->getDefaultLocationFile()->path().contains(pAlbumLeftTabItem->m_mountPath)) {

            if (1 < m_pLeftListView->m_pMountListView->count()) {
                delete pListWidgetItem;
            } else {
                m_pLeftListView->m_pMountListView->clear();
                m_pLeftListView->updatePhotoListView();
            }
            durlAndNameMap.erase(durlAndNameMap.find(qurl));
            break;
        }
    }
//    }
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
//            udispname = QCoreApplication::translate("PathManager", "System Disk");
            udispname = tr("System Disk");
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
            udispname = QCoreApplication::translate("DeepinStorage", "%1 Volume").arg(formatSize(size));
//            udispname = QCoreApplication::translate("DeepinStorage", "%1 ").arg(formatSize(size));
//            udispname += tr("Disk");
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

//获取外部设备列表
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

void AlbumView::importComboBoxChange(QString strText)
{
    if (1 == m_importByPhoneComboBox->currentIndex()) {
        AlbumCreateDialog *dialog = new AlbumCreateDialog(this);
        dialog->show();
        qDebug() << "xxxxxxxxxx" << window()->x();
        qDebug() << "xxxxxxxxxx" << window()->y();
        qDebug() << "xxxxxxxxxx" << dialog->width();
        qDebug() << "xxxxxxxxxx" << window()->width();
        dialog->move(window()->x() + (window()->width() - dialog->width()) / 2, window()->y() + (window()->height() - dialog->height()) / 2);
        connect(dialog, &AlbumCreateDialog::albumAdded, this, [ = ] {
//            emit dApp->signalM->hideExtensionPanel();
            DBManager::instance()->insertIntoAlbum(dialog->getCreateAlbumName(), QStringList(" "));
            onCreateNewAlbumFrom(dialog->getCreateAlbumName());
//            int index = m_pLeftListView->m_pMountListView->count();;
//            QListWidgetItem *pListWidgetItem = new QListWidgetItem();
//            m_pLeftListView->m_pCustomizeListView->insertItem(index, pListWidgetItem);
//            pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
//            QString albumName = dialog->getCreateAlbumName();
//            AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumName);
//            pAlbumLeftTabItem->oriAlbumStatus();
//            m_pLeftListView->m_pCustomizeListView->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
////            m_customAlbumNames << albumName;
            updateImportComboBox();
            m_importByPhoneComboBox->setCurrentIndex(m_importByPhoneComboBox->count() - 1);

//            QIcon icon;
//            icon = utils::base::renderSVG(":/images/logo/resources/images/other/icon_toast_sucess.svg", QSize(20, 20));

//            QString str = tr("Create Album “%1” successfully");
//            DFloatingMessage *pDFloatingMessage = new DFloatingMessage(DFloatingMessage::MessageType::TransientType, m_pwidget);
//            pDFloatingMessage->setMessage(str.arg(dialog->getCreateAlbumName()));
//            pDFloatingMessage->setIcon(icon);
//            DMessageManager::instance()->sendMessage(m_pwidget, pDFloatingMessage);
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
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftListView->m_pMountListView);
        //pListWidgetItem缓存文件挂载路径
        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
        QString strPath = LocationFile->path();
        qDebug() << "strPath :" << strPath << endl;
        pListWidgetItem->setData(Qt::UserRole, strPath);
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
        AlbumLeftTabItem *pAlbumLeftTabItem;
        QString rename = "";
        rename = durlAndNameMap[QUrl(mount->getRootFile()->uri())];
        if ("" == rename) {
            rename = mount->name();
        }
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
        m_pLeftListView->m_pMountListView->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
        if (m_itemClicked == true) {
            m_pLeftListView->m_pMountListView->setCurrentItem(pListWidgetItem);
        }


//        MountLoader *pMountloader = new MountLoader(this);
//        QThread *pLoadThread = new QThread();

//        connect(pMountloader, SIGNAL(needUnMount(QString)), this, SLOT(needUnMount(QString)));
//        pMountloader->moveToThread(pLoadThread);
//        pLoadThread->start();

//        connect(pMountloader, SIGNAL(sigLoadMountImagesStart(QString, QString)), pMountloader, SLOT(onLoadMountImagesStart(QString, QString)));
////        emit pMountloader->sigLoadMountImagesStart(mount->name(), strPath);
//        emit pMountloader->sigLoadMountImagesStart(rename, strPath);


        //判断路径是否存在
        QDir dir(strPath);
        if (!dir.exists()) {
            qDebug() << "onLoadMountImagesStart() !dir.exists()";
            dApp->signalM->sigLoadMountImagesEnd(rename);
            return;
        }

        //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
        if (!strPath.contains("/media/")) {
            bool bFind = findPicturePathByPhone(strPath);
            if (!bFind) {
                qDebug() << "onLoadMountImagesStart() !bFind";
                dApp->signalM->sigLoadMountImagesEnd(rename);
                return;
            }
        }
//        ImageEngineApi::instance()->getImageFilesFromMount(rename, strPath, this);
//        m_pRightPhoneThumbnailList->stopLoadAndClear();
//        QStringList pathlist;
//        pathlist << strPath;
//        m_pRightPhoneThumbnailList->loadFilesFromLocal(pathlist, false);
        ImageEngineApi::instance()->getImageFilesFromMount(rename, strPath, this);

//        m_mountLoaderList.insert(mount->name(), pMountloader);
//        m_loadThreadList.insert(mount->name(), pLoadThread);
//        m_mountLoaderList.insert(strPath, pMountloader);
//        m_loadThreadList.insert(strPath, pLoadThread);
    }
}

void AlbumView::updateExternalDevice(QExplicitlySharedDataPointer<DGioMount> mount)
{
    QListWidgetItem *pListWidgetItem = new QListWidgetItem(m_pLeftListView->m_pMountListView);
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
    m_pLeftListView->m_pMountListView->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    m_pLeftListView->m_pMountListView->setCurrentItem(pListWidgetItem);
    m_mounts.append(mount);
}

void AlbumView::onUpdataAlbumRightTitle(QString titlename)
{
//    int index = m_customAlbumNames.indexOf(m_currentAlbum);
    m_currentAlbum = titlename;
//    m_customAlbumNames.replace(index, m_currentAlbum);
    updateRightView();
}

void AlbumView::onUpdateThumbnailViewSize()
{
//    if (nullptr != m_FavoriteItem) {
//        m_FavoriteItem->setSizeHint(QSize(this->width() - 200, m_pRightFavoriteThumbnailList->getListViewHeight() + 8 + 27));
//    }
//    if (nullptr != m_TrashitemItem) {
//        m_TrashitemItem->setSizeHint(QSize(this->width() - 200, m_pRightTrashThumbnailList->getListViewHeight() + 8 + 27));
//    }
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
        //针对p2p模式
        if (tempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0) {
            path = tempFileInfo.absoluteFilePath();
            return true;
        } else {        //针对MTP模式
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
    m_importByPhoneComboBox->addItem(tr("Gallery"));
    m_importByPhoneComboBox->addItem(tr("New album"));
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
//    QList<ThumbnailListView::ItemInfo> allPaths = m_pRightPhoneThumbnailList->getAllPaths();
    QStringList allPaths = m_pRightPhoneThumbnailList->getAllPaths();
    QString albumNameStr = m_importByPhoneComboBox->currentText();
    ImageEngineApi::instance()->importImageFilesFromMount(albumNameStr, allPaths, this);
//    QStringList picPathList;
//    QStringList newPathList;
//    DBImgInfoList dbInfos;
//    QString strHomePath = QDir::homePath();
//    //获取系统现在的时间
//    QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
//    QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
//    QDir dir;
//    if (!dir.exists(basePath)) {
//        dir.mkpath(basePath);
//    }

//    foreach (ThumbnailListView::ItemInfo info, allPaths) {
//        QString strPath = info.path;
//        QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
//        QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
//        QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(), QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

//        //判断新路径下是否存在目标文件，若存在，先删除掉
//        if (dir.exists(strNewPath)) {
//            dir.remove(strNewPath);
//        }

////        if (QFile::copy(strPath, strNewPath)) {
//        picPathList << strPath;
//        newPathList << strNewPath;
//        QFileInfo fi(strPath);
//        using namespace utils::image;
//        using namespace utils::base;
//        auto mds = getAllMetaData(strPath);
//        QString value = mds.value("DateTimeOriginal");
////        qDebug() << value;
//        DBImgInfo dbi;
//        dbi.fileName = fi.fileName();
//        dbi.filePath = strNewPath;
//        dbi.dirHash = utils::base::hash(QString());
//        if ("" != value) {
//            dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm:ss");
//        } else if (fi.birthTime().isValid()) {
//            dbi.time = fi.birthTime();
//        } else if (fi.metadataChangeTime().isValid()) {
//            dbi.time = fi.metadataChangeTime();
//        } else {
//            dbi.time = QDateTime::currentDateTime();
//        }
//        dbi.changeTime = QDateTime::currentDateTime();

//        dbInfos << dbi;
////        }
//    }

//    MountLoader *pMountloader = new MountLoader(this);
//    QThread *pLoadThread = new QThread();
//    connect(pMountloader, SIGNAL(needUnMount(QString)), this, SLOT(needUnMount(QString)));

//    pMountloader->moveToThread(pLoadThread);
//    pLoadThread->start();

//    connect(pMountloader, SIGNAL(sigCopyPhotoFromPhone(QStringList, QStringList)), pMountloader, SLOT(onCopyPhotoFromPhone(QStringList, QStringList)));
//    emit pMountloader->sigCopyPhotoFromPhone(picPathList, newPathList);

//    if (!dbInfos.isEmpty()) {
//        DBImgInfoList dbInfoList;
//        QStringList pathslist;

//        for (int i = 0; i < dbInfos.length(); i++) {
//            if (m_phonePathAndImage.value(picPathList[i]).isNull()) {
//                continue;
//            }

//            dApp->m_imagemap.insert(dbInfos[i].filePath, m_phonePathAndImage.value(picPathList[i]));

//            pathslist << dbInfos[i].filePath;
//            dbInfoList << dbInfos[i];
//        }

//        if (albumNameStr.length() > 0) {
//            if (COMMON_STR_RECENT_IMPORTED != albumNameStr
//                    && COMMON_STR_TRASH != albumNameStr
//                    && COMMON_STR_FAVORITES != albumNameStr
//                    && ALBUM_PATHTYPE_BY_PHONE != albumNameStr
//                    && 0 != albumNameStr.compare(tr("Gallery"))) {
//                DBManager::instance()->insertIntoAlbumNoSignal(albumNameStr, pathslist);
//            }
//        }

//        DBManager::instance()->insertImgInfos(dbInfoList);

//        if (dbInfoList.length() != allPaths.length()) {
//            emit dApp->signalM->ImportSomeFailed();
//        } else {
//            emit dApp->signalM->ImportSuccess();
//        }
//    } else {
//        emit dApp->signalM->ImportFailed();
//    }

    for (int i = 0; i < m_pLeftListView->m_pMountListView->count(); i++) {
        QListWidgetItem *pListWidgetItem = m_pLeftListView->m_pMountListView->item(i);
        AlbumLeftTabItem *pAlbumLeftTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pMountListView->itemWidget(pListWidgetItem));
        if (!pAlbumLeftTabItem) continue;
        if (albumNameStr == pAlbumLeftTabItem->m_albumNameStr) {
            m_pLeftListView->m_pMountListView->setCurrentRow(i);
            break;
        }
    }
}

//手机照片导入选中
void AlbumView::importSelectBtnClicked()
{
    QStringList selectPaths = m_pRightPhoneThumbnailList->selectedPaths();
    QString albumNameStr = m_importByPhoneComboBox->currentText();
    ImageEngineApi::instance()->importImageFilesFromMount(albumNameStr, selectPaths, this);
//    QStringList picPathList;
//    QStringList newPathList;
//    DBImgInfoList dbInfos;
//    QString strHomePath = QDir::homePath();
//    //获取系统现在的时间
//    QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
//    QString basePath = QString("%1%2%3").arg(strHomePath, "/Pictures/照片/", strDate);
//    QDir dir;
//    if (!dir.exists(basePath)) {
//        dir.mkpath(basePath);
//    }

//    foreach (QString strPath, selectPaths) {
//        //取出文件名称
//        QStringList pathList = strPath.split("/", QString::SkipEmptyParts);
//        QStringList nameList = pathList.last().split(".", QString::SkipEmptyParts);
//        QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(), QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());

//        //判断新路径下是否存在目标文件，若存在，先删除掉
//        if (dir.exists(strNewPath)) {
//            dir.remove(strNewPath);
//        }

////        if (QFile::copy(strPath, strNewPath)) {
//        picPathList << strPath;
//        newPathList << strNewPath;

//        QFileInfo fi(strPath);
//        using namespace utils::image;
//        using namespace utils::base;
//        auto mds = getAllMetaData(strPath);
//        QString value = mds.value("DateTimeOriginal");
////        qDebug() << value;
//        DBImgInfo dbi;
//        dbi.fileName = fi.fileName();
//        dbi.filePath = strNewPath;
//        dbi.dirHash = utils::base::hash(QString());
//        if ("" != value) {
//            dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm:ss");
//        } else if (fi.birthTime().isValid()) {
//            dbi.time = fi.birthTime();
//        } else if (fi.metadataChangeTime().isValid()) {
//            dbi.time = fi.metadataChangeTime();
//        } else {
//            dbi.time = QDateTime::currentDateTime();
//        }

//        dbi.changeTime = QDateTime::currentDateTime();

//        dbInfos << dbi;
////        }
//    }

//    MountLoader *pMountloader = new MountLoader(this);
//    QThread *pLoadThread = new QThread();

//    connect(pMountloader, SIGNAL(needUnMount(QString)), this, SLOT(needUnMount(QString)));
//    pMountloader->moveToThread(pLoadThread);
//    pLoadThread->start();

//    connect(pMountloader, SIGNAL(sigCopyPhotoFromPhone(QStringList, QStringList)), pMountloader, SLOT(onCopyPhotoFromPhone(QStringList, QStringList)));
//    emit pMountloader->sigCopyPhotoFromPhone(picPathList, newPathList);

//    if (!dbInfos.isEmpty()) {
//        DBImgInfoList dbInfoList;
//        QStringList pathslist;

//        for (int i = 0; i < dbInfos.length(); i++) {
//            if (m_phonePathAndImage.value(picPathList[i]).isNull()) {
//                continue;
//            }

//            dApp->m_imagemap.insert(dbInfos[i].filePath, m_phonePathAndImage.value(picPathList[i]));

//            pathslist << dbInfos[i].filePath;
//            dbInfoList << dbInfos[i];
//        }

//        if (albumNameStr.length() > 0) {
//            if (COMMON_STR_RECENT_IMPORTED != albumNameStr
//                    && COMMON_STR_TRASH != albumNameStr
//                    && COMMON_STR_FAVORITES != albumNameStr
//                    && ALBUM_PATHTYPE_BY_PHONE != albumNameStr
//                    && 0 != albumNameStr.compare(tr("Gallery"))) {
//                DBManager::instance()->insertIntoAlbumNoSignal(albumNameStr, pathslist);
//            }
//        }

//        DBManager::instance()->insertImgInfos(dbInfoList);

//        if (dbInfoList.length() != selectPaths.length()) {
//            emit dApp->signalM->ImportSomeFailed();
//        } else {
//            emit dApp->signalM->ImportSuccess();
//        }
//    } else {
//        emit dApp->signalM->ImportFailed();
//    }

    for (int i = 0; i < m_pLeftListView->m_pMountListView->count(); i++) {
        QListWidgetItem *pListWidgetItem = m_pLeftListView->m_pMountListView->item(i);
        AlbumLeftTabItem *pAlbumLeftTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pMountListView->itemWidget(pListWidgetItem));
        if (!pAlbumLeftTabItem) continue;
        if (albumNameStr == pAlbumLeftTabItem->m_albumNameStr) {
            m_pLeftListView->m_pMountListView->setCurrentRow(i);
            break;
        }
    }
}

bool AlbumView::imageMountImported(QStringList &filelist)
{
    emit dApp->signalM->closeWaitDialog();
    return true;
}

void AlbumView::needUnMount(QString path)
{
    QStringList blDevList = m_diskManager->blockDevices();
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
        if (mountPoint.contains(path, Qt::CaseSensitive)) {
            blkget = blk;
            break;
        } else {
            mountPoint = "";
        }
    }
    if ("" == mountPoint) {
        for (auto mount : m_mounts) {
            QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
            if (LocationFile->path().compare(path) == 0 && mount->canUnmount()) {
                mount->unmount(true);
//                m_mounts.removeOne(mount);
                break;
            }
        }
        return;
    }
    for (auto mount : m_mounts) {
        QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
        if (LocationFile->path().compare(path) == 0 && mount->canUnmount()) {
//            mount->unmount(true);

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
//                msgbox.setTitle(tr("Format USB flash drive"));
                msgbox.setTextFormat(Qt::AutoText);
                msgbox.setMessage(tr("Disk is busy, cannot eject now"));
                msgbox.insertButton(1, tr("OK"), false, DDialog::ButtonWarning);

                auto ret = msgbox.exec();
                return;
//                dialogManager->showErrorDialog(tr("Disk is busy, cannot eject now"), QString());
            }
//            m_mounts.removeOne(mount);
            break;
        }
    }
}

//卸载外部设备
void AlbumView::onUnMountSignal(QString unMountPath)
{
//    QMap<QString, MountLoader *>::iterator itmount;
//    itmount = m_mountLoaderList.find(unMountPath);
//    if (itmount != m_mountLoaderList.end()) {
//        if (itmount.value()->isRunning()) {
//            itmount.value()->stopRunning(unMountPath);
//            return;
//        }
//    }
    m_pRightPhoneThumbnailList->stopLoadAndClear();
    QThread::sleep(1);
    needUnMount(unMountPath);
    qDebug() << "111";
}



void AlbumView::onLeftListDropEvent(QModelIndex dropIndex)
{
    qDebug() << "AlbumView::onLeftListDropEvent()";
    ThumbnailListView *currentViewList;
    QStringList dropItemPaths;

    AlbumLeftTabItem *item = (AlbumLeftTabItem *)m_pLeftListView->m_pCustomizeListView->itemWidget(m_pLeftListView->m_pCustomizeListView->item(dropIndex.row()));
    QString dropLeftTabListName = item->m_albumNameStr;
    qDebug() << "currentAlbum: " << m_currentAlbum << " ;dropLeftTabListName: " << dropLeftTabListName;

    //向自己的相册或“已导入”相册拖拽无效
    //“已导入”相册在leftlistwidget.cpp中也屏蔽过
    if ((m_currentAlbum == dropLeftTabListName) /*|| (COMMON_STR_RECENT_IMPORTED == dropLeftTabListName)*/ || 5 == m_pRightStackWidget->currentIndex()) {
        qDebug() << "Can not drop!";
        return;
    }

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

    if (COMMON_STR_TRASH == dropLeftTabListName) {
        //向回收站拖拽，动作删除
        //回收站在leftlistwidget.cpp中屏蔽掉了
    } else {
        //向其他相册拖拽，动作添加
        DBManager::instance()->insertIntoAlbum(item->m_albumNameStr, dropItemPaths);
        m_pLeftListView->m_pCustomizeListView->setCurrentRow(dropIndex.row());
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

//void MountLoader::onLoadMountImagesStart(QString mountName, QString path)
//{
//    qDebug() << "onLoadMountImagesStart() mountName: " << mountName;
//    qDebug() << "onLoadMountImagesStart() path: " << path;
//    QString strPath = path;
//    bIsRunning = true;
//    //判断路径是否存在
//    QDir dir(path);
//    if (!dir.exists()) {
//        qDebug() << "onLoadMountImagesStart() !dir.exists()";
//        dApp->signalM->sigLoadMountImagesEnd(mountName);
//        return;
//    }

//    //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
//    if (!path.contains("/media/")) {
//        bool bFind = findPicturePathByPhone(path);
//        if (!bFind) {
//            qDebug() << "onLoadMountImagesStart() !bFind";
//            dApp->signalM->sigLoadMountImagesEnd(mountName);
//            return;
//        }
//    }

//    //获取所选文件类型过滤器
//    QStringList filters;
//    filters << QString("*.jpeg") << QString("*.jpg")
//            << QString("*.bmp") << QString("*.png")
//            << QString("*.gif")
//            << QString("*.JPEG") << QString("*.JPG")
//            << QString("*.BMP") << QString("*.PNG")
//            << QString("*.GIF")
//            ;

//    //定义迭代器并设置过滤器
//    QDirIterator dir_iterator(path,
//                              filters,
//                              QDir::Files | QDir::NoSymLinks,
//                              QDirIterator::Subdirectories);

//    m_phoneImgPathList.clear();
//    qtpool.setMaxThreadCount(10);
//    qDebug() << "onLoadMountImagesStart() while (dir_iterator.hasNext())";
//    int i = 0;
//    while (dir_iterator.hasNext()) {
//        if (!bIsRunning) {
//            qtpool.waitForDone();
//            break;
//        }
//        i++;
//        dir_iterator.next();
//        QFileInfo fileInfo = dir_iterator.fileInfo();

//        ThreadRenderImage *randerimage = new ThreadRenderImage;
//        randerimage->setData(fileInfo, path, &m_phonePathImage, &m_phoneImgPathList);
//        qtpool.start(randerimage);
////        QThreadPool::globalInstance()->start(randerimage);
////        QImage tImg;

////        QString format = DetectImageFormat(fileInfo.filePath());
////        if (format.isEmpty()) {
////            QImageReader reader(fileInfo.filePath());
////            reader.setAutoTransform(true);
////            if (reader.canRead()) {
////                tImg = reader.read();
////            } else if (path.contains(".tga")) {
////                bool ret = false;
////                tImg = utils::image::loadTga(path, ret);
////            }
////        } else {
////            QImageReader readerF(fileInfo.filePath(), format.toLatin1());
////            readerF.setAutoTransform(true);
////            if (readerF.canRead()) {
////                tImg = readerF.read();
////            } else {
////                qWarning() << "can't read image:" << readerF.errorString()
////                           << format;

////                tImg = QImage(fileInfo.filePath());
////            }
////        }

////        QPixmap pixmap = QPixmap::fromImage(tImg);
////        if (pixmap.isNull()) {
////            qDebug() << "pixmap.isNull()";
////            continue;
////        }

////        pixmap = pixmap.scaledToHeight(100,  Qt::FastTransformation);
////        if (pixmap.isNull()) {
////            pixmap = QPixmap::fromImage(tImg);
////        }

////        m_phonePathImage.insert(fileInfo.filePath(), pixmap);

////        m_phoneImgPathList << fileInfo.filePath();

////        if (0 == m_phoneImgPathList.length() % 50) {
//        if (i >= 50) {
//            qtpool.waitForDone();
////            QThreadPool::globalInstance()->waitForDone();
//            i = 0;
//            m_parent->m_phonePathAndImage = m_phonePathImage;
//            m_parent->m_phoneNameAndPathlist.insert(strPath, m_phoneImgPathList);
//            dApp->signalM->sigLoadMountImagesEnd(mountName);
//        }
////        }
//    }

//    qtpool.waitForDone();
////    QThreadPool::globalInstance()->waitForDone();
//    qDebug() << "onLoadMountImagesStart() m_phoneImgPathList.length()" << m_phoneImgPathList.length();
//    if (0 < m_phoneImgPathList.length()) {
//        m_parent->m_phonePathAndImage = m_phonePathImage;
//        m_parent->m_phoneNameAndPathlist.insert(strPath, m_phoneImgPathList);
//        qDebug() << "onLoadMountImagesStart() strPath:" << strPath;
//    }

//    dApp->signalM->sigLoadMountImagesEnd(mountName);
//    if (bneedunmountpath) {
//        emit needUnMount(m_unmountpath);
//    }

//    bIsRunning = false;
//}

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
//    if (nullptr != m_FavoriteItem) {
//        m_FavoriteItem->setSizeHint(QSize(this->width() - 200, m_pRightFavoriteThumbnailList->getListViewHeight() + 8 + 27));
//    }
//    if (nullptr != m_TrashitemItem) {
//        m_TrashitemItem->setSizeHint(QSize(this->width() - 200, m_pRightTrashThumbnailList->getListViewHeight() + 8 + 27));
//    }
//    if (nullptr != m_noTrashItem) {
//        m_noTrashItem->setSizeHint(QSize(this->width() - 200, m_pRightThumbnailList->getListViewHeight() + 8 + 27));
//    }
    if (nullptr != pPhoneWidget) {
        m_pRightPhoneThumbnailList->setFixedSize(pPhoneWidget->size());
        phonetopwidget->setFixedWidth(pPhoneWidget->size().width());
    }

}

void AlbumView::importDialog()
{
    //导入取消窗口
    m_closeDeviceScan->setDisabled(true);
    m_ignoreDeviceScan->setDisabled(true);
    m_waitDeviceScandailog->show();
    m_waitDailog_timer->start(2000);
    this->setDisabled(true);

//    if (!m_waitDeviceScanMessage) {
//        m_waitDeviceScanMessage = new DMessageBox(this);
//    }
//    m_waitDeviceScanMessage->setWindowFlag(Qt::WindowTitleHint);
//    m_waitDeviceScanMessage->setFixedSize(QSize(422, 183));
//    m_waitDeviceScanMessage->move(769, 414);
//    m_waitDeviceScanMessage->setText(tr("loading images，please wait..."));
//    m_ignoreDeviceScan->setDisabled(true);
//    m_waitDeviceScanMessage->show();
//    m_waitDailog_timer->start(2000);
}

void AlbumView::onWaitDialogClose()
{
    QThread::msleep(100);
//    ImageEngineImportObject::clearAndStopThread();
//    ImageMountGetPathsObject::clearAndStopThread();
//    ImageMountImportPathsObject::clearAndStopThread();
//    m_phonePathAndImage.clear();
//    m_phonePicMap.clear();
//    m_mountPicNum = 0;
    m_currentAlbum = COMMON_STR_RECENT_IMPORTED;
//    //m_currentType = ALBUM_PATHTYPE_BY_PHONE;

    m_pLeftListView->m_pPhotoLibListView->setCurrentRow(0);
    m_currentType = COMMON_STR_RECENT_IMPORTED;
//    updateRightView();
    m_pRightPhoneThumbnailList->stopLoadAndClear();
//    for (auto mount : m_mounts) {
//        emit m_vfsManager->mountRemoved(mount);
//    }
    m_waitDeviceScandailog->close();
    this->setEnabled(true);
}

void AlbumView::onWaitDialogIgnore()
{
    m_waitDeviceScandailog->hide();
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
        m_noTrashItem->setSizeHint(QSize(this->width() - 200, m_pRightThumbnailList->getListViewHeight() + 8 + 27));
    }
    if (nullptr != m_FavoriteItem) {
        m_FavoriteItem->setSizeHint(QSize(this->width() - 200, m_pRightFavoriteThumbnailList->getListViewHeight() + 8 + 27));
    }
    if (nullptr != m_FavoriteItem) {
        m_TrashitemItem->setSizeHint(QSize(this->width() - 200, m_pRightTrashThumbnailList->getListViewHeight() + 8 + 27));
    }
    if (nullptr != m_pNoTrashTitle) {
        m_pNoTrashTitle->setFixedSize(this->width() - 200, 83);
    }
    if (nullptr != m_FavoriteTitle) {
        m_FavoriteTitle->setFixedSize(this->width() - 200, 83);
    }
    if (nullptr != m_TrashTitle) {
        m_TrashTitle->setFixedSize(this->width() - 200, 83);
    }
    if (nullptr != pPhoneWidget) {
        m_pRightPhoneThumbnailList->setFixedSize(pPhoneWidget->size());
        phonetopwidget->setFixedWidth(pPhoneWidget->size().width());
    }
//    m_pStatusBar->move(this->width() / 4, this->height() - 27 - 81);

    //add end 3975
    m_pStatusBar->setFixedWidth(this->width());
    m_pStatusBar->move(0, this->height() - m_pStatusBar->height());
    fatherwidget->setFixedSize(this->size());
    QWidget::resizeEvent(e);
}
