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
#ifndef DBMANAGER_H
#define DBMANAGER_H

// ImageTable
///////////////////////////////////////////////////////
//FilePath           | FileName | Dir        | Time | ChangeTime //
//TEXT primari key   | TEXT     | TEXT       | TEXT | TEXT //
///////////////////////////////////////////////////////

// AlbumTable
/////////////////////////////////////////////////////
//AP               | AlbumName         | FilePath  //
//TEXT primari key | TEXT              | TEXT      //
/////////////////////////////////////////////////////

#include <QObject>
#include <QDateTime>
#include <QMutex>
#include <QDebug>
#include <QSqlDatabase>
#include "albumgloabl.h"
//#include "connectionpool.h"

const QString DATETIME_FORMAT_DATABASE = "yyyy.MM.dd hh:mm";

enum AlbumDBType {
    Favourite,
    Custom
};

class QSqlDatabase;

class DBManager : public QObject
{
    Q_OBJECT
public:
    static DBManager  *instance();
    explicit DBManager(QObject *parent = nullptr);
    ~DBManager()
    {
//        ConnectionPool::release(); //释放数据库连接
    }

    // TableImage
    const QStringList       getAllPaths() const;
    const DBImgInfoList     getAllInfos(int loadCount = 0) const;
    const QStringList       getAllTimelines() const;
    const DBImgInfoList     getInfosByTimeline(const QString &timeline) const;
    const QStringList       getImportTimelines() const;
    const DBImgInfoList     getInfosByImportTimeline(const QString &timeline) const;
//    const DBImgInfo         getInfoByName(const QString &name) const;
    const DBImgInfo         getInfoByPath(const QString &path) const;
//    const DBImgInfo         getInfoByPathHash(const QString &pathHash) const;
    int                     getImgsCount() const;
//    bool                    isImgExist(const QString &path) const;
    void                    insertImgInfos(const DBImgInfoList &infos);
    void                    insertImgInfo(const DBImgInfo &info);
    void                    removeImgInfos(const QStringList &paths);
    void                    removeImgInfosNoSignal(const QStringList &paths);
    const DBImgInfoList     getInfosForKeyword(const QString &keywords) const;
    const DBImgInfoList     getTrashInfosForKeyword(const QString &keywords) const;
    const DBImgInfoList     getInfosForKeyword(const QString &album, const QString &keywords) const;

    // TableAlbum
    const QMultiMap<QString, QString> getAllPathAlbumNames() const;
    const QStringList       getAllAlbumNames(AlbumDBType atype = AlbumDBType::Custom) const;
    const QStringList       getPathsByAlbum(const QString &album, AlbumDBType atype = AlbumDBType::Custom) const;
    const DBImgInfoList     getInfosByAlbum(const QString &album, AlbumDBType atype = AlbumDBType::Custom) const;
    int                     getItemsCountByAlbum(const QString &album, const ItemType &type, AlbumDBType atype = AlbumDBType::Custom) const;
//    int                     getAlbumsCount() const;
    bool                    isAlbumExistInDB(const QString &album, AlbumDBType atype = AlbumDBType::Custom) const;
    bool                    isAllImgExistInAlbum(const QString &album, const QStringList &paths, AlbumDBType atype = AlbumDBType::Custom) const;
    bool                    isImgExistInAlbum(const QString &album, const QString &path, AlbumDBType atype = AlbumDBType::Custom) const;
    void                    insertIntoAlbum(const QString &album, const QStringList &paths, AlbumDBType atype = AlbumDBType::Custom);
    void                    removeAlbum(const QString &album, AlbumDBType atype = AlbumDBType::Custom);
    void                    removeFromAlbum(const QString &album, const QStringList &paths, AlbumDBType atype = AlbumDBType::Custom);
    void                    renameAlbum(const QString &oldAlbum, const QString &newAlbum, AlbumDBType atype = AlbumDBType::Custom);
//    void                    removeFromAlbumNoSignal(const QString &album, const QStringList &paths, AlbumDBType atype = AlbumDBType::Custom);
    void                    insertIntoAlbumNoSignal(const QString &album, const QStringList &paths, AlbumDBType atype = AlbumDBType::Custom);
    // TabelTrash
    const QStringList       getAllTrashPaths() const;
    const DBImgInfoList     getAllTrashInfos() const;
    void                    insertTrashImgInfos(const DBImgInfoList &infos);
    void                    removeTrashImgInfos(const QStringList &paths);
    void                    removeTrashImgInfosNoSignal(const QStringList &paths);
    const DBImgInfo         getTrashInfoByPath(const QString &path) const;
    const DBImgInfoList     getTrashImgInfos(const QString &key, const QString &value) const;
    int                     getTrashImgsCount() const;
    const QSqlDatabase      getDatabase() const;
private:
    const DBImgInfoList     getInfosByNameTimeline(const QString &value) const;
    const DBImgInfoList     getImgInfos(const QString &key, const QString &value, const bool &needlock = true) const;


    void                    checkDatabase();
    static DBManager       *m_dbManager;
private:
    //QString m_connectionName;
    mutable QMutex m_mutex;

//    mutable QMutex m_mutex1;
    mutable QSqlDatabase m_db;
};

#endif // DBMANAGER_H
