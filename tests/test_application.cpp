// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#define private public
#define protected public

#include "application.h"
#include "test_qtestDefine.h"
#include "imagebutton.h"
#include "elidedlabel.h"
#include "ac-desktop-define.h"
#include "globaleventfilter.h"

#include <QDebug>
#include <QDir>
#include <QStringList>
#include <QStandardPaths>
#include <QTest>

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

TEST(GlobalEventFilter, eventfilter)
{
    TEST_CASE_NAME("eventfilter")

    //这个东西因为是在application里面调用，所以扔这里
    GlobalEventFilter filter;

    QKeyEvent e1(QEvent::KeyPress, Qt::Key_Tab, Qt::KeyboardModifier::NoModifier);
    filter.eventFilter(nullptr, &e1);

    QKeyEvent e2(QEvent::KeyRelease, Qt::Key_1, Qt::KeyboardModifier::NoModifier);
    filter.eventFilter(nullptr, &e2);
}

TEST(Application, sendMessage)
{
    //向共享内存里面写数据
    dApp->sendMessage("1");
}
