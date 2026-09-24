// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | FileTrashHelper::queryMountInfo | high | 4 | 3 | 4 |
 *
 * 分支列表 (queryMountInfo):
 *   B1: initData == true → early return, no DBus call
 *   B2: initData == false, DBus reply invalid → return, mountDevices unchanged
 *   B3: initData == false, DBus reply valid but no devices → mountDevices empty
 *   B4: initData == false → initData set to true after call (idempotency)
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QDBusInterface>
#include <QDBusReply>
#include <QVariantMap>
#include <QStringList>
#include <QString>
#include <QMap>
#include <QMultiHash>
#include <memory>

#include "stubext.h"
#include "addr_pri.h"
#include "utils/filetrashhelper.h"

using namespace stub_ext;

// Typedef to avoid commas in ACCESS_PRIVATE_FIELD macro args
using StringStringHash = QMultiHash<QString, QString>;

// Access private members without "#define private public" (ODR-safe)
ACCESS_PRIVATE_FUN(FileTrashHelper, void(), queryMountInfo)
ACCESS_PRIVATE_FIELD(FileTrashHelper, bool, initData)
ACCESS_PRIVATE_FIELD(FileTrashHelper, StringStringHash, mountDevices)

class FileTrashHelperTest : public ::testing::Test
{
protected:
    StubExt stub;

    void SetUp() override
    {
        // No common stubs needed — DBus service is unavailable in test env
    }

    void TearDown() override
    {
        stub.clear();
    }
};

// B1: Already initialized → early return, no change to mountDevices
TEST_F(FileTrashHelperTest, QueryMountInfo_AlreadyInitialized_NoChange)
{
    // Arrange
    FileTrashHelper helper;
    auto &mounts = access_private_field::FileTrashHelpermountDevices(helper);
    access_private_field::FileTrashHelperinitData(helper) = true;
    mounts.clear();
    mounts.insert("test_device", "/media/test");

    // Act
    call_private_fun::FileTrashHelperqueryMountInfo(helper);

    // Assert
    EXPECT_EQ(mounts.size(), 1);
    EXPECT_TRUE(mounts.contains("test_device"));
}

// B2: First call, DBus reply invalid (service unavailable) → mountDevices stays empty
TEST_F(FileTrashHelperTest, QueryMountInfo_DBusError_MountDevicesEmpty)
{
    // Arrange
    FileTrashHelper helper;
    access_private_field::FileTrashHelperinitData(helper) = false;
    auto &mounts = access_private_field::FileTrashHelpermountDevices(helper);
    mounts.clear();

    // Act — DBus service unavailable in test env → reply invalid → early return
    call_private_fun::FileTrashHelperqueryMountInfo(helper);

    // Assert
    EXPECT_TRUE(mounts.isEmpty());
    EXPECT_EQ(mounts.size(), 0);
}

// B4: After first call, initData is set to true (idempotency check)
TEST_F(FileTrashHelperTest, QueryMountInfo_SetsInitDataTrue_AfterFirstCall)
{
    // Arrange
    FileTrashHelper helper;
    auto &init = access_private_field::FileTrashHelperinitData(helper);
    init = false;

    // Act
    call_private_fun::FileTrashHelperqueryMountInfo(helper);

    // Assert
    EXPECT_TRUE(init);
    EXPECT_EQ(access_private_field::FileTrashHelpermountDevices(helper).size(), 0);
}

// B1+B2: Second call after first → no additional DBus calls (initData guard)
TEST_F(FileTrashHelperTest, QueryMountInfo_SecondCallSkipped_ByInitDataGuard)
{
    // Arrange
    FileTrashHelper helper;
    auto &init = access_private_field::FileTrashHelperinitData(helper);
    auto &mounts = access_private_field::FileTrashHelpermountDevices(helper);

    // First call — sets initData=true
    init = false;
    mounts.clear();
    call_private_fun::FileTrashHelperqueryMountInfo(helper);
    EXPECT_TRUE(init);
    EXPECT_TRUE(mounts.isEmpty());

    // Act — second call should be skipped by initData guard
    call_private_fun::FileTrashHelperqueryMountInfo(helper);

    // Assert — still empty, no crash
    EXPECT_TRUE(init);
    EXPECT_EQ(mounts.size(), 0);
}
