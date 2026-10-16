// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Extended unit tests for FileTrashHelper — covers fileCanTrash, moveFileToTrash,
 * removeFile, resetMountInfo, isExternalDevice, isGvfsFile, moveFileToTrashWithDBus.
 *
 * DeletionResult enum values (defined in filetrashhelper.cpp, not exported in header):
 *   kTrashTimeout = 0, kTrashInterfaceError = 1, kTrashInvalidUrl = 2, kTrashSuccess = 3
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QTemporaryFile>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QMultiHash>
#include <QString>
#include <QtDBus/QDBusPendingCall>

#include "stubext.h"
#include "addr_pri.h"
#include "utils/filetrashhelper.h"
#include "unionimage/baseutils.h"

using namespace stub_ext;

// ---- DeletionResult constants (matching enum in filetrashhelper.cpp) ----
static constexpr int TRASH_TIMEOUT = 0;
static constexpr int TRASH_INTERFACE_ERROR = 1;
static constexpr int TRASH_INVALID_URL = 2;
static constexpr int TRASH_SUCCESS = 3;

// ---- Typedefs to avoid commas in macros ----
using StringStringHash = QMultiHash<QString, QString>;

// ---- Private field access ----
ACCESS_PRIVATE_FIELD(FileTrashHelper, bool, initData)
ACCESS_PRIVATE_FIELD(FileTrashHelper, StringStringHash, mountDevices)
ACCESS_PRIVATE_FIELD(FileTrashHelper, QDir, lastDir)

// ---- Private function access ----
ACCESS_PRIVATE_FUN(FileTrashHelper, void(), queryMountInfo)
ACCESS_PRIVATE_FUN(FileTrashHelper, bool(const QString &), isExternalDevice)
ACCESS_PRIVATE_FUN(FileTrashHelper, bool(const QUrl &) const, isGvfsFile)
ACCESS_PRIVATE_FUN(FileTrashHelper, int(const QUrl &), moveFileToTrashWithDBus)
ACCESS_PRIVATE_FUN(QDBusAbstractInterface, QDBusPendingCall(const QString &, const QVariant *, size_t), doAsyncCall)

class FileTrashHelperExtTest : public ::testing::Test
{
protected:
    StubExt stub;

    void SetUp() override
    {
        // DBus services are unavailable in the test environment
    }

    void TearDown() override
    {
        stub.clear();
    }

    // Helper: set initData=true so queryMountInfo is a no-op
    void skipMountQuery(FileTrashHelper &helper)
    {
        access_private_field::FileTrashHelperinitData(helper) = true;
    }
};

// =========================================================================
// fileCanTrash
// =========================================================================

// Local existing file, non-gvfs, non-external → true
TEST_F(FileTrashHelperExtTest, FileCanTrash_LocalExistingFile_ReturnsTrue)
{
    FileTrashHelper helper;
    skipMountQuery(helper);

    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.close();
    QUrl url = QUrl::fromLocalFile(tmp.fileName());

    EXPECT_TRUE(helper.fileCanTrash(url));
}

// Local nonexistent path, non-gvfs, non-external → true
// Note: the source code does NOT check file existence — it only checks
// gvfs/external-device paths. A nonexistent local path returns true.
TEST_F(FileTrashHelperExtTest, FileCanTrash_NonexistentLocalPath_ReturnsTrue)
{
    FileTrashHelper helper;
    skipMountQuery(helper);

    QUrl url = QUrl::fromLocalFile("/tmp/nonexistent_file_12345.txt");
    EXPECT_TRUE(helper.fileCanTrash(url));
}

// GVFS mount path (/run/user/<uid>/gvfs/...) → false
TEST_F(FileTrashHelperExtTest, FileCanTrash_GvfsPath_ReturnsFalse)
{
    FileTrashHelper helper;
    skipMountQuery(helper);

    QUrl url = QUrl::fromLocalFile("/run/user/1000/gvfs/mtp:host/Test");
    EXPECT_FALSE(helper.fileCanTrash(url));
}

// External device path (simulated via mountDevices) → false
TEST_F(FileTrashHelperExtTest, FileCanTrash_ExternalDevicePath_ReturnsFalse)
{
    FileTrashHelper helper;
    auto &mounts = access_private_field::FileTrashHelpermountDevices(helper);
    mounts.clear();
    mounts.insert("usb_dev_1", "/media/external_usb");
    skipMountQuery(helper);

    // Stub resetMountInfo to prevent clearing mountDevices on directory change
    stub.set_lamda(ADDR(FileTrashHelper, resetMountInfo), [](FileTrashHelper *) {});

    QUrl url = QUrl::fromLocalFile("/media/external_usb/photo.jpg");
    EXPECT_FALSE(helper.fileCanTrash(url));
}

// Non-gvfs, non-external local path → true
TEST_F(FileTrashHelperExtTest, FileCanTrash_NormalLocalPath_ReturnsTrue)
{
    FileTrashHelper helper;
    auto &mounts = access_private_field::FileTrashHelpermountDevices(helper);
    mounts.clear();
    skipMountQuery(helper);

    QUrl url = QUrl::fromLocalFile("/home/user/pictures/photo.jpg");
    EXPECT_TRUE(helper.fileCanTrash(url));
}

// Directory change triggers resetMountInfo then re-query
TEST_F(FileTrashHelperExtTest, FileCanTrash_DirectoryChange_TriggersResetAndRequery)
{
    FileTrashHelper helper;
    auto &init = access_private_field::FileTrashHelperinitData(helper);
    auto &mounts = access_private_field::FileTrashHelpermountDevices(helper);
    auto &lastDir = access_private_field::FileTrashHelperlastDir(helper);

    // Simulate prior state: initialized with mount data in directory A
    init = true;
    mounts.insert("dev1", "/media/usb1");
    lastDir = QDir("/tmp");

    // Stub queryMountInfo to just set initData=true (simulating successful query)
    stub.set_lamda(get_private_fun::FileTrashHelperqueryMountInfo(),
                   [](FileTrashHelper *self) {
                       access_private_field::FileTrashHelperinitData(*self) = true;
                   });

    // Call with a different directory → triggers resetMountInfo → initData becomes false
    QUrl url = QUrl::fromLocalFile("/home/user/pictures/img.jpg");
    EXPECT_TRUE(helper.fileCanTrash(url));

    // After call: initData should be true again (re-queried via stub),
    // and mounts should be empty (reset cleared them, stub didn't add any)
    EXPECT_TRUE(init);
    EXPECT_TRUE(mounts.isEmpty());
}

// =========================================================================
// moveFileToTrash
// =========================================================================

// moveFileToTrashWithDBus returns kTrashTimeout → moveFileToTrash returns false
TEST_F(FileTrashHelperExtTest, MoveFileToTrash_DBusTimeout_ReturnsFalse)
{
    FileTrashHelper helper;

    stub.set_lamda(get_private_fun::FileTrashHelpermoveFileToTrashWithDBus(),
                   [](FileTrashHelper *, const QUrl &) -> int {
                       return TRASH_TIMEOUT;
                   });

    QUrl url = QUrl::fromLocalFile("/tmp/nonexistent_for_trash.txt");
    EXPECT_FALSE(helper.moveFileToTrash(url));
}

// moveFileToTrashWithDBus returns kTrashSuccess → moveFileToTrash returns true
TEST_F(FileTrashHelperExtTest, MoveFileToTrash_DBusSuccess_ReturnsTrue)
{
    FileTrashHelper helper;

    stub.set_lamda(get_private_fun::FileTrashHelpermoveFileToTrashWithDBus(),
                   [](FileTrashHelper *, const QUrl &) -> int {
                       return TRASH_SUCCESS;
                   });

    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.close();
    QUrl url = QUrl::fromLocalFile(tmp.fileName());

    EXPECT_TRUE(helper.moveFileToTrash(url));
}

// moveFileToTrashWithDBus returns kTrashInterfaceError, trashFile succeeds → true
TEST_F(FileTrashHelperExtTest, MoveFileToTrash_InterfaceErrorFallbackSuccess_ReturnsTrue)
{
    FileTrashHelper helper;

    stub.set_lamda(get_private_fun::FileTrashHelpermoveFileToTrashWithDBus(),
                   [](FileTrashHelper *, const QUrl &) -> int {
                       return TRASH_INTERFACE_ERROR;
                   });

    stub.set_lamda(ADDR(Libutils::base, trashFile),
                   [](const QString &) -> bool {
                       return true;
                   });

    QUrl url = QUrl::fromLocalFile("/tmp/some_local_file.txt");
    EXPECT_TRUE(helper.moveFileToTrash(url));
}

// moveFileToTrashWithDBus returns kTrashInterfaceError, trashFile fails → false
TEST_F(FileTrashHelperExtTest, MoveFileToTrash_InterfaceErrorFallbackFail_ReturnsFalse)
{
    FileTrashHelper helper;

    stub.set_lamda(get_private_fun::FileTrashHelpermoveFileToTrashWithDBus(),
                   [](FileTrashHelper *, const QUrl &) -> int {
                       return TRASH_INTERFACE_ERROR;
                   });

    stub.set_lamda(ADDR(Libutils::base, trashFile),
                   [](const QString &) -> bool {
                       return false;
                   });

    QUrl url = QUrl::fromLocalFile("/tmp/some_local_file.txt");
    EXPECT_FALSE(helper.moveFileToTrash(url));
}

// moveFileToTrashWithDBus returns kTrashInvalidUrl → moveFileToTrash returns false
TEST_F(FileTrashHelperExtTest, MoveFileToTrash_InvalidUrl_ReturnsFalse)
{
    FileTrashHelper helper;

    stub.set_lamda(get_private_fun::FileTrashHelpermoveFileToTrashWithDBus(),
                   [](FileTrashHelper *, const QUrl &) -> int {
                       return TRASH_INVALID_URL;
                   });

    QUrl url;
    EXPECT_FALSE(helper.moveFileToTrash(url));
}

// =========================================================================
// removeFile
// =========================================================================

// Existing temp file → true, file is deleted
TEST_F(FileTrashHelperExtTest, RemoveFile_ExistingFile_ReturnsTrueAndDeleted)
{
    FileTrashHelper helper;

    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    QString filePath = tmp.fileName();
    tmp.close();

    QUrl url = QUrl::fromLocalFile(filePath);
    EXPECT_TRUE(helper.removeFile(url));
    EXPECT_FALSE(QFile::exists(filePath));
}

// Nonexistent file → false
TEST_F(FileTrashHelperExtTest, RemoveFile_NonexistentFile_ReturnsFalse)
{
    FileTrashHelper helper;

    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    QString filePath = tmp.fileName();
    tmp.close();
    tmp.remove();

    QUrl url = QUrl::fromLocalFile(filePath);
    EXPECT_FALSE(helper.removeFile(url));
}

// Directory → false, no crash (QFile::remove on a directory fails)
TEST_F(FileTrashHelperExtTest, RemoveFile_Directory_NoCrashReturnsFalse)
{
    FileTrashHelper helper;

    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());

    QUrl url = QUrl::fromLocalFile(tmpDir.path());
    EXPECT_FALSE(helper.removeFile(url));
    EXPECT_TRUE(QDir(tmpDir.path()).exists());
}

// =========================================================================
// resetMountInfo
// =========================================================================

// resetMountInfo sets initData=false and clears mountDevices
TEST_F(FileTrashHelperExtTest, ResetMountInfo_ClearsInitAndMountDevices)
{
    FileTrashHelper helper;
    auto &init = access_private_field::FileTrashHelperinitData(helper);
    auto &mounts = access_private_field::FileTrashHelpermountDevices(helper);

    init = true;
    mounts.insert("dev1", "/media/usb1");
    mounts.insert("dev2", "/media/usb2");
    ASSERT_EQ(mounts.size(), 2);

    helper.resetMountInfo();

    EXPECT_FALSE(init);
    EXPECT_TRUE(mounts.isEmpty());
}

// After resetMountInfo, the next fileCanTrash call triggers queryMountInfo
// (verified by stubbing queryMountInfo and checking it was invoked)
TEST_F(FileTrashHelperExtTest, ResetMountInfo_NextFileCanTrash_Requeries)
{
    FileTrashHelper helper;
    auto &init = access_private_field::FileTrashHelperinitData(helper);
    auto &lastDir = access_private_field::FileTrashHelperlastDir(helper);

    // Pre-condition: initData is true (already queried)
    init = true;
    // Align lastDir with the URL directory so fileCanTrash does not treat the
    // call as a directory change — otherwise the directory-change reset would
    // trigger queryMountInfo regardless of the explicit reset below.
    lastDir = QDir("/home/user");

    // Stub queryMountInfo to track invocation
    bool queryCalled = false;
    stub.set_lamda(get_private_fun::FileTrashHelperqueryMountInfo(),
                   [&queryCalled](FileTrashHelper *self) {
                       queryCalled = true;
                       access_private_field::FileTrashHelperinitData(*self) = true;
                   });

    // resetMountInfo → initData becomes false
    helper.resetMountInfo();
    EXPECT_FALSE(init);

    // Next fileCanTrash should call queryMountInfo (because initData is false)
    QUrl url = QUrl::fromLocalFile("/home/user/test.jpg");
    helper.fileCanTrash(url);

    EXPECT_TRUE(queryCalled);
    EXPECT_TRUE(init);
}

// =========================================================================
// isExternalDevice
// =========================================================================

// Path matches a mount device prefix → true
TEST_F(FileTrashHelperExtTest, IsExternalDevice_PathMatches_ReturnsTrue)
{
    FileTrashHelper helper;
    auto &mounts = access_private_field::FileTrashHelpermountDevices(helper);
    mounts.clear();
    mounts.insert("usb_dev", "/media/external_drive");

    bool result = call_private_fun::FileTrashHelperisExternalDevice(helper, "/media/external_drive/photo.jpg");
    EXPECT_TRUE(result);
}

// Path does not match any mount device → false
TEST_F(FileTrashHelperExtTest, IsExternalDevice_PathNoMatch_ReturnsFalse)
{
    FileTrashHelper helper;
    auto &mounts = access_private_field::FileTrashHelpermountDevices(helper);
    mounts.clear();
    mounts.insert("usb_dev", "/media/external_drive");

    bool result = call_private_fun::FileTrashHelperisExternalDevice(helper, "/home/user/photo.jpg");
    EXPECT_FALSE(result);
}

// Empty mountDevices → false
TEST_F(FileTrashHelperExtTest, IsExternalDevice_EmptyMountDevices_ReturnsFalse)
{
    FileTrashHelper helper;
    auto &mounts = access_private_field::FileTrashHelpermountDevices(helper);
    mounts.clear();

    bool result = call_private_fun::FileTrashHelperisExternalDevice(helper, "/any/path/file.jpg");
    EXPECT_FALSE(result);
}

// =========================================================================
// isGvfsFile
// =========================================================================

// GVFS path (/run/user/<uid>/gvfs/...) → true
TEST_F(FileTrashHelperExtTest, IsGvfsFile_GvfsMountPath_ReturnsTrue)
{
    FileTrashHelper helper;

    QUrl url = QUrl::fromLocalFile("/run/user/1000/gvfs/mtp:host/DCIM/IMG_001.jpg");
    bool result = call_private_fun::FileTrashHelperisGvfsFile(helper, url);
    EXPECT_TRUE(result);
}

// Root gvfs path (/root/.gvfs/...) → true
TEST_F(FileTrashHelperExtTest, IsGvfsFile_RootGvfsPath_ReturnsTrue)
{
    FileTrashHelper helper;

    QUrl url = QUrl::fromLocalFile("/root/.gvfs/smb-share/file.txt");
    bool result = call_private_fun::FileTrashHelperisGvfsFile(helper, url);
    EXPECT_TRUE(result);
}

// SMB mount path (/media/.../smbmounts) → true
TEST_F(FileTrashHelperExtTest, IsGvfsFile_SmbMountPath_ReturnsTrue)
{
    FileTrashHelper helper;

    QUrl url = QUrl::fromLocalFile("/media/user/smbmounts/share/file.txt");
    bool result = call_private_fun::FileTrashHelperisGvfsFile(helper, url);
    EXPECT_TRUE(result);
}

// Normal local path → false
TEST_F(FileTrashHelperExtTest, IsGvfsFile_NormalPath_ReturnsFalse)
{
    FileTrashHelper helper;

    QUrl url = QUrl::fromLocalFile("/home/user/pictures/photo.jpg");
    bool result = call_private_fun::FileTrashHelperisGvfsFile(helper, url);
    EXPECT_FALSE(result);
}

// Invalid URL → false
TEST_F(FileTrashHelperExtTest, IsGvfsFile_InvalidUrl_ReturnsFalse)
{
    FileTrashHelper helper;

    QUrl url = QUrl("http://[::1", QUrl::StrictMode);
    ASSERT_FALSE(url.isValid());
    bool result = call_private_fun::FileTrashHelperisGvfsFile(helper, url);
    EXPECT_FALSE(result);
}

// =========================================================================
// moveFileToTrashWithDBus
// =========================================================================

// Invalid URL → returns kTrashInvalidUrl (2)
TEST_F(FileTrashHelperExtTest, MoveFileToTrashWithDBus_InvalidUrl_ReturnsInvalidUrl)
{
    FileTrashHelper helper;

    QUrl url = QUrl("http://[::1", QUrl::StrictMode);
    ASSERT_FALSE(url.isValid());
    int result = call_private_fun::FileTrashHelpermoveFileToTrashWithDBus(helper, url);
    EXPECT_EQ(result, TRASH_INVALID_URL);
}

// Valid URL, DBus service unavailable → returns kTrashInterfaceError (1)
TEST_F(FileTrashHelperExtTest, MoveFileToTrashWithDBus_DBusUnavailable_ReturnsInterfaceError)
{
    FileTrashHelper helper;

    // Intercept the DBus request itself (doAsyncCall is the common entry of
    // QDBusAbstractInterface::asyncCall) so no real Trash call is sent to the
    // file manager daemon — otherwise the daemon would move the file to trash
    // and pop a delete prompt in the UI. Without this stub a real
    // waitForFinished() would also block forever, since the test process has
    // no QApplication and thus no event dispatcher.
    stub.set_lamda(get_private_fun::QDBusAbstractInterfacedoAsyncCall(),
                   [](QDBusAbstractInterface *, const QString &, const QVariant *, size_t) -> QDBusPendingCall {
                       return QDBusPendingCall::fromError(
                           QDBusError(QDBusError::ServiceUnknown, QStringLiteral("no DBus service in unit tests")));
                   });

    QTemporaryFile tmp;
    ASSERT_TRUE(tmp.open());
    tmp.close();
    QUrl url = QUrl::fromLocalFile(tmp.fileName());

    int result = call_private_fun::FileTrashHelpermoveFileToTrashWithDBus(helper, url);
    EXPECT_EQ(result, TRASH_INTERFACE_ERROR);
}
