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

TEST(getImgsCount, db5)
{
    TEST_CASE_NAME("db5")
    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "test";
    QStringList image_list;
    auto finfos = utils::image::getImagesInfo(testPath);
    DBManager::instance()->getImgsCount();
}

TEST(removeTestImagesInfo, db6)
{
    TEST_CASE_NAME("db6")
    QStringList image_list;
    auto finfos = utils::image::getImagesInfo(testPath_test);
    for (auto info : finfos) {
        image_list << info.absoluteFilePath();
    }
    DBManager::instance()->removeImgInfos(QStringList());
    DBImgInfoList dbinfos;
    for (auto i : image_list) {
        DBImgInfo info;
        info.filePath = i;
        dbinfos << info;
    }
    DBManager::instance()->removeImgInfos(image_list);
    DBManager::instance()->removeImgInfosNoSignal(image_list);
}

TEST(getImageByKeyBoard, db8)
{
    TEST_CASE_NAME("db8")
    DBManager::instance()->getInfosForKeyword("");
    DBManager::instance()->getInfosForKeyword("1");
    DBManager::instance()->getInfosForKeyword("a");
}

TEST(getTrashImageCount, db10)
{
    TEST_CASE_NAME("db10")
    DBManager::instance()->getTrashImgsCount();
    DBManager::instance()->getAllTrashInfos();
    DBManager::instance()->getAllTrashPaths();
    DBManager::instance()->getTrashInfoByPath("");
}

TEST(AlbumForTest, db11)
{
    TEST_CASE_NAME("db11")
    QString testPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QDir::separator() + "dbtest";
    QStringList image_list;
    auto finfos = utils::image::getImagesInfo(testPath);
    for (auto info : finfos) {
        image_list << info.absoluteFilePath();
    }
    QStringList partList = image_list.mid(image_list.size() / 2);
    DBManager::instance()->insertIntoAlbum("testAlbum", image_list);
    DBManager::instance()->insertIntoAlbumNoSignal("testAlbum", image_list);
    DBManager::instance()->getPathsByAlbum("testAlbum");
    DBManager::instance()->getInfosByAlbum("testAlbum");
//    DBManager::instance()->removeFromAlbum("testAlbum", partList);
    DBManager::instance()->renameAlbum("testAlbum", "newTestAlbum");
//    DBManager::instance()->removeAlbum("newTestAlbum");
    DBManager::instance()->getTrashInfosForKeyword("");
    QStringList l;
    l << "newTestAlbum";
    DBManager::instance()->removeTrashImgInfosNoSignal(l);

    DBImgInfo info, info1;
    info.dirHash = "test";
    info1.dirHash = "test";
    if (info == info1)
        qDebug() << "same";
}

TEST(DBandImgOperate, DBImgOper)
{
    TEST_CASE_NAME("DBImgOper")
    DBandImgOperate *db = new DBandImgOperate;
    QString pic = testPath_test + "/39elz3.png";
    db->loadOneThumbnail(pic);
    db->loadOneThumbnail(testPath_test + "/39elz3.png123"); //图片不存在的分支
    db->getAllInfos();

    delete db;//用完记得清理掉
}
