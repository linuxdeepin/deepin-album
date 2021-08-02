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
#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

namespace {
const QString DATABASE_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                              + QDir::separator() + "deepin" + QDir::separator() + "deepin-album" + QDir::separator();
const QString DATABASE_NAME = "deepinalbum.db";
const QString EMPTY_HASH_STR = utils::base::hash(QString(" "));

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
    , m_db(QSqlDatabase::addDatabase("QSQLITE", "album_sql_connect"))
{
    m_db.setDatabaseName(DATABASE_PATH + DATABASE_NAME);
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

const DBImgInfoList DBManager::getAllInfos(int loadCount) const
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
        b = query.prepare("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime FROM ImageTable3 order by Time desc");
    } else {
        b = query.prepare("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime FROM ImageTable3 order by Time desc limit 80");
    }
    if (!b || ! query.exec()) {
        qDebug() << query.lastError();
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
    QVariantList pathhashs, filenames, filepaths, dirs, times, changetimes, importtimes;
    for (DBImgInfo info : infos) {
        filenames << info.fileName;
        filepaths << info.filePath;
        pathhashs << utils::base::hash(info.filePath);
        dirs << info.dirHash;
        times << info.time.toString("yyyy.MM.dd");
        changetimes << info.changeTime.toString(DATETIME_FORMAT_DATABASE);
        importtimes << info.importTime.toString(DATETIME_FORMAT_DATABASE);
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
        qDebug() << query.lastError();
    }
    bool b = query.prepare("REPLACE INTO ImageTable3 (PathHash, FilePath, FileName, Dir, Time, ChangeTime, ImportTime) VALUES (?, ?, ?, ?, ?, ?, ?)");
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
    if (! query.execBatch()) {
        if (!query.exec("COMMIT")) {
            qDebug() << query.lastError();
        }
        db.close();
    } else {
        if (!query.exec("COMMIT")) {
            qDebug() << query.lastError();
        }
        db.close();
        mutex.unlock();
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
        pathHashs << utils::base::hash(path);
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
        qDebug() << query.lastError();
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
        qDebug() << query.lastError();
    }
    // Remove from image table
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
        qDebug() << query.lastError();
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
            qDebug() << query.lastError();
        }
        db.close();
    } else {
        if (!query.exec("COMMIT")) {
            qDebug() << query.lastError();
        }
        db.close();
        mutex.unlock();
        emit dApp->signalM->imagesRemoved();
        qDebug() << "------" << __FUNCTION__ << "size = " << infos.size();
        emit dApp->signalM->imagesRemovedPar(infos);
    }
//    qDebug() << "------" << __FUNCTION__ << "" << QThread::currentThreadId();
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
        pathHashs << utils::base::hash(path);
    }

    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
        qDebug() << "begin transaction failed.";
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
        qDebug() << "COMMIT failed.";
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

const QStringList DBManager::getAllAlbumNames(AlbumDBType atype) const
{
    QMutexLocker mutex(&m_mutex);
    QStringList list;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return list;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT DISTINCT AlbumName FROM AlbumTable3 WHERE AlbumDBType =:atype");
    query.bindValue(":atype", atype);
    if (!b || !query.exec()) {
    } else {
        while (query.next()) {
            list << query.value(0).toString();
        }
    }
    db.close();

    return list;
}

const QStringList DBManager::getPathsByAlbum(const QString &album, AlbumDBType atype) const
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
                           "AND a.AlbumName=:album "
                           "AND a.AlbumDBType=:atype "
                           /*"AND FilePath != \" \" "*/);
    query.bindValue(":album", album);
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

const DBImgInfoList DBManager::getInfosByAlbum(const QString &album, AlbumDBType atype) const
{
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT DISTINCT i.FilePath, i.FileName, i.Dir, i.Time, i.ChangeTime, i.ImportTime "
                           "FROM ImageTable3 AS i, AlbumTable3 AS a "
                           "WHERE i.PathHash=a.PathHash "
                           "AND a.AlbumName=:album "
                           "AND a.AlbumDBType=:atype ");
    query.bindValue(":album", album);
    query.bindValue(":atype", atype);
    if (!b || ! query.exec()) {
        //    qWarning() << "Get ImgInfo by album failed: " << query.lastError();
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

int DBManager::getImgsCountByAlbum(const QString &album, AlbumDBType atype) const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return 0;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    QString ps = "SELECT COUNT(*) FROM AlbumTable3 "
                 "WHERE AlbumName =:album "
                 "AND PathHash != \"%1\" "
                 "AND AlbumDBType =:atype ";
    bool b = query.prepare(ps.arg(EMPTY_HASH_STR));
    if (!b) {
        db.close();
        return 0;
    }
    query.bindValue(":album", album);
    query.bindValue(":atype", atype);
    if (query.exec()) {
        query.first();
        db.close();
        return query.value(0).toInt();
    } else {
        db.close();
        return 0;
    }
}

bool DBManager::isImgExistInAlbum(const QString &album, const QString &path, AlbumDBType atype) const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return false;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE PathHash = :hash "
                           "AND AlbumName = :album "
                           "AND AlbumDBType =:atype ");
    if (!b) {
        db.close();
        return false;
    }
    query.bindValue(":hash", utils::base::hash(path));
    query.bindValue(":album", album);
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

bool DBManager::isAlbumExistInDB(const QString &album, AlbumDBType atype) const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return false;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE AlbumName = :album AND AlbumDBType =:atype");
    if (!b) {
        db.close();
        return false;
    }
    query.bindValue(":album", album);
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

void DBManager::insertIntoAlbum(const QString &album, const QStringList &paths, AlbumDBType atype)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (!db.isValid() || album.isEmpty()) {
        return;
    }
    QStringList nameRows, pathHashRows;
    QVariantList atypes;
    for (QString path : paths) {
        nameRows << album;
        pathHashRows << utils::base::hash(path);
        atypes << atype;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
    }
    bool b = query.prepare("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash, AlbumDBType) VALUES (null, ?, ?, ?)");
    if (!b) {
        db.close();
        return;
    }
    query.addBindValue(nameRows);
    query.addBindValue(pathHashRows);
    query.addBindValue(atypes);

    if (! query.execBatch()) {
    }
    if (!query.exec("COMMIT")) {
    }

    //FIXME: Don't insert the repeated filepath into the same album
    //Delete the same data
    QString ps = "DELETE FROM AlbumTable3 where AlbumId NOT IN"
                 "(SELECT min(AlbumId) FROM AlbumTable3 GROUP BY"
                 " AlbumName, PathHash, AlbumDBType) AND PathHash != \"%1\""
                 " AND AlbumDBType =:atype ";
    bool bs = query.prepare(ps.arg(EMPTY_HASH_STR));
    if (!bs) {
        db.close();
        mutex.unlock();
        return;
    }
    query.bindValue(":atype", atype);
    if (!query.exec()) {
        //   qDebug() << "delete same date failed!";
    }
    if (!query.exec("COMMIT")) {
    }
    db.close();
    mutex.unlock();
}

void DBManager::insertIntoAlbumNoSignal(const QString &album, const QStringList &paths, AlbumDBType atype)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid() || album.isEmpty()) {
        return;
    }
    QStringList nameRows, pathHashRows;
    QVariantList atypes;
    for (QString path : paths) {
        nameRows << album;
        pathHashRows << utils::base::hash(path);
        atypes << atype;
    }

    QSqlQuery query(db);
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
        qDebug() << "begin transaction failed.";
    }
    bool b = query.prepare("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash, AlbumDBType) "
                           "VALUES (null, ?, ?, ?)");
    if (!b) {
        db.close();
        return;
    }
    query.addBindValue(nameRows);
    query.addBindValue(pathHashRows);
    query.addBindValue(atypes);
    if (! query.execBatch()) {
    }
    if (!query.exec("COMMIT")) {
        qDebug() << "COMMIT failed.";
    }

    //FIXME: Don't insert the repeated filepath into the same album
    //Delete the same data
    QString ps = "DELETE FROM AlbumTable3 where AlbumId NOT IN "
                 "(SELECT min(AlbumId) FROM AlbumTable3 GROUP BY "
                 " AlbumName, PathHash) AND PathHash != \"%1\" "
                 " AND AlbumDBType =:atype ";
    bool bs = query.prepare(ps.arg(EMPTY_HASH_STR));
    if (!bs) {
        db.close();
        return;
    }
    query.bindValue(":atype", atype);
    if (!query.exec()) {
    }
    if (!query.exec("COMMIT")) {
        qDebug() << "COMMIT failed.";
    }
    db.close();
}


void DBManager::removeAlbum(const QString &album, AlbumDBType atype)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("DELETE FROM AlbumTable3 WHERE AlbumName=:album AND AlbumDBType =:atype");
    if (!b) {
        db.close();
        return;
    }
    query.bindValue(":album", album);
    query.bindValue(":atype", atype);
    if (!query.exec()) {
    }
    db.close();
}

void DBManager::removeFromAlbum(const QString &album, const QStringList &paths, AlbumDBType atype)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }

    QStringList pathHashs;
    QVariantList atypes;
    for (QString path : paths) {
        pathHashs << utils::base::hash(path);
        atypes << atype;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
        qDebug() << "begin transaction failed.";
    }
    // Remove from albums table
    QString qs("DELETE FROM AlbumTable3 WHERE AlbumName=\"%1\" AND PathHash=? AND AlbumDBType=? ");
    bool b = query.prepare(qs.arg(album));
    if (!b) {
        db.close();
        mutex.unlock();
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
        qDebug() << "COMMIT failed.";
    }
    db.close();
    mutex.unlock();
    if (suc)
        emit dApp->signalM->removedFromAlbum(album, paths);
}

void DBManager::renameAlbum(const QString &oldAlbum, const QString &newAlbum, AlbumDBType atype)
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
                           "WHERE AlbumName = :oldName "
                           "AND AlbumDBType = :atype");
    if (!b) {
        db.close();
        return;
    }
    query.bindValue(":newName", newAlbum);
    query.bindValue(":oldName", oldAlbum);
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

    QString queryStr = "SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime FROM ImageTable3 "
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

    QString queryStr = "SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime AlbumName FROM TrashTable3 "
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
            infos << info;
        }
    }
    db.close();
    return infos;
}

const DBImgInfoList DBManager::getInfosForKeyword(const QString &album, const QString &keywords) const
{
    QMutexLocker mutex(&m_mutex);

    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }

    QString queryStr = "SELECT DISTINCT i.FilePath, i.FileName, i.Dir, i.Time, i.ChangeTime, i.ImportTime "
                       "FROM ImageTable3 AS i "
                       "inner join AlbumTable3 AS a on i.PathHash=a.PathHash AND a.AlbumName=:album "
                       "WHERE i.FileName like %" + keywords + "%' OR Time like %" + keywords + "%' ORDER BY Time DESC";

    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare(queryStr);
    query.bindValue(":album", album);


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

    QString queryStr = "SELECT DISTINCT i.FilePath, a.AlbumName "
                       "FROM ImageTable3 AS i, AlbumTable3 AS a "
                       "inner join AlbumTable3 on i.PathHash=a.PathHash "
                       "where a.AlbumDBType = 1";

    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare(queryStr);
    if (!b || ! query.exec()) {
        qWarning() << "getAllPathAlbumNames failed: " << query.lastError();
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
    bool b = query.prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime FROM ImageTable3 "
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
            infos << info;
        }
    }
    db.close();
    return infos;
}

const QSqlDatabase DBManager::getDatabase() const
{
    if (!m_db.open()) {
        qDebug() << "zy------Open database error:" << m_db.lastError();
        m_db = QSqlDatabase::addDatabase("QSQLITE", "album_sql_connect"); //not dbConnection
        m_db.setDatabaseName(DATABASE_PATH + DATABASE_NAME);
        if (!m_db.open()) {
            qDebug() << "zy------Open database error:" << m_db.lastError();
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
    bool tableExist = false;
    QSqlQuery query(db);
    query.setForwardOnly(true);
    bool b = query.prepare("SELECT name FROM sqlite_master "
                           "WHERE type=\"table\" AND name = \"ImageTable3\"");
    if (!b) {
        db.close();
        return;
    }
    if (query.exec() && query.first()) {
        tableExist = ! query.value(0).toString().isEmpty();
    }
    //if tables not exist, create it.
    if (!tableExist) {
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
                                          "ImportTime TEXT)"));
        if (!b) {
            qDebug() << "b CREATE TABLE exec failed.";
        }

        // AlbumTable3
        ///////////////////////////////////////////////////////////////////////////
        //AlbumId               | AlbumName         | PathHash      |AlbumDBType //
        //INTEGER primari key   | TEXT              | TEXT          |TEXT        //
        ///////////////////////////////////////////////////////////////////////////
        bool c = queryCreate.exec(QString("CREATE TABLE IF NOT EXISTS AlbumTable3 ( "
                                          "AlbumId INTEGER primary key, "
                                          "AlbumName TEXT, "
                                          "PathHash TEXT,"
                                          "AlbumDBType INTEGER)"));
        if (!c) {
            qDebug() << "c CREATE TABLE exec failed.";
        }
        // TrashTable
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
                                          "ImportTime TEXT)"));
        if (!d) {
            qDebug() << "d CREATE TABLE exec failed.";
        }
//        // Check if there is an old version table exist or not
//        //TODO: AlbumTable's primary key is changed, need to importVersion again
    } else {
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

        // 判断AlbumTable3中是否有AlbumDBType字段
        QString strSqlDBType = QString::fromLocal8Bit("select * from sqlite_master where name = \"AlbumTable3\" and sql like \"%AlbumDBType%\"");
        QSqlQuery queryType(db);
        bool q2 = queryType.exec(strSqlDBType);
        if (!q2 && queryType.next()) {
            // 无AlbumDBType字段,则增加AlbumDBType字段, 全部赋值为个人相册
            if (queryType.exec(QString("ALTER TABLE \"AlbumTable3\" ADD COLUMN \"AlbumDBType\" INTEGER default %1")
                               .arg("1"))) {
                qDebug() << "add AlbumDBType success";
            }
            if (queryType.exec(QString("update AlbumTable3 SET AlbumDBType = 0 Where AlbumName = \"%1\" ")
                               .arg(COMMON_STR_FAVORITES))) {
            }
        }
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
                                            "ImportTime TEXT default \"%1\")").arg(defaultImportTime))) {
            qDebug() << queryTrashReBuild.lastError();
        }
    }

    QString strSqlTrashTableUpdate = QString::fromLocal8Bit("select * from sqlite_master where name = \"TrashTable\"");
    QSqlQuery queryTrashUpdate(db);
    bool update = queryTrashUpdate.exec(strSqlTrashTableUpdate);
    if (!update) {
        qDebug() << queryTrashUpdate.lastError();
    }
    if (queryTrashUpdate.next()) {
        if (!queryTrashUpdate.exec("REPLACE INTO TrashTable3 (PathHash, FilePath, FileName, Dir, Time, ChangeTime) SELECT PathHash, FilePath, FileName, Dir, Time, ChangeTime From TrashTable ")) {
            qDebug() << queryTrashUpdate.lastError();
        }
        if (!queryTrashUpdate.exec(QString("DROP TABLE TrashTable"))) {
            qDebug() << queryTrashUpdate.lastError();
        }
    }
    db.close();
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
    bool b = query.prepare("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime "
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

    QVariantList pathhashs, filenames, filepaths, dirs, times, changetimes, importtimes;

    for (DBImgInfo info : infos) {
        filenames << info.fileName;
        filepaths << info.filePath;
        pathhashs << utils::base::hash(info.filePath);
        dirs << info.dirHash;
        times << info.time.toString("yyyy.MM.dd");
        changetimes << info.changeTime.toString(DATETIME_FORMAT_DATABASE);
        importtimes << info.importTime.toString(DATETIME_FORMAT_DATABASE);
    }

    // Insert into TrashTable
    QSqlQuery query(db);
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
        qDebug() << "begin transaction failed.";
    }
    bool b = query.prepare("REPLACE INTO TrashTable3 "
                           "(PathHash, FilePath, FileName, Dir, Time, ChangeTime, ImportTime) VALUES (?, ?, ?, ?, ?, ?, ?)");
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
        pathHashs << utils::base::hash(path);
//        infos << getInfoByPath(path);
    }

    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);

    // Remove from image table
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
        qDebug() << "begin transaction failed.";
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
            qDebug() << "COMMIT failed.";
        }
        db.close();
    } else {
        if (!query.exec("COMMIT")) {
            qDebug() << "COMMIT failed.";
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
        pathHashs << utils::base::hash(path);
//        infos << getInfoByPath(path);
    }

    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
        qDebug() << "begin transaction failed.";
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
        qDebug() << "COMMIT failed.";
    }

    // Remove from image table
    if (!query.exec("BEGIN IMMEDIATE TRANSACTION")) {
        qDebug() << "begin transaction failed.";
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
        qDebug() << "COMMIT failed.";
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
    bool b = query.prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime FROM TrashTable3 "
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
        qDebug() << "begin transaction failed.";
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
            qDebug() << "COMMIT failed.";
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
