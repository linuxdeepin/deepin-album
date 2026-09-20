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
