// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QGuiApplication>
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
// ---- Shared stub data ----

static DBImgInfoList g_allPicInfos;
static DBImgInfoList g_classInfos;
static DBImgInfoList g_timelineInfos;
static DBImgInfoList g_importTimelineInfos;
static QList<QDateTime> g_timelines;
static QList<QDateTime> g_importTimelines;
static MovieInfo g_movieInfo;
static QMap<QString, QString> g_metaData;

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
