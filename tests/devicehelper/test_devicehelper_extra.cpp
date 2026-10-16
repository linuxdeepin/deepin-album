// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Extended test coverage for DeviceHelper
 *
 * Covered methods:
 *   - instance()               : singleton semantics
 *   - isSamba(QUrl)            : gvfs / smbmounts path matching
 *   - markUnmounting()         : mark / unmark unmounting paths
 *   - tryBeginRead()           : empty / normal / blocked paths
 *   - endRead()                : removes active read
 *   - waitForReadsDone()       : empty / no reads / active reads timeout
 *   - getAllMountPoints()      : cached devices / no devices
 *   - getMountPointByDeviceId(): empty / cached / not cached
 *   - getDeviceIdByMountPoint(): empty / matching / no match
 *   - loadAllDeviceInfos()     : clears cache (DBus unavailable)
 *   - getAllDeviceIds()        : cached keys / empty cache
 *   - getBlockDeviceIds()      : DBus unavailable → empty
 *   - getProtocalDeviceIds()   : DBus unavailable → empty (private)
 *   - detachDevice()           : empty → false / non-empty → true
 *   - isExist()                : empty / in cache / not in cache
 *   - DeviceReadGuard          : valid / empty / unmounting / destructor
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QDBusInterface>
#include <QDBusReply>
#include <QVariantMap>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <memory>

#include "utils/devicehelper.h"
#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

using QVariantMapMap = QMap<QString, QVariantMap>;

// Access private field m_mapDevicesInfos
ACCESS_PRIVATE_FIELD(DeviceHelper, QVariantMapMap, m_mapDevicesInfos)
// Access private method getProtocalDeviceIds
ACCESS_PRIVATE_FUN(DeviceHelper, QStringList(), getProtocalDeviceIds)

class DeviceHelperExtraTest : public ::testing::Test
{
protected:
    StubExt stub;

    void SetUp() override {}

    void TearDown() override
    {
        stub.clear();
        // Clean up static state to prevent cross-test leakage
        DeviceHelper::markUnmounting("/test_mount", false);
        DeviceHelper::markUnmounting("/media/usb", false);
        DeviceHelper::endRead("/home/user/file");
        DeviceHelper::endRead("/test_mount/file");
        DeviceHelper::endRead("/media/usb/file");
        DeviceHelper::endRead("/media/usb/sub/file");
        DeviceHelper::endRead("/nonexistent/path");
        DeviceHelper::endRead("/test_mount");
    }
};

// ======================== isSamba ========================

TEST_F(DeviceHelperExtraTest, IsSamba_GvfsUserPath_ReturnsTrue)
{
    QUrl url("file:///run/user/1000/gvfs/smb-share");
    EXPECT_TRUE(DeviceHelper::isSamba(url));
}

TEST_F(DeviceHelperExtraTest, IsSamba_RootGvfsPath_ReturnsTrue)
{
    QUrl url("file:///root/.gvfs/share");
    EXPECT_TRUE(DeviceHelper::isSamba(url));
}

TEST_F(DeviceHelperExtraTest, IsSamba_MediaSmbmounts_ReturnsTrue)
{
    QUrl url("file:///media/user/smbmounts/share");
    EXPECT_TRUE(DeviceHelper::isSamba(url));
}

TEST_F(DeviceHelperExtraTest, IsSamba_RunMediaSmbmounts_ReturnsTrue)
{
    QUrl url("file:///run/media/user/smbmounts/share");
    EXPECT_TRUE(DeviceHelper::isSamba(url));
}

TEST_F(DeviceHelperExtraTest, IsSamba_NormalPath_ReturnsFalse)
{
    QUrl url("file:///home/user/Documents");
    EXPECT_FALSE(DeviceHelper::isSamba(url));
}

TEST_F(DeviceHelperExtraTest, IsSamba_InvalidUrl_ReturnsFalse)
{
    QUrl url;
    EXPECT_FALSE(DeviceHelper::isSamba(url));
}

// ======================== markUnmounting + tryBeginRead ========================

TEST_F(DeviceHelperExtraTest, TryBeginRead_EmptyPath_ReturnsFalse)
{
    EXPECT_FALSE(DeviceHelper::tryBeginRead(""));
}

TEST_F(DeviceHelperExtraTest, TryBeginRead_NormalPath_ReturnsTrue)
{
    EXPECT_TRUE(DeviceHelper::tryBeginRead("/home/user/file"));
    DeviceHelper::endRead("/home/user/file");
}

TEST_F(DeviceHelperExtraTest, MarkUnmounting_BlocksTryBeginRead)
{
    DeviceHelper::markUnmounting("/test_mount", true);
    EXPECT_FALSE(DeviceHelper::tryBeginRead("/test_mount/file"));
    EXPECT_FALSE(DeviceHelper::tryBeginRead("/test_mount"));
}

TEST_F(DeviceHelperExtraTest, MarkUnmounting_UnmarkAllowsTryBeginRead)
{
    DeviceHelper::markUnmounting("/test_mount", true);
    DeviceHelper::markUnmounting("/test_mount", false);
    EXPECT_TRUE(DeviceHelper::tryBeginRead("/test_mount/file"));
    DeviceHelper::endRead("/test_mount/file");
}

TEST_F(DeviceHelperExtraTest, MarkUnmounting_EmptyPath_NoOp)
{
    DeviceHelper::markUnmounting("", true);
    EXPECT_TRUE(DeviceHelper::tryBeginRead("/nonexistent/path"));
    DeviceHelper::endRead("/nonexistent/path");
}

// ======================== endRead ========================

TEST_F(DeviceHelperExtraTest, EndRead_RemovesActiveRead)
{
    DeviceHelper::tryBeginRead("/home/user/file");
    DeviceHelper::endRead("/home/user/file");
    EXPECT_TRUE(DeviceHelper::waitForReadsDone("/home/user", 100));
}

// ======================== waitForReadsDone ========================

TEST_F(DeviceHelperExtraTest, WaitForReadsDone_EmptyMountPoint_ReturnsTrue)
{
    EXPECT_TRUE(DeviceHelper::waitForReadsDone("", 100));
}

TEST_F(DeviceHelperExtraTest, WaitForReadsDone_NoActiveReads_ReturnsTrue)
{
    EXPECT_TRUE(DeviceHelper::waitForReadsDone("/nonexistent", 100));
}

TEST_F(DeviceHelperExtraTest, WaitForReadsDone_ActiveReads_ReturnsFalseOnTimeout)
{
    DeviceHelper::tryBeginRead("/media/usb/file");
    EXPECT_FALSE(DeviceHelper::waitForReadsDone("/media/usb", 50));
    DeviceHelper::endRead("/media/usb/file");
}

TEST_F(DeviceHelperExtraTest, WaitForReadsDone_ActiveReadUnderSubpath_ReturnsFalseOnTimeout)
{
    DeviceHelper::tryBeginRead("/media/usb/sub/file");
    EXPECT_FALSE(DeviceHelper::waitForReadsDone("/media/usb", 50));
    DeviceHelper::endRead("/media/usb/sub/file");
}

// ======================== DeviceReadGuard ========================

TEST_F(DeviceHelperExtraTest, DeviceReadGuard_ValidPath_IsActive)
{
    DeviceReadGuard guard("/home/user/valid_file");
    EXPECT_TRUE(guard.isActive());
    // guard destructor calls endRead automatically
}

TEST_F(DeviceHelperExtraTest, DeviceReadGuard_EmptyPath_NotActive)
{
    DeviceReadGuard guard("");
    EXPECT_FALSE(guard.isActive());
}

TEST_F(DeviceHelperExtraTest, DeviceReadGuard_UnderUnmounting_NotActive)
{
    DeviceHelper::markUnmounting("/test_mount", true);
    DeviceReadGuard guard("/test_mount/file");
    EXPECT_FALSE(guard.isActive());
}

TEST_F(DeviceHelperExtraTest, DeviceReadGuard_DestructorCallsEndRead)
{
    {
        DeviceReadGuard guard("/media/usb/sub/file");
        EXPECT_TRUE(guard.isActive());
        EXPECT_FALSE(DeviceHelper::waitForReadsDone("/media/usb", 50));
    }
    // After guard destroyed, reads should be done
    EXPECT_TRUE(DeviceHelper::waitForReadsDone("/media/usb", 100));
}

// ======================== getAllMountPoints ========================

TEST_F(DeviceHelperExtraTest, GetAllMountPoints_WithCachedDevices_ReturnsMountPoints)
{
    DeviceHelper helper;
    // Stub loadAllDeviceInfos to prevent cache clearing
    stub.set_lamda(ADDR(DeviceHelper, loadAllDeviceInfos), [](DeviceHelper *) {});

    auto &cache = access_private_field::DeviceHelperm_mapDevicesInfos(helper);
    cache.clear();

    QVariantMap info1;
    info1["MountPoint"] = "/media/usb1";
    QVariantMap info2;
    info2["MountPoint"] = "/media/usb2";
    QVariantMap info3;  // no mount point — should be excluded
    cache["dev1"] = info1;
    cache["dev2"] = info2;
    cache["dev3"] = info3;

    QStringList points = helper.getAllMountPoints();
    EXPECT_EQ(points.size(), 2);
    EXPECT_TRUE(points.contains("/media/usb1"));
    EXPECT_TRUE(points.contains("/media/usb2"));
}

TEST_F(DeviceHelperExtraTest, GetAllMountPoints_NoDevices_ReturnsEmpty)
{
    DeviceHelper helper;
    // loadAllDeviceInfos called, DBus fails, cache stays empty
    QStringList points = helper.getAllMountPoints();
    EXPECT_TRUE(points.isEmpty());
}

// ======================== getMountPointByDeviceId ========================

TEST_F(DeviceHelperExtraTest, GetMountPointByDeviceId_EmptyId_ReturnsEmpty)
{
    DeviceHelper helper;
    EXPECT_EQ(helper.getMountPointByDeviceId("").toStdString(), "");
}

TEST_F(DeviceHelperExtraTest, GetMountPointByDeviceId_Cached_ReturnsMountPoint)
{
    DeviceHelper helper;
    QString deviceId = "/org/freedesktop/UDisks2/block_devices/sdb1";
    QVariantMap info;
    info["MountPoint"] = "/media/usb";

    auto &cache = access_private_field::DeviceHelperm_mapDevicesInfos(helper);
    cache[deviceId] = info;

    EXPECT_EQ(helper.getMountPointByDeviceId(deviceId).toStdString(), "/media/usb");
}

TEST_F(DeviceHelperExtraTest, GetMountPointByDeviceId_NotCached_ReturnsEmpty)
{
    DeviceHelper helper;
    // Not in cache, loadDeviceInfo tries DBus → fails → empty
    EXPECT_EQ(helper.getMountPointByDeviceId("unknown_device").toStdString(), "");
}

// ======================== getDeviceIdByMountPoint ========================

TEST_F(DeviceHelperExtraTest, GetDeviceIdByMountPoint_EmptyMountPoint_ReturnsEmpty)
{
    DeviceHelper helper;
    EXPECT_EQ(helper.getDeviceIdByMountPoint("").toStdString(), "");
}

TEST_F(DeviceHelperExtraTest, GetDeviceIdByMountPoint_Matching_ReturnsDeviceId)
{
    DeviceHelper helper;
    QString deviceId = "/org/freedesktop/UDisks2/block_devices/sdb1";
    QVariantMap info;
    info["MountPoint"] = "/media/usb";

    auto &cache = access_private_field::DeviceHelperm_mapDevicesInfos(helper);
    cache[deviceId] = info;

    EXPECT_EQ(helper.getDeviceIdByMountPoint("/media/usb").toStdString(),
              deviceId.toStdString());
}

TEST_F(DeviceHelperExtraTest, GetDeviceIdByMountPoint_NoMatch_ReturnsEmpty)
{
    DeviceHelper helper;
    EXPECT_EQ(helper.getDeviceIdByMountPoint("/nonexistent/mount").toStdString(), "");
}

// ======================== loadAllDeviceInfos ========================

TEST_F(DeviceHelperExtraTest, LoadAllDeviceInfos_ClearsCache)
{
    DeviceHelper helper;
    auto &cache = access_private_field::DeviceHelperm_mapDevicesInfos(helper);
    QVariantMap info;
    info["MountPoint"] = "/media/usb";
    cache["dev1"] = info;

    // loadAllDeviceInfos clears cache; DBus fails → cache stays empty
    helper.loadAllDeviceInfos();
    EXPECT_TRUE(cache.isEmpty());
}

// ======================== getAllDeviceIds ========================

TEST_F(DeviceHelperExtraTest, GetAllDeviceIds_ReturnsCachedKeys)
{
    DeviceHelper helper;
    auto &cache = access_private_field::DeviceHelperm_mapDevicesInfos(helper);
    cache.clear();
    cache["dev1"] = QVariantMap();
    cache["dev2"] = QVariantMap();

    QStringList ids = helper.getAllDeviceIds();
    EXPECT_EQ(ids.size(), 2);
    EXPECT_TRUE(ids.contains("dev1"));
    EXPECT_TRUE(ids.contains("dev2"));
}

TEST_F(DeviceHelperExtraTest, GetAllDeviceIds_EmptyCache_ReturnsEmpty)
{
    DeviceHelper helper;
    auto &cache = access_private_field::DeviceHelperm_mapDevicesInfos(helper);
    cache.clear();

    QStringList ids = helper.getAllDeviceIds();
    EXPECT_TRUE(ids.isEmpty());
}

// ======================== getBlockDeviceIds ========================

TEST_F(DeviceHelperExtraTest, GetBlockDeviceIds_DBusUnavailable_ReturnsEmpty)
{
    DeviceHelper helper;
    // DBus service unavailable in test env → reply invalid → empty list
    QStringList ids = helper.getBlockDeviceIds();
    EXPECT_TRUE(ids.isEmpty());
}

// ======================== getProtocalDeviceIds (private) ========================

TEST_F(DeviceHelperExtraTest, GetProtocalDeviceIds_DBusUnavailable_ReturnsEmpty)
{
    DeviceHelper helper;
    QStringList ids = call_private_fun::DeviceHelpergetProtocalDeviceIds(helper);
    EXPECT_TRUE(ids.isEmpty());
}

// ======================== detachDevice ========================

TEST_F(DeviceHelperExtraTest, DetachDevice_EmptyId_ReturnsFalse)
{
    DeviceHelper helper;
    EXPECT_FALSE(helper.detachDevice(""));
}

TEST_F(DeviceHelperExtraTest, DetachDevice_BlockDevice_ReturnsTrue)
{
    DeviceHelper helper;
    EXPECT_TRUE(helper.detachDevice("/org/freedesktop/UDisks2/block_devices/sdb1"));
}

TEST_F(DeviceHelperExtraTest, DetachDevice_ProtocolDevice_ReturnsTrue)
{
    DeviceHelper helper;
    EXPECT_TRUE(helper.detachDevice("protocol_device_1"));
}

// ======================== isExist ========================

TEST_F(DeviceHelperExtraTest, IsExist_EmptyId_ReturnsFalse)
{
    DeviceHelper helper;
    EXPECT_FALSE(helper.isExist(""));
}

TEST_F(DeviceHelperExtraTest, IsExist_InCache_ReturnsTrue)
{
    DeviceHelper helper;
    auto &cache = access_private_field::DeviceHelperm_mapDevicesInfos(helper);
    QVariantMap info;
    info["MountPoint"] = "/media/usb";
    cache["dev1"] = info;

    EXPECT_TRUE(helper.isExist("dev1"));
}

TEST_F(DeviceHelperExtraTest, IsExist_NotInCache_ReturnsFalse)
{
    DeviceHelper helper;
    auto &cache = access_private_field::DeviceHelperm_mapDevicesInfos(helper);
    cache.clear();

    EXPECT_FALSE(helper.isExist("nonexistent_dev"));
}

// ======================== instance ========================

TEST_F(DeviceHelperExtraTest, Instance_ReturnsNonNull)
{
    DeviceHelper *inst = DeviceHelper::instance();
    EXPECT_NE(inst, nullptr);
}

TEST_F(DeviceHelperExtraTest, Instance_ReturnsSameInstance)
{
    DeviceHelper *inst1 = DeviceHelper::instance();
    DeviceHelper *inst2 = DeviceHelper::instance();
    EXPECT_EQ(inst1, inst2);
}
