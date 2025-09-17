// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "albumControl.h"
#include "dbmanager/dbmanager.h"
#include "fileMonitor/fileinotifygroup.h"
#include "imageengine/imageenginethread.h"
#include "utils/devicehelper.h"

#include <DDialog>
#include <DMessageBox>
#include <DGuiApplicationHelper>
#include <DSysInfo>

#include <QStandardPaths>
#include <QFileInfo>
#include <QUrl>
#include <QFileDialog>
#include <QProcess>
#include <QRegularExpression>
#include <QDirIterator>
#include <QCoreApplication>
#include <QFuture>
#include <QtConcurrent>
#include <QApplication>
#include <QDebug>

DWIDGET_USE_NAMESPACE
DGUI_USE_NAMESPACE
DCORE_USE_NAMESPACE

namespace {
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

} //namespace

AlbumControl *AlbumControl::m_instance = nullptr;

QString sizeString(const QString &str)
{
    // qDebug() << "sizeString - Function entry, str:" << str;
    int begin_pos = str.indexOf('.');

    if (begin_pos < 0) {
        // qDebug() << "sizeString - Branch: no decimal point found";
        return str;
    }

    QString size = str;

    while (size.count() - 1 > begin_pos) {
        if (!size.endsWith('0'))
            return size;

        size = size.left(size.count() - 1);
    }

    // qDebug() << "sizeString - Function exit, removed trailing zeros";
    return size.left(size.count() - 1);
}

QString formatSize(qint64 num, bool withUnitVisible = true, int precision = 1, int forceUnit = -1, QStringList unitList = QStringList())
{
    // qDebug() << "formatSize - Function entry, num:" << num << "withUnitVisible:" << withUnitVisible;
    if (num < 0) {
        // qWarning() << "Negative number passed to formatSize():" << num;
        num = 0;
    }

    bool isForceUnit = (forceUnit >= 0);
    QStringList list;
    qreal fileSize(num);

    if (unitList.size() == 0) {
        // qDebug() << "formatSize - Branch: using default unit list";
        list << " B" << " KB" << " MB" << " GB" << " TB"; // should we use KiB since we use 1024 here?
    } else {
        // qDebug() << "formatSize - Branch: using custom unit list";
        list = unitList;
    }

    QStringListIterator i(list);
    QString unit = i.hasNext() ? i.next() : QStringLiteral(" B");

    int index = 0;
    while (i.hasNext()) {
        if (fileSize < 1024 && !isForceUnit) {
            // qDebug() << "formatSize - Branch: file size under 1024 and not forced unit";
            break;
        }

        if (isForceUnit && index == forceUnit) {
            // qDebug() << "formatSize - Branch: reached forced unit index";
            break;
        }

        unit = i.next();
        fileSize /= 1024;
        index++;
    }
    QString unitString = withUnitVisible ? unit : QString();
    // qDebug() << "formatSize - Function exit, final size:" << fileSize << "unit:" << unitString;
    return QString("%1%2").arg(sizeString(QString::number(fileSize, 'f', precision)), unitString);
}

AlbumControl::AlbumControl(QObject *parent)
    : QObject(parent)
{
    qDebug() << "Creating AlbumControl instance";
    initMonitor();
    initDeviceMonitor();

    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::newProcessInstance, this, &AlbumControl::onNewAPPOpen);
    qDebug() << "AlbumControl initialization completed";
}

AlbumControl::~AlbumControl()
{
    // qDebug() << "Destroying AlbumControl instance";
}

AlbumControl *AlbumControl::instance()
{
    // qDebug() << "AlbumControl::instance - Function entry";
    if (!m_instance) {
        // qDebug() << "AlbumControl::instance - Branch: creating new AlbumControl singleton instance";
        m_instance = new AlbumControl();
        m_instance->startMonitor();
    }
    // qDebug() << "AlbumControl::instance - Function exit";
    return m_instance;
}

DBImgInfo AlbumControl::getDBInfo(const QString &srcpath, bool isVideo)
{
    qDebug() << "Getting DB info for path:" << srcpath << "isVideo:" << isVideo;
    using namespace LibUnionImage_NameSpace;
    QFileInfo srcfi(srcpath);
    DBImgInfo dbi;
    dbi.filePath = srcpath;
    dbi.importTime = QDateTime::currentDateTime();
    if (isVideo) {
        //获取视频信息
        qDebug() << "Processing video file";
        MovieInfo movieInfo = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(srcpath));
        if (!movieInfo.valid) {
            qWarning() << "Invalid video file:" << srcpath;
            dbi.itemType = ItemTypeNull;
            return dbi;
        }
        //对视频信息缓存
        m_movieInfos[srcpath] = movieInfo;

        dbi.itemType = ItemTypeVideo;
        dbi.changeTime = srcfi.lastModified();

        if (movieInfo.creation.isValid()) {
            qDebug() << "getDBInfo - Branch: using movie creation time";
            dbi.time = movieInfo.creation;
        } else if (srcfi.birthTime().isValid()) {
            qDebug() << "getDBInfo - Branch: using file birth time";
            dbi.time = srcfi.birthTime();
        } else if (srcfi.metadataChangeTime().isValid()) {
            qDebug() << "getDBInfo - Branch: using metadata change time";
            dbi.time = srcfi.metadataChangeTime();
        } else {
            qDebug() << "getDBInfo - Branch: using default change time";
            dbi.time = dbi.changeTime;
        }
    } else {
        qDebug() << "Processing image file";
        auto mds = getAllMetaData(srcpath);
        QString value = mds.value("DateTimeOriginal");
        dbi.itemType = ItemTypePic;
        dbi.changeTime = QDateTime::fromString(mds.value("DateTimeDigitized"), "yyyy/MM/dd hh:mm");
        if (!value.isEmpty()) {
            qDebug() << "getDBInfo - Branch: using DateTimeOriginal from metadata";
            dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm");
        } else if (srcfi.birthTime().isValid()) {
            qDebug() << "getDBInfo - Branch: using file birth time";
            dbi.time = srcfi.birthTime();
        } else if (srcfi.metadataChangeTime().isValid()) {
            qDebug() << "getDBInfo - Branch: using metadata change time";
            dbi.time = srcfi.metadataChangeTime();
        } else {
            qDebug() << "getDBInfo - Branch: using default change time";
            dbi.time = dbi.changeTime;
        }
    }
    qDebug() << "DB info retrieved successfully for:" << srcpath;
    return dbi;
}

void AlbumControl::initDeviceMonitor()
{
    qDebug() << "Initializing device monitor";
    m_deviceManager = DDeviceManager::instance();
    m_deviceManager->startMonitorWatch();

    DeviceHelper::instance()->loadAllDeviceInfos();
    getAllBlockDeviceName();
    QStringList deviceIds = DeviceHelper::instance()->getAllDeviceIds();
    qDebug() << "Found" << deviceIds.size() << "devices";
    
    for (auto id : deviceIds) {
        QVariantMap deveiceInfo = DeviceHelper::instance()->loadDeviceInfo(id);
        onMounted(id, deveiceInfo.value("MountPoint").toString(), static_cast<DeviceType>(deveiceInfo.value("DeviceType").toInt()));
    }
    
    connect(m_deviceManager, &DDeviceManager::deviceRemoved, this, &AlbumControl::onDeviceRemoved);
    connect(m_deviceManager, &DDeviceManager::mounted, this, &AlbumControl::onMounted);
    connect(m_deviceManager, &DDeviceManager::unmounted, this, &AlbumControl::onUnMounted);
    qDebug() << "Device monitor initialization completed";
}

bool AlbumControl::findPicturePathByPhone(QString &path)
{
    qDebug() << "Searching for picture path in phone directory:" << path;
    QDir dir(path);
    if (!dir.exists()) {
        qWarning() << "Directory does not exist:" << path;
        return false;
    }
    
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfo tempFileInfo;
    foreach (tempFileInfo, fileInfoList) {
        //针对ptp模式
        if (tempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0) {
            path = tempFileInfo.absoluteFilePath();
            qDebug() << "Found picture path:" << path;
            return true;
        } else {        //针对MTP模式
            QDir subDir;
            subDir.setPath(tempFileInfo.absoluteFilePath());
            QFileInfoList subFileInfoList = subDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot);
            for (QFileInfo subTempFileInfo : subFileInfoList) {
                if (subTempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0) {
                    path = subTempFileInfo.absoluteFilePath();
                    qDebug() << "Found picture path in subdirectory:" << path;
                    return true;
                }
            }
        }
    }
    qDebug() << "No picture path found in:" << path;
    return false;
}

void AlbumControl::getAllInfos()
{
    qDebug() << "Getting all image infos";
    m_infoList = DBManager::instance()->getAllInfos();
    qDebug() << "Retrieved" << m_infoList.size() << "image infos";
}

DBImgInfoList AlbumControl::getAllInfosByUID(QString uid)
{
    qDebug() << "AlbumControl::getAllInfosByUID - Function entry, uid:" << uid;
    return DBManager::instance()->getAllInfosByUID(uid);
}

QString AlbumControl::getAllFilters()
{
    qDebug() << "AlbumControl::getAllFilters - Function entry";
    QStringList sList;
    for (const QString &i : LibUnionImage_NameSpace::unionImageSupportFormat())
        sList << ("*." + i);
    //添加视频过滤
    for (const QString &i : LibUnionImage_NameSpace::videoFiletypes())
        sList << "*." + i;
    QString filter = tr("All photos and videos");
    filter.append('(');
    filter.append(sList.join(" "));
    filter.append(')');
    qDebug() << "AlbumControl::getAllFilters - Function exit, filter:" << filter;
    return filter;
}

void AlbumControl::unMountDevice(const QString &devicePath)
{
    qDebug() << "AlbumControl::unMountDevice - Function entry, devicePath:" << devicePath;
    QString deviceId = DeviceHelper::instance()->getDeviceIdByMountPoint(devicePath);
    if (!deviceId.isEmpty() && DeviceHelper::instance()->detachDevice(deviceId)) {
        qDebug() << "AlbumControl::unMountDevice - Branch: device detach initiated, waiting for completion";
        // 等待最多200ms超时
        QEventLoop loop;
        static const int overTime = 200;
        QTimer::singleShot(overTime, &loop, &QEventLoop::quit);
        loop.exec();

        DeviceHelper::instance()->loadAllDeviceInfos();

        // 设备不存在，则卸载成功，否则提示卸载失败
        if (!DeviceHelper::instance()->isExist(deviceId)) {
            qDebug() << "AlbumControl::unMountDevice - Branch: device unmounted successfully";
            m_durlAndNameMap.remove(devicePath);
            m_PhonePicFileMap.remove(devicePath);
        } else {
            qDebug() << "AlbumControl::unMountDevice - Branch: device still exists, showing error dialog";
            DDialog msgbox;
            msgbox.setFixedWidth(400);
            msgbox.setIcon(DMessageBox::standardIcon(DMessageBox::Critical));
            msgbox.setTextFormat(Qt::AutoText);
            msgbox.setMessage(tr("Disk is busy, cannot eject now"));
            msgbox.insertButton(1, tr("OK"), false, DDialog::ButtonNormal);
            auto ret = msgbox.exec();
            Q_UNUSED(ret);
        }
    }

    emit sigMountsChange();
    qDebug() << "AlbumControl::unMountDevice - Function exit";
}

QStringList AlbumControl::getAllUrlPaths(const int &filterType)
{
    qDebug() << "AlbumControl::getAllUrlPaths - Function entry, filterType:" << filterType;
    QStringList pathList;
    ItemType type = ItemType::ItemTypePic;
    switch (filterType) {
    case 0:
        qDebug() << "AlbumControl::getAllUrlPaths - Branch: filter type null";
        type = ItemType::ItemTypeNull;
        break;
    case 1:
        qDebug() << "AlbumControl::getAllUrlPaths - Branch: filter type picture";
        type = ItemType::ItemTypePic;
        break;
    case 2:
        qDebug() << "AlbumControl::getAllUrlPaths - Branch: filter type video";
        type = ItemType::ItemTypeVideo;
        break;
    }
    QStringList list = DBManager::instance()->getAllPaths(type);
    for (QString path : list) {
        pathList << "file://" + path;
    }
    qDebug() << "AlbumControl::getAllUrlPaths - Function exit, returning" << pathList.size() << "paths";
    return pathList;
}

QStringList AlbumControl::getAllPaths(const int &filterType)
{
    qDebug() << "AlbumControl::getAllPaths - Function entry, filterType:" << filterType;
    QStringList pathList;
    ItemType type = ItemType::ItemTypePic;
    switch (filterType) {
    case 0:
        qDebug() << "AlbumControl::getAllPaths - Branch: filter type null";
        type = ItemType::ItemTypeNull;
        break;
    case 1:
        qDebug() << "AlbumControl::getAllPaths - Branch: filter type picture";
        type = ItemType::ItemTypePic;
        break;
    case 2:
        qDebug() << "AlbumControl::getAllPaths - Branch: filter type video";
        type = ItemType::ItemTypeVideo;
        break;
    }

    QStringList list = DBManager::instance()->getAllPaths(type);
    qDebug() << "AlbumControl::getAllPaths - Function exit, returning" << list.size() << "paths";

    return list;
}

QVariantList AlbumControl::getAlbumAllInfos(const int &filterType)
{
    qDebug() << "AlbumControl::getAlbumAllInfos - Function entry, filterType:" << filterType;
    QVariantList reinfoList;
    ItemType type = ItemType::ItemTypePic;
    switch (filterType) {
    case 0:
        qDebug() << "AlbumControl::getAlbumAllInfos - Branch: filter type null";
        type = ItemType::ItemTypeNull;
        break;
    case 1:
        qDebug() << "AlbumControl::getAlbumAllInfos - Branch: filter type picture";
        type = ItemType::ItemTypePic;
        break;
    case 2:
        qDebug() << "AlbumControl::getAlbumAllInfos - Branch: filter type video";
        type = ItemType::ItemTypeVideo;
        break;
    }
    DBImgInfoList infoList = DBManager::instance()->getAllInfosSort(type);
    for (DBImgInfo info : infoList) {
        QVariantMap reMap;
        reMap.insert("url", "file://" + info.filePath);
        reMap.insert("filePath", info.filePath);
        reMap.insert("pathHash", info.pathHash);
        reMap.insert("remainDays", info.remainDays);

        if (info.itemType == ItemTypePic) {
            // qDebug() << "AlbumControl::getAlbumAllInfos - Branch: processing picture item";
            reMap.insert("itemType", "pciture");
        } else if (info.itemType == ItemTypeVideo) {
            // qDebug() << "AlbumControl::getAlbumAllInfos - Branch: processing video item";
            reMap.insert("itemType", "video");
        } else {
            // qDebug() << "AlbumControl::getAlbumAllInfos - Branch: processing other item type";
            reMap.insert("itemType", "other");
        }
        reinfoList << reMap;
        //}
    }
    qDebug() << "AlbumControl::getAlbumAllInfos - Function exit, returning" << reinfoList.size() << "items";
    return reinfoList;
}

bool AlbumControl::importAllImagesAndVideos(const QStringList &paths, const int UID, const bool notifyUI)
{
    qDebug() << "AlbumControl::importAllImagesAndVideos - Function entry, paths count:" << paths.size() << "UID:" << UID << "notifyUI:" << notifyUI;
    //发送导入开始信号
    if (notifyUI) {
        qDebug() << "AlbumControl::importAllImagesAndVideos - Branch: emitting import start signal";
        emit sigImportStart();
    }

    ImportImagesThread *imagesthread = new ImportImagesThread;
    imagesthread->setData(paths, UID);
    imagesthread->setNotifyUI(notifyUI);
    QThreadPool::globalInstance()->start(imagesthread);

    qDebug() << "AlbumControl::importAllImagesAndVideos - Function exit, returning true";
    return true;
}

bool AlbumControl::importAllImagesAndVideosUrl(const QList<QUrl> &paths, const int UID, bool checkRepeat/* = true*/)
{
    qDebug() << "AlbumControl::importAllImagesAndVideosUrl - Function entry, paths count:" << paths.size() << "UID:" << UID << "checkRepeat:" << checkRepeat;
    //发送导入开始信号
    emit sigImportStart();

    ImportImagesThread *imagesthread = new ImportImagesThread;
    imagesthread->setData(paths, UID, checkRepeat);
    QThreadPool::globalInstance()->start(imagesthread);

    qDebug() << "AlbumControl::importAllImagesAndVideosUrl - Function exit, returning true";
    return true;
}

QStringList AlbumControl::getAllTimelinesTitle(const int &filterType)
{
    qDebug() << "AlbumControl::getAllTimelinesTitle - Function entry, filterType:" << filterType;
    return getTimelinesTitle(TimeLineEnum::All, filterType);
}

QStringList AlbumControl::getTimelinesTitlePaths(const QString &titleName, const int &filterType)
{
    qDebug() << "AlbumControl::getTimelinesTitlePaths - Function entry, titleName:" << titleName << "filterType:" << filterType;
    QStringList pathsList;
    DBImgInfoList dblist;
    if (m_yearDateMap.keys().contains(titleName)) {
        qDebug() << "AlbumControl::getTimelinesTitlePaths - Branch: found in year date map";
        dblist = m_yearDateMap.value(titleName);
    } else if (m_monthDateMap.keys().contains(titleName)) {
        qDebug() << "AlbumControl::getTimelinesTitlePaths - Branch: found in month date map";
        dblist = m_monthDateMap.value(titleName);
    } else if (m_dayDateMap.keys().contains(titleName)) {
        qDebug() << "AlbumControl::getTimelinesTitlePaths - Branch: found in day date map";
        dblist = m_dayDateMap.value(titleName);
    } else {
        qDebug() << "AlbumControl::getTimelinesTitlePaths - Branch: found in import timeline paths map";
        dblist = m_importTimeLinePathsMap.value(titleName);
    }
    for (DBImgInfo info : dblist) {
        if (filterType == 2 && info.itemType == ItemTypePic) {
            // qDebug() << "AlbumControl::getTimelinesTitlePaths - Branch: skipping picture due to video filter";
            continue ;
        } else if (filterType == 1 && info.itemType == ItemTypeVideo) {
            // qDebug() << "AlbumControl::getTimelinesTitlePaths - Branch: skipping video due to picture filter";
            continue ;
        }
        pathsList << "file://" + info.filePath;
    }
    qDebug() << "AlbumControl::getTimelinesTitlePaths - Function exit, returning" << pathsList.size() << "paths";
    return pathsList;
}

QStringList AlbumControl::getAllImportTimelinesTitle(const int &filterType)
{
    qDebug() << "AlbumControl::getAllImportTimelinesTitle - Function entry, filterType:" << filterType;
    return getTimelinesTitle(TimeLineEnum::Import, filterType);
}

QVariantMap AlbumControl::getTimelinesTitleInfos(const int &filterType)
{
    qDebug() << "AlbumControl::getTimelinesTitleInfos - Function entry, filterType:" << filterType;
    QVariantMap reMap;
    QStringList alltitles =  getTimelinesTitle(TimeLineEnum::All, filterType);
    for (QString titleName : alltitles) {
        QVariantList list;
        DBImgInfoList dbInfoList = m_timeLinePathsMap.value(titleName);
        for (DBImgInfo info : dbInfoList) {
            QVariantMap tmpMap;
            if (info.itemType == ItemTypePic) {
                if (filterType == 2) {
                    // qDebug() << "AlbumControl::getTimelinesTitleInfos - Branch: skipping picture due to video filter";
                    continue ;
                }
                tmpMap.insert("itemType", "pciture");
            } else if (info.itemType == ItemTypeVideo) {
                if (filterType == 1) {
                    // qDebug() << "AlbumControl::getTimelinesTitleInfos - Branch: skipping video due to picture filter";
                    continue ;
                }
                tmpMap.insert("itemType", "video");
            } else {
                tmpMap.insert("itemType", "other");
            }
            tmpMap.insert("url", "file://" + info.filePath);
            tmpMap.insert("filePath", info.filePath);
            tmpMap.insert("pathHash", info.pathHash);
            tmpMap.insert("remainDays", info.remainDays);
            list << tmpMap;
        }
        if (list.count() > 0) {
            // qDebug() << "AlbumControl::getTimelinesTitleInfos - Branch: adding title" << titleName << "with" << list.count() << "items";
            reMap.insert(titleName, list);
        }
    }
    qDebug() << "AlbumControl::getTimelinesTitleInfos - Function exit, returning" << reMap.size() << "title groups";
    return reMap;
}


QStringList AlbumControl::getYearTimelinesTitle(const int &filterType)
{
    qDebug() << "AlbumControl::getYearTimelinesTitle - Function entry, filterType:" << filterType;
    return getTimelinesTitle(TimeLineEnum::Year, filterType);
}

QVariantMap AlbumControl::getYearTimelinesInfos(const int &filterType)
{
    qDebug() << "AlbumControl::getYearTimelinesInfos - Function entry, filterType:" << filterType;
    QVariantMap reMap;
    QStringList alltitles =  getTimelinesTitle(TimeLineEnum::Year, filterType);
    for (QString titleName : alltitles) {
        QVariantList list;
        DBImgInfoList dbInfoList = m_yearDateMap.value(titleName);
        for (DBImgInfo info : dbInfoList) {
            QVariantMap tmpMap;
            if (info.itemType == ItemTypePic) {
                if (filterType == 2) {
                    continue ;
                }
                tmpMap.insert("itemType", "pciture");
            } else if (info.itemType == ItemTypeVideo) {
                if (filterType == 1) {
                    continue ;
                }
                tmpMap.insert("itemType", "video");
            } else {
                tmpMap.insert("itemType", "other");
            }
            tmpMap.insert("url", "file://" + info.filePath);
            tmpMap.insert("filePath", info.filePath);
            tmpMap.insert("pathHash", info.pathHash);
            tmpMap.insert("remainDays", info.remainDays);
            list << tmpMap;
        }
        if (list.count() > 0) {
            reMap.insert(titleName, list);
        }
    }
    qDebug() << "AlbumControl::getYearTimelinesInfos - Function exit, returning" << reMap.size() << "title groups";
    return reMap;
}

QStringList AlbumControl::getMonthTimelinesTitle(const int &filterType)
{
    qDebug() << "AlbumControl::getMonthTimelinesTitle - Function entry, filterType:" << filterType;
    return getTimelinesTitle(TimeLineEnum::Month, filterType);
}

QVariantMap AlbumControl::getMonthTimelinesInfos(const int &filterType)
{
    qDebug() << "AlbumControl::getMonthTimelinesInfos - Function entry, filterType:" << filterType;
    QVariantMap reMap;
    QStringList alltitles =  getTimelinesTitle(TimeLineEnum::Month, filterType);
    for (QString titleName : alltitles) {
        QVariantList list;
        DBImgInfoList dbInfoList = m_monthDateMap.value(titleName);
        for (DBImgInfo info : dbInfoList) {
            QVariantMap tmpMap;
            if (info.itemType == ItemTypePic) {
                if (filterType == 2) {
                    continue ;
                }
                tmpMap.insert("itemType", "pciture");
            } else if (info.itemType == ItemTypeVideo) {
                if (filterType == 1) {
                    continue ;
                }
                tmpMap.insert("itemType", "video");
            } else {
                tmpMap.insert("itemType", "other");
            }
            tmpMap.insert("url", "file://" + info.filePath);
            tmpMap.insert("filePath", info.filePath);
            tmpMap.insert("pathHash", info.pathHash);
            tmpMap.insert("remainDays", info.remainDays);
            list << tmpMap;
        }
        if (list.count() > 0) {
            reMap.insert(titleName, list);
        }
    }
    qDebug() << "AlbumControl::getMonthTimelinesInfos - Function exit, returning" << reMap.size() << "title groups";
    return reMap;
}

QStringList AlbumControl::getDayTimelinesTitle(const int &filterType)
{
    qDebug() << "AlbumControl::getDayTimelinesTitle - Function entry, filterType:" << filterType;
    return getTimelinesTitle(TimeLineEnum::Day, filterType);
}

QVariantMap AlbumControl::getDayTimelinesInfos(const int &filterType)
{
    qDebug() << "AlbumControl::getDayTimelinesInfos - Function entry, filterType:" << filterType;
    QVariantMap reMap;
    QStringList alltitles =  getTimelinesTitle(TimeLineEnum::Day, filterType);
    for (QString titleName : alltitles) {
        QVariantList list;
        DBImgInfoList dbInfoList = m_dayDateMap.value(titleName);
        for (DBImgInfo info : dbInfoList) {
            QVariantMap tmpMap;
            if (info.itemType == ItemTypePic) {
                if (filterType == 2) {
                    continue ;
                }
                tmpMap.insert("itemType", "pciture");
            } else if (info.itemType == ItemTypeVideo) {
                if (filterType == 1) {
                    continue ;
                }
                tmpMap.insert("itemType", "video");
            } else {
                tmpMap.insert("itemType", "other");
            }
            tmpMap.insert("url", "file://" + info.filePath);
            tmpMap.insert("filePath", info.filePath);
            tmpMap.insert("pathHash", info.pathHash);
            tmpMap.insert("remainDays", info.remainDays);
            list << tmpMap;
        }
        if (list.count() > 0) {
            reMap.insert(titleName, list);
        }
    }
    qDebug() << "AlbumControl::getDayTimelinesInfos - Function exit, returning" << reMap.size() << "title groups";
    return reMap;
}

QStringList AlbumControl::getTimelinesTitle(TimeLineEnum timeEnum, const int &filterType)
{
    qDebug() << "AlbumControl::getTimelinesTitle - Function entry, filterType:" << filterType;
    //设置需要查询的图片视频或者是全部
    ItemType typeItem = ItemType::ItemTypeNull;
    if (filterType == 1) {
        qDebug() << "AlbumControl::getTimelinesTitle - Branch: setting typeItem to ItemTypePic";
        typeItem = ItemType::ItemTypePic;
    } else if (filterType == 2) {
        qDebug() << "AlbumControl::getTimelinesTitle - Branch: setting typeItem to ItemTypeVideo";
        typeItem = ItemType::ItemTypeVideo;
    }
    //已导入
    if (timeEnum == Import) {
        qDebug() << "AlbumControl::getTimelinesTitle - Branch: setting timeEnum to Import";
        QStringList list;
        m_importTimelines = DBManager::instance()->getImportTimelines();
        m_importTimeLinePathsMap.clear();
        QList<QDateTime> tmpDateList = m_importTimelines ;

        for (QDateTime time : tmpDateList) {
            //获取当前时间照片
            DBImgInfoList ImgInfoList = DBManager::instance()->getInfosByImportTimeline(time, typeItem);
            QStringList datelist = time.toString("yyyy.MM.dd.hh.mm").split(".");
            //加时间线标题
            QString date;
            if (datelist.count() > 4) {
                if (ImgInfoList.size() > 0) {
                    date = QString(QObject::tr("%1/%2/%3 %4:%5")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]).arg(datelist[3]).arg(datelist[4]);
                    m_importTimeLinePathsMap.insert(date, ImgInfoList);
                }
            }
        }
        //倒序
        QStringList relist;
        for (int i = m_importTimeLinePathsMap.keys().count() - 1 ; i >= 0 ; i--) {
            relist << m_importTimeLinePathsMap.keys().at(i);
        }

        qDebug() << "AlbumControl::getTimelinesTitle - Function exit, returning" << relist.size() << "import timeline titles";
        return relist;
    }

    //时间线
    m_timelines = DBManager::instance()->getAllTimelines();
    QMap < QString, DBImgInfoList > tmpInfoMap;
    QList<QDateTime> tmpDateList = m_timelines ;

    for (QDateTime time : tmpDateList) {
        //获取当前时间照片
        DBImgInfoList ImgInfoList = DBManager::instance()->getInfosByTimeline(time, typeItem);
        QStringList datelist = time.toString("yyyy.MM.dd.hh.mm").split(".");
        //加时间线标题
        //QString date;
        if (datelist.count() > 4) {
            qDebug() << "AlbumControl::getTimelinesTitle - Branch: processing timeline" << time;
            //date = QString(QObject::tr("%1-%2-%3-%4:%5")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]).arg(datelist[3]).arg(datelist[4]);

            switch (timeEnum) {
            case TimeLineEnum::Year :
                qDebug() << "AlbumControl::getTimelinesTitle - Branch: processing year timeline" << time;
                if (ImgInfoList.size() > 0) {
                    tmpInfoMap.insert(QString(QObject::tr("%1").arg(datelist[0])), ImgInfoList);
                }
                m_yearDateMap = tmpInfoMap;
                break;
            case TimeLineEnum::Month :
                qDebug() << "AlbumControl::getTimelinesTitle - Branch: processing month timeline" << time;
                if (ImgInfoList.size() > 0) {
                    tmpInfoMap.insert(QString(QObject::tr("%1/%2").arg(datelist[0]).arg(datelist[1])), ImgInfoList);
                }
                m_monthDateMap = tmpInfoMap;
                break;
            case TimeLineEnum::Day :
                qDebug() << "AlbumControl::getTimelinesTitle - Branch: processing day timeline" << time;
                if (ImgInfoList.size() > 0) {
                    tmpInfoMap.insert(QString(QObject::tr("%1/%2/%3").arg(datelist[0]).arg(datelist[1]).arg(datelist[2])), ImgInfoList);
                }
                m_dayDateMap = tmpInfoMap;
                break;
            case TimeLineEnum::All :
                qDebug() << "AlbumControl::getTimelinesTitle - Branch: processing all timeline" << time;
                if (ImgInfoList.size() > 0) {
                    tmpInfoMap.insert(QString(QObject::tr("%1/%2/%3 %4:%5")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]).arg(datelist[3]).arg(datelist[4]), ImgInfoList);
                }
                m_timeLinePathsMap = tmpInfoMap;
                break;
            default:
                break;
            }
        }
    }
    //倒序
    QStringList relist;
    for (int i = tmpInfoMap.keys().count() - 1 ; i >= 0 ; i--) {
        relist << tmpInfoMap.keys().at(i);
    }

    qDebug() << "AlbumControl::getTimelinesTitle - Function exit, returning" << relist.size() << "timeline titles";
    return relist;
}

void AlbumControl::initMonitor()
{
    qDebug() << "AlbumControl::initMonitor - Function entry";
    m_fileInotifygroup = new FileInotifyGroup(this) ;
    connect(m_fileInotifygroup, &FileInotifyGroup::sigMonitorChanged, this, &AlbumControl::slotMonitorChanged);
    connect(m_fileInotifygroup, &FileInotifyGroup::sigMonitorDestroyed, this, &AlbumControl::slotMonitorDestroyed);
    qDebug() << "AlbumControl::initMonitor - Function exit";
}

void AlbumControl::startMonitor()
{
    qDebug() << "AlbumControl::startMonitor - Function entry";
    //启动路径监控
    auto monitorPathsTuple = DBManager::getDefaultNotifyPaths_group();
    const QList<QStringList> &paths = std::get<0>(monitorPathsTuple);
    const QStringList &albumNames = std::get<1>(monitorPathsTuple);
    const QList<int> &UIDs = std::get<2>(monitorPathsTuple);
    qDebug() << "AlbumControl::startMonitor - Branch: starting watch for" << UIDs.size() << "albums";
    for (int i = 0; i != UIDs.size(); ++i) {
        m_fileInotifygroup->startWatch(paths.at(i), albumNames.at(i), UIDs.at(i));
    }

    QMap <int, QString> customAutoImportUIDAndPaths = DBManager::instance()->getAllCustomAutoImportUIDAndPath();
    for (QString &eachItem : customAutoImportUIDAndPaths) {
        //0.先检查路径是否存在，不存在直接移除
        QFileInfo info(eachItem);
        int uid = customAutoImportUIDAndPaths.key(eachItem);
        if (!info.exists() || !info.isDir()) {
            // qDebug() << "AlbumControl::startMonitor - Branch: removing non-existent path" << eachItem;
            DBManager::instance()->removeCustomAutoImportPath(customAutoImportUIDAndPaths.key(eachItem));
            continue;
        }

        //1.获取原有的路径
        auto originPaths = DBManager::instance()->getPathsByAlbum(uid);

        //2.获取现在的路径
        QFileInfoList infos = LibUnionImage_NameSpace::getImagesAndVideoInfo(eachItem, false);
        QStringList currentPaths;
        std::transform(infos.begin(), infos.end(), std::back_inserter(currentPaths), [](const QFileInfo & info) {
            return info.isSymLink() ? info.readSymLink() : info.absoluteFilePath();
        });

        //3.1获取已不存在的路径
        QFuture<QString> watcher = QtConcurrent::filtered(originPaths, [currentPaths](const QString & path) {
            return !currentPaths.contains(path);
        });
        watcher.waitForFinished();
        QStringList deleteFiles(watcher.results());

        //3.2移除已导入的路径
        auto watcher2 = QtConcurrent::filter(currentPaths, [originPaths](const QString & path) {
            return !originPaths.contains(path);
        });
        watcher2.waitForFinished();


        //4.删除不存在的路径
        if (!deleteFiles.isEmpty()) {
            DBManager::instance()->removeImgInfos(deleteFiles);
        }

        //5.执行导入
        if (!currentPaths.isEmpty()) {
            QStringList urls;
            for (QString path : currentPaths) {
                urls << QUrl::fromLocalFile(path).toString();
            }
            importAllImagesAndVideos(urls, -1, false);
            insertImportIntoAlbum(uid, urls);
        }
    }
    QStringList pathlist = DBManager::instance()->getAllPaths();
    QStringList needDeletes ;
    for (QString path : pathlist) {
        if (!QFileInfo(path).exists()) {
            needDeletes << path;
        }
    }
    DBManager::instance()->removeImgInfos(needDeletes);
    qDebug() << "AlbumControl::startMonitor - Function exit";
}

bool AlbumControl::checkIfNotified(const QString &dirPath)
{
    qDebug() << "AlbumControl::checkIfNotified - Function entry, dirPath:" << dirPath;
    return DBManager::instance()->checkCustomAutoImportPathIsNotified(dirPath);
}

void AlbumControl::slotMonitorChanged(QStringList fileAdd, QStringList fileDelete, QString album, int UID)
{
    qDebug() << "AlbumControl::slotMonitorChanged - Function entry, fileAdd count:" << fileAdd.size() << "fileDelete count:" << fileDelete.size() << "album:" << album << "UID:" << UID;
    //直接删除图片
    DBManager::instance()->removeImgInfos(fileDelete);
    AlbumDBType atype = AlbumDBType::AutoImport;
    DBManager::instance()->insertIntoAlbum(UID, fileAdd, atype);

    DBImgInfoList dbInfos;
    for (QString path : fileAdd) {

        bool bIsVideo = LibUnionImage_NameSpace::isVideo(path);
        DBImgInfo info =  getDBInfo(path, bIsVideo);
        info.albumUID = QString::number(UID);
        dbInfos << info;
    }
    //导入图片数据库ImageTable3
    DBManager::instance()->insertImgInfos(dbInfos);

    emit sigRefreshCustomAlbum(UID);
    emit sigRefreshImportAlbum();
    emit sigRefreshAllCollection();

    qDebug() << "AlbumControl::slotMonitorChanged - Function exit";
}

void AlbumControl::slotMonitorDestroyed(int UID)
{
    qDebug() << "AlbumControl::slotMonitorDestroyed - Function entry, UID:" << UID;
    //文件夹删除
    emit sigDeleteCustomAlbum(UID);
    qDebug() << "AlbumControl::slotMonitorDestroyed - Function exit";
}

void AlbumControl::onDeviceRemoved(const QString &deviceKey, DeviceType type)
{
    qDebug() << "Device removed - Key:" << deviceKey << "Type:" << static_cast<int>(type);
    onUnMountedExecute(deviceKey, type);
}

void AlbumControl::onMounted(const QString &deviceKey, const QString &mountPoint, DeviceType type)
{
    qDebug() << "Device mounted - Key:" << deviceKey << "MountPoint:" << mountPoint << "Type:" << static_cast<int>(type);

    QString uri = deviceKey;
    QString scheme = QUrl(uri).scheme();

    if (DeviceHelper::isSamba(uri)) {
        qWarning() << "Skipping SMB path:" << uri;
        return;
    }

    if ((scheme == "file") ||  //usb device
        (scheme == "gphoto2") ||                //phone photo
        //(scheme == "afc") ||                  //iPhone document
        (scheme == "mtp") ||                    //android file
        deviceKey.startsWith("/org")) {         //deviceId为/org前缀的外接设备路径
        qDebug() << "Device mounted scheme is file or gphoto2 or mtp or deviceId starts with /org";

        const QVariantMap deviceInfo = DeviceHelper::instance()->loadDeviceInfo(uri, true);
        if (deviceInfo.isEmpty() || deviceInfo.value("MountPoint").toString().isEmpty()) {
            qWarning() << "Empty device info for:" << uri;
            return;
        }

        QString label;
        if (static_cast<DeviceType>(deviceInfo.value("DeviceType").toInt()) == DeviceType::kBlockDevice)
            label = deviceInfo.value("IdLabel").toString();
        else if (static_cast<DeviceType>(deviceInfo.value("DeviceType").toInt()) == DeviceType::kProtocolDevice)
            label = deviceInfo.value("DisplayName").toString();
            
        qDebug() << "Device name:" << label << "Scheme type:" << scheme;
        
        if (m_durlAndNameMap.find(mountPoint) != m_durlAndNameMap.end()) {
            qDebug() << "Device already in list:" << mountPoint;
            return;
        }

        QString strPath = mountPoint;
        if (strPath.isEmpty()) {
            qWarning() << "Empty mount point path";
            return;
        }

        QString rename = m_blkPath2DeviceNameMap[mountPoint];
        if (rename.isEmpty()) {
            rename = label;
        }
        //判断路径是否存在
        bool bFind = false;
        QDir dir(strPath);
        if (!dir.exists()) {
            qWarning() << "Mount point directory does not exist:" << strPath;
            bFind = false;
        } else {
            qDebug() << "Mount point directory exists:" << strPath;
            bFind = true;
        }
        //U盘和硬盘挂载都是/media下的，此处判断若path不包含/media/,再调用findPicturePathByPhone函数搜索DCIM文件目录
        if (!strPath.contains("/media/")) {
            qDebug() << "Mount point path does not contain /media/";
            bFind = findPicturePathByPhone(strPath);
        }

        DeviceHelper::instance()->loadAllDeviceInfos();
        //路径存在
        if (bFind) {
            qDebug() << "Adding device to map - Path:" << strPath << "Name:" << rename;
            m_durlAndNameMap[mountPoint] = rename;
            emit sigMountsChange();
            //发送新增
            emit sigAddDevice(strPath);
        }
    }
    qDebug() << "AlbumControl::onMounted - Function exit";
}

void AlbumControl::onUnMounted(const QString &deviceKey, DeviceType type)
{
    qDebug() << QString("deviceKey:%1 DeviceType:%2").arg(deviceKey).arg(static_cast<int>(type));
    onUnMountedExecute(deviceKey, type);
    qDebug() << "AlbumControl::onUnMounted - Function exit";
}

void AlbumControl::onUnMountedExecute(const QString &deviceKey, DeviceType type)
{
    qDebug() << "AlbumControl::onUnMountedExecute - Function entry, deviceKey:" << deviceKey << "type:" << static_cast<int>(type);
    QVariantMap deviceInfo = DeviceHelper::instance()->loadDeviceInfo(deviceKey);
    if (deviceInfo.isEmpty()) {
        qDebug() << "Device info is empty, returning";
        return;
    }

    QString mountPoint = DeviceHelper::instance()->getMountPointByDeviceId(deviceKey);;
    QString strPath = mountPoint;
    if (!strPath.contains("/media/")) {
        qDebug() << "Not media path, finding phone picture path";
        findPicturePathByPhone(strPath);
    }
    if (m_durlAndNameMap.find(strPath) != m_durlAndNameMap.end()) {
        qDebug() << "Removing device from map:" << strPath;
        m_durlAndNameMap.erase(m_durlAndNameMap.find(strPath));
    }

    DeviceHelper::instance()->loadAllDeviceInfos();

    if (m_PhonePicFileMap.contains(strPath)) {
        qDebug() << "Removing phone file map for:" << strPath;
        m_PhonePicFileMap.remove(strPath);
    }

    emit sigMountsChange();
    qDebug() << "AlbumControl::onUnMountedExecute - Function exit";
}

QJsonObject AlbumControl::createShorcutJson()
{
    qDebug() << "AlbumControl::createShorcutJson - Function entry";
    //Translations
    QJsonObject shortcut1;
    shortcut1.insert("name", "Window sizing");
    shortcut1.insert("value", "Ctrl+Alt+F");
    QJsonObject shortcut2;
    shortcut2.insert("name", tr("Fullscreen"));
    shortcut2.insert("value", "F11");
    QJsonObject shortcut3;
    shortcut3.insert("name", tr("Exit fullscreen/slideshow"));
    shortcut3.insert("value", "Esc");
    QJsonObject shortcut4;
    shortcut4.insert("name", "Close application");
    shortcut4.insert("value", "Alt+F4");
    QJsonObject shortcut5;
    shortcut5.insert("name", tr("Help"));
    shortcut5.insert("value", "F1");
    QJsonObject shortcut6;
    shortcut6.insert("name", tr("Display shortcuts"));
    shortcut6.insert("value", "Ctrl+Shift+?");
    QJsonObject shortcut7;
    shortcut7.insert("name", tr("Display in file manager"));
    shortcut7.insert("value", "Alt+D");
    QJsonObject shortcut8;
    shortcut8.insert("name", tr("Slide show"));
    shortcut8.insert("value", "F5");
    QJsonObject shortcut9;
    shortcut9.insert("name", tr("View"));
    shortcut9.insert("value", "Enter");
    QJsonObject shortcut10;
    shortcut10.insert("name", tr("Export photos"));
    shortcut10.insert("value", "Ctrl+E");
    QJsonObject shortcut11;
    shortcut11.insert("name", tr("Import photos/videos"));
    shortcut11.insert("value", "Ctrl+O");
    QJsonObject shortcut12;
    shortcut12.insert("name", tr("Select all"));
    shortcut12.insert("value", "Ctrl+A");
    QJsonObject shortcut13;
    shortcut13.insert("name", tr("Copy"));
    shortcut13.insert("value", "Ctrl+C");
    QJsonObject shortcut14;
    shortcut14.insert("name", tr("Delete"));
    shortcut14.insert("value", "Delete");
    QJsonObject shortcut15;
    shortcut15.insert("name", tr("Photo/Video info"));
    shortcut15.insert("value", "Ctrl+I");
    QJsonObject shortcut16;
    shortcut16.insert("name", tr("Set as wallpaper"));
    shortcut16.insert("value", "Ctrl+F9");
    QJsonObject shortcut17;
    shortcut17.insert("name", tr("Rotate clockwise"));
    shortcut17.insert("value", "Ctrl+R");
    QJsonObject shortcut18;
    shortcut18.insert("name", tr("Rotate counterclockwise"));
    shortcut18.insert("value", "Ctrl+Shift+R");
    QJsonObject shortcut19;
    shortcut19.insert("name", " ");
    shortcut19.insert("value", "  ");
    QJsonObject shortcut20;
    shortcut20.insert("name", tr("Zoom in"));
    shortcut20.insert("value", "Ctrl+'+'");
    QJsonObject shortcut21;
    shortcut21.insert("name", tr("Zoom out"));
    shortcut21.insert("value", "Ctrl+'-'");
    QJsonObject shortcut22;
    shortcut22.insert("name", tr("Previous"));
    shortcut22.insert("value", "Left");
    QJsonObject shortcut23;
    shortcut23.insert("name", tr("Next"));
    shortcut23.insert("value", "Right");
    QJsonObject shortcut24;
    shortcut24.insert("name", tr("Favorite"));
    shortcut24.insert("value", ".");
    QJsonObject shortcut25;
    shortcut25.insert("name", tr("Unfavorite"));
    shortcut25.insert("value", ".");
    QJsonObject shortcut26;
    shortcut26.insert("name", tr("New album"));
    shortcut26.insert("value", "Ctrl+Shift+N");
    QJsonObject shortcut27;
    shortcut27.insert("name", tr("Rename album"));
    shortcut27.insert("value", "F2");
    QJsonObject shortcut28;
    shortcut28.insert("name", tr("Page up"));
    shortcut28.insert("value", "PageUp");
    QJsonObject shortcut29;
    shortcut29.insert("name", tr("Page down"));
    shortcut29.insert("value", "PageDown");



    QJsonArray shortcutArray1;
    shortcutArray1.append(shortcut2);
    shortcutArray1.append(shortcut8);
    shortcutArray1.append(shortcut3);
    shortcutArray1.append(shortcut9);
    shortcutArray1.append(shortcut10);
    shortcutArray1.append(shortcut11);
    shortcutArray1.append(shortcut12);
    shortcutArray1.append(shortcut13);
    shortcutArray1.append(shortcut14);
    shortcutArray1.append(shortcut15);
    shortcutArray1.append(shortcut16);
    shortcutArray1.append(shortcut17);
    shortcutArray1.append(shortcut18);
    shortcutArray1.append(shortcut7);
//    shortcutArray1.append(shortcut19);
    shortcutArray1.append(shortcut20);
    shortcutArray1.append(shortcut21);
    shortcutArray1.append(shortcut28);
    shortcutArray1.append(shortcut29);
    shortcutArray1.append(shortcut22);
    shortcutArray1.append(shortcut23);
    shortcutArray1.append(shortcut24);
    shortcutArray1.append(shortcut25);
    QJsonArray shortcutArray2;
    shortcutArray2.append(shortcut26);
    shortcutArray2.append(shortcut27);
    QJsonArray shortcutArray3;
    shortcutArray3.append(shortcut5);
    shortcutArray3.append(shortcut6);

//    shortcutArray.append(shortcut1);
//    shortcutArray.append(shortcut4);

    QJsonObject shortcut_group1;
//    shortcut_group.insert("groupName", tr("热键"));
//    shortcut_group.insert("groupName", tr("Hotkey"));
    shortcut_group1.insert("groupName", tr("Photos"));
    shortcut_group1.insert("groupItems", shortcutArray1);
    QJsonObject shortcut_group2;
    shortcut_group2.insert("groupName", tr("Albums"));
    shortcut_group2.insert("groupItems", shortcutArray2);
    QJsonObject shortcut_group3;
    shortcut_group3.insert("groupName", tr("Settings"));
    shortcut_group3.insert("groupItems", shortcutArray3);

    QJsonArray shortcutArrayall;
    shortcutArrayall.append(shortcut_group1);
    shortcutArrayall.append(shortcut_group2);
    shortcutArrayall.append(shortcut_group3);

    QJsonObject main_shortcut;
    main_shortcut.insert("shortcut", shortcutArrayall);

    qDebug() << "AlbumControl::createShorcutJson - Function exit, returning shortcuts JSON";
    return main_shortcut;
}

void AlbumControl::getAllBlockDeviceName()
{
    qDebug() << "AlbumControl::getAllBlockDeviceName - Function entry";
    m_blkPath2DeviceNameMap.clear();
    QStringList blDevList = DeviceHelper::instance()->getBlockDeviceIds();
    for (const QString &blks : blDevList) {
        updateBlockDeviceName(blks);
    }
    qDebug() << "AlbumControl::getAllBlockDeviceName - Function exit, processed" << blDevList.size() << "devices";
}

void AlbumControl::updateBlockDeviceName(const QString &blks)
{
    qDebug() << "AlbumControl::updateBlockDeviceName - Function entry, blks:" << blks;
    const QVariantMap deviceInfo = DeviceHelper::instance()->loadDeviceInfo(blks);
    if (deviceInfo.isEmpty()) {
        qDebug() << "AlbumControl::updateBlockDeviceName - Branch: device info is empty, returning";
        return;
    }

    QStringList mps = deviceInfo.value("MountPoints").toStringList();
    qulonglong size = deviceInfo.value("SizeTotal").toULongLong();
    QString label = deviceInfo.value("IdLabel").toString();
    QString fs = deviceInfo.value("IdType").toString();
    QString udispname = "";
    if (label.startsWith(ddeI18nSym)) {
        qDebug() << "Label starts with ddeI18nSym";
        QString i18nKey = label.mid(ddeI18nSym.size(), label.size() - ddeI18nSym.size());
        udispname = qApp->translate("DeepinStorage", i18nMap.value(i18nKey, i18nKey.toUtf8().constData()));
        goto runend;
    }

    if (mps.contains(QByteArray("/\0", 2))) {
        qDebug() << "Mount points contain /\\0";
        udispname = QCoreApplication::translate("PathManager", "System Disk");
        goto runend;
    }
    if (label.length() == 0) {
        qDebug() << "Label length is 0";
        bool bMediaAvailable = deviceInfo.value("MediaAvailable").toBool();
        bool bOpticalDrive = deviceInfo.value("OpticalDrive").toBool();
        bool bOpticalBlank = deviceInfo.value("OpticalBlank").toBool();
        bool bIsEncrypted = deviceInfo.value("IsEncrypted").toBool();
        QString media = deviceInfo.value("Media").toString();
        QStringList mediaCompatibility = deviceInfo.value("MediaCompatibility").toStringList();
        if (!bMediaAvailable && bOpticalDrive) {
            qDebug() << "Media available is false and optical drive is true";
            QString maxmediacompat;
            for (auto i = opticalmediakv.rbegin(); i != opticalmediakv.rend(); ++i) {
                if (mediaCompatibility.contains(i->first)) {
                    maxmediacompat = i->second;
                    break;
                }
            }
            udispname = QCoreApplication::translate("DeepinStorage", "%1 Drive").arg(maxmediacompat);
            goto runend;
        }
        if (bOpticalBlank) {
            qDebug() << "Optical blank is true";
            udispname = QCoreApplication::translate("DeepinStorage", "Blank %1 Disc").arg(opticalmediamap[media]);
            goto runend;
        }
        if (bIsEncrypted) {
            qDebug() << "Encrypted is true";
            udispname = QCoreApplication::translate("DeepinStorage", "%1 Encrypted").arg(formatSize(qint64(size)));
            goto runend;
        }
        udispname = QCoreApplication::translate("DeepinStorage", "%1 Volume").arg(formatSize(qint64(size)));
        goto runend;
    }
    udispname = label;

runend:
    //blk->mount({});
    QString strPath = "";
    QStringList qbl = deviceInfo.value("MountPoints").toStringList();
    QString mountPoint = "";
    QList<QString>::iterator qb = qbl.begin();
    while (qb != qbl.end()) {
        mountPoint += (*qb);
        ++qb;
    }
    if (!mountPoint.isEmpty()) {
        m_blkPath2DeviceNameMap[mountPoint] = udispname;
        qDebug() << QString("blks:%1 mountPoint:%2 udispname:%3").arg(blks).arg(mountPoint).arg(udispname);
    }
    qDebug() << "AlbumControl::updateBlockDeviceName - Function exit";
    return;
}

bool AlbumControl::isSystemAutoImportAlbum(int uid)
{
    qDebug() << "AlbumControl::isSystemAutoImportAlbum - Function entry, uid:" << uid;
    return getAllSystemAutoImportAlbumId().contains(uid);
}

bool AlbumControl::isNormalAutoImportAlbum(int uid)
{
    qDebug() << "AlbumControl::isNormalAutoImportAlbum - Function entry, uid:" << uid;
    return getAllNormlAutoImportAlbumId().contains(uid);
}

bool AlbumControl::isAutoImportAlbum(int uid)
{
    qDebug() << "AlbumControl::isAutoImportAlbum - Function entry, uid:" << uid;
    return getAllAutoImportAlbumId().contains(uid);
}

bool AlbumControl::isCustomAlbum(int uid)
{
    qDebug() << "AlbumControl::isCustomAlbum - Function entry, uid:" << uid;
    bool bCustom = getAllCustomAlbumId().contains(uid);
    qDebug() << "AlbumControl::isCustomAlbum - Function exit, returning:" << bCustom;
    return bCustom;
}

bool AlbumControl::isDefaultPathExists(int uid)
{
    qDebug() << "AlbumControl::isDefaultPathExists - Function entry, uid:" << uid;
    return DBManager::defaultNotifyPathExists(uid);
}

void AlbumControl::ctrlShiftSlashShortcut(int x, int y, int w, int h)
{
    qDebug() << "AlbumControl::ctrlShiftSlashShortcut - Function entry, x:" << x << "y:" << y << "w:" << w << "h:" << h;
    QRect rect = QRect(x, y, w, h);
    QPoint pos(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
    QStringList shortcutString;
    QJsonObject json = createShorcutJson();

    QString param1 = "-j=" + QString(QJsonDocument(json).toJson());
    QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());
    shortcutString << param1 << param2;

    QProcess *shortcutViewProcess = new QProcess(this);
    shortcutViewProcess->startDetached("deepin-shortcut-viewer", shortcutString);

    connect(shortcutViewProcess, SIGNAL(finished(int)), shortcutViewProcess, SLOT(deleteLater()));
    qDebug() << "AlbumControl::ctrlShiftSlashShortcut - Function exit";
}

QRect AlbumControl::rect(QPoint p1, QPoint p2)
{
    qDebug() << "AlbumControl::rect - Function entry, p1:" << p1 << "p2:" << p2;
    QRect rt = QRect(p1, p2);
    qDebug() << "AlbumControl::rect - Function exit, returning rect:" << rt;
    return rt;
}

QRect AlbumControl::intersected(QRect r1, QRect r2)
{
    qDebug() << "AlbumControl::intersected - Function entry, r1:" << r1 << "r2:" << r2;
    return r1.intersected(r2);
}

int AlbumControl::manhattanLength(QPoint p1, QPoint p2)
{
    qDebug() << "AlbumControl::manhattanLength - Function entry, p1:" << p1 << "p2:" << p2;
    QPoint point(p1 - p2);
    return point.manhattanLength();
}

QString AlbumControl::url2localPath(QUrl url)
{
    qDebug() << "AlbumControl::url2localPath - Function entry, url:" << url;
    return LibUnionImage_NameSpace::localPath(url);
}

QStringList AlbumControl::urls2localPaths(QStringList urls)
{
    qDebug() << "AlbumControl::urls2localPaths - Function entry, urls count:" << urls.size();
    QStringList paths;
    for (auto url : urls) {
        paths << LibUnionImage_NameSpace::localPath(url);
    }

    qDebug() << "AlbumControl::urls2localPaths - Function exit, returning" << paths.size() << "paths";
    return paths;
}

bool AlbumControl::checkRepeatUrls(QStringList imported, QStringList urls, bool bNotify)
{
    qDebug() << "AlbumControl::checkRepeatUrls - Function entry, imported count:" << imported.size() << "urls count:" << urls.size() << "bNotify:" << bNotify;
    bool bRet = false;
    int noReadCount = 0; //记录已存在于相册中的数量，若全部存在，则不进行导入操作
    for (QString url : urls) {
        QFileInfo srcfi(url2localPath(url));
        if (!srcfi.exists()) {  //当前文件不存在
            // qDebug() << "AlbumControl::checkRepeatUrls - Branch: file does not exist:" << url;
            noReadCount++;
            continue;
        }
        if (imported.contains(url)) {
            // qDebug() << "AlbumControl::checkRepeatUrls - Branch: file already imported:" << url;
            noReadCount++;
        }
    }

    // 已全部存在
    if (noReadCount == urls.size()) {
        qDebug() << "AlbumControl::checkRepeatUrls - Branch: all files already exist or imported";
        if (bNotify)
            emit sigRepeatUrls(urls);
        bRet = true;
    }

    qDebug() << "AlbumControl::checkRepeatUrls - Function exit, returning:" << bRet;
    return bRet;
}

QStringList AlbumControl::getImportTimelinesTitlePaths(const QString &titleName, const int &filterType)
{
    qDebug() << "AlbumControl::getImportTimelinesTitlePaths - Function entry, titleName:" << titleName << "filterType:" << filterType;
    QStringList pathsList;
    DBImgInfoList dbInfoList = m_importTimeLinePathsMap.value(titleName);
    for (DBImgInfo info : dbInfoList) {
        if (filterType == 2 && info.itemType == ItemTypePic) {
            // qDebug() << "AlbumControl::getImportTimelinesTitlePaths - Branch: skipping picture due to video filter";
            continue ;
        } else if (filterType == 1 && info.itemType == ItemTypeVideo) {
            // qDebug() << "AlbumControl::getImportTimelinesTitlePaths - Branch: skipping video due to picture filter";
            continue ;
        }
        pathsList << "file://" + info.filePath;
    }
    qDebug() << "AlbumControl::getImportTimelinesTitlePaths - Function exit, returning" << pathsList.size() << "paths";
    return pathsList;
}

QVariantMap AlbumControl::getImportTimelinesTitleInfos(const int &filterType)
{
    qDebug() << "AlbumControl::getImportTimelinesTitleInfos - Function entry, filterType:" << filterType;
    QVariantMap reMap;
    QStringList alltitles = getAllImportTimelinesTitle(filterType);
    for (QString titleName : alltitles) {
        QVariantList list;
        DBImgInfoList dbInfoList = m_importTimeLinePathsMap.value(titleName);
        for (DBImgInfo info : dbInfoList) {
            QVariantMap tmpMap;
            if (info.itemType == ItemTypePic) {
                if (filterType == 2) {
                    // qDebug() << "AlbumControl::getImportTimelinesTitleInfos - Branch: skipping picture due to video filter";
                    continue ;
                }
                tmpMap.insert("itemType", "pciture");
            } else if (info.itemType == ItemTypeVideo) {
                if (filterType == 1) {
                    // qDebug() << "AlbumControl::getImportTimelinesTitleInfos - Branch: skipping video due to picture filter";
                    continue ;
                }
                tmpMap.insert("itemType", "video");
            } else {
                tmpMap.insert("itemType", "other");
            }
            tmpMap.insert("url", "file://" + info.filePath);
            tmpMap.insert("filePath", info.filePath);
            tmpMap.insert("pathHash", info.pathHash);
            tmpMap.insert("remainDays", info.remainDays);
            list << tmpMap;
        }
        if (list.count() > 0) {
            qDebug() << "AlbumControl::getImportTimelinesTitleInfos - Branch: adding title" << titleName << "with" << list.count() << "items";
            reMap.insert(titleName, list);
        }
    }

    qDebug() << "AlbumControl::getImportTimelinesTitleInfos - Function exit, returning" << reMap.size() << "title groups";
    return reMap;
}

QVariantList AlbumControl::getImportTimelinesTitleInfosReverse(const int &filterType)
{
    qDebug() << "AlbumControl::getImportTimelinesTitleInfosReverse - Function entry, filterType:" << filterType;
    QVariantMap reMap = getImportTimelinesTitleInfos(filterType);

    QVariantList reList;
    if (reMap.size()) {
        qDebug() << "AlbumControl::getImportTimelinesTitleInfosReverse - Branch: reversing" << reMap.size() << "title groups";
        for (auto it = --reMap.end(); it != --reMap.begin(); it--) {
            QVariantMap tmpMap;
            tmpMap.insert(it.key(), it.value());
            reList.push_back(tmpMap);
        }
    }

    qDebug() << "AlbumControl::getImportTimelinesTitleInfosReverse - Function exit, returning" << reList.size() << "items";
    return reList;
}

QVariantMap AlbumControl::getAlbumInfos(const int &albumId, const int &filterType)
{
    qDebug() << "AlbumControl::getImportTimelinesTitleInfosReverse - Function entry, filterType:" << filterType;
    QVariantMap reMap;

    QVariantList list;
    DBImgInfoList dbInfoList = DBManager::instance()->getInfosByAlbum(albumId, false);
    QString title = DBManager::instance()->getAlbumNameFromUID(albumId);
    for (DBImgInfo info : dbInfoList) {
        QVariantMap tmpMap;
        if (info.itemType == ItemTypePic) {
            if (filterType == 2) {
                continue ;
            }
            tmpMap.insert("itemType", "pciture");
        } else if (info.itemType == ItemTypeVideo) {
            if (filterType == 1) {
                continue ;
            }
            tmpMap.insert("itemType", "video");
        } else {
            tmpMap.insert("itemType", "other");
        }
        tmpMap.insert("url", "file://" + info.filePath);
        tmpMap.insert("filePath", info.filePath);
        tmpMap.insert("pathHash", info.pathHash);
        tmpMap.insert("remainDays", info.remainDays);
        list << tmpMap;
    }
    if (list.count() > 0) {
        qDebug() << "AlbumControl::getImportTimelinesTitleInfosReverse - Branch: adding title" << title << "with" << list.count() << "items";
        reMap.insert(title, list);
    }
    qDebug() << "AlbumControl::getImportTimelinesTitleInfosReverse - Function exit, returning" << reMap.size() << "title groups";
    return reMap;
}

QVariantMap AlbumControl::getTrashAlbumInfos(const int &filterType)
{
    qDebug() << "AlbumControl::getTrashAlbumInfos - Function entry, filterType:" << filterType;
    QVariantMap reMap;

    QVariantList list;
    DBImgInfoList dbInfoList = getTrashInfos(filterType);
    QString title = QObject::tr("Trash");
    for (DBImgInfo info : dbInfoList) {
        QVariantMap tmpMap;
        if (info.itemType == ItemTypePic) {
            if (filterType == 2) {
                continue ;
            }
            tmpMap.insert("itemType", "pciture");
        } else if (info.itemType == ItemTypeVideo) {
            if (filterType == 1) {
                continue ;
            }
            tmpMap.insert("itemType", "video");
        } else {
            tmpMap.insert("itemType", "other");
        }

        //设置url为删除的路径
        QString realPath = getDeleteFullPath(LibUnionImage_NameSpace::hashByString(info.filePath), DBImgInfo::getFileNameFromFilePath(info.filePath));
        tmpMap.insert("url", "file://" + realPath);
        tmpMap.insert("filePath", "file://" + info.filePath);
        tmpMap.insert("pathHash", info.pathHash);
        tmpMap.insert("remainDays", info.remainDays);
        list << tmpMap;
    }
    if (list.count() > 0) {
        qDebug() << "AlbumControl::getTrashAlbumInfos - Branch: adding title" << title << "with" << list.count() << "items";
        reMap.insert(title, list);
    }
    qDebug() << "AlbumControl::getTrashAlbumInfos - Function exit, returning" << reMap.size() << "title groups";
    return reMap;
}

bool AlbumControl::addCustomAlbumInfos(int albumId, const QList<QUrl> &urls)
{
    qDebug() << "AlbumControl::addCustomAlbumInfos - Function entry, albumId:" << albumId << "urls count:" << urls.size();
    QStringList localpaths;
    for (QUrl path : urls) {
        localpaths << url2localPath(path);
    }
    QStringList curAlbumImgPathList = getAllUrlPaths();
    for (QString imagePath : localpaths) {
        if (QDir(imagePath).exists()) {
            // qDebug() << "AlbumControl::addCustomAlbumInfos - Branch: directory exists:" << imagePath;
            //获取所选文件类型过滤器
            QStringList filters;
            for (QString i : LibUnionImage_NameSpace::unionImageSupportFormat()) {
                filters << "*." + i;
            }

            for (QString i : LibUnionImage_NameSpace::videoFiletypes()) {
                filters << "*." + i;
            }
            //定义迭代器并设置过滤器，包括子目录：QDirIterator::Subdirectories
            QDirIterator dir_iterator(imagePath,
                                      filters,
                                      QDir::Files/* | QDir::NoSymLinks*/,
                                      QDirIterator::Subdirectories);
            QList<QUrl> allfiles;
            while (dir_iterator.hasNext()) {
                dir_iterator.next();
                QFileInfo fileInfo = dir_iterator.fileInfo();
                allfiles << "file://" + fileInfo.filePath();
            }
            if (!allfiles.isEmpty()) {
                addCustomAlbumInfos(albumId, allfiles);
            }
        }
    }
    bool bRet = false;
    QStringList paths;
    for (QUrl url : urls) {
        paths << url2localPath(url);
    }
    AlbumDBType atype;
    if (albumId == 0) {
        atype = Favourite;
    } else if (albumId < 4) {
        atype = AutoImport;
    } else {
        atype = Custom;
    }
    bRet = DBManager::instance()->insertIntoAlbum(albumId, paths, atype);
    emit sigRefreshCustomAlbum(albumId);
    qDebug() << "AlbumControl::addCustomAlbumInfos - Function exit, returning" << bRet;
    return bRet;
}

int AlbumControl::getAllCount(const int &filterType)
{
    qDebug() << "AlbumControl::getAllCount - Function entry, filterType:" << filterType;
    ItemType typeItem = ItemType::ItemTypeNull;
    if (filterType == 1) {
        qDebug() << "AlbumControl::getAllCount - Branch: filter type picture";
        typeItem = ItemType::ItemTypePic;
    } else if (filterType == 2) {
        qDebug() << "AlbumControl::getAllCount - Branch: filter type video";
        typeItem = ItemType::ItemTypeVideo;
    }
    int nCount = DBManager::instance()->getImgsCount(typeItem);
    qDebug() << "AlbumControl::getAllCount - Function exit, returning count:" << nCount;
    return nCount;
}

void AlbumControl::insertTrash(const QList<QUrl> &paths)
{
    qDebug() << "AlbumControl::insertTrash - Function entry, paths count:" << paths.size();
    // notify show progress start
    emit sigDeleteProgress(0, paths.size());
    // 先处理一下UI事件,否则进度条不显示
    QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50);
    QStringList tmpList;
    DBImgInfoList infos;
    for (const QUrl &url : paths) {
        QString imagePath = url2localPath(url);
        QFileInfo info(imagePath);
        
        // 跳过不可写文件
        if (!info.isWritable()) {
            // qDebug() << "AlbumControl::insertTrash - Branch: skipping non-writable file:" << imagePath;
            continue;
        }
        
        tmpList << imagePath;
        
        DBImgInfoList tempInfos = DBManager::instance()->getInfosByPath(imagePath);
        if (tempInfos.size()) {
            // qDebug() << "AlbumControl::insertTrash - Branch: found" << tempInfos.size() << "database entries for:" << imagePath;
            DBImgInfo insertInfo = tempInfos.first();
            QStringList uids;
            for (const DBImgInfo &dbInfo : tempInfos) {
                uids.push_back(dbInfo.albumUID);
            }
            
            insertInfo.albumUID = uids.join(",");
            infos << insertInfo;
        }
    }

    DBManager::instance()->insertTrashImgInfos(infos, true);

    // notify show progress end
    int count = tmpList.size();
    emit sigDeleteProgress(count + 1, count);

    //新增删除主相册数据库
    DBManager::instance()->removeImgInfos(tmpList);
    // 通知前端刷新相关界面，包括自定义相册/我的收藏/合集-所有项目/已导入
    sigRefreshCustomAlbum(-1);
    sigRefreshAllCollection();
    sigRefreshImportAlbum();
    sigRefreshSearchView();
    qDebug() << "AlbumControl::insertTrash - Function exit";
}

void AlbumControl::removeTrashImgInfos(const QList< QUrl > &paths)
{
    qDebug() << "AlbumControl::removeTrashImgInfos - Function entry, paths count:" << paths.size();
    QStringList localPaths ;
    for (QUrl path : paths) {
        localPaths << url2localPath(path);
    }
    DBManager::instance()->removeTrashImgInfos(localPaths);
    qDebug() << "AlbumControl::removeTrashImgInfos - Function exit";
}

QStringList AlbumControl::recoveryImgFromTrash(const QStringList &paths)
{
    qDebug() << "AlbumControl::recoveryImgFromTrash - Function entry, paths count:" << paths.size();
    QStringList localPaths;
    for (QUrl path : paths) {
        if (path.isLocalFile()) {
            // qDebug() << "AlbumControl::recoveryImgFromTrash - Branch: processing local file:" << path;
            localPaths << url2localPath(path);
        } else {
            // qDebug() << "AlbumControl::recoveryImgFromTrash - Branch: processing non-local URL:" << path;
            localPaths << path.toString();
        }
    }
    qDebug() << "AlbumControl::recoveryImgFromTrash - Function exit, returning" << localPaths.size() << "paths";
    return DBManager::instance()->recoveryImgFromTrash(localPaths);
}

void AlbumControl::deleteImgFromTrash(const QStringList &paths)
{
    qDebug() << "AlbumControl::deleteImgFromTrash - Function entry, paths count:" << paths.size();
    QStringList localPaths ;
    for (QUrl path : paths) {
        if (path.isLocalFile()) {
            // qDebug() << "AlbumControl::deleteImgFromTrash - Branch: processing local file:" << path;
            localPaths << url2localPath(path);
        } else {
            // qDebug() << "AlbumControl::deleteImgFromTrash - Branch: processing non-local URL:" << path;
            localPaths << path.toString();
        }
    }
    DBManager::instance()->removeTrashImgInfos(localPaths);
    qDebug() << "AlbumControl::deleteImgFromTrash - Function exit";
}

void AlbumControl::insertCollection(const QList< QUrl > &paths)
{
    qDebug() << "AlbumControl::insertCollection - Function entry, paths count:" << paths.size();
    QStringList tmpList;
    for (QUrl url : paths) {
        tmpList << url2localPath(url);
    }
    DBImgInfoList infos;
    for (QString path : tmpList) {
        infos << DBManager::instance()->getInfoByPath(path);
    }
    qDebug() << "AlbumControl::insertCollection - Function exit, processed" << infos.size() << "items";
}

void AlbumControl::createAlbum(const QString &newName)
{
    qDebug() << "AlbumControl::createAlbum - Function entry, newName:" << newName;
    QString createAlbumName = getNewAlbumName(newName);
    int createUID = DBManager::instance()->createAlbum(createAlbumName, QStringList(" "));
    DBManager::instance()->insertIntoAlbum(createUID, QStringList(" "));
    qDebug() << "AlbumControl::createAlbum - Function exit, created album with UID:" << createUID;
}

QList<int> AlbumControl::getAllNormlAutoImportAlbumId()
{
    qDebug() << "AlbumControl::getAllNormlAutoImportAlbumId - Function entry";
    QMap < int, QString > autoImportAlbum;
    QList<std::pair<int, QString>>  tmpList = DBManager::instance()->getAllAlbumNames(AutoImport);
    for (std::pair<int, QString> tmpPair : tmpList) {
        if (tmpPair.first > 3) {
            // qDebug() << "AlbumControl::getAllNormlAutoImportAlbumId - Branch: adding normal auto import album ID:" << tmpPair.first;
            autoImportAlbum.insert(tmpPair.first, tmpPair.second);
        }
    }
    QList<int> result = autoImportAlbum.keys();
    qDebug() << "AlbumControl::getAllNormlAutoImportAlbumId - Function exit, returning" << result.size() << "album IDs";
    return result;
}

QList<int> AlbumControl::getAllSystemAutoImportAlbumId()
{
    qDebug() << "AlbumControl::getAllSystemAutoImportAlbumId - Function entry";
    QMap < int, QString > systemAlbum;
    QList<std::pair<int, QString>>  tmpList = DBManager::instance()->getAllAlbumNames(AutoImport);
    for (std::pair<int, QString> tmpPair : tmpList) {
        if (tmpPair.first > 0 && tmpPair.first <= 3) {
            // qDebug() << "AlbumControl::getAllSystemAutoImportAlbumId - Branch: adding system album ID:" << tmpPair.first;
            systemAlbum.insert(tmpPair.first, tmpPair.second);
        }
    }
    QList<int> result = systemAlbum.keys();
    qDebug() << "AlbumControl::getAllSystemAutoImportAlbumId - Function exit, returning" << result.size() << "album IDs";
    return result;
}

QList <int> AlbumControl::getAllAutoImportAlbumId()
{
    qDebug() << "AlbumControl::getAllAutoImportAlbumId - Function entry";
    QMap < int, QString > systemAlbum;
    QList<std::pair<int, QString>>  tmpList = DBManager::instance()->getAllAlbumNames(AutoImport);
    for (std::pair<int, QString> tmpPair : tmpList) {
        systemAlbum.insert(tmpPair.first, tmpPair.second);
    }
    QList<int> result = systemAlbum.keys();
    qDebug() << "AlbumControl::getAllAutoImportAlbumId - Function exit, returning" << result.size() << "album IDs";
    return result;
}

QList < int > AlbumControl::getAllCustomAlbumId()
{
    qDebug() << "AlbumControl::getAllCustomAlbumId - Function entry";
    QMap < int, QString > customAlbum;
    QList<std::pair<int, QString>>  tmpList = DBManager::instance()->getAllAlbumNames(Custom);
    for (std::pair<int, QString> tmpPair : tmpList) {
        customAlbum.insert(tmpPair.first, tmpPair.second);
    }
    m_customAlbum = customAlbum;
    QList<int> result = customAlbum.keys();
    qDebug() << "AlbumControl::getAllCustomAlbumId - Function exit, returning" << result.size() << "custom album IDs";
    return result;
}

QList < QString > AlbumControl::getAllCustomAlbumName()
{
    qDebug() << "AlbumControl::getAllCustomAlbumName - Function entry";
    QMap < int, QString > customAlbum;
    QList<std::pair<int, QString>>  tmpList = DBManager::instance()->getAllAlbumNames(Custom);
    for (std::pair<int, QString> tmpPair : tmpList) {
        customAlbum.insert(tmpPair.first, tmpPair.second);
    }
    m_customAlbum = customAlbum;
    qDebug() << "AlbumControl::getAllCustomAlbumName - Function exit";
    return customAlbum.values();
}

QStringList AlbumControl::getAlbumPaths(const int &albumId, const int &filterType)
{
    qDebug() << "1085" << albumId;
    QStringList relist;
    DBImgInfoList dbInfoList = DBManager::instance()->getInfosByAlbum(albumId, false);
    QString title = DBManager::instance()->getAlbumNameFromUID(albumId);
    for (DBImgInfo info : dbInfoList) {
        QVariantMap tmpMap;
        if (info.itemType == ItemTypePic) {
            if (filterType == 2) {
                continue ;
            }
        } else if (info.itemType == ItemTypeVideo) {
            if (filterType == 1) {
                continue ;
            }
        }
        relist << "file://" + info.filePath;
    }
    qDebug() << "AlbumControl::getAlbumPaths - Function exit, returning" << relist.size() << "paths";
    return relist;
}

QString AlbumControl::getCustomAlbumByUid(const int &index)
{
    qDebug() << "AlbumControl::getCustomAlbumByUid - Function entry, index:" << index;
    // 我的收藏和系统相册名称为固定名称，可直接根据索引获取，以便做翻译处理
    if (0 == index)
        return tr("Favorites");
    else if (1 == index)
        return tr("Screen Capture");
    else if (2 == index)
        return tr("Camera");
    else if (3 == index)
        return tr("Draw");

    QString result = DBManager::instance()->getAlbumNameFromUID(index);
    qDebug() << "AlbumControl::getCustomAlbumByUid - Function exit, returning" << result;
    return result;
}

DBImgInfoList AlbumControl::getTrashInfos(const int &filterType)
{
    qDebug() << "AlbumControl::getTrashInfos - Function entry, filterType:" << filterType;
    DBImgInfoList allTrashInfos = DBManager::instance()->getAllTrashInfos_getRemainDays();
    QDateTime currentTime = QDateTime::currentDateTime();
    DBImgInfoList list;
    for (int i = allTrashInfos.size() - 1; i >= 0; i--) {
        DBImgInfo pinfo = allTrashInfos.at(i);
        if (!QFile::exists(pinfo.filePath) &&
                !QFile::exists(getDeleteFullPath(pinfo.pathHash, pinfo.getFileNameFromFilePath()))) {
            allTrashInfos.removeAt(i);
        } else if (pinfo.remainDays <= 0) {
            list << pinfo;
            allTrashInfos.removeAt(i);
        } else if (pinfo.itemType == ItemTypePic) {
            if (filterType == 2) {
                allTrashInfos.removeAt(i);
            }
        } else if (pinfo.itemType == ItemTypeVideo) {
            if (filterType == 1) {
                allTrashInfos.removeAt(i);
            }
        }
    }
    //清理删除时间过长图片
    if (!list.isEmpty()) {
        qDebug() << "AlbumControl::getTrashInfos - Branch: removing" << list.size() << "items from trash database";
        QStringList image_list;
        for (DBImgInfo info : list) {
            image_list << info.filePath;
        }
        DBManager::instance()->removeTrashImgInfosNoSignal(image_list);
    }
    qDebug() << "AlbumControl::getTrashInfos - Function exit, returning" << allTrashInfos.size() << "items";
    return allTrashInfos;
}

DBImgInfoList AlbumControl::getTrashInfos2(const int &filterType)
{
    qDebug() << "AlbumControl::getTrashInfos2 - Function entry, filterType:" << filterType;
    DBImgInfoList allTrashInfos = DBManager::instance()->getAllTrashInfos_getRemainDays();
    QDateTime currentTime = QDateTime::currentDateTime();
    DBImgInfoList list;
    for (int i = allTrashInfos.size() - 1; i >= 0; i--) {
        DBImgInfo pinfo = allTrashInfos.at(i);
        if (!QFile::exists(pinfo.filePath) &&
                !QFile::exists(getDeleteFullPath(pinfo.pathHash, pinfo.getFileNameFromFilePath()))) {
            allTrashInfos.removeAt(i);
        } else if (pinfo.remainDays <= 0) {
            list << pinfo;
            allTrashInfos.removeAt(i);
        } else if (pinfo.itemType != filterType && filterType != ItemTypeNull) {
            allTrashInfos.removeAt(i);
        }
    }
    //清理删除时间过长图片
    if (!list.isEmpty()) {
        qDebug() << "AlbumControl::getTrashInfos2 - Branch: removing" << list.size() << "items from trash database";
        QStringList image_list;
        for (DBImgInfo info : list) {
            image_list << info.filePath;
        }
        DBManager::instance()->removeTrashImgInfosNoSignal(image_list);
    }
    return allTrashInfos;
}

DBImgInfoList AlbumControl::getCollectionInfos()
{
    qDebug() << "AlbumControl::getCollectionInfos - Function entry";
    DBImgInfoList result = DBManager::instance()->getInfosByAlbum(DBManager::SpUID::u_Favorite, false);
    qDebug() << "AlbumControl::getCollectionInfos - Function exit, returning" << result.size() << "items";
    return result;
}

DBImgInfoList AlbumControl::getScreenCaptureInfos()
{
    qDebug() << "AlbumControl::getScreenCaptureInfos - Function entry";
    DBImgInfoList result = DBManager::instance()->getInfosByAlbum(DBManager::SpUID::u_ScreenCapture, false);
    qDebug() << "AlbumControl::getScreenCaptureInfos - Function exit, returning" << result.size() << "items";
    return result;
}

DBImgInfoList AlbumControl::getCameraInfos()
{
    qDebug() << "AlbumControl::getCameraInfos - Function entry";
    DBImgInfoList result = DBManager::instance()->getInfosByAlbum(DBManager::SpUID::u_Camera, false);
    qDebug() << "AlbumControl::getCameraInfos - Function exit, returning" << result.size() << "items";
    return result;
}

QString AlbumControl::getDeleteFullPath(const QString &hash, const QString &fileName)
{
    qDebug() << "AlbumControl::getDeleteFullPath - Function entry, hash:" << hash << "fileName:" << fileName;
    //防止文件过长,采用只用hash的名称;
    QString result = albumGlobal::DELETE_PATH + "/" + hash + "." + QFileInfo(fileName).suffix();
    qDebug() << "AlbumControl::getDeleteFullPath - Function exit, returning:" << result;
    return result;
}

//需求变更：允许相册重名，空字符串返回Unnamed，其余字符串返回本名
const QString AlbumControl::getNewAlbumName(const QString &baseName)
{
    qDebug() << "AlbumControl::getNewAlbumName - Function entry, baseName:" << baseName;
    QString albumName;
    if (baseName.isEmpty()) {
        qDebug() << "AlbumControl::getNewAlbumName - Branch: baseName is empty, using 'Unnamed'";
        albumName = tr("Unnamed");
    } else {
        qDebug() << "AlbumControl::getNewAlbumName - Branch: using provided baseName";
        albumName = baseName;
    }
    qDebug() << "AlbumControl::getNewAlbumName - Function exit, returning:" << albumName;
    return static_cast<const QString>(albumName);
}

bool AlbumControl::canFavorite(const QStringList &pathList)
{
    qDebug() << "AlbumControl::canFavorite - Function entry, pathList count:" << pathList.size();
    bool bCanFavorite = false;
    for (int i = 0; i < pathList.size(); i++) {
        if (!pathList[i].isEmpty() && !photoHaveFavorited(pathList[i])) {
            // qDebug() << "AlbumControl::canFavorite - Branch: found non-favorited item:" << pathList[i];
            bCanFavorite = true;
            break;
        }
    }

    qDebug() << "AlbumControl::canFavorite - Function exit, returning:" << bCanFavorite;
    return bCanFavorite;
}

bool AlbumControl::canAddToCustomAlbum(const int &albumId, const QStringList &pathList)
{
    qDebug() << "AlbumControl::canAddToCustomAlbum - Function entry, albumId:" << albumId << "pathList count:" << pathList.size();
    bool bCanAddToCustom = false;
    for (int i = 0; i < pathList.size(); i++) {
        if (!pathList[i].isEmpty() && !photoHaveAddedToCustomAlbum(albumId, pathList[i])) {
            // qDebug() << "AlbumControl::canAddToCustomAlbum - Branch: found item not in custom album:" << pathList[i];
            bCanAddToCustom = true;
            break;
        }
    }

    qDebug() << "AlbumControl::canAddToCustomAlbum - Function exit, returning:" << bCanAddToCustom;
    return bCanAddToCustom;
}

bool AlbumControl::photoHaveFavorited(const QString &path)
{
    qDebug() << "AlbumControl::photoHaveFavorited - Function entry, path:" << path;
    bool bRet = DBManager::instance()->isImgExistInAlbum(DBManager::SpUID::u_Favorite, url2localPath(path));
    qDebug() << "AlbumControl::photoHaveFavorited - Function exit, returning:" << bRet;
    return bRet;
}

bool AlbumControl::photoHaveAddedToCustomAlbum(int albumId, const QString &path)
{
    qDebug() << "AlbumControl::photoHaveAddedToCustomAlbum - Function entry, albumId:" << albumId << "path:" << path;
    bool result = DBManager::instance()->isImgExistInAlbum(albumId, url2localPath(path));
    qDebug() << "AlbumControl::photoHaveAddedToCustomAlbum - Function exit, returning:" << result;
    return result;
}

int AlbumControl::getCustomAlbumInfoConut(const int &albumId, const int &filterType)
{
    qDebug() << "AlbumControl::getCustomAlbumInfoConut - Function entry, albumId:" << albumId << "filterType:" << filterType;
    int rePicVideoConut = 0;
    DBImgInfoList dbInfoList = DBManager::instance()->getInfosByAlbum(albumId, false);
    for (DBImgInfo info : dbInfoList) {
        QVariantMap tmpMap;
        if (info.itemType == ItemTypePic) {
            if (filterType == 2) {
                qDebug() << "AlbumControl::getCustomAlbumInfoConut - Branch: skipping picture due to video filter";
                continue ;
            }
        } else if (info.itemType == ItemTypeVideo) {
            if (filterType == 1) {
                qDebug() << "AlbumControl::getCustomAlbumInfoConut - Branch: skipping video due to picture filter";
                continue ;
            }
        }
        rePicVideoConut++;
    }
    qDebug() << "AlbumControl::getCustomAlbumInfoConut - Function exit, returning count:" << rePicVideoConut;
    return rePicVideoConut;
}

int AlbumControl::getAllInfoConut(const int &filterType)
{
    qDebug() << "AlbumControl::getAllInfoConut - Function entry, filterType:" << filterType;
    ItemType type = ItemTypeNull;
    if (filterType == 2) {
        qDebug() << "AlbumControl::getAllInfoConut - Branch: filtering for videos only";
        type = ItemTypeVideo ;
    }
    if (filterType == 1) {
        qDebug() << "AlbumControl::getAllInfoConut - Branch: filtering for pictures only";
        type = ItemTypePic ;
    }
    int count = DBManager::instance()->getImgsCount(type);
    qDebug() << "AlbumControl::getAllInfoConut - Function exit, returning count:" << count;
    return count;
}

int AlbumControl::getTrashInfoConut(const int &filterType)
{
    qDebug() << "AlbumControl::getTrashInfoConut - Function entry, filterType:" << filterType;
    DBImgInfoList allTrashInfos;
    if (filterType == 0) {
        qDebug() << "AlbumControl::getTrashInfoConut - Branch: getting all trash items";
        allTrashInfos = getTrashInfos2(ItemTypeNull);
    } else if (filterType == 1) {
        qDebug() << "AlbumControl::getTrashInfoConut - Branch: getting picture trash items";
        allTrashInfos = getTrashInfos2(ItemTypePic);
    } else if (filterType == 2) {
        qDebug() << "AlbumControl::getTrashInfoConut - Branch: getting video trash items";
        allTrashInfos = getTrashInfos2(ItemTypeVideo);
    }

    int count = allTrashInfos.size();
    qDebug() << "AlbumControl::getTrashInfoConut - Function exit, returning count:" << count;
    return count;
}

void AlbumControl::removeAlbum(int UID)
{
    qDebug() << "AlbumControl::removeAlbum - Function entry, UID:" << UID;
    DBManager::instance()->removeAlbum(UID);
    qDebug() << "AlbumControl::removeAlbum - Function exit";
}

void AlbumControl::removeFromAlbum(int UID, const QStringList &paths)
{
    qDebug() << "AlbumControl::removeFromAlbum - Function entry, UID:" << UID << "paths count:" << paths.size();
    AlbumDBType atype = AlbumDBType::Custom;
    if (UID == 0) {
        atype = AlbumDBType::Favourite;
    }
    //判断是否是自动导入
    if (isAutoImportAlbum(UID)) {
        qDebug() << "AlbumControl::removeFromAlbum - Branch: album is auto import type";
        atype = AlbumDBType::AutoImport;
    }

    QStringList localPaths ;
    for (QString path : paths) {
        localPaths << url2localPath(path);
    }

    DBManager::instance()->removeCustomAlbumIdByPaths(UID, localPaths);

    DBManager::instance()->removeFromAlbum(UID, localPaths, atype);
    qDebug() << "AlbumControl::removeFromAlbum - Function exit";
}

bool AlbumControl::insertIntoAlbum(int UID, const QStringList &paths)
{
    qDebug() << "AlbumControl::insertIntoAlbum - Function entry, UID:" << UID << "paths count:" << paths.size();
    AlbumDBType atype = AlbumDBType::Custom;
    if (UID == 0) {
        qDebug() << "AlbumControl::insertIntoAlbum - Branch: inserting into favorites";
        atype = AlbumDBType::Favourite;
    }
    QStringList localPaths ;
    for (QString path : paths) {
        localPaths << url2localPath(path);
    }

    DBManager::instance()->addCustomAlbumIdByPaths(UID, localPaths);

    bool result = DBManager::instance()->insertIntoAlbum(UID, localPaths, atype);
    qDebug() << "AlbumControl::insertIntoAlbum - Function exit, returning:" << result;
    return result;
}

bool AlbumControl::insertImportIntoAlbum(int UID, const QStringList &paths)
{
    qDebug() << "AlbumControl::insertImportIntoAlbum - Function entry, UID:" << UID << "paths count:" << paths.size();
    AlbumDBType atype = AlbumDBType::AutoImport;
    if (UID == 0) {
        qDebug() << "AlbumControl::insertImportIntoAlbum - Branch: inserting into favorites";
        atype = AlbumDBType::Favourite;
    }
    QStringList localPaths ;
    for (QString path : paths) {
        localPaths << url2localPath(path);
    }
    bool result = DBManager::instance()->insertIntoAlbum(UID, localPaths, atype);
    qDebug() << "AlbumControl::insertImportIntoAlbum - Function exit, returning:" << result;
    return result;
}

void AlbumControl::updateInfoPath(const QString &oldPath, const QString &newPath)
{
    qDebug() << "AlbumControl::updateInfoPath - Function entry, oldPath:" << oldPath << "newPath:" << newPath;
    auto oldLocalPath = url2localPath(oldPath);
    auto newLocalPath = url2localPath(newPath);
    bool ok = DBManager::instance()->updateImgPath(oldLocalPath, newLocalPath);

    if (ok) {
        qDebug() << "AlbumControl::updateInfoPath - Branch: path update successful, refreshing UI";
        // 通知前端刷新相关界面，包括自定义相册/我的收藏/合集-所有项目/已导入
        sigRefreshCustomAlbum(-1);
        sigRefreshAllCollection();
        sigRefreshImportAlbum();
        sigRefreshSearchView();
    }
    qDebug() << "AlbumControl::updateInfoPath - Function exit";
}

bool AlbumControl::renameAlbum(int UID, const QString &newName)
{
    qDebug() << "AlbumControl::renameAlbum - Function entry, UID:" << UID << "newName:" << newName;
    DBManager::instance()->renameAlbum(UID, newName);
    qDebug() << "AlbumControl::renameAlbum - Function exit, returning: true";
    return true;
}

QVariant AlbumControl::searchPicFromAlbum(int UID, const QString &keywords, bool useAI)
{
    qDebug() << "AlbumControl::searchPicFromAlbum - Function entry, UID:" << UID << "keywords:" << keywords << "useAI:" << useAI;
    DBImgInfoList dbInfos;
    if (useAI) { //使用AI进行分析
        qDebug() << "AlbumControl::searchPicFromAlbum - Branch: AI search not implemented";
        ;
    } else { //不使用AI分析，直接按文件路径搜索
        qDebug() << "AlbumControl::searchPicFromAlbum - Branch: using keyword search";
        if (UID == -1) {
            qDebug() << "AlbumControl::searchPicFromAlbum - Branch: searching all albums";
            dbInfos = DBManager::instance()->getInfosForKeyword(keywords);
        } else if (UID == -2) {
            qDebug() << "AlbumControl::searchPicFromAlbum - Branch: searching trash";
            dbInfos = DBManager::instance()->getTrashInfosForKeyword(keywords);
        } else {
            qDebug() << "AlbumControl::searchPicFromAlbum - Branch: searching specific album";
            dbInfos = DBManager::instance()->getInfosForKeyword(UID, keywords);
        }
    }

    QStringList paths;
    std::transform(dbInfos.begin(), dbInfos.end(), std::back_inserter(paths), [](const DBImgInfo & info) {
        return "file://" + info.filePath;
    });

    qDebug() << "AlbumControl::searchPicFromAlbum - Function exit, returning" << paths.size() << "paths";
    return paths;
}

DBImgInfoList AlbumControl::searchPicFromAlbum2(int UID, const QString &keywords, bool useAI)
{
    qDebug() << "AlbumControl::searchPicFromAlbum2 - Function entry, UID:" << UID << "keywords:" << keywords << "useAI:" << useAI;
    DBImgInfoList dbInfos;
    if (useAI) { //使用AI进行分析
        qDebug() << "AlbumControl::searchPicFromAlbum2 - Branch: AI search not implemented";
        ;
    } else { //不使用AI分析，直接按文件路径搜索
        qDebug() << "AlbumControl::searchPicFromAlbum2 - Branch: using keyword search";
        if (UID == -1) {
            qDebug() << "AlbumControl::searchPicFromAlbum2 - Branch: searching all albums";
            dbInfos = DBManager::instance()->getInfosForKeyword(keywords);
        } else if (UID == -2) {
            qDebug() << "AlbumControl::searchPicFromAlbum2 - Branch: searching trash";
            dbInfos = DBManager::instance()->getTrashInfosForKeyword(keywords);
        } else {
            qDebug() << "AlbumControl::searchPicFromAlbum2 - Branch: searching specific album";
            dbInfos = DBManager::instance()->getInfosForKeyword(UID, keywords);
        }
    }

    qDebug() << "AlbumControl::searchPicFromAlbum2 - Function exit, returning" << dbInfos.size() << "items";
    return dbInfos;
}

QStringList AlbumControl::imageCanExportFormat(const QString &path)
{
    qDebug() << "AlbumControl::imageCanExportFormat - Function entry, path:" << path;
    QString localPath = url2localPath(path);
    QStringList formats;
    formats << "jpg";
    formats << "jpeg";
    formats << "png";
    formats << "bmp";
    formats << "pgm";
    formats << "xbm";
    formats << "xpm";
    QFileInfo info(localPath);
    info.suffix();
    if (!formats.contains(info.suffix())) {
        if (!info.suffix().isEmpty()) {
            qDebug() << "AlbumControl::imageCanExportFormat - Branch: adding original suffix:" << info.suffix();
            formats << info.suffix();
        }
    }
    qDebug() << "AlbumControl::imageCanExportFormat - Function exit, returning" << formats.size() << "formats";
    return formats;
}

bool AlbumControl::saveAsImage(const QString &path, const QString &saveName, int index, const QString &fileFormat, int pictureQuality, const QString &saveFolder)
{
    qDebug() << "AlbumControl::saveAsImage - Function entry, path:" << path << "saveName:" << saveName << "index:" << index
             << "fileFormat:" << fileFormat << "pictureQuality:" << pictureQuality << "saveFolder:" << saveFolder;
    bool bRet = false;
    QString localPath = url2localPath(path);
    QString savePath;
    QString finalSaveFolder;
    switch (index) {
    case 0:
        qDebug() << "AlbumControl::saveAsImage - Branch: saving to Pictures location";
        finalSaveFolder = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        break;
    case 1:
        qDebug() << "AlbumControl::saveAsImage - Branch: saving to Documents location";
        finalSaveFolder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        break;
    case 2:
        qDebug() << "AlbumControl::saveAsImage - Branch: saving to Download location";
        finalSaveFolder = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        break;
    case 3:
        qDebug() << "AlbumControl::saveAsImage - Branch: saving to Desktop location";
        finalSaveFolder = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        break;
    case 4:
        qDebug() << "AlbumControl::saveAsImage - Branch: saving to Movies location";
        finalSaveFolder = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        break;
    case 5:
        qDebug() << "AlbumControl::saveAsImage - Branch: saving to Music location";
        finalSaveFolder = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        break;
    default :
        qDebug() << "AlbumControl::saveAsImage - Branch: saving to custom folder";
        finalSaveFolder = saveFolder;
        break;
    }
    savePath = finalSaveFolder + "/" + saveName + "." + fileFormat;
    QStringList formats;
    formats << "jpg";
    formats << "jpeg";
    formats << "png";
    formats << "bmp";
    formats << "pgm";
    formats << "xbm";
    formats << "xpm";
    QFileInfo info(localPath);
    if (!formats.contains(info.suffix())) {

        qDebug() << "AlbumControl::saveAsImage - Branch: non-standard format, using direct file copy";
        QFileInfo fileinfo(savePath);
        if (fileinfo.exists() && !fileinfo.isDir()) {
            //目标位置与原图位置相同则直接返回
            if (localPath == savePath) {
                qDebug() << "AlbumControl::saveAsImage - Branch: target path is same as source, returning true";
                return true;
            }
            //目标位置与原图位置不同则先删除再复制
            qDebug() << "AlbumControl::saveAsImage - Branch: target file exists, removing it first";
            if (QFile::remove(savePath)) {
                bRet = QFile::copy(localPath, savePath);
            }
        } else {
            bRet = QFile::copy(localPath, savePath);
            qDebug() << "AlbumControl::saveAsImage - Branch: direct file copy, result:" << bRet;
        }
    } else {
        qDebug() << "AlbumControl::saveAsImage - Branch: standard format, using QImage save";
        QImage m_saveImage;
        QString errMsg;
        LibUnionImage_NameSpace::loadStaticImageFromFile(localPath, m_saveImage, errMsg);
        bRet = m_saveImage.save(savePath, fileFormat.toUpper().toLocal8Bit().data(), pictureQuality);
        qDebug() << "AlbumControl::saveAsImage - Branch: image save result:" << bRet;
    }

    qDebug() << "AlbumControl::saveAsImage - Function exit, returning:" << bRet;
    return bRet;
}

QString AlbumControl::getFolder()
{
    qDebug() << "AlbumControl::getFolder - Function entry";
    QFileDialog dialog;
    QString fileDir("");
    dialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly);
    if (dialog.exec()) {
        fileDir = dialog.selectedFiles().first();
    }
    qDebug() << "AlbumControl::getFolder - Function exit, returning:" << fileDir;
    return fileDir;
}

bool AlbumControl::getFolders(const QStringList &paths)
{
    qDebug() << "AlbumControl::getFolders - Function entry, paths count:" << paths.size();
    bool bRet = true;
    QFileDialog dialog;
    QString fileDir;
    dialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly);
    if (dialog.exec()) {
        fileDir = dialog.selectedFiles().first();
    }
    QStringList localPaths;
    for (QString path : paths) {
        localPaths << url2localPath(path);
    }
    if (!fileDir.isEmpty()) {
        qDebug() << "AlbumControl::getFolders - Branch: processing" << localPaths.size() << "files to copy";
        for (QString path : localPaths) {
            QString savePath = fileDir + "/" + QFileInfo(path).completeBaseName() + "." + QFileInfo(path).completeSuffix();
            QFileInfo fileinfo(savePath);
            if (fileinfo.exists() && !fileinfo.isDir()) {
                //目标位置与原图位置相同则直接返回
                if (path == savePath) {
                    qDebug() << "AlbumControl::getFolders - Branch: target path is same as source, returning true";
                    return true;
                }
                //目标位置与原图位置不同则先删除再复制
                qDebug() << "AlbumControl::getFolders - Branch: target file exists, removing it first:" << savePath;
                if (QFile::remove(savePath)) {
                    bRet = QFile::copy(path, savePath);
                }
            } else {
                bRet = QFile::copy(path, savePath);
                qDebug() << "AlbumControl::getFolders - Branch: direct file copy, result:" << bRet;
            }
        }

    }
    qDebug() << "AlbumControl::getFolders - Function exit, returning:" << bRet;
    return bRet;
}

bool AlbumControl::exportFolders(const QStringList &paths, const QString &dir)
{
    qDebug() << "AlbumControl::exportFolders - Function entry, paths count:" << paths.size() << "dir:" << dir;
    bool bRet = true;
    QFileDialog dialog;
    QString fileDir;
    dialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setOption(QFileDialog::ShowDirsOnly);
    if (dialog.exec()) {
        fileDir = dialog.selectedFiles().first();
    }
    QStringList localPaths;
    for (QString path : paths) {
        localPaths << url2localPath(path);
    }
    if (!fileDir.isEmpty()) {
        QString newDir = fileDir + "/" + dir;
        QDir a;
        a.mkdir(newDir);


        for (QString path : localPaths) {

            QString savePath = newDir + "/" + QFileInfo(path).completeBaseName() + "." + QFileInfo(path).completeSuffix();
            QFileInfo fileinfo(savePath);
            if (fileinfo.exists() && !fileinfo.isDir()) {
                //目标位置与原图位置相同则直接返回
                if (path == savePath) {
                    qDebug() << "AlbumControl::exportFolders - Branch: target path is same as source, returning true";
                    return true;
                }
                //目标位置与原图位置不同则先删除再复制
                if (QFile::remove(savePath)) {
                    bRet = QFile::copy(path, savePath);
                }
            } else {
                bRet = QFile::copy(path, savePath);
            }
        }
    }
    qDebug() << "AlbumControl::exportFolders - Function exit, returning:" << bRet;
    return bRet;
}

void AlbumControl::openDeepinMovie(const QString &path)
{
    qDebug() << "AlbumControl::openDeepinMovie - Function entry, path:" << path;
    QString localPath = url2localPath(path);
    if (LibUnionImage_NameSpace::isVideo(localPath)) {
        qDebug() << "AlbumControl::openDeepinMovie - Branch: path is a video file";
        QProcess *process = new QProcess(this);
        QStringList arguments;

        // try to use ll-cli first
        bool trylinglongSucc = false;

        // check if current is v23, v25 or later
        static const int kMinimalOsVersion = 23;
        const int osMajor = DSysInfo::majorVersion().toInt();
        if (osMajor >= kMinimalOsVersion) {
            qInfo() << "trying to start deepin-movie via Dbus: com.deepin.movie->openFile";
            QDBusMessage message = QDBusMessage::createMethodCall("com.deepin.movie", "/", "com.deepin.movie", "openFile");
            message << path;
            QDBusMessage retMessage = QDBusConnection::sessionBus().call(message);

            if (retMessage.type() != QDBusMessage::ErrorMessage) {
                trylinglongSucc = true;
                qDebug() << "[dbus] Open it with deepin-movie";
            } else {
                qWarning() << retMessage.errorMessage();
            }
        }

        if (!trylinglongSucc) {
            qInfo() << "trying start detached deepin-movie";

            arguments << path;
            bool isopen = process->startDetached("deepin-movie", arguments);
            if (!isopen) {
                arguments.clear();
                arguments << "-o" << path;
                process->startDetached("dde-file-manager", arguments);
            }
        }

        connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));
    }
    qDebug() << "AlbumControl::openDeepinMovie - Function exit";
}

QString AlbumControl::getFileTime(const QString &path1, const QString &path2)
{
    qDebug() << "AlbumControl::getFileTime - Function entry, path1:" << path1 << "path2:" << path2;
    auto time1 = DBManager::instance()->getFileImportTime(url2localPath(path1));
    auto time2 = DBManager::instance()->getFileImportTime(url2localPath(path2));

    auto str1 = time1.toString("yyyy/MM/dd");
    auto str2 = time2.toString("yyyy/MM/dd");

    QString language = QLocale::system().name();
    if (language == "zh_CN") {
        str1 = QString(tr("%1Year%2Month%3Day"))
                   .arg(time1.date().year())
                   .arg(time1.date().month())
                   .arg(time1.date().day());
        str2 = QString(tr("%1Year%2Month%3Day"))
                   .arg(time2.date().year())
                   .arg(time2.date().month())
                   .arg(time2.date().day());
    }
    if (time1 < time2) {
        QString result = str1 + "-" + str2;
        qDebug() << "AlbumControl::getFileTime - Branch: time1 < time2, returning:" << result;
        return result;
    } else {
        QString result = str2 + "-" + str1;
        qDebug() << "AlbumControl::getFileTime - Branch: time1 >= time2, returning:" << result;
        return result;
    }
}

QString AlbumControl::getMovieInfo(const QString key, const QString &path)
{
    qDebug() << "AlbumControl::getMovieInfo - Function entry, key:" << key << "path:" << path;
    QString value = "";
    if (!path.isEmpty()) {
        QString localPath = url2localPath(path);
        if (!m_movieInfos.keys().contains(localPath)) {
            MovieInfo movieInfo = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(localPath));
            //对视频信息缓存
            m_movieInfos[localPath] = movieInfo;
        }
        MovieInfo movieInfo = m_movieInfos.value(localPath);
        if (QString("Video CodecID").contains(key)) {
            value = movieInfo.vCodecID;
        } else if (QString("Video CodeRate").contains(key)) {
            if (movieInfo.vCodeRate == 0) {
                value = "-";
            } else {
                value = movieInfo.vCodeRate > 1000 ? QString::number(movieInfo.vCodeRate / 1000) + " kbps"
                        : QString::number(movieInfo.vCodeRate) + " bps";
            }
        } else if (QString("FPS").contains(key)) {
            value = movieInfo.fps == 0 ? "-" : QString::number(movieInfo.fps) + " fps";
        } else if (QString("Proportion").contains(key)) {
            value = movieInfo.proportion <= 0 ? "-" : QString::number(movieInfo.proportion);
        } else if (QString("Resolution").contains(key)) {
            value = movieInfo.resolution;
        } else if (QString("Audio CodecID").contains(key)) {
            value = movieInfo.aCodeID;
        } else if (QString("Audio CodeRate").contains(key)) {
            if (movieInfo.aCodeRate == 0) {
                value = "-";
            } else {
                value = movieInfo.aCodeRate > 1000 ? QString::number(movieInfo.aCodeRate / 1000) + " kbps"
                        : QString::number(movieInfo.aCodeRate) + " bps";
            }
        } else if (QString("Audio digit").contains(key)) {
            value = movieInfo.aDigit == 0 ? "-" : QString::number(movieInfo.aDigit);
        } else if (QString("Channels").contains(key)) {
            value = movieInfo.channels == 0 ? "-" : QString::number(movieInfo.channels) + tr("Channel");
        } else if (QString("Sampling").contains(key)) {
            value = movieInfo.sampling == 0 ? "-" : QString::number(movieInfo.sampling) + " hz";
        } else if (QString("DateTimeOriginal").contains(key)) {
            QFileInfo info(localPath);
            if (info.lastModified().isValid()) {
                value = info.lastModified().toString("yyyy/MM/dd HH:mm");
            } else if (info.birthTime().isValid()) {
                value = info.birthTime().toString("yyyy/MM/dd HH:mm");
            }
        } else if (QString("Type").contains(key)) {
            value = movieInfo.fileType.toLower();
        } else if (QString("Size").contains(key)) {
            value = movieInfo.sizeStr();
        } else if (QString("Duration").contains(key)) {
            value = movieInfo.duration;
        } else if (QString("Path").contains(key)) {
            value = movieInfo.filePath;
        }
    }
    qDebug() << "AlbumControl::getMovieInfo - Function exit, returning value:" << value;
    return value;
}

int AlbumControl::getYearCount(const QString &year)
{
    qDebug() << "AlbumControl::getYearCount - Function entry, year:" << year;
    int result = DBManager::instance()->getYearCount(year);
    qDebug() << "AlbumControl::getYearCount - Function exit, returning count:" << result;
    return result;
}

QStringList AlbumControl::getYears()
{
    qDebug() << "AlbumControl::getYears - Function entry";
    QStringList result = DBManager::instance()->getYears();
    qDebug() << "AlbumControl::getYears - Function exit, returning" << result.size() << "years";
    return result;
}

int AlbumControl::getMonthCount(const QString &year, const QString &month)
{
    qDebug() << "AlbumControl::getMonthCount - Function entry, year:" << year << "month:" << month;
    int result = DBManager::instance()->getMonthCount(year, month);
    qDebug() << "AlbumControl::getMonthCount - Function exit, returning count:" << result;
    return result;
}

QStringList AlbumControl::getMonthPaths(const QString &year, const QString &month)
{
    qDebug() << "AlbumControl::getMonthPaths - Function entry, year:" << year << "month:" << month;
    QStringList result = DBManager::instance()->getMonthPaths(year, month, 6);
    qDebug() << "AlbumControl::getMonthPaths - Function exit, returning" << result.size() << "paths";
    return result;
}

QStringList AlbumControl::getMonths()
{
    qDebug() << "AlbumControl::getMonths - Function entry";
    QStringList result = DBManager::instance()->getMonths();
    qDebug() << "AlbumControl::getMonths - Function exit, returning" << result.size() << "months";
    return result;
}

QStringList AlbumControl::getDeviceNames()
{
    qDebug() << "AlbumControl::getDeviceNames - Function entry";
    QStringList result = m_durlAndNameMap.values();
    qDebug() << "AlbumControl::getDeviceNames - Function exit, returning" << result.size() << "device names";
    return result;
}

QStringList AlbumControl::getDevicePaths()
{
    qDebug() << "AlbumControl::getDevicePaths - Function entry";
    QStringList result = m_durlAndNameMap.keys();
    qDebug() << "AlbumControl::getDevicePaths - Function exit, returning" << result.size() << "device paths";
    return result;
}

QString AlbumControl::getDeviceName(const QString &devicePath)
{
    qDebug() << "AlbumControl::getDeviceName - Function entry, devicePath:" << devicePath;
    QString result = m_durlAndNameMap.value(devicePath);
    qDebug() << "AlbumControl::getDeviceName - Function exit, returning:" << result;
    return result;
}

DBImgInfoList fromDeviceAlbumInfoList(const QMap<QString, ItemType> &filePairList, const int &filterType)
{
    qDebug() << "fromDeviceAlbumInfoList - Function entry, filePairList count:" << filePairList.size() << "filterType:" << filterType;
    DBImgInfoList infoList;
    for (auto fileItr = filePairList.begin(); fileItr != filePairList.end(); ++fileItr) {
        if (ItemTypeNull != filterType && filterType != fileItr.value()) {
            continue;
        }

        DBImgInfo info;
        info.filePath = fileItr.key();
        info.itemType = fileItr.value();
        info.pathHash = "";
        info.remainDays = 0;

        infoList << info;
    }
    qDebug() << "fromDeviceAlbumInfoList - Function exit, returning" << infoList.size() << "items";
    return infoList;
}

void AlbumControl::loadDeviceAlbumInfoAsync(const QString &devicePath)
{
    qDebug() << "AlbumControl::loadDeviceAlbumInfoAsync - Function entry, devicePath:" << devicePath;
    if (m_PhonePicFileMap.contains(devicePath)) {
        qDebug() << "AlbumControl::loadDeviceAlbumInfoAsync - Branch: device already in cache, returning";
        return;
    }

    m_PhonePicFileMap.insert(devicePath, nullptr);
    QThreadPool::globalInstance()->start([=](){
        // Notify load device info
        Q_EMIT deviceAlbumInfoLoadStart(devicePath);

        //获取所选文件类型过滤器
        QStringList filters;
        for (QString i : LibUnionImage_NameSpace::unionImageSupportFormat()) {
            filters << "*." + i;
        }
        for (QString i : LibUnionImage_NameSpace::videoFiletypes()) {
            filters << "*." + i;
        }

        //定义迭代器并设置过滤器，包括子目录：QDirIterator::Subdirectories
        QDirIterator dir_iterator(devicePath,
                                  filters,
                                  QDir::Files/* | QDir::NoSymLinks*/,
                                  QDirIterator::Subdirectories);

        DeviceInfoPtr devicePtr = DeviceInfoPtr::create();
        while (dir_iterator.hasNext()) {
            dir_iterator.next();

            ItemType type;
            QString filePath = dir_iterator.filePath();
            if (LibUnionImage_NameSpace::isImage(filePath)) {
                type = ItemTypePic;
                devicePtr->picCount++;
            } else if (LibUnionImage_NameSpace::isVideo(filePath)) {
                type = ItemTypeVideo;
                devicePtr->videoCount++;
            } else {
                continue;
            }

            devicePtr->fileTypeMap.insert(filePath, type);
        }

        // GUI thread, notify update data
        QMetaObject::invokeMethod(qApp, [=](){
                m_PhonePicFileMap.insert(devicePath, devicePtr);
                
                Q_EMIT deviceAlbumInfoLoadFinished(devicePath);
                Q_EMIT deviceAlbumInfoCountChanged(devicePath, devicePtr->picCount, devicePtr->videoCount);
            }, Qt::QueuedConnection);
    });
    qDebug() << "AlbumControl::loadDeviceAlbumInfoAsync - Function exit";
}

DBImgInfoList AlbumControl::getDeviceAlbumInfoList(const QString &devicePath, const int &filterType, bool *loading)
{
    qDebug() << "AlbumControl::getDeviceAlbumInfoList - Function entry, devicePath:" << devicePath << "filterType:" << filterType;
    auto itr = m_PhonePicFileMap.find(devicePath);
    if (itr != m_PhonePicFileMap.end()) {
        qDebug() << "AlbumControl::getDeviceAlbumInfoList - Branch: device found in cache";
        DeviceInfoPtr devicePtr = *(itr);
        if (!devicePtr.isNull()) {
            qDebug() << "AlbumControl::getDeviceAlbumInfoList - Branch: device info valid, returning album list";
            return fromDeviceAlbumInfoList(devicePtr->fileTypeMap, filterType);
        }
    }

    qDebug() << "AlbumControl::getDeviceAlbumInfoList - Branch: device not found or null, starting async load";
    // Not load before, mark current device loading.
    loadDeviceAlbumInfoAsync(devicePath);

    if (loading) {
        qDebug() << "AlbumControl::getDeviceAlbumInfoList - Branch: setting loading flag to true";
        *loading = true;
    }
    qDebug() << "AlbumControl::getDeviceAlbumInfoList - Function exit, returning empty list";
    return {};
}

void AlbumControl::getDeviceAlbumInfoCountAsync(const QString &devicePath)
{
    qDebug() << "AlbumControl::getDeviceAlbumInfoCountAsync - Function entry, devicePath:" << devicePath;
    auto itr = m_PhonePicFileMap.find(devicePath);
    if (itr != m_PhonePicFileMap.end()) {
        qDebug() << "AlbumControl::getDeviceAlbumInfoCountAsync - Branch: device found in cache";
        DeviceInfoPtr devicePtr = *(itr);
        if (!devicePtr.isNull()) {
            qDebug() << "AlbumControl::getDeviceAlbumInfoCountAsync - Branch: emitting count info, pics:" << devicePtr->picCount << "videos:" << devicePtr->videoCount;
            Q_EMIT deviceAlbumInfoCountChanged(devicePath, devicePtr->picCount, devicePtr->videoCount);
        }
    }

    // Not load before, mark current device loading.
    loadDeviceAlbumInfoAsync(devicePath);
    qDebug() << "AlbumControl::getDeviceAlbumInfoCountAsync - Function exit";
}

QList<int> AlbumControl::getPicVideoCountFromPaths(const QStringList &paths, const QString &devicePath)
{
    qDebug() << "AlbumControl::getPicVideoCountFromPaths - Function entry, paths count:" << paths.size() << "devicePath:" << devicePath;
    if (paths.isEmpty()) {
        qDebug() << "AlbumControl::getPicVideoCountFromPaths - Branch: paths is empty, returning {0, 0}";
        return {0, 0};
    }

    int countPic = 0;
    int countVideo = 0;

    if (auto devicePtr = m_PhonePicFileMap.value(devicePath)) {
        qDebug() << "AlbumControl::getPicVideoCountFromPaths - Branch: using device cache for counting";
        for (const QString &path : paths) {
            auto type = devicePtr->fileTypeMap.value(path, ItemTypeNull);
            switch (type) {
                case ItemTypePic:
                    countPic++;
                    break;
                case ItemTypeVideo:
                    countVideo++;
                    break;
                default:
                    break;
            }
        }

    } else {
        qDebug() << "AlbumControl::getPicVideoCountFromPaths - Branch: using file system detection for counting";
        for (QString path : paths) {
            if (LibUnionImage_NameSpace::isImage(url2localPath(path))) {
                countPic++;
            } else if (LibUnionImage_NameSpace::isVideo(url2localPath(path))) {
                countVideo++;
            }
        }
    }

    qDebug() << "AlbumControl::getPicVideoCountFromPaths - Function exit, returning pics:" << countPic << "videos:" << countVideo;
    return {countPic, countVideo};
}

void AlbumControl::importFromMountDevice(const QStringList &paths, const int &index)
{
    qDebug() << "AlbumControl::importFromMountDevice - Function entry, paths count:" << paths.size() << "index:" << index;
    //采用线程执行导入
    QThread *thread = QThread::create([ = ] {
        qDebug() << "AlbumControl::importFromMountDevice - Branch: starting import thread";
        QStringList localPaths;
        for (QString path : paths)
        {
            localPaths << url2localPath(path);
        }
        QStringList newPathList;
        DBImgInfoList dbInfos;
        QString strHomePath = QDir::homePath();
        //获取系统现在的时间
        QString strDate = QDateTime::currentDateTime().toString("yyyy-MM-dd");
        QString basePath = QString("%1%2%3/%4").arg(strHomePath, "/Pictures/", tr("Pictures"), strDate);
        QDir dir;
        if (!dir.exists(basePath))
        {
            dir.mkpath(basePath);
        }
        for (QString strPath : localPaths)
        {
            //取出文件名称
            QStringList pathList = strPath.split("/", Qt::SkipEmptyParts);
            QStringList nameList = pathList.last().split(".", Qt::SkipEmptyParts);
            QString strNewPath = QString("%1%2%3%4%5%6").arg(basePath, "/", nameList.first(),
                                                             QString::number(QDateTime::currentDateTime().toMSecsSinceEpoch()), ".", nameList.last());
            //判断新路径下是否存在目标文件，若不存在，继续循环
            if (!dir.exists(strPath)) {
                continue;
            }
            //判断新路径下是否存在目标文件，若存在，先删除掉
            if (dir.exists(strNewPath)) {
                dir.remove(strNewPath);
            }
            newPathList << strNewPath;
            QFileInfo fi(strNewPath);
            //复制失败的图片不算在成功导入
            if (QFile::copy(strPath, strNewPath)) {
                dbInfos << getDBInfo(strNewPath, LibUnionImage_NameSpace::isVideo(strNewPath));
            } else {
                newPathList.removeOne(strNewPath);
            }

        }
        if (!dbInfos.isEmpty())
        {
            QStringList pathslist;
            int idblen = dbInfos.length();
            for (int i = 0; i < idblen; i++) {
                if (m_bneedstop) {
                    return;
                }
                pathslist << dbInfos[i].filePath;
            }


            DBManager::instance()->insertImgInfos(dbInfos);
            if (index > 0) {
                DBManager::instance()->insertIntoAlbum(index, pathslist);
                emit sigRefreshCustomAlbum(index);
            }
            emit sigRefreshImportAlbum();
            emit sigRefreshAllCollection();
        }
    });
    thread->start();
    connect(thread, &QThread::destroyed, thread, &QObject::deleteLater);
    qDebug() << "AlbumControl::importFromMountDevice - Function exit";
}

QString AlbumControl::getYearCoverPath(const QString &year)
{
    qDebug() << "AlbumControl::getYearCoverPath - Function entry, year:" << year;
    auto paths = DBManager::instance()->getYearPaths(year, 1);
    if (paths.isEmpty()) {
        qDebug() << "AlbumControl::getYearCoverPath - Branch: no paths found, returning empty string";
        return "";
    }
    qDebug() << "AlbumControl::getYearCoverPath - Function exit, returning path:" << paths[0];
    return paths[0];
}

//获取指定日期的照片路径
QStringList AlbumControl::getDayPaths(const QString &day)
{
    qDebug() << "AlbumControl::getDayPaths - Function entry, day:" << day;
    QStringList result = DBManager::instance()->getDayPaths(day);
    qDebug() << "AlbumControl::getDayPaths - Function exit, returning" << result.size() << "paths";
    return result;
}

int AlbumControl::getDayInfoCount(const QString &day, const int &filterType)
{
    qDebug() << "AlbumControl::getDayInfoCount - Function entry, day:" << day << "filterType:" << filterType;
    int rePicVideoConut = 0;
    QStringList list = getDayPaths(day);
    for (QString path : list) {
        QVariantMap tmpMap;
        if (LibUnionImage_NameSpace::isImage(url2localPath(path))) {
            if (filterType == ItemTypePic) {
                qDebug() << "AlbumControl::getDayInfoCount - Branch: counting image for picture filter";
                rePicVideoConut++;
            }
        } else if (LibUnionImage_NameSpace::isVideo(url2localPath(path))) {
            if (filterType == ItemTypeVideo) {
                qDebug() << "AlbumControl::getDayInfoCount - Branch: counting video for video filter";
                rePicVideoConut++;
            }
        }
    }
    qDebug() << "AlbumControl::getDayInfoCount - Function exit, returning count:" << rePicVideoConut;
    return rePicVideoConut;
}

//获取日期
QStringList AlbumControl::getDays()
{
    qDebug() << "AlbumControl::getDays - Function entry";
    QStringList result = DBManager::instance()->getDays();
    qDebug() << "AlbumControl::getDays - Function exit, returning" << result.size() << "days";
    return result;
}

int AlbumControl::getImportAlubumCount()
{
    qDebug() << "AlbumControl::getImportAlubumCount - Function entry";
    QMap <int, QString> customAutoImportUIDAndPaths = DBManager::instance()->getAllCustomAutoImportUIDAndPath();
    return customAutoImportUIDAndPaths.count();
}

QList<int> AlbumControl::getImportAlubumAllId()
{
    qDebug() << "AlbumControl::getImportAlubumAllId - Function entry";
    QMap <int, QString> customAutoImportUIDAndPaths = DBManager::instance()->getAllCustomAutoImportUIDAndPath();
    return customAutoImportUIDAndPaths.keys();
}

QStringList AlbumControl::getImportAlubumAllPaths()
{
    qDebug() << "AlbumControl::getImportAlubumAllPaths - Function entry";
    QMap <int, QString> customAutoImportUIDAndPaths = DBManager::instance()->getAllCustomAutoImportUIDAndPath();
    return customAutoImportUIDAndPaths.values();
}

QStringList AlbumControl::getImportAlubumAllNames()
{
    qDebug() << "AlbumControl::getImportAlubumAllNames - Function entry";
    return DBManager::instance()->getAllCustomAutoImportNames();
}

void AlbumControl::removeCustomAutoImportPath(int uid)
{
    qDebug() << "AlbumControl::removeCustomAutoImportPath - Function entry, uid:" << uid;
    return DBManager::instance()->removeCustomAutoImportPath(uid);
}

void AlbumControl::createNewCustomAutoImportAlbum(const QString &path)
{
    qDebug() << "AlbumControl::createNewCustomAutoImportAlbum - Function entry, path:" << path;
    QString folder = path;
    if (!QFileInfo(folder).isDir()) {
        folder = getFolder();
    }

    // 点击取消，不再执行自动导入相册流程
    if (folder.isEmpty())
        return;

    //自定义自动导入路径的相册名是文件夹最后一级的名字
    QString albumName = folder.split('/').last();
    int UID = DBManager::instance()->createNewCustomAutoImportPath(folder, albumName);

    QStringList urls;
    urls << QUrl::fromLocalFile(folder).toString();
    importAllImagesAndVideos(urls, UID);
    qDebug() << "AlbumControl::createNewCustomAutoImportAlbum - Function exit";
}

QString AlbumControl::getVideoTime(const QString &path)
{
    qDebug() << "AlbumControl::getVideoTime - Function entry, path:" << path;
    if (!LibUnionImage_NameSpace::isVideo(url2localPath(path)))
        return "00:00";

    //采用线程执行导入
    QThread *thread = QThread::create([ = ] {
        m_mutex.lock();
        QString reString;
        reString = MovieService::instance()->getMovieInfo(QUrl(path)).duration;
        if (reString == "-")
        {
            reString = "00:00";
        }

        if (reString.left(2) == "00")
        {
            reString = reString.right(5);
        }

        emit sigRefreashVideoTime(path, reString);
        m_mutex.unlock();
    });
    thread->start();
    connect(thread, &QThread::destroyed, thread, &QObject::deleteLater);
    qDebug() << "AlbumControl::getVideoTime - Function exit";
    return "00:00";
}

//外部使用相册打开图片
void AlbumControl::onNewAPPOpen(qint64 pid, const QStringList &arguments)
{
    qDebug() << "New application instance opened - PID:" << pid;
    Q_UNUSED(pid);
    QStringList paths;
    QStringList validPaths;
    
    if (arguments.length() > 1) {
        qDebug() << "AlbumControl::onNewAPPOpen - Branch: processing" << arguments.length() - 1 << "arguments";
        //arguments第1个参数是进程名，图片paths参数需要从下标1开始
        qDebug() << "Processing" << arguments.length() - 1 << "arguments";
        for (int i = 1; i < arguments.size(); ++i) {
            QString qpath = arguments.at(i);
            //BUG#79815，添加文件URL解析（BUG是平板上的，为防止UOS的其它个别版本也改成传URL，干脆直接全部支持）
            auto urlPath = QUrl(qpath);
            if (urlPath.scheme() == "file") {
                qpath = url2localPath(urlPath);
            }

            if (QUrl::fromUserInput(qpath).isLocalFile()) {
                qpath = "file://" + qpath;
            }

            if (LibUnionImage_NameSpace::isImage(url2localPath(qpath)) || 
                LibUnionImage_NameSpace::isVideo(url2localPath(qpath))) {
                validPaths.append(qpath);
                qDebug() << "Valid path found:" << qpath;
            }
            paths.append(qpath);
        }

        if (!paths.isEmpty()) {
            if (validPaths.count() > 0) {
                qDebug() << "Emitting signal to open" << validPaths.count() << "valid files";
                emit sigOpenImageFromFiles(validPaths);
            } else {
                qWarning() << "No valid files found in arguments";
                emit sigInvalidFormat();
            }
        }
    }

    emit sigActiveApplicationWindow();
    qDebug() << "AlbumControl::onNewAPPOpen - Function exit";
}
