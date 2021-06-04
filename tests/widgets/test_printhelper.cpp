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

#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrintPreviewWidget>
#include <QPrinter>
#include <QPainter>
#include <QToolBar>
#include <QCoreApplication>
#include <QImageReader>
#include <QDebug>
#include <QMap>
#include <QTestEventList>
#include <QObject>
#include <QDialog>
#include <QStringList>

#include <DSearchEdit>
#include <DFileDialog>

#include <dprintpreviewdialog.h>
#include <dprintpreviewwidget.h>

#define private public
#define protected public

#include "mainwindow.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include "printhelper.h"
#include "ac-desktop-define.h"
#include "utils/unionimage.h"

#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>

TEST(PrintHelper, PrintHelper_test)
{
    TEST_CASE_NAME("PrintHelper_test")
    MainWindow *w = dApp->getMainWindow();
    w->allPicBtnClicked();
    QTest::qWait(2000);

    int (*dlgexec)() = []() {
        return 1;
    };
    typedef int (*fptr)(QDialog *);
    fptr fptrexec = reinterpret_cast<fptr>(&QDialog::exec);  //obtaining an address
    Stub stub;
    stub.set(fptrexec, dlgexec);

    QStringList paths = DBManager::instance()->getAllPaths();
    QWidget *p = nullptr;

    PrintHelper *p1 = new PrintHelper;
    if (paths.size() < 1)
        paths << testPath_test + "2k9o1m.png";
    PrintHelper::getIntance()->showPrintDialog(paths, p);

    delete p1;//单元测试也要记得释放内存
}
