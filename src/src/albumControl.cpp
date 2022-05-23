#include "albumControl.h"
#include "dbmanager/dbmanager.h"
#include "imageengine/movieservice.h"
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

AlbumControl::AlbumControl(QObject *parent) : QObject(parent)
{

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
    for(QString path : list){
        pathList << "file://"+ path;
    }
    return pathList;
}

void AlbumControl::importAllImagesAndVideos(const QList< QUrl > &paths)
{
    QStringList localpaths;
    DBImgInfoList dbInfos;
    for(QUrl path : paths){
        localpaths << path.toLocalFile();
    }
    QStringList curAlbumImgPathList = getAllPaths();
    for(QString imagePath : localpaths){
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

QStringList AlbumControl::getAllTimelinesTitle()
{
    return getTimelinesTitle(TimeLineEnum::All);
}

QStringList AlbumControl::getTimelinesTitlePaths(const QString &titleName)
{
    QStringList pathsList;
    DBImgInfoList dblist;
    if(m_yearDateMap.keys().contains(titleName)){
        dblist = m_yearDateMap.value(titleName);
    } else if (m_monthDateMap.keys().contains(titleName)) {
        dblist = m_monthDateMap.value(titleName);
    } else if (m_dayDateMap.keys().contains(titleName)) {
        dblist = m_dayDateMap.value(titleName);
    } else {
        dblist = m_timeLinePathsMap.value(titleName);
    }
    for(DBImgInfo info : dblist){
        pathsList << info.filePath;
    }
    return pathsList;
}

QStringList AlbumControl::getYearTimelinesTitle()
{
    return getTimelinesTitle(TimeLineEnum::Year);
}

QStringList AlbumControl::getMonthTimelinesTitle()
{
    return getTimelinesTitle(TimeLineEnum::Month);
}

QStringList AlbumControl::getDayTimelinesTitle()
{
    return getTimelinesTitle(TimeLineEnum::Day);
}

QStringList AlbumControl::getTimelinesTitle(TimeLineEnum timeEnum)
{
    m_timelines = DBManager::instance()->getAllTimelines();
    QMap < QString, DBImgInfoList > tmpInfoMap;

    QList<QDateTime> tmpDateList = m_timelines ;

    for(QDateTime time : tmpDateList){
        //获取当前时间照片
        DBImgInfoList ImgInfoList = DBManager::instance()->getInfosByTimeline(time);
        QStringList datelist = time.toString("yyyy.MM.dd").split(".");
        //加时间线标题
        QString date;
        if (datelist.count() > 2) {
            date = QString(QObject::tr("%1/%2/%3")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]);

            switch (timeEnum) {
            case TimeLineEnum::Year :
                for(DBImgInfo info : ImgInfoList){
                     tmpInfoMap[ QString(QObject::tr("%1/").arg(datelist[0])) ].push_back(info);
                }
                m_yearDateMap = tmpInfoMap;
                break;
            case TimeLineEnum::Month :
                for(DBImgInfo info : ImgInfoList){
                     tmpInfoMap[ QString(QObject::tr("%1/%2").arg(datelist[0]).arg(datelist[1])) ].push_back(info);
                }
                m_monthDateMap = tmpInfoMap;
                break;
            case TimeLineEnum::Day :
                for(DBImgInfo info : ImgInfoList){
                     tmpInfoMap[ QString(QObject::tr("%1/%2/%3").arg(datelist[0]).arg(datelist[1]).arg(datelist[2])) ].push_back(info);
                }
                m_dayDateMap = tmpInfoMap;
                break;
            default:
                date = QString(QObject::tr("%1/%2/%3")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]);
                tmpInfoMap.insertMulti(date,ImgInfoList);
                m_timeLinePathsMap = tmpInfoMap;
                break;
            }
        }
    }

    return tmpInfoMap.keys();
}


QStringList AlbumControl::getAllImportTimelinesTitle()
{
    QStringList list;
    m_importTimelines = DBManager::instance()->getImportTimelines();
    m_importTimeLinePathsMap.clear();
    QList<QDateTime> tmpDateList = m_importTimelines ;

    for(QDateTime time : tmpDateList){
        //获取当前时间照片
        DBImgInfoList ImgInfoList = DBManager::instance()->getInfosByImportTimeline(time);
        QStringList datelist = time.toString("yyyy.MM.dd").split(".");
        //加时间线标题
        QString date;
        if (datelist.count() > 2) {
            date = QString(QObject::tr("%1/%2/%3")).arg(datelist[0]).arg(datelist[1]).arg(datelist[2]);
        }
        list << date;
        m_importTimeLinePathsMap.insertMulti(date,ImgInfoList);
    }

    return list;
}

QStringList AlbumControl::getImportTimelinesTitlePaths(const QString &titleName)
{
    QStringList pathsList;
    DBImgInfoList dbInfoList = m_importTimeLinePathsMap.value(titleName);
    for(DBImgInfo info : dbInfoList){
        pathsList << info.filePath;
    }
    return pathsList;
}

int AlbumControl::getCount()
{
    return DBManager::instance()->getImgsCount();
}

void AlbumControl::insertTrash(const QList< QUrl > &paths)
{
    QStringList tmpList;
    for(QUrl url : paths){
        tmpList << url.toLocalFile();
    }
    DBImgInfoList infos;
    for(QString path : tmpList){
        infos << DBManager::instance()->getInfoByPath(path);
    }
    DBManager::instance()->insertTrashImgInfos(infos,false);
}

void AlbumControl::insertCollection(const QList< QUrl > &paths)
{
    QStringList tmpList;
    for(QUrl url : paths){
        tmpList << url.toLocalFile();
    }
    DBImgInfoList infos;
    for(QString path : tmpList){
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
    QMap < int ,QString > customAlbum;
    QList<std::pair<int, QString>>  tmpList = DBManager::instance()->getAllAlbumNames(Custom);
    for( std::pair<int, QString> tmpPair :tmpList){
       customAlbum.insert(tmpPair.first,tmpPair.second);
    }
    m_customAlbum = customAlbum;
    return customAlbum.keys();
}

QList < QString > AlbumControl::getAllCustomAlbumName()
{
    QMap < int ,QString > customAlbum;
    QList<std::pair<int, QString>>  tmpList = DBManager::instance()->getAllAlbumNames(Custom);
    for( std::pair<int, QString> tmpPair :tmpList){
       customAlbum.insert(tmpPair.first,tmpPair.second);
    }
    m_customAlbum = customAlbum;
    return customAlbum.values();
}

QString AlbumControl::getCustomAlbumByUid(const int &index)
{
    return DBManager::instance()->getAlbumNameFromUID(index);
}


DBImgInfoList AlbumControl::getTrashInfos()
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
