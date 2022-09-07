// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <QMap>
#include <DFileDialog>
#include <DSearchEdit>

#include <QObject>
#include <QDialog>
#include <QStringList>
#include <QTestEventList>
#include <QCommandLineParser>

#define private public
#define protected public

#include "mainwindow.h"
#include "albumcreatedialog.h"
#include "test_qtestDefine.h"
#include "imginfodialog.h"
#include "ac-desktop-define.h"

#include <stub-tool/cpp-stub/stub.h>
#include <stub-tool/stub-ext/stubext.h>

struct CMOption {
    QString shortOption;
    QString longOption;
    QString description;
    QString valueName;
};

TEST(CommandLine, test_CommandLine)
{
    TEST_CASE_NAME("test_CommandLine")
    QStringList image_list;
    auto finfos = utils::image::getImagesAndVideoInfo(testPath_test);
    for (auto info : finfos) {
        image_list << info.absoluteFilePath();
    }
    DBManager::instance()->removeImgInfos(QStringList());

    QStringList paths;
    if (DBManager::instance()->getAllPaths().length() > 0)
        paths << DBManager::instance()->getAllPaths().first();
    else
        paths << testPath_test + "/2e5y8y.jpg";
}
