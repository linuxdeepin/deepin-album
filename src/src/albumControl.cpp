#include "albumControl.h"
#include "dbmanager/dbmanager.h"
#include "imageengine/movieservice.h"
#include "fileMonitor/fileinotifygroup.h"

#include <QUrl>

DBImgInfo getDBInfo(const QString &srcpath, bool isVideo)
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

AlbumControl::AlbumControl(QObject *parent)
    : QObject(parent)
{
    initMonitor();
}

AlbumControl::~AlbumControl()
{

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
    connect(m_fileInotifygroup , &FileInotifyGroup::sigMonitorChanged,this,&AlbumControl::slotMonitorChanged);
    connect(m_fileInotifygroup , &FileInotifyGroup::sigMonitorDestroyed,this,&AlbumControl::slotMonitorDestroyed);
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
    DBManager::instance()->insertIntoAlbum( UID , fileAdd , atype);

    DBImgInfoList dbInfos;
    for (QString path :fileAdd){

        bool bIsVideo = LibUnionImage_NameSpace::isVideo(path);
        DBImgInfo info =  getDBInfo(path, bIsVideo);
        dbInfos << info;
    }
    //导入图片数据库ImageTable3
    DBManager::instance()->insertImgInfos(dbInfos);

}

void AlbumControl::slotMonitorDestroyed(int UID)
{

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
