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

DBManager  *DBManager::m_dbManager = NULL;

DBManager  *DBManager::instance()
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
    QMutexLocker mutex(&m_mutex);
    QStringList paths;
    QSqlDatabase db = getDatabase();
    if (! db.isValid())
        return paths;

    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT "
                  "FilePath "
                  "FROM ImageTable3"/* ORDER BY Time DESC"*/);
    if (! query.exec()) {
        //qWarning() << "Get Data from ImageTable3 failed: " << query.lastError();
//        // 连接使用完后需要释放回数据库连接池
//        ////ConnectionPool::closeConnection(db);
        db.close();
        return paths;
    } else {
        while (query.next()) {
            paths << query.value(0).toString();
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    ////ConnectionPool::closeConnection(db);
    db.close();

    return paths;
}

const DBImgInfoList DBManager::getAllInfos() const
{
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT FilePath, FileName, Dir, Time, ChangeTime "
                  "FROM ImageTable3");
    if (! query.exec()) {
        //  qWarning() << "Get data from ImageTable3 failed: " << query.lastError();
//        // 连接使用完后需要释放回数据库连接池
        ////ConnectionPool::closeConnection(db);
        db.close();
        return infos;
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
//            info.changeTime = stringToDateTime(query.value(4).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);

            infos << info;
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    ////ConnectionPool::closeConnection(db);
    db.close();

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
    query.prepare("SELECT DISTINCT Time "
                  "FROM ImageTable3 ORDER BY Time DESC");
    if (! query.exec()) {
        //   qWarning() << "Get Data from ImageTable3 failed: " << query.lastError();
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return times;
    } else {
        while (query.next()) {
            times << query.value(0).toString();
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
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
    QStringList times;
    QSqlDatabase db = getDatabase();
    if (! db.isValid())
        return times;

    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT DISTINCT ChangeTime "
                  "FROM ImageTable3 ORDER BY ChangeTime DESC");
    if (! query.exec()) {
        //    qWarning() << "Get Data from ImageTable3 failed: " << query.lastError();
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return times;
    } else {
        while (query.next()) {
            times << query.value(0).toString();
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();
    return times;
}

const DBImgInfoList DBManager::getInfosByImportTimeline(const QString &timeline) const
{
    const DBImgInfoList list = getImgInfos("ChangeTime", timeline);
    if (list.count() < 1) {
        return DBImgInfoList();
    } else {
        return list;
    }
}

const DBImgInfo DBManager::getInfoByName(const QString &name) const
{
    DBImgInfoList list = getImgInfos("FileName", name);
    if (list.count() < 1) {
        return DBImgInfo();
    } else {
        return list.first();
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

const DBImgInfo DBManager::getInfoByPathHash(const QString &pathHash) const
{
    DBImgInfoList list = getImgInfos("PathHash", pathHash);
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
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare("SELECT COUNT(*) FROM ImageTable3");
    if (query.exec()) {
        query.first();
        int count = query.value(0).toInt();
        query.exec("COMMIT");
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return count;
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();
    return 0;
}

int DBManager::getImgsCountByDir(const QString &dir) const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (dir.isEmpty() || ! db.isValid()) {
        return 0;
    }

    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT COUNT(*) FROM ImageTable3 "
                  "WHERE Dir=:Dir AND FilePath !=\" \"");
    query.bindValue(":Dir", utils::base::hash(dir));
    if (query.exec()) {
        query.first();
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return query.value(0).toInt();
    } else {
        //  qDebug() << "Get images count by Dir failed :" << query.lastError();
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return 0;
    }
}

const QStringList DBManager::getPathsByDir(const QString &dir) const
{
    QMutexLocker mutex(&m_mutex);
    QStringList list;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return list;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT DISTINCT FilePath FROM ImageTable3 "
                  "WHERE Dir=:dir ");
    query.bindValue(":dir", utils::base::hash(dir));
    if (! query.exec()) {
        //  qWarning() << "Get Paths from ImageTable3 failed: " << query.lastError();
    } else {
        while (query.next()) {
            list << query.value(0).toString();
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();

    return list;
}

bool DBManager::isImgExist(const QString &path) const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (db.isValid()) {
        return false;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare("SELECT COUNT(*) FROM ImageTable3 WHERE FilePath = :path");
    query.bindValue(":path", path);
    if (query.exec()) {
        query.first();
        if (query.value(0).toInt() > 0) {
            query.exec("COMMIT");
            db.close();
            return true;
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();

    return false;
}

void DBManager::insertImgInfos(const DBImgInfoList &infos)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
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

    // Insert into ImageTable3
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare("REPLACE INTO ImageTable3 "
                  "(PathHash, FilePath, FileName, Dir, Time, ChangeTime) VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(pathhashs);
    query.addBindValue(filepaths);
    query.addBindValue(filenames);
    query.addBindValue(dirs);
    query.addBindValue(times);
    query.addBindValue(changetimes);
    if (! query.execBatch()) {
        // qWarning() << "Insert data into ImageTable3 failed: "
        //            << query.lastError();
        query.exec("COMMIT");
        db.close();
    } else {
        query.exec("COMMIT");
        db.close();
        mutex.unlock();
        emit dApp->signalM->imagesInserted(/*infos*/);
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
}

void DBManager::removeImgInfos(const QStringList &paths)
{
    if (paths.isEmpty()) {
        return;
    }
    // Collect info before removing data
    DBImgInfoList infos;
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hash(path);
        infos = getImgInfos("FilePath", path, false);
//        infos << getInfoByPath(path);
    }
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }


    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    QString qs = "DELETE FROM AlbumTable3 WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        //   qWarning() << "Remove data from AlbumTable3 failed: "
        //               << query.lastError();
        query.exec("COMMIT");
    } else {
        query.exec("COMMIT");
    }

    // Remove from image table
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    qs = "DELETE FROM ImageTable3 WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        //  qWarning() << "Remove data from ImageTable3 failed: "
        //              << query.lastError();
        query.exec("COMMIT");
        db.close();
    } else {
        query.exec("COMMIT");
        db.close();
        mutex.unlock();
        emit dApp->signalM->imagesRemoved();
        emit dApp->signalM->imagesRemovedPar(infos);
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
}

void DBManager::removeImgInfosNoSignal(const QStringList &paths)
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
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    QString qs = "DELETE FROM AlbumTable3 WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        //  qWarning() << "Remove data from AlbumTable3 failed: "
        //              << query.lastError();
        query.exec("COMMIT");
    } else {
        query.exec("COMMIT");
    }

    // Remove from image table
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    qs = "DELETE FROM ImageTable3 WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        //  qWarning() << "Remove data from ImageTable3 failed: "
        //              << query.lastError();
        query.exec("COMMIT");
    } else {
        query.exec("COMMIT");
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();
}

void DBManager::removeDir(const QString &dir)
{
    const QString dirHash = utils::base::hash(dir);
    // Collect info before removing data
    DBImgInfoList infos = getImgInfos("Dir", dirHash, false);
    QStringList pathHashs;
    for (auto info : infos) {
        pathHashs << utils::base::hash(info.filePath);
    }
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (dir.isEmpty() || ! db.isValid()) {
        return;
    }


    QSqlQuery query(db);
    // Remove from albums table
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    QString qs = "DELETE FROM AlbumTable3 WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        //   qWarning() << "Remove data from AlbumTable3 failed: "
        //              << query.lastError();
    }
    query.exec("COMMIT");

    // Remove from image table
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    qs = "DELETE FROM ImageTable3 WHERE Dir=:Dir";
    query.prepare(qs);
    query.bindValue(":Dir", dirHash);
    if (! query.exec()) {
        //    qWarning() << "Remove dir's images from ImageTable3 failed: "
        //               << query.lastError();
        query.exec("COMMIT");
        db.close();
    } else {
        query.exec("COMMIT");
        db.close();
        mutex.unlock();
        emit dApp->signalM->imagesRemoved();
        emit dApp->signalM->imagesRemovedPar(infos);
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
}

const DBAlbumInfo DBManager::getAlbumInfo(const QString &album) const
{
    QMutexLocker mutex(&m_mutex);
    DBAlbumInfo info;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return info;
    }

    info.name = album;
    QStringList pathHashs;

    QSqlQuery query(db);
    query.setForwardOnly(true);
    QString ps = "SELECT DISTINCT PathHash FROM AlbumTable3 "
                 "WHERE AlbumName =:name AND PathHash != \"%1\" ";
    query.prepare(ps.arg(EMPTY_HASH_STR));
    query.bindValue(":name", album);
    if (! query.exec()) {
        //    qWarning() << "Get data from AlbumTable3 failed: "
        //               << query.lastError();
    } else {
        while (query.next()) {
            pathHashs << query.value(0).toString();
        }
    }
    db.close();
    mutex.unlock();
    info.count = pathHashs.length();
    if (pathHashs.length() == 1) {
        info.endTime = getInfoByPathHash(pathHashs.first()).time;
        info.beginTime = info.endTime;
    } else if (pathHashs.length() > 1) {
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
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    return info;
}

const QStringList DBManager::getAllAlbumNames() const
{
    QMutexLocker mutex(&m_mutex);
    QStringList list;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return list;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT DISTINCT AlbumName FROM AlbumTable3");
    if (!query.exec()) {
        //    qWarning() << "Get AlbumNames failed: " << query.lastError();
    } else {
        while (query.next()) {
            list << query.value(0).toString();
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();

    return list;
}

const QStringList DBManager::getPathsByAlbum(const QString &album) const
{
    QMutexLocker mutex(&m_mutex);
    QStringList list;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return list;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT DISTINCT i.FilePath "
                  "FROM ImageTable3 AS i, AlbumTable3 AS a "
                  "WHERE i.PathHash=a.PathHash "
                  "AND a.AlbumName=:album "
                  "AND FilePath != \" \" ");
    query.bindValue(":album", album);
    if (! query.exec()) {
        //    qWarning() << "Get Paths from AlbumTable3 failed: " << query.lastError();
    } else {
        while (query.next()) {
            list << query.value(0).toString();
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();

    return list;
}

const DBImgInfoList DBManager::getInfosByAlbum(const QString &album) const
{
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT DISTINCT i.FilePath, i.FileName, i.Dir, i.Time, i.ChangeTime "
                  "FROM ImageTable3 AS i, AlbumTable3 AS a "
                  "WHERE i.PathHash=a.PathHash AND a.AlbumName=:album");
    query.bindValue(":album", album);
    if (! query.exec()) {
        //    qWarning() << "Get ImgInfo by album failed: " << query.lastError();
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
//            info.changeTime = stringToDateTime(query.value(4).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);

            infos << info;
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();
    return infos;
}

int DBManager::getImgsCountByAlbum(const QString &album) const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return 0;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    QString ps = "SELECT COUNT(*) FROM AlbumTable3 "
                 "WHERE AlbumName =:album AND PathHash != \"%1\" ";
    query.prepare(ps.arg(EMPTY_HASH_STR));
    query.bindValue(":album", album);
    if (query.exec()) {
        query.first();
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return query.value(0).toInt();
    } else {
        //   qDebug() << "Get images count error :" << query.lastError();
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return 0;
    }
}

int DBManager::getAlbumsCount() const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return 0;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT COUNT(DISTINCT AlbumName) FROM AlbumTable3");
    if (query.exec()) {
        query.first();
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return query.value(0).toInt();
    } else {
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return 0;
    }
}

bool DBManager::isImgExistInAlbum(const QString &album, const QString &path) const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return false;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE PathHash = :hash "
                  "AND AlbumName = :album");
    query.bindValue(":hash", utils::base::hash(path));
    query.bindValue(":album", album);
    if (query.exec()) {
        query.first();
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return (query.value(0).toInt() == 1);
    } else {
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return false;
    }
}

bool DBManager::isAlbumExistInDB(const QString &album) const
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return false;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE AlbumName = :album");
    query.bindValue(":album", album);
    if (query.exec()) {
        query.first();
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return (query.value(0).toInt() >= 1);
    } else {
//        // 连接使用完后需要释放回数据库连接池
        //ConnectionPool::closeConnection(db);
        db.close();
        return false;
    }
}

void DBManager::insertIntoAlbum(const QString &album, const QStringList &paths)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid() || album.isEmpty()) {
        return;
    }
    QStringList nameRows, pathHashRows;
    for (QString path : paths) {
        nameRows << album;
        pathHashRows << utils::base::hash(path);
    }

    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash) "
                  "VALUES (null, ?, ?)");
    query.addBindValue(nameRows);
    query.addBindValue(pathHashRows);
    if (! query.execBatch()) {
        //   qWarning() << "Insert data into AlbumTable3 failed: "
        //              << query.lastError();
    }
    query.exec("COMMIT");

    //FIXME: Don't insert the repeated filepath into the same album
    //Delete the same data
    QString ps = "DELETE FROM AlbumTable3 where AlbumId NOT IN"
                 "(SELECT min(AlbumId) FROM AlbumTable3 GROUP BY"
                 " AlbumName, PathHash) AND PathHash != \"%1\" ";
    query.prepare(ps.arg(EMPTY_HASH_STR));
    if (!query.exec()) {
        //   qDebug() << "delete same date failed!";
    }
    query.exec("COMMIT");
    db.close();
    mutex.unlock();
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);

    emit dApp->signalM->insertedIntoAlbum(album, paths);
}

void DBManager::insertIntoAlbumNoSignal(const QString &album, const QStringList &paths)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid() || album.isEmpty()) {
        return;
    }
    QStringList nameRows, pathHashRows;
    for (QString path : paths) {
        nameRows << album;
        pathHashRows << utils::base::hash(path);
    }

    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash) "
                  "VALUES (null, ?, ?)");
    query.addBindValue(nameRows);
    query.addBindValue(pathHashRows);
    if (! query.execBatch()) {
        //   qWarning() << "Insert data into AlbumTable3 failed: "
        //             << query.lastError();
    }
    query.exec("COMMIT");

    //FIXME: Don't insert the repeated filepath into the same album
    //Delete the same data
    QString ps = "DELETE FROM AlbumTable3 where AlbumId NOT IN"
                 "(SELECT min(AlbumId) FROM AlbumTable3 GROUP BY"
                 " AlbumName, PathHash) AND PathHash != \"%1\" ";
    query.prepare(ps.arg(EMPTY_HASH_STR));
    if (!query.exec()) {
        //    qDebug() << "delete same date failed!";
    }
    query.exec("COMMIT");
    db.close();
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
}


void DBManager::removeAlbum(const QString &album)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("DELETE FROM AlbumTable3 WHERE AlbumName=:album");
    query.bindValue(":album", album);
    if (!query.exec()) {
        //    qWarning() << "Remove album from database failed: " << query.lastError();
    }
    db.close();
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
}

void DBManager::removeFromAlbum(const QString &album, const QStringList &paths)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }

    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hash(path);
//        qDebug() << "";
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    // Remove from albums table
    QString qs("DELETE FROM AlbumTable3 WHERE AlbumName=\"%1\" AND PathHash=?");
    query.prepare(qs.arg(album));
    query.addBindValue(pathHashs);
    bool suc = false;
    if (! query.execBatch()) {
        //    qWarning() << "Remove images from DB failed: " << query.lastError();
    } else {
        suc = true;
    }
    query.exec("COMMIT");
    db.close();
    mutex.unlock();
    if (suc)
        emit dApp->signalM->removedFromAlbum(album, paths);
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
}

void DBManager::removeFromAlbumNoSignal(const QString &album, const QStringList &paths)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }

    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hash(path);
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    // Remove from albums table
    QString qs("DELETE FROM AlbumTable3 WHERE AlbumName=\"%1\" AND PathHash=?");
    query.prepare(qs.arg(album));
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        //  qWarning() << "Remove images from DB failed: " << query.lastError();
    } else {
//        mutex.unlock();
    }
    query.exec("COMMIT");
    db.close();
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
}

void DBManager::renameAlbum(const QString &oldAlbum, const QString &newAlbum)
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("UPDATE AlbumTable3 SET "
                  "AlbumName = :newName "
                  "WHERE AlbumName = :oldName ");
    query.bindValue(":newName", newAlbum);
    query.bindValue(":oldName", oldAlbum);
    if (! query.exec()) {
        //   qWarning() << "Update album name failed: " << query.lastError();
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
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

    QString queryStr = "SELECT FilePath, FileName, Dir, Time, ChangeTime FROM ImageTable3 "
                       "WHERE FileName like \'\%%1\%\' OR Time like \'\%%1\%\' ORDER BY Time DESC";

    query.prepare(queryStr.arg(value));

    if (!query.exec()) {
        //   qWarning() << "Get Image from database failed: " << query.lastError();
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
//            info.changeTime = stringToDateTime(query.value(4).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);

            infos << info;
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
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

    QString queryStr = "SELECT FilePath, FileName, Dir, Time, ChangeTime, AlbumName FROM TrashTable "
                       "WHERE FileName like \'\%%1\%\' OR Time like \'\%%1\%\' ORDER BY Time DESC";

    query.prepare(queryStr.arg(keywords));

    if (!query.exec()) {
        //  qWarning() << "Get Image from database failed: " << query.lastError();
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
//            info.changeTime = stringToDateTime(query.value(4).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);

            infos << info;
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();
    return infos;
}

const DBImgInfoList DBManager::getInfosForKeyword(const QString &album, const QString &keywords) const
{


//    QString queryStr = "SELECT FilePath, FileName, Dir, Time FROM ImageTable3 "
//                       "WHERE FileName like \'\%%1\%\' OR Time like \'\%%1\%\' ORDER BY Time DESC";

//    query.prepare(queryStr.arg(keywords));
    QMutexLocker mutex(&m_mutex);

    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }

    QString queryStr = "SELECT DISTINCT i.FilePath, i.FileName, i.Dir, i.Time, i.ChangeTime "
                       "FROM ImageTable3 AS i "
                       "inner join AlbumTable3 AS a on i.PathHash=a.PathHash AND a.AlbumName=:album "
                       "WHERE i.FileName like \'\%%1\%\' OR Time like \'\%%1\%\' ORDER BY Time DESC";

    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare(queryStr.arg(keywords));
    query.bindValue(":album", album);


    if (! query.exec()) {
        //   qWarning() << "Get ImgInfo by album failed: " << query.lastError();
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
//            info.changeTime = stringToDateTime(query.value(4).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);

            infos << info;
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();
    return infos;
}

const DBImgInfoList DBManager::getImgInfos(const QString &key, const QString &value, const bool &needlock) const
{
//    if (needlock)
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return infos;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime FROM ImageTable3 "
                          "WHERE %1= :value ORDER BY Time DESC").arg(key));

    query.bindValue(":value", value);

    if (!query.exec()) {
        //   qWarning() << "Get Image from database failed: " << query.lastError();
    } else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.time = stringToDateTime(query.value(3).toString());
//            info.changeTime = stringToDateTime(query.value(4).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);

            infos << info;
        }
    }
    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();
    return infos;
}

const QSqlDatabase DBManager::getDatabase() const
{
//    QMutexLocker mutex(&m_mutex);
//    QSqlDatabase db = ConnectionPool::openConnection();
//    return db;
//    if ( QSqlDatabase::contains(m_connectionName) ) {
//        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
////        mutex.unlock();
//        return db;
//    } else {
//        //if database not open, open it.
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");//not dbConnection
    db.setDatabaseName(DATABASE_PATH + DATABASE_NAME);
    if (! db.open()) {
        //  qWarning() << "Open database error:" << db.lastError();
//            mutex.unlock();
        return QSqlDatabase();
    } else {
//            mutex.unlock();
        return db;
    }
//}
}

//const QSqlDatabase DBManager::getDatabase1() const
//{
////    QMutexLocker mutex(&m_mutex);
////    QSqlDatabase db = ConnectionPool::openConnection();
////    return db;
////    if ( QSqlDatabase::contains(m_connectionName) ) {
////        QSqlDatabase db = QSqlDatabase::database(m_connectionName);
//////        mutex.unlock();
////        return db;
////    } else {
////        //if database not open, open it.
//    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");//not dbConnection
//    db.setDatabaseName(DATABASE_PATH + DATABASE_NAME);
//    if (! db.open()) {
//        qWarning() << "Open database error:" << db.lastError();
////            mutex.unlock();
//        return QSqlDatabase();
//    } else {
////            mutex.unlock();
//        return db;
//    }
////}
//}

void DBManager::checkDatabase()
{
    QDir dd(DATABASE_PATH);
    if (! dd.exists()) {
        //  qDebug() << "create database paths";
        dd.mkpath(DATABASE_PATH);
//        if (dd.exists())
//            qDebug() << "create database succeed!";
//        else
//            qDebug() << "create database failed!";
    } else {
//        qDebug() << "database is exist!";
    }
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    bool tableExist = false;
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT name FROM sqlite_master "
                  "WHERE type=\"table\" AND name = \"ImageTable3\"");
    if (query.exec() && query.first()) {
        tableExist = ! query.value(0).toString().isEmpty();
    }
    //if tables not exist, create it.
    if (! tableExist) {
        QSqlQuery query(db);
        // ImageTable3
        //////////////////////////////////////////////////////////////
        //PathHash           | FilePath | FileName   | Dir  | Time | ChangeTime //
        //TEXT primari key   | TEXT     | TEXT       | TEXT | TEXT | TEXT //
        //////////////////////////////////////////////////////////////
        query.exec(QString("CREATE TABLE IF NOT EXISTS ImageTable3 ( "
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
        query.exec(QString("CREATE TABLE IF NOT EXISTS AlbumTable3 ( "
                           "AlbumId INTEGER primary key, "
                           "AlbumName TEXT, "
                           "PathHash TEXT)"));

        // TrashTable
        //////////////////////////////////////////////////////////////
        //PathHash           | FilePath | FileName   | Dir  | Time | ChangeTime | AlbumName //
        //TEXT primari key   | TEXT     | TEXT       | TEXT | TEXT | TEXT       | TEXT //
        //////////////////////////////////////////////////////////////
        query.exec(QString("CREATE TABLE IF NOT EXISTS TrashTable ( "
                           "PathHash TEXT primary key, "
                           "FilePath TEXT, "
                           "FileName TEXT, "
                           "Dir TEXT, "
                           "Time TEXT, "
                           "ChangeTime TEXT, "
                           "AlbumName TEXT)"));
//        // Check if there is an old version table exist or not

//        //TODO: AlbumTable's primary key is changed, need to importVersion again
//        importVersion1Data();
//        importVersion2Data();
    } else {
        // 判断ImageTable3中是否有ChangeTime字段
        QString strSqlImage = QString::fromLocal8Bit("select sql from sqlite_master where name = \"ImageTable3\" and sql like \"%ChangeTime%\"");
        QSqlQuery queryImage(db);
        queryImage.exec(strSqlImage);
        if (!queryImage.next()) {
            // 无ChangeTime字段,则增加ChangeTime字段,赋值当前时间
            QString strDate = QDateTime::currentDateTime().toString(DATETIME_FORMAT_DATABASE);
            queryImage.exec(QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"ChangeTime\" TEXT default \"%1\"")
                            .arg(strDate));
        }

        // 判断TrashTable中是否有ChangeTime字段
        QString strSqlTrash = QString::fromLocal8Bit("select * from sqlite_master where name = \"TrashTable\" and sql like \"%ChangeTime%\"");
        QSqlQuery queryTrash(db);
        queryTrash.exec(strSqlTrash);
        if (!queryTrash.next()) {
            // 无ChangeTime字段,则增加ChangeTime字段,赋值当前时间
            QString strDate = QDateTime::currentDateTime().toString(DATETIME_FORMAT_DATABASE);
            queryTrash.exec(QString("ALTER TABLE \"TrashTable\" ADD COLUMN \"ChangeTime\" TEXT default \"%1\"")
                            .arg(strDate));
        }

        // 判断TrashTable中是否有AlbumName字段
        QString strSqlTrashs = QString::fromLocal8Bit("select * from sqlite_master where name = \"TrashTable\" and sql like \"%AlbumName%\"");
        QSqlQuery queryTrashs(db);
        queryTrashs.exec(strSqlTrashs);
        if (!queryTrashs.next()) {
            // 无AlbumName字段,则增加AlbumName字段,赋值空
            QString strAlbumName = "";
            queryTrashs.exec(QString("ALTER TABLE \"TrashTable\" ADD COLUMN \"AlbumName\" TEXT default \"%1\"")
                             .arg(strAlbumName));
        }
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();

}

void DBManager::importVersion1Data()
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    bool tableExist = false;
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT name FROM sqlite_master "
                  "WHERE type=\"table\" AND name = \"ImageTable\"");
    if (query.exec() && query.first()) {
        tableExist = ! query.value(0).toString().isEmpty();
    }
    if (tableExist) {
        // Import ImageTable into ImageTable3
        query.clear();
        query.setForwardOnly(true);
        query.prepare("SELECT filename, filepath, time, changeTime "
                      "FROM ImageTable ORDER BY time DESC");
        if (! query.exec()) {
            //  qWarning() << "Import ImageTable into ImageTable3 failed: "
            //              << query.lastError();
            mutex.unlock();
        } else {
            DBImgInfoList infos;
            using namespace utils::base;
            while (query.next()) {
                DBImgInfo info;
                info.fileName = query.value(0).toString();
                info.filePath = query.value(1).toString();
                info.time = stringToDateTime(query.value(2).toString());
//                info.changeTime = stringToDateTime(query.value(3).toString());
                info.changeTime = QDateTime::fromString(query.value(3).toString(), DATETIME_FORMAT_DATABASE);

                infos << info;
            }
            mutex.unlock();
            insertImgInfos(infos);
            mutex.relock();
        }

        // Import AlbumTable into AlbumTable3
        query.clear();
        query.prepare("SELECT DISTINCT a.albumname, i.filepath "
                      "FROM ImageTable AS i, AlbumTable AS a "
                      "WHERE i.filename=a.filename ");
        if (! query.exec()) {
            //   qWarning() << "Import AlbumTable into AlbumTable3 failed: "
            //             << query.lastError();
            mutex.unlock();
        } else {
            // <Album-Paths>
            QMap<QString, QStringList> aps;
            using namespace utils::base;
            while (query.next()) {
                QString album = query.value(0).toString();
                QString path = query.value(1).toString();
                if (aps.keys().contains(album)) {
                    aps[album].append(path);
                } else {
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
            //   qWarning() << "Drop old tables failed: " << query.lastError();
        }
        query.prepare("DROP TABLE ImageTable");
        if (! query.exec()) {
            //   qWarning() << "Drop old tables failed: " << query.lastError();
        }
        mutex.unlock();
    }
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
    db.close();
}

void DBManager::importVersion2Data()
{
    QMutexLocker mutex(&m_mutex);
    QSqlDatabase db = getDatabase();
    if (! db.isValid()) {
        return;
    }
    bool tableExist = false;
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.prepare("SELECT name FROM sqlite_master "
                  "WHERE type=\"table\" AND name = \"ImageTable2\"");
    if (query.exec() && query.first()) {
        tableExist = ! query.value(0).toString().isEmpty();
    }

    if (tableExist) {
        // Import ImageTable2 into ImageTable3
        query.clear();
        query.prepare("SELECT FileName, FilePath, Time, ChangeTime "
                      "FROM ImageTable2 ORDER BY Time DESC");
        if (! query.exec()) {
            //  qWarning() << "Import ImageTable2 into ImageTable3 failed: "
            //             << query.lastError();
            mutex.unlock();
        } else {
            DBImgInfoList infos;
            using namespace utils::base;
            while (query.next()) {
                DBImgInfo info;
                info.fileName = query.value(0).toString();
                info.filePath = query.value(1).toString();
                info.time = stringToDateTime(query.value(2).toString());
//                info.changeTime = stringToDateTime(query.value(3).toString());
                info.changeTime = QDateTime::fromString(query.value(3).toString(), DATETIME_FORMAT_DATABASE);

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
            //    qWarning() << "Import AlbumTable2 into AlbumTable3 failed: "
            //              << query.lastError();
            mutex.unlock();
        } else {
            // <Album-Paths>
            QMap<QString, QStringList> aps;
            using namespace utils::base;
            while (query.next()) {
                QString album = query.value(0).toString();
                QString path = query.value(1).toString();
                if (aps.keys().contains(album)) {
                    aps[album].append(path);
                } else {
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
            //   qWarning() << "Drop old tables failed: " << query.lastError();
        }
        query.prepare("DROP TABLE ImageTable2");
        if (! query.exec()) {
            //   qWarning() << "Drop old tables failed: " << query.lastError();
        }
        mutex.unlock();
    }
    db.close();
//    // 连接使用完后需要释放回数据库连接池
    //ConnectionPool::closeConnection(db);
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
    query.prepare("SELECT "
                  "FilePath "
                  "FROM TrashTable ORDER BY Time DESC");
    if (! query.exec()) {
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
    query.prepare("SELECT FilePath, FileName, Dir, Time, ChangeTime, AlbumName "
                  "FROM TrashTable ORDER BY ChangeTime DESC");
    if (! query.exec()) {
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
//            info.changeTime = stringToDateTime(query.value(4).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.albumname = query.value(5).toString();

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

    QVariantList pathhashs, filenames, filepaths, dirs, times, changetimes, albumnames;

    for (DBImgInfo info : infos) {
        filenames << info.fileName;
        filepaths << info.filePath;
        pathhashs << utils::base::hash(info.filePath);
        dirs << info.dirHash;
        times << utils::base::timeToString(info.time, true);
        changetimes << info.changeTime.toString(DATETIME_FORMAT_DATABASE);
        albumnames << info.albumname;
    }

    // Insert into TrashTable
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare("REPLACE INTO TrashTable "
                  "(PathHash, FilePath, FileName, Dir, Time, ChangeTime, AlbumName) VALUES (?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(pathhashs);
    query.addBindValue(filepaths);
    query.addBindValue(filenames);
    query.addBindValue(dirs);
    query.addBindValue(times);
    query.addBindValue(changetimes);
    query.addBindValue(albumnames);
    if (! query.execBatch()) {
        //   qWarning() << "Insert data into TrashTable failed: "
        //             << query.lastError();
        query.exec("COMMIT");
        db.close();
    } else {
        query.exec("COMMIT");
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
        //  qWarning() << "Remove data from TrashTable failed: "
        //            << query.lastError();
        query.exec("COMMIT");
        db.close();
    } else {
        query.exec("COMMIT");
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
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    QString qs = "DELETE FROM AlbumTable3 WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        //  qWarning() << "Remove data from AlbumTable3 failed: "
        //            << query.lastError();
        query.exec("COMMIT");
    } else {
        query.exec("COMMIT");
    }

    // Remove from image table
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    qs = "DELETE FROM TrashTable WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        //  qWarning() << "Remove data from TrashTable failed: "
        //             << query.lastError();
        query.exec("COMMIT");
    } else {
        query.exec("COMMIT");
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
    query.prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime, AlbumName FROM TrashTable "
                          "WHERE %1= :value ORDER BY Time DESC").arg(key));

    query.bindValue(":value", value);

    if (!query.exec()) {
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
//            info.changeTime = stringToDateTime(query.value(4).toString());
            info.changeTime = QDateTime::fromString(query.value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.albumname = query.value(5).toString();

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
    if (! db.isValid()) {
        return 0;
    }
    QSqlQuery query(db);
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare("SELECT COUNT(*) FROM TrashTable");
    if (query.exec()) {
        query.first();
        int count = query.value(0).toInt();
        query.exec("COMMIT");
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
