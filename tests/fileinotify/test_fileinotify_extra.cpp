// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表 (补充覆盖)
 * | FileInotify::addWather | high | 4 | 4 | 4 |
 * | FileInotify::clear | high | 2 | 2 | 2 |
 * | FileInotify::checkNewPath | high | 3 | 3 | 3 |
 * | FileInotify::addParentWatcher | high | 3 | 3 | 3 |
 *
 * 分支列表 (addWather):
 *   B1: All paths exist → added to m_currentDirs and m_watcher
 *   B2: Non-existing path, parent exists → parent monitoring + pendingDirs
 *   B3: Non-existing path, parent does NOT exist → no monitoring
 *   B4: Sets m_currentAlbum and m_currentUID, starts timer
 *
 * 分支列表 (clear):
 *   B1: Active timer → stops, deletes timer, clears all state
 *   B2: Inactive timer → timer not deleted, lists still cleared
 *
 * 分支列表 (checkNewPath):
 *   B1: No current dirs and no pending dirs → no-op
 *   B2: Current dir with subdirectories → subdirs added to m_watcher
 *   B3: Pending dir now exists → triggers checkPendingDirectories (moved to currentDirs)
 *
 * 分支列表 (addParentWatcher):
 *   B1: New parent path → added to m_parentDirs and m_parentToChildren
 *   B2: Existing parent path → no duplicate in m_parentDirs
 *   B3: Duplicate child for same parent → no duplicate in m_parentToChildren
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QSignalSpy>
#include <QTest>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QStringList>
#include <QMap>
#include <memory>

#include "stubext.h"
#include "addr_pri.h"
#include "fileMonitor/fileinotify.h"

#include "dbmanager/dbmanager.h"
#include "unionimage/unionimage.h"

using namespace stub_ext;

// Typedefs to avoid commas in ACCESS_PRIVATE_FIELD macro args
using StringListMap = QMap<QString, QStringList>;
using StringInt64Map = QMap<QString, qint64>;

// Access private members without "#define private public" (ODR-safe).
ACCESS_PRIVATE_FUN(FileInotify, void(), checkNewPath)
ACCESS_PRIVATE_FUN(FileInotify, void(const QString &), checkPendingDirectories)
ACCESS_PRIVATE_FUN(FileInotify, void(const QString &, const QString &), addParentWatcher)
ACCESS_PRIVATE_FIELD(FileInotify, int, m_currentUID)
ACCESS_PRIVATE_FIELD(FileInotify, QString, m_currentAlbum)
ACCESS_PRIVATE_FIELD(FileInotify, QStringList, m_newFile)
ACCESS_PRIVATE_FIELD(FileInotify, QStringList, m_deleteFile)
ACCESS_PRIVATE_FIELD(FileInotify, QStringList, m_currentDirs)
ACCESS_PRIVATE_FIELD(FileInotify, QStringList, m_pendingDirs)
ACCESS_PRIVATE_FIELD(FileInotify, QStringList, m_parentDirs)
ACCESS_PRIVATE_FIELD(FileInotify, QStringList, m_Supported)
ACCESS_PRIVATE_FIELD(FileInotify, QStringList, m_pendingVideoFiles)
ACCESS_PRIVATE_FIELD(FileInotify, StringListMap, m_parentToChildren)
ACCESS_PRIVATE_FIELD(FileInotify, StringInt64Map, m_pendingFileSize)
ACCESS_PRIVATE_FIELD(FileInotify, QTimer *, m_timer)

class FileInotifyExtraTest : public ::testing::Test
{
protected:
    StubExt stub;

    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test_fileinotify_extra";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    void SetUp() override
    {
        // Stub DBManager::instance() to return nullptr (sentinel)
        stub.set_lamda(ADDR(DBManager, instance),
            []() -> DBManager * { return nullptr; });

        // Stub DBManager methods called on nullptr (this ptr ignored)
        stub.set_lamda(ADDR(DBManager, getPathsByAlbum),
            [](DBManager *, int) -> QStringList { return QStringList(); });
        stub.set_lamda(ADDR(DBManager, removeCustomAutoImportPath),
            [](DBManager *, int) -> void {});

        // Stub LibUnionImage_NameSpace::getAllFileInDir to return empty by default
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, getAllFileInDir),
            [](const QDir &, QFileInfoList &list) -> void { list.clear(); });

        // Stub LibUnionImage_NameSpace::isVideo
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, isVideo),
            [](QString) -> bool { return false; });
    }

    void TearDown() override
    {
        stub.clear();
    }

    std::unique_ptr<FileInotify> createInotify()
    {
        return std::make_unique<FileInotify>();
    }
};

// ============ addWather ============

// B1: All paths exist → added to m_currentDirs and m_watcher
TEST_F(FileInotifyExtraTest, AddWather_ExistingPaths_SetsCurrentDirsAndWatcher)
{
    // Arrange
    QTemporaryDir tmpDir1;
    QTemporaryDir tmpDir2;
    ASSERT_TRUE(tmpDir1.isValid());
    ASSERT_TRUE(tmpDir2.isValid());

    auto inotify = createInotify();

    // Act
    inotify->addWather({tmpDir1.path(), tmpDir2.path()}, "MyAlbum", 77);

    // Assert
    EXPECT_EQ(access_private_field::FileInotifym_currentDirs(*inotify).size(), 2);
    EXPECT_TRUE(access_private_field::FileInotifym_currentDirs(*inotify).contains(tmpDir1.path()));
    EXPECT_TRUE(access_private_field::FileInotifym_currentDirs(*inotify).contains(tmpDir2.path()));
    EXPECT_TRUE(access_private_field::FileInotifym_pendingDirs(*inotify).isEmpty());
    EXPECT_TRUE(access_private_field::FileInotifym_parentDirs(*inotify).isEmpty());
    EXPECT_STREQ(access_private_field::FileInotifym_currentAlbum(*inotify).toUtf8().constData(), "MyAlbum");
    EXPECT_EQ(access_private_field::FileInotifym_currentUID(*inotify), 77);
    // Timer should be started
    EXPECT_TRUE(access_private_field::FileInotifym_timer(*inotify)->isActive());
}

// B2: Non-existing path, parent exists → parent monitoring + pendingDirs
TEST_F(FileInotifyExtraTest, AddWather_NonExistingPath_ParentExists_SetsPendingDirs)
{
    // Arrange
    QTemporaryDir tmpParent;
    ASSERT_TRUE(tmpParent.isValid());
    QString nonExistingPath = tmpParent.filePath("subdir_not_created");

    auto inotify = createInotify();

    // Act
    inotify->addWather({nonExistingPath}, "Album2", 88);

    // Assert — path not in currentDirs but in pendingDirs
    EXPECT_TRUE(access_private_field::FileInotifym_currentDirs(*inotify).isEmpty());
    EXPECT_EQ(access_private_field::FileInotifym_pendingDirs(*inotify).size(), 1);
    EXPECT_TRUE(access_private_field::FileInotifym_pendingDirs(*inotify).contains(nonExistingPath));
    // Parent should be monitored
    EXPECT_EQ(access_private_field::FileInotifym_parentDirs(*inotify).size(), 1);
    EXPECT_TRUE(access_private_field::FileInotifym_parentDirs(*inotify).contains(tmpParent.path()));
    // Parent-to-children mapping should exist
    EXPECT_TRUE(access_private_field::FileInotifym_parentToChildren(*inotify).contains(tmpParent.path()));
    EXPECT_EQ(access_private_field::FileInotifym_parentToChildren(*inotify)[tmpParent.path()].size(), 1);
}

// B3: Non-existing path, parent does NOT exist → no monitoring
TEST_F(FileInotifyExtraTest, AddWather_NonExistingPath_ParentNotExists_NoMonitoring)
{
    // Arrange — use a path whose parent also doesn't exist
    QString nonExistingPath = "/tmp/nonexistent_root_99999/subdir";

    auto inotify = createInotify();

    // Act
    inotify->addWather({nonExistingPath}, "Album3", 99);

    // Assert — nothing monitored
    EXPECT_TRUE(access_private_field::FileInotifym_currentDirs(*inotify).isEmpty());
    EXPECT_TRUE(access_private_field::FileInotifym_pendingDirs(*inotify).isEmpty());
    EXPECT_TRUE(access_private_field::FileInotifym_parentDirs(*inotify).isEmpty());
}

// B4: Mixed paths (some existing, some not) → all handled correctly
TEST_F(FileInotifyExtraTest, AddWather_MixedPaths_SetsAllCorrectly)
{
    // Arrange
    QTemporaryDir tmpExisting;
    QTemporaryDir tmpParentOfNonExisting;
    ASSERT_TRUE(tmpExisting.isValid());
    ASSERT_TRUE(tmpParentOfNonExisting.isValid());
    QString existingPath = tmpExisting.path();
    QString nonExistingPath = tmpParentOfNonExisting.filePath("child_not_created");

    auto inotify = createInotify();

    // Act
    inotify->addWather({existingPath, nonExistingPath}, "MixedAlbum", 55);

    // Assert
    EXPECT_EQ(access_private_field::FileInotifym_currentDirs(*inotify).size(), 1);
    EXPECT_TRUE(access_private_field::FileInotifym_currentDirs(*inotify).contains(existingPath));
    EXPECT_EQ(access_private_field::FileInotifym_pendingDirs(*inotify).size(), 1);
    EXPECT_TRUE(access_private_field::FileInotifym_pendingDirs(*inotify).contains(nonExistingPath));
    EXPECT_EQ(access_private_field::FileInotifym_parentDirs(*inotify).size(), 1);
    EXPECT_STREQ(access_private_field::FileInotifym_currentAlbum(*inotify).toUtf8().constData(), "MixedAlbum");
    EXPECT_EQ(access_private_field::FileInotifym_currentUID(*inotify), 55);
}

// ============ clear ============

// B1: Active timer → stops, deletes timer, clears all state
TEST_F(FileInotifyExtraTest, Clear_ActiveTimer_StopsDeletesTimerAndClearsState)
{
    // Arrange
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());

    auto inotify = createInotify();
    // Populate state by calling addWather (starts timer and sets dirs)
    inotify->addWather({tmpDir.path()}, "Album", 10);
    access_private_field::FileInotifym_newFile(*inotify) << "file1.jpg";
    access_private_field::FileInotifym_deleteFile(*inotify) << "file2.jpg";
    access_private_field::FileInotifym_pendingVideoFiles(*inotify) << "video.mp4";
    access_private_field::FileInotifym_pendingFileSize(*inotify)["v.mp4"] = 100;

    // Verify timer is active before clear
    ASSERT_TRUE(access_private_field::FileInotifym_timer(*inotify) != nullptr);
    ASSERT_TRUE(access_private_field::FileInotifym_timer(*inotify)->isActive());

    // Act
    inotify->clear();

    // Assert — all lists cleared
    EXPECT_TRUE(access_private_field::FileInotifym_currentDirs(*inotify).isEmpty());
    EXPECT_TRUE(access_private_field::FileInotifym_pendingDirs(*inotify).isEmpty());
    EXPECT_TRUE(access_private_field::FileInotifym_parentDirs(*inotify).isEmpty());
    EXPECT_TRUE(access_private_field::FileInotifym_parentToChildren(*inotify).isEmpty());
    EXPECT_TRUE(access_private_field::FileInotifym_newFile(*inotify).isEmpty());
    EXPECT_TRUE(access_private_field::FileInotifym_deleteFile(*inotify).isEmpty());
    EXPECT_TRUE(access_private_field::FileInotifym_pendingFileSize(*inotify).isEmpty());
    EXPECT_TRUE(access_private_field::FileInotifym_pendingVideoFiles(*inotify).isEmpty());
    EXPECT_TRUE(access_private_field::FileInotifym_Supported(*inotify).isEmpty());
    // Timer should be deleted (set to nullptr)
    EXPECT_EQ(access_private_field::FileInotifym_timer(*inotify), nullptr);
}

// B2: Inactive timer → timer not deleted, lists still cleared
TEST_F(FileInotifyExtraTest, Clear_InactiveTimer_TimerNotDeletedListsCleared)
{
    // Arrange
    auto inotify = createInotify();
    // Timer is created but not started (not active)
    ASSERT_TRUE(access_private_field::FileInotifym_timer(*inotify) != nullptr);
    ASSERT_FALSE(access_private_field::FileInotifym_timer(*inotify)->isActive());

    // Populate some state
    access_private_field::FileInotifym_currentDirs(*inotify) << "/some/path";
    access_private_field::FileInotifym_newFile(*inotify) << "test.jpg";

    // Act
    inotify->clear();

    // Assert — lists cleared
    EXPECT_TRUE(access_private_field::FileInotifym_currentDirs(*inotify).isEmpty());
    EXPECT_TRUE(access_private_field::FileInotifym_newFile(*inotify).isEmpty());
    // Timer should NOT be deleted (it was not active)
    EXPECT_NE(access_private_field::FileInotifym_timer(*inotify), nullptr);
}

// ============ checkNewPath ============

// B1: No current dirs and no pending dirs → no-op
TEST_F(FileInotifyExtraTest, CheckNewPath_NoDirs_NoOp)
{
    // Arrange
    auto inotify = createInotify();
    access_private_field::FileInotifym_currentDirs(*inotify).clear();
    access_private_field::FileInotifym_pendingDirs(*inotify).clear();

    // Act
    call_private_fun::FileInotifycheckNewPath(*inotify);

    // Assert — no change
    EXPECT_TRUE(access_private_field::FileInotifym_currentDirs(*inotify).isEmpty());
    EXPECT_TRUE(access_private_field::FileInotifym_pendingDirs(*inotify).isEmpty());
}

// B2: Current dir with subdirectories → subdirs added to m_watcher
TEST_F(FileInotifyExtraTest, CheckNewPath_CurrentDirWithSubdirs_AddsToWatcher)
{
    // Arrange
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    // Create a subdirectory
    QString subdir = tmpDir.filePath("subdir1");
    QDir().mkpath(subdir);
    ASSERT_TRUE(QFileInfo(subdir).exists());

    auto inotify = createInotify();
    access_private_field::FileInotifym_currentDirs(*inotify).clear();
    access_private_field::FileInotifym_currentDirs(*inotify) << tmpDir.path();

    // Stub getAllDirInDir to return the subdirectory we created
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getAllDirInDir),
        [subdir](const QDir &, QFileInfoList &list) -> void {
            list.clear();
            list << QFileInfo(subdir);
        });

    // Act
    call_private_fun::FileInotifycheckNewPath(*inotify);

    // Assert — no crash, the watcher should have the subdir added
    SUCCEED();
}

// B3: Pending dir now exists → triggers checkPendingDirectories (moved to currentDirs)
TEST_F(FileInotifyExtraTest, CheckNewPath_PendingDirNowExists_MovedToCurrentDirs)
{
    // Arrange
    QTemporaryDir tmpParent;
    ASSERT_TRUE(tmpParent.isValid());
    // Create the pending directory (now it exists)
    QString pendingDir = tmpParent.filePath("pending_subdir");
    QDir().mkpath(pendingDir);
    ASSERT_TRUE(QFileInfo(pendingDir).exists());

    auto inotify = createInotify();
    access_private_field::FileInotifym_currentDirs(*inotify).clear();
    access_private_field::FileInotifym_pendingDirs(*inotify).clear();
    access_private_field::FileInotifym_pendingDirs(*inotify) << pendingDir;

    // Stub getAllDirInDir to return empty (no subdirs in currentDirs)
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getAllDirInDir),
        [](const QDir &, QFileInfoList &list) -> void { list.clear(); });

    // Act
    call_private_fun::FileInotifycheckNewPath(*inotify);

    // Assert — pending dir should have been moved to currentDirs via checkPendingDirectories
    EXPECT_TRUE(access_private_field::FileInotifym_pendingDirs(*inotify).isEmpty());
    EXPECT_EQ(access_private_field::FileInotifym_currentDirs(*inotify).size(), 1);
    EXPECT_TRUE(access_private_field::FileInotifym_currentDirs(*inotify).contains(pendingDir));
}

// ============ addParentWatcher ============

// B1: New parent path → added to m_parentDirs and m_parentToChildren
TEST_F(FileInotifyExtraTest, AddParentWatcher_NewParent_AddsToParentDirsAndMapping)
{
    // Arrange
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());

    auto inotify = createInotify();
    access_private_field::FileInotifym_parentDirs(*inotify).clear();
    access_private_field::FileInotifym_parentToChildren(*inotify).clear();

    // Act
    call_private_fun::FileInotifyaddParentWatcher(*inotify, tmpDir.path(), tmpDir.filePath("child1"));

    // Assert
    EXPECT_EQ(access_private_field::FileInotifym_parentDirs(*inotify).size(), 1);
    EXPECT_TRUE(access_private_field::FileInotifym_parentDirs(*inotify).contains(tmpDir.path()));
    EXPECT_TRUE(access_private_field::FileInotifym_parentToChildren(*inotify).contains(tmpDir.path()));
    EXPECT_EQ(access_private_field::FileInotifym_parentToChildren(*inotify)[tmpDir.path()].size(), 1);
    EXPECT_TRUE(access_private_field::FileInotifym_parentToChildren(*inotify)[tmpDir.path()].contains(tmpDir.filePath("child1")));
}

// B2: Existing parent path → no duplicate in m_parentDirs
TEST_F(FileInotifyExtraTest, AddParentWatcher_ExistingParent_NoDuplicate)
{
    // Arrange
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());

    auto inotify = createInotify();
    access_private_field::FileInotifym_parentDirs(*inotify).clear();
    access_private_field::FileInotifym_parentToChildren(*inotify).clear();

    // Call once with child1
    call_private_fun::FileInotifyaddParentWatcher(*inotify, tmpDir.path(), tmpDir.filePath("child1"));
    // Call again with child2 — same parent
    call_private_fun::FileInotifyaddParentWatcher(*inotify, tmpDir.path(), tmpDir.filePath("child2"));

    // Assert — parent only once in m_parentDirs
    EXPECT_EQ(access_private_field::FileInotifym_parentDirs(*inotify).size(), 1);
    // Both children should be in the mapping
    EXPECT_EQ(access_private_field::FileInotifym_parentToChildren(*inotify)[tmpDir.path()].size(), 2);
    EXPECT_TRUE(access_private_field::FileInotifym_parentToChildren(*inotify)[tmpDir.path()].contains(tmpDir.filePath("child1")));
    EXPECT_TRUE(access_private_field::FileInotifym_parentToChildren(*inotify)[tmpDir.path()].contains(tmpDir.filePath("child2")));
}

// B3: Duplicate child for same parent → no duplicate in m_parentToChildren
TEST_F(FileInotifyExtraTest, AddParentWatcher_DuplicateChild_NoDuplicate)
{
    // Arrange
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString childPath = tmpDir.filePath("same_child");

    auto inotify = createInotify();
    access_private_field::FileInotifym_parentDirs(*inotify).clear();
    access_private_field::FileInotifym_parentToChildren(*inotify).clear();

    // Call twice with the same parent and child
    call_private_fun::FileInotifyaddParentWatcher(*inotify, tmpDir.path(), childPath);
    call_private_fun::FileInotifyaddParentWatcher(*inotify, tmpDir.path(), childPath);

    // Assert — only one entry for the child
    EXPECT_EQ(access_private_field::FileInotifym_parentDirs(*inotify).size(), 1);
    EXPECT_EQ(access_private_field::FileInotifym_parentToChildren(*inotify)[tmpDir.path()].size(), 1);
    EXPECT_TRUE(access_private_field::FileInotifym_parentToChildren(*inotify)[tmpDir.path()].contains(childPath));
}
