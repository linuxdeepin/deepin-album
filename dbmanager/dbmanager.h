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

const QString DATETIME_FORMAT_DATABASE = "yyyy.MM.dd hh:mm";

struct DBAlbumInfo {
    QString name;
    int count;
    QDateTime beginTime;
    QDateTime endTime;
};

struct DBImgInfo {
    QString filePath;
    QString fileName;
    QString dirHash;
    QDateTime time;     // 图片创建时间
    QDateTime changeTime;   // 导入时间 Or 删除时间
    QString albumname;      // 图片所属相册名，以","分隔

    bool operator==(const DBImgInfo& other)
    {
        return (filePath == other.filePath &&
                fileName == other.fileName &&
                dirHash == other.dirHash &&
                time == other.time &&
                changeTime == other.changeTime &&
                albumname == other.albumname);
    }

    friend QDebug operator<<(QDebug &dbg, const DBImgInfo& info) {
        dbg << "(DBImgInfo)["
            << "Path:" << info.filePath
            << "Name:" << info.fileName
            << "Dir:" << info.dirHash
            << "Time:" << info.time
            << "ChangeTime:" << info.changeTime
            << "AlbumName:" << info.albumname
            << "]";
        return dbg;
    }
};
typedef QList<DBImgInfo> DBImgInfoList;

class QSqlDatabase;
class DBManager : public QObject
{
    Q_OBJECT
public:
    static DBManager* instance();
    explicit DBManager(QObject *parent = 0);

    // TableImage
    const QStringList       getAllPaths() const;
    const DBImgInfoList     getAllInfos() const;
    const QStringList       getAllTimelines() const;
    const DBImgInfoList     getInfosByTimeline(const QString &timeline) const;
    const QStringList       getImportTimelines() const;
    const DBImgInfoList     getInfosByImportTimeline(const QString &timeline) const;
    const DBImgInfo         getInfoByName(const QString &name) const;
    const DBImgInfo         getInfoByPath(const QString &path) const;
    const DBImgInfo         getInfoByPathHash(const QString &pathHash) const;
    int                     getImgsCount() const;
    int                     getImgsCountByDir(const QString &dir) const;
    const QStringList       getPathsByDir(const QString &dir) const;
    bool                    isImgExist(const QString &path) const;
    void insertImgInfos(const DBImgInfoList &infos);
    void removeImgInfos(const QStringList &paths);
    void removeDir(const QString &dir);
    void removeImgInfosNoSignal(const QStringList &paths);
    const DBImgInfoList     getInfosForKeyword(const QString &keywords) const;
    const DBImgInfoList     getTrashInfosForKeyword(const QString &keywords) const;
    const DBImgInfoList     getInfosForKeyword(const QString &album, const QString &keywords) const;

    // TableAlbum
    const DBAlbumInfo       getAlbumInfo(const QString &album) const;
    const QStringList       getAllAlbumNames() const;
    const QStringList       getPathsByAlbum(const QString &album) const;
    const DBImgInfoList     getInfosByAlbum(const QString &album) const;
    int                     getImgsCountByAlbum(const QString &album) const;
    int                     getAlbumsCount() const;
    bool                    isAlbumExistInDB(const QString &album) const;
    bool                    isImgExistInAlbum(const QString &album, const QString &path) const;
    void insertIntoAlbum(const QString &album, const QStringList &paths);
    void removeAlbum(const QString &album);
    void removeFromAlbum(const QString &album, const QStringList &paths);
    void renameAlbum(const QString &oldAlbum, const QString &newAlbum);
    void removeFromAlbumNoSignal(const QString &album, const QStringList &paths);
    void insertIntoAlbumNoSignal(const QString &album, const QStringList &paths);
    // TabelTrash
    const QStringList       getAllTrashPaths() const;
    const DBImgInfoList     getAllTrashInfos() const;
    void insertTrashImgInfos(const DBImgInfoList &infos);
    void removeTrashImgInfos(const QStringList &paths);
    void removeTrashImgInfosNoSignal(const QStringList &paths);
    const DBImgInfo         getTrashInfoByPath(const QString &path) const;
    const DBImgInfoList     getTrashImgInfos(const QString &key, const QString &value) const;
    int                     getTrashImgsCount() const;

private:
    const DBImgInfoList getInfosByNameTimeline(const QString &value) const;
    const DBImgInfoList getImgInfos(const QString &key, const QString &value) const;
    const QSqlDatabase getDatabase() const;
    void checkDatabase();
    void importVersion1Data();
    void importVersion2Data();

    static DBManager* m_dbManager;
private:
    QString m_connectionName;
    mutable QMutex m_mutex;
};

#endif // DBMANAGER_H
