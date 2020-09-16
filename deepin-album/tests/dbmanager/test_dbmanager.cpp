#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "dbmanager.h"

#include "utils/baseutils.h"
#include "utils/imageutils.h"

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
    QString testPath = "/home/djh/Pictures/test";
    QStringList image_list;
    auto finfos = utils::image::getImagesInfo(testPath);
    DBManager::instance()->getImgsCount();
    DBManager::instance()->isImgExist("");
    DBManager::instance()->isImgExist(finfos.first().absoluteFilePath());
}

TEST(removeTestImagesInfo, db6)
{
    QString testPath = "/home/djh/Pictures/dbtest";
    QStringList image_list;
    auto finfos = utils::image::getImagesInfo(testPath);
    for(auto info : finfos){
        image_list << info.absoluteFilePath();
    }
    DBManager::instance()->removeImgInfos(QStringList());
    DBImgInfoList dbinfos;
    for(auto i:image_list){
        DBImgInfo info;
        info.filePath = i;
        dbinfos << info;
    }
    DBManager::instance()->insertImgInfos(dbinfos);
    DBManager::instance()->removeImgInfos(image_list);
    DBManager::instance()->insertImgInfos(dbinfos);
    DBManager::instance()->removeImgInfosNoSignal(image_list);

}

TEST(getAlbumCount, db7)
{
    DBManager::instance()->getAlbumsCount();
    DBManager::instance()->getAllAlbumNames();
}

TEST(getImageByKeyBoard, db8)
{
    DBManager::instance()->getInfosForKeyword("");
    DBManager::instance()->getInfosForKeyword("1");
    DBManager::instance()->getInfosForKeyword("a");
}

TEST(getTrashImageCount, db10)
{
    DBManager::instance()->getTrashImgsCount();
    DBManager::instance()->getAllTrashInfos();
    DBManager::instance()->getAllTrashPaths();
    DBManager::instance()->getTrashInfoByPath("");
}

TEST(AlbumForTest, db11)
{
    QString testPath = "/home/djh/Pictures/dbtest";
    QStringList image_list;
    auto finfos = utils::image::getImagesInfo(testPath);
    for(auto info : finfos){
        image_list << info.absoluteFilePath();
    }
    QStringList partList = image_list.mid(image_list.size()/2);
    DBManager::instance()->insertIntoAlbum("testAlbum",image_list);
    DBManager::instance()->insertIntoAlbumNoSignal("testAlbum",image_list);
    DBManager::instance()->getPathsByAlbum("testAlbum");
    DBManager::instance()->getInfosByAlbum("testAlbum");
    DBManager::instance()->removeFromAlbum("testAlbum",partList);
    DBManager::instance()->renameAlbum("testAlbum","newTestAlbum");

}
