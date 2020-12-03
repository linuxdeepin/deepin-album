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

TEST(isRunning, ap1)
{
    qDebug() << "application isRunning ap1 count = " << count_testDefine++;
    ASSERT_EQ(false, dApp->isRunning());
}

TEST(sendMessage, ap2)
{
    qDebug() << "application sendMessage ap2 count = " << count_testDefine++;
    ASSERT_EQ(false, dApp->sendMessage(""));
}

TEST(ImageLoader, load)
{
    QStringList list;
    list << testPath_test + "/2e5y8y.png" << "/usr/share/wallpapers/deepin/abc-123.jpg";
    ImageLoader *loader = new ImageLoader(nullptr, list, list);
    loader->updateImageLoader(list);
    ASSERT_EQ(false, loader == nullptr);
}
