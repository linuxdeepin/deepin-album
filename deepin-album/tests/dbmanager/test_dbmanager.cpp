#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "dbmanager.h"

#include "utils/baseutils.h"

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
        if (!res2.isEmpty()) {
            DBImgInfo res3 = DBManager::instance()->getInfoByName(res2.first().fileName);
            ASSERT_FALSE(res3.fileName.isEmpty());
            DBImgInfo res4 = DBManager::instance()->getInfoByPath(res2.first().filePath);
            ASSERT_FALSE(res4.fileName.isEmpty());
            DBImgInfo res5 = DBManager::instance()->getInfoByPathHash(utils::base::hash(res2.first().filePath));
            ASSERT_FALSE(res5.fileName.isEmpty());
        }
    }

}

TEST(getImgsCount, db5)
{
    int res1 = DBManager::instance()->getImgsCount();
    ASSERT_TRUE(res1);
    DBManager::instance()->getImgsCountByDir("");
    DBManager::instance()->getPathsByDir("");
    DBManager::instance()->isImgExist("");
}


