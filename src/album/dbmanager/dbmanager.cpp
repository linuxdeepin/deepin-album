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
std::once_flag DBManager::instanceFlag;

DBManager *DBManager::instance()
{
    //线程安全单例
    std::call_once(instanceFlag, [&]() {
        m_dbManager = new DBManager;
    });
    return m_dbManager;
}

DBManager::DBManager(QObject *parent)
    : QObject(parent)
{
    checkDatabase();
}

const QStringList DBManager::getAllPaths() const
{
    QMutexLocker mutex(&m_mutex);
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

const DBImgInfoList DBManager::getAllInfos(int loadCount)const
{
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);
    bool b = false;
    if (loadCount == 0) {
        b = m_query->prepare("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType FROM ImageTable3 order by Time desc");
    } else {
        b = m_query->prepare("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType FROM ImageTable3 order by Time desc limit 80");
    }
    if (!b || ! m_query->exec()) {
        return infos;
    } else {
        using namespace utils::base;
        while (m_query->next()) {
            DBImgInfo info;
            info.filePath = m_query->value(0).toString();
            info.time = stringToDateTime(m_query->value(3).toString());
            info.changeTime = QDateTime::fromString(m_query->value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(m_query->value(5).toString(), DATETIME_FORMAT_DATABASE);
            info.itemType = static_cast<ItemType>(m_query->value(6).toInt());
            infos << info;
        }
    }
    return infos;
}

const QStringList DBManager::getAllTimelines() const
{
    QMutexLocker mutex(&m_mutex);
    QStringList times;
    m_query->setForwardOnly(true);
    if (!m_query->exec("SELECT DISTINCT Time FROM ImageTable3 ORDER BY Time DESC")) {
        return times;
    } else {
        while (m_query->next()) {
            times << m_query->value(0).toString();
        }
    }
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

    m_query->setForwardOnly(true);
    if (!m_query->exec("SELECT DISTINCT ImportTime FROM ImageTable3 ORDER BY ImportTime DESC")) {
        return importtimes;
    } else {
        while (m_query->next()) {
            importtimes << m_query->value(0).toString();
        }
    }
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

    m_query->setForwardOnly(true);
    if (m_query->exec("SELECT COUNT(*) FROM ImageTable3")) {
        m_query->first();
        int count = m_query->value(0).toInt();
        return count;
    }
    return 0;
}

void DBManager::insertImgInfos(const DBImgInfoList &infos)
{
    QMutexLocker mutex(&m_mutex);
    m_query->setForwardOnly(true);
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
//        qDebug() << query.lastError();
    }
    QString qs("REPLACE INTO ImageTable3 (PathHash, FilePath, FileName, Time, "
               "ChangeTime, ImportTime, FileType) VALUES (?, ?, ?, ?, ?, ?, ?)");

    if (!m_query->prepare(qs)) {
    }

    for (const auto &info : infos) {
        m_query->addBindValue(utils::base::hashByString(info.filePath));
        m_query->addBindValue(info.filePath);
        m_query->addBindValue(info.getFileNameFromFilePath());
        m_query->addBindValue(info.time.toString("yyyy.MM.dd"));
        m_query->addBindValue(info.changeTime.toString(DATETIME_FORMAT_DATABASE));
        m_query->addBindValue(info.importTime.toString(DATETIME_FORMAT_DATABASE));
        m_query->addBindValue(info.itemType);
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
    DBImgInfoList infos;
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hashByString(path);
        infos.append(getImgInfos("FilePath", path));
    }

    QMutexLocker mutex(&m_mutex);

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
    emit dApp->signalM->imagesRemovedPar(infos);
}

void DBManager::removeImgInfosNoSignal(const QStringList &paths)
{
    QMutexLocker mutex(&m_mutex);
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
//        qDebug() << "COMMIT failed.";
    }

    // Remove from image table
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
    }
    qs = "DELETE FROM ImageTable3 WHERE PathHash=:hash";
    if (!m_query->prepare(qs)) {
    }
    for (auto &eachHash : pathHashs) {
        m_query->bindValue(":hash", eachHash);
        if (!m_query->exec()) {
        }
    }

    if (!m_query->exec("COMMIT")) {
    }
}

const QList<std::pair<int, QString>> DBManager::getAllAlbumNames(AlbumDBType atype) const
{
    QMutexLocker mutex(&m_mutex);
    QList<std::pair<int, QString>> list;
    m_query->setForwardOnly(true);
    //以UID和相册名称同时作为筛选条件，名称作为UI显示用，UID作为UI和数据库通信的钥匙
    if (m_query->exec(QString("SELECT DISTINCT UID, AlbumName FROM AlbumTable3 WHERE AlbumDBType =") + QString::number(atype))) {
        while (m_query->next()) {
            list.push_back(std::make_pair(m_query->value(0).toInt(), m_query->value(1).toString()));
        }
    }

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

        monitorPaths.push_back(stdPicPath + "/" + "Screen Capture");
        monitorPaths.push_back(stdPicPath + "/" + "Camera");
        monitorPaths.push_back(stdPicPath + "/" + "Draw");
    }

    //视频路径
    auto stdMoviePaths = QStandardPaths::standardLocations(QStandardPaths::MoviesLocation);
    if (!stdMoviePaths.isEmpty()) {
        auto stdMoviePath = stdMoviePaths[0];

        monitorPaths.push_back(stdMoviePath + "/" + "Screen Capture");
        monitorPaths.push_back(stdMoviePath + "/" + "Camera");
    }

    return monitorPaths;
}

const QStringList DBManager::getPathsByAlbum(int UID, AlbumDBType atype) const
{
    QMutexLocker mutex(&m_mutex);
    QStringList list;
    m_query->setForwardOnly(true);
    bool b = m_query->prepare("SELECT DISTINCT i.FilePath "
                              "FROM ImageTable3 AS i, AlbumTable3 AS a "
                              "WHERE i.PathHash=a.PathHash "
                              "AND a.UID=:UID "
                              "AND a.AlbumDBType=:atype ");
    m_query->bindValue(":UID", UID);
    m_query->bindValue(":atype", atype);
    if (!b || ! m_query->exec()) {
    } else {
        while (m_query->next()) {
            list << m_query->value(0).toString();
        }
    }

    return list;
}

const DBImgInfoList DBManager::getInfosByAlbum(int UID) const
{
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);
    bool b = m_query->prepare("SELECT DISTINCT i.FilePath, i.FileName, i.FileType, i.Time, i.ChangeTime, i.ImportTime "
                              "FROM ImageTable3 AS i, AlbumTable3 AS a "
                              "WHERE i.PathHash=a.PathHash "
                              "AND a.UID=:UID ");
    m_query->bindValue(":UID", UID);
    if (!b || ! m_query->exec()) {
        //    qWarning() << "Get ImgInfo by album failed: " << query.lastError();
    } else {
        using namespace utils::base;
        while (m_query->next()) {
            DBImgInfo info;
            info.filePath = m_query->value(0).toString();
            info.itemType = static_cast<ItemType>(m_query->value(2).toInt());
            info.time = stringToDateTime(m_query->value(3).toString());
            info.changeTime = QDateTime::fromString(m_query->value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(m_query->value(5).toString(), DATETIME_FORMAT_DATABASE);
            infos << info;
        }
    }
    return infos;
}

int DBManager::getItemsCountByAlbum(int UID, const ItemType &type) const
{
    int count = 0;
    QMutexLocker mutex(&m_mutex);
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
    QMutexLocker mutex(&m_mutex);
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

bool DBManager::isImgExistInAlbum(int UID, const QString &path, AlbumDBType atype) const
{
    QMutexLocker mutex(&m_mutex);
    m_query->setForwardOnly(true);
    bool b = m_query->prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE PathHash = :hash "
                              "AND UID = :UID "
                              "AND AlbumDBType =:atype ");
    if (!b) {
        return false;
    }
    m_query->bindValue(":hash", utils::base::hashByString(path));
    m_query->bindValue(":UID", UID);
    m_query->bindValue(":atype", atype);
    if (m_query->exec()) {
        m_query->first();
        return (m_query->value(0).toInt() == 1);
    } else {
        return false;
    }
}

QString DBManager::getAlbumNameFromUID(int UID) const
{
    QMutexLocker mutex(&m_mutex);
    m_query->setForwardOnly(true);
    bool b = m_query->exec(QString("SELECT DISTINCT AlbumName FROM AlbumTable3 WHERE UID=%1").arg(UID));
    if (!b || !m_query->next()) {
        return QString();
    }

    return m_query->value(0).toString();
}

AlbumDBType DBManager::getAlbumDBTypeFromUID(int UID) const
{
    QMutexLocker mutex(&m_mutex);
    m_query->setForwardOnly(true);
    bool b = m_query->exec(QString("SELECT DISTINCT AlbumDBType FROM AlbumTable3 WHERE UID=%1").arg(UID));
    if (!b || !m_query->next()) {
        return TypeCount;
    }

    return static_cast<AlbumDBType>(m_query->value(0).toInt());
}

bool DBManager::isAlbumExistInDB(int UID, AlbumDBType atype) const
{
    QMutexLocker mutex(&m_mutex);
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
    QMutexLocker mutex(&m_mutex);
    int currentUID = albumMaxUID++;
    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs << utils::base::hashByString(path);
    }
    m_query->setForwardOnly(true);
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
    }
    auto qs = QString("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash, AlbumDBType, UID) VALUES (null, \"%1\", ?, %2, %3)").arg(album).arg(atype).arg(currentUID);
    bool b = m_query->prepare(qs);
    if (!b) {
        return -1;
    }

    for (auto &eachHash : pathHashs) {
        m_query->addBindValue(eachHash);
        if (!m_query->exec()) {
        }
    }

    if (!m_query->exec("COMMIT")) {
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
    QMutexLocker mutex(&m_mutex);
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
                         " VALUES (null, \"%1\", %2, %3, ?)")
                 .arg(album).arg(atype).arg(UID);
    if (!m_query->prepare(qs)) {
    }
    for (auto &eachPath : paths) {
        m_query->addBindValue(utils::base::hashByString(eachPath));
        if (!m_query->exec()) {
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

    return true;
}

void DBManager::removeAlbum(int UID)
{
    QMutexLocker mutex(&m_mutex);
    if (!m_query->exec(QString("DELETE FROM AlbumTable3 WHERE UID=") + QString::number(UID))) {
    }
}

void DBManager::removeFromAlbum(int UID, const QStringList &paths, AlbumDBType atype)
{
    QMutexLocker mutex(&m_mutex);

    QStringList pathHashs;
    for (QString path : paths) {
        pathHashs.push_back(utils::base::hashByString(path));
    }
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
    QMutexLocker mutex(&m_mutex);
    if (!m_query->exec(QString("UPDATE AlbumTable3 SET AlbumName=%1 WHERE UID=%2 AND AlbumDBType=%3").arg(newAlbum).arg(UID).arg(atype))) {
    }
}

const DBImgInfoList DBManager::getInfosByNameTimeline(const QString &value) const
{
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);

    QString queryStr = "SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType FROM ImageTable3 "
                       "WHERE FileName like '%" + value + "%' OR Time like '%" + value + "%' ORDER BY Time DESC";

    bool b = m_query->prepare(queryStr);

    if (!b || !m_query->exec()) {
    } else {
        using namespace utils::base;
        while (m_query->next()) {
            DBImgInfo info;
            info.filePath = m_query->value(0).toString();
            info.time = stringToDateTime(m_query->value(3).toString());
            info.changeTime = QDateTime::fromString(m_query->value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(m_query->value(5).toString(), DATETIME_FORMAT_DATABASE);
            info.itemType = static_cast<ItemType>(m_query->value(6).toInt());
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
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);

    //切换到UID后，纯关键字搜索应该不受影响
    QString queryStr = "SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType AlbumName FROM TrashTable3 "
                       "WHERE FileName like '%" + keywords + "%' OR Time like '%" + keywords + "%' ORDER BY Time DESC";

    bool b = m_query->prepare(queryStr);

    if (!b || !m_query->exec()) {
    } else {
        using namespace utils::base;
        while (m_query->next()) {
            DBImgInfo info;
            info.filePath = m_query->value(0).toString();
            info.time = stringToDateTime(m_query->value(3).toString());
            info.changeTime = QDateTime::fromString(m_query->value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(m_query->value(5).toString(), DATETIME_FORMAT_DATABASE);
            info.itemType = ItemType(m_query->value(6).toInt());
            infos << info;
        }
    }
    return infos;
}

const DBImgInfoList DBManager::getInfosForKeyword(int UID, const QString &keywords) const
{
    QMutexLocker mutex(&m_mutex);

    DBImgInfoList infos;

    QString queryStr = "SELECT DISTINCT i.FilePath, i.FileName, i.Dir, i.Time, i.ChangeTime, i.ImportTime "
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
            info.time = stringToDateTime(m_query->value(3).toString());
            info.changeTime = QDateTime::fromString(m_query->value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(m_query->value(5).toString(), DATETIME_FORMAT_DATABASE);
            infos << info;
        }
    }
    return infos;
}

const QMultiMap<QString, QString> DBManager::getAllPathAlbumNames() const
{
    QMutexLocker mutex(&m_mutex);

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

const DBImgInfoList DBManager::getImgInfos(const QString &key, const QString &value) const
{
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);
    bool b = m_query->prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType FROM ImageTable3 "
                                      "WHERE %1= :value ORDER BY Time DESC").arg(key));

    m_query->bindValue(":value", value);

    if (!b || !m_query->exec()) {
    } else {
        using namespace utils::base;
        while (m_query->next()) {
            DBImgInfo info;
            info.filePath = m_query->value(0).toString();
            info.time = stringToDateTime(m_query->value(3).toString());
            info.changeTime = QDateTime::fromString(m_query->value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(m_query->value(5).toString(), DATETIME_FORMAT_DATABASE);
            info.itemType = static_cast<ItemType>(m_query->value(6).toInt());
            infos << info;
        }
    }
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

    //这里再去检查已有的数据库
    if (!m_query->exec("SELECT FullPath FROM CustomAutoImportPathTable3")) {
        return true;
    }

    while (m_query->next()) {
        auto currentPath = m_query->value(0).toString();
        if (path.startsWith(currentPath) || currentPath.startsWith(path)) {
            return true;
        }
    }

    return false;
}

int DBManager::createNewCustomAutoImportPath(const QString &path, const QString &albumName)
{
    QMutexLocker mutex(&m_mutex);

    //1.新建相册
    int UID = albumMaxUID++;

    if (!m_query->exec(QString("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash, AlbumDBType, UID) VALUES (null, \"%1\", \"%2\", %3, %4)")
                       .arg(albumName).arg("7215ee9c7d9dc229d2921a40e899ec5f").arg(AutoImport).arg(UID))) {
        return -1;
    }

    //2.新建保存路径

    if (!m_query->exec(QString("INSERT INTO CustomAutoImportPathTable3 (UID, FullPath, AlbumName) VALUES (%1, \"%2\", \"%3\")")
                       .arg(UID).arg(path).arg(albumName))) {
        return -1;
    }

    return UID;
}

void DBManager::removeCustomAutoImportPath(int UID)
{
    QMutexLocker mutex(&m_mutex);
    m_query->setForwardOnly(true);

    //0.查询在该监控路径下的图片
    if (!m_query->exec(QString("SELECT PathHash FROM AlbumTable3 WHERE UID=") + QString::number(UID))) {
    }
    QStringList hashs;
    while (m_query->next()) {
        hashs.push_back(m_query->value(0).toString());
    }

    //1.删除图片
    if (!m_query->exec("BEGIN")) {
    }
    if (m_query->prepare("DELETE FROM ImageTable3 WHERE PathHash=:hash")) {
    }
    for (auto &eachHash : hashs) {
        m_query->bindValue(":hash", eachHash);
        if (!m_query->exec()) {
        }
    }
    if (!m_query->exec("COMMIT")) {
    }

    //2.删除路径
    if (!m_query->exec(QString("DELETE FROM CustomAutoImportPathTable3 WHERE UID=") + QString::number(UID))) {
    }

    //2.删除相册
    if (!m_query->exec(QString("DELETE FROM AlbumTable3 WHERE UID=") + QString::number(UID))) {
    }
}

std::map<int, QString> DBManager::getAllCustomAutoImportUIDAndPath()
{
    std::map<int, QString> result;

    QMutexLocker mutex(&m_mutex);
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

    // ImageTable3
    //////////////////////////////////////////////////////////////
    //PathHash           | FilePath | FileName   | Dir  | Time | ChangeTime | ImportTime//
    //TEXT primari key   | TEXT     | TEXT       | TEXT | TEXT | TEXT       | TEXT      //
    //////////////////////////////////////////////////////////////
    bool b = m_query->exec(QString("CREATE TABLE IF NOT EXISTS ImageTable3 ( "
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
    bool c = m_query->exec(QString("CREATE TABLE IF NOT EXISTS AlbumTable3 ( "
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
    bool d = m_query->exec(QString("CREATE TABLE IF NOT EXISTS TrashTable3 ( "
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
        if (m_query->exec(QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"ChangeTime\" TEXT default \"%1\"")
                          .arg(strDate))) {
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
        if (m_query->exec(QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"ImportTime\" TEXT default \"%1\"")
                          .arg(strDate))) {
            qDebug() << "add ImportTime success";
        }
    }

    // 判断ImageTable3中是否有FileType字段，区分是图片还是视频
    QString strSqlFileType = QString::fromLocal8Bit(
                                 "select * from sqlite_master where name = 'ImageTable3' and sql like '%FileType%'");
    int fileType = static_cast<int>(ItemTypePic);
    if (!m_query->exec(strSqlFileType)) {
        qDebug() << "add FileType failed";
    }
    if (!m_query->next()) {
        // 无FileType字段,则增加FileType字段,赋值1,默认是图片
        if (m_query->exec(QString("ALTER TABLE \"ImageTable3\" ADD COLUMN \"FileType\" INTEGER default \"%1\"")
                          .arg(QString::number(fileType)))) {
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
                qDebug() << "add FileType success";
            }
        }
    }

    // 判断AlbumTable3中是否有AlbumDBType字段
    QString strSqlDBType = QString::fromLocal8Bit("select * from sqlite_master where name = \"AlbumTable3\" and sql like \"%AlbumDBType%\"");
    bool q2 = m_query->exec(strSqlDBType);
    if (q2 && !m_query->next()) {
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
        //1.获取当前已存在的album name
        if (m_query->exec(QString("SELECT DISTINCT \"AlbumName\" FROM \"AlbumTable3\""))) {
            qDebug() << "search album name success";
        }

        //2.在内存中为其进行编号
        QStringList nameList;
        while (m_query->next()) {
            auto currentName = m_query->value(0).toString();
            nameList.push_back(currentName);
        }

        //3.插入特殊UID字段
        insertSpUID("Favorite", Favourite, u_Favorite);
        insertSpUID("Screen Capture", AutoImport, u_ScreenCapture);//使用album name的SQL语句注意加冒号
        insertSpUID("Camera", AutoImport, u_Camera);
        insertSpUID("Draw", AutoImport, u_Draw);

        //4.初始化max uid
        albumMaxUID = u_CustomStart;

        //5.写入数据库
        for (auto &currentName : nameList) {
            if (!m_query->exec(QString("UPDATE \"AlbumTable3\" SET UID = %1 WHERE \"AlbumName\" = \"%2\"").arg(albumMaxUID++).arg(currentName))) {
                qDebug() << "update AlbumTable3 UID failed";
            }
        }

        uidIsInited = true;
    }

    if (!uidIsInited) {
        //预先插入特殊UID
        insertSpUID("Favorite", Favourite, u_Favorite);
        insertSpUID("Screen Capture", AutoImport, u_ScreenCapture);//使用album name的SQL语句注意加冒号
        insertSpUID("Camera", AutoImport, u_Camera);
        insertSpUID("Draw", AutoImport, u_Draw);

        //搜索当前最大ID值
        if (!m_query->exec("SELECT max(UID) FROM AlbumTable3") || !m_query->next()) {
            qDebug() << "find max UID failed";
        }
        albumMaxUID = m_query->value(0).toInt();
        albumMaxUID++;
    }

    // 判断TrashTable的版本
    QString strSqlTrashTable = QString::fromLocal8Bit("select * from sqlite_master where name = \"TrashTable3\"");
    bool build = m_query->exec(strSqlTrashTable);
    if (!build) {
        qDebug() << m_query->lastError();
    }
    if (!m_query->next()) {
        //无新版TrashTable，则创建新表，导入旧表数据
        QString defaultImportTime = QDateTime::currentDateTime().toString(DATETIME_FORMAT_DATABASE);
        if (!m_query->exec(QString("CREATE TABLE IF NOT EXISTS TrashTable3 ( "
                                   "PathHash TEXT primary key, "
                                   "FilePath TEXT, "
                                   "FileName TEXT, "
                                   "Dir TEXT, "
                                   "Time TEXT, "
                                   "ChangeTime TEXT, "
                                   "ImportTime TEXT default \"%1\", "
                                   "FileType INTEGER)").arg(defaultImportTime))) {
            qDebug() << m_query->lastError();
        }
    } else {
        //判断TrashTable3是否包含FileType
        QString strSqlFileType = QString::fromLocal8Bit("select * from sqlite_master where name = \"TrashTable3\" and sql like \"%FileType%\"");
        bool q2 = m_query->exec(strSqlFileType);
        if (!q2) {
            qDebug() << m_query->lastError();
        }
        if (!m_query->next()) {
            // 无FileType字段,则增加FileType字段, 全部赋值为图片
            int type = ItemType::ItemTypePic;
            if (m_query->exec(QString("ALTER TABLE \"TrashTable3\" ADD COLUMN \"FileType\" INTEGER default %1")
                              .arg(QString::number(type)))) {
                qDebug() << "add AlbumDBType success";
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

    //每次启动后释放一次文件空间，防止占用过多无效空间
    if (!m_query->exec("VACUUM")) {
    }
}

void DBManager::insertSpUID(const QString &albumName, AlbumDBType astype, SpUID UID)
{
    //1.检查当前需要新建的sp相册是否存在
    if (!m_query->exec(QString("SELECT UID FROM AlbumTable3 WHERE UID=%1").arg(UID)) || m_query->next()) {
        return;
    }

    //2.不存在则插入数据
    if (!m_query->exec(QString("REPLACE INTO AlbumTable3 (AlbumId, AlbumName, PathHash, AlbumDBType, UID) VALUES (null, \"%1\", \"%2\", %3, %4)")
                       .arg(albumName).arg("7215ee9c7d9dc229d2921a40e899ec5f").arg(astype).arg(UID))) {
        qWarning() << "insertSpUID failed" << m_query->lastError().text();
    }
}

const DBImgInfoList DBManager::getAllTrashInfos() const
{
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);
    bool b = m_query->prepare("SELECT FilePath, Time, ChangeTime, ImportTime, FileType, PathHash "
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
            info.time = stringToDateTime(m_query->value(1).toString());
            info.changeTime = QDateTime::fromString(m_query->value(2).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(m_query->value(3).toString(), DATETIME_FORMAT_DATABASE);
            info.itemType = ItemType(m_query->value(4).toInt());
            info.pathHash = m_query->value(5).toString();
            infos << info;
        }
    }
    return infos;
}

void DBManager::insertTrashImgInfos(const DBImgInfoList &infos)
{
    if (infos.isEmpty()) {
        return;
    }

    //图片删除步骤

    //1.生成路径hash，复制图片到deepin-album-delete，删除原图至回收站
    QStringList pathHashs;
    for (const auto &info : infos) {
        //计算路径hash
        auto hash = utils::base::hashByString(info.filePath);
        pathHashs.push_back(hash);

        //复制操作
        QFile::copy(info.filePath, utils::base::getDeleteFullPath(hash, info.getFileNameFromFilePath()));

        //删除原图至回收站
        utils::base::trashFile(info.filePath);
    }

    QMutexLocker mutex(&m_mutex);

    //2.向数据库插入数据
    m_query->setForwardOnly(true);
    if (!m_query->exec("BEGIN IMMEDIATE TRANSACTION")) {
        qDebug() << "begin transaction failed.";
    }

    QString qs("REPLACE INTO TrashTable3 "
               "(PathHash, FilePath, FileName, Time, ChangeTime, ImportTime, FileType) VALUES (?, ?, ?, ?, ?, ?, ?)");
    if (!m_query->prepare(qs)) {
    }
    for (int i = 0; i != infos.size(); ++i) {
        m_query->addBindValue(pathHashs[i]); //复用上面生成的hash
        m_query->addBindValue(infos[i].filePath);
        m_query->addBindValue(infos[i].getFileNameFromFilePath());
        m_query->addBindValue(infos[i].time.toString("yyyy.MM.dd"));
        m_query->addBindValue(infos[i].changeTime.toString(DATETIME_FORMAT_DATABASE));
        m_query->addBindValue(infos[i].importTime.toString(DATETIME_FORMAT_DATABASE));
        m_query->addBindValue(infos[i].itemType);
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

    QMutexLocker mutex(&m_mutex);
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
    }

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

    QMutexLocker mutex(&m_mutex);

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

    //执行恢复步骤
    for (int i = 0; i != paths.size(); ++i) {
        auto deletePath = utils::base::getDeleteFullPath(pathHashs[i], DBImgInfo::getFileNameFromFilePath(paths[i])); //获取删除缓存路径
        if (!QFile::exists(deletePath)) { //文件不存在，表示要么是老版相册，要么是缓存文件已被破坏，此时需要判定文件恢复成功
            successedHashs.push_back(pathHashs[i]);
            continue;
        }
        QString recoveryName = paths[i];
        if (QFile::exists(recoveryName)) { //文件已存在，加副本标记
            recoveryName.append(tr("(copy)"));
            if (recoveryName.size() > 255) {
                failedFiles.push_back(paths[i]); //文件名过长，恢复失败
                continue;
            }
        }

        if (QFile::rename(deletePath, recoveryName)) { //尝试正常恢复
            successedHashs.push_back(pathHashs[i]); //正常恢复成功
        } else { //正常恢复失败，尝试恢复至内部路径
            if (!internalRecoveryDir.exists()) { //检查文件夹是否存在，不存在则创建
                internalRecoveryDir.mkpath(internalRecoveryPath);
            }
            recoveryName = internalRecoveryPath + "/" + DBImgInfo::getFileNameFromFilePath(paths[i]);
            if (QFile::exists(recoveryName)) { //文件已存在，加副本标记
                recoveryName.append(tr("(copy)"));
                if (recoveryName.size() > 255) {
                    failedFiles.push_back(paths[i]); //文件名过长，恢复失败
                    continue;
                }
            }
            //尝试恢复至内部路径
            if (QFile::rename(deletePath, recoveryName)) {
                successedHashs.push_back(pathHashs[i]); //恢复至内部路径
            } else { //TODO：极端特殊情况：使用内部恢复路径恢复失败
                continue;
            }
        }

        //文件名改变，需要刷新另外两个表的文件名和hash数据
        if (recoveryName != paths[i]) {
            changedPaths.push_back(std::make_tuple(pathHashs[i], recoveryName, utils::base::hashByData(recoveryName)));
        }
    }

    //3.数据库操作
    if (!successedHashs.isEmpty()) { //如果没有成功恢复的文件，则以下操作全部不会执行
        m_query->setForwardOnly(true);

        //3.1获取恢复成功的文件数据
        DBImgInfoList infos;
        QString qs("SELECT FilePath, Time, ChangeTime, ImportTime, FileType FROM TrashTable3 WHERE PathHash=:value");
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
                    continue;
                }

                info.time = utils::base::stringToDateTime(m_query->value(1).toString());
                info.changeTime = QDateTime::fromString(m_query->value(2).toString(), DATETIME_FORMAT_DATABASE);
                info.importTime = QDateTime::currentDateTime();
                info.itemType = ItemType(m_query->value(4).toInt());

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
             "ChangeTime, ImportTime, FileType) VALUES (?, ?, ?, ?, ?, ?, ?)";
        if (!m_query->prepare(qs)) {
        }
        for (const auto &info : infos) {
            m_query->addBindValue(info.pathHash);
            m_query->addBindValue(info.filePath);
            m_query->addBindValue(info.getFileNameFromFilePath());
            m_query->addBindValue(info.time.toString("yyyy.MM.dd"));
            m_query->addBindValue(info.changeTime.toString(DATETIME_FORMAT_DATABASE));
            m_query->addBindValue(info.importTime.toString(DATETIME_FORMAT_DATABASE));
            m_query->addBindValue(info.itemType);
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

    QMutexLocker mutex(&m_mutex);

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
    QMutexLocker mutex(&m_mutex);
    DBImgInfoList infos;
    m_query->setForwardOnly(true);
    bool b = m_query->prepare(QString("SELECT FilePath, FileName, Dir, Time, ChangeTime, ImportTime, FileType FROM TrashTable3 "
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
            info.time = utils::base::stringToDateTime(m_query->value(3).toString());
            info.changeTime = QDateTime::fromString(m_query->value(4).toString(), DATETIME_FORMAT_DATABASE);
            info.importTime = QDateTime::fromString(m_query->value(5).toString(), DATETIME_FORMAT_DATABASE);
            info.itemType = ItemType(m_query->value(6).toInt());

            infos << info;
        }
    }
    return infos;
}

int DBManager::getTrashImgsCount() const
{
    QMutexLocker mutex(&m_mutex);
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
    QMutexLocker mutex(&m_mutex);
    m_query->setForwardOnly(true);
    if (m_query->exec(QString("SELECT COUNT(*) FROM AlbumTable3 WHERE UID=%1 AND PathHash<>\"%2\"")
                      .arg(UID).arg("7215ee9c7d9dc229d2921a40e899ec5f"))) {
        m_query->first();
        int count = m_query->value(0).toInt();
        return count;
    }
    return 0;
}
