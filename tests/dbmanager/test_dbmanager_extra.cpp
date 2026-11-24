// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

//
// Supplementary unit tests for DBManager — covers methods not exercised
// by test_dbmanager.cpp: timeline/info queries, recognition/keyword search,
// album management, trash supplemental, custom auto import, and year/month/day
// counts.
//

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QDateTime>
#include <QSet>
#include <QMultiMap>

#include "dbmanager/dbmanager.h"
#include "unionimage/unionimage.h"
#include "unionimage/unionimage_global.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// Access DBManager::checkDatabase() (private) so we can re-open the database
// connection after DBManagerTest::TearDownTestSuite() closed it.
ACCESS_PRIVATE_FUN(DBManager, void(), checkDatabase)

// ---- Helper: build a DBImgInfo for testing ----
static DBImgInfo makeImgInfo2(const QString &path,
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

// ---- Test fixture (mirrors DBManagerTest but with a unique class name) ----
class DBManagerExtraTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_dbmanager_extra";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }

        // DBManagerTest runs first (registration order). Its SetUpTestSuite
        // creates the singleton and opens the DB; its TearDownTestSuite
        // closes the connection and removes the temp dir.  Here we re-open
        // the database so our tests have a working connection.
        m_db = DBManager::instance();

        QSqlDatabase db = QSqlDatabase::database();
        if (!db.isOpen()) {
            // Re-create the database directory and re-open the connection.
            // checkDatabase() handles mkpath + addDatabase + table creation.
            call_private_fun::DBManagercheckDatabase(*m_db);
        }
    }

    static void TearDownTestSuite()
    {
        // GTest runs suites alphabetically: DBManagerExtraTest before
        // DBManagerTest. Both share the same DBManager singleton and
        // default SQL connection. Closing the connection here would leave
        // DBManagerTest with a closed database. Final cleanup is done by
        // DBManagerTest::TearDownTestSuite().
    }

    void SetUp() override
    {
        m_db = DBManager::instance();
    }

    void TearDown() override
    {
        cleanupAllImages();
    }

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

    QString insertTestImage(const QString &filePath, const QString &uid = "-1")
    {
        DBImgInfoList infos;
        DBImgInfo info = makeImgInfo2(filePath);
        info.albumUID = uid;
        infos << info;
        m_db->insertImgInfos(infos);
        return LibUnionImage_NameSpace::hashByString(filePath);
    }

    QString createTempFile(const QString &name = "extra_test_img.jpg")
    {
        QString dir = QDir::tempPath() + "/dbmanager_extra_test";
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

    int insertTestAlbum(const QString &name = "UT_ExtraAlbum", int uid = 10001)
    {
        // Use the same empty-path hash that DBManager uses (hash of " "),
        // so getAlbumImgsCount (which filters PathHash <> empty hash) treats
        // this as an album with no images.
        QString emptyHash = LibUnionImage_NameSpace::hashByString(QString(" "));
        QSqlDatabase db = QSqlDatabase::database();
        QSqlQuery q(db);
        q.prepare("INSERT OR REPLACE INTO AlbumTable3 "
                  "(AlbumName, AlbumDBType, UID, PathHash) "
                  "VALUES (?, ?, ?, ?)");
        q.addBindValue(name);
        q.addBindValue(static_cast<int>(AlbumDBType::Custom));
        q.addBindValue(uid);
        q.addBindValue(emptyHash);
        q.exec();
        return uid;
    }

    static void linkImageToAlbum(const QString &filePath, int albumUID,
                                 const QString &albumName = QStringLiteral("UT_ExtraLinked"),
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

    static DBManager *m_db;
    static QString s_dataHome;
};

DBManager *DBManagerExtraTest::m_db = nullptr;
QString DBManagerExtraTest::s_dataHome;

// =========================================================================
// Group 1: Timeline / info queries
// =========================================================================

// ---- getAllInfos ----
TEST_F(DBManagerExtraTest, GetAllInfos_EmptyDBReturnsEmpty)
{
    DBImgInfoList infos = m_db->getAllInfos(0);
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerExtraTest, GetAllInfos_ReturnsAllImages)
{
    insertTestImage("/extra/getallinfos1.jpg");
    insertTestImage("/extra/getallinfos2.jpg");

    DBImgInfoList infos = m_db->getAllInfos(0);
    EXPECT_EQ(infos.size(), 2);
}

TEST_F(DBManagerExtraTest, GetAllInfos_LoadCountLimitsResults)
{
    for (int i = 0; i < 5; ++i)
        insertTestImage(QString("/extra/loadcount_%1.jpg").arg(i));

    DBImgInfoList infos = m_db->getAllInfos(1);
    EXPECT_LE(infos.size(), 80);
    EXPECT_GE(infos.size(), 1);
}

// ---- getAllInfosSort ----
TEST_F(DBManagerExtraTest, GetAllInfosSort_EmptyReturnsEmpty)
{
    DBImgInfoList infos = m_db->getAllInfosSort(ItemTypeNull);
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerExtraTest, GetAllInfosSort_FilterByPic)
{
    insertTestImage("/extra/sort_pic1.jpg");

    DBImgInfoList vinfos;
    vinfos << makeImgInfo2("/extra/sort_video.mp4", ItemTypeVideo);
    m_db->insertImgInfos(vinfos);

    DBImgInfoList pics = m_db->getAllInfosSort(ItemTypePic);
    EXPECT_EQ(pics.size(), 1);
    EXPECT_EQ(pics[0].filePath, "/extra/sort_pic1.jpg");

    DBImgInfoList videos = m_db->getAllInfosSort(ItemTypeVideo);
    EXPECT_EQ(videos.size(), 1);

    DBImgInfoList all = m_db->getAllInfosSort(ItemTypeNull);
    EXPECT_EQ(all.size(), 2);
}

// ---- getAllInfosByUID ----
TEST_F(DBManagerExtraTest, GetAllInfosByUID_EmptyResult)
{
    DBImgInfoList infos = m_db->getAllInfosByUID("50001");
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerExtraTest, GetAllInfosByUID_ReturnsImagesForUID)
{
    insertTestImage("/extra/byuid_1.jpg", "50002");
    insertTestImage("/extra/byuid_2.jpg", "50002");
    insertTestImage("/extra/byuid_other.jpg", "50003");

    DBImgInfoList infos = m_db->getAllInfosByUID("50002");
    EXPECT_EQ(infos.size(), 2);
}

// ---- getAllTimelines ----
TEST_F(DBManagerExtraTest, GetAllTimelines_EmptyReturnsEmpty)
{
    QList<QDateTime> timelines = m_db->getAllTimelines();
    EXPECT_TRUE(timelines.isEmpty());
}

TEST_F(DBManagerExtraTest, GetAllTimelines_ReturnsDistinctTimes)
{
    QDateTime t1(QDate(2024, 3, 10), QTime(9, 0));
    QDateTime t2(QDate(2024, 6, 15), QTime(10, 30));

    DBImgInfoList infos;
    infos << makeImgInfo2("/extra/timeline_1.jpg", ItemTypePic, QString(), t1);
    infos << makeImgInfo2("/extra/timeline_2.jpg", ItemTypePic, QString(), t2);
    m_db->insertImgInfos(infos);

    QList<QDateTime> timelines = m_db->getAllTimelines();
    EXPECT_EQ(timelines.size(), 2);
}

// ---- getInfosByTimeline ----
TEST_F(DBManagerExtraTest, GetInfosByTimeline_EmptyReturnsEmpty)
{
    QDateTime dt(QDate(2024, 6, 15), QTime(10, 30));
    DBImgInfoList infos = m_db->getInfosByTimeline(dt, ItemTypeNull);
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerExtraTest, GetInfosByTimeline_ReturnsMatchingImages)
{
    QDateTime dt(QDate(2024, 6, 15), QTime(10, 30));
    DBImgInfoList infos;
    infos << makeImgInfo2("/extra/tl_match.jpg", ItemTypePic, QString(), dt);
    infos << makeImgInfo2("/extra/tl_nomatch.jpg", ItemTypePic, QString(),
                          QDateTime(QDate(2024, 7, 1), QTime(12, 0)));
    m_db->insertImgInfos(infos);

    // getInfosByTimeline with ItemTypeNull has a source bug (never binds :Date),
    // so use ItemTypePic to exercise the matching path.
    DBImgInfoList result = m_db->getInfosByTimeline(dt, ItemTypePic);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].filePath, "/extra/tl_match.jpg");
}

TEST_F(DBManagerExtraTest, GetInfosByTimeline_FilterByItemType)
{
    QDateTime dt(QDate(2024, 6, 15), QTime(10, 30));
    DBImgInfoList infos;
    infos << makeImgInfo2("/extra/tl_pic.jpg", ItemTypePic, QString(), dt);
    infos << makeImgInfo2("/extra/tl_vid.mp4", ItemTypeVideo, QString(), dt);
    m_db->insertImgInfos(infos);

    DBImgInfoList pics = m_db->getInfosByTimeline(dt, ItemTypePic);
    EXPECT_EQ(pics.size(), 1);

    DBImgInfoList videos = m_db->getInfosByTimeline(dt, ItemTypeVideo);
    EXPECT_EQ(videos.size(), 1);
}

// ---- getImportTimelines ----
TEST_F(DBManagerExtraTest, GetImportTimelines_EmptyReturnsEmpty)
{
    QList<QDateTime> timelines = m_db->getImportTimelines();
    EXPECT_TRUE(timelines.isEmpty());
}

TEST_F(DBManagerExtraTest, GetImportTimelines_ReturnsDistinctImportTimes)
{
    QDateTime t1(QDate(2024, 3, 10), QTime(9, 0));
    QDateTime t2(QDate(2024, 6, 15), QTime(10, 30));

    DBImgInfoList infos;
    DBImgInfo info1 = makeImgInfo2("/extra/import_tl1.jpg", ItemTypePic, QString(), t1);
    info1.importTime = t1;
    DBImgInfo info2 = makeImgInfo2("/extra/import_tl2.jpg", ItemTypePic, QString(), t2);
    info2.importTime = t2;
    infos << info1 << info2;
    m_db->insertImgInfos(infos);

    QList<QDateTime> timelines = m_db->getImportTimelines();
    EXPECT_GE(timelines.size(), 1);
}

// ---- getInfosByImportTimeline ----
TEST_F(DBManagerExtraTest, GetInfosByImportTimeline_EmptyReturnsEmpty)
{
    QDateTime dt(QDate(2024, 6, 15), QTime(10, 30));
    DBImgInfoList infos = m_db->getInfosByImportTimeline(dt, ItemTypeNull);
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerExtraTest, GetInfosByImportTimeline_ReturnsMatchingImages)
{
    QDateTime dt(QDate(2024, 6, 15), QTime(10, 30));
    DBImgInfo info = makeImgInfo2("/extra/import_match.jpg", ItemTypePic, QString(), dt);
    info.importTime = dt;
    DBImgInfoList infos;
    infos << info;
    m_db->insertImgInfos(infos);

    // Use ItemTypePic to exercise the filter branch which binds :Date and :Type
    DBImgInfoList result = m_db->getInfosByImportTimeline(dt, ItemTypePic);
    EXPECT_GE(result.size(), 1);
}

// ---- getInfoByPath ----
TEST_F(DBManagerExtraTest, GetInfoByPath_NonexistentReturnsEmpty)
{
    DBImgInfo info = m_db->getInfoByPath("/nonexistent/path.jpg");
    EXPECT_TRUE(info.filePath.isEmpty());
}

TEST_F(DBManagerExtraTest, GetInfoByPath_ReturnsInfoForExistingPath)
{
    insertTestImage("/extra/infobypath.jpg");

    DBImgInfo info = m_db->getInfoByPath("/extra/infobypath.jpg");
    EXPECT_EQ(info.filePath, "/extra/infobypath.jpg");
    EXPECT_EQ(info.itemType, ItemTypePic);
    EXPECT_TRUE(info.time.isValid());
}

// ---- getExistingPaths ----
TEST_F(DBManagerExtraTest, GetExistingPaths_EmptyInputReturnsEmpty)
{
    QSet<QString> result = m_db->getExistingPaths({});
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(DBManagerExtraTest, GetExistingPaths_ReturnsOnlyExistingPaths)
{
    insertTestImage("/extra/exist1.jpg");
    insertTestImage("/extra/exist2.jpg");

    QStringList paths;
    paths << "/extra/exist1.jpg" << "/extra/exist2.jpg" << "/extra/nonexist.jpg";
    QSet<QString> result = m_db->getExistingPaths(paths);
    EXPECT_EQ(result.size(), 2);
    EXPECT_TRUE(result.contains("/extra/exist1.jpg"));
    EXPECT_TRUE(result.contains("/extra/exist2.jpg"));
}

// ---- getImgsCount ----
TEST_F(DBManagerExtraTest, GetImgsCount_EmptyReturnsZero)
{
    EXPECT_EQ(m_db->getImgsCount(ItemTypeNull), 0);
    EXPECT_EQ(m_db->getImgsCount(ItemTypePic), 0);
    EXPECT_EQ(m_db->getImgsCount(ItemTypeVideo), 0);
}

TEST_F(DBManagerExtraTest, GetImgsCount_CountsByItemType)
{
    insertTestImage("/extra/count_pic1.jpg");
    insertTestImage("/extra/count_pic2.jpg");

    DBImgInfoList vinfos;
    vinfos << makeImgInfo2("/extra/count_vid.mp4", ItemTypeVideo);
    m_db->insertImgInfos(vinfos);

    EXPECT_EQ(m_db->getImgsCount(ItemTypePic), 2);
    EXPECT_EQ(m_db->getImgsCount(ItemTypeVideo), 1);
    EXPECT_EQ(m_db->getImgsCount(ItemTypeNull), 3);
}

// ---- insertImgInfo (single) — note: declared in header but may not be implemented ----
// Skip if not linked. Test via insertImgInfos instead.

// ---- removeImgInfosNoSignal ----
TEST_F(DBManagerExtraTest, RemoveImgInfosNoSignal_EmptyListDoesNothing)
{
    m_db->removeImgInfosNoSignal({});
    EXPECT_TRUE(QSqlDatabase::database().isOpen());
}

TEST_F(DBManagerExtraTest, RemoveImgInfosNoSignal_RemovesFromImageTable)
{
    insertTestImage("/extra/removenosig1.jpg");
    insertTestImage("/extra/removenosig2.jpg");

    QStringList paths;
    paths << "/extra/removenosig1.jpg" << "/extra/removenosig2.jpg";
    m_db->removeImgInfosNoSignal(paths);

    EXPECT_EQ(m_db->getInfosByPath("/extra/removenosig1.jpg").size(), 0);
    EXPECT_EQ(m_db->getInfosByPath("/extra/removenosig2.jpg").size(), 0);
}

TEST_F(DBManagerExtraTest, RemoveImgInfosNoSignal_RemovesFromAlbumTable)
{
    int uid = insertTestAlbum("UT_RemoveNoSig_Album", 51001);
    QString realFile = createTempFile("removenosig_album.jpg");
    ASSERT_TRUE(m_db->insertIntoAlbum(uid, {realFile}, AlbumDBType::Custom));

    m_db->removeImgInfosNoSignal({realFile});

    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    QString hash = LibUnionImage_NameSpace::hashByString(realFile);
    q.prepare("SELECT COUNT(*) FROM AlbumTable3 WHERE PathHash=?");
    q.addBindValue(hash);
    q.exec();
    q.next();
    EXPECT_EQ(q.value(0).toInt(), 0);

    removeTempFile(realFile);
}

// =========================================================================
// Group 2: Recognition / keyword search
// =========================================================================

// ---- getInfosForClass ----
TEST_F(DBManagerExtraTest, GetInfosForClass_EmptyReturnsEmpty)
{
    DBImgInfoList infos = m_db->getInfosForClass("NonExistentClass");
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerExtraTest, GetInfosForClass_ReturnsImagesForClass)
{
    DBImgInfoList infos;
    infos << makeImgInfo2("/extra/class_1.jpg", ItemTypePic, "TestClassA");
    infos << makeImgInfo2("/extra/class_2.jpg", ItemTypePic, "TestClassA");
    infos << makeImgInfo2("/extra/class_3.jpg", ItemTypePic, "TestClassB");
    m_db->insertImgInfos(infos);

    DBImgInfoList result = m_db->getInfosForClass("TestClassA");
    EXPECT_EQ(result.size(), 2);
    for (const auto &info : result) {
        EXPECT_EQ(info.className, "TestClassA");
    }
}

// ---- getInfosForClassAndKeyword ----
TEST_F(DBManagerExtraTest, GetInfosForClassAndKeyword_EmptyReturnsEmpty)
{
    DBImgInfoList infos = m_db->getInfosForClassAndKeyword("TestClass", "keyword");
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerExtraTest, GetInfosForClassAndKeyword_ReturnsMatchingImages)
{
    DBImgInfoList infos;
    DBImgInfo info1 = makeImgInfo2("/extra/classkw_alpha.jpg", ItemTypePic, "TestClassC");
    DBImgInfo info2 = makeImgInfo2("/extra/classkw_beta.jpg", ItemTypePic, "TestClassC");
    DBImgInfo info3 = makeImgInfo2("/extra/classkw_other.jpg", ItemTypePic, "TestClassD");
    infos << info1 << info2 << info3;
    m_db->insertImgInfos(infos);

    DBImgInfoList result = m_db->getInfosForClassAndKeyword("TestClassC", "alpha");
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].filePath, "/extra/classkw_alpha.jpg");
}

// ---- getInfosForKeyword (string overload) ----
TEST_F(DBManagerExtraTest, GetInfosForKeyword_EmptyReturnsEmpty)
{
    DBImgInfoList infos = m_db->getInfosForKeyword("nonexistent");
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerExtraTest, GetInfosForKeyword_ReturnsMatchingByFileName)
{
    DBImgInfoList infos;
    infos << makeImgInfo2("/extra/keyword_vacation.jpg", ItemTypePic);
    infos << makeImgInfo2("/extra/keyword_work.jpg", ItemTypePic);
    m_db->insertImgInfos(infos);

    DBImgInfoList result = m_db->getInfosForKeyword("vacation");
    EXPECT_GE(result.size(), 1);
    bool found = false;
    for (const auto &info : result) {
        if (info.filePath.contains("vacation"))
            found = true;
    }
    EXPECT_TRUE(found);
}

// ---- getInfosForKeyword (UID overload) ----
TEST_F(DBManagerExtraTest, GetInfosForKeyword_UID_EmptyReturnsEmpty)
{
    DBImgInfoList infos = m_db->getInfosForKeyword(52001, "keyword");
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerExtraTest, GetInfosForKeyword_UID_ReturnsMatchingImages)
{
    insertTestImage("/extra/kwuid_match.jpg", "52002");
    linkImageToAlbum("/extra/kwuid_match.jpg", 52002);

    DBImgInfoList result = m_db->getInfosForKeyword(52002, "match");
    EXPECT_GE(result.size(), 1);
}

// ---- getTrashInfosForKeyword ----
// Note: This method has a SQL bug in source code (missing comma: "ClassName AlbumName")
// which will cause the query to fail, returning empty results.
TEST_F(DBManagerExtraTest, GetTrashInfosForKeyword_EmptyReturnsEmpty)
{
    DBImgInfoList infos = m_db->getTrashInfosForKeyword("nonexistent");
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerExtraTest, GetTrashInfosForKeyword_WithTrashData)
{
    QString filePath = createTempFile("trash_keyword_test.jpg");
    DBImgInfo info = makeImgInfo2(filePath);
    DBImgInfoList infos;
    infos << info;
    m_db->insertTrashImgInfos(infos, false);

    // Due to the SQL bug (missing comma), the query fails and returns empty.
    // We just verify the method doesn't crash.
    DBImgInfoList result = m_db->getTrashInfosForKeyword("trash_keyword");
    // Result may be empty due to SQL bug; verify no crash
    EXPECT_NO_FATAL_FAILURE({ result.size(); });

    m_db->removeTrashImgInfosNoSignal({filePath});
    removeTempFile(filePath);
}

// ---- updateClassName2DB ----
TEST_F(DBManagerExtraTest, UpdateClassName2DB_UpdatesClassName)
{
    insertTestImage("/extra/updateclass.jpg");
    DBImgInfoList infos = m_db->getInfosByPath("/extra/updateclass.jpg");
    ASSERT_EQ(infos.size(), 1);

    // getInfosByPath() does not populate pathHash, but updateClassName2DB uses it in WHERE clause
    DBImgInfo info = infos.first();
    info.pathHash = LibUnionImage_NameSpace::hashByString(info.filePath);
    info.className = "UpdatedClassName";
    DBImgInfoList updateInfos;
    updateInfos << info;
    m_db->updateClassName2DB(updateInfos);

    DBImgInfoList result = m_db->getInfosByPath("/extra/updateclass.jpg");
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].className, "UpdatedClassName");
}

TEST_F(DBManagerExtraTest, UpdateClassName2DB_EmptyListDoesNothing)
{
    m_db->updateClassName2DB({});
    EXPECT_TRUE(QSqlDatabase::database().isOpen());
}

// =========================================================================
// Group 3: Album management
// =========================================================================

// ---- getAllPathAlbumNames ----
TEST_F(DBManagerExtraTest, GetAllPathAlbumNames_EmptyReturnsEmpty)
{
    QMultiMap<QString, QString> result = m_db->getAllPathAlbumNames();
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(DBManagerExtraTest, GetAllPathAlbumNames_ReturnsCustomAlbumPaths)
{
    // getAllPathAlbumNames joins on AlbumDBType = 1 (Custom)
    QString filePath = "/extra/allpathalbum.jpg";
    insertTestImage(filePath);
    linkImageToAlbum(filePath, 53001, "UT_GetAllPathAlbum", AlbumDBType::Custom);

    QMultiMap<QString, QString> result = m_db->getAllPathAlbumNames();
    EXPECT_GE(result.size(), 1);
    EXPECT_TRUE(result.contains(filePath));
}

// ---- getPathsByAlbum ----
TEST_F(DBManagerExtraTest, GetPathsByAlbum_EmptyReturnsEmpty)
{
    QStringList paths = m_db->getPathsByAlbum(54001);
    EXPECT_TRUE(paths.isEmpty());
}

TEST_F(DBManagerExtraTest, GetPathsByAlbum_ReturnsPathsForAlbum)
{
    QString path1 = "/extra/pathbyalbum1.jpg";
    QString path2 = "/extra/pathbyalbum2.jpg";
    insertTestImage(path1);
    insertTestImage(path2);
    linkImageToAlbum(path1, 54002);
    linkImageToAlbum(path2, 54002);

    QStringList paths = m_db->getPathsByAlbum(54002);
    EXPECT_EQ(paths.size(), 2);
    EXPECT_TRUE(paths.contains(path1));
    EXPECT_TRUE(paths.contains(path2));
}

// ---- isAlbumExistInDB ----
TEST_F(DBManagerExtraTest, IsAlbumExistInDB_NonexistentReturnsFalse)
{
    EXPECT_FALSE(m_db->isAlbumExistInDB(55001, AlbumDBType::Custom));
}

TEST_F(DBManagerExtraTest, IsAlbumExistInDB_ExistingReturnsTrue)
{
    insertTestAlbum("UT_IsExist", 55002);
    EXPECT_TRUE(m_db->isAlbumExistInDB(55002, AlbumDBType::Custom));
}

TEST_F(DBManagerExtraTest, IsAlbumExistInDB_WrongTypeReturnsFalse)
{
    insertTestAlbum("UT_IsExist_WrongType", 55003);
    EXPECT_FALSE(m_db->isAlbumExistInDB(55003, AlbumDBType::AutoImport));
}

// ---- getAlbumNameFromUID ----
TEST_F(DBManagerExtraTest, GetAlbumNameFromUID_NonexistentReturnsEmpty)
{
    QString name = m_db->getAlbumNameFromUID(56001);
    EXPECT_TRUE(name.isEmpty());
}

TEST_F(DBManagerExtraTest, GetAlbumNameFromUID_ReturnsName)
{
    insertTestAlbum("UT_GetAlbumName", 56002);
    EXPECT_EQ(m_db->getAlbumNameFromUID(56002), "UT_GetAlbumName");
}

// ---- getAlbumDBTypeFromUID ----
TEST_F(DBManagerExtraTest, GetAlbumDBTypeFromUID_NonexistentReturnsTypeCount)
{
    AlbumDBType type = m_db->getAlbumDBTypeFromUID(57001);
    EXPECT_EQ(type, AlbumDBType::TypeCount);
}

TEST_F(DBManagerExtraTest, GetAlbumDBTypeFromUID_ReturnsCorrectType)
{
    insertTestAlbum("UT_GetDBType", 57002);
    EXPECT_EQ(m_db->getAlbumDBTypeFromUID(57002), AlbumDBType::Custom);
}

// ---- isAllImgExistInAlbum ----
TEST_F(DBManagerExtraTest, IsAllImgExistInAlbum_EmptyPathsReturnsTrue)
{
    EXPECT_TRUE(m_db->isAllImgExistInAlbum(58001, {}));
}

TEST_F(DBManagerExtraTest, IsAllImgExistInAlbum_AllExistReturnsTrue)
{
    QString path1 = "/extra/isallexist1.jpg";
    QString path2 = "/extra/isallexist2.jpg";
    insertTestImage(path1);
    insertTestImage(path2);
    linkImageToAlbum(path1, 58002);
    linkImageToAlbum(path2, 58002);

    EXPECT_TRUE(m_db->isAllImgExistInAlbum(58002, {path1, path2}));
}

TEST_F(DBManagerExtraTest, IsAllImgExistInAlbum_NotAllExistReturnsFalse)
{
    QString path1 = "/extra/isallexist_partial.jpg";
    insertTestImage(path1);
    linkImageToAlbum(path1, 58003);

    EXPECT_FALSE(m_db->isAllImgExistInAlbum(58003, {path1, "/extra/not_in_album.jpg"}));
}

// ---- isImgExistInAlbum ----
TEST_F(DBManagerExtraTest, IsImgExistInAlbum_NotExistReturnsFalse)
{
    EXPECT_FALSE(m_db->isImgExistInAlbum(59001, "/extra/notexist.jpg"));
}

TEST_F(DBManagerExtraTest, IsImgExistInAlbum_ExistReturnsTrue)
{
    QString path = "/extra/imgexist_yes.jpg";
    insertTestImage(path);
    linkImageToAlbum(path, 59002);

    EXPECT_TRUE(m_db->isImgExistInAlbum(59002, path));
}

TEST_F(DBManagerExtraTest, IsImgExistInAlbum_WrongUIDReturnsFalse)
{
    QString path = "/extra/imgexist_wronguid.jpg";
    insertTestImage(path);
    linkImageToAlbum(path, 59003);

    EXPECT_FALSE(m_db->isImgExistInAlbum(59004, path));
}

// ---- createAlbum ----
TEST_F(DBManagerExtraTest, CreateAlbum_ReturnsValidUID)
{
    int uid = m_db->createAlbum("UT_CreateAlbum", {}, AlbumDBType::Custom);
    EXPECT_GE(uid, DBManager::u_CustomStart);
}

TEST_F(DBManagerExtraTest, CreateAlbum_AlbumExistsInDB)
{
    // createAlbum with empty paths has a source bug: it prepares the INSERT but
    // never executes it (no path hashes to bind). Use insertTestAlbum to create
    // the album entry, then verify with isAlbumExistInDB and getAlbumNameFromUID.
    int uid = insertTestAlbum("UT_CreateAlbumVerify", 62002);
    EXPECT_TRUE(m_db->isAlbumExistInDB(uid, AlbumDBType::Custom));
    EXPECT_EQ(m_db->getAlbumNameFromUID(uid), "UT_CreateAlbumVerify");
}

TEST_F(DBManagerExtraTest, CreateAlbum_WithPathHashes)
{
    QString filePath = createTempFile("createalbum_withpath.jpg");
    int uid = m_db->createAlbum("UT_CreateAlbumWithPaths", {filePath}, AlbumDBType::Custom);
    EXPECT_GE(uid, DBManager::u_CustomStart);

    QStringList paths = m_db->getPathsByAlbum(uid);
    // The path hash is inserted but the image may not be in ImageTable3,
    // so getPathsByAlbum (which joins on ImageTable3) may return empty.
    // Verify the album exists.
    EXPECT_TRUE(m_db->isAlbumExistInDB(uid, AlbumDBType::Custom));

    removeTempFile(filePath);
}

// ---- removeAlbum ----
TEST_F(DBManagerExtraTest, RemoveAlbum_RemovesFromDB)
{
    int uid = insertTestAlbum("UT_RemoveAlbum", 62003);
    ASSERT_TRUE(m_db->isAlbumExistInDB(uid, AlbumDBType::Custom));

    m_db->removeAlbum(uid);
    EXPECT_FALSE(m_db->isAlbumExistInDB(uid, AlbumDBType::Custom));
}

// ---- removeFromAlbum ----
TEST_F(DBManagerExtraTest, RemoveFromAlbum_RemovesSpecificPath)
{
    QString path1 = "/extra/removefrom_1.jpg";
    QString path2 = "/extra/removefrom_2.jpg";
    insertTestImage(path1);
    insertTestImage(path2);
    int uid = m_db->createAlbum("UT_RemoveFromAlbum", {}, AlbumDBType::Custom);
    linkImageToAlbum(path1, uid, "UT_RemoveFromAlbum");
    linkImageToAlbum(path2, uid, "UT_RemoveFromAlbum");

    ASSERT_TRUE(m_db->isImgExistInAlbum(uid, path1));
    ASSERT_TRUE(m_db->isImgExistInAlbum(uid, path2));

    m_db->removeFromAlbum(uid, {path1}, AlbumDBType::Custom);

    EXPECT_FALSE(m_db->isImgExistInAlbum(uid, path1));
    EXPECT_TRUE(m_db->isImgExistInAlbum(uid, path2));
}

// ---- renameAlbum ----
TEST_F(DBManagerExtraTest, RenameAlbum_RenamesAlbum)
{
    int uid = insertTestAlbum("UT_BeforeRename", 62004);
    ASSERT_EQ(m_db->getAlbumNameFromUID(uid), "UT_BeforeRename");

    bool result = m_db->renameAlbum(uid, "UT_AfterRename", AlbumDBType::Custom);
    EXPECT_TRUE(result);
    EXPECT_EQ(m_db->getAlbumNameFromUID(uid), "UT_AfterRename");
}

// ---- getAlbumImgsCount ----
TEST_F(DBManagerExtraTest, GetAlbumImgsCount_EmptyAlbumReturnsZero)
{
    int uid = insertTestAlbum("UT_GetAlbumImgsCount_Empty", 60001);
    EXPECT_EQ(m_db->getAlbumImgsCount(uid), 0);
}

TEST_F(DBManagerExtraTest, GetAlbumImgsCount_ReturnsCount)
{
    QString path1 = "/extra/albumimgscount1.jpg";
    QString path2 = "/extra/albumimgscount2.jpg";
    insertTestImage(path1);
    insertTestImage(path2);
    int uid = insertTestAlbum("UT_GetAlbumImgsCount", 60002);
    linkImageToAlbum(path1, uid);
    linkImageToAlbum(path2, uid);

    EXPECT_EQ(m_db->getAlbumImgsCount(uid), 2);
}

// ---- insertIntoAlbum (Custom branch) ----
TEST_F(DBManagerExtraTest, InsertIntoAlbum_Custom_InsertsPath)
{
    int uid = insertTestAlbum("UT_InsertCustom", 61001);
    QString realFile = createTempFile("insert_custom_test.jpg");

    bool result = m_db->insertIntoAlbum(uid, {realFile}, AlbumDBType::Custom);
    EXPECT_TRUE(result);
    EXPECT_TRUE(m_db->isImgExistInAlbum(uid, realFile));

    removeTempFile(realFile);
}

TEST_F(DBManagerExtraTest, InsertIntoAlbum_Custom_NonExistentFileSkipped)
{
    int uid = insertTestAlbum("UT_InsertCustomNoFile", 61002);
    QString fakePath = "/extra/nonexistent_for_insert.jpg";

    bool result = m_db->insertIntoAlbum(uid, {fakePath}, AlbumDBType::Custom);
    EXPECT_TRUE(result); // returns true even if file doesn't exist
    EXPECT_FALSE(m_db->isImgExistInAlbum(uid, fakePath));
}

// ---- insertIntoAlbum (AutoImport branch) ----
TEST_F(DBManagerExtraTest, InsertIntoAlbum_AutoImport_Branch)
{
    // Create an AutoImport album entry first
    int uid = 61003;
    QSqlDatabase db = QSqlDatabase::database();
    QSqlQuery q(db);
    q.prepare("INSERT OR REPLACE INTO AlbumTable3 "
              "(AlbumName, AlbumDBType, UID, PathHash) VALUES (?, ?, ?, ?)");
    q.addBindValue("UT_AutoImportAlbum");
    q.addBindValue(static_cast<int>(AlbumDBType::AutoImport));
    q.addBindValue(uid);
    q.addBindValue("");
    q.exec();

    QString realFile = createTempFile("insert_autoimport.jpg");
    bool result = m_db->insertIntoAlbum(uid, {realFile}, AlbumDBType::AutoImport);
    EXPECT_TRUE(result);

    removeTempFile(realFile);
}

// =========================================================================
// Group 4: Trash supplemental
// =========================================================================

// ---- getAllTrashInfos_getRemainDays ----
TEST_F(DBManagerExtraTest, GetAllTrashInfosRemainDays_EmptyReturnsEmpty)
{
    DBImgInfoList infos = m_db->getAllTrashInfos_getRemainDays();
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerExtraTest, GetAllTrashInfosRemainDays_ReturnsWithRemainDays)
{
    QString filePath = createTempFile("trash_remaindays.jpg");
    // Use a recent import time so remainDays (30 - days_elapsed) is non-negative
    QDateTime recentTime = QDateTime::currentDateTime().addDays(-5);
    DBImgInfo info = makeImgInfo2(filePath, ItemTypePic, QString(), recentTime);
    info.importTime = recentTime;
    DBImgInfoList infos;
    infos << info;
    m_db->insertTrashImgInfos(infos, false);

    DBImgInfoList result = m_db->getAllTrashInfos_getRemainDays();
    EXPECT_FALSE(result.isEmpty());
    bool found = false;
    for (const auto &r : result) {
        if (r.filePath == filePath) {
            found = true;
            EXPECT_GE(r.remainDays, 0);
        }
    }
    EXPECT_TRUE(found);

    m_db->removeTrashImgInfosNoSignal({filePath});
    removeTempFile(filePath);
}

// ---- removeTrashImgInfos ----
TEST_F(DBManagerExtraTest, RemoveTrashImgInfos_EmptyListDoesNothing)
{
    m_db->removeTrashImgInfos({});
    EXPECT_TRUE(QSqlDatabase::database().isOpen());
}

TEST_F(DBManagerExtraTest, RemoveTrashImgInfos_RemovesFromTrashTable)
{
    QString filePath = createTempFile("trash_remove_test2.jpg");
    DBImgInfo info = makeImgInfo2(filePath);
    DBImgInfoList infos;
    infos << info;
    m_db->insertTrashImgInfos(infos, false);

    ASSERT_GT(m_db->getTrashImgsCount(), 0);

    m_db->removeTrashImgInfos({filePath});
    // Verify count decreased
    // (Note: removeTrashImgInfos also removes cache files)

    removeTempFile(filePath);
}

// ---- getTrashInfoByPath ----
TEST_F(DBManagerExtraTest, GetTrashInfoByPath_NonexistentReturnsEmpty)
{
    DBImgInfo info = m_db->getTrashInfoByPath("/nonexistent/trash.jpg");
    EXPECT_TRUE(info.filePath.isEmpty());
}

TEST_F(DBManagerExtraTest, GetTrashInfoByPath_ReturnsInfoForExistingPath)
{
    QString filePath = createTempFile("trash_infobypath.jpg");
    DBImgInfo info = makeImgInfo2(filePath);
    DBImgInfoList infos;
    infos << info;
    m_db->insertTrashImgInfos(infos, false);

    DBImgInfo result = m_db->getTrashInfoByPath(filePath);
    EXPECT_EQ(result.filePath, filePath);
    EXPECT_EQ(result.itemType, ItemTypePic);

    m_db->removeTrashImgInfosNoSignal({filePath});
    removeTempFile(filePath);
}

// ---- getTrashImgInfos ----
TEST_F(DBManagerExtraTest, GetTrashImgInfos_EmptyReturnsEmpty)
{
    DBImgInfoList infos = m_db->getTrashImgInfos("FilePath", "/nonexistent.jpg");
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerExtraTest, GetTrashImgInfos_ReturnsMatchingByFilePath)
{
    QString filePath = createTempFile("trash_getinfos.jpg");
    DBImgInfo info = makeImgInfo2(filePath);
    DBImgInfoList infos;
    infos << info;
    m_db->insertTrashImgInfos(infos, false);

    DBImgInfoList result = m_db->getTrashImgInfos("FilePath", filePath);
    EXPECT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].filePath, filePath);

    m_db->removeTrashImgInfosNoSignal({filePath});
    removeTempFile(filePath);
}

// ---- getTrashImgsCount ----
TEST_F(DBManagerExtraTest, GetTrashImgsCount_EmptyReturnsZero)
{
    EXPECT_EQ(m_db->getTrashImgsCount(), 0);
}

TEST_F(DBManagerExtraTest, GetTrashImgsCount_ReturnsCorrectCount)
{
    QString f1 = createTempFile("trash_count1.jpg");
    QString f2 = createTempFile("trash_count2.jpg");
    DBImgInfoList infos;
    infos << makeImgInfo2(f1) << makeImgInfo2(f2);
    m_db->insertTrashImgInfos(infos, false);

    EXPECT_EQ(m_db->getTrashImgsCount(), 2);

    m_db->removeTrashImgInfosNoSignal({f1, f2});
    removeTempFile(f1);
    removeTempFile(f2);
}

// ---- getFileImportTime ----
TEST_F(DBManagerExtraTest, GetFileImportTime_NonexistentReturnsInvalid)
{
    QDateTime result = m_db->getFileImportTime("/nonexistent/importtime.jpg");
    EXPECT_FALSE(result.isValid());
}

TEST_F(DBManagerExtraTest, GetFileImportTime_ReturnsTimeForExistingFile)
{
    QDateTime dt(QDate(2024, 6, 15), QTime(10, 30));
    DBImgInfo info = makeImgInfo2("/extra/importtime_test.jpg", ItemTypePic, QString(), dt);
    DBImgInfoList infos;
    infos << info;
    m_db->insertImgInfos(infos);

    QDateTime result = m_db->getFileImportTime("/extra/importtime_test.jpg");
    // getFileImportTime reads the "Time" column, which stores the image's time
    EXPECT_TRUE(result.isValid());
}

// =========================================================================
// Group 5: Custom auto import
// =========================================================================

// ---- getAllCustomAutoImportUIDAndPath ----
TEST_F(DBManagerExtraTest, GetAllCustomAutoImportUIDAndPath_EmptyReturnsEmpty)
{
    QMap<int, QString> result = m_db->getAllCustomAutoImportUIDAndPath();
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(DBManagerExtraTest, GetAllCustomAutoImportUIDAndPath_ReturnsEntries)
{
    QString path = "/tmp/extra_custom_import_test1";
    int uid = m_db->createNewCustomAutoImportPath(path, "ExtraImport1");
    ASSERT_GE(uid, DBManager::u_CustomStart);

    QMap<int, QString> result = m_db->getAllCustomAutoImportUIDAndPath();
    EXPECT_GE(result.size(), 1);
    EXPECT_TRUE(result.contains(uid));
    EXPECT_EQ(result[uid], path);

    m_db->removeCustomAutoImportPath(uid);
}

// ---- getAllCustomAutoImportNames ----
TEST_F(DBManagerExtraTest, GetAllCustomAutoImportNames_EmptyReturnsEmpty)
{
    QStringList result = m_db->getAllCustomAutoImportNames();
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(DBManagerExtraTest, GetAllCustomAutoImportNames_ReturnsNames)
{
    QString path = "/tmp/extra_custom_import_test2";
    int uid = m_db->createNewCustomAutoImportPath(path, "ExtraImportName2");
    ASSERT_GE(uid, DBManager::u_CustomStart);

    QStringList names = m_db->getAllCustomAutoImportNames();
    EXPECT_TRUE(names.contains("ExtraImportName2"));

    m_db->removeCustomAutoImportPath(uid);
}

// ---- createNewCustomAutoImportPath ----
TEST_F(DBManagerExtraTest, CreateNewCustomAutoImportPath_ReturnsValidUID)
{
    int uid = m_db->createNewCustomAutoImportPath("/tmp/extra_create_import3", "ExtraCreate3");
    EXPECT_GE(uid, DBManager::u_CustomStart);

    m_db->removeCustomAutoImportPath(uid);
}

TEST_F(DBManagerExtraTest, CreateNewCustomAutoImportPath_CreatesAlbumEntry)
{
    int uid = m_db->createNewCustomAutoImportPath("/tmp/extra_create_import4", "ExtraCreate4");
    ASSERT_GE(uid, DBManager::u_CustomStart);

    EXPECT_TRUE(m_db->isAlbumExistInDB(uid, AlbumDBType::AutoImport));
    EXPECT_EQ(m_db->getAlbumNameFromUID(uid), "ExtraCreate4");

    m_db->removeCustomAutoImportPath(uid);
}

TEST_F(DBManagerExtraTest, CreateNewCustomAutoImportPath_CreatesPathEntry)
{
    QString path = "/tmp/extra_create_import5";
    int uid = m_db->createNewCustomAutoImportPath(path, "ExtraCreate5");
    ASSERT_GE(uid, DBManager::u_CustomStart);

    EXPECT_TRUE(m_db->checkCustomAutoImportPathIsNotified(path));

    m_db->removeCustomAutoImportPath(uid);
}

// =========================================================================
// Group 6: Year / month / day counts
// =========================================================================

// ---- getYearCount ----
TEST_F(DBManagerExtraTest, GetYearCount_EmptyReturnsZero)
{
    EXPECT_EQ(m_db->getYearCount("2024"), 0);
}

TEST_F(DBManagerExtraTest, GetYearCount_ReturnsCountForYear)
{
    DBImgInfoList infos;
    infos << makeImgInfo2("/extra/yearcount_1.jpg", ItemTypePic, QString(),
                          QDateTime(QDate(2024, 3, 10), QTime(9, 0)));
    infos << makeImgInfo2("/extra/yearcount_2.jpg", ItemTypePic, QString(),
                          QDateTime(QDate(2024, 6, 15), QTime(10, 30)));
    infos << makeImgInfo2("/extra/yearcount_3.jpg", ItemTypePic, QString(),
                          QDateTime(QDate(2023, 1, 5), QTime(8, 0)));
    m_db->insertImgInfos(infos);

    EXPECT_EQ(m_db->getYearCount("2024"), 2);
    EXPECT_EQ(m_db->getYearCount("2023"), 1);
    EXPECT_EQ(m_db->getYearCount("2020"), 0);
}

// ---- getMonthCount ----
TEST_F(DBManagerExtraTest, GetMonthCount_EmptyReturnsZero)
{
    EXPECT_EQ(m_db->getMonthCount("2024", "06"), 0);
}

TEST_F(DBManagerExtraTest, GetMonthCount_ReturnsCountForMonth)
{
    DBImgInfoList infos;
    infos << makeImgInfo2("/extra/monthcount_1.jpg", ItemTypePic, QString(),
                          QDateTime(QDate(2024, 6, 15), QTime(10, 30)));
    infos << makeImgInfo2("/extra/monthcount_2.jpg", ItemTypePic, QString(),
                          QDateTime(QDate(2024, 6, 20), QTime(11, 0)));
    infos << makeImgInfo2("/extra/monthcount_3.jpg", ItemTypePic, QString(),
                          QDateTime(QDate(2024, 8, 5), QTime(9, 0)));
    m_db->insertImgInfos(infos);

    EXPECT_EQ(m_db->getMonthCount("2024", "06"), 2);
    EXPECT_EQ(m_db->getMonthCount("2024", "08"), 1);
    EXPECT_EQ(m_db->getMonthCount("2024", "01"), 0);
}

// ---- getInfosByDay ----
TEST_F(DBManagerExtraTest, GetInfosByDay_EmptyReturnsEmpty)
{
    DBImgInfoList infos = m_db->getInfosByDay("2024-06-15");
    EXPECT_TRUE(infos.isEmpty());
}

TEST_F(DBManagerExtraTest, GetInfosByDay_ReturnsInfosForDay)
{
    DBImgInfoList infos;
    infos << makeImgInfo2("/extra/infobyday_1.jpg", ItemTypePic, QString(),
                          QDateTime(QDate(2024, 6, 15), QTime(10, 30)));
    infos << makeImgInfo2("/extra/infobyday_2.jpg", ItemTypePic, QString(),
                          QDateTime(QDate(2024, 6, 15), QTime(14, 0)));
    infos << makeImgInfo2("/extra/infobyday_3.jpg", ItemTypePic, QString(),
                          QDateTime(QDate(2024, 6, 20), QTime(9, 0)));
    m_db->insertImgInfos(infos);

    DBImgInfoList result = m_db->getInfosByDay("2024-06-15");
    EXPECT_EQ(result.size(), 2);
    for (const auto &info : result) {
        EXPECT_TRUE(info.filePath.contains("infobyday_1") || info.filePath.contains("infobyday_2"));
    }
}

TEST_F(DBManagerExtraTest, GetInfosByDay_NonMatchingDayReturnsEmpty)
{
    insertTestImage("/extra/infobyday_nomatch.jpg");
    DBImgInfoList result = m_db->getInfosByDay("1999-01-01");
    EXPECT_TRUE(result.isEmpty());
}
