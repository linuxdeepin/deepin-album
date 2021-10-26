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
