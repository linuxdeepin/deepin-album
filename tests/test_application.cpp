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
#include "test_qtestDefine.h"
#include "imagebutton.h"
#include "elidedlabel.h"
#include "ac-desktop-define.h"

#include <QDebug>
#include <QDir>
#include <QStringList>
#include <QStandardPaths>
#include <QTest>

TEST(isRunning, ap1)
{
    TEST_CASE_NAME("ap1")
    ASSERT_EQ(false, dApp->isRunning());
    QTest::qWait(300);
}

TEST(sendMessage, ap2)
{
    TEST_CASE_NAME("ap1")
    ASSERT_EQ(false, dApp->sendMessage(""));
}
#if 0
// 文件夹拷贝,暂不使用
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
#endif

TEST(ImageButton, imgbtn)
{
    TEST_CASE_NAME("ap1")
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
    TEST_CASE_NAME("ap1")
    ElidedLabel *lab = new ElidedLabel;
    lab->setText("test");
    lab->onThemeChanged(ViewerThemeManager::AppTheme::Dark);
    QSize size = lab->size();
    lab->resize(size.width() + 1, size.height() + 1);
    lab->update();
    lab->deleteLater();
}
