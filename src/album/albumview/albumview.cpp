// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
#include "ac-desktop-define.h"
#include "mainwindow.h"
#include "leftlistview.h"
#include "ac-desktop-define.h"
#include "batchoperatewidget.h"
#include "noresultwidget.h"
#include "imagedataservice.h"

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
const int MAINWINDOW_NEEDCUT_WIDTH = 775;
const int MAINWINDOW_DEVICE_MIN_WIDTH = 716;
const int PHONE_TITLE_NORMAL_WIDTH = 185;
const int PHONE_TITLE_MAX_WIDTH = 525;
const int LISTVIEW_MINMUN_WIDTH = 520;
const int PHONEPICTOTAL_FIX_HEIGHT = 23;

const int MAINWINDOW_MIN_WIDTH = 835;
const int BTN_FIX_HEIGHT = 36;
const int IMPORTBYPHONE_MIN_WIDTH = 58;
const int IMPORTBYPHONE_MAX_WIDTH = 213;
const int IMPORTALLBYPHONE_MIN_WIDTH = 94;
const int IMPORTALLBYPHONE_MAX_WIDTH = 100;
const int IMPORTSELBYPHONE_MIN_WIDTH = 53;
const int IMPORTSELBYPHONE_MAX_WIDTH = 100;

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

AlbumView::AlbumView()
    : m_iAlubmPicsNum(0), m_currentAlbum(COMMON_STR_RECENT_IMPORTED), m_currentUID(-1)
    , m_currentType(COMMON_STR_RECENT_IMPORTED), m_selPicNum(0), m_itemClicked(false)
    , m_pRightStackWidget(nullptr), m_pLeftListView(nullptr), m_pStatusBar(nullptr)
    , m_pRightWidget(nullptr), m_pRightPhoneThumbnailList(nullptr), m_pwidget(nullptr)
    , m_pRightTrashThumbnailList(nullptr)
    , pImportTimeLineWidget(nullptr), m_waitDeviceScandialog(nullptr)
    , m_pImportView(nullptr),  m_pImpTimeLineView(nullptr)
    , m_pFavoriteTitle(nullptr), m_pFavoritePicTotal(nullptr), m_pPhoneTitle(nullptr)
    , m_pPhonePicTotal(nullptr), m_pSearchView(nullptr), m_vfsManager(nullptr)
    , m_diskManager(nullptr)
    , m_importByPhoneWidget(nullptr), m_importByPhoneComboBox(nullptr)
    , m_importAllByPhoneBtn(nullptr), m_importSelectByPhoneBtn(nullptr), m_mountPicNum(0)
    , pPhoneWidget(nullptr), phonetopwidget(nullptr)
    , isIgnore(true), m_waitDailog_timer(nullptr)
{
    m_iAlubmPicsNum = DBManager::instance()->getImgsCount();
    m_vfsManager = new DGioVolumeManager;
    m_diskManager = new DDiskManager(this);
    m_diskManager->setWatchChanges(true);

    iniWaitDiolag();
    setAcceptDrops(true);

    initLeftView();
    initRightView();

    DWidget *leftwidget = new DWidget;
    leftwidget->setFixedWidth(180);

    QVBoxLayout *pvLayout = new QVBoxLayout();
    pvLayout->setContentsMargins(0, 0, 0, 0);
    leftwidget->setLayout(pvLayout);
    pvLayout->addWidget(m_pLeftListView);

    QHBoxLayout *pLayout = new QHBoxLayout();
    pLayout->setContentsMargins(0, 0, 0, 0);
    pLayout->setSpacing(0);
    pLayout->addWidget(leftwidget);
    pLayout->addWidget(m_pRightWidget);
    this->setLayout(pLayout);

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
    m_customThumbnailList->stopLoadAndClear();
    m_pRightPhoneThumbnailList->stopLoadAndClear();
    m_pRightTrashThumbnailList->stopLoadAndClear();
    m_favoriteThumbnailList->stopLoadAndClear();

    ImageEngineImportObject::clearAndStopThread();
    ImageMountGetPathsObject::clearAndStopThread();
    ImageMountImportPathsObject::clearAndStopThread();
    m_pImpTimeLineView->getFatherStatusBar(nullptr);
//    if (m_vfsManager) {
//        delete  m_vfsManager;
//        m_vfsManager = nullptr;
    //    }
}

bool AlbumView::imageImported(bool success)
{
    Q_UNUSED(success);
    emit dApp->signalM->closeWaitDialog();
    return true;
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
    // 添加重复照片提示
    connect(dApp->signalM, &SignalManager::RepeatImportingTheSamePhotos, this, &AlbumView::onRepeatImportingTheSamePhotos);
    connect(m_pLeftListView, &LeftListView::itemClicked, this, &AlbumView::leftTabClicked);
    connect(m_pLeftListView, &LeftListView::sigLeftTabClicked, this, &AlbumView::leftTabClicked);
    connect(m_pLeftListView, &LeftListView::updateCurrentItemType, this, &AlbumView::setCurrentItemType);
    connect(dApp->signalM, &SignalManager::sigCreateNewAlbumFromDialog, this, &AlbumView::onCreateNewAlbumFromDialog);
#if 1
    connect(dApp->signalM, &SignalManager::sigCreateNewAlbumFrom, this, &AlbumView::onCreateNewAlbumFrom);
    connect(m_customThumbnailList, &ThumbnailListView::sigMouseMove, this, &AlbumView::ongMouseMove);
    connect(m_favoriteThumbnailList, &ThumbnailListView::sigMouseMove, this, &AlbumView::ongMouseMove);
    // 解决最近删除页面ctrl+all选择所有图片，恢复按钮为灰色的问题
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::sigSelectAll, this, &AlbumView::onSelectAll);
#endif
    connect(dApp->signalM, &SignalManager::sigLoadOnePhoto, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::imagesInserted, this, &AlbumView::updateRightView);
    //todo
    connect(dApp->signalM, &SignalManager::imagesRemoved, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::insertedIntoAlbum, this, &AlbumView::onInsertedIntoAlbum, Qt::QueuedConnection);
    connect(dApp->signalM, &SignalManager::removedFromAlbum, this, &AlbumView::updateAlbumView);
    //最近删除中数据有变化，刷新缓存
    connect(dApp->signalM, &SignalManager::imagesTrashInserted, this, &AlbumView::onTrashInfosChanged);
    connect(dApp->signalM, &SignalManager::imagesTrashRemoved, this, &AlbumView::onTrashInfosChanged);
    connect(m_customThumbnailList, &ThumbnailListView::openImage, this, &AlbumView::onOpenImageCustom);
    connect(m_favoriteThumbnailList, &ThumbnailListView::openImage, this, &AlbumView::onOpenImageFav);
    connect(m_pLeftListView, &LeftListView::sigSlideShow, this, &AlbumView::onSlideShowCustom);
    connect(m_customThumbnailList, &ThumbnailListView::sigSlideShow, this, &AlbumView::onSlideShowCustom);
    connect(m_favoriteThumbnailList, &ThumbnailListView::sigSlideShow, this, &AlbumView::onSlideShowFav);
    connect(dApp->signalM, &SignalManager::sigUpdataAlbumRightTitle, this, &AlbumView::onUpdataAlbumRightTitle);
    connect(dApp->signalM, &SignalManager::sigUpdateImageLoader, this, &AlbumView::updateRightView);
    connect(dApp->signalM, &SignalManager::sigUpdateTrashImageLoader, this, &AlbumView::updateRightView);
#ifndef tablet_PC
    connect(m_vfsManager, &DGioVolumeManager::mountAdded, this, &AlbumView::onVfsMountChangedAdd);
    connect(m_vfsManager, &DGioVolumeManager::mountRemoved, this, &AlbumView::onVfsMountChangedRemove);
    connect(m_vfsManager, &DGioVolumeManager::volumeAdded, [](QExplicitlySharedDataPointer<DGioVolume> vol) {
        if (vol->volumeMonitorName().contains(QRegularExpression("(MTP|GPhoto2|Afc)$"))) {
            vol->mount();
        }
    });
#endif
    connect(m_diskManager, &DDiskManager::fileSystemAdded, this, &AlbumView::onFileSystemAdded);
    connect(m_diskManager, &DDiskManager::blockDeviceAdded, this, &AlbumView::onBlockDeviceAdded);
    connect(m_importAllByPhoneBtn, &DPushButton::clicked, this, &AlbumView::importAllBtnClicked);
    connect(m_importSelectByPhoneBtn, &DPushButton::clicked, this, &AlbumView::importSelectBtnClicked);
    connect(m_pStatusBar->m_pSlider, &DSlider::valueChanged, dApp->signalM, &SignalManager::emitSliderValueChg);
    connect(m_pLeftListView->m_pCustomizeListView, &LeftListWidget::signalDropEvent, this, &AlbumView::onLeftListDropEvent);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &AlbumView::onThemeTypeChanged);
#if 1
    connect(m_pRightPhoneThumbnailList, &ThumbnailListView::customContextMenuRequested, this, &AlbumView::onRightPhoneCustomContextMenuRequested);
    connect(m_pRightPhoneThumbnailList, &ThumbnailListView::sigMouseRelease, this, &AlbumView::onRightPhoneThumbnailListMouseRelease);
    connect(m_pImpTimeLineView, &ImportTimeLineView::sigUpdatePicNum, this, &AlbumView::updatePicNum);
    //筛选显示，当先列表中内容为无结果
    connect(m_pImpTimeLineView, &ImportTimeLineView::sigNoPicOrNoVideo, this, &AlbumView::slotNoPicOrNoVideo);
    //缩略图选中项发生变化
    //已导入
    connect(m_pImpTimeLineView->getListView()->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AlbumView::sltSelectionChanged);
    //自定义相册
    connect(m_customThumbnailList->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AlbumView::sltSelectionChanged);
    //我的收藏
    connect(m_favoriteThumbnailList->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AlbumView::sltSelectionChanged);
    //设备
    connect(m_pRightPhoneThumbnailList->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AlbumView::sltSelectionChanged);
    //最近删除
    connect(m_pRightTrashThumbnailList->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &AlbumView::sltSelectionChanged);
#endif
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::trashRecovery, this, &AlbumView::onTrashRecoveryBtnClicked);
    connect(m_pImportView->m_pImportBtn, &DPushButton::clicked, this, &AlbumView::onImportViewImportBtnClicked);
    connect(dApp->signalM, &SignalManager::sigImportFailedToView, this, &AlbumView::onImportFailedToView);
    connect(m_importByPhoneComboBox, &DComboBox::currentTextChanged, this, &AlbumView::importComboBoxChange);
    connect(dApp->signalM, &SignalManager::sigShortcutKeyDelete, this, &AlbumView::onKeyDelete);
    connect(dApp->signalM, &SignalManager::sigShortcutKeyF2, this, &AlbumView::onKeyF2);
    //2020年03月26日15:12:23
    connect(dApp->signalM, &SignalManager::waitDevicescan, this, &AlbumView::importDialog);
    connect(m_waitDailog_timer, &QTimer::timeout, this, &AlbumView::onWaitDailogTimeout);
    //在外部绑定内部按钮事件
    connect(m_waitDeviceScandialog->m_closeDeviceScan, &DPushButton::clicked, this, &AlbumView::onWaitDialogClose);
    connect(m_waitDeviceScandialog->m_ignoreDeviceScan, &DPushButton::clicked, this, &AlbumView::onWaitDialogIgnore);
    connect(m_pLeftListView->m_pMountListWidget, &DListWidget::clicked, this, &AlbumView::onLeftListViewMountListWidgetClicked);
    connect(m_waitDeviceScandialog, &Waitdevicedialog::closed, this, &AlbumView::onWaitDialogClose);
    connect(ImageEngineApi::instance(), &ImageEngineApi::sigMountFileListLoadReady, this, &AlbumView::sltLoadMountFileList, Qt::QueuedConnection);
    //路径监控相关
    connect(dApp->signalM, &SignalManager::sigMonitorDestroyed, this, &AlbumView::onMonitorDestroyed);

    // 字体改变时,不同尺寸下同步调整标题栏区域控件显示大小
    connect(qApp, &QGuiApplication::fontChanged, this, &AlbumView::adjustTitleContent);
}

void AlbumView::initLeftView()
{
    m_pLeftListView = new LeftListView(this);
    m_pLeftListView->setFocusPolicy(Qt::NoFocus);
    m_pLeftListView->m_pPhotoLibListView->setCurrentRow(0);

    //init externalDevice
#ifndef tablet_PC
    m_mounts = getVfsMountList();
    initExternalDevice();
    updateDeviceLeftList();
#endif
}

void AlbumView::onCreateNewAlbumFromDialog(const QString &newalbumname, int UID)
{
    //需要对UID进行判断，如果是自动导入的UID，需要和其它自动导入的项挨着
    int index = -1;
    if (DBManager::instance()->getAlbumDBTypeFromUID(UID) == AutoImport) {
        for (int i = 0; i < m_pLeftListView->m_pCustomizeListView->count(); i++) { //尝试搜索第一个非AutoImport的位置
            auto item = m_pLeftListView->m_pCustomizeListView->item(i);
            auto pTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pCustomizeListView->itemWidget(item));
            if (pTabItem) {
                if (DBManager::instance()->getAlbumDBTypeFromUID(pTabItem->m_UID) != AutoImport) {
                    index = i; //搜索到了以后，让它插入到这个位置
                    break;
                }
            }
        }
        //如果没有搜索到，则表示没有非AutoImport的项，此时直接插入到最后
        if (index == -1) {
            index = m_pLeftListView->m_pCustomizeListView->count();
        }
    } else { //如果不是AutoImport，直接插入到末尾
        index = m_pLeftListView->m_pCustomizeListView->count();
    }

    //这一段是在检查是否已经存在相同的UID
    //现有逻辑会触发进来两次，需要进行检查
    bool isExsit = false;
    for (int i = 0; i < m_pLeftListView->m_pCustomizeListView->count(); i++) {
        QListWidgetItem *item = m_pLeftListView->m_pCustomizeListView->item(i);
        AlbumLeftTabItem *pTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pCustomizeListView->itemWidget(item));
        if (pTabItem) {
            if (UID == pTabItem->m_UID) {
                isExsit = true;
                m_pLeftListView->m_pCustomizeListView->setCurrentRow(i);
                break;
            }
        }
    }

    //如果UID不存在，则执行新建操作
    if (!isExsit) {
        //创建item的时候第一个项不能指定widget，需要是nullptr才能使下面的insertItem生效
        QListWidgetItem *pListWidgetItem = new QListWidgetItem(nullptr, ablumType);//hj add data to listwidgetitem to Distinguish item's type
        m_pLeftListView->m_pCustomizeListView->insertItem(index, pListWidgetItem);
        pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
        AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(newalbumname, UID, COMMON_STR_CREATEALBUM);
        m_pLeftListView->m_pCustomizeListView->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
        m_pLeftListView->m_pCustomizeListView->setCurrentRow(index);
    }

    m_pLeftListView->onUpdateLeftListview();
    //清除其他已选中的项
    QModelIndex index2;
    emit m_pLeftListView->m_pCustomizeListView->pressed(index2);
}

void AlbumView::onCreateNewAlbumFrom(const QString &albumname, int UID)
{
    int index = m_pLeftListView->m_pCustomizeListView->count();
    QListWidgetItem *pListWidgetItem = new QListWidgetItem();
    pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
    AlbumLeftTabItem *pAlbumLeftTabItem = new AlbumLeftTabItem(albumname, UID);
    m_pLeftListView->m_pCustomizeListView->insertItem(index, pListWidgetItem);
    m_pLeftListView->m_pCustomizeListView->setItemWidget(pListWidgetItem, pAlbumLeftTabItem);
    m_pLeftListView->onUpdateLeftListview();
}

void AlbumView::iniWaitDiolag()
{
    m_waitDeviceScandialog = new Waitdevicedialog(this);
    m_waitDailog_timer = new QTimer(this);
    m_waitDeviceScandialog->m_closeDeviceScan = new DPushButton(tr("Cancel"));
    m_waitDeviceScandialog->m_ignoreDeviceScan = new DPushButton(tr("Ignore"));
    m_waitDeviceScandialog->waitTips = new DLabel(tr("Loading..."));
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

    //初始化自定义相册列表
    initCustomAlbumWidget();
    //初始化最近删除列表
    initTrashWidget();
    //初始化收藏列表
    initFavoriteWidget();
    //Search View
    m_pSearchView = new SearchView;
    //初始化设备列表
    initPhoneWidget();

//add start 3975
    m_pStatusBar = new StatusBar(this);
    m_pStatusBar->raise();
    m_pStatusBar->setFixedWidth(this->width() - m_pLeftListView->width());
    m_pStatusBar->move(m_pLeftListView->width(), this->height() - m_pStatusBar->height());

    pImportTimeLineWidget = new DWidget();
    pImportTimeLineWidget->setBackgroundRole(DPalette::Window);
    QVBoxLayout *importLineLayout = new QVBoxLayout(pImportTimeLineWidget);
    importLineLayout->setContentsMargins(0, 0, 0, 0);
    pImportTimeLineWidget->setLayout(importLineLayout);

    m_pImpTimeLineView = new ImportTimeLineView(m_pRightStackWidget);
    m_pImpTimeLineView->setContentsMargins(0, 0, 0, 0);
    m_pImpTimeLineView->getFatherStatusBar(m_pStatusBar->m_pSlider);
    importLineLayout->addWidget(m_pImpTimeLineView);
//add end 3975
// Add View
    m_pRightStackWidget->addWidget(m_pImportView);       // 空白导入界面
    m_pRightStackWidget->addWidget(m_pCustomAlbumWidget);    // 自定义相册界面
    m_pRightStackWidget->addWidget(m_pTrashWidget);      // 最近删除
    m_pRightStackWidget->addWidget(m_pFavoriteWidget);   // 我的收藏
    m_pRightStackWidget->addWidget(m_pSearchView);       // 搜索界面页面
    m_pRightStackWidget->addWidget(pPhoneWidget);        // 设备界面
    m_pRightStackWidget->addWidget(pImportTimeLineWidget); // 已导入界面

// Statusbar
    QVBoxLayout *pVBoxLayout = new QVBoxLayout();
    pVBoxLayout->setContentsMargins(0, 0, 0, m_pStatusBar->height());
    pVBoxLayout->addWidget(m_pRightStackWidget);
    m_pRightWidget->setLayout(pVBoxLayout);

    if (0 < DBManager::instance()->getImgsCount()) {
        m_customThumbnailList->setFrameShape(DTableView::NoFrame);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TIMELINE_IMPORT);
        m_pStatusBar->show();
    } else {
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
        m_pStatusBar->setVisible(false); //隐藏底部栏
    }
}
//初始化最近删除列表
void AlbumView::initTrashWidget()
{
    //最近删除页
    m_pTrashWidget = new DWidget();
    DPalette palcolor3 = DApplicationHelper::instance()->palette(m_pTrashWidget);
    palcolor3.setBrush(DPalette::Base, palcolor3.color(DPalette::Window));
    m_pTrashWidget->setPalette(palcolor3);
    QVBoxLayout *p_Trash = new QVBoxLayout();
    //左侧距离分界线20px，8+缩略图spacing+缩略图边框
    p_Trash->setContentsMargins(8, 0, 0, 0);
    m_pTrashWidget->setLayout(p_Trash);
    //最近删除列表
    m_pRightTrashThumbnailList = new ThumbnailListView(ThumbnailDelegate::AlbumViewTrashType, -1, COMMON_STR_TRASH);
    m_pRightTrashThumbnailList->setFrameShape(DTableView::NoFrame);
    m_pRightTrashThumbnailList->setObjectName("RightTrashThumbnail");
    m_pRightTrashThumbnailList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_pRightTrashThumbnailList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_pRightTrashThumbnailList->setContentsMargins(0, 0, 0, 0);
    p_Trash->addWidget(m_pRightTrashThumbnailList);

    //初始化筛选无结果窗口
    m_trashNoResultWidget = new NoResultWidget(this);
    p_Trash->addWidget(m_trashNoResultWidget);
    m_trashNoResultWidget->setVisible(false);

    //筛选显示，当先列表中内容为无结果
    connect(m_pRightTrashThumbnailList, &ThumbnailListView::sigNoPicOrNoVideo, this, &AlbumView::slotNoPicOrNoVideo);
    //重新更改了最近删除的顶部布局   2020-4-17 xiaolong
    QHBoxLayout *Layout1 = new QHBoxLayout();
    Layout1->setContentsMargins(20, 0, 19, 0);

    m_TrashDescritionLab = new DLabel();
    DFontSizeManager::instance()->bind(m_TrashDescritionLab, DFontSizeManager::T6, QFont::Medium);
    QPalette pal = DApplicationHelper::instance()->palette(m_TrashDescritionLab);
    QColor color_BT = pal.color(DPalette::BrightText);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
    }
    m_TrashDescritionLab->setForegroundRole(DPalette::Text);
    m_TrashDescritionLab->setPalette(pal);
    m_trashNoticeFullStr = tr("The files will be permanently deleted after the days shown on them");
    m_TrashDescritionLab->setText(m_trashNoticeFullStr);
    Layout1->addWidget(m_TrashDescritionLab);

    m_trashBatchOperateWidget = new BatchOperateWidget(m_pRightTrashThumbnailList, BatchOperateWidget::AlbumViewTrashType, m_pTrashWidget);
    Layout1->addStretch(100);
//    hlayoutDateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//    hlayoutDateLabel->setContentsMargins(0, 0, 19, 0);
    Layout1->addWidget(m_trashBatchOperateWidget);
    connect(m_trashBatchOperateWidget, &BatchOperateWidget::signalBatchSelectChanged, this, &AlbumView::onBatchSelectChanged, Qt::QueuedConnection);
    connect(m_trashBatchOperateWidget, &BatchOperateWidget::sigCancelAll, this, [ = ](bool cancel) {
        if (QLocale::system().language() == QLocale::Uigur) {
        } else {
            onBatchSelectChanged(cancel);
        }
    }, Qt::QueuedConnection);

    //最近删除标题,不使用布局
    m_TrashTitleLab = new DLabel(m_pTrashWidget);
    m_TrashTitleLab->setText(tr("Trash"));
    DFontSizeManager::instance()->bind(m_TrashTitleLab, DFontSizeManager::T3, QFont::DemiBold);
    m_TrashTitleLab->setFixedHeight(trash_title_height);
    m_TrashTitleLab->setForegroundRole(DPalette::TextTitle);
    m_TrashTitleLab->adjustSize();

    m_TrashTitleWidget = new DWidget(m_pTrashWidget);
    m_TrashTitleWidget->setLayout(Layout1);
    DPalette ppal_light3 = DApplicationHelper::instance()->palette(m_TrashTitleWidget);
    ppal_light3.setBrush(DPalette::Background, ppal_light3.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light3 = new QGraphicsOpacityEffect;
    opacityEffect_light3->setOpacity(0.95);
    m_TrashTitleWidget->setPalette(ppal_light3);
    m_TrashTitleWidget->setGraphicsEffect(opacityEffect_light3);
    m_TrashTitleWidget->setAutoFillBackground(true);
    m_TrashTitleWidget->move(0, 0);
    m_TrashTitleWidget->setFixedSize(this->width() - LEFT_VIEW_WIDTH, trash_title_height);
}
//初始化自定义相册列表
void AlbumView::initCustomAlbumWidget()
{
    m_pCustomAlbumWidget = new DWidget(); //add 3975
    DPalette palcolor = DApplicationHelper::instance()->palette(m_pCustomAlbumWidget);
    palcolor.setBrush(DPalette::Base, palcolor.color(DPalette::Window));
    m_pCustomAlbumWidget->setPalette(palcolor);
    QVBoxLayout *p_all = new QVBoxLayout();
    p_all->setContentsMargins(8, 0, 0, 0);
    m_pCustomAlbumWidget->setLayout(p_all);

    //自定义相册标题的外层布局
    QHBoxLayout *pHLayout = new QHBoxLayout();
    pHLayout->setContentsMargins(20, 0, 19, 0);

    m_customAlbumTitleLabel = new DLabel(m_pCustomAlbumWidget);
    DFontSizeManager::instance()->bind(m_customAlbumTitleLabel, DFontSizeManager::T3, QFont::DemiBold);
    m_customAlbumTitleLabel->setForegroundRole(DPalette::TextTitle);
    m_customAlbumTitleLabel->setFixedHeight(custom_title_height);
    m_customAlbumTitleLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    m_customAlbumTitleLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);

    m_pRightPicTotal = new DLabel();
    m_pRightPicTotal->setFocusPolicy(Qt::NoFocus);
    m_pRightPicTotal->setFixedHeight(20);
    m_pRightPicTotal->setAttribute(Qt::WA_TransparentForMouseEvents, true);
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
    // 调整布局,适配维语
    pHLayout->addWidget(m_pRightPicTotal);
    pHLayout->addStretch();

    m_customThumbnailList = new ThumbnailListView(ThumbnailDelegate::AlbumViewCustomType, -1, COMMON_STR_CUSTOM);
    //筛选显示，当先列表中内容为无结果
    connect(m_customThumbnailList, &ThumbnailListView::sigNoPicOrNoVideo, this, &AlbumView::slotNoPicOrNoVideo);
    m_customThumbnailList->setFrameShape(DTableView::NoFrame);
    m_customThumbnailList->setViewportMargins(0, 0, 0, 0);
    m_customThumbnailList->setContentsMargins(0, 0, 0, 0);
    p_all->addWidget(m_customThumbnailList);
    //初始化筛选无结果窗口
    m_customNoResultWidget = new NoResultWidget(this);
    p_all->addWidget(m_customNoResultWidget);
    m_customNoResultWidget->setVisible(false);

    m_customBatchOperateWidget = new BatchOperateWidget(m_customThumbnailList, BatchOperateWidget::NullType, m_customAlbumTitle);
    pHLayout->addStretch(100);
//    hlayoutDateLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
//    hlayoutDateLabel->setContentsMargins(0, 0, 19, 0);
    pHLayout->addWidget(m_customBatchOperateWidget);
    connect(m_customBatchOperateWidget, &BatchOperateWidget::signalBatchSelectChanged, this, &AlbumView::onBatchSelectChanged, Qt::QueuedConnection);
    connect(m_customBatchOperateWidget, &BatchOperateWidget::sigCancelAll, this, [ = ](bool cancel) {
        if (QLocale::system().language() == QLocale::Uigur) {
        } else {
            onBatchSelectChanged(cancel);
        }
    }, Qt::QueuedConnection);

    m_customAlbumTitle = new DWidget(m_pCustomAlbumWidget);
    m_customAlbumTitle->setFocusPolicy(Qt::ClickFocus);
    m_customAlbumTitle->setLayout(pHLayout);

    DPalette ppal_light = DApplicationHelper::instance()->palette(m_customAlbumTitle);
    ppal_light.setBrush(DPalette::Background, ppal_light.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light = new QGraphicsOpacityEffect;
    opacityEffect_light->setOpacity(0.95);
    m_customAlbumTitle->setPalette(ppal_light);
    m_customAlbumTitle->setGraphicsEffect(opacityEffect_light);
    m_customAlbumTitle->setAutoFillBackground(true);
    m_customAlbumTitle->move(0, 0);
    m_customAlbumTitle->setFixedSize(this->width() - LEFT_VIEW_WIDTH, custom_title_height);
}
//初始化收藏列表
void AlbumView::initFavoriteWidget()
{
    //add end 3975
    // Favorite View
    m_pFavoriteWidget = new DWidget(); //add 3975
    m_pFavoriteWidget->setFocusPolicy(Qt::ClickFocus);
    //add start 3975
    DPalette palcolor2 = DApplicationHelper::instance()->palette(m_pFavoriteWidget);
    palcolor2.setBrush(DPalette::Base, palcolor2.color(DPalette::Window));
    m_pFavoriteWidget->setPalette(palcolor2);
    QVBoxLayout *p_Favorite = new QVBoxLayout();
    //左侧距离分界线20px，8+缩略图spacing+缩略图边框
    p_Favorite->setContentsMargins(8, 0, 0, 0);
    m_pFavoriteWidget->setLayout(p_Favorite);

    m_favoriteThumbnailList = new ThumbnailListView(ThumbnailDelegate::AlbumViewFavoriteType, DBManager::u_Favorite, COMMON_STR_FAVORITES);
    //筛选显示，当先列表中内容为无结果
    connect(m_favoriteThumbnailList, &ThumbnailListView::sigNoPicOrNoVideo, this, &AlbumView::slotNoPicOrNoVideo);
    m_favoriteThumbnailList->setFrameShape(DTableView::NoFrame);
    m_favoriteThumbnailList->setObjectName("RightFavoriteThumbnail");
    m_favoriteThumbnailList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_favoriteThumbnailList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_favoriteThumbnailList->setContentsMargins(0, 0, 0, 0);
    p_Favorite->addWidget(m_favoriteThumbnailList);
    //初始化筛选无结果窗口
    m_favoriteNoResultWidget = new NoResultWidget(this);
    p_Favorite->addWidget(m_favoriteNoResultWidget);
    m_favoriteNoResultWidget->setVisible(false);

    //我的收藏悬浮标题
    QHBoxLayout *lNumberLayout = new QHBoxLayout();
    lNumberLayout->setContentsMargins(20, 0, 19, 0);

    m_pFavoriteTitle = new DLabel(m_pFavoriteWidget);
    m_pFavoriteTitle->setFixedHeight(favorite_title_height);
    DFontSizeManager::instance()->bind(m_pFavoriteTitle, DFontSizeManager::T3, QFont::DemiBold);
    m_pFavoriteTitle->setForegroundRole(DPalette::TextTitle);
    m_pFavoriteTitle->setText(tr("Favorites"));
    m_pFavoriteTitle->setFixedWidth(150); //BUG#93470 手动调大控件长度

    m_pFavoritePicTotal = new DLabel();
    m_pFavoritePicTotal->setFixedHeight(20);
    DFontSizeManager::instance()->bind(m_pFavoritePicTotal, DFontSizeManager::T6, QFont::Medium);
    m_pFavoritePicTotal->setForegroundRole(DPalette::TextTips);

    QPalette pal = DApplicationHelper::instance()->palette(m_pFavoritePicTotal);
    QColor color_BT = pal.color(DPalette::BrightText);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
    }
    m_pFavoritePicTotal->setForegroundRole(DPalette::Text);
    m_pFavoritePicTotal->setPalette(pal);
    lNumberLayout->addWidget(m_pFavoritePicTotal);

    m_FavoriteTitleWidget = new DWidget(m_pFavoriteWidget);
    m_FavoriteTitleWidget->setFocusPolicy(Qt::NoFocus);
    m_FavoriteTitleWidget->setLayout(lNumberLayout);

    m_favoriteBatchOperateWidget = new BatchOperateWidget(m_favoriteThumbnailList, BatchOperateWidget::NullType, m_FavoriteTitleWidget);
    lNumberLayout->addStretch(100);
    lNumberLayout->addWidget(m_favoriteBatchOperateWidget, 0, Qt::AlignRight | Qt::AlignVCenter);
    connect(m_favoriteBatchOperateWidget, &BatchOperateWidget::signalBatchSelectChanged, this, &AlbumView::onBatchSelectChanged);
    connect(m_favoriteBatchOperateWidget, &BatchOperateWidget::sigCancelAll, this, &AlbumView::onBatchSelectChanged);

    DPalette ppal_light2 = DApplicationHelper::instance()->palette(m_FavoriteTitleWidget);
    ppal_light2.setBrush(DPalette::Background, ppal_light2.color(DPalette::Base));
    QGraphicsOpacityEffect *opacityEffect_light2 = new QGraphicsOpacityEffect;
    opacityEffect_light2->setOpacity(0.95);
    m_FavoriteTitleWidget->setPalette(ppal_light2);
    m_FavoriteTitleWidget->setGraphicsEffect(opacityEffect_light2);
    m_FavoriteTitleWidget->setAutoFillBackground(true);
    m_FavoriteTitleWidget->move(0, 0);
    m_FavoriteTitleWidget->setFixedSize(this->width() - m_pLeftListView->width() - 20, favorite_title_height);
    //add end 3975
}
//初始化设备列表
void AlbumView::initPhoneWidget()
{
    // Phone View
    pPhoneWidget = new DWidget();
    pPhoneWidget->setBackgroundRole(DPalette::Window);
    QHBoxLayout *thumbnailListLayout = new QHBoxLayout(pPhoneWidget);
    thumbnailListLayout->setContentsMargins(8, 0, 0, 0);

    m_pPhoneTitle = new DLabel();
    m_pPhoneTitle->setFixedHeight(36);
    if (QLocale::system().language() == QLocale::Tibetan) {
        m_pPhoneTitle->setFixedHeight(36 + 25);
    }
    DFontSizeManager::instance()->bind(m_pPhoneTitle, DFontSizeManager::T3, QFont::DemiBold);
    m_pPhoneTitle->setForegroundRole(DPalette::TextTitle);

    //给m_pPhonePicTotal布局,外接设备界面
    QHBoxLayout *PhonePicTotalLayout = new QHBoxLayout();
    PhonePicTotalLayout->setContentsMargins(0, 0, 0, 0);

    m_pPhonePicTotal = new DLabel();
    m_pPhonePicTotal->setFixedHeight(PHONEPICTOTAL_FIX_HEIGHT);
    if (QLocale::system().language() == QLocale::Tibetan) {
        m_pPhonePicTotal->setFixedHeight(PHONEPICTOTAL_FIX_HEIGHT + 10);
    }
    DFontSizeManager::instance()->bind(m_pPhonePicTotal, DFontSizeManager::T6, QFont::Medium);
    m_pPhonePicTotal->setForegroundRole(DPalette::TextTips);
    QPalette pal = DApplicationHelper::instance()->palette(m_pPhonePicTotal);
    QColor color_BT = pal.color(DPalette::BrightText);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    if (themeType == DGuiApplicationHelper::LightType) {
        color_BT.setAlphaF(0.5);
        pal.setBrush(DPalette::Text, color_BT);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        color_BT.setAlphaF(0.75);
        pal.setBrush(DPalette::Text, color_BT);
    }
    m_pPhonePicTotal->setForegroundRole(DPalette::Text);
    m_pPhonePicTotal->setPalette(pal);

    m_pRightPhoneThumbnailList = new ThumbnailListView(ThumbnailDelegate::AlbumViewPhoneType, -1, ALBUM_PATHTYPE_BY_PHONE);
    m_pRightPhoneThumbnailList->setListViewUseFor(ThumbnailListView::Mount);
    m_pRightPhoneThumbnailList->setFrameShape(DTableView::NoFrame);
    m_pRightPhoneThumbnailList->setViewportMargins(0, 0, 0, 0);
    thumbnailListLayout->addWidget(m_pRightPhoneThumbnailList);

    PhonePicTotalLayout->addWidget(m_pPhonePicTotal);
    PhonePicTotalLayout->addStretch();

    QVBoxLayout *pPhoneVBoxLayout = new QVBoxLayout();
    pPhoneVBoxLayout->setContentsMargins(20, 0, 0, 0);
    pPhoneVBoxLayout->addSpacing(4);
    pPhoneVBoxLayout->addWidget(m_pPhoneTitle);
    pPhoneVBoxLayout->addSpacing(22);
    pPhoneVBoxLayout->addLayout(PhonePicTotalLayout);
    pPhoneVBoxLayout->addSpacing(-1);


    //手机相片导入窗体
    m_importByPhoneWidget = new DWidget;
    QHBoxLayout *mainImportLayout = new QHBoxLayout;
    DLabel *importLabel = new DLabel();
    importLabel->setText(tr("Import to:"));
    DFontSizeManager::instance()->bind(importLabel, DFontSizeManager::T6, QFont::Medium);
    importLabel->setForegroundRole(DPalette::TextTitle);
    importLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_importByPhoneComboBox = new DComboBox;
    m_importByPhoneComboBox->setFixedSize(QSize(213, 36));
    m_importByPhoneComboBox->setEnabled(false);
    m_importAllByPhoneBtn = new DPushButton(tr("Import All"));
    DFontSizeManager::instance()->bind(m_importAllByPhoneBtn, DFontSizeManager::T6);
    m_importAllByPhoneBtn->setFixedSize(100, 36);
    DPalette importAllByPhoneBtnPa = DApplicationHelper::instance()->palette(m_importAllByPhoneBtn);
    importAllByPhoneBtnPa.setBrush(DPalette::Highlight, QColor(0, 0, 0, 0));
    m_importAllByPhoneBtn->setPalette(importAllByPhoneBtnPa);
    m_importAllByPhoneBtn->setEnabled(false);

    m_importSelectByPhoneBtn = new DSuggestButton(tr("Import"));
    DFontSizeManager::instance()->bind(m_importSelectByPhoneBtn, DFontSizeManager::T6);

    m_importSelectByPhoneBtn->setFixedSize(QSize(100, 36));
    m_importSelectByPhoneBtn->setEnabled(false);
    mainImportLayout->addWidget(importLabel);
    mainImportLayout->setSpacing(5);
    mainImportLayout->addWidget(m_importByPhoneComboBox);
    mainImportLayout->addWidget(m_importAllByPhoneBtn);
    mainImportLayout->addWidget(m_importSelectByPhoneBtn);
    m_importByPhoneWidget->setLayout(mainImportLayout);

    QHBoxLayout *allHLayout = new QHBoxLayout;
    allHLayout->setContentsMargins(0, 0, 0, 0);
    allHLayout->addLayout(pPhoneVBoxLayout, 1);
    allHLayout->addStretch();
    allHLayout->addWidget(m_importByPhoneWidget, 1);
    m_pRightPhoneThumbnailList->setParent(pPhoneWidget);
    phonetopwidget = new DWidget(pPhoneWidget);
    QGraphicsOpacityEffect *opacityEffect_lightphone = new QGraphicsOpacityEffect;
    opacityEffect_lightphone->setOpacity(0.95);
    phonetopwidget->setGraphicsEffect(opacityEffect_lightphone);
    phonetopwidget->setAutoFillBackground(true);
    phonetopwidget->setFixedSize(pPhoneWidget->width() - 15, 81);//BUG#93779 -15把右侧滚动条露出来
    phonetopwidget->setLayout(allHLayout);
    phonetopwidget->move(0, 0);
    phonetopwidget->raise();
}

void AlbumView::resetLabelCount(int photosCount, int videosCount, DLabel *lable)
{
    QString photosStr;
    if (photosCount == 1) {
        photosStr = StatusBar::tr("1 photo");
    } else if (photosCount > 1) {
        photosStr = StatusBar::tr("%n photos", "", photosCount);
    }

    QString videosStr;
    if (videosCount == 1) {
        videosStr = StatusBar::tr("1 video");
    } else if (videosCount > 1) {
        videosStr = StatusBar::tr("%n videos", "", videosCount);
    }

    if (photosCount > 0 && videosCount == 0) {
        lable->setText(photosStr);
    } else if (photosCount == 0 && videosCount > 0) {
        lable->setText(videosStr);
    } else if (photosCount > 0 && videosCount > 0) {
        lable->setText(photosStr + " " + videosStr);
    }

    // 记录我的收藏、自定义相册、设备视图总数的全量文本内容，以便做截断显示
    if (lable == m_pFavoritePicTotal)
        m_FavoritePicTotalFullStr = lable->text();
    else if (lable == m_pRightPicTotal)
        m_CustomRightPicTotalFullStr = lable->text();
    else if (lable == m_pPhonePicTotal)
        m_PhonePicTotalFullStr = lable->text();
}

void AlbumView::updateRightView()
{
    if (!m_customAlbumTitleLabel) {
        return;
    }
    m_customAlbumTitleLabel->setVisible(true);
    m_pRightPicTotal->setVisible(true);
    if (!m_spinner) {
        return;
    }
    m_spinner->hide();
    m_spinner->stop();
    if (COMMON_STR_RECENT_IMPORTED == m_currentType) {
        updateRightImportView();
    } else if (COMMON_STR_TRASH == m_currentType) {
        updateRightTrashView();
        setAcceptDrops(false);
        emit sigSearchEditIsDisplay(false);
    } else if (COMMON_STR_FAVORITES == m_currentType) {
        updateRightMyFavoriteView();
    } else if (COMMON_STR_CUSTOM == m_currentType) {
        updateRightCustomAlbumView();
    } else if (ALBUM_PATHTYPE_BY_PHONE == m_currentType) {
        m_itemClicked = true;
        updateRightMountView();
        setAcceptDrops(false);
        emit sigSearchEditIsDisplay(false);
    }
    updatePicNum();
    adjustTitleContent();
}

void AlbumView::updateAlbumView(int UID)
{
    if (UID == DBManager::SpUID::u_Favorite) { //注意不要把下面那个if提上来用&&连接
        if (m_currentUID == DBManager::SpUID::u_Favorite) { //只有当前的UID也是我的收藏时才执行刷新，否则会自动跳转过去
            updateRightMyFavoriteView();
        }
    } else {
        updateRightCustomAlbumView();
    }
}

void AlbumView::updateDeviceLeftList()
{
    bool tempB = false;
    m_mounts.count() > 0 ? tempB = true : tempB = false;
    // 有设备接入时，左边栏显示设备item
    m_pLeftListView->m_pMountListWidget->setVisible(tempB);
    m_pLeftListView->m_pMountWidget->setVisible(tempB);
    m_pLeftListView->m_pMountListWidget->setFixedHeight(m_pLeftListView->m_pMountListWidget->count() * 40);
}

void AlbumView::setCurrentItemType(int type)
{
    m_currentItemType = static_cast<AblumType>(type);
}

// 更新已导入列表
void AlbumView::updateRightImportView()
{
    m_iAlubmPicsNum = DBManager::instance()->getImgsCount();

    if (0 < m_iAlubmPicsNum) {
        m_pImpTimeLineView->clearAndStartLayout();
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TIMELINE_IMPORT);
        m_pStatusBar->setVisible(true);
    } else {
        m_pImpTimeLineView->clearAndStartLayout();
        m_pImportView->setAlbumname(QString());
        m_pImportView->setUID(-1);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
        m_pStatusBar->setVisible(false); //隐藏底部栏
    }
    emit sigSearchEditIsDisplay(true);
    setAcceptDrops(true);
}

// 更新个人收藏列表
void AlbumView::updateRightMyFavoriteView()
{
    using namespace utils::image;
    DBImgInfoList infos;
    infos = DBManager::instance()->getInfosByAlbum(m_currentUID, false);
    m_favoriteThumbnailList->clearAll();
    //插入上方空白项
    m_favoriteThumbnailList->insertBlankOrTitleItem(ItemTypeBlank, "", "", favorite_title_height);
    m_favoriteThumbnailList->insertThumbnailByImgInfos(infos);
    //重置数量显示
    resetLabelCount(m_favoriteThumbnailList->getAppointTypeItemCount(ItemTypePic)
                    , m_favoriteThumbnailList->getAppointTypeItemCount(ItemTypeVideo), m_pFavoritePicTotal);
    m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_FAVORITE_LIST);
    m_FavoriteTitleWidget->setVisible(infos.size() > 0);
    m_pStatusBar->setVisible(true);
    emit sigSearchEditIsDisplay(true);
    setAcceptDrops(false);
}

// 更新外接设备右侧视图
void AlbumView::updateRightMountView()
{
    if (!isVisible()) {
        return;
    }
    qDebug() << "------更新外接设备右侧视图-----";
    m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_PHONE);
    if (!m_pLeftListView) {
        return;
    }
    QString strPath;
    if (m_pLeftListView->m_pMountListWidget->currentItem()) {
        strPath = m_pLeftListView->m_pMountListWidget->currentItem()->data(Qt::UserRole).toString();
        if (!strPath.contains("/media/") && !strPath.contains("DCIM")) {
            findPicturePathByPhone(strPath);
        }
    }
    //先关闭之前循环的线程
    ImageEngineApi::instance()->setThreadShouldStop();
    //发送信号到子线程加载文件信息
    emit ImageEngineApi::instance()->sigLoadMountFileList(strPath);
    //TODO 保存每个挂载设备的加载状态
    if (mountLoadStatus.contains(strPath) && mountLoadStatus[strPath]) {
        return;
    }
    mountLoadStatus[strPath] = true;
    //显示等待加载窗口
    emit dApp->signalM->waitDevicescan();
}

// 更新新建相册列表
void AlbumView::updateRightCustomAlbumView()
{
    qDebug() << "---------" << __FUNCTION__ << "---";
    //bug78951 更新时需清空
    using namespace utils::image;
    DBImgInfoList infos;
    infos = DBManager::instance()->getInfosByAlbum(m_currentUID, false);
    //先清除所有
    m_customThumbnailList->clearAll();
    //添加上方空白栏
    m_customThumbnailList->insertBlankOrTitleItem(ItemTypeBlank, "", "", custom_title_height);
    //添加图片信息
    m_customThumbnailList->insertThumbnailByImgInfos(infos);
//    m_iAlubmPicsNum = DBManager::instance()->getImgsCountByAlbum(m_currentAlbum);
    m_iAlubmPicsNum = infos.size();
    if (0 < m_iAlubmPicsNum) {
        m_customAlbumTitleLabel->setText(m_currentAlbum);
        m_customAlbumTitleLabel->adjustSize();
        m_customAlbumTitleLabel->move(m_customAlbumTitle->width() / 2 - m_customAlbumTitleLabel->width() / 2, 0);
        //根据长度调整字符串
        QString Str = utils::base::reorganizationStr(m_customAlbumTitleLabel->font(), m_currentAlbum,
                                                     2 * m_customBatchOperateWidget->x() - m_customAlbumTitle->width());
        m_customAlbumTitleLabel->setText(Str);
        m_customAlbumTitleLabel->adjustSize();

        m_customAlbumTitleLabel->move(m_customAlbumTitle->width() / 2 - m_customAlbumTitleLabel->width() / 2, 0);
        m_customAlbumTitleLabel->raise();
        //重置数量显示
        resetLabelCount(m_customThumbnailList->getAppointTypeItemCount(ItemTypePic)
                        , m_customThumbnailList->getAppointTypeItemCount(ItemTypeVideo), m_pRightPicTotal);
        m_customThumbnailList->m_imageType = m_currentAlbum;
        m_customThumbnailList->m_currentUID = m_currentUID;
        m_customThumbnailList->stopLoadAndClear();
        //todo
//        m_pRightThumbnailList->loadFilesFromLocal(infos);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
        m_pStatusBar->show();
    } else {
        m_customThumbnailList->stopLoadAndClear();
        //todo
//        m_pRightThumbnailList->loadFilesFromLocal(infos);
        m_pImportView->setAlbumname(m_currentAlbum);
        m_pImportView->setUID(m_currentUID);
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_IMPORT);
        m_pStatusBar->setVisible(false); //隐藏底部栏
    }

    emit sigSearchEditIsDisplay(true);

    if (DBManager::instance()->getAlbumDBTypeFromUID(m_currentUID) == AutoImport) {
        setAcceptDrops(false); //自动导入路径禁止拖放
    } else {
        setAcceptDrops(true);
    }
}

void AlbumView::updateRightTrashView()
{
    auto allTrashInfos = DBManager::instance()->getAllTrashInfos_getRemainDays();
    QDateTime currentTime = QDateTime::currentDateTime();
    DBImgInfoList list;
    for (int i = allTrashInfos.size() - 1; i >= 0; i--) {
        DBImgInfo pinfo = allTrashInfos.at(i);
        if (!QFile::exists(pinfo.filePath) &&
                !QFile::exists(utils::base::getDeleteFullPath(pinfo.pathHash, pinfo.getFileNameFromFilePath()))) {
            allTrashInfos.removeAt(i);
        } else if (pinfo.remainDays <= 0) {
            list << pinfo;
            allTrashInfos.removeAt(i);
        }
    }
    //清理删除时间过长图片
    if (!list.isEmpty()) {
        ImageEngineApi::instance()->cleanUpTrash(list);
    }

    m_pRightTrashThumbnailList->clearAll();
    m_pRightTrashThumbnailList->insertBlankOrTitleItem(ItemTypeBlank, "", "", trash_title_height);
    m_pRightTrashThumbnailList->insertThumbnailByImgInfos(allTrashInfos);
    m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TRASH_LIST);
    m_trashBatchOperateWidget->setVisible(!allTrashInfos.isEmpty());
    m_pStatusBar->setVisible(true);
    m_pRightTrashThumbnailList->stopLoadAndClear();
}

void AlbumView::leftTabClicked()
{
    emit dApp->signalM->SearchEditClear();
    //若点击当前的item，则不做任何处理
    //判定规则：相同的UID+相同的名称+相同的类型，三者同时相同即判定为同一个项
    //不加相同的名称会导致无法在 已导入、最近删除、我的收藏 三个选项间切换
    if (m_currentUID == m_pLeftListView->getItemCurrentUID()
            && m_currentAlbum == m_pLeftListView->getItemCurrentName()
            && m_currentItemType == m_pLeftListView->getItemDataType()) {
        SearchReturnUpdate();
        return;
    }
    m_currentAlbum = m_pLeftListView->getItemCurrentName();
    m_currentType = m_pLeftListView->getItemCurrentType();
    m_currentUID = m_pLeftListView->getItemCurrentUID();
    m_currentItemType = static_cast<AblumType>(m_pLeftListView->getItemDataType());
    qDebug() << "------" << __FUNCTION__ << "";
    updateRightView();
}

bool AlbumView::checkIfNotified(const QString &dirPath)
{
    return DBManager::instance()->checkCustomAutoImportPathIsNotified(dirPath);
}

void AlbumView::onAddNewNotifyDir(const QString &dirPath)
{
    //自定义自动导入路径的相册名是文件夹最后一级的名字
    auto albumName = dirPath.split('/').last();

    //新建数据库
    int UID = DBManager::instance()->createNewCustomAutoImportPath(dirPath, albumName);

    //插入左侧label
    onCreateNewAlbumFromDialog(albumName, UID);

    //刷新图片
    //1.获取所有图片和视频
    QFileInfoList infos = utils::image::getImagesAndVideoInfo(dirPath, false);
    QStringList importFiles;
    std::transform(infos.begin(), infos.end(), std::back_inserter(importFiles), [](const QFileInfo & info) {
        return info.absoluteFilePath();
    });

    //2.导入进去
    ImageEngineApi::instance()->ImportImagesFromFileList(importFiles, albumName, UID, this, false, AutoImport, true);
}

void AlbumView::onTrashRecoveryBtnClicked()
{
    QStringList paths;
    paths = m_pRightTrashThumbnailList->selectedPaths();
    ImageEngineApi::instance()->recoveryImagesFromTrash(paths);
}

void AlbumView::onOpenImageFav(int row, const QString &path, bool bFullScreen)
{
    SignalManager::ViewInfo info;
    info.album = "";
//    info.lastPanel = nullptr;  //todo imageviewer
    info.fullScreen = bFullScreen;
    auto imagelist = m_favoriteThumbnailList->getFileList(row, ItemType::ItemTypePic);
    if (imagelist.size() > 0) {
        info.paths << imagelist;
        info.path = path;
    } else {
        info.paths.clear();
    }
    info.dBImgInfos = m_favoriteThumbnailList->getAllFileInfo(row);
    info.viewType = m_currentAlbum;
    info.viewMainWindowID = VIEW_MAINWINDOW_ALBUM;

    if (bFullScreen) {
        emit dApp->signalM->sigViewImage(info, Operation_FullScreen);
    } else {
        emit dApp->signalM->sigViewImage(info, Operation_NoOperation);
    }
}

void AlbumView::onOpenImageCustom(int row, const QString &path, bool bFullScreen)
{
    SignalManager::ViewInfo info;
    info.album = m_currentAlbum;
//    info.lastPanel = nullptr;  //todo imageviewer
    info.fullScreen = bFullScreen;
    auto imagelist = m_customThumbnailList->getFileList(row, ItemType::ItemTypePic);
    if (imagelist.size() > 0) {
        info.paths << imagelist;
        info.path = path;
    } else {
        info.paths.clear();
    }
    info.dBImgInfos = m_customThumbnailList->getAllFileInfo(row);
    info.viewType = m_currentAlbum;
    info.viewMainWindowID = VIEW_MAINWINDOW_ALBUM;

    if (bFullScreen) {
        emit dApp->signalM->sigViewImage(info, Operation_FullScreen, true, m_currentAlbum, m_currentUID);
    } else {
        emit dApp->signalM->sigViewImage(info, Operation_NoOperation, true, m_currentAlbum, m_currentUID);
    }
}

void AlbumView::onSlideShowFav(const QString &path)
{
    runSlideShow(path, m_favoriteThumbnailList);
}

void AlbumView::onSlideShowCustom(const QString &path)
{
    runSlideShow(path, m_customThumbnailList);
}

void AlbumView::runSlideShow(const QString &path, ThumbnailListView *listView)
{
    SignalManager::ViewInfo info;
    info.album = "";

    auto photolist = listView->selectedPaths();
    if (photolist.size() > 1) {
        //如果选中数目大于1，则幻灯片播放选中项
        info.paths = photolist;
        info.path = photolist.at(0);
    } else {
        //如果选中项只有一项，则幻灯片播放全部
        info.paths = listView->getFileList();
        info.path = path;
    }
    info.fullScreen = true;
    info.slideShow = true;
    info.viewType = m_currentAlbum;
    info.viewMainWindowID = VIEW_MAINWINDOW_ALBUM;
    emit dApp->signalM->startSlideShow(info);
}

void AlbumView::dragEnterEvent(QDragEnterEvent *e)
{
    if (!utils::base::checkMimeUrls(e->mimeData()->urls())) {
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
    ImageEngineApi::instance()->ImportImagesFromUrlList(urls, m_currentAlbum, m_currentUID, this);
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
        paths = m_pImpTimeLineView->selectPaths();
        m_pImpTimeLineView->getListView()->clearSelection();
        if (!paths.isEmpty()) {
            bMoveToTrash = true;
        }
    } else if (COMMON_STR_TRASH == m_currentType) {
        paths = m_pRightTrashThumbnailList->selectedPaths();
        if (!paths.isEmpty()) {
            ImgDeleteDialog *dialog = new ImgDeleteDialog(this, paths.size(), true);
            dialog->setObjectName("deteledialog");
            if (dialog->exec() > 0) {
                m_pRightTrashThumbnailList->clearSelection();
                ImageEngineApi::instance()->moveImagesToTrash(paths, true);
            }
        }
    } else if (COMMON_STR_FAVORITES == m_currentType) {
        paths = m_favoriteThumbnailList->selectedPaths();
        if (!paths.isEmpty()) {
            m_favoriteThumbnailList->clearSelection();
            DBManager::instance()->removeFromAlbum(DBManager::SpUID::u_Favorite, paths, AlbumDBType::Favourite);
            bMoveToTrash = true;
        }
    } else if (COMMON_STR_CUSTOM == m_currentType) {
        paths = m_customThumbnailList->selectedPaths();
        // 在不是自动导入相册的前提下：如果没有选中的照片，或相册中的照片数为0，则删除相册
        if ((paths.isEmpty() || DBManager::instance()->getItemsCountByAlbum(m_currentUID, ItemTypeNull) == 0) && !DBManager::isDefaultAutoImportDB(m_currentUID)) {
            QListWidgetItem *item = m_pLeftListView->m_pCustomizeListView->currentItem();
            AlbumLeftTabItem *pTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pCustomizeListView->itemWidget(item));
            m_deleteDialog = new AlbumDeleteDialog;
            connect(m_deleteDialog, &AlbumDeleteDialog::deleteAlbum, this, [ = ]() {
                QString str = pTabItem->m_albumNameStr;
                auto dbType = DBManager::instance()->getAlbumDBTypeFromUID(pTabItem->m_UID);

                if (dbType == AutoImport) { //自动导入删除步骤
                    DBManager::instance()->removeCustomAutoImportPath(pTabItem->m_UID);
                } else { //常规相册删除步骤
                    DBManager::instance()->removeAlbum(pTabItem->m_UID);
                }

                if (m_pLeftListView->m_pCustomizeListView->count() > 1) {
                    delete item;
                    m_currentItemType = ablumType;
                } else {
                    m_pLeftListView->updateCustomizeListView();
                    m_pLeftListView->updatePhotoListView();
                    m_currentItemType = photosType;
                }

                leftTabClicked();
                m_pLeftListView->onUpdateLeftListview();
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
    qDebug() << "挂载设备增加：" << mount->name();
    //TODO:
    //Support android phone, iPhone, and usb devices. Not support ftp, smb mount, non removeable disk now
    QString uri = mount->getRootFile()->uri();
    QString scheme = QUrl(uri).scheme();
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
        mountLoadStatus[strPath] = false;
        QString rename = "";
        rename = durlAndNameMap[QUrl(mount->getRootFile()->uri())];
        if ("" == rename) {
            rename = mount->name();
        }
        //判断路径是否存在
        bool bFind = false;
        QDir dir(strPath);
        if (!dir.exists()) {
            qDebug() << "onLoadMountImagesStart() !dir.exists()";
            bFind = false;
        } else {
            bFind = true;
        }
        //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,再调用findPicturePathByPhone函数搜索DCIM文件目录
        if (!strPath.contains("/media/")) {
            bFind = findPicturePathByPhone(strPath);
        }
        isIgnore = true;
        updateExternalDevice(mount, strPath);
        updateDeviceLeftList();
        if (bFind) {
            qDebug() << "------挂载路径存在，更新右侧视图---------";
            updateRightMountView();
        }
    }
}

//卸载外接设备
void AlbumView::onVfsMountChangedRemove(QExplicitlySharedDataPointer<DGioMount> mount)
{
    if (m_waitDeviceScandialog) {
        m_waitDeviceScandialog->close();
    }
    qDebug() << "zy------AlbumView::onVfsMountChangedRemove";
    QString uri = mount->getRootFile()->uri();
    QString strPath = mount->getDefaultLocationFile()->path();
    if (!strPath.contains("/media/")) {
        findPicturePathByPhone(strPath);
    }
    for (int i = 0; i < m_pLeftListView->m_pMountListWidget->count(); i++) {
        QListWidgetItem *pListWidgetItem = m_pLeftListView->m_pMountListWidget->item(i);
        AlbumLeftTabItem *pAlbumLeftTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pMountListWidget->itemWidget(pListWidgetItem));
        if (!pAlbumLeftTabItem || !pListWidgetItem)
            break;
        QString rename = "";
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
                m_currentItemType = devType;
            } else {
                m_pLeftListView->m_pMountListWidget->clear();
                m_pLeftListView->updatePhotoListView();
                m_currentItemType = photosType;
            }
            emit dApp->signalM->sigDevStop(strPath);
            durlAndNameMap.erase(durlAndNameMap.find(qurl));
            break;
        }
    }
    leftTabClicked();
    m_mounts = getVfsMountList();
    //通知子线程，设备已卸载
    emit ImageEngineApi::instance()->sigDeciveUnMount(strPath);
    for (auto endmount : m_mounts) {
        if (uri == endmount->getRootFile()->uri()) {
            //刚卸载的设备再次挂载上，需要走add逻辑
            qDebug() << "---Current remove decive is the same as current add--";
            m_mounts.removeOne(endmount);
        }
    }
    updateDeviceLeftList();
}

void AlbumView::getAllDeviceName()
{
    QStringList blDevList = DDiskManager::blockDevices(QVariantMap());
    for (const QString &blks : blDevList) {
        QSharedPointer<DBlockDevice> blk(DDiskManager::createBlockDevice(blks));
        QScopedPointer<DDiskDevice> drv1(DDiskManager::createDiskDevice(blk->drive()));

        if (!blk->hasFileSystem() && !drv1->mediaCompatibility().join(" ").contains("optical") && !blk->isEncrypted()) {
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
        // 不能强制挂载，否则卸载不了
        //blk->mount({});
        QByteArrayList qbl = blk->mountPoints();
        QString mountPoint = "file://";
        QList<QByteArray>::iterator qb = qbl.begin();
        while (qb != qbl.end()) {
            mountPoint += (*qb);
            ++qb;
        }
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
    const QList<QExplicitlySharedDataPointer<DGioMount> > mounts = utils::base::getMounts_safe();
    for (auto mount : mounts) {
        //TODO:
        //Support android phone, iPhone, and usb devices. Not support ftp, smb, non removeable disk now
        QString scheme = QUrl(mount->getRootFile()->uri()).scheme();
        if ((scheme == "file" /*&& mount->canEject()*/) ||  //usb device
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

void AlbumView::importComboBoxChange(QString strText)
{
    Q_UNUSED(strText);
    if (1 == m_importByPhoneComboBox->currentIndex()) {
        AlbumCreateDialog *dialog = new AlbumCreateDialog(this);
        dialog->show();
        qDebug() << "xxxxxxxxxx" << window()->x();
        dialog->move(window()->x() + (window()->width() - dialog->width()) / 2, window()->y() + (window()->height() - dialog->height()) / 2);
        connect(dialog, &AlbumCreateDialog::albumAdded, this, [ = ] {
            DBManager::instance()->insertIntoAlbum(dialog->getCreateUID(), QStringList(" "));
            onCreateNewAlbumFrom(dialog->getCreateAlbumName(), dialog->getCreateUID());
            updateImportComboBox();
            m_importByPhoneComboBox->setCurrentIndex(m_importByPhoneComboBox->count() - 1);
        });
        connect(dialog, &AlbumCreateDialog::sigClose, this, [ = ] {
            m_importByPhoneComboBox->setCurrentIndex(0);
        });
    }
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
        mountLoadStatus.clear();
        mountLoadStatus.insert(strPath, false);
        //判断路径是否存在
        bool bFind = false;
        QDir dir(strPath);
        if (!dir.exists()) {
            delete pListWidgetItem;
            pListWidgetItem = nullptr;
            qDebug() << "onLoadMountImagesStart() !dir.exists()";
            dApp->signalM->sigLoadMountImagesEnd(rename);
        } else {
            bFind = true;
        }
        //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,在调用findPicturePathByPhone函数搜索DCIM文件目录
        if (!strPath.contains("/media/")) {
            bFind = findPicturePathByPhone(strPath);
            if (!bFind) {
                qDebug() << "onLoadMountImagesStart() !bFind";
                dApp->signalM->sigLoadMountImagesEnd(rename);
            }
        }
        if (pListWidgetItem) {
            m_pLeftListView->m_pMountListWidget->addItem(pListWidgetItem);
            pListWidgetItem->setData(Qt::UserRole, strPath);
            pListWidgetItem->setData(Qt::UserRole + 1, rename);
            qDebug() << "------" << __FUNCTION__ << "---strPath = " << strPath << " ---rename = " << rename;
            QVariant value = QVariant::fromValue(mount);
            pListWidgetItem->setData(Qt::UserRole + 2, value);
            pListWidgetItem->setSizeHint(QSize(LEFT_VIEW_LISTITEM_WIDTH, LEFT_VIEW_LISTITEM_HEIGHT));
            AlbumLeftTabItem *pAlbumLeftTabItem;

            if (strPath.contains("/media/")) {
                pAlbumLeftTabItem = new AlbumLeftTabItem(rename, -1, ALBUM_PATHTYPE_BY_U);
            } else {
                pAlbumLeftTabItem = new AlbumLeftTabItem(rename, -1, ALBUM_PATHTYPE_BY_PHONE);
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
                //todo
//                ImageEngineApi::instance()->getImageFilesFromMount(rename, strPath, this);
            }
        }
#endif
    }
}

void AlbumView::updateExternalDevice(QExplicitlySharedDataPointer<DGioMount> mount, QString strPath)
{
    qDebug() << "------更新外部设备---： " << strPath;
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
        pAlbumLeftTabItem = new AlbumLeftTabItem(rename, -1, ALBUM_PATHTYPE_BY_U);
    } else {
        pAlbumLeftTabItem = new AlbumLeftTabItem(rename, -1, ALBUM_PATHTYPE_BY_PHONE);
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
        // 设置当前的相册名及类型，正确跳转其他页面
        m_currentAlbum = rename;
        m_currentType = AblumType::devType;
    }
    //右侧视图同时切换
    m_pRightPhoneThumbnailList->stopLoadAndClear(true);     //清除已有数据
    m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_PHONE);
    m_pStatusBar->setVisible(true);
    m_mounts = getVfsMountList();
}

void AlbumView::onUpdataAlbumRightTitle(const QString &titlename)
{
    m_currentAlbum = titlename;
    updateRightView();
}

void AlbumView::SearchReturnUpdate()
{
    if (RIGHT_VIEW_SEARCH == m_pRightStackWidget->currentIndex()) {
        if (COMMON_STR_TRASH == m_currentAlbum) {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TRASH_LIST);
            //最近删除内没有图片，隐藏批量处理按钮
            int count = DBManager::instance()->getTrashImgsCount();
            m_trashBatchOperateWidget->setVisible(count != 0);
        } else if (COMMON_STR_FAVORITES == m_currentAlbum) {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_FAVORITE_LIST);
            int count = DBManager::instance()->getAlbumImgsCount(m_currentUID);
            //收藏内没有图片，隐藏批量处理按钮
            m_FavoriteTitleWidget->setVisible(count != 0);
        } else if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum) {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_TIMELINE_IMPORT);
        } else {
            m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_THUMBNAIL_LIST);
            //更新搜索结果为空，清除搜索界面没有切换到初始状态   xiaolong 2020/05/22
            updateRightCustomAlbumView();
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
        }
    }
    return false;
}

void AlbumView::updateImportComboBox()
{
    m_importByPhoneComboBox->clear();
    m_importByPhoneComboBox->addItem(tr("Gallery"), -1);
    m_importByPhoneComboBox->addItem(tr("New album"));
    auto allAlbumNames = DBManager::instance()->getAllAlbumNames(Custom); //只检索自定义相册
    for (auto albumName : allAlbumNames) {
        m_importByPhoneComboBox->addItem(albumName.second, albumName.first); //显示相册名，然后藏一个相册UID
    }
    m_importByPhoneComboBox->setCurrentText(tr("Gallery"));     //默认选中
}

//手机照片全部导入
void AlbumView::importAllBtnClicked()
{
    QStringList allPaths = m_pRightPhoneThumbnailList->getFileList();
    importFromMountDevice(allPaths);
}

//手机照片导入选中
void AlbumView::importSelectBtnClicked()
{
    QStringList selectPaths = m_pRightPhoneThumbnailList->selectedPaths();
    importFromMountDevice(selectPaths);
}

//手机照片导入具体实现步骤
void AlbumView::importFromMountDevice(const QStringList &paths)
{
    QString albumName = m_importByPhoneComboBox->currentText();
    int UID = m_importByPhoneComboBox->currentData().toInt();
    if (m_importByPhoneComboBox->currentIndex() == 0) {
        albumName = "";
        UID = -1;
    }

    ImageEngineApi::instance()->importImageFilesFromMount(albumName, UID, paths, this); //执行导入

    for (int i = 0; i < m_pLeftListView->m_pMountListWidget->count(); i++) {
        QListWidgetItem *pListWidgetItem = m_pLeftListView->m_pMountListWidget->item(i);
        AlbumLeftTabItem *pAlbumLeftTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pMountListWidget->itemWidget(pListWidgetItem));
        if (!pAlbumLeftTabItem) continue;
        if (UID == pAlbumLeftTabItem->m_UID) { //切换条件用UID同步
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

void AlbumView::needUnMount(const QString &path)
{
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
        QList<QByteArray>::iterator qb = qbl.begin();
        while (qb != qbl.end()) {
            mountPoint += (*qb);
            ++qb;
        }
        if (mountPoint.contains(path, Qt::CaseSensitive)) {
            blkget = blk;
            break;
        } else {
            mountPoint = "";
        }
    }
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
            QExplicitlySharedDataPointer<DGioFile> LocationFile = mount->getDefaultLocationFile();
            if (LocationFile->path().compare(path) == 0 && mount->canUnmount() && !blkget.isNull()) { //增加blkget为空判断，某些情况下卸载设备会导致程序闪退
                QScopedPointer<DDiskDevice> drv(DDiskManager::createDiskDevice(blkget->drive()));
                QScopedPointer<DBlockDevice> cbblk(DDiskManager::createBlockDevice(blkget->cryptoBackingDevice()));
                if (!drv->removable()) {
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
                if (drv->canPowerOff()) {
                    drv->powerOff({});
                }
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
            //调整逻辑，先尝试卸载，在确认能够卸载后开始处理界面
            m_pRightPhoneThumbnailList->stopLoadAndClear();
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
            QUrl qurl(mount->getRootFile()->uri());
            durlAndNameMap.erase(durlAndNameMap.find(qurl));
            break;
        }
    }
    m_mounts = getVfsMountList();
    updateDeviceLeftList();
}

//卸载外部设备
void AlbumView::onUnMountSignal(const QString &unMountPath)
{
//    m_pRightPhoneThumbnailList->stopLoadAndClear();
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
        currentViewList = m_favoriteThumbnailList;
        dropItemPaths = currentViewList->getDagItemPath();
    } else if (COMMON_STR_TRASH == m_currentAlbum) {
        currentViewList = m_pRightTrashThumbnailList;
        dropItemPaths = currentViewList->getDagItemPath();
    } else if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum) {
        dropItemPaths = m_pImpTimeLineView->selectPaths();
    } else {
        currentViewList = m_customThumbnailList;
        dropItemPaths = currentViewList->getDagItemPath();
    }
    qDebug() << "dropItemPaths: " << dropItemPaths;
    //向其他相册拖拽，动作添加
    DBManager::instance()->insertIntoAlbum(item->m_UID, dropItemPaths);
    //LMH0509,为了解决24887 【相册】【5.6.9.13】拖动已导入相册中的图片到新建相册，相册崩溃
    QModelIndex index;
    emit m_pLeftListView->m_pCustomizeListView->pressed(index);
    m_pLeftListView->m_pCustomizeListView->setCurrentRow(dropIndex.row());
}

void AlbumView::updatePicNum()
{
    int photoSelctCount = 0;
    int videoSelctCount = 0;
    if (RIGHT_VIEW_SEARCH == m_pRightStackWidget->currentIndex()) {
        photoSelctCount = m_pSearchView->m_pThumbnailListView->getAppointTypeSelectItemCount(ItemTypePic);
        videoSelctCount = m_pSearchView->m_pThumbnailListView->getAppointTypeSelectItemCount(ItemTypeVideo);
    } else {
        if (m_currentAlbum == COMMON_STR_TRASH) {
            photoSelctCount = m_pRightTrashThumbnailList->getAppointTypeSelectItemCount(ItemTypePic);
            videoSelctCount = m_pRightTrashThumbnailList->getAppointTypeSelectItemCount(ItemTypeVideo);
        } else if (m_currentAlbum == COMMON_STR_FAVORITES) {
            photoSelctCount = m_favoriteThumbnailList->getAppointTypeSelectItemCount(ItemTypePic);
            videoSelctCount = m_favoriteThumbnailList->getAppointTypeSelectItemCount(ItemTypeVideo);
        } else if (m_currentAlbum == COMMON_STR_RECENT_IMPORTED) {
            photoSelctCount = m_pImpTimeLineView->getListView()->getAppointTypeSelectItemCount(ItemTypePic);
            videoSelctCount = m_pImpTimeLineView->getListView()->getAppointTypeSelectItemCount(ItemTypeVideo);
        } else if (RIGHT_VIEW_PHONE == m_pRightStackWidget->currentIndex()) {
            photoSelctCount = m_pRightPhoneThumbnailList->getAppointTypeSelectItemCount(ItemTypePic);
            videoSelctCount = m_pRightPhoneThumbnailList->getAppointTypeSelectItemCount(ItemTypeVideo);
        } else {
            photoSelctCount = m_customThumbnailList->getAppointTypeSelectItemCount(ItemTypePic);
            videoSelctCount = m_customThumbnailList->getAppointTypeSelectItemCount(ItemTypeVideo);
        }
    }

    if (photoSelctCount > 0 || videoSelctCount > 0) {
        m_pStatusBar->resetSelectedStatue(photoSelctCount, videoSelctCount);
    } else {
        restorePicNum();
    }
}

void AlbumView::restorePicNum()
{
    int photoCount = 0;
    int videoCount = 0;
    if (4 == m_pRightStackWidget->currentIndex()) {
        photoCount = m_pSearchView->m_pThumbnailListView->getAppointTypeItemCount(ItemTypePic);
        videoCount = m_pSearchView->m_pThumbnailListView->getAppointTypeItemCount(ItemTypeVideo);
    } else {
        if (COMMON_STR_RECENT_IMPORTED == m_currentAlbum) {
            photoCount = m_pImpTimeLineView->getListView()->getAppointTypeItemCount(ItemTypePic);
            videoCount = m_pImpTimeLineView->getListView()->getAppointTypeItemCount(ItemTypeVideo);
        } else if (COMMON_STR_TRASH == m_currentAlbum) {
            photoCount = m_pRightTrashThumbnailList->getAppointTypeItemCount(ItemTypePic);
            videoCount = m_pRightTrashThumbnailList->getAppointTypeItemCount(ItemTypeVideo);
        } else if (COMMON_STR_FAVORITES == m_currentAlbum) {
            photoCount = m_favoriteThumbnailList->getAppointTypeItemCount(ItemTypePic);
            videoCount = m_favoriteThumbnailList->getAppointTypeItemCount(ItemTypeVideo);
        } else if (RIGHT_VIEW_PHONE == m_pRightStackWidget->currentIndex()) {
            photoCount = m_pRightPhoneThumbnailList->getAppointTypeItemCount(ItemTypePic);
            videoCount = m_pRightPhoneThumbnailList->getAppointTypeItemCount(ItemTypeVideo);
        } else {
            photoCount = m_customThumbnailList->getAppointTypeItemCount(ItemTypePic);
            videoCount = m_customThumbnailList->getAppointTypeItemCount(ItemTypeVideo);
        }
    }
    if ((photoCount + videoCount) <= 0) {
        m_pStatusBar->setVisible(false);
    } else {
        m_pStatusBar->setVisible(true);
    }

    m_pStatusBar->resetUnselectedStatue(photoCount, videoCount);
}

void AlbumView::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    if (nullptr != pPhoneWidget) {
        phonetopwidget->setFixedWidth(pPhoneWidget->width() - 15);//BUG#93779 -15把右侧滚动条露出来
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
    //响应关闭事件，直接关闭线程
    ImageEngineApi::instance()->setThreadShouldStop();
    m_waitDeviceScandialog->close();
    this->setEnabled(true);
}

void AlbumView::onWaitDialogIgnore()
{
    isIgnore = false;
    m_waitDeviceScandialog->hide();
    this->setEnabled(true);
}

void AlbumView::onRepeatImportingTheSamePhotos(QStringList importPaths, QStringList duplicatePaths, int UID)
{
    Q_UNUSED(importPaths)
    if (m_currentUID == UID && dApp->getMainWindow()->getCurrentViewType() == 2) {
        m_customThumbnailList->selectPhotos(duplicatePaths);
    }
}

void AlbumView::ongMouseMove()
{
    updatePicNum();
}

void AlbumView::onSelectAll()
{
    updatePicNum();
}

void AlbumView::onInsertedIntoAlbum(int UID, QStringList pathlist)
{
    qDebug() << "添加到目的相册：" << UID;
    Q_UNUSED(pathlist);
    if (m_currentUID != UID) {
        return;
    }
    if (m_currentType == COMMON_STR_CUSTOM) //如果需要更新的为当前界面
        updateRightView();
}

void AlbumView::onFileSystemAdded(const QString &dbusPath)
{
    DBlockDevice *blDev = DDiskManager::createBlockDevice(dbusPath);
    blDev->mount({});
}

void AlbumView::onMonitorDestroyed(int UID)
{
    //直接把左侧栏删了即可，数据库操作交给别的地方
    for (int i = 0; i < m_pLeftListView->m_pCustomizeListView->count(); i++) {
        QListWidgetItem *item = m_pLeftListView->m_pCustomizeListView->item(i);
        AlbumLeftTabItem *pTabItem = dynamic_cast<AlbumLeftTabItem *>(m_pLeftListView->m_pCustomizeListView->itemWidget(item));
        if (pTabItem) {
            if (UID == pTabItem->m_UID) {
                delete item;
                break;
            }
        }
    }

    //刷新界面
    if (this->isVisible()) {
        leftTabClicked();
        m_pLeftListView->onUpdateLeftListview();
    }
}

void AlbumView::onBlockDeviceAdded(const QString &blks)
{
    QSharedPointer<DBlockDevice> blk(DDiskManager::createBlockDevice(blks));
    QScopedPointer<DDiskDevice> drv1(DDiskManager::createDiskDevice(blk->drive()));
    if (!blk->hasFileSystem() && !drv1->mediaCompatibility().join(" ").contains("optical") && !blk->isEncrypted()) {
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
    }

    if (mps.contains(QByteArray("/\0", 2))) {
        udispname = QCoreApplication::translate("PathManager", "System Disk");
        goto runend;
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
        }
        if (drv->opticalBlank()) {
            udispname = QCoreApplication::translate("DeepinStorage", "Blank %1 Disc").arg(opticalmediamap[drv->media()]);
            goto runend;
        }
        if (pblk->isEncrypted() && !blk) {
            udispname = QCoreApplication::translate("DeepinStorage", "%1 Encrypted").arg(formatSize(qint64(size)));
            goto runend;
        }
        udispname = QCoreApplication::translate("DeepinStorage", "%1 Volume").arg(formatSize(qint64(size)));
        goto runend;
    }
    udispname = label;

runend:
    blk->mount({});
    QByteArrayList qbl = blk->mountPoints();
    QString mountPoint = "file://";
    QList<QByteArray>::iterator qb = qbl.begin();
    while (qb != qbl.end()) {
        mountPoint += (*qb);
        ++qb;
    }
    QUrl qurl(mountPoint);
    durlAndNameMap[qurl] = udispname;
    return;
}

void AlbumView::onThemeTypeChanged(DGuiApplicationHelper::ColorType themeType)
{
    //add start 3975
    DPalette palcolor = DApplicationHelper::instance()->palette(m_pCustomAlbumWidget);
    palcolor.setBrush(DPalette::Base, palcolor.color(DPalette::Window));
    m_pCustomAlbumWidget->setPalette(palcolor);

    DPalette ppal_light = DApplicationHelper::instance()->palette(m_customAlbumTitle);
    ppal_light.setBrush(DPalette::Background, ppal_light.color(DPalette::Base));
    m_customAlbumTitle->setPalette(ppal_light);

    DPalette palcolor2 = DApplicationHelper::instance()->palette(m_pFavoriteWidget);
    palcolor2.setBrush(DPalette::Base, palcolor2.color(DPalette::Window));
    m_pFavoriteWidget->setPalette(palcolor2);

    DPalette ppal_light2 = DApplicationHelper::instance()->palette(m_FavoriteTitleWidget);
    ppal_light2.setBrush(DPalette::Background, ppal_light2.color(DPalette::Base));
    m_FavoriteTitleWidget->setPalette(ppal_light2);

    DPalette palcolor3 = DApplicationHelper::instance()->palette(m_pTrashWidget);
    palcolor3.setBrush(DPalette::Base, palcolor3.color(DPalette::Window));
    m_pTrashWidget->setPalette(palcolor3);

    DPalette ppal_light3 = DApplicationHelper::instance()->palette(m_TrashTitleWidget);
    ppal_light3.setBrush(DPalette::Background, ppal_light3.color(DPalette::Base));
    m_TrashTitleWidget->setPalette(ppal_light3);
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
    m_TrashDescritionLab->setForegroundRole(DPalette::Text);
    m_TrashDescritionLab->setPalette(pal);
    // 设备相册界面标题
    DPalette phonePal = DApplicationHelper::instance()->palette(m_pPhonePicTotal);
    QColor phone_Color_BT = phonePal.color(DPalette::BrightText);
    if (themeType == DGuiApplicationHelper::LightType) {
        phone_Color_BT.setAlphaF(0.5);
        phonePal.setBrush(DPalette::Text, phone_Color_BT);
    } else if (themeType == DGuiApplicationHelper::DarkType) {
        phone_Color_BT.setAlphaF(0.75);
        phonePal.setBrush(DPalette::Text, phone_Color_BT);
    }
    m_pPhonePicTotal->setForegroundRole(DPalette::Text);
    m_pPhonePicTotal->setPalette(phonePal);
}

void AlbumView::onRightPhoneCustomContextMenuRequested()
{
    QStringList paths = m_pRightPhoneThumbnailList->selectedPaths();
    if (0 < paths.length()) {
        m_importSelectByPhoneBtn->setEnabled(true);
    } else {
        m_importSelectByPhoneBtn->setEnabled(false);
    }

    updatePicNum();
}

void AlbumView::onRightPhoneThumbnailListMouseRelease()
{
    QStringList paths = m_pRightPhoneThumbnailList->selectedPaths();
    if (0 < paths.length()) {
        m_importSelectByPhoneBtn->setEnabled(true);
    } else {
        m_importSelectByPhoneBtn->setEnabled(false);
    }

    updatePicNum();
}

void AlbumView::onImportViewImportBtnClicked()
{
    m_customAlbumTitleLabel->setVisible(false);
    m_pRightPicTotal->setVisible(false);
    emit dApp->signalM->startImprot();
    m_pImportView->onImprotBtnClicked();
}

void AlbumView::onImportFailedToView()
{
    if (isVisible()) {
        updateRightView();
    }
}

void AlbumView::onWaitDailogTimeout()
{
    m_waitDeviceScandialog->m_ignoreDeviceScan->setEnabled(true);
    m_waitDeviceScandialog->m_closeDeviceScan->setEnabled(true);
    m_waitDailog_timer->stop();
}

void AlbumView::onLeftListViewMountListWidgetClicked(const QModelIndex &index)
{
    Q_UNUSED(index);
    return;
//    m_pRightPhoneThumbnailList->stopLoadAndClear(false);
//    m_pLeftListView->setFocus();
//    qDebug() << "------" << __FUNCTION__ << "";
//    updateRightView();
}

void AlbumView::sltLoadMountFileList(const QString &path, QStringList fileList)
{
    //线程已处理完成数据，通知等待窗口计时自动隐藏
    QTimer::singleShot(1500, this, [ = ] {
        m_waitDeviceScandialog->close();
    });
    QString strPath;
    QString phoneTitle;
    if (m_pLeftListView->m_pMountListWidget->currentItem()) {
        strPath = m_pLeftListView->m_pMountListWidget->currentItem()->data(Qt::UserRole).toString();
        if (!strPath.contains("/media/") && !strPath.contains("DCIM")) {
            findPicturePathByPhone(strPath);
        }
        phoneTitle = m_pLeftListView->m_pMountListWidget->currentItem()->data(Qt::UserRole + 1).toString();
    }
    if (path != strPath) {
        return;
    }
    //更新外部设备页面标题等状态
    m_importByPhoneComboBox->setEnabled(true);
    m_importAllByPhoneBtn->setEnabled(true);
    updateImportComboBox();
    m_phoneTitleFullStr = phoneTitle;
    m_pPhoneTitle->setText(phoneTitle);
    QFontMetrics elideFont(m_pPhoneTitle->font());
    m_pPhoneTitle->setText(elideFont.elidedText(phoneTitle, Qt::ElideRight, 525));
    if (m_iAlubmPicsNum > 0) {
        m_pRightPhoneThumbnailList->m_imageType = ALBUM_PATHTYPE_BY_PHONE;
        m_pRightPhoneThumbnailList->m_currentUID = -1;
        QStringList paths = m_pRightPhoneThumbnailList->selectedPaths();
        if (0 < paths.length()) {
            m_importSelectByPhoneBtn->setEnabled(true);
        } else {
            m_importSelectByPhoneBtn->setEnabled(false);
        }
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_PHONE);
        m_pStatusBar->setVisible(true);
    } else {
        m_pPhoneTitle->setText(m_currentAlbum);
        m_pPhoneTitle->setText(elideFont.elidedText(m_currentAlbum, Qt::ElideRight, 525));
        m_pRightPhoneThumbnailList->m_imageType = ALBUM_PATHTYPE_BY_PHONE;
        m_pRightPhoneThumbnailList->m_currentUID = -1;
        m_pRightStackWidget->setCurrentIndex(RIGHT_VIEW_PHONE);
        m_pStatusBar->setVisible(true);
    }
    setAcceptDrops(false);

    using namespace utils::image;
    DBImgInfoList infos;
    for (int i = 0; i < fileList.size(); i++) {
        DBImgInfo info;
        if (utils::base::isVideo(fileList.at(i))) {
            info.itemType = ItemType::ItemTypeVideo;
        }
        info.filePath = fileList.at(i);
        infos.append(info);
    }
    //先清除所有
    m_pRightPhoneThumbnailList->clearAll();
    //添加上方空白栏
    m_pRightPhoneThumbnailList->insertBlankOrTitleItem(ItemTypeBlank, "", "", m_importByPhoneWidget->height());
    //添加图片信息
    m_pRightPhoneThumbnailList->insertThumbnailByImgInfos(infos);

    //有有效图片时，设置按钮状态
    if (m_pRightPhoneThumbnailList->m_model->rowCount() <= 0) {
        m_importByPhoneComboBox->setEnabled(false);
        m_importAllByPhoneBtn->setEnabled(false);
        m_importSelectByPhoneBtn->setEnabled(false);
    }

    //重置数量显示
    resetLabelCount(m_pRightPhoneThumbnailList->getAppointTypeItemCount(ItemTypePic)
                    , m_pRightPhoneThumbnailList->getAppointTypeItemCount(ItemTypeVideo), m_pPhonePicTotal);

    adjustTitleContent();
    m_pStatusBar->m_pAllPicNumLabel->setText(m_pPhonePicTotal->text());
    qDebug() << "------图片缩略图加载完成---" << QThread::currentThreadId() << infos.size();
}

//筛选显示，当先列表中内容为无结果
void AlbumView::slotNoPicOrNoVideo(bool isNoResult)
{
    if (sender() == m_customThumbnailList) {
        m_customNoResultWidget->setVisible(isNoResult);
        m_customThumbnailList->setVisible(!isNoResult);
        m_customAlbumTitleLabel->setVisible(!isNoResult);
        m_pRightPicTotal->setVisible(!isNoResult);
    } else if (sender() == m_favoriteThumbnailList) {
        m_favoriteNoResultWidget->setVisible(isNoResult);
        m_favoriteThumbnailList->setVisible(!isNoResult);
        m_pFavoriteTitle->setVisible(!isNoResult);
        m_pFavoritePicTotal->setVisible(!isNoResult);
    } else if (sender() == m_pRightTrashThumbnailList) {
        m_trashNoResultWidget->setVisible(isNoResult);
        m_pRightTrashThumbnailList->setVisible(!isNoResult);
        m_TrashTitleLab->setVisible(!isNoResult);
        m_TrashDescritionLab->setVisible(!isNoResult);
    }
    if (isNoResult) {
        m_pStatusBar->m_pAllPicNumLabel->setText(QObject::tr("No results"));
    } else {
        updatePicNum();
    }
}

void AlbumView::sltSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected)
    Q_UNUSED(deselected)
    updatePicNum();
}

void AlbumView::onTrashInfosChanged()
{
    updateRightView();
}

void AlbumView::restoreTitleDisplay()
{
    //顶部栏不存在时的标题处理（涉及样式还原）
    if (!m_trashBatchOperateWidget->isVisible()) {//最近删除界面标题
        m_TrashTitleLab->setText(tr("Trash"));
        // 最近删除内容被清空后，顶部操作栏隐藏，需要重新调整"最近删除"标题显示大小
        m_TrashTitleLab->adjustSize();
        m_TrashTitleLab->show();
        m_TrashTitleLab->raise();
    }
    if (!m_customBatchOperateWidget->isVisible()) {//自定义相册界面标题
        m_customAlbumTitleLabel->setText(m_currentAlbum);
        m_customAlbumTitleLabel->show();
        m_customAlbumTitleLabel->raise();
    }
    if (!m_favoriteBatchOperateWidget->isVisible()) {//我的收藏界面标题
        m_pFavoriteTitle->setText(tr("Favorites"));
        m_pFavoriteTitle->show();
        m_pFavoriteTitle->raise();
    }
}

void AlbumView::adjustTitleContent()
{
    m_spinner->move(width() / 2 + 60, (height()) / 2 - 20);
    m_pwidget->setFixedSize(this->width(), this->height() - 23);
    m_pwidget->move(0, 0);
    //我的收藏
    if (nullptr != m_FavoriteTitleWidget) {
        m_FavoriteTitleWidget->setFixedSize(this->width() - m_pLeftListView->width() - magin_offset, favorite_title_height);
        if (topLevelWidget()->width() <= MAINWINDOW_NEEDCUT_WIDTH)
            m_pFavoriteTitle->move(topLevelWidget()->width() - LISTVIEW_MINMUN_WIDTH, 0);
        else
            m_pFavoriteTitle->move(m_FavoriteTitleWidget->width() / 2 - m_pFavoriteTitle->width() / 2, 0);
        if (m_pFavoriteTitle->isVisible())
            m_pFavoriteTitle->raise();
    }
    //最近删除
    if (nullptr != m_TrashTitleWidget) {
        m_TrashTitleWidget->setFixedSize(this->width() - m_pLeftListView->width(), trash_title_height);
        if (topLevelWidget()->width() <= MAINWINDOW_NEEDCUT_WIDTH)
            m_TrashTitleLab->move(topLevelWidget()->width() - LISTVIEW_MINMUN_WIDTH, 0);
        else
            m_TrashTitleLab->move(m_TrashTitleWidget->width() / 2 - m_TrashTitleLab->width() / 2, 0);
        if (m_TrashTitleLab->isVisible())
            m_TrashTitleLab->raise();
    }
    //设备
    if (nullptr != pPhoneWidget) {
        if (topLevelWidget()->width() <= MAINWINDOW_DEVICE_MIN_WIDTH) {
            m_pPhoneTitle->setFixedWidth(PHONE_TITLE_NORMAL_WIDTH - (MAINWINDOW_DEVICE_MIN_WIDTH - topLevelWidget()->width()));
            m_pPhonePicTotal->setFixedWidth(PHONE_TITLE_NORMAL_WIDTH - (MAINWINDOW_DEVICE_MIN_WIDTH - topLevelWidget()->width()));
        } else {
            m_pPhoneTitle->setMaximumWidth(PHONE_TITLE_MAX_WIDTH);
            m_pPhonePicTotal->setMaximumWidth(PHONE_TITLE_MAX_WIDTH);
        }
        phonetopwidget->setFixedWidth(pPhoneWidget->width() - 15);//BUG#93779 -15把右侧滚动条露出来
    }
    //自定义相册
    if (nullptr != m_customAlbumTitle) {
        m_customAlbumTitle->setFixedSize(this->width() - m_pLeftListView->width() - magin_offset, custom_title_height);
        if (topLevelWidget()->width() <= MAINWINDOW_NEEDCUT_WIDTH)
            m_customAlbumTitleLabel->move(topLevelWidget()->width() - LISTVIEW_MINMUN_WIDTH, 0);
        else
            m_customAlbumTitleLabel->move(m_customAlbumTitle->width() / 2 - m_customAlbumTitleLabel->width() / 2, 0);
        if (m_customAlbumTitleLabel->isVisible())
            m_customAlbumTitleLabel->raise();
    }

    //add end 3975
    m_pStatusBar->setFixedWidth(this->width() - m_pLeftListView->width());
    m_pStatusBar->move(m_pLeftListView->width(), this->height() - m_pStatusBar->height());
    m_pStatusBar->raise();

    // 最近删除 描述label截断显示处理
    adaptiveTrashDescritionLabel();

    // 我的收藏 总数label截断显示处理
    QString elidedText = utils::base::reorganizationStr(m_pFavoritePicTotal->font(), m_FavoritePicTotalFullStr, m_pFavoriteTitle->x() - 30);
    m_pFavoritePicTotal->setText(elidedText);
    if (elidedText != m_FavoritePicTotalFullStr) {
        m_pFavoritePicTotal->setToolTip(m_FavoritePicTotalFullStr);
    } else {
        m_pFavoritePicTotal->setToolTip("");
    }

    // 自定义相册 总数label截断显示处理
    elidedText = utils::base::reorganizationStr(m_pRightPicTotal->font(), m_CustomRightPicTotalFullStr, m_customAlbumTitleLabel->x() - 30);
    m_pRightPicTotal->setText(elidedText);
    if (elidedText != m_CustomRightPicTotalFullStr) {
        m_pRightPicTotal->setToolTip(m_CustomRightPicTotalFullStr);
    } else {
        m_pRightPicTotal->setToolTip("");
    }

    // 设备识别 标题和总数label截断显示处理
    elidedText = utils::base::reorganizationStr(m_pPhoneTitle->font(), m_phoneTitleFullStr, m_pPhoneTitle->width());
    m_pPhoneTitle->setText(elidedText);
    m_pPhoneTitle->update();
    if (elidedText != m_phoneTitleFullStr) {
        m_pPhoneTitle->setToolTip(m_phoneTitleFullStr);
    } else {
        m_pPhoneTitle->setToolTip("");
    }
    elidedText = utils::base::reorganizationStr(m_pPhonePicTotal->font(), m_PhonePicTotalFullStr, m_pPhonePicTotal->width());
    m_pPhonePicTotal->setText(elidedText);
    m_pPhonePicTotal->update();
    if (elidedText != m_PhonePicTotalFullStr) {
        m_pPhonePicTotal->setToolTip(m_PhonePicTotalFullStr);
    } else {
        m_pPhonePicTotal->setToolTip("");
    }
    if (QLocale::system().language() == QLocale::Uighur) {
        if (m_TrashTitleLab->isVisible()) {//最近删除界面标题
            int size = m_trashBatchOperateWidget->x() - (m_TrashTitleLab->x() + m_TrashTitleLab->width());
            QString Str = utils::base::reorganizationStr(m_TrashTitleLab->font(), tr("Trash"), m_TrashTitleLab->width() + size);
            if (Str.length() > 0) {
                m_TrashTitleLab->show();
                m_TrashTitleLab->raise();
            } else {
                m_TrashTitleLab->hide();
            }
            m_TrashTitleLab->setText(Str);
            if (Str != tr("Trash")) {
                m_TrashTitleLab->setToolTip(tr("Trash"));
            } else {
                m_TrashTitleLab->setToolTip("");
            }
        } else if (m_customAlbumTitleLabel->isVisible()) {//自定义相册界面标题
            m_customAlbumTitleLabel->setText(m_currentAlbum);
            m_customAlbumTitleLabel->adjustSize();
            m_customAlbumTitleLabel->move(m_customAlbumTitle->width() / 2 - m_customAlbumTitleLabel->width() / 2, 0);
            QString Str = utils::base::reorganizationStr(m_customAlbumTitleLabel->font(), m_currentAlbum,
                                                         2 * m_customBatchOperateWidget->x() - m_customAlbumTitle->width());
            if (Str.length() > 0) {
                m_customAlbumTitleLabel->show();
                m_customAlbumTitleLabel->raise();
            } else {
                m_customAlbumTitleLabel->hide();
            }
            m_customAlbumTitleLabel->setText(Str);
            m_customAlbumTitleLabel->adjustSize();
            m_customAlbumTitleLabel->move(m_customAlbumTitle->width() / 2 - m_customAlbumTitleLabel->width() / 2, 0);
            m_customAlbumTitleLabel->raise();
            if (Str != m_currentAlbum) {
                m_customAlbumTitleLabel->setToolTip(m_currentAlbum);
            } else {
                m_customAlbumTitleLabel->setToolTip("");
            }
        } else if (m_pFavoriteTitle->isVisible()) {//我的收藏界面标题
            int size = m_favoriteBatchOperateWidget->x() - (m_pFavoriteTitle->x() + m_pFavoriteTitle->width());
            QString Str = utils::base::reorganizationStr(m_pFavoriteTitle->font(), tr("Favorites"), m_pFavoriteTitle->width() + size);
            if (Str.length() > 0) {
                m_pFavoriteTitle->show();
                m_pFavoriteTitle->raise();
            } else {
                m_pFavoriteTitle->hide();
            }
            m_pFavoriteTitle->setText(Str);
            if (Str != tr("Favorites")) {
                m_pFavoriteTitle->setToolTip(tr("Favorites"));
            } else {
                m_pFavoriteTitle->setToolTip("");
            }
        }
        restoreTitleDisplay();
    } else {
        onBatchSelectChanged(false);
    }

    if (topLevelWidget()->width() <= MAINWINDOW_MIN_WIDTH) {
        m_importByPhoneComboBox->setMinimumWidth(IMPORTBYPHONE_MIN_WIDTH);
        m_importByPhoneComboBox->setMaximumWidth(IMPORTBYPHONE_MAX_WIDTH);
        m_importByPhoneComboBox->setFixedHeight(BTN_FIX_HEIGHT);

        m_importAllByPhoneBtn->setMinimumWidth(IMPORTALLBYPHONE_MIN_WIDTH);
        m_importAllByPhoneBtn->setMaximumWidth(IMPORTALLBYPHONE_MAX_WIDTH);
        m_importAllByPhoneBtn->setFixedHeight(BTN_FIX_HEIGHT);

        m_importSelectByPhoneBtn->setMinimumWidth(IMPORTSELBYPHONE_MIN_WIDTH);
        m_importSelectByPhoneBtn->setMaximumWidth(IMPORTSELBYPHONE_MAX_WIDTH);
        m_importSelectByPhoneBtn->setFixedHeight(BTN_FIX_HEIGHT);
    } else {
        m_importByPhoneComboBox->setMaximumWidth(IMPORTBYPHONE_MAX_WIDTH);
        m_importAllByPhoneBtn->setFixedSize(IMPORTALLBYPHONE_MAX_WIDTH, BTN_FIX_HEIGHT);
        m_importSelectByPhoneBtn->setFixedSize(IMPORTSELBYPHONE_MAX_WIDTH, BTN_FIX_HEIGHT);
    }
}

void AlbumView::adaptiveTrashDescritionLabel()
{
    //然后还要看情况把旁边那个label搞成省略号
    auto descritionText = utils::base::reorganizationStr(m_TrashDescritionLab->font(), m_trashNoticeFullStr, m_TrashTitleLab->x() - 30);
    m_TrashDescritionLab->setText(descritionText);
    if (descritionText != m_trashNoticeFullStr) {
        m_TrashDescritionLab->setToolTip(m_trashNoticeFullStr);
    } else {
        m_TrashDescritionLab->setToolTip("");
    }
}

void AlbumView::onBatchSelectChanged(bool isBatchSelect)
{
    //维语特殊处理
    if (QLocale::system().language() == QLocale::Uighur) {
        if (m_favoriteBatchOperateWidget->isVisible()) {
            if (isBatchSelect) {
                m_pFavoriteTitle->hide();
                m_pFavoritePicTotal->hide();
            } else {
                m_pFavoriteTitle->show();
                m_pFavoriteTitle->raise();
                m_pFavoritePicTotal->show();
            }
        }
        if (m_trashBatchOperateWidget->isVisible()) {
            if (isBatchSelect) {
                m_TrashTitleLab->hide();
                m_TrashDescritionLab->hide();
            } else {
                m_TrashTitleLab->show();
                m_TrashTitleLab->raise();
                m_TrashDescritionLab->show();
            }
        }
        if (m_customBatchOperateWidget->isVisible()) {
            if (isBatchSelect) {
                m_customAlbumTitleLabel->hide();
                m_pRightPicTotal->hide();
            } else {
                m_customAlbumTitleLabel->show();
                m_customAlbumTitleLabel->raise();
                m_pRightPicTotal->show();
            }
        }
        if (m_TrashTitleLab->isVisible() && m_trashBatchOperateWidget->isVisible()) {//最近删除界面标题
            int size = m_trashBatchOperateWidget->x() - (m_TrashTitleLab->x() + m_TrashTitleLab->width());
            QString Str = utils::base::reorganizationStr(m_TrashTitleLab->font(), tr("Trash"), m_TrashTitleLab->width() + size);
            if (Str.length() > 0) {
                m_TrashTitleLab->show();
                m_TrashTitleLab->raise();
            } else {
                m_TrashTitleLab->hide();
            }
            m_TrashTitleLab->setText(Str);
            if (Str != tr("Trash")) {
                m_TrashTitleLab->setToolTip(tr("Trash"));
            } else {
                m_TrashTitleLab->setToolTip("");
            }
        }
        if (m_customAlbumTitleLabel->isVisible() && m_customBatchOperateWidget->isVisible()) {//自定义相册界面标题
            m_customAlbumTitleLabel->setText(m_currentAlbum);
            m_customAlbumTitleLabel->adjustSize();
            m_customAlbumTitleLabel->move(m_customAlbumTitle->width() / 2 - m_customAlbumTitleLabel->width() / 2, 0);
            QString Str = utils::base::reorganizationStr(m_customAlbumTitleLabel->font(), m_currentAlbum,
                                                         2 * m_customBatchOperateWidget->x() - m_customAlbumTitle->width());
            if (Str.length() > 0) {
                m_customAlbumTitleLabel->show();
                m_customAlbumTitleLabel->raise();
            } else {
                m_customAlbumTitleLabel->hide();
            }
            m_customAlbumTitleLabel->setText(Str);
            m_customAlbumTitleLabel->adjustSize();
            m_customAlbumTitleLabel->move(m_customAlbumTitle->width() / 2 - m_customAlbumTitleLabel->width() / 2, 0);
            m_customAlbumTitleLabel->raise();
            if (Str != m_currentAlbum) {
                m_customAlbumTitleLabel->setToolTip(m_currentAlbum);
            } else {
                m_customAlbumTitleLabel->setToolTip("");
            }
        }
        if (m_pFavoriteTitle->isVisible() && m_favoriteBatchOperateWidget->isVisible()) {//我的收藏界面标题
            int size = m_favoriteBatchOperateWidget->x() - (m_pFavoriteTitle->x() + m_pFavoriteTitle->width());
            QString Str = utils::base::reorganizationStr(m_pFavoriteTitle->font(), tr("Favorites"), m_pFavoriteTitle->width() + size);
            if (Str.length() > 0) {
                m_pFavoriteTitle->show();
                m_pFavoriteTitle->raise();
            } else {
                m_pFavoriteTitle->hide();
            }
            m_pFavoriteTitle->setText(Str);
            if (Str != tr("Favorites")) {
                m_pFavoriteTitle->setToolTip(tr("Favorites"));
            } else {
                m_pFavoriteTitle->setToolTip("");
            }
        }

        ///注意：维语处理结束后不会执行后续代码
        return;
    }

    //顶部栏存在时的标题处理
    if (m_trashBatchOperateWidget->isVisible()) {//最近删除界面标题
        int size = m_trashBatchOperateWidget->x() - (m_TrashTitleLab->x() + m_TrashTitleLab->width());
        QString Str = utils::base::reorganizationStr(m_TrashTitleLab->font(), tr("Trash"), m_TrashTitleLab->width() + size);
        if (Str.length() > 0) {
            m_TrashTitleLab->show();
            m_TrashTitleLab->raise();
        } else {
            m_TrashTitleLab->hide();
        }
        m_TrashTitleLab->setText(Str);
        m_TrashTitleLab->adjustSize();
        if (Str != tr("Trash")) {
            m_TrashTitleLab->setToolTip(tr("Trash"));
        } else {
            m_TrashTitleLab->setToolTip("");
        }
    }
    if (m_customBatchOperateWidget->isVisible()) {//自定义相册界面标题
        int size = m_customBatchOperateWidget->x() - (m_customAlbumTitleLabel->x() + m_customAlbumTitleLabel->width());
        QString Str = utils::base::reorganizationStr(m_customAlbumTitleLabel->font(), m_currentAlbum, m_customAlbumTitleLabel->width() + size);
        if (Str.length() > 0) {
            m_customAlbumTitleLabel->show();
            m_customAlbumTitleLabel->raise();
        } else {
            m_customAlbumTitleLabel->hide();
        }
        m_customAlbumTitleLabel->setText(Str);
        m_customAlbumTitleLabel->adjustSize();
        if (Str != m_currentAlbum) {
            m_customAlbumTitleLabel->setToolTip(m_currentAlbum);
        } else {
            m_customAlbumTitleLabel->setToolTip("");
        }
    }
    if (m_favoriteBatchOperateWidget->isVisible()) {//我的收藏界面标题
        int size = m_favoriteBatchOperateWidget->x() - (m_pFavoriteTitle->x() + m_pFavoriteTitle->width());
        QString Str = utils::base::reorganizationStr(m_pFavoriteTitle->font(), tr("Favorites"), m_pFavoriteTitle->width() + size);
        if (Str.length() > 0) {
            m_pFavoriteTitle->show();
            m_pFavoriteTitle->raise();
        } else {
            m_pFavoriteTitle->hide();
        }
        m_pFavoriteTitle->setText(Str);
        if (Str != tr("Favorites")) {
            m_pFavoriteTitle->setToolTip(tr("Favorites"));
        } else {
            m_pFavoriteTitle->setToolTip("");
        }
    }

    restoreTitleDisplay();
}

void AlbumView::resizeEvent(QResizeEvent *e)
{
    adjustTitleContent();
    QWidget::resizeEvent(e);
}

void AlbumView::showEvent(QShowEvent *e)
{
    adjustTitleContent();
    QWidget::showEvent(e);
}
