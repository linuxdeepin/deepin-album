// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dbmanager.h"
#include "application.h"
#include "controller/signalmanager.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "albumgloabl.h"

#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>

#include "imageengineapi.h"

DBManager *DBManager::m_dbManager = nullptr;
std::once_flag DBManager::instanceFlag;
QReadWriteLock DBManager::m_fileMutex;

DBManager *DBManager::instance()
{
    //线程安全单例
    std::call_once(instanceFlag, []() {
        m_dbManager = new DBManager;
    });
    return m_dbManager;
}

DBManager::DBManager(QObject *parent)
    : QObject(parent)
{
    //指定路径
    DATABASE_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator();
    qDebug() << "----current DataBase path--" << DATABASE_PATH;
    DATABASE_NAME = "deepinalbum.db";
    EMPTY_HASH_STR = utils::base::hashByString(QString(" "));
    checkDatabase();
}

const QStringList DBManager::getAllPaths() const
{
    QMutexLocker mutex(&m_dbMutex);
    QStringList paths;

    m_query->setForwardOnly(true);
    if (!m_query->exec("SELECT FilePath FROM ImageTable3")) {
        return paths;
    } else {
        while (m_query->next()) {
            paths << m_query->value(0).toString();
        }
    }

    return paths;
}

const DBImgInfoList DBManager::getAllInfos(const ItemType &itemType, const int &loadCount)const
{
    QMutexLocker mutex(&m_dbMutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);

    QString sqlFileType = "";
    if (itemType == ItemTypePic)
        sqlFileType = "WHERE FileType=3";
    else if (itemType == ItemTypeVideo)
        sqlFileType = "WHERE FileType=4";

    bool b = false;
    if (loadCount == 0) {
        b = m_query->prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType, PathHash, ClassName FROM ImageTable3 %1 order by Time desc").arg(sqlFileType));
    } else {
        b = m_query->prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType, PathHash, ClassName FROM ImageTable3 order by Time desc limit 80 %1").arg(sqlFileType));
    }
    if (!b || ! m_query->exec()) {
        return infos;
    } else {
        using namespace utils::base;
        while (m_query->next()) {
            DBImgInfo info;
            info.filePath = m_query->value(0).toString();
            info.time = m_query->value(3).toDateTime();
            info.changeTime = m_query->value(4).toDateTime();
            info.importTime = m_query->value(5).toDateTime();
            info.itemType = static_cast<ItemType>(m_query->value(6).toInt());
            info.pathHash = m_query->value(7).toString();
            info.className = m_query->value(8).toString();
            infos << info;
        }
    }
    return infos;
}

const QList<QDateTime> DBManager::getAllTimelines() const
{
    QMutexLocker mutex(&m_dbMutex);
    QList<QDateTime> times;
    m_query->setForwardOnly(true);
    // 时间线界面按日期展示相册内容，所以从数据库筛选出日期即可，不用筛选出具体时间
    if (!m_query->exec("SELECT DISTINCT substr(Time, 0, 11)  FROM ImageTable3 ORDER BY Time DESC")) {
        return times;
    } else {
        while (m_query->next()) {
            times << m_query->value(0).toDateTime();
        }
    }
    return times;
}

const DBImgInfoList DBManager::getInfosByTimeline(const QDateTime &timeline) const
{
    QMutexLocker mutex(&m_dbMutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);

    // 使用模糊搜索获取对应日期的图片或视频
    QString sqlStr = QString("SELECT FilePath, FileType, ClassName FROM ImageTable3 "
                             "WHERE Time LIKE \"%%1%\" ORDER BY Time DESC").arg(timeline.date().toString(Qt::ISODate));
    bool b = m_query->prepare(sqlStr);
    if (!b || !m_query->exec()) {
    } else {
        while (m_query->next()) {
            DBImgInfo info;
            info.filePath = m_query->value(0).toString();
            info.itemType = static_cast<ItemType>(m_query->value(1).toInt());
            info.className = m_query->value(2).toString();
            infos << info;
        }
    }

    return infos;
}

const QList<QDateTime> DBManager::getImportTimelines() const
{
    QMutexLocker mutex(&m_dbMutex);
    QList<QDateTime> importtimes;

    m_query->setForwardOnly(true);
    if (!m_query->exec("SELECT DISTINCT STRFTIME(\"%Y-%m-%d %H:%M\", ImportTime) FROM ImageTable3 ORDER BY ImportTime DESC")) {
    } else {
        while (m_query->next()) {
            importtimes << m_query->value(0).toDateTime();
        }
    }
    return importtimes;
}

const DBImgInfoList DBManager::getInfosByImportTimeline(const QDateTime &timeline) const
{
    QMutexLocker mutex(&m_dbMutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);

    bool b = m_query->prepare(QString("SELECT FilePath, FileType, ClassName FROM ImageTable3 "
                                      "WHERE STRFTIME(\"%Y-%m-%d %H:%M\", ImportTime) = STRFTIME(\"%Y-%m-%d %H:%M\", :Date) ORDER BY Time DESC"));
    m_query->bindValue(":Date", timeline);
    if (!b || !m_query->exec()) {
    } else {
        while (m_query->next()) {
            DBImgInfo info;
            info.filePath = m_query->value(0).toString();
            info.itemType = static_cast<ItemType>(m_query->value(1).toInt());
            info.className = m_query->value(2).toString();
            infos << info;
        }
    }

    return infos;
}

const DBImgInfo DBManager::getInfoByPath(const QString &path) const
{
    DBImgInfoList list = getImgInfos("FilePath", path, true);
    if (list.count() != 1) {
        return DBImgInfo();
    } else {
        return list.first();
    }
}

int DBManager::getImgsCount() const
{
    QMutexLocker mutex(&m_dbMutex);

    m_query->setForwardOnly(true);
    if (m_query->exec("SELECT COUNT(*) FROM ImageTable3")) {
        m_query->first();
        int count = m_query->value(0).toInt();
        return count;
    }
    return 0;
}

int DBManager::getImagesCount() const
{
    QMutexLocker mutex(&m_dbMutex);

    m_query->setForwardOnly(true);
    if (m_query->exec("SELECT COUNT(*) FROM ImageTable3 WHERE FileType=3")) {
        m_query->first();
        int count = m_query->value(0).toInt();
        return count;
    }
    return 0;
}

void DBManager::insertImgInfos(const DBImgInfoList &infos)
{
    QMutexLocker mutex(&m_dbMutex);
    m_query->setForwardOnly(true);
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << query.lastError();
    }
    QString qs("REPLACE INTO ImageTable3 (PathHash, FilePath, FileName, Time, "
               "ChangeTime, ImportTime, FileType, ClassName) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

    if (!m_query->prepare(qs)) {
    }

    for (const auto &info : infos) {
        m_query->addBindValue(utils::base::hashByString(info.filePath));
        m_query->addBindValue(info.filePath);
        m_query->addBindValue(info.getFileNameFromFilePath());
        m_query->addBindValue(info.time);
        m_query->addBindValue(info.changeTime);
        m_query->addBindValue(info.importTime);
        m_query->addBindValue(info.itemType);
        m_query->addBindValue(info.className);
        if (!m_query->exec()) {
            ;
        }
    }

    if (!m_query->exec("COMMIT")) {
//            qDebug() << query.lastError();
    }
    mutex.unlock();
    //暂时屏蔽以节省导入时内存，如果有问题再放开
    /*for (DBImgInfo info : infos) {
        ImageEngineApi::instance()->addImageData(info.filePath, info);
    }*/
    emit dApp->signalM->imagesInserted();
}

void DBManager::removeImgInfos(const QStringList &paths)
{
    qDebug() << "------" << __FUNCTION__ << "---size = " << paths.size();
    if (paths.isEmpty()) {
        return;
    }
    // Collect info before removing data
    QStringList pathHashs;
    std::transform(paths.begin(), paths.end(), std::back_inserter(pathHashs), [](const QString & path) {
        return utils::base::hashByString(path);
    });

    QMutexLocker mutex(&m_dbMutex);

    // Remove from albums table
    m_query->setForwardOnly(true);
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << query.lastError();
    }
    QString qs("DELETE FROM AlbumTable3 WHERE PathHash=:hash");
    if (!m_query->prepare(qs)) {
    }
    for (auto &eachHash : pathHashs) {
        m_query->bindValue(":hash", eachHash);
        if (!m_query->exec()) {
            ;
        }
    }
    if (!m_query->exec("COMMIT")) {
//        qDebug() << query.lastError();
    }

    // Remove from image table
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << query.lastError();
    }
    qs = "DELETE FROM ImageTable3 WHERE PathHash=:hash";
    if (!m_query->prepare(qs)) {
    }
    for (auto &eachHash : pathHashs) {
        m_query->bindValue(":hash", eachHash);
        if (!m_query->exec()) {
            ;
        }
    }
    if (!m_query->exec("COMMIT")) {
//            qDebug() << query.lastError();
    }
    mutex.unlock();
    emit dApp->signalM->imagesRemoved();
    qDebug() << "------" << __FUNCTION__ << "size = " << paths.size();
    emit dApp->signalM->imagesRemovedPar(paths);
}

void DBManager::removeImgInfosNoSignal(const QStringList &paths)
{
    QMutexLocker mutex(&m_dbMutex);
    if (paths.isEmpty()) {
        return;
    }

    // Collect info before removing data
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hashByString(path);
    }

    // Remove from albums table
    m_query->setForwardOnly(true);
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << "begin transaction failed.";
    }
    QString qs("DELETE FROM AlbumTable3 WHERE PathHash=\"%1\"");
    for (auto &eachHash : pathHashs) {
        if (!m_query->exec(qs.arg(eachHash))) {
            ;
        }
    }

    if (!m_query->exec("COMMIT")) {
        //        qDebug() << "COMMIT failed.";
    }

    // Remove from image table
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
    }
    qs = "DELETE FROM ImageTable3 WHERE PathHash=\"%1\"";
    for (auto &eachHash : pathHashs) {
        if (!m_query->exec(qs.arg(eachHash))) {
        }
    }

    if (!m_query->exec("COMMIT")) {
    }
}

const QList<std::pair<int, QString>> DBManager::getAllAlbumNames(AlbumDBType atype) const
{
    QMutexLocker mutex(&m_dbMutex);
    QList<std::pair<int, QString>> list;
    m_query->setForwardOnly(true);
    //以UID和相册名称同时作为筛选条件，名称作为UI显示用，UID作为UI和数据库通信的钥匙
    if (m_query->exec(QString("SELECT DISTINCT UID, AlbumName FROM AlbumTable3 WHERE AlbumDBType=%1 ORDER BY UID").arg(atype))) {
        while (m_query->next()) {
            list.push_back(std::make_pair(m_query->value(0).toInt(), m_query->value(1).toString()));
        }
    }

    return list;
}

bool DBManager::isDefaultAutoImportDB(int UID)
{
    if (UID > u_Favorite && UID < u_CustomStart) {
        return true;
    } else {
        return false;
    }
}

std::tuple<QStringList, QStringList, QList<int>> DBManager::getDefaultNotifyPaths()
{
    //图片路径
    QStringList monitorPaths;
    QStringList monitorAlbumNames;
    QList<int>  monitorAlbumUIDs;
    auto stdPicPaths = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    if (!stdPicPaths.isEmpty()) {
        auto stdPicPath = stdPicPaths[0];

        monitorPaths.push_back(stdPicPath + "/Screenshots");
        monitorPaths.push_back(stdPicPath + "/Camera");
        monitorPaths.push_back(stdPicPath + "/Draw");

        monitorAlbumNames.push_back(tr("Screen Capture"));
        monitorAlbumNames.push_back(tr("Camera"));
        monitorAlbumNames.push_back(tr("Draw"));

        monitorAlbumUIDs.push_back(DBManager::SpUID::u_ScreenCapture);
        monitorAlbumUIDs.push_back(DBManager::SpUID::u_Camera);
        monitorAlbumUIDs.push_back(DBManager::SpUID::u_Draw);
    }

    //视频路径
    auto stdMoviePaths = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    if (!stdMoviePaths.isEmpty()) {
        auto stdMoviePath = stdMoviePaths[0];

        monitorPaths.push_back(stdMoviePath + "/Screen Recordings");
        monitorPaths.push_back(stdMoviePath + "/Camera");

        monitorAlbumNames.push_back(tr("Screen Capture"));
        monitorAlbumNames.push_back(tr("Camera"));

        monitorAlbumUIDs.push_back(DBManager::SpUID::u_ScreenCapture);
        monitorAlbumUIDs.push_back(DBManager::SpUID::u_Camera);
    }

    //返回tuple数据
    return std::make_tuple(monitorPaths, monitorAlbumNames, monitorAlbumUIDs);
}

std::tuple<QList<QStringList>, QStringList, QList<int>> DBManager::getDefaultNotifyPaths_group()
{
    QList<QStringList> monitorPaths;
    QStringList monitorAlbumNames;
    QList<int>  monitorAlbumUIDs;

    //获取基础路径
    auto stdPicPaths = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    auto stdPicPath = stdPicPaths[0];
    auto stdMoviePaths = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    auto stdMoviePath = stdMoviePaths[0];

    //截图录屏
    monitorPaths.push_back({stdPicPath + "/Screenshots", stdMoviePath + "/Screen Recordings"});
    monitorAlbumNames.push_back(tr("Screen Capture"));
    monitorAlbumUIDs.push_back(DBManager::SpUID::u_ScreenCapture);

    //相机
    monitorPaths.push_back({stdPicPath + "/Camera", stdMoviePath + "/Camera"});
    monitorAlbumNames.push_back(tr("Camera"));
    monitorAlbumUIDs.push_back(DBManager::SpUID::u_Camera);

    //画板
    monitorPaths.push_back({stdPicPath + "/Draw"});
    monitorAlbumNames.push_back(tr("Draw"));
    monitorAlbumUIDs.push_back(DBManager::SpUID::u_Draw);

    //返回tuple数据
    return std::make_tuple(monitorPaths, monitorAlbumNames, monitorAlbumUIDs);
}

bool DBManager::defaultNotifyPathExists(int UID)
{
    if (!isDefaultAutoImportDB(UID)) { //如果连默认导入UID都不是，直接返回
        return false;
    }

    auto pathsListTuple = getDefaultNotifyPaths();
    auto uidList = std::get<2>(pathsListTuple);
    auto pathsList = std::get<0>(pathsListTuple);

    bool isExists = false;
    for (int i = 0; i != uidList.size(); ++i) {
        if (uidList[i] == UID) {
            QFileInfo currentPath(pathsList[i]);
            if (currentPath.exists() && currentPath.isDir()) {
                isExists = true;
                break;
            }
        }
    }
    return isExists;
}

const QStringList DBManager::getPathsByAlbum(int UID) const
{
    QMutexLocker mutex(&m_dbMutex);
    QStringList list;
    m_query->setForwardOnly(true);
    bool b = m_query->prepare("SELECT DISTINCT i.FilePath "
                              "FROM ImageTable3 AS i, AlbumTable3 AS a "
                              "WHERE i.PathHash=a.PathHash "
                              "AND a.UID=:UID ");
    m_query->bindValue(":UID", UID);
    if (!b || ! m_query->exec()) {
    } else {
        while (m_query->next()) {
            list << m_query->value(0).toString();
        }
    }

    return list;
}

const DBImgInfoList DBManager::getInfosByAlbum(int UID, bool needTimeData) const
{
    QMutexLocker mutex(&m_dbMutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);

    if (needTimeData) {
        bool b = m_query->prepare(QString("SELECT DISTINCT i.FilePath, i.FileType, i.Time, i.ChangeTime, i.ImportTime, i.ClassName "
                                          "FROM ImageTable3 AS i, AlbumTable3 AS a "
                                          "WHERE i.PathHash=a.PathHash "
                                          "AND a.UID=:UID ORDER BY Time DESC").arg(UID));
        if (!b || ! m_query->exec()) {
        } else {
            while (m_query->next()) {
                DBImgInfo info;
                info.filePath = m_query->value(0).toString();
                info.itemType = static_cast<ItemType>(m_query->value(1).toInt());
                info.time = m_query->value(2).toDateTime();
                info.changeTime = m_query->value(3).toDateTime();
                info.importTime = m_query->value(4).toDateTime();
                info.className = m_query->value(5).toString();
                infos << info;
            }
        }
    } else {
        bool b = m_query->prepare(QString("SELECT DISTINCT i.FilePath, i.FileType, i.ClassName "
                                          "FROM ImageTable3 AS i, AlbumTable3 AS a "
                                          "WHERE i.PathHash=a.PathHash "
                                          "AND a.UID=%1 ORDER BY Time DESC").arg(UID));
        if (!b || ! m_query->exec()) {
        } else {
            while (m_query->next()) {
                DBImgInfo info;
                info.filePath = m_query->value(0).toString();
                info.itemType = static_cast<ItemType>(m_query->value(1).toInt());
                info.className = m_query->value(2).toString();
                infos << info;
            }
        }
    }

    return infos;
}

int DBManager::getItemsCountByAlbum(int UID, const ItemType &type) const
{
    int count = 0;
    QMutexLocker mutex(&m_dbMutex);
    m_query->setForwardOnly(true);
    bool b = m_query->prepare("SELECT i.FileType "
                              "FROM ImageTable3 AS i, AlbumTable3 AS a "
                              "WHERE i.PathHash=a.PathHash "
                              "AND a.UID=:UID ");
    m_query->bindValue(":UID", UID);
    if (!b || ! m_query->exec()) {
        //    qWarning() << "Get ImgInfo by album failed: " << query.lastError();
    } else {
        while (m_query->next()) {
            ItemType itemType = static_cast<ItemType>(m_query->value(0).toInt());
            if (type == ItemTypeNull || itemType == type) {
                count++;
            }
        }
    }
    qDebug() << __FUNCTION__ << "---count = " << count;
    return count;
}

//判断是否所有要查询的数据都在要查询的相册中
bool DBManager::isAllImgExistInAlbum(int UID, const QStringList &paths, AlbumDBType atype) const
{
    QMutexLocker mutex(&m_dbMutex);
    m_query->setForwardOnly(true);
    QString sql("SELECT COUNT(*) FROM AlbumTable3 WHERE PathHash In ( %1 ) AND UID = :UID AND AlbumDBType =:atype ");

    QString hashList;
    for (int i = 0; i < paths.size(); i++) {
        QString path = paths.at(i);

        if (hashList.length() > 0) {
            hashList += ", ";
        }
        hashList += "'";
        hashList += utils::base::hashByString(path);
        hashList += "'";
    }

    bool b = m_query->prepare(sql.arg(hashList));

    if (!b) {
        return false;
    }
    m_query->bindValue(":UID", UID);
    m_query->bindValue(":atype", atype);
    if (m_query->exec()) {
        m_query->first();
        if (m_query->value(0).toInt() == paths.size()) {
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

bool DBManager::isImgExistInAlbum(int UID, const QString &path) const
{
    QMutexLocker mutex(&m_dbMutex);
    m_query->setForwardOnly(true);
    bool b = m_query->prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE PathHash = :hash "
                              "AND UID = :UID ");
    if (!b) {
        return false;
    }
    m_query->bindValue(":hash", utils::base::hashByString(path));
    m_query->bindValue(":UID", UID);
    if (m_query->exec()) {
        m_query->first();
        return (m_query->value(0).toInt() == 1);
    } else {
        return false;
    }
}

QString DBManager::getAlbumNameFromUID(int UID) const
{
    QMutexLocker mutex(&m_dbMutex);
    m_query->setForwardOnly(true);
    bool b = m_query->exec(QString("SELECT DISTINCT AlbumName FROM AlbumTable3 WHERE UID=%1").arg(UID));
    if (!b || !m_query->next()) {
        return QString();
    }

    return m_query->value(0).toString();
}

AlbumDBType DBManager::getAlbumDBTypeFromUID(int UID) const
{
    QMutexLocker mutex(&m_dbMutex);
    m_query->setForwardOnly(true);
    bool b = m_query->exec(QString("SELECT DISTINCT AlbumDBType FROM AlbumTable3 WHERE UID=%1").arg(UID));
    if (!b || !m_query->next()) {
        return TypeCount;
    }

    return static_cast<AlbumDBType>(m_query->value(0).toInt());
}

bool DBManager::isAlbumExistInDB(int UID, AlbumDBType atype) const
{
    QMutexLocker mutex(&m_dbMutex);
    m_query->setForwardOnly(true);
    bool b = m_query->prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE UID = :UID AND AlbumDBType =:atype");
    if (!b) {
        return false;
    }
    m_query->bindValue(":UID", UID);
    m_query->bindValue(":atype", atype);
    if (m_query->exec()) {
        m_query->first();
        return (m_query->value(0).toInt() >= 1);
    } else {
        return false;
    }
}

int DBManager::createAlbum(const QString &album, const QStringList &paths, AlbumDBType atype)
{
    QMutexLocker mutex(&m_dbMutex);
    int currentUID = albumMaxUID++;
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hashByString(path);
    }
    m_query->setForwardOnly(true);
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
    }
    auto qs = QString("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash, AlbumDBType, UID) VALUES (null, ?, ?, %1, %2)").arg(atype).arg(currentUID);
    bool b = m_query->prepare(qs);
    if (!b) {
        qDebug() << m_query->lastError().text();
        return -1;
    }

    for (auto &eachHash : pathHashs) {
        m_query->addBindValue(album);
        m_query->addBindValue(eachHash);
        if (!m_query->exec()) {
            qDebug() << m_query->lastError().text();
        }
    }

    if (!m_query->exec("COMMIT")) {
        qDebug() << m_query->lastError().text();
    }

    //FIXME: Don't insert the repeated filepath into the same album
    //Delete the same data
    QString ps = "DELETE FROM AlbumTable3 where AlbumId NOT IN"
                 "(SELECT min(AlbumId) FROM AlbumTable3 GROUP BY"
                 " UID, PathHash, AlbumDBType) AND PathHash != \"%1\""
                 " AND AlbumDBType =%2 ";
    if (!m_query->exec(ps.arg(EMPTY_HASH_STR).arg(atype))) {
        //   qDebug() << "delete same date failed!";
    }

    //把当前UID传出去
    return currentUID;
}

bool DBManager::insertIntoAlbum(int UID, const QStringList &paths, AlbumDBType atype)
{
    QMutexLocker mutex(&m_dbMutex);
    m_query->setForwardOnly(true);

    if (!m_query->exec(QString("SELECT DISTINCT AlbumName FROM AlbumTable3 WHERE UID=%1").arg(UID)) || !m_query->next()) {
        qWarning() << m_query->lastError().text();
        return false; //没找到这个UID，需要先执行创建
    }

    auto album = m_query->value(0).toString();

    m_query->setForwardOnly(true);
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
    }

    QString qs = QString("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, AlbumDBType, UID, PathHash)"
                         " VALUES (null, ?, %1, %2, ?)").arg(atype).arg(UID);
    if (!m_query->prepare(qs)) {
    }
    for (auto &eachPath : paths) {
        if (QFile::exists(eachPath)) { //需要路径存在才能执行添加到相册
            m_query->addBindValue(album);
            m_query->addBindValue(utils::base::hashByString(eachPath));
            if (!m_query->exec()) {
            }
        }
    }

    if (!m_query->exec("COMMIT")) {
    }

    //FIXME: Don't insert the repeated filepath into the same album
    //Delete the same data
    QString ps = "DELETE FROM AlbumTable3 where AlbumId NOT IN"
                 "(SELECT min(AlbumId) FROM AlbumTable3 GROUP BY"
                 " UID, PathHash, AlbumDBType) AND PathHash != \"%1\""
                 " AND AlbumDBType = %2 ";
    if (!m_query->exec(ps.arg(EMPTY_HASH_STR).arg(atype))) {
        //   qDebug() << "delete same date failed!";
    }

    mutex.unlock();

    //发信号通知上层
    emit dApp->signalM->insertedIntoAlbum(UID, paths);

    return true;
}

void DBManager::removeAlbum(int UID)
{
    QMutexLocker mutex(&m_dbMutex);
    if (!m_query->exec(QString("DELETE FROM AlbumTable3 WHERE UID=") + QString::number(UID))) {
    }
}

void DBManager::removeFromAlbum(int UID, const QStringList &paths, AlbumDBType atype)
{
    QMutexLocker mutex(&m_dbMutex);

    QStringList pathHashs;
    std::transform(paths.begin(), paths.end(), std::back_inserter(pathHashs), [](const QString & path) {
        return utils::base::hashByString(path);
    });
    bool success = true;
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
        ;
    }
    QString qs = QString("DELETE FROM AlbumTable3 WHERE UID=%1 AND PathHash=:hash AND AlbumDBType=%2").arg(UID).arg(atype);
    if (!m_query->prepare(qs)) {
    }
    for (auto &eachHashs : pathHashs) {
        m_query->bindValue(":hash", eachHashs);
        if (!m_query->exec()) {
            success = false;
        }
    }
    if (!m_query->exec("COMMIT")) {
        ;
    }
    mutex.unlock();
    if (success) {
        emit dApp->signalM->removedFromAlbum(UID, paths);
    }
}

void DBManager::renameAlbum(int UID, const QString &newAlbum, AlbumDBType atype)
{
    QMutexLocker mutex(&m_dbMutex);

    QString qs = QString("UPDATE AlbumTable3 SET AlbumName=:name WHERE UID=%1 AND AlbumDBType=%2").arg(UID).arg(atype);
    if (!m_query->prepare(qs)) {
    }
    m_query->bindValue(":name", newAlbum);

    if (!m_query->exec()) {
    }
}

void DBManager::updateClassName2DB(const DBImgInfoList &infos)
{
    QMutexLocker mutex(&m_dbMutex);

    m_query->setForwardOnly(true);
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << m_query->lastError();
    }

    QString qs = QString("UPDATE ImageTable3 SET ClassName=:classname WHERE PathHash=:pathhash");
    if (!m_query->prepare(qs)) {
//        qDebug() << m_query->lastError();
    }

    for (auto info : infos) {
        m_query->bindValue(":classname", info.className);
        m_query->bindValue(":pathhash", info.pathHash);
        if (!m_query->exec()) {
            //qDebug() << "update classname failed";
        }
    }

    if (!m_query->exec("COMMIT")) {
        //qDebug() << m_query->lastError();
    }
    mutex.unlock();
}

const DBImgInfoList DBManager::getInfosByNameTimeline(const QString &value) const
{
    QMutexLocker mutex(&m_dbMutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);

    QString queryStr = "SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType, ClassName FROM ImageTable3 "
                       "WHERE FileName like '%" + value + "%' OR Time like '%" + value + "%' ORDER BY Time DESC";

    bool b = m_query->prepare(queryStr);

    if (!b || !m_query->exec()) {
    } else {
        using namespace utils::base;
        while (m_query->next()) {
            DBImgInfo info;
            info.filePath = m_query->value(0).toString();
            info.time = m_query->value(3).toDateTime();
            info.changeTime = m_query->value(4).toDateTime();
            info.importTime = m_query->value(5).toDateTime();
            info.itemType = static_cast<ItemType>(m_query->value(6).toInt());
            info.className = m_query->value(7).toString();
            infos << info;
        }
    }
    return infos;
}

const DBImgInfoList DBManager::getInfosForKeyword(const QString &keywords) const
{
    const DBImgInfoList list = getInfosByNameTimeline(keywords);
    if (list.count() < 1) {
        return DBImgInfoList();
    } else {
        return list;
    }
}

const DBImgInfoList DBManager::getTrashInfosForKeyword(const QString &keywords) const
{
    QMutexLocker mutex(&m_dbMutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);

    //切换到UID后，纯关键字搜索应该不受影响
    QString queryStr = "SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType, ClassName AlbumName FROM TrashTable3 "
                       "WHERE FileName like '%" + keywords + "%' OR Time like '%" + keywords + "%' ORDER BY Time DESC";

    bool b = m_query->prepare(queryStr);

    if (!b || !m_query->exec()) {
    } else {
        using namespace utils::base;
        while (m_query->next()) {
            DBImgInfo info;
            info.filePath = m_query->value(0).toString();
            info.time = m_query->value(3).toDateTime();
            info.changeTime = m_query->value(4).toDateTime();
            info.importTime = m_query->value(5).toDateTime();
            info.itemType = ItemType(m_query->value(6).toInt());
            info.className = m_query->value(7).toString();
            infos << info;
        }
    }
    return infos;
}

const DBImgInfoList DBManager::getInfosForKeyword(int UID, const QString &keywords) const
{
    QMutexLocker mutex(&m_dbMutex);

    DBImgInfoList infos;

    QString queryStr = "SELECT DISTINCT i.FilePath, i.FileName, i.Dir, i.Time, i.ChangeTime, i.ImportTime, i.FileType, i.ClassName "
                       "FROM ImageTable3 AS i "
                       "inner join AlbumTable3 AS a on i.PathHash=a.PathHash AND a.UID=:UID "
                       "WHERE i.FileName like '%" + keywords + "%' ORDER BY Time DESC"; //OR Time like '%" + keywords + "%' 移除按时间搜索

    m_query->setForwardOnly(true);
    bool b = m_query->prepare(queryStr);
    m_query->bindValue(":UID", UID);

    if (!b || ! m_query->exec()) {
    } else {
        using namespace utils::base;
        while (m_query->next()) {
            DBImgInfo info;
            info.filePath = m_query->value(0).toString();
            info.time = m_query->value(3).toDateTime();
            info.changeTime = m_query->value(4).toDateTime();
            info.importTime = m_query->value(5).toDateTime();
            info.itemType = ItemType(m_query->value(6).toInt());
            info.className = m_query->value(7).toString();
            infos << info;
        }
    }
    return infos;
}

const QMultiMap<QString, QString> DBManager::getAllPathAlbumNames() const
{
    QMutexLocker mutex(&m_dbMutex);

    QMultiMap<QString, QString> infos;

    QString queryStr = "SELECT DISTINCT i.FilePath, a.UID "
                       "FROM ImageTable3 AS i, AlbumTable3 AS a "
                       "inner join AlbumTable3 on i.PathHash=a.PathHash "
                       "where a.AlbumDBType = 1";

    m_query->setForwardOnly(true);
    bool b = m_query->prepare(queryStr);
    if (!b || ! m_query->exec()) {
//        qWarning() << "getAllPathAlbumNames failed: " << query.lastError();
    } else {
        using namespace utils::base;
        while (m_query->next()) {
            infos.insert(m_query->value(0).toString(), m_query->value(1).toString());
        }
    }
    return infos;
}

const DBImgInfoList DBManager::getImgInfos(const QString &key, const QString &value, bool needTimeData) const
{
    QMutexLocker mutex(&m_dbMutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);

    if (needTimeData) {
        bool b = m_query->prepare(QString("SELECT FilePath, Time, ChangeTime, ImportTime, FileType, ClassName FROM ImageTable3 "
                                          "WHERE %1= \"%2\" ORDER BY Time DESC").arg(key).arg(value));
        if (!b || !m_query->exec()) {
        } else {
            while (m_query->next()) {
                DBImgInfo info;
                info.filePath = m_query->value(0).toString();
                info.time = m_query->value(1).toDateTime();
                info.changeTime = m_query->value(2).toDateTime();
                info.importTime = m_query->value(3).toDateTime();
                info.itemType = static_cast<ItemType>(m_query->value(4).toInt());
                info.className = m_query->value(5).toString();
                infos << info;
            }
        }
    } else { //取消读取时间数据以加速
        bool b = m_query->prepare(QString("SELECT FilePath, FileType, ClassName FROM ImageTable3 "
                                          "WHERE %1= \"%2\" ORDER BY Time DESC").arg(key).arg(value));
        if (!b || !m_query->exec()) {
        } else {
            while (m_query->next()) {
                DBImgInfo info;
                info.filePath = m_query->value(0).toString();
                info.itemType = static_cast<ItemType>(m_query->value(1).toInt());
                info.className = m_query->value(2).toString();
                infos << info;
            }
        }
    }
    return infos;
}

bool DBManager::checkCustomAutoImportPathIsNotified(const QString &path)
{
    //检查是否是默认路径，这一段不涉及数据库操作，不需要加锁
    auto defaultPath = getDefaultNotifyPaths();
    auto pathsList = std::get<0>(defaultPath);
    for (auto &eachPath : pathsList) {
        if (path.startsWith(eachPath) || eachPath.startsWith(path)) {
            if (path.size() > eachPath.size() && path.at(eachPath.size()) == '/') {
                return true;
            } else if (eachPath.size() > path.size() && eachPath.at(path.size()) == '/') {
                return true;
            } else if (eachPath.size() == path.size()) {
                return true;
            }
        }
    }

    QMutexLocker mutex(&m_dbMutex);

    //这里再去检查已有的数据库
    if (!m_query->exec("SELECT FullPath FROM CustomAutoImportPathTable3")) {
        return true;
    }

    while (m_query->next()) {
        auto eachPath = m_query->value(0).toString();
        if (path.startsWith(eachPath) || eachPath.startsWith(path)) {
            if (path.size() > eachPath.size() && path.at(eachPath.size()) == '/') {
                return true;
            } else if (eachPath.size() > path.size() && eachPath.at(path.size()) == '/') {
                return true;
            } else if (eachPath.size() == path.size()) {
                return true;
            }
        }
    }

    return false;
}

int DBManager::createNewCustomAutoImportPath(const QString &path, const QString &albumName)
{
    QMutexLocker mutex(&m_dbMutex);

    //1.新建相册
    int UID = albumMaxUID++;

    QString qs1 = QString("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash, AlbumDBType, UID) VALUES (null, ?, \"%1\", %2, %3)")
                  .arg("7215ee9c7d9dc229d2921a40e899ec5f").arg(AutoImport).arg(UID);
    if (!m_query->prepare(qs1)) {
    }
    m_query->addBindValue(albumName);

    if (!m_query->exec()) {
        return -1;
    }

    //2.新建保存路径
    QString qs2 = QString("INSERT INTO CustomAutoImportPathTable3 (UID, FullPath, AlbumName) VALUES (%1, ?, ?)").arg(UID);
    if (!m_query->prepare(qs2)) {
    }
    m_query->addBindValue(path);
    m_query->addBindValue(albumName);

    if (!m_query->exec()) {
        return -1;
    }

    return UID;
}

void DBManager::removeCustomAutoImportPath(int UID)
{
    QMutexLocker mutex(&m_dbMutex);
    m_query->setForwardOnly(true);

    //0.查询在该监控路径下的图片
    if (!m_query->exec(QString("SELECT PathHash FROM AlbumTable3 WHERE UID=") + QString::number(UID))) {
    }
    QStringList hashs;
    while (m_query->next()) {
        hashs.push_back(m_query->value(0).toString());
    }

    //移除占位hash
    auto removeIter = std::remove_if(hashs.begin(), hashs.end(), [](const QString & hash) {
        return hash == "7215ee9c7d9dc229d2921a40e899ec5f";
    });
    hashs.erase(removeIter, hashs.end());

    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
    }

    //1.删除图片

    //1.1查询要删除的图片的路径，后面用于通知上层UI进行改变
    QStringList paths;
    if (!m_query->prepare("SELECT FilePath FROM ImageTable3 WHERE PathHash=:hash")) {
    }
    for (auto &eachHash : hashs) {
        m_query->bindValue(":hash", eachHash);
        if (!m_query->exec() || !m_query->next()) {
        }
        paths.push_back(m_query->value(0).toString());
    }

    //1.2执行删除
    if (!m_query->prepare("DELETE FROM ImageTable3 WHERE PathHash=:hash")) {
    }
    for (auto &eachHash : hashs) {
        m_query->bindValue(":hash", eachHash);
        if (!m_query->exec()) {
        }
    }

    //2.删除路径
    if (!m_query->exec(QString("DELETE FROM CustomAutoImportPathTable3 WHERE UID=") + QString::number(UID))) {
    }

    //3.删除相册
    if (!m_query->prepare("DELETE FROM AlbumTable3 WHERE PathHash=:hash")) {
    }
    for (auto &eachHash : hashs) {
        m_query->bindValue(":hash", eachHash);
        if (!m_query->exec()) {
        }
    }

    //补个刀以清除占位hash
    if (!m_query->exec(QString("DELETE FROM AlbumTable3 WHERE UID=") + QString::number(UID))) {
    }

    if (!m_query->exec("COMMIT")) {
    }

    //发送信号通知上层
    mutex.unlock();
    emit dApp->signalM->imagesRemoved();
    emit dApp->signalM->imagesRemovedPar(paths);
}

std::map<int, QString> DBManager::getAllCustomAutoImportUIDAndPath()
{
    std::map<int, QString> result;

    QMutexLocker mutex(&m_dbMutex);
    m_query->setForwardOnly(true);

    if (!m_query->exec("SELECT UID, FullPath FROM CustomAutoImportPathTable3")) {
        return result;
    }

    while (m_query->next()) {
        result.insert(std::make_pair(m_query->value(0).toInt(), m_query->value(1).toString()));
    }

    return result;
}

void DBManager::checkDatabase()
{
    //先建文件夹，再建数据库
    QDir dd(DATABASE_PATH);
    if (! dd.exists()) {
        dd.mkpath(DATABASE_PATH);
    }

    auto db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(DATABASE_PATH + DATABASE_NAME);
    if (!db.open()) {
        qDebug() << "DataBase open fail";
        qDebug() << db.lastError().text();
    }
    m_query = new QSqlQuery(db);

    // 创建Table的语句都是加了IF NOT EXISTS的，直接运行就可以了
    // 注释里面的是实际我们希望的类型，而下面的SQL语句是SQLite3接受的类型

    // ImageTable3
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    //PathHash                    | FilePath | FileName   | Dir  | Time      | ChangeTime | ImportTime //
    //CHARACTER(32) primari key   | TEXT     | TEXT       | TEXT | TIMESTAMP | TIMESTAMP  | TIMESTAMP  //
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    bool b = m_query->exec(QString("CREATE TABLE IF NOT EXISTS ImageTable3 ( "
                                   "PathHash TEXT primary key, "
                                   "FilePath TEXT, "
                                   "FileName TEXT, "
                                   "Dir TEXT, "
                                   "Time TEXT, "
                                   "ChangeTime TEXT, "
                                   "ClassName TEXT, "
                                   "ImportTime TEXT, "
                                   "FileType INTEGER, "
                                   "DataHash TEXT)"));
    if (!b) {
        qDebug() << "b CREATE TABLE exec failed.";
    }

    // AlbumTable3
    ///////////////////////////////////////////////////////////////////////////////////////
    //AlbumId               | AlbumName    | PathHash        |AlbumDBType    |UID        //
    //INTEGER primari key   | TEXT         | CHARACTER(32)   |TEXT           |INTEGER    //
    ///////////////////////////////////////////////////////////////////////////////////////
    bool c = m_query->exec(QString("CREATE TABLE IF NOT EXISTS AlbumTable3 ( "
                                   "AlbumId INTEGER primary key, "
                                   "AlbumName TEXT, "
                                   "PathHash TEXT, "
                                   "AlbumDBType INTEGER,"
                                   "UID INTEGER)"));
    if (!c) {
        qDebug() << "c CREATE TABLE exec failed.";
    }
    // TrashTable3
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    //PathHash                    | FilePath | FileName   | Dir  | Time      | ChangeTime | ImportTime //
    //CHARACTER(32) primari key   | TEXT     | TEXT       | TEXT | TIMESTAMP | TIMESTAMP  | TIMESTAMP  //
    /////////////////////////////////////////////////////////////////////////////////////////////////////
    bool d = m_query->exec(QString("CREATE TABLE IF NOT EXISTS TrashTable3 ( "
                                   "PathHash TEXT primary key, "
                                   "FilePath TEXT, "
                                   "FileName TEXT, "
                                   "Dir TEXT, "
                                   "Time TEXT, "
                                   "ChangeTime TEXT, "
                                   "ImportTime TEXT, "
                                   "ClassName TEXT, "
                                   "FileType INTEGER)"));
    if (!d) {
        qDebug() << "d CREATE TABLE exec failed.";
    }

    // 自定义导入路径表
    // CustomAutoImportPathTable3
    //////////////////////////////////////////////////////////////
    //UID                 | FullPath | AlbumName
    //INTEGER primari key | TEXT     | TEXT
    //////////////////////////////////////////////////////////////
    bool e = m_query->exec(QString("CREATE TABLE IF NOT EXISTS CustomAutoImportPathTable3 ( "
                                   "UID INTEGER primary key, "
                                   "FullPath TEXT, "
                                   "AlbumName TEXT)"));
    if (!e) {
        qDebug() << "d CREATE TABLE exec failed.";
    }

    // 判断ImageTable3中是否有ChangeTime字段
    QString strSqlImage = QString::fromLocal8Bit("select sql from sqlite_master where name = \"ImageTable3\" and sql like \"%ChangeTime%\"");
    bool q = m_query->exec(strSqlImage);
    if (!q && m_query->next()) {
        // 无ChangeTime字段,则增加ChangeTime字段,赋值当前时间
        QString strDate = QDateTime::currentDateTime().toString(DATETIME_FORMAT_DATABASE);
        if (m_query->exec(QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"ChangeTime\" TEXT"))) {
            qDebug() << "add ChangeTime success";
        }
    }

    // 判断ImageTable3中是否有ImportTime字段
    QString strSqlImportTime = "select sql from sqlite_master where name = 'ImageTable3' and sql like '%ImportTime%'";
    if (!m_query->exec(strSqlImportTime)) {
        qDebug() << m_query->lastError();
    }
    if (!m_query->next()) {
        // 无ImportTime字段,则增加ImportTime字段
        QString strDate = QDateTime::currentDateTime().toString(DATETIME_FORMAT_DATABASE);
        if (m_query->exec(QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"ImportTime\" TEXT"))) {
            qDebug() << "add ImportTime success";
        }
    }

    // 判断ImageTable3中是否有FileType字段，区分是图片还是视频
    if (!m_query->exec("select * from sqlite_master where name = 'ImageTable3' and sql like '%FileType%'")) {
        qDebug() << "add FileType failed";
    }
    if (!m_query->next()) {
        // 无FileType字段,则增加FileType字段,赋值1,默认是图片
        if (m_query->exec(QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"FileType\" INTEGER default \"%1\"")
                          .arg(QString::number(ItemTypePic)))) {
            qDebug() << "add FileType success";
        }
    }

    // 判断ImageTable3中是否有DataHash字段，根据文件内容产生的hash
    QString strDataHash = QString::fromLocal8Bit(
                              "select * from sqlite_master where name = 'ImageTable3' and sql like '%DataHash%'");
    if (m_query->exec(strDataHash)) {
        if (!m_query->next()) {
            // DataHash,则增加DataHash字段
            if (m_query->exec(QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"DataHash\" TEXT default \"%1\"")
                              .arg(""))) {
                qDebug() << "add DataHash success";
            }
        }
    }

    // 判断ImageTable3中是否有ClassName字段
    QString strClassName = QString::fromLocal8Bit(
                              "select * from sqlite_master where name = 'ImageTable3' and sql like '%ClassName%'");
    if (m_query->exec(strClassName)) {
        if (!m_query->next()) {
            // ClassName,则增加ClassName字段
            if (m_query->exec(QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"ClassName\" TEXT default \"%1\"")
                              .arg(""))) {
                qDebug() << "add ClassName success";
            }
        }
    }

    // 判断AlbumTable3中是否有AlbumDBType字段
    QString strSqlDBType = QString::fromLocal8Bit("select * from sqlite_master where name = \"AlbumTable3\" and sql like \"%AlbumDBType%\"");
    if (m_query->exec(strSqlDBType) && !m_query->next()) {
        // 无AlbumDBType字段,则增加AlbumDBType字段, 全部赋值为个人相册
        if (m_query->exec(QString("ALTER TABLE \"AlbumTable3\" ADD COLUMN \"AlbumDBType\" INTEGER default %1")
                          .arg("1"))) {
            qDebug() << "add AlbumDBType success";
        }
        if (m_query->exec(QString("update AlbumTable3 SET AlbumDBType = 0 Where AlbumName = \"%1\" ")
                          .arg(COMMON_STR_FAVORITES))) {
        }
    }

    // 判断AlbumTable3中是否有UID字段
    // UID初始化标记
    bool uidIsInited = false;
    QString strSqlUID = QString::fromLocal8Bit("select * from sqlite_master where name = \"AlbumTable3\" and sql like \"%UID%\"");
    bool q3 = m_query->exec(strSqlUID);
    if (q3 && !m_query->next()) {
        // 无UID字段，则需要主动添加
        if (m_query->exec(QString("ALTER TABLE \"AlbumTable3\" ADD COLUMN \"UID\" INTEGER default %1")
                          .arg("0"))) {
            qDebug() << "add UID success";
        }

        // UID字段添加完成后，还需要主动为其进行赋值
        //1.获取当前已存在的album name，但要确保不能影响到收藏相册
        if (m_query->exec(QString("SELECT DISTINCT \"AlbumName\" FROM \"AlbumTable3\" WHERE AlbumDBType <> %1 ").arg(Favourite))) {
            qDebug() << "search album name success";
        }

        //2.在内存中为其进行编号
        QStringList nameList;
        while (m_query->next()) {
            auto currentName = m_query->value(0).toString();
            nameList.push_back(currentName);
        }

        //3.初始化max uid
        albumMaxUID = u_CustomStart;

        //4.写入数据库
        for (auto &currentName : nameList) {
            if (!m_query->exec(QString("UPDATE AlbumTable3 SET UID = %1 WHERE AlbumName = \"%2\"").arg(albumMaxUID++).arg(currentName))) {
                qDebug() << "update AlbumTable3 UID failed";
            }
        }

        uidIsInited = true;
    }

    //插入特殊UID
    insertSpUID("Favorite", Favourite, u_Favorite);
    insertSpUID("Screen Capture", AutoImport, u_ScreenCapture);//使用album name的SQL语句注意加冒号
    insertSpUID("Camera", AutoImport, u_Camera);
    insertSpUID("Draw", AutoImport, u_Draw);

    if (!uidIsInited) {
        //搜索当前最大ID值
        if (!m_query->exec("SELECT max(UID) FROM AlbumTable3") || !m_query->next()) {
            albumMaxUID = u_CustomStart; //没找到
        } else {
            albumMaxUID = m_query->value(0).toInt();
            if (albumMaxUID < u_CustomStart - 1) { //如果当前的UID过小，后面+1后仍然在特殊UID范围内
                albumMaxUID = u_CustomStart - 1;
            }
            albumMaxUID++; //+1以避开已有相册
        }
    }

    // 判断TrashTable的版本
    QString strSqlTrashTable = QString::fromLocal8Bit("select * from sqlite_master where name = \"TrashTable3\"");
    bool build = m_query->exec(strSqlTrashTable);
    if (!build) {
        qDebug() << m_query->lastError();
    }
    if (!m_query->next()) {
        //无新版TrashTable，则创建新表，导入旧表数据
        if (!m_query->exec(QString("CREATE TABLE IF NOT EXISTS TrashTable3 ( "
                                   "PathHash TEXT primary key, "
                                   "FilePath TEXT, "
                                   "FileName TEXT, "
                                   "Dir TEXT, "
                                   "Time TEXT, "
                                   "ChangeTime TEXT, "
                                   "ImportTime TEXT, "
                                   "ClassName TEXT, "
                                   "FileType INTEGER)"))) {
            qDebug() << m_query->lastError();
        }
    } else {
        //判断TrashTable3是否包含FileType
        if (!m_query->exec("select * from sqlite_master where name = \"TrashTable3\" and sql like \"%FileType%\"")) {
            qDebug() << m_query->lastError();
        }
        if (!m_query->next()) {
            // 无FileType字段,则增加FileType字段, 全部赋值为图片
            if (m_query->exec(QString("ALTER TABLE \"TrashTable3\" ADD COLUMN \"FileType\" INTEGER default %1")
                              .arg(QString::number(ItemType::ItemTypePic)))) {
                qDebug() << "add FileType success";
            }
        }

        //判断TrashTable3是否包含ClassName
        if (!m_query->exec("select * from sqlite_master where name = \"TrashTable3\" and sql like \"%ClassName%\"")) {
            qDebug() << m_query->lastError();
        }
        if (!m_query->next()) {
            // 无ClassName字段,则增加ClassName字段
            if (m_query->exec(QString("ALTER TABLE \"TrashTable3\" ADD COLUMN \"ClassName\" INTEGER default %1")
                              .arg(""))) {
                qDebug() << "add ClassName success";
            }
        }
    }

    QString strSqlTrashTableUpdate = QString::fromLocal8Bit("select * from sqlite_master where name = \"TrashTable\"");
    bool update = m_query->exec(strSqlTrashTableUpdate);
    if (!update) {
        qDebug() << m_query->lastError();
    }
    if (m_query->next()) {
        if (!m_query->exec("REPLACE INTO TrashTable3 (PathHash, FilePath, FileName, Dir, Time, ChangeTime)"
                           " SELECT PathHash, FilePath, FileName, Dir, Time, ChangeTime From TrashTable ")) {
            qDebug() << m_query->lastError();
        }
        if (!m_query->exec(QString("DROP TABLE TrashTable"))) {
            qDebug() << m_query->lastError();
        }
    }

    //创建索引以加速
    if (!m_query->exec("CREATE INDEX IF NOT EXISTS album_hash_index ON AlbumTable3 (PathHash)")) {
    }

    if (!m_query->exec("CREATE INDEX IF NOT EXISTS image_hash_index ON ImageTable3 (PathHash)")) {
    }

    if (!m_query->exec("CREATE INDEX IF NOT EXISTS trash_hash_index ON TrashTable3 (PathHash)")) {
    }

    //新版删除需求的数据表策略
    //1.沿用老版的TrashTable3表，不做任何改变
    //2.PathHash作为存放在deepin-album-delete下的文件名，但是为了方便用户维修电脑，把原始文件名带在后面
    //3.数据库里面存的是标准的PathHash，不带文件名
    //4.如果PathHash所代表的文件没有在deepin-album-delete下找到，则表示是老版本的删除策略，恢复的时候以此为依据

    //创建删除缓存路径
    QDir deleteCacheDir(albumGlobal::DELETE_PATH);
    if (!deleteCacheDir.exists()) {
        deleteCacheDir.mkpath(albumGlobal::DELETE_PATH);
    }

    //检查时间数据，以相册版本表为准
    if (!m_query->exec("SELECT * FROM AlbumVersion")) {
        //创建表
        if (!m_query->exec("CREATE TABLE AlbumVersion (version TEXT primary key)")) {
        }
        if (!m_query->exec("INSERT INTO AlbumVersion (version) VALUES (\"5.9\")")) {
        }
        checkTimeColumn("ImageTable3");
        checkTimeColumn("TrashTable3");
    }

    //每次启动后释放一次文件空间，防止占用过多无效空间
    if (!m_query->exec("VACUUM")) {
    }

    //在清理数据库本体的同时，还需要清理一下delete目录
    QFileInfoList deleteInfos;
    utils::image::getAllFileInDir(deleteCacheDir, deleteInfos);
    if (!deleteInfos.isEmpty()) {
        auto fullTrashInfos = getAllTrashInfos(false);
        QStringList trashPaths;
        std::transform(fullTrashInfos.begin(), fullTrashInfos.end(), std::back_inserter(trashPaths), [](const DBImgInfo & info) {
            return utils::base::getDeleteFullPath(info.pathHash, info.getFileNameFromFilePath());
        });

        auto watcher = QtConcurrent::map(deleteInfos, [trashPaths](const QFileInfo & info) {
            auto currentPath = info.absoluteFilePath();
            if (!trashPaths.contains(currentPath)) {
                QFile::remove(currentPath);
            }
        });
        watcher.waitForFinished();
    }
}

void DBManager::checkTimeColumn(const QString &tableName)
{
    //检查并切换所有时间数据
    if (m_query->exec(QString("SELECT Time, ChangeTime, ImportTime, PathHash FROM %1").arg(tableName))) {
        std::vector<std::tuple<QString, QDateTime, QDateTime, QDateTime>> needUpdate;
        while (m_query->next()) {
            auto time = utils::base::analyzeDateTime(m_query->value(0));
            auto changeTime = utils::base::analyzeDateTime(m_query->value(1));
            auto importTime = utils::base::analyzeDateTime(m_query->value(2));
            if (time.second || changeTime.second || importTime.second) {
                needUpdate.push_back(std::make_tuple(m_query->value(3).toString(), time.first, changeTime.first, importTime.first));
            }
        }
        if (!needUpdate.empty()) {
            if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
            }

            for (const auto &eachData : needUpdate) {
                if (!m_query->prepare("UPDATE ImageTable3 SET Time = :t, ChangeTime = :ct, ImportTime = :it WHERE PathHash = :ph")) {
                }
                m_query->bindValue(":t", std::get<1>(eachData));
                m_query->bindValue(":ct", std::get<2>(eachData));
                m_query->bindValue(":it", std::get<3>(eachData));
                m_query->bindValue(":ph", std::get<0>(eachData));
                if (m_query->exec()) {
                }
            }

            if (m_query->exec("COMMIT")) {
            }
        }
    }
}

void DBManager::insertSpUID(const QString &albumName, AlbumDBType astype, SpUID UID)
{
    //0.路径不存在，BUG#111917，只检查默认路径，不要把收藏也搞进来了，否则会导致后续收藏失败
    if (UID != u_Favorite && !defaultNotifyPathExists(UID)) {
        //路径不存在则删除已有的相册
        removeCustomAutoImportPath(UID);
        return;
    }

    //1.检查当前需要新建的sp相册是否存在
    if (!m_query->exec(QString("SELECT UID FROM AlbumTable3 WHERE UID=%1").arg(UID)) || m_query->next()) {
        return;
    }

    //2.不存在则插入数据
    if (!m_query->exec(QString("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash, AlbumDBType, UID) VALUES (null, \"%1\", \"%2\", %3, %4)")
                       .arg(albumName).arg("7215ee9c7d9dc229d2921a40e899ec5f").arg(astype).arg(UID))) {
        qWarning() << "insertSpUID failed" << m_query->lastError().text();
    }

    //2.1.如果是收藏的话，需要对历史收藏的图片进行迁移
    if (UID == u_Favorite) {
        if (!m_query->exec(QString("UPDATE AlbumTable3 SET UID = %1 WHERE AlbumDBType = %2").arg(UID).arg(Favourite))) {
            qDebug() << "update Favorite failed";
        }
    }
}

const DBImgInfoList DBManager::getAllTrashInfos(bool needTimeData) const
{
    QMutexLocker mutex(&m_dbMutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);

    if (needTimeData) {
        bool b = m_query->prepare("SELECT FilePath, Time, ChangeTime, ImportTime, FileType, PathHash, ClassName "
                                  "FROM TrashTable3 ORDER BY ImportTime DESC");
        if (!b || ! m_query->exec()) {
            return infos;
        } else {
            while (m_query->next()) {
                DBImgInfo info;
                info.filePath = m_query->value(0).toString();
                if (info.filePath.isEmpty()) //如果路径为空
                    continue;
                info.time = m_query->value(1).toDateTime();
                info.changeTime = m_query->value(2).toDateTime();
                info.importTime = m_query->value(3).toDateTime();
                info.itemType = ItemType(m_query->value(4).toInt());
                info.pathHash = m_query->value(5).toString();
                info.className = m_query->value(6).toString();
                infos << info;
            }
        }
    } else {
        bool b = m_query->prepare("SELECT FilePath, FileType, PathHash, ClassName "
                                  "FROM TrashTable3 ORDER BY ImportTime DESC");
        if (!b || ! m_query->exec()) {
            return infos;
        } else {
            using namespace utils::base;
            while (m_query->next()) {
                DBImgInfo info;
                info.filePath = m_query->value(0).toString();
                if (info.filePath.isEmpty()) //如果路径为空
                    continue;
                info.itemType = ItemType(m_query->value(1).toInt());
                info.pathHash = m_query->value(2).toString();
                info.className = m_query->value(3).toString();
                infos << info;
            }
        }
    }
    return infos;
}

const DBImgInfoList DBManager::getAllTrashInfos_getRemainDays() const
{
    QMutexLocker mutex(&m_dbMutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);

    //中间那坨东西就是现在距离导入的时候过了多久
    bool b = m_query->prepare("SELECT FilePath, julianday('now') - julianday(STRFTIME(\"%Y-%m-%d\", ImportTime)), FileType, PathHash, ClassName FROM TrashTable3 ORDER BY ImportTime DESC");
    if (!b || ! m_query->exec()) {
        return infos;
    } else {
        using namespace utils::base;
        while (m_query->next()) {
            DBImgInfo info;
            info.filePath = m_query->value(0).toString();
            if (info.filePath.isEmpty()) //如果路径为空
                continue;
            info.remainDays = 30 - static_cast<int>(m_query->value(1).toDouble());
            info.itemType = ItemType(m_query->value(2).toInt());
            info.pathHash = m_query->value(3).toString();
            info.className = m_query->value(4).toString();
            infos << info;
        }
    }
    return infos;
}

void DBManager::insertTrashImgInfos(const DBImgInfoList &infos, bool showWaitDialog)
{
    if (infos.isEmpty()) {
        return;
    }

    //图片删除步骤

    //0.锁定文件操作权限
    m_fileMutex.lockForWrite();

    //1.生成路径hash，复制图片到deepin-album-delete，删除原图至回收站
    QStringList pathHashs;
    for (const auto &info : infos) {
        //计算路径hash
        QString hash;
        if (QFile::exists(info.filePath)) {
            hash = utils::base::hashByString(info.filePath);

            //复制操作，上面那个QFile::copy是异步拷贝，下面那个utils::base::syncCopy是会阻塞的同步拷贝
            //QFile::copy(info.filePath, utils::base::getDeleteFullPath(hash, info.getFileNameFromFilePath()));
            utils::base::syncCopy(info.filePath, utils::base::getDeleteFullPath(hash, info.getFileNameFromFilePath()));

            if (showWaitDialog) {
                emit dApp->signalM->progressOfWaitDialog(infos.size(), infos.indexOf(info) + 1);
            }

            //判断文件路径来自于哪里
            QString path = info.filePath;
            if (path.startsWith("/media/") || // U盘
                    path.contains("smb-share:server=") || //smb地址
                    path.contains("gphoto2:host=Apple") || //apple phone
                    path.contains("ftp:host=") || //ftp路径
                    path.contains("gphoto2:host=") || //ptp路径
                    path.contains("mtp:host=") || //mtp路径
                    path.contains(QDir::homePath() + "/.local/share/Trash") || //垃圾箱
                    utils::base::isVaultFile(path)) { //保险箱
                hash.clear();
            } else {
                utils::base::trashFile(info.filePath);
            }
        }
        pathHashs.push_back(hash); //不能丢进if，否则下面会炸
    }

    //文件操作完毕，释放锁
    m_fileMutex.unlock();

    QMutexLocker mutex(&m_dbMutex);

    //2.向数据库插入数据
    m_query->setForwardOnly(true);
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
        qDebug() << "begin transaction failed.";
    }

    QString qs("REPLACE INTO TrashTable3 "
               "(PathHash, FilePath, FileName, Time, ChangeTime, ImportTime, FileType, ClassName) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    if (!m_query->prepare(qs)) {
    }
    for (int i = 0; i != infos.size(); ++i) {
        if (pathHashs[i].isEmpty()) {
            continue;
        }
        m_query->addBindValue(pathHashs[i]); //复用上面生成的hash
        m_query->addBindValue(infos[i].filePath);
        m_query->addBindValue(infos[i].getFileNameFromFilePath());
        m_query->addBindValue(infos[i].time);
        m_query->addBindValue(infos[i].changeTime);
        m_query->addBindValue(infos[i].importTime);
        m_query->addBindValue(infos[i].itemType);
        m_query->addBindValue(infos[i].className);
        if (!m_query->exec()) {
        }
    }

    if (!m_query->exec("COMMIT")) {
        qDebug() << "COMMIT failed.";
    }

    mutex.unlock();

    //3.通知UI模块有图片删除
    emit dApp->signalM->imagesTrashInserted();
}

void DBManager::removeTrashImgInfos(const QStringList &paths)
{
    if (paths.isEmpty()) {
        return;
    }

    //计算路径hash
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hashByString(path);
    }

    QMutexLocker mutex(&m_dbMutex);
    m_query->setForwardOnly(true);

    // Remove from image table
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << "begin transaction failed.";
    }
    QString qs("DELETE FROM TrashTable3 WHERE PathHash=:hash");
    if (!m_query->prepare(qs)) {
    }
    for (const auto &path : paths) {
        m_query->bindValue(":hash", utils::base::hashByString(path));
        if (!m_query->exec()) {
        }
    }

    if (!m_query->exec("COMMIT")) {
//            qDebug() << "COMMIT failed.";
    }

    //删除deepin-album-delete下的缓存文件
    for (int i = 0; i != paths.size(); ++i) {
        auto deletePath = utils::base::getDeleteFullPath(pathHashs[i], DBImgInfo::getFileNameFromFilePath(paths[i]));
        QFile::remove(deletePath);
        utils::base::delTrashFile(paths[i]);
    }

    mutex.unlock();
    emit dApp->signalM->imagesTrashRemoved();
}

QStringList DBManager::recoveryImgFromTrash(const QStringList &paths)
{
    if (paths.isEmpty()) {
        return QStringList();
    }

    //1.计算路径hash
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hashByString(path);
    }

    //2.尝试恢复文件

    QMutexLocker mutex(&m_dbMutex);

    //获取内部恢复路径
    auto stdPicPaths = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    QString internalRecoveryPath;
    QDir internalRecoveryDir;
    if (!stdPicPaths.isEmpty()) {
        internalRecoveryPath = stdPicPaths[0] + "/" + "Albums";
        internalRecoveryDir.setPath(internalRecoveryPath);
    }

    QStringList successedHashs; //恢复成功的hash
    QStringList failedFiles;    //恢复失败的文件名
    std::vector<std::tuple<QString, QString, QString>> changedPaths;//恢复成功但路径变了，0：原始路径hash，1：当前路径，2：当前路径的hash
    QMap <QString, QString> succesedPaths;//保存成功的hash key,路径为value
    //执行恢复步骤
    for (int i = 0; i != paths.size(); ++i) {
        auto deletePath = utils::base::getDeleteFullPath(pathHashs[i], DBImgInfo::getFileNameFromFilePath(paths[i])); //获取删除缓存路径
        if (!QFile::exists(deletePath)) { //文件不存在，表示要么是老版相册，要么是缓存文件已被破坏，此时需要判定文件恢复成功
            successedHashs.push_back(pathHashs[i]);
            continue;
        }
        QString recoveryName = paths[i];
        if (QFile::exists(recoveryName)) { //文件已存在，加副本标记
            if (recoveryName.startsWith("/media/") || // U盘
                    recoveryName.contains("smb-share:server=") || //smb地址
                    recoveryName.contains("gphoto2:host=Apple") || //apple phone
                    recoveryName.contains("ftp:host=") || //ftp路径
                    recoveryName.contains("gphoto2:host=") || //ptp路径
                    recoveryName.contains("mtp:host=") || //mtp路径
                    recoveryName.contains(QDir::homePath() + "/.local/share/Trash") || //垃圾箱
                    utils::base::isVaultFile(recoveryName)) { //保险箱

            } else {
                QFileInfo info(recoveryName);
                QString name = info.completeBaseName();
                name.append(tr("(copy)"));
                recoveryName = info.dir().path() + "/" + name + "." + info.completeSuffix();
            }
            if (recoveryName.size() > 255) {
                failedFiles.push_back(paths[i]); //文件名过长，恢复失败
                continue;
            }
        }

        if (QFile::rename(deletePath, recoveryName)) { //尝试正常恢复
            successedHashs.push_back(pathHashs[i]); //正常恢复成功
            succesedPaths.insert(pathHashs[i], recoveryName);
            utils::base::delTrashFile(paths[i]); //同步删除回收站数据
        } else { //正常恢复失败，尝试恢复至内部路径
            if (!internalRecoveryDir.exists()) { //检查文件夹是否存在，不存在则创建
                internalRecoveryDir.mkpath(internalRecoveryPath);
            }
            recoveryName = internalRecoveryPath + "/" + DBImgInfo::getFileNameFromFilePath(paths[i]);
            if (QFile::exists(recoveryName)) { //文件已存在，加副本标记
                QFileInfo info(recoveryName);
                QString name = info.completeBaseName();
                recoveryName = info.dir().path() + "/" + name + tr("(copy)") + "." + info.completeSuffix();
                int number = 1;
                //防止
                while (QFile::exists(recoveryName)) {
                    recoveryName = info.dir().path() + "/" + name + tr("(copy)") + QString::number(number++) + "." + info.completeSuffix();
                }
                QString strName = name + tr("(copy)") + QString::number(number++);
                if (strName.size() > 255) {
//                    failedFiles.push_back(paths[i]); //文件名过长，恢复失败
//                    continue;
                    //文件过长,修改名称
                    while (QFile::exists(recoveryName)) {
                        recoveryName = info.dir().path() + "/" +  tr("(copy)") + QString::number(number++) + "." + info.completeSuffix();
                    }
                }
            }
            //尝试恢复至内部路径
            if (QFile::rename(deletePath, recoveryName)) {
                successedHashs.push_back(pathHashs[i]); //恢复至内部路径
                succesedPaths.insert(pathHashs[i], recoveryName);
                utils::base::delTrashFile(paths[i]); //同步删除回收站数据
            } else { //TODO：极端特殊情况：使用内部恢复路径恢复失败
                continue;
            }
        }

        //文件名改变，需要刷新另外两个表的文件名和hash数据
        if (recoveryName != paths[i]) {
            changedPaths.push_back(std::make_tuple(pathHashs[i], recoveryName, utils::base::hashByString(recoveryName)));
        }
    }

    //3.数据库操作
    if (!successedHashs.isEmpty()) { //如果没有成功恢复的文件，则以下操作全部不会执行
        m_query->setForwardOnly(true);

        //3.1获取恢复成功的文件数据
        DBImgInfoList infos;
        QString qs("SELECT FilePath, Time, ChangeTime, ImportTime, FileType, ClassName FROM TrashTable3 WHERE PathHash=:value");
        if (!m_query->prepare(qs)) {
        }

        for (const auto &hash : successedHashs) {
            m_query->bindValue(":value", hash);
            if (m_query->exec() && m_query->next()) {
                //数据读取
                DBImgInfo info;
                info.filePath = m_query->value(0).toString();

                //此处需要额外判断路径是否存在，如果不存在则表示是缓存文件已破坏，只能无视
                if (!QFile::exists(info.filePath)) {
                    if (QFile::exists(succesedPaths.value(hash))) {
                        info.filePath = succesedPaths.value(hash);
                    } else {
                        continue;
                    }

                }

                info.time = m_query->value(1).toDateTime();
                info.changeTime = m_query->value(2).toDateTime();
                info.importTime = QDateTime::currentDateTime();
                info.itemType = ItemType(m_query->value(4).toInt());
                info.className = m_query->value(5).toString();

                //如果文件名改变则刷新数据
                auto iter = std::find_if(changedPaths.begin(), changedPaths.end(), [hash](const auto & item) {
                    return hash == std::get<0>(item);
                });

                if (iter != changedPaths.end()) { //改变了就写入新的hash和路径
                    info.filePath = std::get<1>(*iter);
                    info.pathHash = std::get<2>(*iter);
                } else { //没变就写入原来的hash
                    info.pathHash = hash;
                }

                infos.push_back(info);
            }
        }

        //3.2把恢复成功的文件数据清理掉
        if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
        }
        qs = "DELETE FROM TrashTable3 WHERE PathHash=:hash";
        if (!m_query->prepare(qs)) {
        }
        for (const auto &hash : successedHashs) {
            m_query->bindValue(":hash", hash);
            if (!m_query->exec()) {
            }
        }
        if (!m_query->exec("COMMIT")) {
        }

        //3.3把恢复成功的文件数据刷回ImageTable3，这里需要重复利用已经计算好的hash，所以不调用已有的API
        if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
        }
        qs = "REPLACE INTO ImageTable3 (PathHash, FilePath, FileName, Time, "
             "ChangeTime, ImportTime, FileType, ClassName) VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
        if (!m_query->prepare(qs)) {
        }
        for (const auto &info : infos) {
            m_query->addBindValue(info.pathHash);
            m_query->addBindValue(info.filePath);
            m_query->addBindValue(info.getFileNameFromFilePath());
            m_query->addBindValue(info.time);
            m_query->addBindValue(info.changeTime);
            m_query->addBindValue(info.importTime);
            m_query->addBindValue(info.itemType);
            m_query->addBindValue(info.className);
            if (!m_query->exec()) {
            }
        }
        if (!m_query->exec("COMMIT")) {
        }

        mutex.unlock();

        //4.发送信号通知外层控件有最近删除的文件被恢复
        emit dApp->signalM->imagesTrashRemoved();
        emit dApp->signalM->imagesInserted();
    }

    //5.返回失败的文件
    return failedFiles;
}

void DBManager::removeTrashImgInfosNoSignal(const QStringList &paths)
{
    if (paths.isEmpty()) {
        return;
    }

    QMutexLocker mutex(&m_dbMutex);

    //计算路径hash
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hashByString(path);
    }

    //从AlbumTable3删除
    m_query->setForwardOnly(true);
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << "begin transaction failed.";
    }
    QString qs("DELETE FROM AlbumTable3 WHERE PathHash=:hash");
    if (!m_query->prepare(qs)) {
    }
    for (const auto &hash : pathHashs) {
        m_query->bindValue(":hash", hash);
        if (!m_query->exec()) {
        }
    }
    if (!m_query->exec("COMMIT")) {
//        qDebug() << "COMMIT failed.";
    }

    //从TrashTable3删除
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << "begin transaction failed.";
    }
    qs = "DELETE FROM TrashTable3 WHERE PathHash=:hash";
    if (!m_query->prepare(qs)) {
    }
    for (const auto &hash : pathHashs) {
        m_query->bindValue(":hash", hash);
        if (!m_query->exec()) {
        }
    }
    if (!m_query->exec("COMMIT")) {
//        qDebug() << "COMMIT failed.";
    }

    //删除deepin-album-delete下的缓存文件
    for (int i = 0; i != paths.size(); ++i) {
        auto deletePath = utils::base::getDeleteFullPath(pathHashs[i], DBImgInfo::getFileNameFromFilePath(paths[i]));
        QFile::remove(deletePath);
    }
}

const DBImgInfo DBManager::getTrashInfoByPath(const QString &path) const
{
    DBImgInfoList list = getTrashImgInfos("FilePath", path);
    if (list.count() != 1) {
        return DBImgInfo();
    } else {
        return list.first();
    }
}

const DBImgInfoList DBManager::getTrashImgInfos(const QString &key, const QString &value) const
{
    QMutexLocker mutex(&m_dbMutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);
    bool b = m_query->prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType, ClassName FROM TrashTable3 "
                                      "WHERE %1= :value ORDER BY Time DESC").arg(key));

    m_query->bindValue(":value", value);

    if (!b || !m_query->exec()) {
        //  qWarning() << "Get Image from database failed: " << query.lastError();
    } else {
        while (m_query->next()) {
            DBImgInfo info;
            info.filePath = m_query->value(0).toString();
            if (info.filePath.isEmpty()) //如果路径为空
                continue;
            info.time = m_query->value(3).toDateTime();
            info.changeTime = m_query->value(4).toDateTime();
            info.importTime = m_query->value(5).toDateTime();
            info.itemType = ItemType(m_query->value(6).toInt());
            info.className = m_query->value(7).toString();

            infos << info;
        }
    }
    return infos;
}

int DBManager::getTrashImgsCount() const
{
    QMutexLocker mutex(&m_dbMutex);
    m_query->setForwardOnly(true);
    if (m_query->exec("SELECT COUNT(*) FROM TrashTable3")) {
        m_query->first();
        int count = m_query->value(0).toInt();
        return count;
    }
    return 0;
}

int DBManager::getAlbumImgsCount(int UID) const
{
    QMutexLocker mutex(&m_dbMutex);
    m_query->setForwardOnly(true);
    if (m_query->exec(QString("SELECT COUNT(*) FROM AlbumTable3 WHERE UID=%1 AND PathHash<>\"%2\"")
                      .arg(UID).arg("7215ee9c7d9dc229d2921a40e899ec5f"))) {
        m_query->first();
        int count = m_query->value(0).toInt();
        return count;
    }
    return 0;
}
