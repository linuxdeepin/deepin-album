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
    m_infoList = DBManager::instance()->getAllInfos(100);
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

