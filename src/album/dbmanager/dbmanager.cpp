/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "dbmanager.h"
#include "application.h"
#include "controller/signalmanager.h"
#include "utils/baseutils.h"
#include "albumgloabl.h"

#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

#include "imageengineapi.h"

namespace {
const QString DATABASE_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                              + QDir::separator() + "deepin" + QDir::separator() + "deepin-album" + QDir::separator();
const QString DATABASE_NAME = "deepinalbum.db";
const QString EMPTY_HASH_STR = utils::base::hashByString(QString(" "));

}  // namespace

DBManager *DBManager::m_dbManager = nullptr;

DBManager *DBManager::instance()
{
    if (!m_dbManager) {
        m_dbManager = new DBManager();
    }
    return m_dbManager;
}

DBManager::DBManager(QObject *parent)
    : QObject(parent)
{
    m_db = getDatabase();
    checkDatabase();
}

const QStringList DBManager::getAllPaths() const
{
    QMutexLocker mutex(&m_mutex);
    QStringList paths;
    QSqlDatabase db = getDatabase();
    if (! db.isValid())
        return paths;

    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT "
                           "FilePath "
                           "FROM ImageTable3"/* ORDER BY Time DESC"*/);
    if (!b || ! query.exec()) {
        db.close();
        return paths;
    } else {
        while (query.next()) {
            paths << query.value(0).toString();
        }
    }
    // 连接使用完后需要释放回数据库连接池
    db.close();
    return paths;
}

const DBImgInfoList DBManager::getAllInfos(int loadCount)const
{
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = false;
    if (loadCount == 0) {
        b = query.prepare("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType FROM ImageTable3 order by ChangeTime desc");
    } else {
        b = query.prepare("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType FROM ImageTable3 order by ChangeTime desc limit 80");
    }
    if (!b || ! query.exec()) {
//        qDebug() << query.lastError();
        return infos;
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(query.value(5).toString(), DATETIME_FORMAT_DATABASE);
            info.itemType = static_cast<ItemType>(query.value(6).toInt());
            infos << info;
        }
    }
    return infos;
}

const QStringList DBManager::getAllTimelines() const
{
    QMutexLocker mutex(&m_mutex);
    QStringList times;
    QSqlDatabase db = getDatabase();
    if (! db.isValid())
        return times;

    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT DISTINCT Time FROM ImageTable3 ORDER BY Time DESC");
    if (!b || ! query.exec()) {
        db.close();
        return times;
    } else {
        while (query.next()) {
            times << query.value(0).toString();
        }
    }
    db.close();
    return times;
}

const DBImgInfoList DBManager::getInfosByTimeline(const QString &timeline) const
{
    const DBImgInfoList list = getImgInfos("Time", timeline);
    if (list.count() < 1) {
        return DBImgInfoList();
    } else {
        return list;
    }
}

const QStringList DBManager::getImportTimelines() const
{
    QMutexLocker mutex(&m_mutex);
    QStringList importtimes;
    QSqlDatabase db = getDatabase();
    if (! db.isValid())
        return importtimes;

    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT DISTINCT ImportTime FROM ImageTable3 ORDER BY ImportTime DESC");
    if (!b || !query.exec()) {
        db.close();
        return importtimes;
    } else {
        while (query.next()) {
            importtimes << query.value(0).toString();
        }
    }
    db.close();
    return importtimes;
}

const DBImgInfoList DBManager::getInfosByImportTimeline(const QString &timeline) const
{
    const DBImgInfoList list = getImgInfos("ImportTime", timeline);
    if (list.count() < 1) {
        return DBImgInfoList();
    } else {
        return list;
    }
}

const DBImgInfo DBManager::getInfoByPath(const QString &path) const
{
    DBImgInfoList list = getImgInfos("FilePath", path);
    if (list.count() != 1) {
        return DBImgInfo();
    } else {
        return list.first();
    }
}

int DBManager::getImgsCount() const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return 0;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
        db.close();
        return 0;
    }
    if (!query.prepare("SELECT COUNT(*) FROM ImageTable3")) {
        db.close();
        return 0;
    }
    if (query.exec()) {
        query.first();
        int count = query.value(0).toInt();
        if (!query.exec("COMMIT")) {
        }
        db.close();
        return count;
    }
    db.close();
    return 0;
}

void DBManager::insertImgInfos(const DBImgInfoList &infos)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (infos.isEmpty() || ! db.isValid()) {
        return;
    }
    QVariantList pathhashs, filenames, filepaths, dirs, times, changetimes, importtimes, fileTypes;
    for (DBImgInfo info : infos) {
        filenames << info.fileName;
        filepaths << info.filePath;
        pathhashs << utils::base::hashByString(info.filePath);
        dirs << info.dirHash;
        times << info.time.toString("yyyy.MM.dd");
        changetimes << info.changeTime.toString(DATETIME_FORMAT_DATABASE);
        importtimes << info.importTime.toString(DATETIME_FORMAT_DATABASE);
        fileTypes << info.itemType;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << query.lastError();
    }
    bool b = query.prepare("REPLACE INTO ImageTable3 (PathHash, FilePath, FileName, Dir, Time, "
                           "ChangeTime, ImportTime, FileType) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    if (!b) {
        db.close();
        return;
    }
    query.addBindValue(pathhashs);
    query.addBindValue(filepaths);
    query.addBindValue(filenames);
    query.addBindValue(dirs);
    query.addBindValue(times);
    query.addBindValue(changetimes);
    query.addBindValue(importtimes);
    query.addBindValue(fileTypes);
    if (! query.execBatch()) {
        if (!query.exec("COMMIT")) {
//            qDebug() << query.lastError();
        }
        db.close();
    } else {
        if (!query.exec("COMMIT")) {
//            qDebug() << query.lastError();
        }
        db.close();
        mutex.unlock();
        for (DBImgInfo info : infos) {
            ImageEngineApi::instance()->addImageData(info.filePath, info);
        }
        emit dApp->signalM->imagesInserted(/*infos*/);
    }
}

void DBManager::removeImgInfos(const QStringList &paths)
{
    qDebug() << "------" << __FUNCTION__ << "---size = " << paths.size();
    if (paths.isEmpty()) {
        return;
    }
    // Collect info before removing data
    DBImgInfoList infos;
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hashByString(path);
        infos.append(getImgInfos("FilePath", path, false));
    }
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << query.lastError();
    }
    QString qs = "DELETE FROM AlbumTable3 WHERE PathHash=?";
    bool b = query.prepare(qs);
    if (!b) {
        db.close();
        return;
    }
    query.addBindValue(pathHashs);
    query.execBatch();
    if (!query.exec("COMMIT")) {
//        qDebug() << query.lastError();
    }
    // Remove from image table
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << query.lastError();
    }
    qs = "DELETE FROM ImageTable3 WHERE PathHash=?";
    bool bs = query.prepare(qs);
    if (!bs) {
        db.close();
        return;
    }
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        if (!query.exec("COMMIT")) {
//            qDebug() << query.lastError();
        }
        db.close();
    } else {
        if (!query.exec("COMMIT")) {
//            qDebug() << query.lastError();
        }
        db.close();
        mutex.unlock();
        emit dApp->signalM->imagesRemoved();
        qDebug() << "------" << __FUNCTION__ << "size = " << paths.size();
        emit dApp->signalM->imagesRemovedPar(infos);
    }
}

void DBManager::removeImgInfosNoSignal(const QStringList &paths)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (paths.isEmpty() || ! db.isValid()) {
        return;
    }

    // Collect info before removing data
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hashByString(path);
    }

    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << "begin transaction failed.";
    }
    QString qs = "DELETE FROM AlbumTable3 WHERE PathHash=?";
    bool b = query.prepare(qs);
    if (!b) {
        db.close();
        return;
    }
    query.addBindValue(pathHashs);
    query.execBatch();
    if (!query.exec("COMMIT")) {
//        qDebug() << "COMMIT failed.";
    }

    // Remove from image table
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
    }
    qs = "DELETE FROM ImageTable3 WHERE PathHash=?";
    bool bs = query.prepare(qs);
    if (!bs) {
        db.close();
        return;
    }
    query.addBindValue(pathHashs);
    query.execBatch();
    if (!query.exec("COMMIT")) {
    }
    db.close();
}

const QList<std::pair<int, QString>> DBManager::getAllAlbumNames(AlbumDBType atype) const
{
    QMutexLocker mutex(&m_mutex);
    QList<std::pair<int, QString>> list;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return list;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    //以UID和相册名称同时作为筛选条件，名称作为UI显示用，UID作为UI和数据库通信的钥匙
    bool b = query.prepare("SELECT DISTINCT UID, AlbumName FROM AlbumTable3 WHERE AlbumDBType =:atype");
    query.bindValue(":atype", atype);
    if (!b || !query.exec()) {
    } else {
        while (query.next()) {
            list.push_back(std::make_pair(query.value(0).toInt(), query.value(1).toString()));
        }
    }
    db.close();

    return list;
}

bool DBManager::isDefaultAutoImportDB(int UID) const
{
    if (UID > u_Favorite && UID < u_CustomStart) {
        return true;
    } else {
        return false;
    }
}

QStringList DBManager::getDefaultNotifyPaths() const
{
    QStringList monitorPaths;

    //图片路径
    auto stdPicPaths = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    if (!stdPicPaths.isEmpty()) {
        auto stdPicPath = stdPicPaths[0];

        monitorPaths.push_back(stdPicPath + "/" + "Camera");
        monitorPaths.push_back(stdPicPath + "/" + "Screen Capture");
        monitorPaths.push_back(stdPicPath + "/" + "Draw");
    }

    //视频路径
    auto stdMoviePaths = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    if (!stdMoviePaths.isEmpty()) {
        auto stdMoviePath = stdMoviePaths[0];

        monitorPaths.push_back(stdMoviePath + "/" + "Camera");
        monitorPaths.push_back(stdMoviePath + "/" + "Screen Capture");
    }

    return monitorPaths;
}

const QStringList DBManager::getPathsByAlbum(int UID, AlbumDBType atype) const
{
    QMutexLocker mutex(&m_mutex);
    QStringList list;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return list;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT DISTINCT i.FilePath "
                           "FROM ImageTable3 AS i, AlbumTable3 AS a "
                           "WHERE i.PathHash=a.PathHash "
                           "AND a.UID=:UID "
                           "AND a.AlbumDBType=:atype ");
    query.bindValue(":UID", UID);
    query.bindValue(":atype", atype);
    if (!b || ! query.exec()) {
    } else {
        while (query.next()) {
            list << query.value(0).toString();
        }
    }
    db.close();

    return list;
}

const DBImgInfoList DBManager::getInfosByAlbum(int UID) const
{
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT DISTINCT i.FilePath, i.FileName, i.FileType, i.Time, i.ChangeTime, i.ImportTime "
                           "FROM ImageTable3 AS i, AlbumTable3 AS a "
                           "WHERE i.PathHash=a.PathHash "
                           "AND a.UID=:UID ");
    query.bindValue(":UID", UID);
    if (!b || ! query.exec()) {
        //    qWarning() << "Get ImgInfo by album failed: " << query.lastError();
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.itemType = static_cast<ItemType>(query.value(2).toInt());
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(query.value(5).toString(), DATETIME_FORMAT_DATABASE);
            infos << info;
        }
    }
    db.close();
    return infos;
}

int DBManager::getItemsCountByAlbum(int UID, const ItemType &type, AlbumDBType atype) const
{
    int count = 0;
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return 0;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT i.FileType "
                           "FROM ImageTable3 AS i, AlbumTable3 AS a "
                           "WHERE i.PathHash=a.PathHash "
                           "AND a.UID=:UID "
                           "AND a.AlbumDBType=:atype ");
    query.bindValue(":UID", UID);
    query.bindValue(":atype", atype);
    if (!b || ! query.exec()) {
        //    qWarning() << "Get ImgInfo by album failed: " << query.lastError();
    } else {
        using namespace utils::base;
        while (query.next()) {
            ItemType itemType = static_cast<ItemType>(query.value(0).toInt());
            if (type == ItemTypeNull) {
                count++;
            } else if (itemType == type) {
                count++;
            }
        }
    }
    db.close();
    qDebug() << __FUNCTION__ << "---count = " << count;
    return count;
}
//判断是否所有要查询的数据都在要查询的相册中
bool DBManager::isAllImgExistInAlbum(int UID, const QStringList &paths, AlbumDBType atype) const
{
    //QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return false;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    QString sql;
    sql += "SELECT COUNT(*) FROM AlbumTable3 WHERE PathHash In ( %1 ) AND UID = :UID AND AlbumDBType =:atype ";

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

    bool b = query.prepare(sql.arg(hashList));

    if (!b) {
        db.close();
        return false;
    }
    query.bindValue(":UID", UID);
    query.bindValue(":atype", atype);
    if (query.exec()) {
        query.first();
        db.close();
        if (query.value(0).toInt() == paths.size()) {
            return true;
        } else {
            return false;
        }
    } else {
        db.close();
        return false;
    }
}

bool DBManager::isImgExistInAlbum(int UID, const QString &path, AlbumDBType atype) const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return false;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE PathHash = :hash "
                           "AND UID = :UID "
                           "AND AlbumDBType =:atype ");
    if (!b) {
        db.close();
        return false;
    }
    query.bindValue(":hash", utils::base::hashByString(path));
    query.bindValue(":UID", UID);
    query.bindValue(":atype", atype);
    if (query.exec()) {
        query.first();
        db.close();
        return (query.value(0).toInt() == 1);
    } else {
        db.close();
        return false;
    }
}

QString DBManager::getAlbumNameFromUID(int UID) const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        return QString();
    }

    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.exec(QString("SELECT DISTINCT AlbumName FROM AlbumTable3 WHERE UID=%1").arg(UID));
    if (!b || !query.next()) {
        db.close();
        return QString();
    }

    return query.value(0).toString();
}

AlbumDBType DBManager::getAlbumDBTypeFromUID(int UID) const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        return TypeCount;
    }

    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.exec(QString("SELECT DISTINCT AlbumDBType FROM AlbumTable3 WHERE UID=%1").arg(UID));
    if (!b || !query.next()) {
        db.close();
        return TypeCount;
    }

    return static_cast<AlbumDBType>(query.value(0).toInt());
}

bool DBManager::isAlbumExistInDB(int UID, AlbumDBType atype) const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return false;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE UID = :UID AND AlbumDBType =:atype");
    if (!b) {
        db.close();
        return false;
    }
    query.bindValue(":UID", UID);
    query.bindValue(":atype", atype);
    if (query.exec()) {
        query.first();
        // 连接使用完后需要释放回数据库连接池
        db.close();
        return (query.value(0).toInt() >= 1);
    } else {
        // 连接使用完后需要释放回数据库连接池
        db.close();
        return false;
    }
}

int DBManager::createAlbum(const QString &album, const QStringList &paths, AlbumDBType atype)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();

    if (!db.isValid()) {
        return -1;
    }
    int currentUID = albumMaxUID++;
    QStringList nameRows, pathHashRows;
    QVariantList atypes, uids;
    for (QString path : paths) {
        nameRows << album;
        pathHashRows << utils::base::hashByString(path);
        atypes << atype;
        uids << currentUID;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
    }
    bool b = query.prepare("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash, AlbumDBType, UID) VALUES (null, ?, ?, ?, ?)");
    if (!b) {
        db.close();
        return -1;
    }
    query.addBindValue(nameRows);
    query.addBindValue(pathHashRows);
    query.addBindValue(atypes);
    query.addBindValue(uids);

    if (! query.execBatch()) {
    }
    if (!query.exec("COMMIT")) {
    }

    //FIXME: Don't insert the repeated filepath into the same album
    //Delete the same data
    QString ps = "DELETE FROM AlbumTable3 where AlbumId NOT IN"
                 "(SELECT min(AlbumId) FROM AlbumTable3 GROUP BY"
                 " UID, PathHash, AlbumDBType) AND PathHash != \"%1\""
                 " AND AlbumDBType =:atype ";
    bool bs = query.prepare(ps.arg(EMPTY_HASH_STR));
    if (!bs) {
        db.close();
        mutex.unlock();
        return -1;
    }
    query.bindValue(":atype", atype);
    if (!query.exec()) {
        //   qDebug() << "delete same date failed!";
    }
    if (!query.exec("COMMIT")) {
    }
    db.close();
    mutex.unlock();

    //把当前UID传出去
    return currentUID;
}

bool DBManager::insertIntoAlbum(int UID, const QStringList &paths, AlbumDBType atype)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();

    if (!db.isValid()) {
        return false;
    }

    QSqlQuery query(db);

    if (!query.exec(QString("SELECT DISTINCT AlbumName FROM AlbumTable3 WHERE UID=%1").arg(UID)) || !query.next()) {
        return false; //没找到这个UID，需要先执行创建
    }

    auto album = query.value(0).toString();

    QStringList nameRows, pathHashRows;
    QVariantList atypes, uids;
    for (QString path : paths) {
        nameRows << album;
        pathHashRows << utils::base::hashByString(path);
        atypes << atype;
        uids << UID;
    }

    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
    }
    bool b = query.prepare("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash, AlbumDBType, UID) VALUES (null, ?, ?, ?, ?)");
    if (!b) {
        db.close();
        return false;
    }
    query.addBindValue(nameRows);
    query.addBindValue(pathHashRows);
    query.addBindValue(atypes);
    query.addBindValue(uids);

    if (! query.execBatch()) {
    }
    if (!query.exec("COMMIT")) {
    }

    //FIXME: Don't insert the repeated filepath into the same album
    //Delete the same data
    QString ps = "DELETE FROM AlbumTable3 where AlbumId NOT IN"
                 "(SELECT min(AlbumId) FROM AlbumTable3 GROUP BY"
                 " UID, PathHash, AlbumDBType) AND PathHash != \"%1\""
                 " AND AlbumDBType =:atype ";
    bool bs = query.prepare(ps.arg(EMPTY_HASH_STR));
    if (!bs) {
        db.close();
        mutex.unlock();
        return false;
    }
    query.bindValue(":atype", atype);
    if (!query.exec()) {
        //   qDebug() << "delete same date failed!";
    }
    if (!query.exec("COMMIT")) {
    }
    db.close();
    mutex.unlock();
    return true;
}

void DBManager::removeAlbum(int UID, AlbumDBType atype)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("DELETE FROM AlbumTable3 WHERE UID=:UID AND AlbumDBType =:atype");
    if (!b) {
        db.close();
        return;
    }
    query.bindValue(":UID", UID);
    query.bindValue(":atype", atype);
    if (!query.exec()) {
    }
    db.close();
}

void DBManager::removeFromAlbum(int UID, const QStringList &paths, AlbumDBType atype)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }

    QStringList pathHashs;
    QVariantList atypes;
    for (QString path : paths) {
        pathHashs << utils::base::hashByString(path);
        atypes << atype;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << "begin transaction failed.";
    }
    // Remove from albums table
    QString qs("DELETE FROM AlbumTable3 WHERE UID=%1 AND PathHash=? AND AlbumDBType=? ");
    bool b = query.prepare(qs.arg(UID));
    if (!b) {
        db.close();
        return;
    }
    query.addBindValue(pathHashs);
    query.addBindValue(atypes);
    bool suc = false;
    if (! query.execBatch()) {
    } else {
        suc = true;
    }
    if (!query.exec("COMMIT")) {
//        qDebug() << "COMMIT failed.";
    }
    db.close();
    mutex.unlock();
    if (suc)
        emit dApp->signalM->removedFromAlbum(UID, paths);
}

void DBManager::renameAlbum(int UID, const QString &newAlbum, AlbumDBType atype)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    QSqlQuery query(db);

    query.setForwardOnly(true);
    bool b = query.prepare("UPDATE AlbumTable3 SET "
                           "AlbumName = :newName "
                           "WHERE UID = :UID "
                           "AND AlbumDBType = :atype");
    if (!b) {
        db.close();
        return;
    }
    query.bindValue(":newName", newAlbum);
    query.bindValue(":UID", UID);
    query.bindValue(":atype",  atype);
    if (! query.exec()) {
    }
    db.close();
}

const DBImgInfoList DBManager::getInfosByNameTimeline(const QString &value) const
{
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);

    QString queryStr = "SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType FROM ImageTable3 "
                       "WHERE FileName like '%" + value + "%' OR Time like '%" + value + "%' ORDER BY Time DESC";

    bool b = query.prepare(queryStr);

    if (!b || !query.exec()) {
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(query.value(5).toString(), DATETIME_FORMAT_DATABASE);
            info.itemType = static_cast<ItemType>(query.value(6).toInt());
            infos << info;
        }
    }
    db.close();
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
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);

    //切换到UID后，纯关键字搜索应该不受影响
    QString queryStr = "SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType AlbumName FROM TrashTable3 "
                       "WHERE FileName like '%" + keywords + "%' OR Time like '%" + keywords + "%' ORDER BY Time DESC";

    bool b = query.prepare(queryStr);

    if (!b || !query.exec()) {
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(query.value(5).toString(), DATETIME_FORMAT_DATABASE);
            info.itemType = ItemType(query.value(6).toInt());
            infos << info;
        }
    }
    db.close();
    return infos;
}

const DBImgInfoList DBManager::getInfosForKeyword(int UID, const QString &keywords) const
{
    QMutexLocker mutex(&m_mutex);

    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }

    QString queryStr = "SELECT DISTINCT i.FilePath, i.FileName, i.Dir, i.Time, i.ChangeTime, i.ImportTime "
                       "FROM ImageTable3 AS i "
                       "inner join AlbumTable3 AS a on i.PathHash=a.PathHash AND a.UID=:UID "
                       "WHERE i.FileName like '%" + keywords + "%' ORDER BY Time DESC"; //OR Time like '%" + keywords + "%' 移除按时间搜索

    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare(queryStr);
    query.bindValue(":UID", UID);


    if (!b || ! query.exec()) {
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(query.value(5).toString(), DATETIME_FORMAT_DATABASE);
            infos << info;
        }
    }
    db.close();
    return infos;
}

const QMultiMap<QString, QString> DBManager::getAllPathAlbumNames() const
{
    QMutexLocker mutex(&m_mutex);

    QMultiMap<QString, QString> infos;
    infos.clear();
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }

    QString queryStr = "SELECT DISTINCT i.FilePath, a.UID "
                       "FROM ImageTable3 AS i, AlbumTable3 AS a "
                       "inner join AlbumTable3 on i.PathHash=a.PathHash "
                       "where a.AlbumDBType = 1";

    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare(queryStr);
    if (!b || ! query.exec()) {
//        qWarning() << "getAllPathAlbumNames failed: " << query.lastError();
    } else {
        using namespace utils::base;
        while (query.next()) {
            infos.insert(query.value(0).toString(), query.value(1).toString());
        }
    }
    db.close();
    return infos;
}

const DBImgInfoList DBManager::getImgInfos(const QString &key, const QString &value, const bool &needlock) const
{
    Q_UNUSED(needlock)
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType FROM ImageTable3 "
                                   "WHERE %1= :value ORDER BY Time DESC").arg(key));

    query.bindValue(":value", value);

    if (!b || !query.exec()) {
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(query.value(5).toString(), DATETIME_FORMAT_DATABASE);
            info.itemType = static_cast<ItemType>(query.value(6).toInt());
            infos << info;
        }
    }
    db.close();
    return infos;
}

bool DBManager::checkCustomAutoImportPathIsNotified(const QString &path)
{
    //检查是否是默认路径，这一段不涉及数据库操作，不需要加锁
    auto defaultPath = getDefaultNotifyPaths();
    for (auto &eachPath : defaultPath) {
        if (path.startsWith(eachPath) || eachPath.startsWith(path)) {
            return true;
        }
    }

    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) { //数据库炸了，返回true禁止外部操作
        return true;
    }

    //这里再去检查已有的数据库
    QSqlQuery query(db);
    if (!query.exec("SELECT FullPath FROM CustomAutoImportPathTable3")) {
        return true;
    }

    while (query.next()) {
        auto currentPath = query.value(0).toString();
        if (path.startsWith(currentPath) || currentPath.startsWith(path)) {
            return true;
        }
    }

    return false;
}

int DBManager::createNewCustomAutoImportPath(const QString &path, const QString &albumName)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return -1;
    }

    //1.新建相册
    int UID = albumMaxUID++;
    QSqlQuery query(db);

    if (!query.prepare("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash, AlbumDBType, UID) VALUES (null, ?, ?, ?, ?)")) {
        return -1;
    }

    query.addBindValue(albumName);
    query.addBindValue("7215ee9c7d9dc229d2921a40e899ec5f");
    query.addBindValue(AutoImport);
    query.addBindValue(UID);

    if (!query.exec()) {
        return -1;
    }

    //2.新建保存路径
    query.clear();

    if (!query.prepare("INSERT INTO CustomAutoImportPathTable3 (UID, FullPath, AlbumName) VALUES (?, ?, ?)")) {
        return -1;
    }

    query.addBindValue(UID);
    query.addBindValue(path);
    query.addBindValue(albumName);

    if (!query.exec()) {
        return -1;
    }

    return UID;
}

void DBManager::removeCustomAutoImportPath(int UID)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }

    //TODO：目前暂时不管删除问题

    //1.删除图片

    //2.删除路径

    //3.删除相册
}

std::map<int, QString> DBManager::getAllCustomAutoImportUIDAndPath()
{
    std::map<int, QString> result;

    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return result;
    }

    QSqlQuery query(db);
    if (!query.exec("SELECT UID, FullPath FROM CustomAutoImportPathTable3")) {
        return result;
    }

    while (query.next()) {
        result.insert(std::make_pair(query.value(0).toInt(), query.value(1).toString()));
    }

    return result;
}

const QSqlDatabase DBManager::getDatabase() const
{
    if (!m_db.open()) {
        m_db = QSqlDatabase::addDatabase("QSQLITE", "album_sql_connect"); //not dbConnection
        m_db.setDatabaseName(DATABASE_PATH + DATABASE_NAME);
        if (!m_db.open()) {
            return QSqlDatabase();
        }
    }
    return m_db;
}

void DBManager::checkDatabase()
{
    QDir dd(DATABASE_PATH);
    if (! dd.exists()) {
        dd.mkpath(DATABASE_PATH);
    } else {
    }
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }

    // 创建Table的语句都是加了IF NOT EXISTS的，直接运行就可以了
    QSqlQuery queryCreate(db);
    // ImageTable3
    //////////////////////////////////////////////////////////////
    //PathHash           | FilePath | FileName   | Dir  | Time | ChangeTime | ImportTime//
    //TEXT primari key   | TEXT     | TEXT       | TEXT | TEXT | TEXT       | TEXT      //
    //////////////////////////////////////////////////////////////
    bool b = queryCreate.exec(QString("CREATE TABLE IF NOT EXISTS ImageTable3 ( "
                                      "PathHash TEXT primary key, "
                                      "FilePath TEXT, "
                                      "FileName TEXT, "
                                      "Dir TEXT, "
                                      "Time TEXT, "
                                      "ChangeTime TEXT, "
                                      "ImportTime TEXT, "
                                      "FileType INTEGER, "
                                      "DataHash TEXT)"));
    if (!b) {
        qDebug() << "b CREATE TABLE exec failed.";
    }

    // AlbumTable3
    ///////////////////////////////////////////////////////////////////////////////////////
    //AlbumId               | AlbumName         | PathHash      |AlbumDBType    |UID     //
    //INTEGER primari key   | TEXT              | TEXT          |TEXT           |INTEGER //
    ///////////////////////////////////////////////////////////////////////////////////////
    bool c = queryCreate.exec(QString("CREATE TABLE IF NOT EXISTS AlbumTable3 ( "
                                      "AlbumId INTEGER primary key, "
                                      "AlbumName TEXT, "
                                      "PathHash TEXT,"
                                      "AlbumDBType INTEGER,"
                                      "UID INTEGER)"));
    if (!c) {
        qDebug() << "c CREATE TABLE exec failed.";
    }
    // TrashTable3
    //////////////////////////////////////////////////////////////
    //PathHash           | FilePath | FileName   | Dir  | Time | ChangeTime | ImportTime//
    //TEXT primari key   | TEXT     | TEXT       | TEXT | TEXT | TEXT       | TEXT      //
    //////////////////////////////////////////////////////////////
    bool d = queryCreate.exec(QString("CREATE TABLE IF NOT EXISTS TrashTable3 ( "
                                      "PathHash TEXT primary key, "
                                      "FilePath TEXT, "
                                      "FileName TEXT, "
                                      "Dir TEXT, "
                                      "Time TEXT, "
                                      "ChangeTime TEXT, "
                                      "ImportTime TEXT,"
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
    bool e = queryCreate.exec(QString("CREATE TABLE IF NOT EXISTS CustomAutoImportPathTable3 ( "
                                      "UID INTEGER primary key, "
                                      "FullPath TEXT, "
                                      "AlbumName TEXT)"));
    if (!e) {
        qDebug() << "d CREATE TABLE exec failed.";
    }

    // 判断ImageTable3中是否有ChangeTime字段
    QString strSqlImage = QString::fromLocal8Bit("select sql from sqlite_master where name = \"ImageTable3\" and sql like \"%ChangeTime%\"");
    QSqlQuery queryImage1(db);
    bool q = queryImage1.exec(strSqlImage);
    if (!q && queryImage1.next()) {
        // 无ChangeTime字段,则增加ChangeTime字段,赋值当前时间
        QString strDate = QDateTime::currentDateTime().toString(DATETIME_FORMAT_DATABASE);
        if (queryImage1.exec(QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"ChangeTime\" TEXT default \"%1\"")
                             .arg(strDate))) {
            qDebug() << "add ChangeTime success";
        }
    }

    // 判断ImageTable3中是否有ImportTime字段
    QString strSqlImportTime = "select sql from sqlite_master where name = 'ImageTable3' and sql like '%ImportTime%'";
    QSqlQuery queryImage2(db);
    if (!queryImage2.exec(strSqlImportTime)) {
        qDebug() << queryImage2.lastError();
    }
    if (!queryImage2.next()) {
        // 无ImportTime字段,则增加ImportTime字段
        QString strDate = QDateTime::currentDateTime().toString(DATETIME_FORMAT_DATABASE);
        if (queryImage2.exec(QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"ImportTime\" TEXT default \"%1\"")
                             .arg(strDate))) {
            qDebug() << "add ImportTime success";
        }
    }

    // 判断ImageTable3中是否有FileType字段，区分是图片还是视频
    QString strSqlFileType = QString::fromLocal8Bit(
                                 "select * from sqlite_master where name = 'ImageTable3' and sql like '%FileType%'");
    QSqlQuery queryFileType(db);
    int fileType = static_cast<int>(ItemTypePic);
    if (!queryFileType.exec(strSqlFileType)) {
        qDebug() << "add FileType failed";
    }
    if (!queryFileType.next()) {
        // 无FileType字段,则增加FileType字段,赋值1,默认是图片
        if (queryFileType.exec(QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"FileType\" INTEGER default \"%1\"")
                               .arg(QString::number(fileType)))) {
            qDebug() << "add FileType success";
        }
    }

    // 判断ImageTable3中是否有DataHash字段，根据文件内容产生的hash
    QString strDataHash = QString::fromLocal8Bit(
                              "select * from sqlite_master where name = 'ImageTable3' and sql like '%DataHash%'");
    QSqlQuery queryDataHash(db);
    if (queryDataHash.exec(strDataHash)) {
        if (!queryDataHash.next()) {
            // DataHash,则增加DataHash字段
            if (queryDataHash.exec(QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"DataHash\" TEXT default \"%1\"")
                                   .arg(""))) {
                qDebug() << "add FileType success";
            }
        }
    }

    // 判断AlbumTable3中是否有AlbumDBType字段
    QString strSqlDBType = QString::fromLocal8Bit("select * from sqlite_master where name = \"AlbumTable3\" and sql like \"%AlbumDBType%\"");
    QSqlQuery queryType(db);
    bool q2 = queryType.exec(strSqlDBType);
    if (q2 && !queryType.next()) {
        // 无AlbumDBType字段,则增加AlbumDBType字段, 全部赋值为个人相册
        if (queryType.exec(QString("ALTER TABLE \"AlbumTable3\" ADD COLUMN \"AlbumDBType\" INTEGER default %1")
                           .arg("1"))) {
            qDebug() << "add AlbumDBType success";
        }
        if (queryType.exec(QString("update AlbumTable3 SET AlbumDBType = 0 Where AlbumName = \"%1\" ")
                           .arg(COMMON_STR_FAVORITES))) {
        }
    }

    // 判断AlbumTable3中是否有UID字段
    // UID初始化标记
    bool uidIsInited = false;
    QString strSqlUID = QString::fromLocal8Bit("select * from sqlite_master where name = \"AlbumTable3\" and sql like \"%UID%\"");
    QSqlQuery queryUID(db);
    bool q3 = queryUID.exec(strSqlUID);
    if (q3 && !queryUID.next()) {
        // 无UID字段，则需要主动添加
        if (queryUID.exec(QString("ALTER TABLE \"AlbumTable3\" ADD COLUMN \"UID\" INTEGER default %1")
                          .arg("0"))) {
            qDebug() << "add UID success";
        }

        // UID字段添加完成后，还需要主动为其进行赋值
        //1.获取当前已存在的album name
        if (queryUID.exec(QString("SELECT DISTINCT \"AlbumName\" FROM \"AlbumTable3\""))) {
            qDebug() << "search album name success";
        }

        //2.在内存中为其进行编号
        QStringList nameList;
        while (queryUID.next()) {
            auto currentName = queryUID.value(0).toString();
            nameList.push_back(currentName);
        }

        //3.插入特殊UID字段
        insertSpUID(db, "Favorite", Favourite, u_Favorite);
        insertSpUID(db, "Camera", AutoImport, u_Camera);
        insertSpUID(db, "Screen Capture", AutoImport, u_ScreenCapture);//使用album name的SQL语句注意加冒号
        insertSpUID(db, "Draw", AutoImport, u_Draw);

        //4.初始化max uid
        albumMaxUID = u_CustomStart;

        //5.写入数据库
        for (auto &currentName : nameList) {
            if (!queryUID.exec(QString("UPDATE \"AlbumTable3\" SET UID = %1 WHERE \"AlbumName\" = \"%2\"").arg(albumMaxUID++).arg(currentName))) {
                qDebug() << "update AlbumTable3 UID failed";
            }
        }

        uidIsInited = true;
    }

    if (!uidIsInited) {
        //预先插入特殊UID
        insertSpUID(db, "Favorite", Favourite, u_Favorite);
        insertSpUID(db, "Camera", AutoImport, u_Camera);
        insertSpUID(db, "Screen Capture", AutoImport, u_ScreenCapture);//使用album name的SQL语句注意加冒号
        insertSpUID(db, "Draw", AutoImport, u_Draw);

        //搜索当前最大ID值
        QSqlQuery sqlId(db);
        if (!sqlId.exec("SELECT max(UID) FROM AlbumTable3") || !sqlId.next()) {
            qDebug() << "find max UID failed";
        }
        albumMaxUID = sqlId.value(0).toInt();
        albumMaxUID++;
    }

    // 判断TrashTable的版本
    QString strSqlTrashTable = QString::fromLocal8Bit("select * from sqlite_master where name = \"TrashTable3\"");
    QSqlQuery queryTrashReBuild(db);
    bool build = queryTrashReBuild.exec(strSqlTrashTable);
    if (!build) {
        qDebug() << queryTrashReBuild.lastError();
    }
    if (!queryTrashReBuild.next()) {
        //无新版TrashTable，则创建新表，导入旧表数据
        QString defaultImportTime = QDateTime::currentDateTime().toString(DATETIME_FORMAT_DATABASE);
        if (!queryTrashReBuild.exec(QString("CREATE TABLE IF NOT EXISTS TrashTable3 ( "
                                            "PathHash TEXT primary key, "
                                            "FilePath TEXT, "
                                            "FileName TEXT, "
                                            "Dir TEXT, "
                                            "Time TEXT, "
                                            "ChangeTime TEXT, "
                                            "ImportTime TEXT default \"%1\", "
                                            "FileType INTEGER)").arg(defaultImportTime))) {
            qDebug() << queryTrashReBuild.lastError();
        }
    } else {
        //判断TrashTable3是否包含FileType
        QString strSqlFileType = QString::fromLocal8Bit("select * from sqlite_master where name = \"TrashTable3\" and sql like \"%FileType%\"");
        QSqlQuery queryFileType(db);
        bool q2 = queryFileType.exec(strSqlFileType);
        if (!q2) {
            qDebug() << queryFileType.lastError();
        }
        if (!queryFileType.next()) {
            // 无FileType字段,则增加FileType字段, 全部赋值为图片
            int type = ItemType::ItemTypePic;
            if (queryFileType.exec(QString("ALTER TABLE \"TrashTable3\" ADD COLUMN \"FileType\" INTEGER default %1")
                                   .arg(QString::number(type)))) {
                qDebug() << "add AlbumDBType success";
            }
        }
    }

    QString strSqlTrashTableUpdate = QString::fromLocal8Bit("select * from sqlite_master where name = \"TrashTable\"");
    QSqlQuery queryTrashUpdate(db);
    bool update = queryTrashUpdate.exec(strSqlTrashTableUpdate);
    if (!update) {
        qDebug() << queryTrashUpdate.lastError();
    }
    if (queryTrashUpdate.next()) {
        if (!queryTrashUpdate.exec("REPLACE INTO TrashTable3 (PathHash, FilePath, FileName, Dir, Time, ChangeTime)"
                                   " SELECT PathHash, FilePath, FileName, Dir, Time, ChangeTime From TrashTable ")) {
            qDebug() << queryTrashUpdate.lastError();
        }
        if (!queryTrashUpdate.exec(QString("DROP TABLE TrashTable"))) {
            qDebug() << queryTrashUpdate.lastError();
        }
    }
    db.close();
}

void DBManager::insertSpUID(QSqlDatabase &db, const QString &albumName, AlbumDBType astype, SpUID UID)
{
    //使用传入的db防死锁
    QSqlQuery query(db);

    //1.检查当前需要新建的sp相册是否存在
    if (!query.exec(QString("SELECT UID FROM AlbumTable3 WHERE UID=%1").arg(UID)) || query.next()) {
        return;
    }

    //2.不存在则插入数据
    query.clear();

    if (!query.prepare("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash, AlbumDBType, UID) VALUES (null, ?, ?, ?, ?)")) {
        qDebug() << "insertSpUID failed";
    }

    query.addBindValue(albumName);
    query.addBindValue("7215ee9c7d9dc229d2921a40e899ec5f");
    query.addBindValue(astype);
    query.addBindValue(UID);

    if (!query.exec()) {
        qDebug() << "insertSpUID failed";
    }
}

const QStringList DBManager::getAllTrashPaths() const
{
    QMutexLocker mutex(&m_mutex);
    QStringList paths;
    QSqlDatabase db = getDatabase();
    if (! db.isValid())
        return paths;

    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT "
                           "FilePath "
                           "FROM TrashTable3 ORDER BY Time DESC");
    if (!b || ! query.exec()) {
        //   qWarning() << "Get Data from TrashTable failed: " << query.lastError();
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return paths;
    } else {
        while (query.next()) {
            paths << query.value(0).toString();
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();
    return paths;
}

const DBImgInfoList DBManager::getAllTrashInfos() const
{
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType "
                           "FROM TrashTable3 ORDER BY ImportTime DESC");
    if (!b || ! query.exec()) {
        //  qWarning() << "Get data from TrashTable failed: " << query.lastError();
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return infos;
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            if (info.filePath.isEmpty()) //如果路径为空
                continue;
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(query.value(5).toString(), DATETIME_FORMAT_DATABASE);
            info.itemType = ItemType(query.value(6).toInt());
            infos << info;
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();
    return infos;
}

void DBManager::insertTrashImgInfos(const DBImgInfoList &infos)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (infos.isEmpty() || ! db.isValid()) {
        return;
    }

    QVariantList pathhashs, filenames, filepaths, dirs, times, changetimes, importtimes, filetypes;

    for (DBImgInfo info : infos) {
        filenames << info.fileName;
        filepaths << info.filePath;
        pathhashs << utils::base::hashByString(info.filePath);
        dirs << info.dirHash;
        times << info.time.toString("yyyy.MM.dd");
        changetimes << info.changeTime.toString(DATETIME_FORMAT_DATABASE);
        importtimes << info.importTime.toString(DATETIME_FORMAT_DATABASE);
        filetypes << static_cast<int>(info.itemType);
    }

    // Insert into TrashTable
    QSqlQuery query(db);
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
        qDebug() << "begin transaction failed.";
    }
    bool b = query.prepare("REPLACE INTO TrashTable3 "
                           "(PathHash, FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType) VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    if (!b) {
        db.close();
        return;
    }
    query.addBindValue(pathhashs);
    query.addBindValue(filepaths);
    query.addBindValue(filenames);
    query.addBindValue(dirs);
    query.addBindValue(times);
    query.addBindValue(changetimes);
    query.addBindValue(importtimes);
    query.addBindValue(filetypes);
    if (! query.execBatch()) {
        //   qWarning() << "Insert data into TrashTable failed: "
        //             << query.lastError();
        if (!query.exec("COMMIT")) {
            qDebug() << "COMMIT failed.";
        }
        db.close();
    } else {
        if (!query.exec("COMMIT")) {
            qDebug() << "COMMIT failed.";
        }
        db.close();
        mutex.unlock();
        emit dApp->signalM->imagesTrashInserted(/*infos*/);
    }
    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
}

void DBManager::removeTrashImgInfos(const QStringList &paths)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (paths.isEmpty() || ! db.isValid()) {
        return;
    }

    // Collect info before removing data
//    DBImgInfoList infos;
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hashByString(path);
//        infos << getInfoByPath(path);
    }

    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);

    // Remove from image table
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << "begin transaction failed.";
    }
    QString qs = "DELETE FROM TrashTable3 WHERE PathHash=?";
    bool b = query.prepare(qs);
    if (!b) {
        db.close();
        return;
    }
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        //  qWarning() << "Remove data from TrashTable failed: "
        //            << query.lastError();
        if (!query.exec("COMMIT")) {
//            qDebug() << "COMMIT failed.";
        }
        db.close();
    } else {
        if (!query.exec("COMMIT")) {
//            qDebug() << "COMMIT failed.";
        }
        db.close();
        mutex.unlock();
        emit dApp->signalM->imagesTrashRemoved(/*infos*/);
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
}

void DBManager::removeTrashImgInfosNoSignal(const QStringList &paths)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (paths.isEmpty() || ! db.isValid()) {
        return;
    }

    // Collect info before removing data
//    DBImgInfoList infos;
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hashByString(path);
//        infos << getInfoByPath(path);
    }

    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << "begin transaction failed.";
    }
    QString qs = "DELETE FROM AlbumTable3 WHERE PathHash=?";
    bool b = query.prepare(qs);
    if (!b) {
        db.close();
        return;
    }
    query.addBindValue(pathHashs);
    query.execBatch();
    if (!query.exec("COMMIT")) {
//        qDebug() << "COMMIT failed.";
    }

    // Remove from image table
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << "begin transaction failed.";
    }
    qs = "DELETE FROM TrashTable3 WHERE PathHash=?";
    bool bs = query.prepare(qs);
    if (!bs) {
        db.close();
        return;
    }
    query.addBindValue(pathHashs);
    query.execBatch();
    if (!query.exec("COMMIT")) {
//        qDebug() << "COMMIT failed.";
    }
    // 连接使用完后需要释放回数据库连接池
    ////ConnectionPool::closeConnection(db);
    db.close();
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
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType FROM TrashTable3 "
                                   "WHERE %1= :value ORDER BY Time DESC").arg(key));

    query.bindValue(":value", value);

    if (!b || !query.exec()) {
        //  qWarning() << "Get Image from database failed: " << query.lastError();
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            if (info.filePath.isEmpty()) //如果路径为空
                continue;
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(query.value(5).toString(), DATETIME_FORMAT_DATABASE);
            info.itemType = ItemType(query.value(6).toInt());

            infos << info;
        }
    }
    // 连接使用完后需要释放回数据库连接池
    ////ConnectionPool::closeConnection(db);
    db.close();
    return infos;
}

int DBManager::getTrashImgsCount() const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.isValid()) {
        return 0;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << "begin transaction failed.";
    }
    bool b = query.prepare("SELECT COUNT(*) FROM TrashTable3");
    if (!b) {
        db.close();
        return 0;
    }
    if (query.exec()) {
        query.first();
        int count = query.value(0).toInt();
        if (!query.exec("COMMIT")) {
//            qDebug() << "COMMIT failed.";
        }
        // 连接使用完后需要释放回数据库连接池
        ////ConnectionPool::closeConnection(db);
        db.close();
        return count;
    }
    // 连接使用完后需要释放回数据库连接池
    ////ConnectionPool::closeConnection(db);
    db.close();
    return 0;
}
