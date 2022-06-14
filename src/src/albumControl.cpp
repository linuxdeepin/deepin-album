#include "albumControl.h"
#include "dbmanager/dbmanager.h"
#include "fileMonitor/fileinotifygroup.h"

#include <QStandardPaths>
#include <QFileInfo>
#include <QUrl>
#include <QFileDialog>
#include <QProcess>
#include <QRegularExpression>
#include <QDirIterator>
#include <QCoreApplication>

AlbumControl::AlbumControl(QObject *parent)
    : QObject(parent)
{
    initMonitor();
    initDeviceMonitor();

}

AlbumControl::~AlbumControl()
{

}

DBImgInfo AlbumControl::getDBInfo(const QString &srcpath, bool isVideo)
{
    using namespace LibUnionImage_NameSpace;
    QFileInfo srcfi(srcpath);
    DBImgInfo dbi;
    dbi.filePath = srcpath;
    dbi.importTime = QDateTime::currentDateTime();
    if (isVideo) {
        dbi.itemType = ItemTypeVideo;
        //获取视频信息
        MovieInfo movieInfo = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(srcpath));
        //对视频信息缓存
        m_movieInfos[srcpath]=movieInfo;

        dbi.changeTime = srcfi.lastModified();

        if (movieInfo.creation.isValid()) {
            dbi.time = movieInfo.creation;
        } else if (srcfi.birthTime().isValid()) {
            dbi.time = srcfi.birthTime();
        } else if (srcfi.metadataChangeTime().isValid()) {
            dbi.time = srcfi.metadataChangeTime();
        } else {
            dbi.time = dbi.changeTime;
        }
    } else {
        auto mds = getAllMetaData(srcpath);
        QString value = mds.value("DateTimeOriginal");
        dbi.itemType = ItemTypePic;
        dbi.changeTime = QDateTime::fromString(mds.value("DateTimeDigitized"), "yyyy/MM/dd hh:mm");
        if (!value.isEmpty()) {
            dbi.time = QDateTime::fromString(value, "yyyy/MM/dd hh:mm");
        } else if (srcfi.birthTime().isValid()) {
            dbi.time = srcfi.birthTime();
        } else if (srcfi.metadataChangeTime().isValid()) {
            dbi.time = srcfi.metadataChangeTime();
        } else {
            dbi.time = dbi.changeTime;
        }
    }
    return dbi;
}

void AlbumControl::initDeviceMonitor()
{
    m_vfsManager = new DGioVolumeManager(this);
    m_diskManager = new DDiskManager(this);

    connect(m_vfsManager, &DGioVolumeManager::mountAdded, this, &AlbumControl::onVfsMountChangedAdd);
    connect(m_vfsManager, &DGioVolumeManager::mountRemoved, this, &AlbumControl::onVfsMountChangedRemove);
    connect(m_vfsManager, &DGioVolumeManager::volumeAdded, [](QExplicitlySharedDataPointer<DGioVolume> vol) {
        if (vol->volumeMonitorName().contains(QRegularExpression("(MTP|GPhoto2|Afc)$"))) {
            vol->mount();
        }
    });

    QList<QExplicitlySharedDataPointer<DGioMount> > list = getVfsMountList();
    for( auto mount : list ){
        onVfsMountChangedAdd(mount);
    }
}

bool AlbumControl::findPicturePathByPhone(QString &path)
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
            for (QFileInfo subTempFileInfo : subFileInfoList) {
                if (subTempFileInfo.fileName().compare(ALBUM_PATHNAME_BY_PHONE) == 0) {
                    path = subTempFileInfo.absoluteFilePath();
                    return true;
                }
            }
        }
    }
    return false;
}

QList<QExplicitlySharedDataPointer<DGioMount> > AlbumControl::getMounts()
{
    static QMutex mutex;
    mutex.lock();
    auto result = DGioVolumeManager::getMounts();
    mutex.unlock();
    return result;
}

void AlbumControl::getAllInfos()
{
    m_infoList = DBManager::instance()->getAllInfos();
}

QStringList AlbumControl::getAllPaths()
{
    QStringList pathList;
    QStringList list = DBManager::instance()->getAllPaths();
    for (QString path : list) {
        pathList << "file://" + path;
    }
    return pathList;
}

void AlbumControl::importAllImagesAndVideos(const QList< QUrl > &paths)
{
    QStringList localpaths;
    DBImgInfoList dbInfos;
    for (QUrl path : paths) {
        localpaths << path.toLocalFile();
    }
    QStringList curAlbumImgPathList = getAllPaths();
    for (QString imagePath : localpaths) {
        bool bIsVideo = LibUnionImage_NameSpace::isVideo(imagePath);
        if (!bIsVideo && !LibUnionImage_NameSpace::imageSupportRead(imagePath)) {
            continue;
        }
        QFileInfo srcfi(imagePath);
        if (!srcfi.exists()) {  //当前文件不存在
            continue;
        }
        if (curAlbumImgPathList.contains(imagePath)) {
        }
        DBImgInfo info =  getDBInfo(imagePath, bIsVideo);
        dbInfos << info;
    }
    std::sort(dbInfos.begin(), dbInfos.end(), [](const DBImgInfo & lhs, const DBImgInfo & rhs) {
        return lhs.changeTime > rhs.changeTime;
    });
    //已全部存在，无需导入
    if (dbInfos.size() > 0) {
        //导入图片数据库ImageTable3
        DBManager::instance()->insertImgInfos(dbInfos);
        emit sigRefreshImportAlbum();
    }

}

QStringList AlbumControl::getAllTimelinesTitle(const int &filterType)
{
    return getTimelinesTitle(TimeLineEnum::All, filterType);
}

QStringList AlbumControl::getTimelinesTitlePaths(const QString &titleName, const int &filterType)
{
    QStringList pathsList;
    DBImgInfoList dblist;
    if (m_yearDateMap.keys().contains(titleName)) {
        dblist = m_yearDateMap.value(titleName);
    } else if (m_monthDateMap.keys().contains(titleName)) {
        dblist = m_monthDateMap.value(titleName);
    } else if (m_dayDateMap.keys().contains(titleName)) {
        dblist = m_dayDateMap.value(titleName);
    } else {
        dblist = m_timeLinePathsMap.value(titleName);
    }
    for (DBImgInfo info : dblist) {
        if (filterType == 2 && info.itemType == ItemTypePic) {
            continue ;
        } else if (filterType == 1 && info.itemType == ItemTypeVideo) {
            continue ;
        }
        pathsList << "file://" + info.filePath;
    }
    return pathsList;
}

QStringList AlbumControl::getAllImportTimelinesTitle(const int &filterType)
{
    return getTimelinesTitle(TimeLineEnum::Import, filterType);
}

QVariantMap AlbumControl::getTimelinesTitleInfos(const int &filterType)
{
    QVariantMap reMap;
    QStringList alltitles =  getTimelinesTitle(TimeLineEnum::All, filterType);
    for (QString titleName : alltitles) {
        QVariantList list;
        DBImgInfoList dbInfoList = m_timeLinePathsMap.value(titleName);
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
    return reMap;
}


QStringList AlbumControl::getYearTimelinesTitle(const int &filterType)
{
    return getTimelinesTitle(TimeLineEnum::Year, filterType);
}

QVariantMap AlbumControl::getYearTimelinesInfos(const int &filterType)
{
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
    return reMap;
}

QStringList AlbumControl::getMonthTimelinesTitle(const int &filterType)
{
    return getTimelinesTitle(TimeLineEnum::Month, filterType);
}

QVariantMap AlbumControl::getMonthTimelinesInfos(const int &filterType)
{
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
    return reMap;
}

QStringList AlbumControl::getDayTimelinesTitle(const int &filterType)
{
    return getTimelinesTitle(TimeLineEnum::Day, filterType);
}

QVariantMap AlbumControl::getDayTimelinesInfos(const int &filterType)
{
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
    return reMap;
}

QStringList AlbumControl::getTimelinesTitle(TimeLineEnum timeEnum, const int &filterType)
{
    //设置需要查询的图片视频或者是全部
    ItemType typeItem = ItemType::ItemTypeNull;
    if (filterType == 1) {
        typeItem = ItemType::ItemTypePic;
    } else if (filterType == 2) {
        typeItem = ItemType::ItemTypeVideo;
    }
    //已导入
    if (timeEnum == Import) {
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
                    date = QString(QObject::tr("%1/%2/%3/%4/%5")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]).arg(datelist[3]).arg(datelist[4]);
                    m_importTimeLinePathsMap.insertMulti(date, ImgInfoList);
                }

            }
        }
        //倒序
        QStringList relist;
        for (int i = m_importTimeLinePathsMap.keys().count() - 1 ; i >= 0 ; i--) {
            relist << m_importTimeLinePathsMap.keys().at(i);
        }

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
        QString date;
        if (datelist.count() > 4) {
            date = QString(QObject::tr("%1/%2/%3/%4/%5")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]).arg(datelist[3]).arg(datelist[4]);

            switch (timeEnum) {
            case TimeLineEnum::Year :
                if (ImgInfoList.size() > 0) {
                    tmpInfoMap.insertMulti(QString(QObject::tr("%1/").arg(datelist[0])), ImgInfoList);
                }
                m_yearDateMap = tmpInfoMap;
                break;
            case TimeLineEnum::Month :
                if (ImgInfoList.size() > 0) {
                    tmpInfoMap.insertMulti(QString(QObject::tr("%1/%2").arg(datelist[0]).arg(datelist[1])), ImgInfoList);
                }
                m_monthDateMap = tmpInfoMap;
                break;
            case TimeLineEnum::Day :
                if (ImgInfoList.size() > 0) {
                    tmpInfoMap.insertMulti(QString(QObject::tr("%1/%2/%3").arg(datelist[0]).arg(datelist[1]).arg(datelist[2])), ImgInfoList);
                }
                m_dayDateMap = tmpInfoMap;
                break;
            case TimeLineEnum::All :
                if (ImgInfoList.size() > 0) {
                    tmpInfoMap.insertMulti(QString(QObject::tr("%1/%2/%3/%4/%5")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]).arg(datelist[3]).arg(datelist[4]), ImgInfoList);
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

    return relist;
}

void AlbumControl::initMonitor()
{
    m_fileInotifygroup = new FileInotifyGroup(this) ;
    connect(m_fileInotifygroup, &FileInotifyGroup::sigMonitorChanged, this, &AlbumControl::slotMonitorChanged);
    connect(m_fileInotifygroup, &FileInotifyGroup::sigMonitorDestroyed, this, &AlbumControl::slotMonitorDestroyed);
    startMonitor();
}

void AlbumControl::startMonitor()
{
    //启动路径监控
    auto monitorPathsTuple = DBManager::getDefaultNotifyPaths_group();
    const QList<QStringList> &paths = std::get<0>(monitorPathsTuple);
    const QStringList &albumNames = std::get<1>(monitorPathsTuple);
    const QList<int> &UIDs = std::get<2>(monitorPathsTuple);
    for (int i = 0; i != UIDs.size(); ++i) {
        m_fileInotifygroup->startWatch(paths.at(i), albumNames.at(i), UIDs.at(i));
    }

}

bool AlbumControl::checkIfNotified(const QString &dirPath)
{
    return DBManager::instance()->checkCustomAutoImportPathIsNotified(dirPath);
}

void AlbumControl::slotMonitorChanged(QStringList fileAdd, QStringList fileDelete, QString album, int UID)
{
    //直接删除图片
    DBManager::instance()->removeImgInfos(fileDelete);
    AlbumDBType atype = AlbumDBType::AutoImport;
    DBManager::instance()->insertIntoAlbum(UID, fileAdd, atype);

    DBImgInfoList dbInfos;
    for (QString path : fileAdd) {

        bool bIsVideo = LibUnionImage_NameSpace::isVideo(path);
        DBImgInfo info =  getDBInfo(path, bIsVideo);
        dbInfos << info;
    }
    //导入图片数据库ImageTable3
    DBManager::instance()->insertImgInfos(dbInfos);

    emit sigRefreshCustomAlbum(UID);
    emit sigRefreshImportAlbum();

}

void AlbumControl::slotMonitorDestroyed(int UID)
{
    //文件夹删除
    emit sigDeleteCustomAlbum(UID);
}

void AlbumControl::onVfsMountChangedAdd(QExplicitlySharedDataPointer<DGioMount> mount)
{
    qDebug() << "挂载设备增加：" << mount->name();

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
        QString rename = "";
        qDebug()<<QUrl(mount->getRootFile()->uri()) ;
        rename = m_durlAndNameMap[mount->getRootFile()->path()];
        if ("" == rename) {
            rename = mount->name();
        }
        if ("" == rename) {
            rename = mount->name();
        }
        m_durlAndNameMap[mount->getRootFile()->path()] = rename ;
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
        qDebug()<<bFind;
        //路径存在
        if(bFind){
            m_mounts << mount;
            //挂在路径
            sltLoadMountFileList(strPath);
            //名称
            rename ;
        }
    }
     emit sigMountsChange();
}

void AlbumControl::onVfsMountChangedRemove(QExplicitlySharedDataPointer<DGioMount> mount)
{
    QString uri = mount->getRootFile()->uri();
    QString strPath = mount->getDefaultLocationFile()->path();
    m_durlAndNameMap.erase(m_durlAndNameMap.find(mount->getRootFile()->path()));
    m_PhonePicFileMap.erase(m_PhonePicFileMap.find(mount->getRootFile()->path()));

    for (auto mountLoop : m_mounts) {
        QString uriLoop = mountLoop->getRootFile()->uri();
        if (uri == uriLoop) {
            qDebug() << "Already has this device in mount list. uri:" << uriLoop;
            m_mounts.removeOne(mountLoop);
            break;
        }
    }
    emit sigMountsChange();
}

void AlbumControl::sltLoadMountFileList(const QString &path)
{
    QString strPath = path;
    if (!m_PhonePicFileMap.contains(strPath)) {
        //获取所选文件类型过滤器
        QStringList filters;
        for (QString i : LibUnionImage_NameSpace::unionImageSupportFormat()) {
            filters << "*." + i;
        }

        for (QString i : LibUnionImage_NameSpace::videoFiletypes()) {
            filters << "*." + i;
        }
        //定义迭代器并设置过滤器，包括子目录：QDirIterator::Subdirectories
        QDirIterator dir_iterator(strPath,
                                  filters,
                                  QDir::Files | QDir::NoSymLinks,
                                  QDirIterator::Subdirectories);
        QStringList allfiles;
        while (dir_iterator.hasNext()) {
            dir_iterator.next();
            QFileInfo fileInfo = dir_iterator.fileInfo();
            allfiles << fileInfo.filePath();
        }
        //重置标志位，可以执行线程
        m_PhonePicFileMap[strPath] = allfiles;
        //发送信号
    } else {
        //已加载过的设备，直接发送缓存的路径
    }
}

const QList<QExplicitlySharedDataPointer<DGioMount> > AlbumControl::getVfsMountList()
{
    QList<QExplicitlySharedDataPointer<DGioMount> > result;
    const QList<QExplicitlySharedDataPointer<DGioMount> > mounts = getMounts();
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

QStringList AlbumControl::getImportTimelinesTitlePaths(const QString &titleName, const int &filterType)
{
    QStringList pathsList;
    DBImgInfoList dbInfoList = m_importTimeLinePathsMap.value(titleName);
    for (DBImgInfo info : dbInfoList) {
        if (filterType == 2 && info.itemType == ItemTypePic) {
            continue ;
        } else if (filterType == 1 && info.itemType == ItemTypeVideo) {
            continue ;
        }
        pathsList << "file://" + info.filePath;
    }
    return pathsList;
}

QVariantMap AlbumControl::getImportTimelinesTitleInfos(const int &filterType)
{
    QVariantMap reMap;
    QStringList alltitles = getAllImportTimelinesTitle(filterType);
    for (QString titleName : alltitles) {
        QVariantList list;
        DBImgInfoList dbInfoList = m_importTimeLinePathsMap.value(titleName);
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
    return reMap;
}

QVariantMap AlbumControl::getAlbumInfos(const int &albumId, const int &filterType)
{
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
        reMap.insert(title, list);
    }
    return reMap;
}

QVariantMap AlbumControl::getTrashAlbumInfos(const int &filterType)
{
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
        reMap.insert(title, list);
    }
    return reMap;
}

bool AlbumControl::addCustomAlbumInfos(const int &albumId, const QList<QUrl> &urls, const int &imoprtType)
{
    Q_UNUSED(imoprtType);

    bool bRet = false;
    QStringList paths;
    for (QUrl url : urls) {
        paths << url.toLocalFile();
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
    return bRet;
}

int AlbumControl::getCount()
{
    return DBManager::instance()->getImgsCount();
}

void AlbumControl::insertTrash(const QList< QUrl > &paths)
{
    QStringList tmpList;
    for (QUrl url : paths) {
        tmpList << url.toLocalFile();
    }
    DBImgInfoList infos;
    for (QString path : tmpList) {
        infos << DBManager::instance()->getInfoByPath(path);
    }
    DBManager::instance()->insertTrashImgInfos(infos, false);
}

void AlbumControl::removeTrashImgInfos(const QList< QUrl > &paths)
{
    QStringList localPaths ;
    for (QUrl path : paths) {
        localPaths << path.toLocalFile();
    }
    DBManager::instance()->removeTrashImgInfos(localPaths);
}

QStringList AlbumControl::recoveryImgFromTrash(const QStringList &paths)
{
    QStringList localPaths ;
    for (QUrl path : paths) {
        localPaths << path.toLocalFile();
    }
    return DBManager::instance()->recoveryImgFromTrash(localPaths);
}

void AlbumControl::deleteImgFromTrash(const QStringList &paths)
{
    QStringList localPaths ;
    for (QUrl path : paths) {
        localPaths << path.toLocalFile();
    }
    DBManager::instance()->removeTrashImgInfos(localPaths);
}

void AlbumControl::insertCollection(const QList< QUrl > &paths)
{
    QStringList tmpList;
    for (QUrl url : paths) {
        tmpList << url.toLocalFile();
    }
    DBImgInfoList infos;
    for (QString path : tmpList) {
        infos << DBManager::instance()->getInfoByPath(path);
    }
}

void AlbumControl::createAlbum(const QString &newName)
{
    QString createAlbumName = getNewAlbumName(newName);
    int createUID = DBManager::instance()->createAlbum(createAlbumName, QStringList(" "));
    DBManager::instance()->insertIntoAlbum(createUID, QStringList(" "));
}

QList < int > AlbumControl::getAllCustomAlbumId()
{
    QMap < int, QString > customAlbum;
    QList<std::pair<int, QString>>  tmpList = DBManager::instance()->getAllAlbumNames(Custom);
    for (std::pair<int, QString> tmpPair : tmpList) {
        customAlbum.insert(tmpPair.first, tmpPair.second);
    }
    m_customAlbum = customAlbum;
    return customAlbum.keys();
}

QList < QString > AlbumControl::getAllCustomAlbumName()
{
    QMap < int, QString > customAlbum;
    QList<std::pair<int, QString>>  tmpList = DBManager::instance()->getAllAlbumNames(Custom);
    for (std::pair<int, QString> tmpPair : tmpList) {
        customAlbum.insert(tmpPair.first, tmpPair.second);
    }
    m_customAlbum = customAlbum;
    return customAlbum.values();
}

QString AlbumControl::getCustomAlbumByUid(const int &index)
{
    // 从数据获取我的收藏单词不对，为保证V20数据库兼容性，在此做特殊特殊处理
    if (0 == index)
        return "Favorites";

    return DBManager::instance()->getAlbumNameFromUID(index);
}

DBImgInfoList AlbumControl::getTrashInfos(const int &filterType)
{
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
    return DBManager::instance()->getInfosByAlbum(DBManager::SpUID::u_Favorite, false);
}

DBImgInfoList AlbumControl::getScreenCaptureInfos()
{
    return DBManager::instance()->getInfosByAlbum(DBManager::SpUID::u_ScreenCapture, false);
}

DBImgInfoList AlbumControl::getCameraInfos()
{
    return DBManager::instance()->getInfosByAlbum(DBManager::SpUID::u_Camera, false);
}

QString AlbumControl::getDeleteFullPath(const QString &hash, const QString &fileName)
{
    //防止文件过长,采用只用hash的名称;
    return albumGlobal::DELETE_PATH + "/" + hash + "." + QFileInfo(fileName).suffix();
}

//需求变更：允许相册重名，空字符串返回Unnamed，其余字符串返回本名
const QString AlbumControl::getNewAlbumName(const QString &baseName)
{
    QString albumName;
    if (baseName.isEmpty()) {
        albumName = tr("Unnamed");
    } else {
        albumName = baseName;
    }
    return static_cast<const QString>(albumName);
}

bool AlbumControl::canFavorite(const QStringList &pathList)
{
    bool bCanFavorite = false;
    for (int i = 0; i < pathList.size(); i++) {
        if (!pathList[i].isEmpty() && !photoHaveFavorited(pathList[i])) {
            bCanFavorite = true;
            break;
        }
    }

    return bCanFavorite;
}

bool AlbumControl::photoHaveFavorited(const QString &path)
{
    return DBManager::instance()->isImgExistInAlbum(DBManager::SpUID::u_Favorite, QUrl(path).toLocalFile());
}

QVariantMap AlbumControl::getPathsInfoMap(const QString &path)
{
    QVariantMap reMap;
    QString localPath = QUrl(path).toLocalFile();
    DBImgInfo info = DBManager::instance()->getInfoByPath(localPath);
    reMap.insert("url", "file://" + info.filePath);
    reMap.insert("filePath", info.filePath);
    reMap.insert("pathHash", info.pathHash);
    reMap.insert("remainDays", info.remainDays);
    if (info.itemType == ItemTypePic) {
        reMap.insert("itemType", "pciture");
    } else if (info.itemType == ItemTypeVideo) {
        reMap.insert("itemType", "video");
    } else {
        reMap.insert("itemType", "other");
    }
    return reMap;
}

QStringList AlbumControl::getPathsInfoList(const QString &path)
{
    QStringList reList;
    QString localPath = QUrl(path).toLocalFile();
    DBImgInfo info = DBManager::instance()->getInfoByPath(localPath);
    reList << "file://" + info.filePath;
    reList << info.filePath ;
    reList << info.pathHash ;
    reList << QString::number(info.remainDays);
    if (info.itemType == ItemTypePic) {
        reList << "pciture";
    } else if (info.itemType == ItemTypeVideo) {
        reList << "video";
    } else {
        reList << "other";
    }
    return reList;
}

QString AlbumControl::getPathsInfoData(const QString &path, const QString &key)
{
    QString value;
    QString localPath = QUrl(path).toLocalFile();
    DBImgInfo info = DBManager::instance()->getInfoByPath(localPath);
    if (key.contains("url")) {
        value = "file://" + info.filePath;
    } else if (key.contains("filePath")) {
        value = info.filePath;
    } else if (key.contains("pathHash")) {
        value = info.pathHash;
    } else if (key.contains("remainDays")) {
        value = QString::number(info.remainDays);
    } else if (key.contains("itemType")) {
        if (info.itemType == ItemTypePic) {
            value = "pciture";
        } else if (info.itemType == ItemTypeVideo) {
            value = "video";
        } else {
            value = "other";
        }
    } else if (key.contains("time")) {
        value = info.time.toString("yyyy.MM.dd.hh.mm");
    } else if (key.contains("changeTime")) {
        value = info.changeTime.toString("yyyy.MM.dd.hh.mm");
    } else if (key.contains("importTime")) {
        value = info.importTime.toString("yyyy.MM.dd.hh.mm");
    }
    return value;
}

int AlbumControl::getCustomAlbumInfoConut(const int &albumId, const int &filterType)
{
    int rePicVideoConut = 0;
    DBImgInfoList dbInfoList = DBManager::instance()->getInfosByAlbum(albumId, false);
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
        rePicVideoConut++;
    }
    return rePicVideoConut;
}

int AlbumControl::getAllInfoConut(const int &filterType)
{
    ItemType type = ItemTypeNull;
    if (filterType == 2) {
        type = ItemTypeVideo ;
    }
    if (filterType == 1) {
        type = ItemTypePic ;
    }
    return DBManager::instance()->getImgsCount(type);
}

int AlbumControl::getTrashInfoConut(const int &filterType)
{
    DBImgInfoList allTrashInfos = getTrashInfos(filterType);
    return allTrashInfos.size();
}

void AlbumControl::removeAlbum(int UID)
{
    DBManager::instance()->removeAlbum(UID);
}

void AlbumControl::removeFromAlbum(int UID, const QStringList &paths)
{
    AlbumDBType atype = AlbumDBType::Custom;
    if (UID == 0) {
        atype = AlbumDBType::Favourite;
    }
    QStringList localPaths ;
    for (QString path : paths) {
        localPaths << QUrl(path).toLocalFile();
    }
    DBManager::instance()->removeFromAlbum(UID, localPaths, atype);
}

bool AlbumControl::insertIntoAlbum(int UID, const QStringList &paths)
{
    AlbumDBType atype = AlbumDBType::Custom;
    if (UID == 0) {
        atype = AlbumDBType::Favourite;
    }
    QStringList localPaths ;
    for (QString path : paths) {
        localPaths << QUrl(path).toLocalFile();
    }
    return DBManager::instance()->insertIntoAlbum(UID, localPaths, atype);
}

QVariant AlbumControl::searchPicFromAlbum(int UID, const QString &keywords, bool useAI)
{
    DBImgInfoList dbInfos;
    if (useAI) { //使用AI进行分析
        ;
    } else { //不使用AI分析，直接按文件路径搜索
        if (UID == -1) {
            dbInfos = DBManager::instance()->getInfosForKeyword(keywords);
        } else if (UID == -2) {
            dbInfos = DBManager::instance()->getTrashInfosForKeyword(keywords);
        } else {
            dbInfos = DBManager::instance()->getInfosForKeyword(UID, keywords);
        }
    }

    QStringList paths;
    std::transform(dbInfos.begin(), dbInfos.end(), std::back_inserter(paths), [](const DBImgInfo & info) {
        return "file://" + info.filePath;
    });

    return paths;
}

QStringList AlbumControl::imageCanExportFormat(const QString &path)
{
    QString localPath = QUrl(path).toLocalFile();
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
        if (!info.suffix().isEmpty())
            formats << info.suffix();
    }
    return formats;

}

bool AlbumControl::saveAsImage(const QString &path, const QString &saveName, int index, const QString &fileFormat, int pictureQuality, const QString &saveFolder)
{
    bool bRet = false;
    QString localPath = QUrl(path).toLocalFile();
    QString savePath;
    QString finalSaveFolder;
    switch (index) {
    case 0:
        finalSaveFolder = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
        break;
    case 1:
        finalSaveFolder = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        break;
    case 2:
        finalSaveFolder = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        break;
    case 3:
        finalSaveFolder = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        break;
    case 4:
        finalSaveFolder = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        break;
    case 5:
        finalSaveFolder = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
        break;
    default :
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

        QFileInfo fileinfo(savePath);
        if (fileinfo.exists() && !fileinfo.isDir()) {
            //目标位置与原图位置相同则直接返回
            if (localPath == savePath) {
                return true;
            }
            //目标位置与原图位置不同则先删除再复制
            if (QFile::remove(savePath)) {
                bRet = QFile::copy(localPath, savePath);
            }
        } else {
            bRet = QFile::copy(localPath, savePath);
        }
    } else {
        QImage m_saveImage;
        QString errMsg;
        LibUnionImage_NameSpace::loadStaticImageFromFile(localPath, m_saveImage, errMsg);
        bRet = m_saveImage.save(savePath, fileFormat.toUpper().toLocal8Bit().data(), pictureQuality);
    }


    return bRet;
}

QString AlbumControl::getFolder()
{
    QFileDialog dialog;
    QString fileDir;
    dialog.setDirectory(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    dialog.setViewMode(QFileDialog::Detail);
    dialog.setFileMode(QFileDialog::DirectoryOnly);
    if (dialog.exec()) {
        fileDir = dialog.selectedFiles().first();
    }
    return fileDir;
}

void AlbumControl::openDeepinMovie(const QString &path)
{
    QString localPath = QUrl(path).toLocalFile();
    QProcess *process = new QProcess(this);
    QStringList arguments;
    arguments << localPath;
    bool isopen = process->startDetached("deepin-movie", arguments);
    if (!isopen) {
        arguments.clear();
        arguments << "-o" << localPath ;
        process->startDetached("dde-file-manager", arguments);
    }
}

QString AlbumControl::getFileTime(const QString &path1, const QString &path2)
{
    auto time1 = DBManager::instance()->getFileImportTime(QUrl(path1).toLocalFile());
    auto time2 = DBManager::instance()->getFileImportTime(QUrl(path2).toLocalFile());

    auto str1 = time1.toString("yyyy/MM/dd");
    auto str2 = time2.toString("yyyy/MM/dd");

    if(time1 < time2) {
        return str1 + "-" + str2;
    } else {
        return str2 + "-" + str1;
    }
}

QString AlbumControl::getMovieInfo(const QString key, const QString &path)
{
    QString value="";
    if(!path.isEmpty()){
        QString localPath = QUrl(path).toLocalFile();
        if (!m_movieInfos.keys().contains(localPath)){
            MovieInfo movieInfo = MovieService::instance()->getMovieInfo(QUrl::fromLocalFile(localPath));
            //对视频信息缓存
            m_movieInfos[localPath]= movieInfo;
        }
        MovieInfo movieInfo = m_movieInfos.value(localPath);
        if(QString("Video CodecID").contains(key)){
            value = movieInfo.vCodecID;
        }
        else if(QString("Video CodeRate").contains(key)){
            value = movieInfo.vCodeRate == 0 ? "-" : QString::number(movieInfo.vCodeRate) + " kbps";
        }
        else if(QString("FPS").contains(key)){
            value = movieInfo.fps == 0 ? "-" : QString::number(movieInfo.fps) + " fps";
        }
        else if(QString("Proportion").contains(key)){
            value = movieInfo.proportion <= 0 ? "-" : QString::number(movieInfo.proportion);
        }
        else if(QString("Resolution").contains(key)){
            value = movieInfo.resolution;
        }
        else if(QString("Audio CodecID").contains(key)){
            value = movieInfo.aCodeID;
        }
        else if(QString("Audio CodeRate").contains(key)){
            value = movieInfo.aCodeRate == 0 ? "-" : QString::number(movieInfo.aCodeRate) + " kbps";
        }
        else if(QString("Audio digit").contains(key)){
            value = movieInfo.aDigit;
        }
        else if(QString("Channels").contains(key)){
            value = movieInfo.channels == 0 ? "-" : QString::number(movieInfo.channels) + tr("Channel");
        }
        else if(QString("Sampling").contains(key)){
            value = movieInfo.sampling == 0 ? "-" : QString::number(movieInfo.sampling) + " hz";
        }
        else if(QString("DateTimeOriginal").contains(key)){
            QFileInfo info(localPath);
            if (info.lastModified().isValid()) {
                value = info.lastModified().toString("yyyy/MM/dd HH:mm");
            } else if (info.birthTime().isValid()) {
                value = info.birthTime().toString("yyyy/MM/dd HH:mm");
            }
        }
        else if(QString("Type").contains(key)){
            value = movieInfo.fileType.toLower();
        }
        else if(QString("Size").contains(key)){
            value = movieInfo.sizeStr();
        }
        else if(QString("Duration").contains(key)){
            value = movieInfo.duration;
        }
        else if(QString("Path").contains(key)){
            value = movieInfo.filePath;
        }
    }
    return value;
}

int AlbumControl::getYearCount(const QString &year)
{
    return DBManager::instance()->getYearCount(year);
}

QStringList AlbumControl::getYears()
{
    return DBManager::instance()->getYears();
}

int AlbumControl::getMonthCount(const QString &year, const QString &month)
{
    return DBManager::instance()->getMonthCount(year, month);
}

QStringList AlbumControl::getMonths()
{
    return DBManager::instance()->getMonths();
}

QStringList AlbumControl::getDeviceNames()
{
    return m_durlAndNameMap.values();
}

QStringList AlbumControl::getDevicePaths()
{
    return m_durlAndNameMap.keys();
}

QString AlbumControl::getDeviceName(const QString &devicePath)
{
    return m_durlAndNameMap.value(devicePath);
}

QStringList AlbumControl::getDevicePicPaths(const QString &path)
{
    QStringList pathsList;
    QStringList list = m_PhonePicFileMap.value(path);
    for(QString path : list ){
        pathsList << "file://" + path;
    }
    return pathsList;
}
