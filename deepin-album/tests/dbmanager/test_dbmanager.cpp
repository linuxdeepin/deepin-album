#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "dbmanager.h"

TEST(getAllPaths, db1)
{
    QStringList res = DBManager::instance()->getAllPaths();
    ASSERT_FALSE(res.isEmpty());
}

TEST(getAllPaths, db2)
{
    DBImgInfoList res = DBManager::instance()->getAllInfos();
    ASSERT_FALSE(res.isEmpty());
}

TEST(getAllTimelines, db3)
{
    QStringList res1 = DBManager::instance()->getAllTimelines();
    ASSERT_FALSE(res1.isEmpty());
    if (!res1.isEmpty()) {
        DBImgInfoList res2 = DBManager::instance()->getInfosByTimeline(res1.first());
        ASSERT_FALSE(res2.isEmpty());
    }
}

TEST(getImportTimelines, db4)
{
    QStringList res1 = DBManager::instance()->getImportTimelines();
    ASSERT_FALSE(res1.isEmpty());
    if (!res1.isEmpty()) {
        DBImgInfoList res2 = DBManager::instance()->getInfosByImportTimeline(res1.first());
        ASSERT_FALSE(res2.isEmpty());
    }
}


//const DBImgInfo         getInfoByName(const QString &name) const;
//const DBImgInfo         getInfoByPath(const QString &path) const;
//const DBImgInfo         getInfoByPathHash(const QString &pathHash) const;
//int                     getImgsCount() const;
//int                     getImgsCountByDir(const QString &dir) const;
//const QStringList       getPathsByDir(const QString &dir) const;
//bool                    isImgExist(const QString &path) const;
