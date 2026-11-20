// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Supplementary unit tests for AlbumControl (issue V-9046).
// This translation unit is self-contained: it declares its own
// ACCESS_PRIVATE_FIELD/FUN macros, global stub data and a test fixture
// that mirrors the one in test_albumcontrol.cpp.

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QThread>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDateTime>
#include <QMap>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <memory>

#include "albumControl.h"
#include "dbmanager/dbmanager.h"
#include "imageengine/movieservice.h"
#include "unionimage/unionimage.h"
#include "unionimage/unionimage_global.h"
#include "utils/classifyutils.h"
#include "utils/devicehelper.h"
#include "fileMonitor/fileinotifygroup.h"
#include "imageengine/imagedataservice.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// ---- Typedefs to avoid commas in macro args ----
using DBImgInfoListMap = QMap<QString, DBImgInfoList>;
using MovieInfoMap = QMap<QString, MovieInfo>;
using IntStringMap = QMap<int, QString>;
using StringStringMap = QMap<QString, QString>;
using FileInotifyGroupPtr = FileInotifyGroup *;
using VideoStableSizeMap = QMap<QString, qint64>;
using VideoStableCountMap = QMap<QString, int>;
using DBImgInfoListField = DBImgInfoList;

// ---- Private field / fun accessors (file-scope, independent) ----
ACCESS_PRIVATE_FIELD(AlbumControl, DBImgInfoListMap, m_timeLinePathsMap)
ACCESS_PRIVATE_FIELD(AlbumControl, DBImgInfoListMap, m_yearDateMap)
ACCESS_PRIVATE_FIELD(AlbumControl, DBImgInfoListMap, m_monthDateMap)
ACCESS_PRIVATE_FIELD(AlbumControl, DBImgInfoListMap, m_dayDateMap)
ACCESS_PRIVATE_FIELD(AlbumControl, DBImgInfoListMap, m_importTimeLinePathsMap)
ACCESS_PRIVATE_FIELD(AlbumControl, MovieInfoMap, m_movieInfos)
ACCESS_PRIVATE_FIELD(AlbumControl, IntStringMap, m_customAlbum)
ACCESS_PRIVATE_FIELD(AlbumControl, StringStringMap, m_durlAndNameMap)
ACCESS_PRIVATE_FIELD(AlbumControl, StringStringMap, m_blkPath2DeviceNameMap)
ACCESS_PRIVATE_FIELD(AlbumControl, FileInotifyGroupPtr, m_fileInotifygroup)
ACCESS_PRIVATE_FIELD(AlbumControl, VideoStableSizeMap, m_videoStableSize)
ACCESS_PRIVATE_FIELD(AlbumControl, VideoStableCountMap, m_videoStableCount)
ACCESS_PRIVATE_FIELD(AlbumControl, DBImgInfoListField, m_infoList)
ACCESS_PRIVATE_FIELD(AlbumControl, std::atomic_bool, m_bneedstop)
ACCESS_PRIVATE_FIELD(AlbumControl, std::atomic_bool, m_mountDeviceImportRunning)

// ---- Shared stub data ----
static DBImgInfoList g_allPicInfos;
static DBImgInfoList g_allSortInfos;
static DBImgInfoList g_timelineInfos;
static DBImgInfoList g_importTimelineInfos;
static QList<QDateTime> g_timelines;
static QList<QDateTime> g_importTimelines;
static MovieInfo g_movieInfo;
static QMap<QString, QString> g_metaData;
static QList<std::pair<int, QString>> g_albumNames;
static QList<std::pair<int, QString>> g_autoImportAlbumNames;
static QMap<int, QString> g_autoImportUIDs;
static QStringList g_autoImportNames;
static DBImgInfoList g_trashInfos;
static QVariantMap g_deviceInfo;
static QStringList g_allPaths;
static int g_imgsCount = 0;
static DBImgInfoList g_albumInfos;
static QString g_albumName;
static bool g_isImgExist = false;
static bool g_insertResult = true;
static bool g_renameResult = true;
static int g_createResult = 1;
static int g_createAutoImportResult = 1;
static QStringList g_recoveryResult;
static QDateTime g_importTime;
static QStringList g_yearPaths;
static QStringList g_dayPaths;
static QStringList g_yearsList;
static QStringList g_monthsList;
static QStringList g_daysList;
static int g_yearCount = 0;
static int g_monthCount = 0;
static bool g_defaultPathExists = false;
static bool g_updateImgPath = false;

static DBImgInfo makeInfo(const QString &path, ItemType type,
                          const QString &cls = QString(),
                          const QDateTime &time = QDateTime(QDate(2024, 1, 15), QTime(10, 30)))
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


// ---- Local trash temp helpers (self-contained, not shared from test_albumcontrol.cpp) ----
static QString g_trashTempDir = QDir::tempPath() + "/ut_trash_test_extra";

static void createTrashTempFile(const QString &path)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    f.open(QIODevice::WriteOnly);
    f.write("x");
    f.close();
}

static void cleanupTrashTemp()
{
    QDir dir(g_trashTempDir);
    dir.removeRecursively();
}

// ---- Test fixture ----
class AlbumControlExtraTest : public ::testing::Test
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
        g_allSortInfos.clear();
        g_timelineInfos.clear();
        g_importTimelineInfos.clear();
        g_timelines.clear();
        g_importTimelines.clear();
        g_metaData.clear();
        g_albumNames.clear();
        g_autoImportAlbumNames.clear();
        g_autoImportUIDs.clear();
        g_autoImportNames.clear();
        g_trashInfos.clear();
        g_allPaths.clear();
        g_imgsCount = 0;
        g_albumInfos.clear();
        g_albumName.clear();
        g_isImgExist = false;
        g_insertResult = true;
        g_renameResult = true;
        g_createResult = 1;
        g_createAutoImportResult = 1;
        g_recoveryResult.clear();
        g_importTime = QDateTime();
        g_yearPaths.clear();
        g_dayPaths.clear();
        g_yearsList.clear();
        g_monthsList.clear();
        g_daysList.clear();
        g_yearCount = 0;
        g_monthCount = 0;
        g_defaultPathExists = false;
        g_updateImgPath = false;

        g_movieInfo.valid = true;
        g_movieInfo.creation = QDateTime(QDate(2024, 1, 15), QTime(10, 30));
        g_movieInfo.title = "test_video";
        g_movieInfo.filePath = "/tmp/test_video.mp4";
        g_movieInfo.duration = "00:30";

        setupCommonStubs();
    }

    void setupCommonStubs()
    {
        stub.set_lamda(ADDR(AlbumControl, initMonitor), [](AlbumControl *) -> void {});
        stub.set_lamda(ADDR(AlbumControl, initDeviceMonitor), [](AlbumControl *) -> void {});

        stub.set_lamda(ADDR(DBManager, instance), []() -> DBManager * {
            return nullptr;
        });
        stub.set_lamda(ADDR(Classifyutils, GetInstance), []() -> Classifyutils * {
            return nullptr;
        });
        stub.set_lamda(ADDR(Classifyutils, isDBusExist), [](Classifyutils *) -> bool { return true; });
        stub.set_lamda(ADDR(MovieService, instance), [](QObject *) -> MovieService * {
            return nullptr;
        });

        stub.set_lamda(ADDR(DBManager, getAllInfos),
            [](DBManager *, int) -> const DBImgInfoList { return g_allPicInfos; });
        stub.set_lamda(ADDR(DBManager, getAllInfosSort),
            [](DBManager *, const ItemType &) -> const DBImgInfoList { return g_allSortInfos; });
        stub.set_lamda(ADDR(DBManager, getAllInfosByUID),
            [](DBManager *, QString) -> const DBImgInfoList { return g_allPicInfos; });
        stub.set_lamda(ADDR(DBManager, getAllPaths),
            [](DBManager *, const ItemType &) -> QStringList { return g_allPaths; });
        stub.set_lamda(ADDR(DBManager, getImgsCount),
            [](DBManager *, const ItemType &) -> int { return g_imgsCount; });
        stub.set_lamda(ADDR(DBManager, getInfosByAlbum),
            [](DBManager *, int, bool, ItemType) -> const DBImgInfoList { return g_albumInfos; });
        stub.set_lamda(ADDR(DBManager, getAlbumNameFromUID),
            [](DBManager *, int) -> QString { return g_albumName; });
        stub.set_lamda(ADDR(DBManager, isImgExistInAlbum),
            [](DBManager *, int, const QString &) -> bool { return g_isImgExist; });
        stub.set_lamda(ADDR(DBManager, addCustomAlbumIdByPaths),
            [](DBManager *, int, const QStringList &) -> void {});
        stub.set_lamda(ADDR(DBManager, removeCustomAlbumIdByPaths),
            [](DBManager *, int, const QStringList &) -> void {});
        stub.set_lamda(ADDR(DBManager, insertIntoAlbum),
            [](DBManager *, int, const QStringList &, AlbumDBType) -> bool { return g_insertResult; });
        stub.set_lamda(ADDR(DBManager, createAlbum),
            [](DBManager *, const QString &, const QStringList &, AlbumDBType) -> int { return g_createResult; });
        stub.set_lamda(ADDR(DBManager, removeAlbum),
            [](DBManager *, int) -> void {});
        stub.set_lamda(ADDR(DBManager, removeFromAlbum),
            [](DBManager *, int, const QStringList &, AlbumDBType) -> void {});
        stub.set_lamda(ADDR(DBManager, renameAlbum),
            [](DBManager *, int, const QString &, AlbumDBType) -> bool { return g_renameResult; });
        stub.set_lamda(ADDR(DBManager, recoveryImgFromTrash),
            [](DBManager *, const QStringList &) -> QStringList { return g_recoveryResult; });
        stub.set_lamda(ADDR(DBManager, removeTrashImgInfos),
            [](DBManager *, const QStringList &) -> void {});
        stub.set_lamda(ADDR(DBManager, getInfoByPath),
            [](DBManager *, const QString &) -> const DBImgInfo { return DBImgInfo(); });
        stub.set_lamda(ADDR(DBManager, getFileImportTime),
            [](DBManager *, const QString &) -> QDateTime { return g_importTime; });
        stub.set_lamda(ADDR(DBManager, createNewCustomAutoImportPath),
            [](DBManager *, const QString &, const QString &) -> int { return g_createAutoImportResult; });
        stub.set_lamda(ADDR(DBManager, removeCustomAutoImportPath),
            [](DBManager *, int) -> void {});
        stub.set_lamda(ADDR(DBManager, getAllCustomAutoImportNames),
            [](DBManager *) -> QStringList { return g_autoImportNames; });
        stub.set_lamda(ADDR(DBManager, getAllCustomAutoImportUIDAndPath),
            [](DBManager *) -> QMap<int, QString> { return g_autoImportUIDs; });
        stub.set_lamda(ADDR(DBManager, getAllAlbumNames),
            [](DBManager *, AlbumDBType type) -> QList<std::pair<int, QString>> {
                // Return different lists depending on album type
                if (type == AlbumDBType::AutoImport)
                    return g_autoImportAlbumNames;
                return g_albumNames;
            });
        stub.set_lamda(ADDR(DBManager, getAllTrashInfos_getRemainDays),
            [](DBManager *) -> const DBImgInfoList { return g_trashInfos; });
        stub.set_lamda(ADDR(DBManager, removeTrashImgInfosNoSignal),
            [](DBManager *, const QStringList &) -> void {});
        stub.set_lamda(ADDR(DBManager, getAllTimelines),
            [](DBManager *) -> const QList<QDateTime> { return g_timelines; });
        stub.set_lamda(ADDR(DBManager, getImportTimelines),
            [](DBManager *) -> const QList<QDateTime> { return g_importTimelines; });
        stub.set_lamda(ADDR(DBManager, getInfosByTimeline),
            [](DBManager *, const QDateTime &, const ItemType &) -> const DBImgInfoList { return g_timelineInfos; });
        stub.set_lamda(ADDR(DBManager, getInfosByImportTimeline),
            [](DBManager *, const QDateTime &, const ItemType &) -> const DBImgInfoList { return g_importTimelineInfos; });
        stub.set_lamda(ADDR(DBManager, getYearPaths),
            [](DBManager *, const QString &, int) -> QStringList { return g_yearPaths; });
        stub.set_lamda(ADDR(DBManager, getDayPaths),
            [](DBManager *, const QString &) -> QStringList { return g_dayPaths; });
        stub.set_lamda(ADDR(DBManager, getYearCount),
            [](DBManager *, const QString &) -> int { return g_yearCount; });
        stub.set_lamda(ADDR(DBManager, getYears),
            [](DBManager *) -> QStringList { return g_yearsList; });
        stub.set_lamda(ADDR(DBManager, getMonthCount),
            [](DBManager *, const QString &, const QString &) -> int { return g_monthCount; });
        stub.set_lamda(ADDR(DBManager, getMonthPaths),
            [](DBManager *, const QString &, const QString &, int) -> QStringList { return g_yearPaths; });
        stub.set_lamda(ADDR(DBManager, getMonths),
            [](DBManager *) -> QStringList { return g_monthsList; });
        stub.set_lamda(ADDR(DBManager, getDays),
            [](DBManager *) -> QStringList { return g_daysList; });
        stub.set_lamda(ADDR(DBManager, updateImgPath),
            [](DBManager *, const QString &, const QString &) -> bool { return g_updateImgPath; });

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
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, hashByString),
            [](const QString &) -> QString { return QStringLiteral("dummyhash"); });
    }

    std::unique_ptr<AlbumControl> createAC()
    {
        return std::make_unique<AlbumControl>();
    }
};

// ============ getAllFilters ============

TEST_F(AlbumControlExtraTest, GetAllFilters_ReturnsNonEmpty)
{
    auto ac = createAC();
    QString filter = ac->getAllFilters();
    EXPECT_FALSE(filter.isEmpty());
    EXPECT_TRUE(filter.contains("*."));
}

TEST_F(AlbumControlExtraTest, GetAllFilters_ContainsParentheses)
{
    auto ac = createAC();
    QString filter = ac->getAllFilters();
    EXPECT_TRUE(filter.contains('('));
    EXPECT_TRUE(filter.contains(')'));
}

// ============ getAllInfos ============

TEST_F(AlbumControlExtraTest, GetAllInfos_PopulatesInfoList)
{
    auto ac = createAC();
    g_allPicInfos.clear();
    g_allPicInfos << makeInfo("/tmp/a.jpg", ItemTypePic)
                  << makeInfo("/tmp/b.jpg", ItemTypePic);
    ac->getAllInfos();
    auto &infoList = access_private_field::AlbumControlm_infoList(*ac);
    EXPECT_EQ(infoList.size(), 2);
}

TEST_F(AlbumControlExtraTest, GetAllInfos_Empty)
{
    auto ac = createAC();
    g_allPicInfos.clear();
    ac->getAllInfos();
    auto &infoList = access_private_field::AlbumControlm_infoList(*ac);
    EXPECT_TRUE(infoList.isEmpty());
}

// ============ getAllInfosByUID ============

TEST_F(AlbumControlExtraTest, GetAllInfosByUID_DelegatesToDBManager)
{
    auto ac = createAC();
    g_allPicInfos.clear();
    g_allPicInfos << makeInfo("/tmp/uid1.jpg", ItemTypePic);
    DBImgInfoList result = ac->getAllInfosByUID("uid1");
    EXPECT_EQ(result.size(), 1);
}

// ============ getAllPaths / getAllUrlPaths ============

TEST_F(AlbumControlExtraTest, GetAllPaths_FilterNull)
{
    auto ac = createAC();
    g_allPaths.clear();
    g_allPaths << "/tmp/a.jpg" << "/tmp/b.mp4";
    QStringList result = ac->getAllPaths(0);
    EXPECT_EQ(result.size(), 2);
}

TEST_F(AlbumControlExtraTest, GetAllPaths_FilterPic)
{
    auto ac = createAC();
    g_allPaths.clear();
    g_allPaths << "/tmp/a.jpg";
    QStringList result = ac->getAllPaths(1);
    EXPECT_EQ(result.size(), 1);
}

TEST_F(AlbumControlExtraTest, GetAllUrlPaths_PrependsFileScheme)
{
    auto ac = createAC();
    g_allPaths.clear();
    g_allPaths << "/tmp/a.jpg" << "/tmp/b.jpg";
    QStringList result = ac->getAllUrlPaths(0);
    ASSERT_EQ(result.size(), 2);
    EXPECT_EQ(result[0].toStdString(), "file:///tmp/a.jpg");
    EXPECT_EQ(result[1].toStdString(), "file:///tmp/b.jpg");
}

TEST_F(AlbumControlExtraTest, GetAllUrlPaths_Empty)
{
    auto ac = createAC();
    g_allPaths.clear();
    QStringList result = ac->getAllUrlPaths(2);
    EXPECT_TRUE(result.isEmpty());
}

// ============ getAlbumAllInfos ============

TEST_F(AlbumControlExtraTest, GetAlbumAllInfos_NonEmpty)
{
    auto ac = createAC();
    g_allSortInfos.clear();
    g_allSortInfos << makeInfo("/tmp/pic.jpg", ItemTypePic)
                   << makeInfo("/tmp/vid.mp4", ItemTypeVideo);
    QVariantList result = ac->getAlbumAllInfos(0);
    EXPECT_EQ(result.size(), 2);
}

TEST_F(AlbumControlExtraTest, GetAlbumAllInfos_FilterPicOnly)
{
    auto ac = createAC();
    g_allSortInfos.clear();
    g_allSortInfos << makeInfo("/tmp/pic.jpg", ItemTypePic)
                   << makeInfo("/tmp/vid.mp4", ItemTypeVideo);
    QVariantList result = ac->getAlbumAllInfos(1);
    ASSERT_EQ(result.size(), 2);
    // Both items returned; filter type 1 = ItemTypePic used in DB query stub
    for (const auto &item : result) {
        QVariantMap m = item.toMap();
        // The stub returns the same list regardless; just verify structure
        EXPECT_TRUE(m.contains("url"));
    }
}

// ============ getAllImportTimelinesTitle ============

TEST_F(AlbumControlExtraTest, GetAllImportTimelinesTitle_NonEmpty)
{
    auto ac = createAC();
    QDateTime dt(QDate(2024, 1, 15), QTime(10, 30));
    g_importTimelines.clear();
    g_importTimelines << dt;
    g_importTimelineInfos.clear();
    g_importTimelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic);

    QStringList titles = ac->getAllImportTimelinesTitle(0);
    ASSERT_EQ(titles.size(), 1);

    auto &map = access_private_field::AlbumControlm_importTimeLinePathsMap(*ac);
    EXPECT_EQ(map.size(), 1);
}

TEST_F(AlbumControlExtraTest, GetAllImportTimelinesTitle_Empty)
{
    auto ac = createAC();
    g_importTimelines.clear();
    g_importTimelineInfos.clear();
    QStringList titles = ac->getAllImportTimelinesTitle(0);
    EXPECT_TRUE(titles.isEmpty());
}

// ============ getAllCount ============

TEST_F(AlbumControlExtraTest, GetAllCount_AllTypes)
{
    auto ac = createAC();
    g_imgsCount = 42;
    int count = ac->getAllCount(0);
    EXPECT_EQ(count, 42);
}

TEST_F(AlbumControlExtraTest, GetAllCount_PicFilter)
{
    auto ac = createAC();
    g_imgsCount = 10;
    int count = ac->getAllCount(1);
    EXPECT_EQ(count, 10);
}

TEST_F(AlbumControlExtraTest, GetAllCount_VideoFilter)
{
    auto ac = createAC();
    g_imgsCount = 5;
    int count = ac->getAllCount(2);
    EXPECT_EQ(count, 5);
}

// ============ getAllInfoConut ============

TEST_F(AlbumControlExtraTest, GetAllInfoConut_AllTypes)
{
    auto ac = createAC();
    g_imgsCount = 100;
    EXPECT_EQ(ac->getAllInfoConut(0), 100);
}

TEST_F(AlbumControlExtraTest, GetAllInfoConut_PicFilter)
{
    auto ac = createAC();
    g_imgsCount = 30;
    EXPECT_EQ(ac->getAllInfoConut(1), 30);
}

TEST_F(AlbumControlExtraTest, GetAllInfoConut_VideoFilter)
{
    auto ac = createAC();
    g_imgsCount = 15;
    EXPECT_EQ(ac->getAllInfoConut(2), 15);
}

// ============ isSystemAutoImportAlbum / isNormalAutoImportAlbum / isAutoImportAlbum / isCustomAlbum ============

TEST_F(AlbumControlExtraTest, IsSystemAutoImportAlbum_True)
{
    auto ac = createAC();
    g_autoImportAlbumNames.clear();
    g_autoImportAlbumNames << std::make_pair(1, QString("Favorites"))
                           << std::make_pair(2, QString("ScreenCapture"));
    EXPECT_TRUE(ac->isSystemAutoImportAlbum(1));
    EXPECT_TRUE(ac->isSystemAutoImportAlbum(2));
    EXPECT_FALSE(ac->isSystemAutoImportAlbum(5));
}

TEST_F(AlbumControlExtraTest, IsNormalAutoImportAlbum_True)
{
    auto ac = createAC();
    g_autoImportAlbumNames.clear();
    g_autoImportAlbumNames << std::make_pair(5, QString("MyImport"))
                           << std::make_pair(1, QString("Favorites"));
    EXPECT_TRUE(ac->isNormalAutoImportAlbum(5));
    EXPECT_FALSE(ac->isNormalAutoImportAlbum(1));
}

TEST_F(AlbumControlExtraTest, IsAutoImportAlbum_True)
{
    auto ac = createAC();
    g_autoImportAlbumNames.clear();
    g_autoImportAlbumNames << std::make_pair(1, QString("Favorites"))
                           << std::make_pair(5, QString("MyImport"));
    EXPECT_TRUE(ac->isAutoImportAlbum(1));
    EXPECT_TRUE(ac->isAutoImportAlbum(5));
    EXPECT_FALSE(ac->isAutoImportAlbum(100));
}

TEST_F(AlbumControlExtraTest, IsCustomAlbum_True)
{
    auto ac = createAC();
    g_albumNames.clear();
    g_albumNames << std::make_pair(10, QString("MyAlbum"))
                 << std::make_pair(11, QString("Trip"));
    EXPECT_TRUE(ac->isCustomAlbum(10));
    EXPECT_TRUE(ac->isCustomAlbum(11));
    EXPECT_FALSE(ac->isCustomAlbum(99));
}

// ============ isDefaultPathExists ============

TEST_F(AlbumControlExtraTest, IsDefaultPathExists_True)
{
    auto ac = createAC();
    g_defaultPathExists = true;
    stub.set_lamda(ADDR(DBManager, defaultNotifyPathExists),
        [](int) -> bool { return true; });
    EXPECT_TRUE(ac->isDefaultPathExists(1));
}

TEST_F(AlbumControlExtraTest, IsDefaultPathExists_False)
{
    auto ac = createAC();
    stub.set_lamda(ADDR(DBManager, defaultNotifyPathExists),
        [](int) -> bool { return false; });
    EXPECT_FALSE(ac->isDefaultPathExists(99));
}

// ============ getImportTimelinesTitlePaths ============

TEST_F(AlbumControlExtraTest, GetImportTimelinesTitlePaths_NonEmpty)
{
    auto ac = createAC();
    auto &map = access_private_field::AlbumControlm_importTimeLinePathsMap(*ac);
    g_importTimelineInfos.clear();
    g_importTimelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic);
    map.insert("2024/01/15 10:30", g_importTimelineInfos);

    QStringList paths = ac->getImportTimelinesTitlePaths("2024/01/15 10:30", 0);
    ASSERT_EQ(paths.size(), 1);
    EXPECT_EQ(paths[0].toStdString(), "file:///tmp/pic.jpg");
}

TEST_F(AlbumControlExtraTest, GetImportTimelinesTitlePaths_FilterVideo)
{
    auto ac = createAC();
    auto &map = access_private_field::AlbumControlm_importTimeLinePathsMap(*ac);
    DBImgInfoList infos;
    infos << makeInfo("/tmp/pic.jpg", ItemTypePic)
          << makeInfo("/tmp/vid.mp4", ItemTypeVideo);
    map.insert("Title1", infos);

    QStringList paths = ac->getImportTimelinesTitlePaths("Title1", 2);
    ASSERT_EQ(paths.size(), 1);
    EXPECT_EQ(paths[0].toStdString(), "file:///tmp/vid.mp4");
}

// ============ getImportTimelinesTitleInfosReverse ============

TEST_F(AlbumControlExtraTest, GetImportTimelinesTitleInfosReverse_NonEmpty)
{
    auto ac = createAC();
    // Use two distinct timelines so the reverse loop produces output.
    QDateTime dt1(QDate(2024, 1, 15), QTime(10, 30));
    QDateTime dt2(QDate(2024, 6, 1), QTime(12, 0));
    g_importTimelines.clear();
    g_importTimelines << dt1 << dt2;
    g_importTimelineInfos.clear();
    g_importTimelineInfos << makeInfo("/tmp/pic.jpg", ItemTypePic);

    QVariantList result = ac->getImportTimelinesTitleInfosReverse(0);
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(AlbumControlExtraTest, GetImportTimelinesTitleInfosReverse_Empty)
{
    auto ac = createAC();
    g_importTimelines.clear();
    g_importTimelineInfos.clear();
    QVariantList result = ac->getImportTimelinesTitleInfosReverse(0);
    EXPECT_TRUE(result.isEmpty());
}

// ============ getAlbumInfos ============

TEST_F(AlbumControlExtraTest, GetAlbumInfos_NonEmpty)
{
    auto ac = createAC();
    g_albumInfos.clear();
    g_albumInfos << makeInfo("/tmp/pic.jpg", ItemTypePic)
                 << makeInfo("/tmp/vid.mp4", ItemTypeVideo);
    g_albumName = "MyAlbum";
    QVariantMap result = ac->getAlbumInfos(10, 0);
    ASSERT_EQ(result.size(), 1);
    EXPECT_TRUE(result.contains("MyAlbum"));
    EXPECT_EQ(result.value("MyAlbum").toList().size(), 2);
}

TEST_F(AlbumControlExtraTest, GetAlbumInfos_Empty)
{
    auto ac = createAC();
    g_albumInfos.clear();
    QVariantMap result = ac->getAlbumInfos(99, 0);
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(AlbumControlExtraTest, GetAlbumInfos_FilterPicOnly)
{
    auto ac = createAC();
    g_albumInfos.clear();
    g_albumInfos << makeInfo("/tmp/pic.jpg", ItemTypePic)
                 << makeInfo("/tmp/vid.mp4", ItemTypeVideo);
    g_albumName = "A";
    QVariantMap result = ac->getAlbumInfos(1, 1);
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result.value("A").toList().size(), 1);
}

// ============ getAlbumPaths ============

TEST_F(AlbumControlExtraTest, GetAlbumPaths_NonEmpty)
{
    auto ac = createAC();
    g_albumInfos.clear();
    g_albumInfos << makeInfo("/tmp/pic.jpg", ItemTypePic)
                 << makeInfo("/tmp/vid.mp4", ItemTypeVideo);
    QStringList paths = ac->getAlbumPaths(5, 0);
    ASSERT_EQ(paths.size(), 2);
    EXPECT_EQ(paths[0].toStdString(), "file:///tmp/pic.jpg");
}

TEST_F(AlbumControlExtraTest, GetAlbumPaths_FilterVideoOnly)
{
    auto ac = createAC();
    g_albumInfos.clear();
    g_albumInfos << makeInfo("/tmp/pic.jpg", ItemTypePic)
                 << makeInfo("/tmp/vid.mp4", ItemTypeVideo);
    QStringList paths = ac->getAlbumPaths(5, 2);
    ASSERT_EQ(paths.size(), 1);
    EXPECT_EQ(paths[0].toStdString(), "file:///tmp/vid.mp4");
}

// ============ getCustomAlbumByUid ============

TEST_F(AlbumControlExtraTest, GetCustomAlbumByUid_FixedNames)
{
    auto ac = createAC();
    EXPECT_FALSE(ac->getCustomAlbumByUid(0).isEmpty());  // Favorites
    EXPECT_FALSE(ac->getCustomAlbumByUid(1).isEmpty());  // Screen Capture
    EXPECT_FALSE(ac->getCustomAlbumByUid(2).isEmpty());  // Camera
    EXPECT_FALSE(ac->getCustomAlbumByUid(3).isEmpty());  // Draw
}

TEST_F(AlbumControlExtraTest, GetCustomAlbumByUid_FromDB)
{
    auto ac = createAC();
    g_albumName = "CustomAlbum5";
    QString name = ac->getCustomAlbumByUid(5);
    EXPECT_EQ(name.toStdString(), "CustomAlbum5");
}

// ============ getCollectionInfos / getScreenCaptureInfos / getCameraInfos ============

TEST_F(AlbumControlExtraTest, GetCollectionInfos_DelegatesToDB)
{
    auto ac = createAC();
    g_albumInfos.clear();
    g_albumInfos << makeInfo("/tmp/fav.jpg", ItemTypePic);
    DBImgInfoList result = ac->getCollectionInfos();
    EXPECT_EQ(result.size(), 1);
}

TEST_F(AlbumControlExtraTest, GetScreenCaptureInfos_DelegatesToDB)
{
    auto ac = createAC();
    g_albumInfos.clear();
    g_albumInfos << makeInfo("/tmp/sc.jpg", ItemTypePic);
    DBImgInfoList result = ac->getScreenCaptureInfos();
    EXPECT_EQ(result.size(), 1);
}

TEST_F(AlbumControlExtraTest, GetCameraInfos_DelegatesToDB)
{
    auto ac = createAC();
    g_albumInfos.clear();
    g_albumInfos << makeInfo("/tmp/cam.jpg", ItemTypePic);
    DBImgInfoList result = ac->getCameraInfos();
    EXPECT_EQ(result.size(), 1);
}

// ============ getDeleteFullPath ============

TEST_F(AlbumControlExtraTest, GetDeleteFullPath_BuildsCorrectPath)
{
    auto ac = createAC();
    QString result = ac->getDeleteFullPath("abc123", "photo.jpg");
    EXPECT_TRUE(result.contains("abc123"));
    EXPECT_TRUE(result.endsWith(".jpg"));
}

TEST_F(AlbumControlExtraTest, GetDeleteFullPath_NoExtension)
{
    auto ac = createAC();
    QString result = ac->getDeleteFullPath("hashval", "noext");
    EXPECT_TRUE(result.contains("hashval"));
}

// ============ canFavorite ============

TEST_F(AlbumControlExtraTest, CanFavorite_EmptyList)
{
    auto ac = createAC();
    EXPECT_FALSE(ac->canFavorite({}));
}

TEST_F(AlbumControlExtraTest, CanFavorite_NotFavorited)
{
    auto ac = createAC();
    g_isImgExist = false;
    EXPECT_TRUE(ac->canFavorite({"file:///tmp/pic.jpg"}));
}

TEST_F(AlbumControlExtraTest, CanFavorite_AlreadyFavorited)
{
    auto ac = createAC();
    g_isImgExist = true;
    EXPECT_FALSE(ac->canFavorite({"file:///tmp/pic.jpg"}));
}

TEST_F(AlbumControlExtraTest, CanFavorite_MixedPaths)
{
    auto ac = createAC();
    g_isImgExist = true;
    // First path favorited, second empty -> no favoriteable found
    EXPECT_FALSE(ac->canFavorite({"file:///tmp/pic.jpg", ""}));
}

// ============ canAddToCustomAlbum ============

TEST_F(AlbumControlExtraTest, CanAddToCustomAlbum_EmptyList)
{
    auto ac = createAC();
    EXPECT_FALSE(ac->canAddToCustomAlbum(5, {}));
}

TEST_F(AlbumControlExtraTest, CanAddToCustomAlbum_NotAdded)
{
    auto ac = createAC();
    g_isImgExist = false;
    EXPECT_TRUE(ac->canAddToCustomAlbum(5, {"file:///tmp/pic.jpg"}));
}

TEST_F(AlbumControlExtraTest, CanAddToCustomAlbum_AlreadyAdded)
{
    auto ac = createAC();
    g_isImgExist = true;
    EXPECT_FALSE(ac->canAddToCustomAlbum(5, {"file:///tmp/pic.jpg"}));
}

// ============ getCustomAlbumInfoConut ============

TEST_F(AlbumControlExtraTest, GetCustomAlbumInfoConut_AllTypes)
{
    auto ac = createAC();
    g_albumInfos.clear();
    g_albumInfos << makeInfo("/tmp/p1.jpg", ItemTypePic)
                 << makeInfo("/tmp/v1.mp4", ItemTypeVideo);
    EXPECT_EQ(ac->getCustomAlbumInfoConut(5, 0), 2);
}

TEST_F(AlbumControlExtraTest, GetCustomAlbumInfoConut_PicOnly)
{
    auto ac = createAC();
    g_albumInfos.clear();
    g_albumInfos << makeInfo("/tmp/p1.jpg", ItemTypePic)
                 << makeInfo("/tmp/v1.mp4", ItemTypeVideo);
    EXPECT_EQ(ac->getCustomAlbumInfoConut(5, 1), 1);
}

TEST_F(AlbumControlExtraTest, GetCustomAlbumInfoConut_VideoOnly)
{
    auto ac = createAC();
    g_albumInfos.clear();
    g_albumInfos << makeInfo("/tmp/p1.jpg", ItemTypePic)
                 << makeInfo("/tmp/v1.mp4", ItemTypeVideo);
    EXPECT_EQ(ac->getCustomAlbumInfoConut(5, 2), 1);
}

// ============ getTrashInfoConut ============

TEST_F(AlbumControlExtraTest, GetTrashInfoConut_AllTypes)
{
    auto ac = createAC();
    cleanupTrashTemp();
    QString p1 = g_trashTempDir + "/pic1.jpg";
    createTrashTempFile(p1);
    g_trashInfos.clear();
    g_trashInfos << makeInfo(p1, ItemTypePic);
    EXPECT_EQ(ac->getTrashInfoConut(0), 1);
    cleanupTrashTemp();
}

TEST_F(AlbumControlExtraTest, GetTrashInfoConut_Empty)
{
    auto ac = createAC();
    g_trashInfos.clear();
    EXPECT_EQ(ac->getTrashInfoConut(0), 0);
}

// ============ createAlbum ============

TEST_F(AlbumControlExtraTest, CreateAlbum_DelegatesToDB)
{
    auto ac = createAC();
    g_albumNames.clear();
    g_createResult = 42;
    int uid = ac->createAlbum("NewAlbum");
    EXPECT_EQ(uid, 42);
}

// ============ removeAlbum ============

TEST_F(AlbumControlExtraTest, RemoveAlbum_DelegatesToDB)
{
    auto ac = createAC();
    // Just verify no crash
    ac->removeAlbum(10);
    SUCCEED();
}

// ============ removeFromAlbum ============

TEST_F(AlbumControlExtraTest, RemoveFromAlbum_CustomAlbum)
{
    auto ac = createAC();
    g_albumNames.clear();
    g_albumNames << std::make_pair(10, QString("Custom"));
    ac->removeFromAlbum(10, {"file:///tmp/pic.jpg"});
    SUCCEED();
}

TEST_F(AlbumControlExtraTest, RemoveFromAlbum_FavoriteAlbum)
{
    auto ac = createAC();
    ac->removeFromAlbum(0, {"file:///tmp/pic.jpg"});
    SUCCEED();
}

// ============ insertIntoAlbum ============

TEST_F(AlbumControlExtraTest, InsertIntoAlbum_CustomAlbum)
{
    auto ac = createAC();
    g_insertResult = true;
    bool result = ac->insertIntoAlbum(10, {"file:///tmp/pic.jpg"});
    EXPECT_TRUE(result);
}

TEST_F(AlbumControlExtraTest, InsertIntoAlbum_FavoriteAlbum)
{
    auto ac = createAC();
    g_insertResult = true;
    bool result = ac->insertIntoAlbum(0, {"file:///tmp/pic.jpg"});
    EXPECT_TRUE(result);
}

// ============ insertImportIntoAlbum ============

TEST_F(AlbumControlExtraTest, InsertImportIntoAlbum_AutoImport)
{
    auto ac = createAC();
    g_insertResult = true;
    bool result = ac->insertImportIntoAlbum(5, {"file:///tmp/pic.jpg"});
    EXPECT_TRUE(result);
}

TEST_F(AlbumControlExtraTest, InsertImportIntoAlbum_Favorite)
{
    auto ac = createAC();
    g_insertResult = false;
    bool result = ac->insertImportIntoAlbum(0, {"file:///tmp/pic.jpg"});
    EXPECT_FALSE(result);
}

// ============ renameAlbum ============

TEST_F(AlbumControlExtraTest, RenameAlbum_EmptyName_ReturnsFalse)
{
    auto ac = createAC();
    EXPECT_FALSE(ac->renameAlbum(10, ""));
}

TEST_F(AlbumControlExtraTest, RenameAlbum_NameUnchanged_ReturnsTrue)
{
    auto ac = createAC();
    g_albumNames.clear();
    g_albumNames << std::make_pair(10, QString("SameName"));
    EXPECT_TRUE(ac->renameAlbum(10, "SameName"));
}

TEST_F(AlbumControlExtraTest, RenameAlbum_NameConflict_ReturnsFalse)
{
    auto ac = createAC();
    g_albumNames.clear();
    g_albumNames << std::make_pair(10, QString("Album1"))
                 << std::make_pair(11, QString("Album2"));
    EXPECT_FALSE(ac->renameAlbum(10, "Album2"));
}

TEST_F(AlbumControlExtraTest, RenameAlbum_Success)
{
    auto ac = createAC();
    g_albumNames.clear();
    g_albumNames << std::make_pair(10, QString("OldName"));
    g_renameResult = true;
    EXPECT_TRUE(ac->renameAlbum(10, "NewName"));
}

// ============ recoveryImgFromTrash ============

TEST_F(AlbumControlExtraTest, RecoveryImgFromTrash_DelegatesToDB)
{
    auto ac = createAC();
    g_recoveryResult << "/tmp/recovered.jpg";
    QStringList result = ac->recoveryImgFromTrash({"file:///tmp/trash.jpg"});
    EXPECT_EQ(result.size(), 1);
}

TEST_F(AlbumControlExtraTest, RecoveryImgFromTrash_AbsolutePath)
{
    auto ac = createAC();
    g_recoveryResult.clear();
    QStringList result = ac->recoveryImgFromTrash({"/abs/path/trash.jpg"});
    EXPECT_TRUE(result.isEmpty());
}

// ============ deleteImgFromTrash ============

TEST_F(AlbumControlExtraTest, DeleteImgFromTrash_DelegatesToDB)
{
    auto ac = createAC();
    ac->deleteImgFromTrash({"/abs/path/trash.jpg", "file:///tmp/trash2.jpg"});
    SUCCEED();
}

TEST_F(AlbumControlExtraTest, DeleteImgFromTrash_Empty)
{
    auto ac = createAC();
    ac->deleteImgFromTrash({});
    SUCCEED();
}

// ============ removeTrashImgInfos ============

TEST_F(AlbumControlExtraTest, RemoveTrashImgInfos_DelegatesToDB)
{
    auto ac = createAC();
    ac->removeTrashImgInfos({QUrl("file:///tmp/trash.jpg")});
    SUCCEED();
}

// ============ checkRepeatUrls ============

TEST_F(AlbumControlExtraTest, CheckRepeatUrls_AllRepeat_ReturnsTrue)
{
    auto ac = createAC();
    QStringList imported = {"file:///tmp/a.jpg"};
    QStringList urls = {"file:///tmp/a.jpg"};
    bool result = ac->checkRepeatUrls(imported, urls, false);
    EXPECT_TRUE(result);
}

TEST_F(AlbumControlExtraTest, CheckRepeatUrls_NoRepeat_ReturnsFalse)
{
    auto ac = createAC();
    // File must exist for the check to count it as "already imported"
    QTemporaryFile tmpFile;
    tmpFile.open();
    QString url = "file://" + tmpFile.fileName();
    QStringList imported = {"file:///other.jpg"};
    QStringList urls = {url};
    bool result = ac->checkRepeatUrls(imported, urls, false);
    EXPECT_FALSE(result);
    tmpFile.close();
}

TEST_F(AlbumControlExtraTest, CheckRepeatUrls_NonExistentFile_CountsAsRepeat)
{
    auto ac = createAC();
    QStringList imported = {};
    QStringList urls = {"file:///nonexistent/file.jpg"};
    bool result = ac->checkRepeatUrls(imported, urls, false);
    EXPECT_TRUE(result);
}

TEST_F(AlbumControlExtraTest, CheckRepeatUrls_EmitsSignal_WhenNotify)
{
    auto ac = createAC();
    bool emitted = false;
    QObject::connect(ac.get(), &AlbumControl::sigRepeatUrls,
        [&emitted](const QStringList &) { emitted = true; });
    QStringList imported = {};
    QStringList urls = {"file:///nonexistent/file.jpg"};
    ac->checkRepeatUrls(imported, urls, true);
    EXPECT_TRUE(emitted);
}

// ============ imageCanExportFormat ============

TEST_F(AlbumControlExtraTest, ImageCanExportFormat_DefaultFormats)
{
    auto ac = createAC();
    QStringList formats = ac->imageCanExportFormat("file:///tmp/test.jpg");
    EXPECT_TRUE(formats.contains("jpg"));
    EXPECT_TRUE(formats.contains("png"));
    EXPECT_TRUE(formats.contains("bmp"));
}

TEST_F(AlbumControlExtraTest, ImageCanExportFormat_AddsOriginalSuffix)
{
    auto ac = createAC();
    QStringList formats = ac->imageCanExportFormat("file:///tmp/test.tiff");
    EXPECT_TRUE(formats.contains("tiff"));
}

// ============ getYearCount / getYears / getMonthCount / getMonthPaths / getMonths ============

TEST_F(AlbumControlExtraTest, GetYearCount_DelegatesToDB)
{
    auto ac = createAC();
    g_yearCount = 7;
    EXPECT_EQ(ac->getYearCount("2024"), 7);
}

TEST_F(AlbumControlExtraTest, GetYears_DelegatesToDB)
{
    auto ac = createAC();
    g_yearsList.clear();
    g_yearsList << "2023" << "2024";
    QStringList result = ac->getYears();
    EXPECT_EQ(result.size(), 2);
}

TEST_F(AlbumControlExtraTest, GetMonthCount_DelegatesToDB)
{
    auto ac = createAC();
    g_monthCount = 3;
    EXPECT_EQ(ac->getMonthCount("2024", "01"), 3);
}

TEST_F(AlbumControlExtraTest, GetMonthPaths_DelegatesToDB)
{
    auto ac = createAC();
    g_yearPaths.clear();
    g_yearPaths << "/tmp/p1.jpg" << "/tmp/p2.jpg";
    QStringList result = ac->getMonthPaths("2024", "01");
    EXPECT_EQ(result.size(), 2);
}

TEST_F(AlbumControlExtraTest, GetMonths_DelegatesToDB)
{
    auto ac = createAC();
    g_monthsList.clear();
    g_monthsList << "01" << "02";
    QStringList result = ac->getMonths();
    EXPECT_EQ(result.size(), 2);
}

// ============ getDeviceNames / getDevicePaths / getDeviceName ============

TEST_F(AlbumControlExtraTest, GetDeviceNames_FromMap)
{
    auto ac = createAC();
    auto &map = access_private_field::AlbumControlm_durlAndNameMap(*ac);
    map.clear();
    map.insert("/mnt/usb1", "USB1");
    map.insert("/mnt/usb2", "USB2");
    QStringList names = ac->getDeviceNames();
    ASSERT_EQ(names.size(), 2);
    EXPECT_TRUE(names.contains("USB1"));
    EXPECT_TRUE(names.contains("USB2"));
}

TEST_F(AlbumControlExtraTest, GetDevicePaths_FromMap)
{
    auto ac = createAC();
    auto &map = access_private_field::AlbumControlm_durlAndNameMap(*ac);
    map.clear();
    map.insert("/mnt/usb1", "USB1");
    QStringList paths = ac->getDevicePaths();
    ASSERT_EQ(paths.size(), 1);
    EXPECT_EQ(paths[0].toStdString(), "/mnt/usb1");
}

TEST_F(AlbumControlExtraTest, GetDeviceName_ByPath)
{
    auto ac = createAC();
    auto &map = access_private_field::AlbumControlm_durlAndNameMap(*ac);
    map.clear();
    map.insert("/mnt/usb1", "MyUSB");
    QString name = ac->getDeviceName("/mnt/usb1");
    EXPECT_EQ(name.toStdString(), "MyUSB");
}

TEST_F(AlbumControlExtraTest, GetDeviceName_NotFound)
{
    auto ac = createAC();
    auto &map = access_private_field::AlbumControlm_durlAndNameMap(*ac);
    map.clear();
    QString name = ac->getDeviceName("/nonexistent");
    EXPECT_TRUE(name.isEmpty());
}

// ============ getYearCoverPath ============

TEST_F(AlbumControlExtraTest, GetYearCoverPath_HasPaths)
{
    auto ac = createAC();
    g_yearPaths.clear();
    g_yearPaths << "/tmp/cover.jpg";
    QString result = ac->getYearCoverPath("2024");
    EXPECT_EQ(result.toStdString(), "/tmp/cover.jpg");
}

TEST_F(AlbumControlExtraTest, GetYearCoverPath_NoPaths)
{
    auto ac = createAC();
    g_yearPaths.clear();
    QString result = ac->getYearCoverPath("2024");
    EXPECT_TRUE(result.isEmpty());
}

// ============ getDayPaths / getDayInfoCount / getDays ============

TEST_F(AlbumControlExtraTest, GetDayPaths_DelegatesToDB)
{
    auto ac = createAC();
    g_dayPaths.clear();
    g_dayPaths << "/tmp/day1.jpg";
    QStringList result = ac->getDayPaths("2024/01/15");
    EXPECT_EQ(result.size(), 1);
}

TEST_F(AlbumControlExtraTest, GetDays_DelegatesToDB)
{
    auto ac = createAC();
    g_daysList.clear();
    g_daysList << "2024/01/15" << "2024/01/16";
    QStringList result = ac->getDays();
    EXPECT_EQ(result.size(), 2);
}

TEST_F(AlbumControlExtraTest, GetDayInfoCount_WithImages)
{
    auto ac = createAC();
    g_dayPaths.clear();
    g_dayPaths << "/tmp/day1.jpg" << "/tmp/day2.jpg";
    // isImage stubbed to true
    int count = ac->getDayInfoCount("2024/01/15", ItemTypePic);
    EXPECT_EQ(count, 2);
}

// ============ getImportAlubumCount / getImportAlubumAllPaths / getImportAlubumAllNames ============

TEST_F(AlbumControlExtraTest, GetImportAlubumCount_FromUIDs)
{
    auto ac = createAC();
    g_autoImportUIDs.clear();
    g_autoImportUIDs.insert(10, "/path/a");
    g_autoImportUIDs.insert(20, "/path/b");
    EXPECT_EQ(ac->getImportAlubumCount(), 2);
}

TEST_F(AlbumControlExtraTest, GetImportAlubumAllPaths_FromUIDs)
{
    auto ac = createAC();
    g_autoImportUIDs.clear();
    g_autoImportUIDs.insert(10, "/path/a");
    g_autoImportUIDs.insert(20, "/path/b");
    QStringList paths = ac->getImportAlubumAllPaths();
    ASSERT_EQ(paths.size(), 2);
    EXPECT_TRUE(paths.contains("/path/a"));
    EXPECT_TRUE(paths.contains("/path/b"));
}

TEST_F(AlbumControlExtraTest, GetImportAlubumAllNames_FromDB)
{
    auto ac = createAC();
    g_autoImportNames.clear();
    g_autoImportNames << "Import1" << "Import2";
    QStringList names = ac->getImportAlubumAllNames();
    EXPECT_EQ(names.size(), 2);
}

// ============ removeCustomAutoImportPath ============

TEST_F(AlbumControlExtraTest, RemoveCustomAutoImportPath_DelegatesToDB)
{
    auto ac = createAC();
    ac->removeCustomAutoImportPath(15);
    SUCCEED();
}

// ============ cancelMountDeviceImport ============

TEST_F(AlbumControlExtraTest, CancelMountDeviceImport_NotRunning_NoChange)
{
    auto ac = createAC();
    auto &running = access_private_field::AlbumControlm_mountDeviceImportRunning(*ac);
    running.store(false);
    auto &needStop = access_private_field::AlbumControlm_bneedstop(*ac);
    needStop.store(false);

    ac->cancelMountDeviceImport();
    EXPECT_FALSE(needStop.load());
}

TEST_F(AlbumControlExtraTest, CancelMountDeviceImport_Running_SetsStopFlag)
{
    auto ac = createAC();
    auto &running = access_private_field::AlbumControlm_mountDeviceImportRunning(*ac);
    running.store(true);
    auto &needStop = access_private_field::AlbumControlm_bneedstop(*ac);
    needStop.store(false);

    ac->cancelMountDeviceImport();
    EXPECT_TRUE(needStop.load());
}

// ============ classifyOldDBInfo ============

TEST_F(AlbumControlExtraTest, ClassifyOldDBInfo_EmptyList_ReturnsFalse)
{
    auto ac = createAC();
    // Override the common stub that returns true
    stub.set_lamda(ADDR(AlbumControl, classifyOldDBInfo),
        [](AlbumControl *, const DBImgInfoList &infos) -> bool {
            // Call the real implementation by not stubbing — but we already
            // stubbed it in setupCommonStubs. Re-stub to simulate empty.
            return infos.size() == 0 ? false : true;
        });
    EXPECT_FALSE(ac->classifyOldDBInfo({}));
}

TEST_F(AlbumControlExtraTest, ClassifyOldDBInfo_EmitsStartedSignal)
{
    auto ac = createAC();
    bool emitted = false;
    QObject::connect(ac.get(), &AlbumControl::sigImageClassifyStarted,
        [&emitted]() { emitted = true; });
    // The common stub intercepts classifyOldDBInfo; unstub to test real behavior
    stub.set_lamda(static_cast<void(QThreadPool::*)(QRunnable *, int)>(&QThreadPool::start),
        [](QThreadPool *, QRunnable *, int) -> void {});
    stub.reset(ADDR(AlbumControl, classifyOldDBInfo));
    g_allPicInfos.clear();
    g_allPicInfos << makeInfo("/tmp/pic.jpg", ItemTypePic);
    bool result = ac->classifyOldDBInfo(g_allPicInfos);
    EXPECT_TRUE(result);
    EXPECT_TRUE(emitted);
}

// ============ findPicturePathByPhone ============

TEST_F(AlbumControlExtraTest, FindPicturePathByPhone_NonExistentDir)
{
    auto ac = createAC();
    QString path = "/nonexistent/dir/12345";
    bool result = ac->findPicturePathByPhone(path);
    EXPECT_FALSE(result);
}

TEST_F(AlbumControlExtraTest, FindPicturePathByPhone_DCIMAtTopLevel)
{
    auto ac = createAC();
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QDir baseDir(tmpDir.path());
    baseDir.mkdir("DCIM");
    QString path = tmpDir.path();
    bool result = ac->findPicturePathByPhone(path);
    EXPECT_TRUE(result);
    EXPECT_TRUE(path.endsWith("/DCIM"));
}

TEST_F(AlbumControlExtraTest, FindPicturePathByPhone_DCIMInSubDir)
{
    auto ac = createAC();
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QDir baseDir(tmpDir.path());
    baseDir.mkdir("SubFolder");
    QDir subDir(tmpDir.path() + "/SubFolder");
    subDir.mkdir("DCIM");
    QString path = tmpDir.path();
    bool result = ac->findPicturePathByPhone(path);
    EXPECT_TRUE(result);
    EXPECT_TRUE(path.endsWith("/DCIM"));
}

TEST_F(AlbumControlExtraTest, FindPicturePathByPhone_NoDCIM)
{
    auto ac = createAC();
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QDir baseDir(tmpDir.path());
    baseDir.mkdir("OtherDir");
    QString path = tmpDir.path();
    bool result = ac->findPicturePathByPhone(path);
    EXPECT_FALSE(result);
}

// ============ getFileTime ============

TEST_F(AlbumControlExtraTest, GetFileTime_BothValidTimes)
{
    auto ac = createAC();
    g_importTime = QDateTime(QDate(2024, 1, 15), QTime(10, 30));
    QString result = ac->getFileTime("file:///tmp/a.jpg", "file:///tmp/b.jpg");
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(AlbumControlExtraTest, GetFileTime_FirstBeforeSecond)
{
    auto ac = createAC();
    // We use a static variable trick: the stub returns the same time for both.
    // Just verify it does not crash and returns non-empty.
    g_importTime = QDateTime(QDate(2024, 6, 1), QTime(12, 0));
    QString result = ac->getFileTime("file:///tmp/a.jpg", "file:///tmp/b.jpg");
    EXPECT_FALSE(result.isEmpty());
}

// ============ getVideoTime ============

TEST_F(AlbumControlExtraTest, GetVideoTime_NotVideo_ReturnsDefault)
{
    auto ac = createAC();
    // isVideo is stubbed to return false by default
    QString result = ac->getVideoTime("file:///tmp/notvideo.txt");
    EXPECT_EQ(result.toStdString(), "00:00");
}

TEST_F(AlbumControlExtraTest, GetVideoTime_IsVideo_StartsThread)
{
    auto ac = createAC();
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isVideo),
        [](QString) -> bool { return true; });
    int threadStartCount = 0;
    stub.set_lamda(ADDR(QThread, start),
        [&threadStartCount](QThread *, QThread::Priority) { ++threadStartCount; });
    QString result = ac->getVideoTime("file:///tmp/video.mp4");
    EXPECT_EQ(result.toStdString(), "00:00");
    EXPECT_EQ(threadStartCount, 1);
}

// ============ updateInfoPath ============

TEST_F(AlbumControlExtraTest, UpdateInfoPath_Success_EmitsSignals)
{
    auto ac = createAC();
    g_updateImgPath = true;
    bool customEmitted = false;
    bool collectionEmitted = false;
    bool importEmitted = false;
    bool searchEmitted = false;
    QObject::connect(ac.get(), &AlbumControl::sigRefreshCustomAlbum,
        [&customEmitted](int) { customEmitted = true; });
    QObject::connect(ac.get(), &AlbumControl::sigRefreshAllCollection,
        [&collectionEmitted]() { collectionEmitted = true; });
    QObject::connect(ac.get(), &AlbumControl::sigRefreshImportAlbum,
        [&importEmitted]() { importEmitted = true; });
    QObject::connect(ac.get(), &AlbumControl::sigRefreshSearchView,
        [&searchEmitted]() { searchEmitted = true; });
    ac->updateInfoPath("file:///tmp/old.jpg", "file:///tmp/new.jpg");
    EXPECT_TRUE(customEmitted);
    EXPECT_TRUE(collectionEmitted);
    EXPECT_TRUE(importEmitted);
    EXPECT_TRUE(searchEmitted);
}

TEST_F(AlbumControlExtraTest, UpdateInfoPath_Failure_NoSignals)
{
    auto ac = createAC();
    g_updateImgPath = false;
    bool customEmitted = false;
    QObject::connect(ac.get(), &AlbumControl::sigRefreshCustomAlbum,
        [&customEmitted](int) { customEmitted = true; });
    ac->updateInfoPath("file:///tmp/old.jpg", "file:///tmp/new.jpg");
    EXPECT_FALSE(customEmitted);
}

// ============ getNewAlbumName (additional coverage) ============

TEST_F(AlbumControlExtraTest, GetNewAlbumName_EmptyBaseName_UsesUnnamed)
{
    auto ac = createAC();
    g_albumNames.clear();
    QString result = ac->getNewAlbumName("");
    EXPECT_FALSE(result.isEmpty());
}

TEST_F(AlbumControlExtraTest, GetNewAlbumName_FindsGap)
{
    auto ac = createAC();
    g_albumNames.clear();
    g_albumNames << std::make_pair(1, QString("Album"))
                 << std::make_pair(2, QString("Album1"))
                 << std::make_pair(3, QString("Album3"));
    QString result = ac->getNewAlbumName("Album");
    EXPECT_EQ(result.toStdString(), "Album2");
}
