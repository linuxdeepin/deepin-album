/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     ZhangYong <zhangyong@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "application.h"
#include "dbmanager.h"
#include "DBandImgOperate.h"
#include "utils/baseutils.h"
#include "utils/imageutils.h"
#include "../test_qtestDefine.h"
#include "ac-desktop-define.h"
#include "mainwindow.h"
#include "imageengineapi.h"
#include "utils/baseutils.h"

TEST(getImgsCount, db5)
{
    TEST_CASE_NAME("db5")
    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "AlbumtestResource/test";
    QStringList image_list;
    auto finfos = utils::image::getImagesAndVideoInfo(testPath);
    DBManager::instance()->getImgsCount();
}

TEST(removeTestImagesInfo, db6)
{
    TEST_CASE_NAME("db6")
    QStringList image_list;
    auto finfos = utils::image::getImagesAndVideoInfo(testPath_test);
    for (auto info : finfos) {
        image_list << info.absoluteFilePath();
    }
    DBManager::instance()->removeImgInfos(QStringList());
    DBManager::instance()->removeImgInfos(image_list);
    image_list << QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "AlbumtestResource/test.png";
    DBManager::instance()->removeImgInfosNoSignal(image_list);
    DBManager::instance()->getAllPathAlbumNames();
}

TEST(getImageByKeyBoard, db8)
{
    TEST_CASE_NAME("db8")
    DBManager::instance()->getInfosForKeyword("");
    DBManager::instance()->getInfosForKeyword("1");
    DBManager::instance()->getInfosForKeyword("a");
    DBManager::instance()->getInfosForKeyword(DBManager::SpUID::u_Draw, "jpg");
    DBManager::instance()->getInfosForKeyword(10, "a");
}

TEST(getTrashImageCount, db10)
{
    TEST_CASE_NAME("db10")
    DBManager::instance()->getTrashImgsCount();
    DBManager::instance()->getAllTrashInfos(true);
    DBManager::instance()->getAllTrashInfos(false);
    DBManager::instance()->getTrashInfoByPath("");
}

TEST(AlbumForTest, db11)
{
    TEST_CASE_NAME("db11")
    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "AlbumtestResource/dbtest";
    QStringList image_list;
    auto finfos = utils::image::getImagesAndVideoInfo(testPath);
    for (auto info : finfos) {
        image_list << info.absoluteFilePath();
    }
    QStringList partList = image_list.mid(image_list.size() / 2);
    int UID = DBManager::instance()->createAlbum("testAlbum", {});
    DBManager::instance()->insertIntoAlbum(UID, image_list);
    DBManager::instance()->getPathsByAlbum(UID);
    DBManager::instance()->getInfosByAlbum(UID, true);
    DBManager::instance()->getInfosByAlbum(UID, false);
//    DBManager::instance()->removeFromAlbum("testAlbum", partList);
    DBManager::instance()->renameAlbum(UID, "newTestAlbum");
//    DBManager::instance()->removeAlbum("newTestAlbum");
    DBManager::instance()->getTrashInfosForKeyword("");
    QStringList l;
    l << "newTestAlbum";
    DBManager::instance()->removeTrashImgInfosNoSignal(l);
}

TEST(DBandImgOperate, setThreadShouldStop)
{
    TEST_CASE_NAME("setThreadShouldStop")
    ImageEngineApi::instance()->setThreadShouldStop();
}

TEST(DBManager, getAlbumNameFromUID)
{
    TEST_CASE_NAME("getAlbumNameFromUID")
    DBManager::instance()->getAlbumNameFromUID(DBManager::SpUID::u_Draw);  //查询成功
    QTest::qWait(200);
    DBManager::instance()->getAlbumNameFromUID(DBManager::SpUID::u_Camera);//查询失败
    QTest::qWait(200);
}

TEST(DBManager, isAlbumExistInDB)
{
    TEST_CASE_NAME("getAlbumNameFromUID")
    DBManager::instance()->isAlbumExistInDB(DBManager::SpUID::u_Draw, AutoImport);//查询成功
    QTest::qWait(200);
    DBManager::instance()->isAlbumExistInDB(DBManager::SpUID::u_Camera, Custom);//查询失败
    QTest::qWait(200);
}

TEST(DBManager, AutoImport)
{
    TEST_CASE_NAME("checkCustomAutoImportPathIsNotified")

    auto testPath = testPath_Pictures + "/album_ut_mount_point";

    //预先检查是否有
    DBManager::instance()->checkCustomAutoImportPathIsNotified(testPath);

    //添加进去后再检查是否有
    int uid = DBManager::instance()->createNewCustomAutoImportPath(testPath, "album_ut_mount_point");
    QTest::qWait(2000);
    auto c = DBManager::instance()->checkCustomAutoImportPathIsNotified(testPath);
    ASSERT_EQ(c, true);

    //完成删除后最后检查是否有
    DBManager::instance()->removeCustomAutoImportPath(uid);
    QTest::qWait(2000);
    DBManager::instance()->checkCustomAutoImportPathIsNotified(testPath);
}

TEST(DBManager, UpdateDateTime)
{
    TEST_CASE_NAME("UpdateDateTime")

    QVariant date1(QString("2021.11.11 11:11"));
    auto result1 = utils::base::analyzeDateTime(date1);
    ASSERT_EQ(result1.second, true);

    QVariant date2(QString("2022-04-14T19:44:00.000"));
    auto result2 = utils::base::analyzeDateTime(date2);
    ASSERT_EQ(result2.second, false);
}
