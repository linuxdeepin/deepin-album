/*
* Copyright (C) 2019 ~ 2020 UnionTech Software Technology Co.,Ltd
*
* Author:     dengjinhui<dengjinhui@uniontech.com>
* Maintainer: dengjinhui<dengjinhui@uniontech.com>
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "application.h"
#include "test_qtestDefine.h"
#include "imagebutton.h"
#include "elidedlabel.h"

#include <QDebug>
#include <QDir>
#include <QStringList>
#include <QStandardPaths>
#include <QTest>

TEST(isRunning, ap1)
{
    TEST_CASE_NAME("ap1")
    qDebug() << "application isRunning ap1 count = " << count_testDefine++;
    ASSERT_EQ(false, dApp->isRunning());
    QTest::qWait(300);
}

TEST(sendMessage, ap2)
{
    TEST_CASE_NAME("ap2")
    qDebug() << "application sendMessage ap2 count = " << count_testDefine++;
    ASSERT_EQ(false, dApp->sendMessage(""));
}

// 文件夹拷贝
bool copyDirFiles(const QString &fromDir, const QString &toDir)
{
    QDir sourceDir(fromDir);
    QDir targetDir(toDir);

    if (!targetDir.exists()) {
        if (!targetDir.mkdir(targetDir.absolutePath())) {
            return false;
        }
    }

    QFileInfoList fileInfoList = sourceDir.entryInfoList();
    for (auto fileInfo : fileInfoList) {
        if (fileInfo.fileName() == "." || fileInfo.fileName() == "..") {
            continue;
        }

        if (fileInfo.isDir()) {
            if (!copyDirFiles(fileInfo.filePath(), targetDir.filePath(fileInfo.fileName()))) {
                return false;
            }
        } else {
            if (!QFile::copy(fileInfo.filePath(), targetDir.filePath(fileInfo.fileName()))) {
                return false;
            }
        }
    }

    return true;
}

//拷贝图片
void CopyPicToLocal()
{
//    TEST_CASE_NAME("other")
    QDir dir;
    dir.cd("./../../tests/testResource");//脚本终端run
    // 启动方式不同，路径不同
//    if (!dir.path().contains("testResource")) {
//        dir.setPath("../../../tests/resource");
//    }

    QStringList stringList = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation);
    if (stringList.size() > 0) {
        stringList[0].append("/Picture");

        QDir deleteDir(stringList[0]);
        deleteDir.removeRecursively();

        QTest::qWait(50);
        copyDirFiles(dir.path(), stringList[0]);
    }
}

TEST(ImageLoader, load)
{
	TEST_CASE_NAME("load")
    QStringList list/* = QStandardPaths::standardLocations(QStandardPaths::PicturesLocation)*/;
    list  << "/usr/share/wallpapers/deepin/abc-123.jpg";
    ImageLoader *loader = new ImageLoader(nullptr, list, list);
    loader->updateImageLoader(list);
    ASSERT_EQ(false, loader == nullptr);
}

TEST(ImageButton, imgbtn)
{
    TEST_CASE_NAME("load")
    qDebug() << "ImageButton imgbtn count = " << count_testDefine++;
    ImageButton *btn = new ImageButton();
    btn->setDisabled(false);
    btn->setTooltipVisible(false);
    btn->show();
    QPoint point = btn->pos();
    QTestEventList e;
    e.addMouseMove(point);
    e.addMouseMove(point + QPoint(1, 1));
    e.simulate(btn);
    QTest::qWait(100);
    btn->hide();
    e.clear();
}

TEST(ElidedLabel, namelabel)
{
    TEST_CASE_NAME("load")
    qDebug() << "ElidedLabel namelabel = " << count_testDefine++;
    ElidedLabel *lab = new ElidedLabel;
    lab->setText("test");
    lab->onThemeChanged(ViewerThemeManager::AppTheme::Dark);
    QSize size = lab->size();
    lab->resize(size.width() + 1, size.height() + 1);
    lab->update();
    lab->deleteLater();
}
