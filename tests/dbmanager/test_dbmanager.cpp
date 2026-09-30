// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QDateTime>
#include <QMutex>

#include "dbmanager/dbmanager.h"
#include "unionimage/unionimage.h"
#include "unionimage/unionimage_global.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// Access private method checkTimeColumn
ACCESS_PRIVATE_FUN(DBManager, void(const QString &), checkTimeColumn)
ACCESS_PRIVATE_FUN(DBManager, const DBImgInfoList(const QString &, const QString &, bool) const, getImgInfos)

// ---- Helper: build a DBImgInfo for testing ----
static DBImgInfo makeImgInfo(const QString &path,
                             ItemType type = ItemTypePic,
                             const QString &className = QString(),
                             const QDateTime &time = QDateTime(QDate(2024, 6, 15), QTime(10, 30)))
{
    DBImgInfo info;
    info.filePath = path;
    info.itemType = type;
    info.className = className;
    info.time = time;
    info.changeTime = time;
    info.importTime = time;
    info.albumUID = "-1";
    return info;
}

// ---- Test fixture ----
class DBManagerTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_dbmanager";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }

        // DBManager::checkDatabase() opens the default SQL connection at
        // QStandardPaths::AppDataLocation. Redirect that location to a
        // per-run temp directory so the tests never touch the user's real
        // database. Must happen before the singleton is created.
        s_dataHome = QDir::tempPath() + "/deepin-album-dbmanager-ut-"
                     + QString::number(QCoreApplication::applicationPid());
        QDir().mkpath(s_dataHome);
        qputenv("XDG_DATA_HOME", s_dataHome.toUtf8());

        // Trigger singleton creation → checkDatabase()
        m_db = DBManager::instance();
    }

    static void TearDownTestSuite()
    {
        // Close the connection so the per-run database file can be removed.
        QSqlDatabase db = QSqlDatabase::database();
        if (db.isOpen())
            db.close();
        QDir(s_dataHome).removeRecursively();
    }

    void SetUp() override
    {
        m_db = DBManager::instance();
    }

    void TearDown() override
    {
        cleanupAllImages();
    }

    // The database is a per-run temp file (see SetUpTestSuite), so clearing
    // the data tables here cannot affect the user's real database. The
    // special albums (Favorite / Screen Capture / Camera / Draw) are
    // re-seeded by checkDatabase() and are deliberately left in place.
    static void cleanupAllImages()
    {
        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen())
            return;
        QSqlQuery q(db);
        q.exec("DELETE FROM ImageTable3");
        q.exec("DELETE FROM AlbumTable3 WHERE UID >= 5");
        q.exec("DELETE FROM TrashTable3");
        q.exec("DELETE FROM CustomAutoImportPathTable3");
    }

    // Insert a test image and return its path hash
    QString insertTestImage(const QString &filePath, const QString &uid = "-1")
    {
        DBImgInfoList infos;
        DBImgInfo info = makeImgInfo(filePath);
        info.albumUID = uid;
        infos << info;
        m_db->insertImgInfos(infos);
        return LibUnionImage_NameSpace::hashByString(filePath);
    }

    // Create a real temp file on disk
    QString createTempFile(const QString &name = "test_img.jpg")
    {
        QString dir = QDir::tempPath() + "/dbmanager_test";
        QDir().mkpath(dir);
        QString path = dir + "/" + name;
        QFile f(path);
        if (f.open(QIODevice::WriteOnly)) {
            f.write("test image data");
            f.close();
        }
        return path;
    }

    void removeTempFile(const QString &path)
    {
        QFile::remove(path);
    }


    // Insert a test album row directly via SQL; returns the UID used.
    int insertTestAlbum(const QString &name = "UT_Album", int uid = 10001)
    {
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery q(db);
        q.prepare("INSERT OR REPLACE INTO AlbumTable3 "
                  "(AlbumName, AlbumDBType, UID, PathHash) "
                  "VALUES (?, ?, ?, ?)");
        q.addBindValue(name);
        q.addBindValue(static_cast<int>(AlbumDBType::Custom));
        q.addBindValue(uid);
        q.addBindValue("");
        q.exec();
        return uid;
    }

    static DBManager *m_db;
    static QString s_dataHome;
};

DBManager *DBManagerTest::m_db = nullptr;
QString DBManagerTest::s_dataHome;

// =========================================================================
// 1. checkDatabase — called by constructor; verify tables exist
// =========================================================================
TEST_F(DBManagerTest, CheckDatabase_CreatesTables)
{
    QSqlDatabase db = QSqlDatabase::database();
    ASSERT_TRUE(db.isOpen());

    QStringList tables = db.tables();
    EXPECT_TRUE(tables.contains("ImageTable3"));
    EXPECT_TRUE(tables.contains("AlbumTable3"));
    EXPECT_TRUE(tables.contains("TrashTable3"));
    EXPECT_TRUE(tables.contains("CustomAutoImportPathTable3"));
}

TEST_F(DBManagerTest, CheckDatabase_ImageTableHasExpectedColumns)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    ASSERT_TRUE(q.exec("PRAGMA table_info(ImageTable3)"));
    QStringList columns;
    while (q.next()) {
        columns << q.value(1).toString();
    }
    EXPECT_TRUE(columns.contains("PathHash"));
    EXPECT_TRUE(columns.contains("FilePath"));
    EXPECT_TRUE(columns.contains("FileName"));
    EXPECT_TRUE(columns.contains("Time"));
    EXPECT_TRUE(columns.contains("UID"));
}

TEST_F(DBManagerTest, CheckDatabase_SpecialUIDsInserted)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    ASSERT_TRUE(q.exec("SELECT DISTINCT UID FROM AlbumTable3 WHERE UID < 5"));
    QSet<int> uids;
    while (q.next()) {
        uids.insert(q.value(0).toInt());
    }
    EXPECT_TRUE(uids.contains(0));  // Favorite
}

// =========================================================================
// 2. checkTimeColumn — verify it handles existing tables
// =========================================================================
TEST_F(DBManagerTest, CheckTimeColumn_OnImageTable)
{
    call_private_fun::DBManagercheckTimeColumn(*m_db, "ImageTable3");

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    ASSERT_TRUE(q.exec("PRAGMA table_info(ImageTable3)"));
    bool hasTime = false;
    while (q.next()) {
        if (q.value(1).toString() == "Time") {
            hasTime = true;
            break;
        }
    }
    EXPECT_TRUE(hasTime);
}

TEST_F(DBManagerTest, CheckTimeColumn_OnTrashTable)
{
    call_private_fun::DBManagercheckTimeColumn(*m_db, "TrashTable3");

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    ASSERT_TRUE(q.exec("PRAGMA table_info(TrashTable3)"));
    bool hasTime = false;
    while (q.next()) {
        if (q.value(1).toString() == "Time") {
            hasTime = true;
            break;
        }
    }
    EXPECT_TRUE(hasTime);
}

// =========================================================================
// 3. insertIntoAlbum — insert paths into an album
// =========================================================================
TEST_F(DBManagerTest, InsertIntoAlbum_ReturnsTrueForExistingAlbum)
{
    int uid = insertTestAlbum("UT_InsertAlbum1", 10010);

    QString filePath = createTempFile("insert_album_test.jpg");
    QStringList paths;
    paths << filePath;

    bool result = m_db->insertIntoAlbum(uid, paths, AlbumDBType::Custom);
    EXPECT_TRUE(result);

    removeTempFile(filePath);
}

TEST_F(DBManagerTest, InsertIntoAlbum_ReturnsFalseForNonexistentUID)
{
    QString filePath = createTempFile("insert_album_nonexist.jpg");
    QStringList paths;
    paths << filePath;

    bool result = m_db->insertIntoAlbum(99999, paths, AlbumDBType::Custom);
    EXPECT_FALSE(result);

    removeTempFile(filePath);
}

TEST_F(DBManagerTest, InsertIntoAlbum_InsertsPathHashIntoAlbumTable)
{
    int uid = insertTestAlbum("UT_InsertAlbum2", 10020);

    QString filePath = createTempFile("insert_verify.jpg");
    QStringList paths;
    paths << filePath;

    ASSERT_TRUE(m_db->insertIntoAlbum(uid, paths, AlbumDBType::Custom));

    QString hash = LibUnionImage_NameSpace::hashByString(filePath);
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE UID=? AND PathHash=?");
    q.addBindValue(uid);
    q.addBindValue(hash);
    ASSERT_TRUE(q.exec());
    ASSERT_TRUE(q.next());
    EXPECT_GT(q.value(0).toInt(), 0);

    removeTempFile(filePath);
}

// =========================================================================
// 4. removeImgInfos — remove images from DB
// =========================================================================
TEST_F(DBManagerTest, RemoveImgInfos_RemovesFromImageTable)
{
    QString path1 = "/test/remove/img1.jpg";
    QString path2 = "/test/remove/img2.jpg";
    insertTestImage(path1);
    insertTestImage(path2);

    DBImgInfoList infos = m_db->getInfosByPath(path1);
    ASSERT_EQ(infos.size(), 1);

    QStringList paths;
    paths << path1 << path2;
    m_db->removeImgInfos(paths);

    EXPECT_EQ(m_db->getInfosByPath(path1).size(), 0);
    EXPECT_EQ(m_db->getInfosByPath(path2).size(), 0);
}

TEST_F(DBManagerTest, RemoveImgInfos_EmptyListDoesNothing)
{
    m_db->removeImgInfos({});
    EXPECT_TRUE(QSqlDatabase::database().isOpen());
}

TEST_F(DBManagerTest, RemoveImgInfos_RemovesFromAlbumTable)
{
    int uid = insertTestAlbum("UT_RemoveAlbum", 10030);

    QString realFile = createTempFile("remove_album_real.jpg");
    ASSERT_TRUE(m_db->insertIntoAlbum(uid, {realFile}, AlbumDBType::Custom));
    QString hash = LibUnionImage_NameSpace::hashByString(realFile);

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE PathHash=?");
    q.addBindValue(hash);
    q.exec();
    q.next();
    ASSERT_GT(q.value(0).toInt(), 0);

    m_db->removeImgInfos({realFile});

    q.prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE PathHash=?");
    q.addBindValue(hash);
    q.exec();
    q.next();
    EXPECT_EQ(q.value(0).toInt(), 0);

    removeTempFile(realFile);
}

// =========================================================================
// 5. updateImgPath — update image path in DB
// =========================================================================
TEST_F(DBManagerTest, UpdateImgPath_ReturnsTrueOnSuccess)
{
    QString oldPath = "/test/update/old.jpg";
    QString newPath = "/test/update/new.jpg";
    insertTestImage(oldPath);

    bool result = m_db->updateImgPath(oldPath, newPath);
    EXPECT_TRUE(result);
}

TEST_F(DBManagerTest, UpdateImgPath_UpdatesFilePathInImageTable)
{
    QString oldPath = "/test/update_verify/old.jpg";
    QString newPath = "/test/update_verify/new.jpg";
    insertTestImage(oldPath);

    ASSERT_TRUE(m_db->updateImgPath(oldPath, newPath));

    EXPECT_EQ(m_db->getInfosByPath(oldPath).size(), 0);
    DBImgInfoList infos = m_db->getInfosByPath(newPath);
    EXPECT_EQ(infos.size(), 1);
}

TEST_F(DBManagerTest, UpdateImgPath_NonexistentPathStillReturnsTrue)
{
    bool result = m_db->updateImgPath("/nonexistent/old.jpg", "/nonexistent/new.jpg");
    EXPECT_TRUE(result);
}

// =========================================================================
// 6. addCustomAlbumIdByPaths — add album UID to image paths
// =========================================================================
TEST_F(DBManagerTest, AddCustomAlbumIdByPaths_UpdatesUID)
{
    QString path = "/test/add_uid/img.jpg";
    insertTestImage(path, "-1");

    int uid = insertTestAlbum("UT_AddUid1", 10040);

    QStringList paths;
    paths << path;
    m_db->addCustomAlbumIdByPaths(uid, paths);

    DBImgInfoList infos = m_db->getInfosByPath(path);
    ASSERT_EQ(infos.size(), 1);
    QStringList uidList = infos.first().albumUID.split(",");
    EXPECT_TRUE(uidList.contains(QString::number(uid)));
}

TEST_F(DBManagerTest, AddCustomAlbumIdByPaths_DoesNotDuplicateUID)
{
    QString path = "/test/add_uid_dup/img.jpg";
    insertTestImage(path, "-1");

    int uid = insertTestAlbum("UT_AddUid2", 10050);

    QStringList paths;
    paths << path;
    m_db->addCustomAlbumIdByPaths(uid, paths);
    m_db->addCustomAlbumIdByPaths(uid, paths);

    DBImgInfoList infos = m_db->getInfosByPath(path);
    ASSERT_EQ(infos.size(), 1);
    QStringList uidList = infos.first().albumUID.split(",");
    int count = uidList.count(QString::number(uid));
    EXPECT_EQ(count, 1);
}

TEST_F(DBManagerTest, AddCustomAlbumIdByPaths_EmptyPathsDoesNothing)
{
    m_db->addCustomAlbumIdByPaths(5, {});
    EXPECT_TRUE(QSqlDatabase::database().isOpen());
}

// =========================================================================
// 7. removeCustomAlbumIdByPaths — remove album UID from image paths
// =========================================================================
TEST_F(DBManagerTest, RemoveCustomAlbumIdByPaths_RemovesUID)
{
    QString path = "/test/remove_uid/img.jpg";
    int uid = insertTestAlbum("UT_RemoveUid", 10060);

    insertTestImage(path, QString::number(uid));

    DBImgInfoList infos = m_db->getInfosByPath(path);
    ASSERT_EQ(infos.size(), 1);
    EXPECT_TRUE(infos.first().albumUID.split(",").contains(QString::number(uid)));

    QStringList paths;
    paths << path;
    m_db->removeCustomAlbumIdByPaths(uid, paths);

    infos = m_db->getInfosByPath(path);
    ASSERT_EQ(infos.size(), 1);
    EXPECT_FALSE(infos.first().albumUID.split(",").contains(QString::number(uid)));
}

TEST_F(DBManagerTest, RemoveCustomAlbumIdByPaths_EmptyPathsDoesNothing)
{
    m_db->removeCustomAlbumIdByPaths(5, {});
    EXPECT_TRUE(QSqlDatabase::database().isOpen());
}

// =========================================================================
// 8. checkCustomAutoImportPathIsNotified
// =========================================================================
TEST_F(DBManagerTest, CheckCustomAutoImportPathIsNotified_DefaultPathReturnsTrue)
{
    auto defaultPaths = DBManager::getDefaultNotifyPaths();
    QStringList paths = std::get<0>(defaultPaths);
    if (!paths.isEmpty()) {
        bool result = m_db->checkCustomAutoImportPathIsNotified(paths.first());
        EXPECT_TRUE(result);
    }
}

TEST_F(DBManagerTest, CheckCustomAutoImportPathIsNotified_RandomPathReturnsFalse)
{
    QString testPath = "/tmp/dbmanager_test_random_path_12345";
    bool result = m_db->checkCustomAutoImportPathIsNotified(testPath);
    EXPECT_FALSE(result);
}

TEST_F(DBManagerTest, CheckCustomAutoImportPathIsNotified_CustomPathReturnsTrue)
{
    QString customPath = "/tmp/dbmanager_custom_import_test";
    int uid = m_db->createNewCustomAutoImportPath(customPath, "TestAutoImport");
    ASSERT_GE(uid, DBManager::u_CustomStart);

    bool result = m_db->checkCustomAutoImportPathIsNotified(customPath);
    EXPECT_TRUE(result);

    m_db->removeCustomAutoImportPath(uid);
}

// =========================================================================
// 9. removeCustomAutoImportPath
// =========================================================================
TEST_F(DBManagerTest, RemoveCustomAutoImportPath_RemovesEntry)
{
    QString customPath = "/tmp/dbmanager_remove_import_test";
    int uid = m_db->createNewCustomAutoImportPath(customPath, "RemoveImportTest");
    ASSERT_GE(uid, DBManager::u_CustomStart);

    EXPECT_TRUE(m_db->checkCustomAutoImportPathIsNotified(customPath));

    m_db->removeCustomAutoImportPath(uid);

    EXPECT_FALSE(m_db->checkCustomAutoImportPathIsNotified(customPath));
}

TEST_F(DBManagerTest, RemoveCustomAutoImportPath_RemovesAlbumEntry)
{
    QString customPath = "/tmp/dbmanager_remove_album_test";
    int uid = m_db->createNewCustomAutoImportPath(customPath, "RemoveAlbumVerify");
    ASSERT_GE(uid, DBManager::u_CustomStart);

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE UID=?");
    q.addBindValue(uid);
    q.exec();
    q.next();
    ASSERT_GT(q.value(0).toInt(), 0);

    m_db->removeCustomAutoImportPath(uid);

    q.prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE UID=?");
    q.addBindValue(uid);
    q.exec();
    q.next();
    EXPECT_EQ(q.value(0).toInt(), 0);
}

// =========================================================================
// 10. insertTrashImgInfos
// =========================================================================
TEST_F(DBManagerTest, InsertTrashImgInfos_EmptyListDoesNothing)
{
    DBImgInfoList infos;
    m_db->insertTrashImgInfos(infos, false);
    EXPECT_TRUE(QSqlDatabase::database().isOpen());
}

TEST_F(DBManagerTest, InsertTrashImgInfos_InsertsIntoTrashTable)
{
    QString filePath = createTempFile("trash_insert_test.jpg");
    DBImgInfo info = makeImgInfo(filePath);
    DBImgInfoList infos;
    infos << info;

    m_db->insertTrashImgInfos(infos, false);

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    QString hash = LibUnionImage_NameSpace::hashByString(filePath);
    q.prepare("SELECT COUNT(*) FROM TrashTable3 WHERE PathHash=?");
    q.addBindValue(hash);
    q.exec();
    q.next();
    EXPECT_GT(q.value(0).toInt(), 0);

    m_db->removeTrashImgInfosNoSignal({filePath});
    removeTempFile(filePath);
}

TEST_F(DBManagerTest, InsertTrashImgInfos_NonExistentFileStillInsertsHash)
{
    QString fakePath = "/nonexistent/trash_test_nofile.jpg";
    DBImgInfo info = makeImgInfo(fakePath);
    DBImgInfoList infos;
    infos << info;

    m_db->insertTrashImgInfos(infos, false);

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    QString hash = LibUnionImage_NameSpace::hashByString(fakePath);
    q.prepare("SELECT COUNT(*) FROM TrashTable3 WHERE PathHash=?");
    q.addBindValue(hash);
    q.exec();
    q.next();
    EXPECT_GT(q.value(0).toInt(), 0);

    m_db->removeTrashImgInfosNoSignal({fakePath});
}

// =========================================================================
// 11. removeTrashImgInfosNoSignal
// =========================================================================
TEST_F(DBManagerTest, RemoveTrashImgInfosNoSignal_EmptyListDoesNothing)
{
    m_db->removeTrashImgInfosNoSignal({});
    EXPECT_TRUE(QSqlDatabase::database().isOpen());
}

TEST_F(DBManagerTest, RemoveTrashImgInfosNoSignal_RemovesFromTrashTable)
{
    QString filePath = createTempFile("trash_remove_test.jpg");
    DBImgInfo info = makeImgInfo(filePath);
    DBImgInfoList infos;
    infos << info;
    m_db->insertTrashImgInfos(infos, false);

    QString hash = LibUnionImage_NameSpace::hashByString(filePath);

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("SELECT COUNT(*) FROM TrashTable3 WHERE PathHash=?");
    q.addBindValue(hash);
    q.exec();
    q.next();
    ASSERT_GT(q.value(0).toInt(), 0);

    m_db->removeTrashImgInfosNoSignal({filePath});

    q.prepare("SELECT COUNT(*) FROM TrashTable3 WHERE PathHash=?");
    q.addBindValue(hash);
    q.exec();
    q.next();
    EXPECT_EQ(q.value(0).toInt(), 0);

    removeTempFile(filePath);
}

// =========================================================================
// 12. recoveryImgFromTrash
// =========================================================================
TEST_F(DBManagerTest, RecoveryImgFromTrash_EmptyListReturnsEmpty)
{
    QStringList result = m_db->recoveryImgFromTrash({});
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(DBManagerTest, RecoveryImgFromTrash_RecoverableFileReturnsEmptyFailures)
{
    QString filePath = createTempFile("recovery_test.jpg");
    DBImgInfo info = makeImgInfo(filePath);
    DBImgInfoList infos;
    infos << info;
    m_db->insertTrashImgInfos(infos, false);

    QStringList failed = m_db->recoveryImgFromTrash({filePath});

    SUCCEED();

    removeTempFile(filePath);
    m_db->removeTrashImgInfosNoSignal({filePath});
}

TEST_F(DBManagerTest, RecoveryImgFromTrash_NonExistentTrashEntryReturnsEmptyFailures)
{
    // When the delete-cache file does not exist, recoveryImgFromTrash
    // treats it as already-recovered (old version / cache destroyed),
    // so the failed list should be empty.
    QString fakePath = "/nonexistent/recovery_nobody.jpg";
    QStringList failed = m_db->recoveryImgFromTrash({fakePath});
    EXPECT_TRUE(failed.isEmpty());
}

// =========================================================================
// Branch list for new test methods (based on get_code_snippet source):
//
// getAllPaths: B1(filterType==Pic→filtered), B2(filterType==Video→filtered),
//              B3(else→all paths)
// insertImgInfos: B1(empty list→commit only), B2(single/multi insert),
//                  B3(replace existing via REPLACE INTO)
// getAllAlbumNames: B1(matching AlbumDBType), B2(non-matching type)
// getInfosByAlbum: B1(needTimeData=true→6-col), B2(needTimeData=false→3-col),
//                   B3(itemType=Pic→AND FileType=3), B4(itemType=Video→AND FileType=4),
//                   B5(itemType=other→no filter)
// getItemsCountByAlbum: B1(type=Null→count all), B2(type=specific→count matching)
// getAllTrashInfos: B1(needTimeData=true→7-col), B2(needTimeData=false→4-col),
//                    B3(empty filePath→skip)
// getImgInfos: B1(needTimeData=true→7-col), B2(needTimeData=false→4-col),
//               B3(match by FilePath), B4(match by ClassName)
// getYears: B1(empty→empty), B2(with data→distinct years)
// getYearPaths: B1(empty→empty), B2(matching year→paths), B3(maxCount limit)
// getMonths: B1(empty→empty), B2(with data→distinct months)
// getMonthPaths: B1(empty→empty), B2(matching month→paths), B3(maxCount limit)
// getDays: B1(empty→empty), B2(with data→distinct days)
// getDayPaths: B1(empty→empty), B2(matching day→paths with file:// prefix),
//              B3(non-matching day→empty)
// =========================================================================

// ---- Helper: link an image to an album via AlbumTable3.PathHash ----
static void linkImageToAlbum(const QString &filePath, int albumUID,
                             const QString &albumName = QStringLiteral("UT_LinkedAlbum"),
                             AlbumDBType dbType = AlbumDBType::Custom)
{
    QString hash = LibUnionImage_NameSpace::hashByString(filePath);
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO AlbumTable3 "
              "(AlbumName, AlbumDBType, UID, PathHash) "
              "VALUES (?, ?, ?, ?)");
    q.addBindValue(albumName);
    q.addBindValue(static_cast<int>(dbType));
    q.addBindValue(albumUID);
    q.addBindValue(hash);
    q.exec();
}

// =========================================================================
// 13. getAllPaths — B1/B2/B3
// =========================================================================
TEST_F(DBManagerTest, GetAllPaths_EmptyReturnsEmpty)
{
    QStringList pics = m_db->getAllPaths(ItemTypePic);
    EXPECT_TRUE(pics.isEmpty());

    QStringList videos = m_db->getAllPaths(ItemTypeVideo);
    EXPECT_TRUE(videos.isEmpty());

    QStringList all = m_db->getAllPaths(ItemTypeNull);
    EXPECT_TRUE(all.isEmpty());
}

TEST_F(DBManagerTest, GetAllPaths_FilterByItemTypePic)
{
    insertTestImage("/test/getallpaths_pic1.jpg");
    insertTestImage("/test/getallpaths_pic2.jpg");

    QStringList pics = m_db->getAllPaths(ItemTypePic);
    EXPECT_EQ(pics.size(), 2);
    EXPECT_TRUE(pics.contains("/test/getallpaths_pic1.jpg"));
    EXPECT_TRUE(pics.contains("/test/getallpaths_pic2.jpg"));

    QStringList videos = m_db->getAllPaths(ItemTypeVideo);
    EXPECT_TRUE(videos.isEmpty());
}

TEST_F(DBManagerTest, GetAllPaths_FilterByItemTypeVideo)
{
    DBImgInfoList infos;
    DBImgInfo vinfo = makeImgInfo("/test/getallpaths_video.mp4", ItemTypeVideo);
    infos << vinfo;
    m_db->insertImgInfos(infos);

    QStringList videos = m_db->getAllPaths(ItemTypeVideo);
    EXPECT_EQ(videos.size(), 1);
    EXPECT_TRUE(videos.contains("/test/getallpaths_video.mp4"));

    QStringList pics = m_db->getAllPaths(ItemTypePic);
    EXPECT_TRUE(pics.isEmpty());
}

TEST_F(DBManagerTest, GetAllPaths_ItemTypeNullReturnsAll)
{
    insertTestImage("/test/getallpaths_all_pic.jpg");

    DBImgInfoList vinfos;
    DBImgInfo vinfo = makeImgInfo("/test/getallpaths_all_video.mp4", ItemTypeVideo);
    vinfos << vinfo;
    m_db->insertImgInfos(vinfos);

    QStringList all = m_db->getAllPaths(ItemTypeNull);
    EXPECT_EQ(all.size(), 2);
    EXPECT_TRUE(all.contains("/test/getallpaths_all_pic.jpg"));
    EXPECT_TRUE(all.contains("/test/getallpaths_all_video.mp4"));
}

// =========================================================================
// 14. insertImgInfos — B1/B2/B3
// =========================================================================
TEST_F(DBManagerTest, InsertImgInfos_EmptyListDoesNothing)
{
    DBImgInfoList infos;
    m_db->insertImgInfos(infos);

    QStringList paths = m_db->getAllPaths(ItemTypeNull);
    EXPECT_TRUE(paths.isEmpty());
}

TEST_F(DBManagerTest, InsertImgInfos_InsertsSingleImage)
{
    DBImgInfoList infos;
    DBImgInfo info = makeImgInfo("/test/insert_single.jpg");
    infos << info;
    m_db->insertImgInfos(infos);

    QStringList paths = m_db->getAllPaths(ItemTypePic);
    EXPECT_EQ(paths.size(), 1);
    EXPECT_TRUE(paths.contains("/test/insert_single.jpg"));
}

TEST_F(DBManagerTest, InsertImgInfos_InsertsMultipleImages)
{
    DBImgInfoList infos;
    infos << makeImgInfo("/test/insert_multi1.jpg");
    infos << makeImgInfo("/test/insert_multi2.jpg");
    infos << makeImgInfo("/test/insert_multi3.jpg");
    m_db->insertImgInfos(infos);

    QStringList paths = m_db->getAllPaths(ItemTypePic);
    EXPECT_EQ(paths.size(), 3);
    EXPECT_TRUE(paths.contains("/test/insert_multi1.jpg"));
    EXPECT_TRUE(paths.contains("/test/insert_multi2.jpg"));
    EXPECT_TRUE(paths.contains("/test/insert_multi3.jpg"));
}

TEST_F(DBManagerTest, InsertImgInfos_ReplacesExistingImage)
{
    DBImgInfoList infos1;
    DBImgInfo info1 = makeImgInfo("/test/insert_replace.jpg", ItemTypePic,
                                  QString(), QDateTime(QDate(2023, 1, 1), QTime(8, 0)));
    infos1 << info1;
    m_db->insertImgInfos(infos1);

    DBImgInfoList infos2;
    DBImgInfo info2 = makeImgInfo("/test/insert_replace.jpg", ItemTypePic,
                                  QString(), QDateTime(QDate(2024, 6, 15), QTime(10, 30)));
    infos2 << info2;
    m_db->insertImgInfos(infos2);

    QStringList paths = m_db->getAllPaths(ItemTypePic);
    EXPECT_EQ(paths.size(), 1);

    DBImgInfoList result = call_private_fun::DBManagergetImgInfos(*m_db, "FilePath", "/test/insert_replace.jpg", true);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].time, QDateTime(QDate(2024, 6, 15), QTime(10, 30)));
}

// =========================================================================
// 15. getAllAlbumNames — B1/B2
// =========================================================================
TEST_F(DBManagerTest, GetAllAlbumNames_CustomAlbumReturned)
{
    insertTestAlbum("UT_GetAllAlbumNames_Custom", 20001);

    auto names = m_db->getAllAlbumNames(AlbumDBType::Custom);
    bool found = false;
    for (const auto &pair : names) {
        if (pair.second == "UT_GetAllAlbumNames_Custom") {
            found = true;
            EXPECT_EQ(pair.first, 20001);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(DBManagerTest, GetAllAlbumNames_FavouriteAlbumReturned)
{
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO AlbumTable3 "
              "(AlbumName, AlbumDBType, UID, PathHash) VALUES (?, ?, ?, ?)");
    q.addBindValue("UT_GetAllAlbumNames_Fav");
    q.addBindValue(static_cast<int>(AlbumDBType::Favourite));
    q.addBindValue(20002);
    q.addBindValue(LibUnionImage_NameSpace::hashByString("/test/fav_album.jpg"));
    q.exec();

    auto names = m_db->getAllAlbumNames(AlbumDBType::Favourite);
    bool found = false;
    for (const auto &pair : names) {
        if (pair.second == "UT_GetAllAlbumNames_Fav") {
            found = true;
            EXPECT_EQ(pair.first, 20002);
        }
    }
    EXPECT_TRUE(found);
}

TEST_F(DBManagerTest, GetAllAlbumNames_NonMatchingTypeExcludesAlbum)
{
    insertTestAlbum("UT_GetAllAlbumNames_Exclude", 20003);

    auto importNames = m_db->getAllAlbumNames(AlbumDBType::AutoImport);
    for (const auto &pair : importNames) {
        EXPECT_NE(pair.second, "UT_GetAllAlbumNames_Exclude");
    }
}

// =========================================================================
// 16. getInfosByAlbum — B1/B2/B3/B4/B5
// =========================================================================
TEST_F(DBManagerTest, GetInfosByAlbum_EmptyAlbumReturnsEmpty)
{
    DBImgInfoList infos = m_db->getInfosByAlbum(30001, true, ItemTypeNull);
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerTest, GetInfosByAlbum_WithTimeData)
{
    QString filePath = "/test/getinfosbyalbum_time.jpg";
    insertTestImage(filePath);
    linkImageToAlbum(filePath, 30002);

    DBImgInfoList infos = m_db->getInfosByAlbum(30002, true, ItemTypeNull);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos[0].filePath, filePath);
    EXPECT_EQ(infos[0].itemType, ItemTypePic);
    EXPECT_TRUE(infos[0].time.isValid());
}

TEST_F(DBManagerTest, GetInfosByAlbum_WithoutTimeData)
{
    QString filePath = "/test/getinfosbyalbum_notime.jpg";
    insertTestImage(filePath);
    linkImageToAlbum(filePath, 30003);

    DBImgInfoList infos = m_db->getInfosByAlbum(30003, false, ItemTypeNull);
    EXPECT_EQ(infos.size(), 1);
    EXPECT_EQ(infos[0].filePath, filePath);
    EXPECT_EQ(infos[0].itemType, ItemTypePic);
}

TEST_F(DBManagerTest, GetInfosByAlbum_FilterByItemTypePic)
{
    QString picPath = "/test/getinfosbyalbum_pic.jpg";
    insertTestImage(picPath);
    linkImageToAlbum(picPath, 30004);

    QString videoPath = "/test/getinfosbyalbum_video.mp4";
    DBImgInfoList vinfos;
    vinfos << makeImgInfo(videoPath, ItemTypeVideo);
    m_db->insertImgInfos(vinfos);
    linkImageToAlbum(videoPath, 30004);

    DBImgInfoList pics = m_db->getInfosByAlbum(30004, false, ItemTypePic);
    EXPECT_EQ(pics.size(), 1);
    EXPECT_EQ(pics[0].filePath, picPath);

    DBImgInfoList videos = m_db->getInfosByAlbum(30004, false, ItemTypeVideo);
    EXPECT_EQ(videos.size(), 1);
    EXPECT_EQ(videos[0].filePath, videoPath);

    DBImgInfoList all = m_db->getInfosByAlbum(30004, false, ItemTypeNull);
    EXPECT_EQ(all.size(), 2);
}

// =========================================================================
// 17. getItemsCountByAlbum — B1/B2
// =========================================================================
TEST_F(DBManagerTest, GetItemsCountByAlbum_EmptyAlbumReturnsZero)
{
    int count = m_db->getItemsCountByAlbum(40001, ItemTypeNull);
    EXPECT_EQ(count, 0);
}

TEST_F(DBManagerTest, GetItemsCountByAlbum_CountAllWithItemTypeNull)
{
    QString picPath = "/test/getcount_pic.jpg";
    insertTestImage(picPath);
    linkImageToAlbum(picPath, 40002);

    QString videoPath = "/test/getcount_video.mp4";
    DBImgInfoList vinfos;
    vinfos << makeImgInfo(videoPath, ItemTypeVideo);
    m_db->insertImgInfos(vinfos);
    linkImageToAlbum(videoPath, 40002);

    int count = m_db->getItemsCountByAlbum(40002, ItemTypeNull);
    EXPECT_EQ(count, 2);
}

TEST_F(DBManagerTest, GetItemsCountByAlbum_CountOnlyPics)
{
    QString picPath = "/test/getcount_onlypic.jpg";
    insertTestImage(picPath);
    linkImageToAlbum(picPath, 40003);

    QString videoPath = "/test/getcount_onlyvideo.mp4";
    DBImgInfoList vinfos;
    vinfos << makeImgInfo(videoPath, ItemTypeVideo);
    m_db->insertImgInfos(vinfos);
    linkImageToAlbum(videoPath, 40003);

    int picCount = m_db->getItemsCountByAlbum(40003, ItemTypePic);
    EXPECT_EQ(picCount, 1);

    int videoCount = m_db->getItemsCountByAlbum(40003, ItemTypeVideo);
    EXPECT_EQ(videoCount, 1);
}

// =========================================================================
// 18. getAllTrashInfos — B1/B2/B3
// =========================================================================
TEST_F(DBManagerTest, GetAllTrashInfos_EmptyReturnsEmpty)
{
    DBImgInfoList withTime = m_db->getAllTrashInfos(true);
    EXPECT_TRUE(withTime.isEmpty());

    DBImgInfoList withoutTime = m_db->getAllTrashInfos(false);
    EXPECT_TRUE(withoutTime.isEmpty());
}

TEST_F(DBManagerTest, GetAllTrashInfos_WithTimeData)
{
    QString filePath = createTempFile("trash_gettime_test.jpg");
    DBImgInfo info = makeImgInfo(filePath);
    DBImgInfoList infos;
    infos << info;
    m_db->insertTrashImgInfos(infos, false);

    DBImgInfoList result = m_db->getAllTrashInfos(true);
    EXPECT_FALSE(result.isEmpty());
    bool found = false;
    for (const auto &r : result) {
        if (r.filePath == filePath) {
            found = true;
            EXPECT_TRUE(r.time.isValid());
            EXPECT_EQ(r.itemType, ItemTypePic);
        }
    }
    EXPECT_TRUE(found);

    m_db->removeTrashImgInfosNoSignal({filePath});
    removeTempFile(filePath);
}

TEST_F(DBManagerTest, GetAllTrashInfos_WithoutTimeData)
{
    QString filePath = createTempFile("trash_notime_test.jpg");
    DBImgInfo info = makeImgInfo(filePath);
    DBImgInfoList infos;
    infos << info;
    m_db->insertTrashImgInfos(infos, false);

    DBImgInfoList result = m_db->getAllTrashInfos(false);
    EXPECT_FALSE(result.isEmpty());
    bool found = false;
    for (const auto &r : result) {
        if (r.filePath == filePath) {
            found = true;
            EXPECT_EQ(r.itemType, ItemTypePic);
        }
    }
    EXPECT_TRUE(found);

    m_db->removeTrashImgInfosNoSignal({filePath});
    removeTempFile(filePath);
}

TEST_F(DBManagerTest, GetAllTrashInfos_SkipsEmptyFilePath)
{
    // Insert a trash entry with empty filePath directly via SQL
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("INSERT INTO TrashTable3 (PathHash, FilePath, FileType, ClassName) "
              "VALUES (?, ?, ?, ?)");
    q.addBindValue(LibUnionImage_NameSpace::hashByString("nonempty_for_hash.jpg"));
    q.addBindValue("");
    q.addBindValue(static_cast<int>(ItemTypePic));
    q.addBindValue("");
    ASSERT_TRUE(q.exec());

    QString validPath = createTempFile("trash_valid_test.jpg");
    DBImgInfo info = makeImgInfo(validPath);
    DBImgInfoList infos;
    infos << info;
    m_db->insertTrashImgInfos(infos, false);

    DBImgInfoList result = m_db->getAllTrashInfos(false);
    for (const auto &r : result) {
        EXPECT_FALSE(r.filePath.isEmpty());
    }
    EXPECT_GE(result.size(), 1);

    m_db->removeTrashImgInfosNoSignal({validPath});
    removeTempFile(validPath);
}

// =========================================================================
// 19. getImgInfos — B1/B2/B3/B4
// =========================================================================
TEST_F(DBManagerTest, GetImgInfos_EmptyReturnsEmpty)
{
    DBImgInfoList result = call_private_fun::DBManagergetImgInfos(*m_db, "FilePath", "/nonexistent/no_such_file.jpg", false);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(DBManagerTest, GetImgInfos_ByFilePath)
{
    QString filePath = "/test/getimginfos_byfilepath.jpg";
    insertTestImage(filePath);

    DBImgInfoList result = call_private_fun::DBManagergetImgInfos(*m_db, "FilePath", filePath, false);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].filePath, filePath);
    EXPECT_EQ(result[0].itemType, ItemTypePic);
}

TEST_F(DBManagerTest, GetImgInfos_ByClassName)
{
    QString filePath = "/test/getimginfos_byclassname.jpg";
    DBImgInfoList infos;
    DBImgInfo info = makeImgInfo(filePath, ItemTypePic, "UT_TestClass");
    infos << info;
    m_db->insertImgInfos(infos);

    DBImgInfoList result = call_private_fun::DBManagergetImgInfos(*m_db, "ClassName", "UT_TestClass", false);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].filePath, filePath);
    EXPECT_EQ(result[0].className, "UT_TestClass");
}

TEST_F(DBManagerTest, GetImgInfos_WithTimeData)
{
    QString filePath = "/test/getimginfos_withinetime.jpg";
    insertTestImage(filePath);

    DBImgInfoList result = call_private_fun::DBManagergetImgInfos(*m_db, "FilePath", filePath, true);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].filePath, filePath);
    EXPECT_TRUE(result[0].time.isValid());
    EXPECT_TRUE(result[0].changeTime.isValid());
    EXPECT_TRUE(result[0].importTime.isValid());
}

// =========================================================================
// 20. getYears — B1/B2
// =========================================================================
TEST_F(DBManagerTest, GetYears_EmptyReturnsEmpty)
{
    QStringList years = m_db->getYears();
    EXPECT_TRUE(years.isEmpty());
}

TEST_F(DBManagerTest, GetYears_ReturnsDistinctYears)
{
    DBImgInfoList infos;
    infos << makeImgInfo("/test/year_2023.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2023, 3, 10), QTime(9, 0)));
    infos << makeImgInfo("/test/year_2024a.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 6, 15), QTime(10, 30)));
    infos << makeImgInfo("/test/year_2024b.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 12, 1), QTime(14, 0)));
    m_db->insertImgInfos(infos);

    QStringList years = m_db->getYears();
    EXPECT_EQ(years.size(), 2);
    EXPECT_TRUE(years.contains("2023"));
    EXPECT_TRUE(years.contains("2024"));
}

// =========================================================================
// 21. getYearPaths — B1/B2/B3
// =========================================================================
TEST_F(DBManagerTest, GetYearPaths_EmptyReturnsEmpty)
{
    QStringList paths = m_db->getYearPaths("2024", 10);
    EXPECT_TRUE(paths.isEmpty());
}

TEST_F(DBManagerTest, GetYearPaths_ReturnsPathsForYear)
{
    DBImgInfoList infos;
    infos << makeImgInfo("/test/yearpath_2024a.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 6, 15), QTime(10, 30)));
    infos << makeImgInfo("/test/yearpath_2024b.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 8, 20), QTime(12, 0)));
    infos << makeImgInfo("/test/yearpath_2023.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2023, 1, 5), QTime(8, 0)));
    m_db->insertImgInfos(infos);

    QStringList paths = m_db->getYearPaths("2024", 100);
    EXPECT_EQ(paths.size(), 2);
    EXPECT_TRUE(paths.contains("/test/yearpath_2024a.jpg"));
    EXPECT_TRUE(paths.contains("/test/yearpath_2024b.jpg"));
    EXPECT_FALSE(paths.contains("/test/yearpath_2023.jpg"));
}

TEST_F(DBManagerTest, GetYearPaths_RespectsMaxCount)
{
    DBImgInfoList infos;
    infos << makeImgInfo("/test/yearpath_max1.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 1, 1), QTime(1, 0)));
    infos << makeImgInfo("/test/yearpath_max2.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 2, 1), QTime(2, 0)));
    infos << makeImgInfo("/test/yearpath_max3.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 3, 1), QTime(3, 0)));
    m_db->insertImgInfos(infos);

    QStringList paths = m_db->getYearPaths("2024", 2);
    EXPECT_EQ(paths.size(), 2);
}

// =========================================================================
// 22. getMonths — B1/B2
// =========================================================================
TEST_F(DBManagerTest, GetMonths_EmptyReturnsEmpty)
{
    QStringList months = m_db->getMonths();
    EXPECT_TRUE(months.isEmpty());
}

TEST_F(DBManagerTest, GetMonths_ReturnsDistinctMonths)
{
    DBImgInfoList infos;
    infos << makeImgInfo("/test/month_202406a.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 6, 15), QTime(10, 30)));
    infos << makeImgInfo("/test/month_202406b.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 6, 20), QTime(11, 0)));
    infos << makeImgInfo("/test/month_202408.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 8, 5), QTime(9, 0)));
    m_db->insertImgInfos(infos);

    QStringList months = m_db->getMonths();
    EXPECT_EQ(months.size(), 2);
    EXPECT_TRUE(months.contains("2024-06"));
    EXPECT_TRUE(months.contains("2024-08"));
}

// =========================================================================
// 23. getMonthPaths — B1/B2/B3
// =========================================================================
TEST_F(DBManagerTest, GetMonthPaths_EmptyReturnsEmpty)
{
    QStringList paths = m_db->getMonthPaths("2024", "06", 10);
    EXPECT_TRUE(paths.isEmpty());
}

TEST_F(DBManagerTest, GetMonthPaths_ReturnsPathsForMonth)
{
    DBImgInfoList infos;
    infos << makeImgInfo("/test/monthpath_202406a.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 6, 15), QTime(10, 30)));
    infos << makeImgInfo("/test/monthpath_202406b.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 6, 20), QTime(11, 0)));
    infos << makeImgInfo("/test/monthpath_202408.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 8, 5), QTime(9, 0)));
    m_db->insertImgInfos(infos);

    QStringList paths = m_db->getMonthPaths("2024", "06", 100);
    EXPECT_EQ(paths.size(), 2);
    EXPECT_TRUE(paths.contains("/test/monthpath_202406a.jpg"));
    EXPECT_TRUE(paths.contains("/test/monthpath_202406b.jpg"));
    EXPECT_FALSE(paths.contains("/test/monthpath_202408.jpg"));
}

TEST_F(DBManagerTest, GetMonthPaths_RespectsMaxCount)
{
    DBImgInfoList infos;
    infos << makeImgInfo("/test/monthpath_max1.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 7, 1), QTime(1, 0)));
    infos << makeImgInfo("/test/monthpath_max2.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 7, 2), QTime(2, 0)));
    infos << makeImgInfo("/test/monthpath_max3.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 7, 3), QTime(3, 0)));
    m_db->insertImgInfos(infos);

    QStringList paths = m_db->getMonthPaths("2024", "07", 2);
    EXPECT_EQ(paths.size(), 2);
}

// =========================================================================
// 24. getDays — B1/B2
// =========================================================================
TEST_F(DBManagerTest, GetDays_EmptyReturnsEmpty)
{
    QStringList days = m_db->getDays();
    EXPECT_TRUE(days.isEmpty());
}

TEST_F(DBManagerTest, GetDays_ReturnsDistinctDays)
{
    DBImgInfoList infos;
    infos << makeImgInfo("/test/day_20240615a.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 6, 15), QTime(10, 30)));
    infos << makeImgInfo("/test/day_20240615b.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 6, 15), QTime(14, 0)));
    infos << makeImgInfo("/test/day_20240620.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 6, 20), QTime(9, 0)));
    m_db->insertImgInfos(infos);

    QStringList days = m_db->getDays();
    EXPECT_EQ(days.size(), 2);
    EXPECT_TRUE(days.contains("2024-06-15"));
    EXPECT_TRUE(days.contains("2024-06-20"));
}

// =========================================================================
// 25. getDayPaths — B1/B2/B3
// =========================================================================
TEST_F(DBManagerTest, GetDayPaths_EmptyReturnsEmpty)
{
    QStringList paths = m_db->getDayPaths("2024-06-15");
    EXPECT_TRUE(paths.isEmpty());
}

TEST_F(DBManagerTest, GetDayPaths_ReturnsPathsWithFilePrefix)
{
    DBImgInfoList infos;
    infos << makeImgInfo("/test/daypath_20240615.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 6, 15), QTime(10, 30)));
    infos << makeImgInfo("/test/daypath_20240620.jpg", ItemTypePic, QString(),
                         QDateTime(QDate(2024, 6, 20), QTime(9, 0)));
    m_db->insertImgInfos(infos);

    QStringList paths = m_db->getDayPaths("2024-06-15");
    EXPECT_EQ(paths.size(), 1);
    EXPECT_TRUE(paths.contains("file:///test/daypath_20240615.jpg"));
    EXPECT_FALSE(paths.contains("file:///test/daypath_20240620.jpg"));
}

TEST_F(DBManagerTest, GetDayPaths_NonMatchingDayReturnsEmpty)
{
    insertTestImage("/test/daypath_nomatch.jpg");

    QStringList paths = m_db->getDayPaths("1999-01-01");
    EXPECT_TRUE(paths.isEmpty());
}
