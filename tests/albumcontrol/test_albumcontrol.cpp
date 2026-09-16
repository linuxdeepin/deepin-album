// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QThread>
#include <memory>
#include <QDateTime>
#include <QMap>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "albumControl.h"
#include "dbmanager/dbmanager.h"
#include "imageengine/movieservice.h"
#include "unionimage/unionimage.h"
#include "unionimage/unionimage_global.h"
#include "utils/classifyutils.h"
#include "utils/devicehelper.h"
#include "fileMonitor/fileinotifygroup.h"

#include <QFile>
#include <QDir>
#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// Typedefs to avoid commas in ACCESS_PRIVATE_FIELD macro args
using DBImgInfoListMap = QMap<QString, DBImgInfoList>;
using MovieInfoMap = QMap<QString, MovieInfo>;

// Private field accessors
ACCESS_PRIVATE_FIELD(AlbumControl, DBImgInfoListMap, m_timeLinePathsMap)
ACCESS_PRIVATE_FIELD(AlbumControl, DBImgInfoListMap, m_yearDateMap)
ACCESS_PRIVATE_FIELD(AlbumControl, DBImgInfoListMap, m_monthDateMap)
ACCESS_PRIVATE_FIELD(AlbumControl, DBImgInfoListMap, m_dayDateMap)
ACCESS_PRIVATE_FIELD(AlbumControl, DBImgInfoListMap, m_importTimeLinePathsMap)
ACCESS_PRIVATE_FIELD(AlbumControl, MovieInfoMap, m_movieInfos)
using IntStringMap = QMap<int, QString>;
ACCESS_PRIVATE_FIELD(AlbumControl, IntStringMap, m_customAlbum)
using StringStringMap = QMap<QString, QString>;
ACCESS_PRIVATE_FIELD(AlbumControl, StringStringMap, m_durlAndNameMap)
ACCESS_PRIVATE_FIELD(AlbumControl, StringStringMap, m_blkPath2DeviceNameMap)
using FileInotifyGroupPtr = FileInotifyGroup*;
ACCESS_PRIVATE_FIELD(AlbumControl, FileInotifyGroupPtr, m_fileInotifygroup)
ACCESS_PRIVATE_FUN(AlbumControl, void(const QString &), updateBlockDeviceName)
// ---- Shared stub data ----

static DBImgInfoList g_allPicInfos;
static DBImgInfoList g_classInfos;
static DBImgInfoList g_timelineInfos;
static DBImgInfoList g_importTimelineInfos;
static QList<QDateTime> g_timelines;
static QList<QDateTime> g_importTimelines;
static MovieInfo g_movieInfo;
static QMap<QString, QString> g_metaData;
static QList<std::pair<int, QString>> g_albumNames;
static QMap<int, QString> g_autoImportUIDs;
static DBImgInfoList g_trashInfos;
static QVariantMap g_deviceInfo;

static DBImgInfo makeInfo(const QString &path, ItemType type,
                          const QString &cls = QString(),
                          const QDateTime &time = QDateTime(QDate(2024,1,15), QTime(10,30)))
{
    DBImgInfo info;
    info.filePath = path;
    info.itemType = type;
    info.className = cls;
    info.time = time;
    info.importTime = time;
    info.changeTime = time;
    return info;
}

// ---- Test fixture ----

class AlbumControlTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    StubExt stub;

    void SetUp() override
    {
        g_allPicInfos.clear();
        g_classInfos.clear();
        g_timelineInfos.clear();
        g_importTimelineInfos.clear();
        g_timelines.clear();
        g_importTimelines.clear();
        g_metaData.clear();
        g_albumNames.clear();
        g_autoImportUIDs.clear();
        g_trashInfos.clear();

        g_movieInfo.valid = true;
        g_movieInfo.creation = QDateTime(QDate(2024, 1, 15), QTime(10, 30));
        g_movieInfo.title = "test_video";
        g_movieInfo.filePath = "/tmp/test_video.mp4";

        setupCommonStubs();
    }

    void setupCommonStubs()
    {
        stub.set_lamda(ADDR(AlbumControl, initMonitor), [](AlbumControl *) -> void {});
        stub.set_lamda(ADDR(AlbumControl, initDeviceMonitor), [](AlbumControl *) -> void {});

        stub.set_lamda(ADDR(DBManager, instance), []() -> DBManager * {
            return nullptr;  // sentinel: all methods on this object are stubbed
        });
        stub.set_lamda(ADDR(Classifyutils, GetInstance), []() -> Classifyutils * {
            return nullptr;  // sentinel: all methods on this object are stubbed
        });
        stub.set_lamda(ADDR(Classifyutils, isDBusExist), [](Classifyutils *) -> bool {
            return true;
        });
        stub.set_lamda(ADDR(MovieService, instance), [](QObject *) -> MovieService * {
            return nullptr;  // sentinel: all methods on this object are stubbed
        });

        stub.set_lamda(ADDR(DBManager, getAllInfos),
            [](DBManager *, int) -> const DBImgInfoList { return g_allPicInfos; });
        stub.set_lamda(ADDR(DBManager, getInfosForClass),
            [](DBManager *, const QString &cls) -> const DBImgInfoList {
                if (cls == "Scenery") return g_classInfos;
                return DBImgInfoList();
            });
        stub.set_lamda(ADDR(DBManager, getAllTimelines),
            [](DBManager *) -> const QList<QDateTime> { return g_timelines; });
        stub.set_lamda(ADDR(DBManager, getImportTimelines),
            [](DBManager *) -> const QList<QDateTime> { return g_importTimelines; });
        stub.set_lamda(ADDR(DBManager, getInfosByTimeline),
            [](DBManager *, const QDateTime &, const ItemType &) -> const DBImgInfoList {
                return g_timelineInfos;
            });
        stub.set_lamda(ADDR(DBManager, getInfosByImportTimeline),
            [](DBManager *, const QDateTime &, const ItemType &) -> const DBImgInfoList {
                return g_importTimelineInfos;
            });

        stub.set_lamda(ADDR(AlbumControl, classifyOldDBInfo),
            [](AlbumControl *, const DBImgInfoList &) -> bool { return true; });

        stub.set_lamda(ADDR(MovieService, getMovieInfo),
            [](MovieService *, const QUrl &) -> MovieInfo { return g_movieInfo; });

        stub.set_lamda(ADDR(LibUnionImage_NameSpace, getAllMetaData),
            [](const QString &) -> QMap<QString, QString> { return g_metaData; });
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, isImage),
            [](const QString &) -> bool { return true; });
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, isVideo),
            [](QString) -> bool { return false; });
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, localPath),
            [](const QUrl &url) -> QString { return url.toLocalFile(); });

        stub.set_lamda(ADDR(DBManager, getAllAlbumNames),
            [](DBManager *, AlbumDBType) -> QList<std::pair<int, QString>> { return g_albumNames; });
        stub.set_lamda(ADDR(DBManager, getAllCustomAutoImportUIDAndPath),
            [](DBManager *) -> QMap<int, QString> { return g_autoImportUIDs; });
        stub.set_lamda(ADDR(DBManager, getAllTrashInfos_getRemainDays),
            [](DBManager *) -> const DBImgInfoList { return g_trashInfos; });
        stub.set_lamda(ADDR(DBManager, removeTrashImgInfosNoSignal),
            [](DBManager *, const QStringList &) -> void {});
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, hashByString),
            [](const QString &) -> QString { return QStringLiteral("dummyhash"); });
    }

    std::unique_ptr<AlbumControl> createAC()
    {
        return std::make_unique<AlbumControl>();
    }
};

// ============ getDBInfo ============

TEST_F(AlbumControlTest, GetDBInfo_Video_Valid)
{
    auto ac = createAC();
    DBImgInfo result = ac->getDBInfo("/tmp/test.mp4", true);
    EXPECT_EQ(result.itemType, ItemTypeVideo);
    EXPECT_EQ(result.filePath.toStdString(), "/tmp/test.mp4");
}

TEST_F(AlbumControlTest, GetDBInfo_Video_Invalid)
{
    auto ac = createAC();
    g_movieInfo.valid = false;
    DBImgInfo result = ac->getDBInfo("/tmp/bad.mp4", true);
    EXPECT_EQ(result.itemType, ItemTypeNull);
}

TEST_F(AlbumControlTest, GetDBInfo_Image_WithMetaData)
{
    auto ac = createAC();
    g_metaData["DateTimeOriginal"] = "2024/01/15 10:30";
    DBImgInfo result = ac->getDBInfo("/tmp/pic.jpg", false);
    EXPECT_EQ(result.itemType, ItemTypePic);
    EXPECT_TRUE(result.time.isValid());
}

TEST_F(AlbumControlTest, GetDBInfo_Image_NoMetaData)
{
    auto ac = createAC();
    DBImgInfo result = ac->getDBInfo("/tmp/pic.jpg", false);
    EXPECT_EQ(result.itemType, ItemTypePic);
}

// ============ getPicVideoCountFromPaths ============

TEST_F(AlbumControlTest, GetPicVideoCount_EmptyPaths)
{
    auto ac = createAC();
    QList<int> result = ac->getPicVideoCountFromPaths({}, "/dev/sdb1");
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], 0);
    EXPECT_EQ(result[1], 0);
}

TEST_F(AlbumControlTest, GetPicVideoCount_NoDeviceCache)
{
    auto ac = createAC();
    QList<int> result = ac->getPicVideoCountFromPaths(
        {"file:///tmp/a.jpg", "file:///tmp/b.jpg"}, "/dev/nonexistent");
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0], 2);
    EXPECT_EQ(result[1], 0);
}


// ============ getClassificationData ============

TEST_F(AlbumControlTest, GetClassificationData_ServiceUnavailable)
{
    auto ac = createAC();
    stub.set_lamda(ADDR(Classifyutils, isDBusExist), [](Classifyutils *) -> bool { return false; });
    QVariantList result = ac->getClassificationData();
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(AlbumControlTest, GetClassificationData_HasUnclassified)
{
    auto ac = createAC();
    g_allPicInfos.clear();
    g_allPicInfos << makeInfo("/tmp/unclassified.jpg", ItemTypePic, "");
    QVariantList result = ac->getClassificationData();
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(AlbumControlTest, GetClassificationData_Classified)
{
    auto ac = createAC();
    g_allPicInfos.clear();
    g_allPicInfos << makeInfo("/tmp/pic1.jpg", ItemTypePic, "Scenery");
    g_classInfos.clear();
    g_classInfos << makeInfo("/tmp/pic1.jpg", ItemTypePic, "Scenery");

    QVariantList result = ac->getClassificationData();
    ASSERT_EQ(result.size(), 1);
    QVariantMap entry = result.first().toMap();
    EXPECT_EQ(entry.value("className").toString().toStdString(), "Scenery");
    EXPECT_EQ(entry.value("count").toInt(), 1);
    EXPECT_EQ(entry.value("thumbnail").toString().toStdString(), "/tmp/pic1.jpg");
}

TEST_F(AlbumControlTest, GetClassificationData_NoImagesInClass)
{
    auto ac = createAC();
    g_allPicInfos.clear();
    g_allPicInfos << makeInfo("/tmp/pic.jpg", ItemTypePic, "Other");
    g_classInfos.clear();
    QVariantList result = ac->getClassificationData();
    EXPECT_TRUE(result.isEmpty());
}

// ============ getTimelinesTitle (public method) ============

TEST_F(AlbumControlTest, GetTimelinesTitle_All)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_timelines.clear();
    g_timelines << dt;
    g_timelineInfos.clear();
    g_timelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic);

    QStringList titles = ac->getTimelinesTitle(AlbumControl::All, 0);
    ASSERT_EQ(titles.size(), 1);

    auto &map = access_private_field::AlbumControlm_timeLinePathsMap(*ac);
    EXPECT_EQ(map.size(), 1);
}

TEST_F(AlbumControlTest, GetTimelinesTitle_Import)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_importTimelines.clear();
    g_importTimelines << dt;
    g_importTimelineInfos.clear();
    g_importTimelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic);

    QStringList titles = ac->getTimelinesTitle(AlbumControl::Import, 0);
    ASSERT_EQ(titles.size(), 1);

    auto &map = access_private_field::AlbumControlm_importTimeLinePathsMap(*ac);
    EXPECT_EQ(map.size(), 1);
}

TEST_F(AlbumControlTest, GetTimelinesTitle_Year)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_timelines.clear();
    g_timelines << dt;
    g_timelineInfos.clear();
    g_timelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic);

    QStringList titles = ac->getTimelinesTitle(AlbumControl::Year, 0);
    ASSERT_EQ(titles.size(), 1);

    auto &map = access_private_field::AlbumControlm_yearDateMap(*ac);
    EXPECT_EQ(map.size(), 1);
}

// ============ getTimelinesTitleInfos ============

TEST_F(AlbumControlTest, GetTimelinesTitleInfos_All)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_timelines.clear();
    g_timelines << dt;
    g_timelineInfos.clear();
    g_timelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic);

    QVariantMap result = ac->getTimelinesTitleInfos(0);
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(AlbumControlTest, GetTimelinesTitleInfos_FilterPicOnly)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_timelines.clear();
    g_timelines << dt;
    g_timelineInfos.clear();
    g_timelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic)
                    << makeInfo("/tmp/vid.mp4", ItemTypeVideo);

    QVariantMap result = ac->getTimelinesTitleInfos(1);
    ASSERT_FALSE(result.isEmpty());
    for (const auto &val : result) {
        for (const auto &item : val.toList()) {
            EXPECT_NE(item.toMap().value("itemType").toString().toStdString(), "video");
        }
    }
}

// ============ getDayTimelinesInfos ============

TEST_F(AlbumControlTest, GetDayTimelinesInfos_NonEmpty)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_timelines.clear();
    g_timelines << dt;
    g_timelineInfos.clear();
    g_timelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic);

    QVariantMap result = ac->getDayTimelinesInfos(0);
    EXPECT_FALSE(result.isEmpty());

    auto &dayMap = access_private_field::AlbumControlm_dayDateMap(*ac);
    EXPECT_FALSE(dayMap.isEmpty());
}

TEST_F(AlbumControlTest, GetDayTimelinesInfos_Empty)
{
    auto ac = createAC();
    g_timelines.clear();
    g_timelineInfos.clear();

    QVariantMap result = ac->getDayTimelinesInfos(0);
    EXPECT_TRUE(result.isEmpty());
}

// ============ getMonthTimelinesInfos ============

TEST_F(AlbumControlTest, GetMonthTimelinesInfos_NonEmpty)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_timelines.clear();
    g_timelines << dt;
    g_timelineInfos.clear();
    g_timelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic);

    QVariantMap result = ac->getMonthTimelinesInfos(0);
    EXPECT_FALSE(result.isEmpty());

    auto &monthMap = access_private_field::AlbumControlm_monthDateMap(*ac);
    EXPECT_FALSE(monthMap.isEmpty());
}

TEST_F(AlbumControlTest, GetMonthTimelinesInfos_FilterVideoOnly)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_timelines.clear();
    g_timelines << dt;
    g_timelineInfos.clear();
    g_timelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic)
                    << makeInfo("/tmp/vid.mp4", ItemTypeVideo);

    QVariantMap result = ac->getMonthTimelinesInfos(2);
    ASSERT_FALSE(result.isEmpty());
    for (const auto &val : result) {
        for (const auto &item : val.toList()) {
            EXPECT_NE(item.toMap().value("itemType").toString().toStdString(), "picture");
        }
    }
}

// ============ getAllCustomAlbumName ============

TEST_F(AlbumControlTest, GetAllCustomAlbumName_Empty)
{
    auto ac = createAC();
    g_albumNames.clear();
    QList<QString> result = ac->getAllCustomAlbumName();
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(AlbumControlTest, GetAllCustomAlbumName_MultipleAlbums)
{
    auto ac = createAC();
    g_albumNames.clear();
    g_albumNames << std::make_pair(1, QString("Album1"))
                 << std::make_pair(2, QString("Album2"))
                 << std::make_pair(3, QString("Album3"));
    QList<QString> result = ac->getAllCustomAlbumName();
    ASSERT_EQ(result.size(), 3);
    EXPECT_EQ(result[0].toStdString(), "Album1");
    EXPECT_EQ(result[1].toStdString(), "Album2");
    EXPECT_EQ(result[2].toStdString(), "Album3");
    auto &customAlbum = access_private_field::AlbumControlm_customAlbum(*ac);
    EXPECT_EQ(customAlbum.size(), 3);
}

// ============ getImportAlubumAllId ============

TEST_F(AlbumControlTest, GetImportAlubumAllId_Empty)
{
    auto ac = createAC();
    g_autoImportUIDs.clear();
    QList<int> result = ac->getImportAlubumAllId();
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(AlbumControlTest, GetImportAlubumAllId_MultipleEntries)
{
    auto ac = createAC();
    g_autoImportUIDs.clear();
    g_autoImportUIDs.insert(10, "/path/a");
    g_autoImportUIDs.insert(20, "/path/b");
    QList<int> result = ac->getImportAlubumAllId();
    ASSERT_EQ(result.size(), 2);
    EXPECT_TRUE(result.contains(10));
    EXPECT_TRUE(result.contains(20));
}

// ============ getImportTimelinesTitleInfos ============

TEST_F(AlbumControlTest, GetImportTimelinesTitleInfos_NonEmpty)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_importTimelines.clear();
    g_importTimelines << dt;
    g_importTimelineInfos.clear();
    g_importTimelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic);

    QVariantMap result = ac->getImportTimelinesTitleInfos(0);
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(AlbumControlTest, GetImportTimelinesTitleInfos_FilterPicOnly)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_importTimelines.clear();
    g_importTimelines << dt;
    g_importTimelineInfos.clear();
    g_importTimelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic)
                          << makeInfo("/tmp/vid.mp4", ItemTypeVideo);

    QVariantMap result = ac->getImportTimelinesTitleInfos(1);
    ASSERT_FALSE(result.isEmpty());
    for (const auto &val : result) {
        for (const auto &item : val.toList()) {
            EXPECT_NE(item.toMap().value("itemType").toString().toStdString(), "video");
        }
    }
}

TEST_F(AlbumControlTest, GetImportTimelinesTitleInfos_FilterVideoOnly)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_importTimelines.clear();
    g_importTimelines << dt;
    g_importTimelineInfos.clear();
    g_importTimelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic)
                          << makeInfo("/tmp/vid.mp4", ItemTypeVideo);

    QVariantMap result = ac->getImportTimelinesTitleInfos(2);
    ASSERT_FALSE(result.isEmpty());
    for (const auto &val : result) {
        for (const auto &item : val.toList()) {
            EXPECT_NE(item.toMap().value("itemType").toString().toStdString(), "pciture");
        }
    }
}

TEST_F(AlbumControlTest, GetImportTimelinesTitleInfos_Empty)
{
    auto ac = createAC();
    g_importTimelines.clear();
    g_importTimelineInfos.clear();
    QVariantMap result = ac->getImportTimelinesTitleInfos(0);
    EXPECT_TRUE(result.isEmpty());
}

// ============ getNewAlbumName ============

TEST_F(AlbumControlTest, GetNewAlbumName_EmptyBaseName)
{
    auto ac = createAC();
    g_albumNames.clear();
    QString result = ac->getNewAlbumName("");
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(AlbumControlTest, GetNewAlbumName_NameNotInUse)
{
    auto ac = createAC();
    g_albumNames.clear();
    g_albumNames << std::make_pair(1, QString("Existing"));
    QString result = ac->getNewAlbumName("NewAlbum");
    EXPECT_EQ(result.toStdString(), "NewAlbum");
}

TEST_F(AlbumControlTest, GetNewAlbumName_NameInUse_GeneratesSuffix)
{
    auto ac = createAC();
    g_albumNames.clear();
    g_albumNames << std::make_pair(1, QString("Album"))
                 << std::make_pair(2, QString("Album1"));
    QString result = ac->getNewAlbumName("Album");
    EXPECT_EQ(result.toStdString(), "Album2");
}

TEST_F(AlbumControlTest, GetNewAlbumName_NameWithGap)
{
    auto ac = createAC();
    g_albumNames.clear();
    g_albumNames << std::make_pair(1, QString("Album"))
                 << std::make_pair(2, QString("Album1"))
                 << std::make_pair(3, QString("Album3"));
    QString result = ac->getNewAlbumName("Album");
    EXPECT_EQ(result.toStdString(), "Album2");
}

// ============ getYearTimelinesInfos ============

TEST_F(AlbumControlTest, GetYearTimelinesInfos_NonEmpty)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_timelines.clear();
    g_timelines << dt;
    g_timelineInfos.clear();
    g_timelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic);

    QVariantMap result = ac->getYearTimelinesInfos(0);
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(AlbumControlTest, GetYearTimelinesInfos_FilterPicOnly)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_timelines.clear();
    g_timelines << dt;
    g_timelineInfos.clear();
    g_timelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic)
                    << makeInfo("/tmp/vid.mp4", ItemTypeVideo);

    QVariantMap result = ac->getYearTimelinesInfos(1);
    ASSERT_FALSE(result.isEmpty());
    for (const auto &val : result) {
        for (const auto &item : val.toList()) {
            EXPECT_NE(item.toMap().value("itemType").toString().toStdString(), "video");
        }
    }
}

TEST_F(AlbumControlTest, GetYearTimelinesInfos_Empty)
{
    auto ac = createAC();
    g_timelines.clear();
    g_timelineInfos.clear();
    QVariantMap result = ac->getYearTimelinesInfos(0);
    EXPECT_TRUE(result.isEmpty());
}

// ============ getTrashInfos ============

static QString g_trashTempDir = QDir::tempPath() + "/ut_trash_test";

static void createTrashTempFile(const QString &path)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write("x");
    f.close();
}

static void cleanupTrashTempDir()
{
    QDir dir(g_trashTempDir);
    dir.removeRecursively();
}

TEST_F(AlbumControlTest, GetTrashInfos_Empty)
{
    auto ac = createAC();
    g_trashInfos.clear();
    DBImgInfoList result = ac->getTrashInfos(0);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(AlbumControlTest, GetTrashInfos_FilesExist_ReturnsAll)
{
    auto ac = createAC();
    cleanupTrashTempDir();
    QString p1 = g_trashTempDir + "/pic1.jpg";
    QString p2 = g_trashTempDir + "/vid1.mp4";
    createTrashTempFile(p1);
    createTrashTempFile(p2);

    g_trashInfos.clear();
    g_trashInfos << makeInfo(p1, ItemTypePic)
                 << makeInfo(p2, ItemTypeVideo);
    DBImgInfoList result = ac->getTrashInfos(0);
    EXPECT_EQ(result.size(), 2);
    cleanupTrashTempDir();
}

TEST_F(AlbumControlTest, GetTrashInfos_RemainDaysZero_Removed)
{
    auto ac = createAC();
    cleanupTrashTempDir();
    QString p1 = g_trashTempDir + "/pic1.jpg";
    createTrashTempFile(p1);

    DBImgInfo info = makeInfo(p1, ItemTypePic);
    info.remainDays = 0;
    g_trashInfos.clear();
    g_trashInfos << info;
    DBImgInfoList result = ac->getTrashInfos(0);
    EXPECT_TRUE(result.isEmpty());
    cleanupTrashTempDir();
}

TEST_F(AlbumControlTest, GetTrashInfos_FilterType2_PicturesRemoved)
{
    auto ac = createAC();
    cleanupTrashTempDir();
    QString p1 = g_trashTempDir + "/pic1.jpg";
    QString p2 = g_trashTempDir + "/vid1.mp4";
    createTrashTempFile(p1);
    createTrashTempFile(p2);

    g_trashInfos.clear();
    g_trashInfos << makeInfo(p1, ItemTypePic)
                 << makeInfo(p2, ItemTypeVideo);
    DBImgInfoList result = ac->getTrashInfos(2);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].itemType, ItemTypeVideo);
    cleanupTrashTempDir();
}

TEST_F(AlbumControlTest, GetTrashInfos_FilterType1_VideosRemoved)
{
    auto ac = createAC();
    cleanupTrashTempDir();
    QString p1 = g_trashTempDir + "/pic1.jpg";
    QString p2 = g_trashTempDir + "/vid1.mp4";
    createTrashTempFile(p1);
    createTrashTempFile(p2);

    g_trashInfos.clear();
    g_trashInfos << makeInfo(p1, ItemTypePic)
                 << makeInfo(p2, ItemTypeVideo);
    DBImgInfoList result = ac->getTrashInfos(1);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].itemType, ItemTypePic);
    cleanupTrashTempDir();
}

TEST_F(AlbumControlTest, GetTrashInfos_NonExistentFile_Removed)
{
    auto ac = createAC();
    g_trashInfos.clear();
    g_trashInfos << makeInfo("/nonexistent/path/pic.jpg", ItemTypePic);
    DBImgInfoList result = ac->getTrashInfos(0);
    EXPECT_TRUE(result.isEmpty());
}

// ============ getTrashAlbumInfos ============

TEST_F(AlbumControlTest, GetTrashAlbumInfos_NonEmpty)
{
    auto ac = createAC();
    cleanupTrashTempDir();
    QString p1 = g_trashTempDir + "/pic1.jpg";
    createTrashTempFile(p1);

    g_trashInfos.clear();
    g_trashInfos << makeInfo(p1, ItemTypePic);
    QVariantMap result = ac->getTrashAlbumInfos(0);
    EXPECT_FALSE(result.isEmpty());
    cleanupTrashTempDir();
}

TEST_F(AlbumControlTest, GetTrashAlbumInfos_Empty)
{
    auto ac = createAC();
    g_trashInfos.clear();
    QVariantMap result = ac->getTrashAlbumInfos(0);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(AlbumControlTest, GetTrashAlbumInfos_FilterType2_VideoOnly)
{
    auto ac = createAC();
    cleanupTrashTempDir();
    QString p1 = g_trashTempDir + "/pic1.jpg";
    QString p2 = g_trashTempDir + "/vid1.mp4";
    createTrashTempFile(p1);
    createTrashTempFile(p2);

    g_trashInfos.clear();
    g_trashInfos << makeInfo(p1, ItemTypePic)
                 << makeInfo(p2, ItemTypeVideo);
    QVariantMap result = ac->getTrashAlbumInfos(2);
    ASSERT_FALSE(result.isEmpty());
    for (const auto &val : result) {
        for (const auto &item : val.toList()) {
            EXPECT_NE(item.toMap().value("itemType").toString().toStdString(), "pciture");
        }
    }
    cleanupTrashTempDir();
}

// ============ urls2localPaths ============

TEST_F(AlbumControlTest, Urls2localPaths_Empty)
{
    auto ac = createAC();
    QStringList result = ac->urls2localPaths({});
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(AlbumControlTest, Urls2localPaths_MultipleUrls)
{
    auto ac = createAC();
    QStringList urls = {"file:///tmp/a.jpg", "file:///tmp/b.png"};
    QStringList result = ac->urls2localPaths(urls);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].toStdString(), "/tmp/a.jpg");
    EXPECT_EQ(result[1].toStdString(), "/tmp/b.png");
}

// ============ onNewAPPOpen ============

TEST_F(AlbumControlTest, OnNewAPPOpen_EmptyArguments)
{
    auto ac = createAC();
    ac->onNewAPPOpen(0, {});
}

TEST_F(AlbumControlTest, OnNewAPPOpen_ValidImagePath)
{
    auto ac = createAC();
    bool emitted = false;
    QObject::connect(ac.get(), &AlbumControl::sigOpenImageFromFiles,
        [&emitted](const QStringList &) { emitted = true; });
    ac->onNewAPPOpen(0, {"appname", "file:///tmp/test.jpg"});
    EXPECT_TRUE(emitted);
}

TEST_F(AlbumControlTest, OnNewAPPOpen_InvalidFormat)
{
    auto ac = createAC();
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isImage),
        [](const QString &) -> bool { return false; });
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isVideo),
        [](QString) -> bool { return false; });
    bool emitted = false;
    QObject::connect(ac.get(), &AlbumControl::sigInvalidFormat,
        [&emitted]() { emitted = true; });
    ac->onNewAPPOpen(0, {"appname", "file:///tmp/test.unknown"});
    EXPECT_TRUE(emitted);
}

// ============ startMonitor ============

TEST_F(AlbumControlTest, StartMonitor_EmptyPaths)
{
    auto ac = createAC();
    stub.set_lamda(ADDR(DBManager, getDefaultNotifyPaths_group),
        []() -> std::tuple<QList<QStringList>, QStringList, QList<int>> { return {}; });
    stub.set_lamda(ADDR(DBManager, getAllPaths),
        [](DBManager *, const ItemType &) -> QStringList { return {}; });
    stub.set_lamda(ADDR(DBManager, removeImgInfos),
        [](DBManager *, const QStringList &) -> void {});
    ac->startMonitor();
}

TEST_F(AlbumControlTest, StartMonitor_NonExistentCustomPath)
{
    auto ac = createAC();
    stub.set_lamda(ADDR(DBManager, getDefaultNotifyPaths_group),
        []() -> std::tuple<QList<QStringList>, QStringList, QList<int>> { return {}; });
    stub.set_lamda(ADDR(DBManager, getAllPaths),
        [](DBManager *, const ItemType &) -> QStringList { return {}; });
    stub.set_lamda(ADDR(DBManager, removeImgInfos),
        [](DBManager *, const QStringList &) -> void {});
    g_autoImportUIDs.clear();
    g_autoImportUIDs[1] = "/nonexistent/path/ut_12345";
    bool removeCalled = false;
    stub.set_lamda(ADDR(DBManager, removeCustomAutoImportPath),
        [&removeCalled](DBManager *, int) -> void { removeCalled = true; });
    ac->startMonitor();
    EXPECT_TRUE(removeCalled);
}

// ============ importFromMountDevice ============

static bool g_insertImgInfosCalled = false;
static bool g_insertIntoAlbumCalled = false;

TEST_F(AlbumControlTest, ImportFromMountDevice_EmptyPaths)
{
    auto ac = createAC();

    g_insertImgInfosCalled = false;
    g_insertIntoAlbumCalled = false;

    stub.set_lamda(ADDR(DBManager, insertImgInfos),
        [](DBManager *, const DBImgInfoList &) -> void {
            g_insertImgInfosCalled = true;
        });
    stub.set_lamda(ADDR(DBManager, insertIntoAlbum),
        [](DBManager *, int, const QStringList &, AlbumDBType) -> bool {
            g_insertIntoAlbumCalled = true;
            return true;
        });

    // Intercept QThread::start to capture the QThread pointer that
    // importFromMountDevice creates, without actually starting it.
    // This lets us synchronize deterministically after the call returns.
    QThread *capturedThread = nullptr;
    stub.set_lamda(ADDR(QThread, start),
        [&capturedThread](QThread *self, QThread::Priority) {
            capturedThread = self;
        });

    ac->importFromMountDevice({}, 0);

    // Restore real QThread::start, then start the captured thread and
    // block until it finishes so the test observes the async result.
    stub.reset(ADDR(QThread, start));
    ASSERT_NE(capturedThread, nullptr)
        << "importFromMountDevice should have created a QThread";
    capturedThread->start();
    capturedThread->wait(5000);

    EXPECT_FALSE(g_insertImgInfosCalled)
        << "Empty paths should not trigger DBManager::insertImgInfos";
    EXPECT_FALSE(g_insertIntoAlbumCalled)
        << "Empty paths should not trigger DBManager::insertIntoAlbum";
}

// ============ onMounted ============

TEST_F(AlbumControlTest, OnMounted_SambaPath)
{
    auto ac = createAC();
    stub.set_lamda(ADDR(DeviceHelper, isSamba),
        [](const QUrl &) -> bool { return true; });
    ac->onMounted("smb://server/share", "/mnt/smb", DeviceType::kBlockDevice);
    auto &map = access_private_field::AlbumControlm_durlAndNameMap(*ac);
    EXPECT_TRUE(map.isEmpty());
}

TEST_F(AlbumControlTest, OnMounted_EmptyDeviceInfo)
{
    auto ac = createAC();
    stub.set_lamda(ADDR(DeviceHelper, isSamba),
        [](const QUrl &) -> bool { return false; });
    stub.set_lamda(ADDR(DeviceHelper, instance),
        []() -> DeviceHelper * { return nullptr; });
    g_deviceInfo.clear();
    stub.set_lamda(ADDR(DeviceHelper, loadDeviceInfo),
        [](DeviceHelper *, const QString &, bool) -> QVariantMap { return g_deviceInfo; });
    ac->onMounted("file:///dev/sdb1", "/mnt/usb", DeviceType::kBlockDevice);
    auto &map = access_private_field::AlbumControlm_durlAndNameMap(*ac);
    EXPECT_TRUE(map.isEmpty());
}

TEST_F(AlbumControlTest, OnMounted_ValidBlockDevice)
{
    auto ac = createAC();
    stub.set_lamda(ADDR(DeviceHelper, isSamba),
        [](const QUrl &) -> bool { return false; });
    stub.set_lamda(ADDR(DeviceHelper, instance),
        []() -> DeviceHelper * { return nullptr; });
    g_deviceInfo.clear();
    g_deviceInfo["DeviceType"] = static_cast<int>(DeviceType::kBlockDevice);
    g_deviceInfo["MountPoint"] = "/mnt/usb";
    g_deviceInfo["IdLabel"] = "USBDrive";
    stub.set_lamda(ADDR(DeviceHelper, loadDeviceInfo),
        [](DeviceHelper *, const QString &, bool) -> QVariantMap { return g_deviceInfo; });
    stub.set_lamda(ADDR(DeviceHelper, loadAllDeviceInfos),
        [](DeviceHelper *) -> void {});
    stub.set_lamda(ADDR(AlbumControl, findPicturePathByPhone),
        [](AlbumControl *, QString &) -> bool { return true; });
    ac->onMounted("file:///dev/sdb1", "/mnt/usb", DeviceType::kBlockDevice);
    auto &map = access_private_field::AlbumControlm_durlAndNameMap(*ac);
    EXPECT_FALSE(map.isEmpty());
}

TEST_F(AlbumControlTest, OnMounted_AlreadyMounted)
{
    auto ac = createAC();
    stub.set_lamda(ADDR(DeviceHelper, isSamba),
        [](const QUrl &) -> bool { return false; });
    stub.set_lamda(ADDR(DeviceHelper, instance),
        []() -> DeviceHelper * { return nullptr; });
    g_deviceInfo.clear();
    g_deviceInfo["DeviceType"] = static_cast<int>(DeviceType::kBlockDevice);
    g_deviceInfo["MountPoint"] = "/mnt/usb";
    g_deviceInfo["IdLabel"] = "USBDrive";
    stub.set_lamda(ADDR(DeviceHelper, loadDeviceInfo),
        [](DeviceHelper *, const QString &, bool) -> QVariantMap { return g_deviceInfo; });
    auto &map = access_private_field::AlbumControlm_durlAndNameMap(*ac);
    map.insert("/mnt/usb", "USBDrive");
    ac->onMounted("file:///dev/sdb1", "/mnt/usb", DeviceType::kBlockDevice);
    EXPECT_EQ(map.size(), 1);
}

// ============ updateBlockDeviceName ============

TEST_F(AlbumControlTest, UpdateBlockDeviceName_EmptyDeviceInfo)
{
    auto ac = createAC();
    stub.set_lamda(ADDR(DeviceHelper, instance),
        []() -> DeviceHelper * { return nullptr; });
    g_deviceInfo.clear();
    stub.set_lamda(ADDR(DeviceHelper, loadDeviceInfo),
        [](DeviceHelper *, const QString &, bool) -> QVariantMap { return g_deviceInfo; });
    call_private_fun::AlbumControlupdateBlockDeviceName(*ac, QString("/dev/sdb1"));
    auto &map = access_private_field::AlbumControlm_blkPath2DeviceNameMap(*ac);
    EXPECT_TRUE(map.isEmpty());
}

TEST_F(AlbumControlTest, UpdateBlockDeviceName_WithLabel)
{
    auto ac = createAC();
    stub.set_lamda(ADDR(DeviceHelper, instance),
        []() -> DeviceHelper * { return nullptr; });
    g_deviceInfo.clear();
    g_deviceInfo["MountPoints"] = QStringList{"/mnt/usb"};
    g_deviceInfo["SizeTotal"] = 1000000;
    g_deviceInfo["IdLabel"] = "MyUSB";
    g_deviceInfo["IdType"] = "ext4";
    stub.set_lamda(ADDR(DeviceHelper, loadDeviceInfo),
        [](DeviceHelper *, const QString &, bool) -> QVariantMap { return g_deviceInfo; });
    call_private_fun::AlbumControlupdateBlockDeviceName(*ac, QString("/dev/sdb1"));
    auto &map = access_private_field::AlbumControlm_blkPath2DeviceNameMap(*ac);
    ASSERT_EQ(map.size(), 1);
    EXPECT_EQ(map.value("/mnt/usb").toStdString(), "MyUSB");
}

TEST_F(AlbumControlTest, UpdateBlockDeviceName_I18nLabel)
{
    auto ac = createAC();
    stub.set_lamda(ADDR(DeviceHelper, instance),
        []() -> DeviceHelper * { return nullptr; });
    g_deviceInfo.clear();
    g_deviceInfo["MountPoints"] = QStringList{"/mnt/usb"};
    g_deviceInfo["SizeTotal"] = 1000000;
    g_deviceInfo["IdLabel"] = "_dde_test";
    g_deviceInfo["IdType"] = "ext4";
    stub.set_lamda(ADDR(DeviceHelper, loadDeviceInfo),
        [](DeviceHelper *, const QString &, bool) -> QVariantMap { return g_deviceInfo; });
    call_private_fun::AlbumControlupdateBlockDeviceName(*ac, QString("/dev/sdb1"));
    auto &map = access_private_field::AlbumControlm_blkPath2DeviceNameMap(*ac);
    ASSERT_EQ(map.size(), 1);
    EXPECT_EQ(map.value("/mnt/usb").toStdString(), "test");
}
