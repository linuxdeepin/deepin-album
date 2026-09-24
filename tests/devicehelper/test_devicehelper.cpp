// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | DeviceHelper::loadDeviceInfo | high | 5 | 3 | 5 |
 *
 * 分支列表 (loadDeviceInfo):
 *   B1: deviceId.isEmpty() → return empty QVariantMap
 *   B2: isSamba(deviceId) == true → return empty QVariantMap
 *   B3: cached in m_mapDevicesInfos && !reload → return cached QVariantMap
 *   B4: block device path ("/org/freedesktop/..."), DBus reply invalid → return empty
 *   B5: protocol device path, DBus reply invalid → return empty
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QDBusInterface>
#include <QDBusReply>
#include <QVariantMap>
#include <QString>
#include <memory>

#include "utils/devicehelper.h"
#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// Type alias to avoid commas in macro
using QVariantMapMap = QMap<QString, QVariantMap>;

// Access private members
ACCESS_PRIVATE_FIELD(DeviceHelper, QVariantMapMap, m_mapDevicesInfos)

// isSamba checks: QUrl.toLocalFile() matches (^/run/user/\d+/gvfs/|^/root/.gvfs/|^(/run)?/media/[\s\S]*/smbmounts)
// We trigger it by passing file:///run/user/1000/gvfs/ as deviceId — no stub needed.

class DeviceHelperTest : public ::testing::Test
{
protected:
    StubExt stub;

    void SetUp() override
    {
        // No stubs needed — isSamba is tested via real URL matching
    }

    void TearDown() override
    {
        stub.clear();
    }
};

// B1: Empty deviceId → return empty QVariantMap
TEST_F(DeviceHelperTest, LoadDeviceInfo_EmptyDeviceId_ReturnsEmpty)
{
    // Arrange
    DeviceHelper helper;

    // Act
    QVariantMap result = helper.loadDeviceInfo("", false);

    // Assert
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(result.size(), 0);
}

// B2: Samba device (URL matches gvfs pattern) → return empty QVariantMap
TEST_F(DeviceHelperTest, LoadDeviceInfo_SambaDevice_ReturnsEmpty)
{
    // Arrange
    DeviceHelper helper;
    // file:///run/user/1000/gvfs/ → isSamba returns true (matches gvfs regex)
    QString sambaDeviceId = "file:///run/user/1000/gvfs/smb-share";

    // Act
    QVariantMap result = helper.loadDeviceInfo(sambaDeviceId, false);

    // Assert
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(result.size(), 0);
}

// B3: Cached device info, reload=false → return cached value
TEST_F(DeviceHelperTest, LoadDeviceInfo_CachedNoReload_ReturnsCached)
{
    // Arrange
    DeviceHelper helper;
    QString deviceId = "/org/freedesktop/UDisks2/block_devices/sdb1";
    QVariantMap cachedInfo;
    cachedInfo["MountPoint"] = "/media/usb";
    cachedInfo["DeviceType"] = static_cast<int>(1);

    auto &cache = access_private_field::DeviceHelperm_mapDevicesInfos(helper);
    cache[deviceId] = cachedInfo;

    // Act
    QVariantMap result = helper.loadDeviceInfo(deviceId, false);

    // Assert
    EXPECT_EQ(result.value("MountPoint").toString().toStdString(), "/media/usb");
    EXPECT_EQ(result.value("DeviceType").toInt(), 1);
}

// B4: Block device path, DBus reply invalid (service unavailable in test env) → return empty
TEST_F(DeviceHelperTest, LoadDeviceInfo_BlockDevice_DBusError_ReturnsEmpty)
{
    // Arrange
    DeviceHelper helper;
    // In test env, DBus service is unavailable → QDBusReply::isValid() == false

    // Act
    QVariantMap result = helper.loadDeviceInfo(
        "/org/freedesktop/UDisks2/block_devices/sdb1", true);

    // Assert
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(result.size(), 0);
}

// B5: Protocol device path, DBus reply invalid → return empty
TEST_F(DeviceHelperTest, LoadDeviceInfo_ProtocolDevice_DBusError_ReturnsEmpty)
{
    // Arrange
    DeviceHelper helper;
    // Non-block device path (doesn't start with /org/freedesktop/)

    // Act
    QVariantMap result = helper.loadDeviceInfo("device://protocol/device1", true);

    // Assert
    EXPECT_TRUE(result.isEmpty());
    EXPECT_EQ(result.size(), 0);
}
