#ifndef DBMANAGER_H
#define DBMANAGER_H

// ImageTable
///////////////////////////////////////////////////////
//FilePath           | FileName | Dir        | Time  //
//TEXT primari key   | TEXT     | TEXT       | TEXT  //
///////////////////////////////////////////////////////

// AlbumTable
/////////////////////////////////////////////////////
//AP               | AlbumName         | FilePath  //
//TEXT primari key | TEXT              | TEXT      //
/////////////////////////////////////////////////////

#include "utils/baseutils.h"
#include <QObject>
#include <QDateTime>
#include <QMutex>
#include <QDebug>
#include <QDebug>
#include <QDir>
#include <QMutex>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>

struct DBAlbumInfo {
    QString name;
    int count;
    QDateTime beginTime;
    QDateTime endTime;
};

struct DBImgBaseInfo {
    QString fileName;               // 名称
    QString fileFormat;             // 类型
    QString fileSize;               // 大小
    QString dimension;              // 尺寸
    QDateTime time;                 // 时间
};

struct DBImgDetailedInfo
{
    QString exposureMode;           // 曝光模式
    QString exposureProgram;        // 曝光程度
    QString exposureTime;           // 曝光时间
    QString flashLamp;              // 闪光灯
    QString apertureValue;          // 光圈大小
    QString focalLength;            // 焦距
    QString ISOSpeedRatings;        // ISO光感度
    QString maxApertureValue;       // 最大光圈值
    QString meteringMode;           // 测光模式
    QString whiteBalance;           // 白平衡
    QString flashExposureComp;      // 闪光灯补偿
    QString cameraModel;            // 镜头型号
};

struct DBImgInfo {
    QString filePath;                       // 路径
    QString thumbnailPath;                  // 缩略图路径
    QString dirHash;
    DBImgBaseInfo imgBaseInfo;              // 基本信息
    DBImgDetailedInfo imgDetailedInfo;      // 详细信息
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

private:
    const DBImgInfoList getImgInfos(const QString &key, const QString &value) const;
    const QSqlDatabase getDatabase() const;
    void checkDatabase();

    static DBManager* m_dbManager;
private:
    QString m_connectionName;
    mutable QMutex m_mutex;
};

#endif // DBMANAGER_H
