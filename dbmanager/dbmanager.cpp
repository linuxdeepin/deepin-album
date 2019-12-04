/*
 * Copyright (C) 2016 ~ 2018 Deepin Technology Co., Ltd.
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

const QString DATABASE_PATH = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() + "deepin" + QDir::separator() + "deepin-album" + QDir::separator();
//const QString DATABASE_PATH = QDir::homePath() + "/.local/share/deepin/deepin-album/";
const QString DATABASE_NAME = "deepinalbum.db";
const QString EMPTY_HASH_STR = utils::base::hash(QString(" "));

}  // namespace

DBManager *DBManager::m_dbManager = NULL;

DBManager *DBManager::instance()
{
    if (!m_dbManager) {
        m_dbManager = new DBManager();
    }

    return m_dbManager;
}

DBManager::DBManager(QObject *parent)
    : QObject(parent)
    , m_connectionName("default_connection")
{
    checkDatabase();
}

const QStringList DBManager::getAllPaths() const
{
    QStringList paths;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid())
        return paths;

    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare( "SELECT "
                   "FilePath "
                   "FROM ImageTable3 ORDER BY Time DESC");
    if (! query.exec()) {
        qWarning() << "Get Data from ImageTable3 failed: " << query.lastError();
        mutex.unlock();
        return paths;
    }
    else {
        while (query.next()) {
            paths << query.value(0).toString();
        }
    }
    mutex.unlock();

    return paths;
}

const DBImgInfoList DBManager::getAllInfos() const
{
    DBImgInfoList infos;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare( "SELECT FilePath, FileName, Dir, Time, ChangeTime "
                   "FROM ImageTable3");
    if (! query.exec()) {
        qWarning() << "Get data from ImageTable3 failed: " << query.lastError();
        mutex.unlock();
        return infos;
    }
    else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = stringToDateTime(query.value(4).toString());

            infos << info;
        }
    }
    mutex.unlock();

    return infos;
}

const QStringList DBManager::getAllTimelines() const
{
    QStringList times;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid())
        return times;

    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare( "SELECT DISTINCT Time "
                   "FROM ImageTable3 ORDER BY Time DESC");
    if (! query.exec()) {
        qWarning() << "Get Data from ImageTable3 failed: " << query.lastError();
        mutex.unlock();
        return times;
    }
    else {
        while (query.next()) {
            times << query.value(0).toString();
        }
    }
    mutex.unlock();

    return times;
}

const DBImgInfoList DBManager::getInfosByTimeline(const QString &timeline) const
{
    const DBImgInfoList list = getImgInfos("Time", timeline);
    if (list.count() < 1) {
        return DBImgInfoList();
    }
    else {
        return list;
    }
}

const QStringList DBManager::getImportTimelines() const
{
    QStringList times;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid())
        return times;

    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare( "SELECT DISTINCT ChangeTime "
                   "FROM ImageTable3 ORDER BY ChangeTime DESC");
    if (! query.exec()) {
        qWarning() << "Get Data from ImageTable3 failed: " << query.lastError();
        mutex.unlock();
        return times;
    }
    else {
        while (query.next()) {
            times << query.value(0).toString();
        }
    }
    mutex.unlock();

    return times;
}

const DBImgInfoList DBManager::getInfosByImportTimeline(const QString &timeline) const
{
    const DBImgInfoList list = getImgInfos("ChangeTime", timeline);
    if (list.count() < 1) {
        return DBImgInfoList();
    }
    else {
        return list;
    }
}

const DBImgInfo DBManager::getInfoByName(const QString &name) const
{
    DBImgInfoList list = getImgInfos("FileName", name);
    if (list.count() < 1) {
        return DBImgInfo();
    }
    else {
        return list.first();
    }
}

const DBImgInfo DBManager::getInfoByPath(const QString &path) const
{
    DBImgInfoList list = getImgInfos("FilePath", path);
    if (list.count() != 1) {
        return DBImgInfo();
    }
    else {
        return list.first();
    }
}

const DBImgInfo DBManager::getInfoByPathHash(const QString &pathHash) const
{
    DBImgInfoList list = getImgInfos("PathHash", pathHash);
    if (list.count() != 1) {
        return DBImgInfo();
    }
    else {
        return list.first();
    }
}

int DBManager::getImgsCount() const
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return 0;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare( "SELECT COUNT(*) FROM ImageTable3" );
    if (query.exec()) {
        query.first();
        int count = query.value(0).toInt();
        query.exec("COMMIT");
        mutex.unlock();
        return count;
    }
    mutex.unlock();
    return 0;
}

int DBManager::getImgsCountByDir(const QString &dir) const
{
    const QSqlDatabase db = getDatabase();
    if (dir.isEmpty() || ! db.isValid()) {
        return 0;
    }

    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare("SELECT COUNT(*) FROM ImageTable3 "
                          "WHERE Dir=:Dir AND FilePath !=\" \"");
    query.bindValue(":Dir", utils::base::hash(dir));
    if (query.exec()) {
        query.first();
        mutex.unlock();
        return query.value(0).toInt();
    }
    else {
        qDebug() << "Get images count by Dir failed :" << query.lastError();
        mutex.unlock();
        return 0;
    }
}

const QStringList DBManager::getPathsByDir(const QString &dir) const
{
    QStringList list;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return list;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare( "SELECT DISTINCT FilePath FROM ImageTable3 "
                   "WHERE Dir=:dir " );
    query.bindValue(":dir", utils::base::hash(dir));
    if (! query.exec() ) {
        qWarning() << "Get Paths from ImageTable3 failed: " << query.lastError();
        mutex.unlock();
    }
    else {
        while (query.next()) {
            list << query.value(0).toString();
        }
    }
    mutex.unlock();

    return list;
}

bool DBManager::isImgExist(const QString &path) const
{
    const QSqlDatabase db = getDatabase();
    if (db.isValid()) {
        return false;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare( "SELECT COUNT(*) FROM ImageTable3 WHERE FilePath = :path" );
    query.bindValue( ":path", path );
    if (query.exec()) {
        query.first();
        if (query.value(0).toInt() > 0) {
            query.exec("COMMIT");
            mutex.unlock();
            return true;
        }
    }
    mutex.unlock();

    return false;
}

void DBManager::insertImgInfos(const DBImgInfoList &infos)
{
    const QSqlDatabase db = getDatabase();
    if (infos.isEmpty() || ! db.isValid()) {
        return;
    }

    QVariantList pathhashs, filenames, filepaths, dirs, times, changetimes;

    for (DBImgInfo info : infos) {
        filenames << info.fileName;
        filepaths << info.filePath;
        pathhashs << utils::base::hash(info.filePath);
        dirs << info.dirHash;
        times << utils::base::timeToString(info.time, true);
        changetimes << info.changeTime.toString(DATETIME_FORMAT_DATABASE);
    }

    QMutexLocker mutex(&m_mutex);
    // Insert into ImageTable3
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare( "REPLACE INTO ImageTable3 "
                   "(PathHash, FilePath, FileName, Dir, Time, ChangeTime) VALUES (?, ?, ?, ?, ?, ?)" );
    query.addBindValue(pathhashs);
    query.addBindValue(filepaths);
    query.addBindValue(filenames);
    query.addBindValue(dirs);
    query.addBindValue(times);
    query.addBindValue(changetimes);
    if (! query.execBatch()) {
        qWarning() << "Insert data into ImageTable3 failed: "
                   << query.lastError();
        query.exec("COMMIT");
        mutex.unlock();
    }
    else {
        query.exec("COMMIT");
        mutex.unlock();
        emit dApp->signalM->imagesInserted(infos);
    }
}

void DBManager::removeImgInfos(const QStringList &paths)
{
    QSqlDatabase db = getDatabase();
    if (paths.isEmpty() || ! db.isValid()) {
        return;
    }

    // Collect info before removing data
    DBImgInfoList infos;
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hash(path);
        infos << getInfoByPath(path);
    }

    QMutexLocker mutex(&m_mutex);
    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    QString qs = "DELETE FROM AlbumTable3 WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove data from AlbumTable3 failed: "
                   << query.lastError();
        query.exec("COMMIT");
    }
    else {
        query.exec("COMMIT");
    }

    // Remove from image table
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    qs = "DELETE FROM ImageTable3 WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove data from ImageTable3 failed: "
                   << query.lastError();
        query.exec("COMMIT");
    }
    else {
        mutex.unlock();
        emit dApp->signalM->imagesRemoved(infos);
        query.exec("COMMIT");
    }
    mutex.unlock();
}

void DBManager::removeImgInfosNoSignal(const QStringList &paths)
{
    QSqlDatabase db = getDatabase();
    if (paths.isEmpty() || ! db.isValid()) {
        return;
    }

    // Collect info before removing data
    DBImgInfoList infos;
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hash(path);
        infos << getInfoByPath(path);
    }

    QMutexLocker mutex(&m_mutex);
    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    QString qs = "DELETE FROM AlbumTable3 WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove data from AlbumTable3 failed: "
                   << query.lastError();
        query.exec("COMMIT");
    }
    else {
        query.exec("COMMIT");
    }

    // Remove from image table
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    qs = "DELETE FROM ImageTable3 WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove data from ImageTable3 failed: "
                   << query.lastError();
        query.exec("COMMIT");
    }
    else {
        mutex.unlock();
        query.exec("COMMIT");
    }
    mutex.unlock();
}

void DBManager::removeDir(const QString &dir)
{
    QSqlDatabase db = getDatabase();
    if (dir.isEmpty() || ! db.isValid()) {
        return;
    }

    const QString dirHash = utils::base::hash(dir);
    // Collect info before removing data
    DBImgInfoList infos = getImgInfos("Dir", dirHash);
    QStringList pathHashs;
    for (auto info : infos) {
        pathHashs << utils::base::hash(info.filePath);
    }

    QMutexLocker mutex(&m_mutex);
    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    QString qs = "DELETE FROM AlbumTable3 WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove data from AlbumTable3 failed: "
                   << query.lastError();
    }
    query.exec("COMMIT");

    // Remove from image table
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    qs = "DELETE FROM ImageTable3 WHERE Dir=:Dir";
    query.prepare(qs);
    query.bindValue(":Dir", dirHash);
    if (! query.exec()) {
        qWarning() << "Remove dir's images from ImageTable3 failed: "
                   << query.lastError();
        query.exec("COMMIT");
        mutex.unlock();
    }
    else {
        query.exec("COMMIT");
        emit dApp->signalM->imagesRemoved(infos);
    }
}

const DBAlbumInfo DBManager::getAlbumInfo(const QString &album) const
{
    DBAlbumInfo info;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return info;
    }

    info.name = album;
    QStringList pathHashs;

    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    QString ps = "SELECT DISTINCT PathHash FROM AlbumTable3 "
                 "WHERE AlbumName =:name AND PathHash != \"%1\" ";
    query.prepare( ps.arg(EMPTY_HASH_STR) );
    query.bindValue(":name", album);
    if ( ! query.exec() ) {
        qWarning() << "Get data from AlbumTable3 failed: "
                   << query.lastError();
    }
    else {
        while (query.next()) {
            pathHashs << query.value(0).toString();
        }
    }
    mutex.unlock();
    info.count = pathHashs.length();
    if (pathHashs.length() == 1) {
        info.endTime = getInfoByPathHash(pathHashs.first()).time;
        info.beginTime = info.endTime;
    }
    else if (pathHashs.length() > 1) {
        //TODO: The images' info in AlbumTable need dateTime
        //If: without those, need to loop access dateTime
        foreach (QString pHash,  pathHashs) {
            QDateTime tmpTime = getInfoByPathHash(pHash).time;
            if (tmpTime < info.beginTime || info.beginTime.isNull()) {
                info.beginTime = tmpTime;
            }

            if (tmpTime > info.endTime || info.endTime.isNull()) {
                info.endTime = tmpTime;
            }
        }
    }

    return info;
}

const QStringList DBManager::getAllAlbumNames() const
{
    QStringList list;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return list;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare( "SELECT DISTINCT AlbumName FROM AlbumTable3" );
    if ( !query.exec() ) {
        qWarning() << "Get AlbumNames failed: " << query.lastError();
    }
    else {
        while (query.next()) {
            list << query.value(0).toString();
        }
    }
    mutex.unlock();

    return list;
}

const QStringList DBManager::getPathsByAlbum(const QString &album) const
{
    QStringList list;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return list;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare("SELECT DISTINCT i.FilePath "
                  "FROM ImageTable3 AS i, AlbumTable3 AS a "
                  "WHERE i.PathHash=a.PathHash "
                  "AND a.AlbumName=:album "
                  "AND FilePath != \" \" ");
    query.bindValue(":album", album);
    if (! query.exec() ) {
        qWarning() << "Get Paths from AlbumTable3 failed: " << query.lastError();
    }
    else {
        while (query.next()) {
            list << query.value(0).toString();
        }
    }
    mutex.unlock();

    return list;
}

const DBImgInfoList DBManager::getInfosByAlbum(const QString &album) const
{
    DBImgInfoList infos;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare("SELECT DISTINCT i.FilePath, i.FileName, i.Dir, i.Time, i.ChangeTime "
                  "FROM ImageTable3 AS i, AlbumTable3 AS a "
                  "WHERE i.PathHash=a.PathHash AND a.AlbumName=:album");
    query.bindValue(":album", album);
    if (! query.exec()) {
        qWarning() << "Get ImgInfo by album failed: " << query.lastError();
        mutex.unlock();
    }
    else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = stringToDateTime(query.value(4).toString());

            infos << info;
        }
    }
    mutex.unlock();
    return infos;
}

int DBManager::getImgsCountByAlbum(const QString &album) const
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return 0;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    QString ps = "SELECT COUNT(*) FROM AlbumTable3 "
                 "WHERE AlbumName =:album AND PathHash != \"%1\" ";
    query.prepare( ps.arg(EMPTY_HASH_STR) );
    query.bindValue(":album", album);
    if (query.exec()) {
        query.first();
        mutex.unlock();
        return query.value(0).toInt();
    }
    else {
        qDebug() << "Get images count error :" << query.lastError();
        mutex.unlock();
        return 0;
    }
}

int DBManager::getAlbumsCount() const
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return 0;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare("SELECT COUNT(DISTINCT AlbumName) FROM AlbumTable3");
    if (query.exec()) {
        query.first();
        mutex.unlock();
        return query.value(0).toInt();
    }
    else {
        mutex.unlock();
        return 0;
    }
}

bool DBManager::isImgExistInAlbum(const QString &album, const QString &path) const
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return false;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare( "SELECT COUNT(*) FROM AlbumTable3 WHERE PathHash = :hash "
                   "AND AlbumName = :album");
    query.bindValue( ":hash", utils::base::hash(path) );
    query.bindValue( ":album", album );
    if (query.exec()) {
        query.first();
        mutex.unlock();
        return (query.value(0).toInt() == 1);
    }
    else {
        mutex.unlock();
        return false;
    }
}

bool DBManager::isAlbumExistInDB(const QString &album) const
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return false;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare( "SELECT COUNT(*) FROM AlbumTable3 WHERE AlbumName = :album");
    query.bindValue( ":album", album );
    if (query.exec()) {
        query.first();
        mutex.unlock();
        return (query.value(0).toInt() >= 1);
    }
    else {
        mutex.unlock();
        return false;
    }
}

void DBManager::insertIntoAlbum(const QString &album, const QStringList &paths)
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid() || album.isEmpty()) {
        return;
    }
    QStringList nameRows, pathHashRows;
    for (QString path : paths) {
        nameRows << album;
        pathHashRows << utils::base::hash(path);
    }

    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash) "
                  "VALUES (null, ?, ?)");
    query.addBindValue(nameRows);
    query.addBindValue(pathHashRows);
    if (! query.execBatch()) {
        qWarning() << "Insert data into AlbumTable3 failed: "
                   << query.lastError();
    }
    query.exec("COMMIT");

    //FIXME: Don't insert the repeated filepath into the same album
    //Delete the same data
    QString ps = "DELETE FROM AlbumTable3 where AlbumId NOT IN"
                 "(SELECT min(AlbumId) FROM AlbumTable3 GROUP BY"
                 " AlbumName, PathHash) AND PathHash != \"%1\" ";
    query.prepare(ps.arg(EMPTY_HASH_STR));
    if (!query.exec()) {
        qDebug() << "delete same date failed!";
    }
    query.exec("COMMIT");
    mutex.unlock();

    emit dApp->signalM->insertedIntoAlbum(album, paths);
}

void DBManager::insertIntoAlbumNoSignal(const QString &album, const QStringList &paths)
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid() || album.isEmpty()) {
        return;
    }
    QStringList nameRows, pathHashRows;
    for (QString path : paths) {
        nameRows << album;
        pathHashRows << utils::base::hash(path);
    }

    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash) "
                  "VALUES (null, ?, ?)");
    query.addBindValue(nameRows);
    query.addBindValue(pathHashRows);
    if (! query.execBatch()) {
        qWarning() << "Insert data into AlbumTable3 failed: "
                   << query.lastError();
    }
    query.exec("COMMIT");

    //FIXME: Don't insert the repeated filepath into the same album
    //Delete the same data
    QString ps = "DELETE FROM AlbumTable3 where AlbumId NOT IN"
                 "(SELECT min(AlbumId) FROM AlbumTable3 GROUP BY"
                 " AlbumName, PathHash) AND PathHash != \"%1\" ";
    query.prepare(ps.arg(EMPTY_HASH_STR));
    if (!query.exec()) {
        qDebug() << "delete same date failed!";
    }
    query.exec("COMMIT");
    mutex.unlock();
}


void DBManager::removeAlbum(const QString &album)
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare("DELETE FROM AlbumTable3 WHERE AlbumName=:album");
    query.bindValue(":album", album);
    if (!query.exec()) {
        qWarning() << "Remove album from database failed: " << query.lastError();
    }
    mutex.unlock();
}

void DBManager::removeFromAlbum(const QString &album, const QStringList &paths)
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }

    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hash(path);
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    // Remove from albums table
    QString qs("DELETE FROM AlbumTable3 WHERE AlbumName=\"%1\" AND PathHash=?");
    query.prepare(qs.arg(album));
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove images from DB failed: " << query.lastError();
    }
    else {
        mutex.unlock();
        emit dApp->signalM->removedFromAlbum(album, paths);
    }
    query.exec("COMMIT");
    mutex.unlock();
}

void DBManager::removeFromAlbumNoSignal(const QString &album, const QStringList &paths)
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }

    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hash(path);
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    // Remove from albums table
    QString qs("DELETE FROM AlbumTable3 WHERE AlbumName=\"%1\" AND PathHash=?");
    query.prepare(qs.arg(album));
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove images from DB failed: " << query.lastError();
    }
    else {
        mutex.unlock();
    }
    query.exec("COMMIT");
    mutex.unlock();
}

void DBManager::renameAlbum(const QString &oldAlbum, const QString &newAlbum)
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare("UPDATE AlbumTable3 SET "
                  "AlbumName = :newName "
                  "WHERE AlbumName = :oldName ");
    query.bindValue( ":newName", newAlbum );
    query.bindValue( ":oldName", oldAlbum );
    if (! query.exec()) {
        qWarning() << "Update album name failed: " << query.lastError();
    }
    mutex.unlock();
}

const DBImgInfoList DBManager::getInfosByNameTimeline(const QString &value) const
{
    DBImgInfoList infos;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);

    QString queryStr = "SELECT FilePath, FileName, Dir, Time, ChangeTime FROM ImageTable3 "
                       "WHERE FileName like \'\%%1\%\' OR Time like \'\%%1\%\' ORDER BY Time DESC";

    query.prepare(queryStr.arg(value));

    if (!query.exec()) {
        qWarning() << "Get Image from database failed: " << query.lastError();
        mutex.unlock();
    }
    else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = stringToDateTime(query.value(4).toString());

            infos << info;
        }
    }
    mutex.unlock();
    return infos;
}

const DBImgInfoList DBManager::getInfosForKeyword(const QString &keywords) const
{
    const DBImgInfoList list = getInfosByNameTimeline(keywords);
    if (list.count() < 1) {
        return DBImgInfoList();
    }
    else {
        return list;
    }
}

const DBImgInfoList DBManager::getTrashInfosForKeyword(const QString &keywords) const
{
    DBImgInfoList infos;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);

    QString queryStr = "SELECT FilePath, FileName, Dir, Time, ChangeTime FROM TrashTable "
                       "WHERE FileName like \'\%%1\%\' OR Time like \'\%%1\%\' ORDER BY Time DESC";

    query.prepare(queryStr.arg(keywords));

    if (!query.exec()) {
        qWarning() << "Get Image from database failed: " << query.lastError();
        mutex.unlock();
    }
    else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = stringToDateTime(query.value(4).toString());

            infos << info;
        }
    }
    mutex.unlock();
    return infos;
}

const DBImgInfoList DBManager::getInfosForKeyword(const QString &album, const QString &keywords) const
{


//    QString queryStr = "SELECT FilePath, FileName, Dir, Time FROM ImageTable3 "
//                       "WHERE FileName like \'\%%1\%\' OR Time like \'\%%1\%\' ORDER BY Time DESC";

//    query.prepare(queryStr.arg(keywords));

    DBImgInfoList infos;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QMutexLocker mutex(&m_mutex);

    QString queryStr = "SELECT DISTINCT i.FilePath, i.FileName, i.Dir, i.Time, i.ChangeTime "
                       "FROM ImageTable3 AS i "
                       "inner join AlbumTable3 AS a on i.PathHash=a.PathHash AND a.AlbumName=:album "
                       "WHERE i.FileName like \'\%%1\%\' OR Time like \'\%%1\%\' ORDER BY Time DESC";

    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare(queryStr.arg(keywords));
    query.bindValue(":album", album);


    if (! query.exec()) {
        qWarning() << "Get ImgInfo by album failed: " << query.lastError();
        mutex.unlock();
    }
    else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = stringToDateTime(query.value(4).toString());

            infos << info;
        }
    }
    mutex.unlock();
    return infos;
}

const DBImgInfoList DBManager::getImgInfos(const QString &key, const QString &value) const
{
    DBImgInfoList infos;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime FROM ImageTable3 "
                          "WHERE %1= :value ORDER BY Time DESC").arg(key));

    query.bindValue(":value", value);

    if (!query.exec()) {
        qWarning() << "Get Image from database failed: " << query.lastError();
        mutex.unlock();
    }
    else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = stringToDateTime(query.value(4).toString());

            infos << info;
        }
    }
    mutex.unlock();
    return infos;
}

const QSqlDatabase DBManager::getDatabase() const
{
    QMutexLocker mutex(&m_mutex);
    if( QSqlDatabase::contains(m_connectionName) ) {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
        mutex.unlock();
        return db;
    }
    else {
        //if database not open, open it.
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);//not dbConnection
        qDebug()<<DATABASE_PATH + DATABASE_NAME;
        db.setDatabaseName(DATABASE_PATH + DATABASE_NAME);
        if (! db.open()) {
            qWarning()<< "Open database error:" << db.lastError();
            mutex.unlock();
            return QSqlDatabase();
        }
        else {
            mutex.unlock();
            return db;
        }
    }
}

void DBManager::checkDatabase()
{
    QDir dd(DATABASE_PATH);
    if (! dd.exists()) {
        qDebug() << "create database paths";
        dd.mkpath(DATABASE_PATH);
        if (dd.exists())
            qDebug() << "create database succeed!";
        else
            qDebug() << "create database failed!";
    } else {
        qDebug() << "database is exist!";
    }
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    bool tableExist = false;
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare( "SELECT name FROM sqlite_master "
                   "WHERE type=\"table\" AND name = \"ImageTable3\"");
    if (query.exec() && query.first()) {
        tableExist = ! query.value(0).toString().isEmpty();
    }
    //if tables not exist, create it.
    if ( ! tableExist ) {
        QSqlQuery query(db);
        // ImageTable3
        //////////////////////////////////////////////////////////////
        //PathHash           | FilePath | FileName   | Dir  | Time | ChangeTime //
        //TEXT primari key   | TEXT     | TEXT       | TEXT | TEXT | TEXT //
        //////////////////////////////////////////////////////////////
        query.exec( QString("CREATE TABLE IF NOT EXISTS ImageTable3 ( "
                            "PathHash TEXT primary key, "
                            "FilePath TEXT, "
                            "FileName TEXT, "
                            "Dir TEXT, "
                            "Time TEXT, "
                            "ChangeTime TEXT)"));

        // AlbumTable3
        //////////////////////////////////////////////////////////
        //AlbumId               | AlbumName         | PathHash  //
        //INTEGER primari key   | TEXT              | TEXT      //
        //////////////////////////////////////////////////////////
        query.exec( QString("CREATE TABLE IF NOT EXISTS AlbumTable3 ( "
                            "AlbumId INTEGER primary key, "
                            "AlbumName TEXT, "
                            "PathHash TEXT)") );

        // TrashTable
        //////////////////////////////////////////////////////////////
        //PathHash           | FilePath | FileName   | Dir  | Time | ChangeTime  //
        //TEXT primari key   | TEXT     | TEXT       | TEXT | TEXT | TEXT //
        //////////////////////////////////////////////////////////////
        query.exec( QString("CREATE TABLE IF NOT EXISTS TrashTable ( "
                            "PathHash TEXT primary key, "
                            "FilePath TEXT, "
                            "FileName TEXT, "
                            "Dir TEXT, "
                            "Time TEXT, "
                            "ChangeTime TEXT )"));
//        // Check if there is an old version table exist or not

//        //TODO: AlbumTable's primary key is changed, need to importVersion again
//        importVersion1Data();
//        importVersion2Data();
    }
    else {
       // 判断ImageTable3中是否有ChangeTime字段
        QString strSqlImage = QString::fromLocal8Bit("select sql from sqlite_master where name = \"ImageTable3\" and sql like \"%ChangeTime%\"");
        QSqlQuery queryImage(db);
        queryImage.exec(strSqlImage);
        if (!queryImage.next()){
            // 无ChangeTime字段,则增加ChangeTime字段,赋值当前时间
            QString strDate = QDateTime::currentDateTime().toString(DATETIME_FORMAT_DATABASE);
            queryImage.exec( QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"ChangeTime\" TEXT default \"%1\"")
                        .arg(strDate));
        }

        // 判断TrashTable中是否有ChangeTime字段
         QString strSqlTrash = QString::fromLocal8Bit("select * from sqlite_master where name = \"TrashTable\" and sql like \"%ChangeTime%\"");
         QSqlQuery queryTrash(db);
         queryTrash.exec(strSqlTrash);
         if (!queryTrash.next()){
             // 无ChangeTime字段,则增加ChangeTime字段,赋值当前时间
             QString strDate = QDateTime::currentDateTime().toString(DATETIME_FORMAT_DATABASE);
             queryTrash.exec( QString("ALTER TABLE \"TrashTable\" ADD COLUMN \"ChangeTime\" TEXT default \"%1\"")
                         .arg(strDate));
         }
    }
    mutex.unlock();

}

void DBManager::importVersion1Data()
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    bool tableExist = false;
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare( "SELECT name FROM sqlite_master "
                   "WHERE type=\"table\" AND name = \"ImageTable\"");
    if (query.exec() && query.first()) {
        tableExist = ! query.value(0).toString().isEmpty();
    }
    mutex.unlock();
    if (tableExist) {
        // Import ImageTable into ImageTable3
        query.clear();
        query.setForwardOnly(true);
        QMutexLocker mutex(&m_mutex);
        query.prepare( "SELECT filename, filepath, time, changeTime "
                       "FROM ImageTable ORDER BY time DESC");
        if (! query.exec()) {
            qWarning() << "Import ImageTable into ImageTable3 failed: "
                       << query.lastError();
            mutex.unlock();
        }
        else {
            DBImgInfoList infos;
            using namespace utils::base;
            while (query.next()) {
                DBImgInfo info;
                info.fileName = query.value(0).toString();
                info.filePath = query.value(1).toString();
                info.time = stringToDateTime(query.value(2).toString());
                info.changeTime = stringToDateTime(query.value(3).toString());

                infos << info;
            }
            mutex.unlock();
            insertImgInfos(infos);
        }

        mutex.relock();
        // Import AlbumTable into AlbumTable3
        query.clear();
        query.prepare("SELECT DISTINCT a.albumname, i.filepath "
                      "FROM ImageTable AS i, AlbumTable AS a "
                      "WHERE i.filename=a.filename ");
        if (! query.exec()) {
            qWarning() << "Import AlbumTable into AlbumTable3 failed: "
                       << query.lastError();
            mutex.unlock();
        }
        else {
            // <Album-Paths>
            QMap<QString, QStringList> aps;
            using namespace utils::base;
            while (query.next()) {
                QString album = query.value(0).toString();
                QString path = query.value(1).toString();
                if (aps.keys().contains(album)) {
                    aps[album].append(path);
                }
                else {
                    aps.insert(album, QStringList(path));
                }
            }
            mutex.unlock();
            for (QString album : aps.keys()) {
                insertIntoAlbum(album, aps[album]);
            }
        }

        mutex.relock();
        // Drop old table
        query.clear();
        query.prepare("DROP TABLE AlbumTable");
        if (! query.exec()) {
            qWarning() << "Drop old tables failed: " << query.lastError();
        }
        query.prepare("DROP TABLE ImageTable");
        if (! query.exec()) {
            qWarning() << "Drop old tables failed: " << query.lastError();
        }
        mutex.unlock();
    }
}

void DBManager::importVersion2Data()
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    bool tableExist = false;
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare( "SELECT name FROM sqlite_master "
                   "WHERE type=\"table\" AND name = \"ImageTable2\"");
    if (query.exec() && query.first()) {
        tableExist = ! query.value(0).toString().isEmpty();
    }

    if (tableExist) {
        // Import ImageTable2 into ImageTable3
        query.clear();
        query.prepare( "SELECT FileName, FilePath, Time, ChangeTime "
                       "FROM ImageTable2 ORDER BY Time DESC");
        if (! query.exec()) {
            qWarning() << "Import ImageTable2 into ImageTable3 failed: "
                       << query.lastError();
            mutex.unlock();
        }
        else {
            DBImgInfoList infos;
            using namespace utils::base;
            while (query.next()) {
                DBImgInfo info;
                info.fileName = query.value(0).toString();
                info.filePath = query.value(1).toString();
                info.time = stringToDateTime(query.value(2).toString());
                info.changeTime = stringToDateTime(query.value(3).toString());

                infos << info;
            }
            mutex.unlock();
            insertImgInfos(infos);
        }

        // Import AlbumTable2 into AlbumTable3
        query.clear();
        QMutexLocker mutex(&m_mutex);
        query.prepare(" SELECT AlbumName, FilePath FROM AlbumTable2 ");
        if (! query.exec()) {
            qWarning() << "Import AlbumTable2 into AlbumTable3 failed: "
                       << query.lastError();
            mutex.unlock();
        }
        else {
            // <Album-Paths>
            QMap<QString, QStringList> aps;
            using namespace utils::base;
            while (query.next()) {
                QString album = query.value(0).toString();
                QString path = query.value(1).toString();
                if (aps.keys().contains(album)) {
                    aps[album].append(path);
                }
                else {
                    aps.insert(album, QStringList(path));
                }
            }
            mutex.unlock();
            for (QString album : aps.keys()) {
                insertIntoAlbum(album, aps[album]);
            }
        }

        mutex.relock();
        // Drop old table
        query.clear();
        query.prepare("DROP TABLE AlbumTable2");
        if (! query.exec()) {
            qWarning() << "Drop old tables failed: " << query.lastError();
        }
        query.prepare("DROP TABLE ImageTable2");
        if (! query.exec()) {
            qWarning() << "Drop old tables failed: " << query.lastError();
        }
        mutex.unlock();
    }
}

const QStringList DBManager::getAllTrashPaths() const
{
    QStringList paths;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid())
        return paths;

    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare( "SELECT "
                   "FilePath "
                   "FROM TrashTable ORDER BY Time DESC");
    if (! query.exec()) {
        qWarning() << "Get Data from TrashTable failed: " << query.lastError();
        mutex.unlock();
        return paths;
    }
    else {
        while (query.next()) {
            paths << query.value(0).toString();
        }
    }
    mutex.unlock();

    return paths;
}

const DBImgInfoList DBManager::getAllTrashInfos() const
{
    DBImgInfoList infos;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare( "SELECT FilePath, FileName, Dir, Time, ChangeTime "
                   "FROM TrashTable");
    if (! query.exec()) {
        qWarning() << "Get data from TrashTable failed: " << query.lastError();
        mutex.unlock();
        return infos;
    }
    else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = stringToDateTime(query.value(4).toString());

            infos << info;
        }
    }
    mutex.unlock();

    return infos;
}

void DBManager::insertTrashImgInfos(const DBImgInfoList &infos)
{
    const QSqlDatabase db = getDatabase();
    if (infos.isEmpty() || ! db.isValid()) {
        return;
    }

    QVariantList pathhashs, filenames, filepaths, dirs, times, changetimes;

    for (DBImgInfo info : infos) {
        filenames << info.fileName;
        filepaths << info.filePath;
        pathhashs << utils::base::hash(info.filePath);
        dirs << info.dirHash;
        times << utils::base::timeToString(info.time, true);
        changetimes << info.changeTime.toString(DATETIME_FORMAT_DATABASE);
    }

    QMutexLocker mutex(&m_mutex);
    // Insert into TrashTable
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare( "REPLACE INTO TrashTable "
                   "(PathHash, FilePath, FileName, Dir, Time, ChangeTime) VALUES (?, ?, ?, ?, ?, ?)" );
    query.addBindValue(pathhashs);
    query.addBindValue(filepaths);
    query.addBindValue(filenames);
    query.addBindValue(dirs);
    query.addBindValue(times);
    query.addBindValue(changetimes);
    if (! query.execBatch()) {
        qWarning() << "Insert data into TrashTable failed: "
                   << query.lastError();
        query.exec("COMMIT");
        mutex.unlock();
    }
    else {
        query.exec("COMMIT");
        mutex.unlock();
        emit dApp->signalM->imagesTrashInserted(infos);
    }
}

void DBManager::removeTrashImgInfos(const QStringList &paths)
{
    QSqlDatabase db = getDatabase();
    if (paths.isEmpty() || ! db.isValid()) {
        return;
    }

    // Collect info before removing data
    DBImgInfoList infos;
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hash(path);
        infos << getInfoByPath(path);
    }

    QMutexLocker mutex(&m_mutex);
    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);
//    query.exec("BEGIN IMMEDIATE TRANSACTION");
//    QString qs = "DELETE FROM AlbumTable3 WHERE PathHash=?";
//    query.prepare(qs);
//    query.addBindValue(pathHashs);
//    if (! query.execBatch()) {
//        qWarning() << "Remove data from AlbumTable3 failed: "
//                   << query.lastError();
//        query.exec("COMMIT");
//    }
//    else {
//        query.exec("COMMIT");
//    }

    // Remove from image table
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    QString qs = "DELETE FROM TrashTable WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove data from TrashTable failed: "
                   << query.lastError();
        query.exec("COMMIT");
        mutex.unlock();
    }
    else {
        mutex.unlock();
        emit dApp->signalM->imagesTrashRemoved(infos);
        query.exec("COMMIT");
    }
}

void DBManager::removeTrashImgInfosNoSignal(const QStringList &paths)
{
    QSqlDatabase db = getDatabase();
    if (paths.isEmpty() || ! db.isValid()) {
        return;
    }

    // Collect info before removing data
    DBImgInfoList infos;
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hash(path);
        infos << getInfoByPath(path);
    }

    QMutexLocker mutex(&m_mutex);
    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    QString qs = "DELETE FROM AlbumTable3 WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove data from AlbumTable3 failed: "
                   << query.lastError();
        query.exec("COMMIT");
    }
    else {
        query.exec("COMMIT");
    }

    // Remove from image table
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    qs = "DELETE FROM TrashTable WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove data from TrashTable failed: "
                   << query.lastError();
        query.exec("COMMIT");
        mutex.unlock();
    }
    else {
        mutex.unlock();
        query.exec("COMMIT");
    }
}

const DBImgInfo DBManager::getTrashInfoByPath(const QString &path) const
{
    DBImgInfoList list = getTrashImgInfos("FilePath", path);
    if (list.count() != 1) {
        return DBImgInfo();
    }
    else {
        return list.first();
    }
}

const DBImgInfoList DBManager::getTrashImgInfos(const QString &key, const QString &value) const
{
    DBImgInfoList infos;
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime FROM TrashTable "
                          "WHERE %1= :value ORDER BY Time DESC").arg(key));

    query.bindValue(":value", value);

    if (!query.exec()) {
        qWarning() << "Get Image from database failed: " << query.lastError();
        mutex.unlock();
    }
    else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
            info.changeTime = stringToDateTime(query.value(4).toString());

            infos << info;
        }
    }
    mutex.unlock();
    return infos;
}

int DBManager::getTrashImgsCount() const
{
    const QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return 0;
    }
    QMutexLocker mutex(&m_mutex);
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare( "SELECT COUNT(*) FROM TrashTable" );
    if (query.exec()) {
        query.first();
        int count = query.value(0).toInt();
        query.exec("COMMIT");
        mutex.unlock();
        return count;
    }
    mutex.unlock();
    return 0;
}
