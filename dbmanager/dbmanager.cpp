#include "dbmanager.h"

namespace {

const QString DATABASE_PATH = QDir::homePath() + "/.local/share/deepin/deepin-album/";
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
                   "WHERE type=\"table\" AND name = \"ImageTable\"");
    if (query.exec() && query.first()) {
        tableExist = ! query.value(0).toString().isEmpty();
    }
    //if tables not exist, create it.
    if ( ! tableExist ) {
        QSqlQuery query(db);
        // ImageTable
        //////////////////////////////////////////////////////////////
        //PathHash           | FilePath | thumbnailPath | Dir  | fileName | fileFormat | fileSize | dimension | time | exposureMode | exposureProgram | exposureTime | flashLamp | apertureValue | focalLength | ISOSpeedRatings | maxApertureValue | meteringMode | whiteBalance | flashExposureComp | cameraModel //
        //TEXT primari key   | TEXT     | TEXT          | TEXT | TEXT     | TEXT       | TEXT     | TEXT      | TEXT | TEXT         | TEXT            | TEXT         | TEXT      | TEXT          | TEXT        | TEXT            | TEXT             | TEXT         | TEXT         | TEXT              | TEXT        //
        //////////////////////////////////////////////////////////////
        query.exec( QString("CREATE TABLE IF NOT EXISTS ImageTable ( "
                            "PathHash TEXT primary key, "
                            "FilePath TEXT, "
                            "thumbnailPath TEXT, "
                            "FileName TEXT, "
                            "Dir TEXT, "
                            "Time TEXT )"));

        // AlbumTable
        //////////////////////////////////////////////////////////
        //AlbumId               | AlbumName         | PathHash  //
        //INTEGER primari key   | TEXT              | TEXT      //
        //////////////////////////////////////////////////////////
        query.exec( QString("CREATE TABLE IF NOT EXISTS AlbumTable ( "
                            "AlbumId INTEGER primary key, "
                            "AlbumName TEXT, "
                            "PathHash TEXT)") );
    }

    mutex.unlock();
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
                   "FROM ImageTable ORDER BY Time DESC");
    if (! query.exec()) {
        qWarning() << "Get Data from ImageTable failed: " << query.lastError();
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
    query.prepare( "SELECT FilePath, thumbnailPath, FileName, Dir, Time "
                   "FROM ImageTable ORDER BY Time DESC");
    if (! query.exec()) {
        qWarning() << "Get data from ImageTable failed: " << query.lastError();
        mutex.unlock();
        return infos;
    }
    else {
        using namespace utils::base;
        while (query.next()) {
            DBImgInfo info;
            info.filePath = query.value(0).toString();
            info.thumbnailPath = query.value(1).toString();
            info.imgBaseInfo.fileName = query.value(2).toString();
            info.dirHash = query.value(3).toString();
            info.imgBaseInfo.time = stringToDateTime(query.value(4).toString());
//            info.filePath = query.value(0).toString();
//            info.thumbnailPath = query.value(1).toString();
//            info.imgBaseInfo.fileName = query.value(2).toString();
//            info.imgBaseInfo.fileFormat = query.value(3).toString();
//            info.imgBaseInfo.fileSize = query.value(4).toString();
//            info.imgBaseInfo.dimension = query.value(5).toString();
//            info.imgBaseInfo.time = stringToDateTime(query.value(6).toString());
//            info.imgDetailedInfo.exposureMode = query.value(7).toString();
//            info.imgDetailedInfo.exposureProgram = query.value(8).toString();
//            info.imgDetailedInfo.exposureTime = query.value(9).toString();
//            info.imgDetailedInfo.flashLamp = query.value(10).toString();
//            info.imgDetailedInfo.apertureValue = query.value(11).toString();
//            info.imgDetailedInfo.focalLength = query.value(12).toString();
//            info.imgDetailedInfo.ISOSpeedRatings = query.value(13).toString();
//            info.imgDetailedInfo.maxApertureValue = query.value(14).toString();
//            info.imgDetailedInfo.meteringMode = query.value(15).toString();
//            info.imgDetailedInfo.whiteBalance = query.value(16).toString();
//            info.imgDetailedInfo.flashExposureComp = query.value(17).toString();
//            info.imgDetailedInfo.cameraModel = query.value(18).toString();

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
                   "FROM ImageTable ORDER BY Time DESC");
    if (! query.exec()) {
        qWarning() << "Get Data from ImageTable failed: " << query.lastError();
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
{/*
    const DBImgInfoList list = getImgInfos("Time", timeline);
    if (list.count() < 1) {
        return DBImgInfoList();
    }
    else {
        return list;
    }*/
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
    query.prepare( "SELECT COUNT(*) FROM ImageTable" );
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
    query.prepare("SELECT COUNT(*) FROM ImageTable "
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
    query.prepare( "SELECT DISTINCT FilePath FROM ImageTable "
                   "WHERE Dir=:dir " );
    query.bindValue(":dir", utils::base::hash(dir));
    if (! query.exec() ) {
        qWarning() << "Get Paths from ImageTable failed: " << query.lastError();
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
    query.prepare( "SELECT COUNT(*) FROM ImageTable WHERE FilePath = :path" );
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

    QVariantList pathhashs, filenames, filepaths, thumbnailPaths, dirs, times;

    for (DBImgInfo info : infos) {
        filenames << info.imgBaseInfo.fileName;
        filepaths << info.filePath;
        pathhashs << utils::base::hash(info.filePath);
        thumbnailPaths << info.thumbnailPath;
        dirs << info.dirHash;
        times << utils::base::timeToString(info.imgBaseInfo.time, true);
    }

    QMutexLocker mutex(&m_mutex);
    // Insert into ImageTable
    QSqlQuery query( db );
    query.setForwardOnly(true);
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    query.prepare( "REPLACE INTO ImageTable "
                   "(PathHash, FilePath, FileName, Dir, Time) VALUES (?, ?, ?, ?, ?)" );
    query.addBindValue(pathhashs);
    query.addBindValue(filepaths);
    query.addBindValue(filenames);
    query.addBindValue(dirs);
    query.addBindValue(times);
    if (! query.execBatch()) {
        qWarning() << "Insert data into ImageTable failed: "
                   << query.lastError();
        query.exec("COMMIT");
        mutex.unlock();
    }
    else {
        query.exec("COMMIT");
        mutex.unlock();
//        emit dApp->signalM->imagesInserted(infos);
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
    QString qs = "DELETE FROM AlbumTable WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove data from AlbumTable failed: "
                   << query.lastError();
        query.exec("COMMIT");
    }
    else {
        query.exec("COMMIT");
    }

    // Remove from image table
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    qs = "DELETE FROM ImageTable WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove data from ImageTable failed: "
                   << query.lastError();
        query.exec("COMMIT");
    }
    else {
//        emit dApp->signalM->imagesRemoved(infos);
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
    QString qs = "DELETE FROM AlbumTable WHERE PathHash=?";
    query.prepare(qs);
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove data from AlbumTable failed: "
                   << query.lastError();
    }
    query.exec("COMMIT");

    // Remove from image table
    query.exec("BEGIN IMMEDIATE TRANSACTION");
    qs = "DELETE FROM ImageTable WHERE Dir=:Dir";
    query.prepare(qs);
    query.bindValue(":Dir", dirHash);
    if (! query.exec()) {
        qWarning() << "Remove dir's images from ImageTable failed: "
                   << query.lastError();
        query.exec("COMMIT");
    }
    else {
        query.exec("COMMIT");
//        emit dApp->signalM->imagesRemoved(infos);
    }
    mutex.unlock();
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
    QString ps = "SELECT DISTINCT PathHash FROM AlbumTable "
                 "WHERE AlbumName =:name AND PathHash != \"%1\" ";
    query.prepare( ps.arg(EMPTY_HASH_STR) );
    query.bindValue(":name", album);
    if ( ! query.exec() ) {
        qWarning() << "Get data from AlbumTable failed: "
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
        info.endTime = getInfoByPathHash(pathHashs.first()).imgBaseInfo.time;
        info.beginTime = info.endTime;
    }
    else if (pathHashs.length() > 1) {
        //TODO: The images' info in AlbumTable need dateTime
        //If: without those, need to loop access dateTime
        foreach (QString pHash,  pathHashs) {
            QDateTime tmpTime = getInfoByPathHash(pHash).imgBaseInfo.time;
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
    query.prepare( "SELECT DISTINCT AlbumName FROM AlbumTable" );
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
                  "FROM ImageTable AS i, AlbumTable AS a "
                  "WHERE i.PathHash=a.PathHash "
                  "AND a.AlbumName=:album "
                  "AND FilePath != \" \" ");
    query.bindValue(":album", album);
    if (! query.exec() ) {
        qWarning() << "Get Paths from AlbumTable failed: " << query.lastError();
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
    query.prepare("SELECT DISTINCT i.FilePath, i.FileName, i.Dir, i.Time "
                  "FROM ImageTable AS i, AlbumTable AS a "
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
            info.imgBaseInfo.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.imgBaseInfo.time = stringToDateTime(query.value(3).toString());

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
    QString ps = "SELECT COUNT(*) FROM AlbumTable "
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
    query.prepare("SELECT COUNT(DISTINCT AlbumName) FROM AlbumTable");
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
    query.prepare( "SELECT COUNT(*) FROM AlbumTable WHERE PathHash = :hash "
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
    query.prepare( "SELECT COUNT(*) FROM AlbumTable WHERE AlbumName = :album");
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
    query.prepare("REPLACE INTO AlbumTable (AlbumId, AlbumName, PathHash) "
                  "VALUES (null, ?, ?)");
    query.addBindValue(nameRows);
    query.addBindValue(pathHashRows);
    if (! query.execBatch()) {
        qWarning() << "Insert data into AlbumTable failed: "
                   << query.lastError();
    }
    query.exec("COMMIT");

    //FIXME: Don't insert the repeated filepath into the same album
    //Delete the same data
    QString ps = "DELETE FROM AlbumTable where AlbumId NOT IN"
                 "(SELECT min(AlbumId) FROM AlbumTable GROUP BY"
                 " AlbumName, PathHash) AND PathHash != \"%1\" ";
    query.prepare(ps.arg(EMPTY_HASH_STR));
    if (!query.exec()) {
        qDebug() << "delete same date failed!";
    }
    query.exec("COMMIT");
    mutex.unlock();

//    emit dApp->signalM->insertedIntoAlbum(album, paths);
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
    query.prepare("DELETE FROM AlbumTable WHERE AlbumName=:album");
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
    QString qs("DELETE FROM AlbumTable WHERE AlbumName=\"%1\" AND PathHash=?");
    query.prepare(qs.arg(album));
    query.addBindValue(pathHashs);
    if (! query.execBatch()) {
        qWarning() << "Remove images from DB failed: " << query.lastError();
    }
    else {
//        emit dApp->signalM->removedFromAlbum(album, paths);
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
    query.prepare("UPDATE AlbumTable SET "
                  "AlbumName = :newName "
                  "WHERE AlbumName = :oldName ");
    query.bindValue( ":newName", newAlbum );
    query.bindValue( ":oldName", oldAlbum );
    if (! query.exec()) {
        qWarning() << "Update album name failed: " << query.lastError();
    }
    mutex.unlock();
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
    query.prepare(QString("SELECT FilePath, FileName, Dir, Time FROM ImageTable "
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
            info.imgBaseInfo.fileName = query.value(1).toString();
            info.dirHash = query.value(2).toString();
            info.imgBaseInfo.time = stringToDateTime(query.value(3).toString());

            infos << info;
        }
    }
    mutex.unlock();
    return infos;
}
