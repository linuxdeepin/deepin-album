// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | FileInotify::getAllPicture | high | 4 | 3 | 4 |
 * | FileInotify::onNeedSendPictures | high | 3 | 3 | 3 |
 * | FileInotify::checkPendingDirectories | high | 3 | 3 | 3 |
 *
 * 分支列表 (getAllPicture):
 *   B1: m_currentDirs.isEmpty() → cleanup DB, emit pathDestroyed, return
 *   B2: dir.exists() == false → remove from m_currentDirs, continue
 *   B3: isFirst == true → find new files, skip delete check
 *   B4: isFirst == false → find new files AND deleted files
 *
 * 分支列表 (onNeedSendPictures):
 *   B1: m_newFile.empty() && m_deleteFile.empty() → skip sigMonitorChanged emit
 *   B2: m_newFile not empty → emit sigMonitorChanged, clear m_newFile
 *   B3: m_pendingFileSize has stable file (size unchanged) → emit sigVideoFileStable
 *
 * 分支列表 (checkPendingDirectories):
 *   B1: No pending dir matches changedPath → no change to m_currentDirs
 *   B2: Pending dir matches → moved from m_pendingDirs to m_currentDirs
 *   B3: Parent dir has no remaining children → removed from m_parentDirs
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
// getAllPicture() and onNeedSendPictures() are public and called directly.
ACCESS_PRIVATE_FUN(FileInotify, void(), checkNewPath)
ACCESS_PRIVATE_FUN(FileInotify, void(const QString &), checkPendingDirectories)
ACCESS_PRIVATE_FIELD(FileInotify, int, m_currentUID)
ACCESS_PRIVATE_FIELD(FileInotify, QStringList, m_newFile)
ACCESS_PRIVATE_FIELD(FileInotify, QStringList, m_deleteFile)
ACCESS_PRIVATE_FIELD(FileInotify, QStringList, m_currentDirs)
ACCESS_PRIVATE_FIELD(FileInotify, QStringList, m_pendingDirs)
ACCESS_PRIVATE_FIELD(FileInotify, QStringList, m_parentDirs)
ACCESS_PRIVATE_FIELD(FileInotify, QStringList, m_Supported)
ACCESS_PRIVATE_FIELD(FileInotify, StringListMap, m_parentToChildren)
ACCESS_PRIVATE_FIELD(FileInotify, StringInt64Map, m_pendingFileSize)

class FileInotifyTest : public ::testing::Test
{
protected:
    StubExt stub;

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

// ============ getAllPicture ============

// B1: Empty m_currentDirs → emit pathDestroyed, cleanup
TEST_F(FileInotifyTest, GetAllPicture_EmptyDirs_EmitsPathDestroyed)
{
    // Arrange
    auto inotify = createInotify();
    access_private_field::FileInotifym_currentDirs(*inotify).clear();
    access_private_field::FileInotifym_currentUID(*inotify) = 42;

    QSignalSpy spy(inotify.get(), &FileInotify::pathDestroyed);

    // Act
    inotify->getAllPicture(true);

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toInt(), 42);
}

// B2: Non-existent dir in m_currentDirs → removed from list
TEST_F(FileInotifyTest, GetAllPicture_NonExistentDir_RemovedFromList)
{
    // Arrange
    auto inotify = createInotify();
    QTemporaryDir tmpDir;  // Use temp dir for unique non-existent path
    QString nonExistentPath = tmpDir.filePath("nonexistent_99999");
    access_private_field::FileInotifym_currentDirs(*inotify).clear();
    access_private_field::FileInotifym_currentDirs(*inotify) << nonExistentPath;
    access_private_field::FileInotifym_currentUID(*inotify) = 1;

    QSignalSpy spy(inotify.get(), &FileInotify::pathDestroyed);

    // Act
    inotify->getAllPicture(true);

    // Assert — dir removed, list becomes empty, pathDestroyed emitted
    EXPECT_TRUE(access_private_field::FileInotifym_currentDirs(*inotify).isEmpty());
    EXPECT_EQ(spy.count(), 1);
}

// B3: isFirst=true → find new files, no delete check
TEST_F(FileInotifyTest, GetAllPicture_IsFirstTrue_FindsNewFiles)
{
    // Arrange
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    // Create a test file
    QString filePath = tmpDir.filePath("test.jpg");
    QFile testFile(filePath);
    testFile.open(QIODevice::WriteOnly);
    testFile.write("test");
    testFile.close();

    auto inotify = createInotify();
    access_private_field::FileInotifym_currentDirs(*inotify).clear();
    access_private_field::FileInotifym_currentDirs(*inotify) << tmpDir.path();
    access_private_field::FileInotifym_currentUID(*inotify) = 1;

    if (!access_private_field::FileInotifym_Supported(*inotify).contains("JPG"))
        access_private_field::FileInotifym_Supported(*inotify) << "JPG";

    // Stub getAllFileInDir to return our test file
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getAllFileInDir),
        [filePath](const QDir &, QFileInfoList &list) -> void {
            list.clear();
            list << QFileInfo(filePath);
        });

    // DBManager returns empty list (no existing files)
    stub.set_lamda(ADDR(DBManager, getPathsByAlbum),
        [](DBManager *, int) -> QStringList { return QStringList(); });

    access_private_field::FileInotifym_newFile(*inotify).clear();

    // Act
    inotify->getAllPicture(true);

    // Assert
    EXPECT_FALSE(access_private_field::FileInotifym_newFile(*inotify).isEmpty());
    EXPECT_GT(access_private_field::FileInotifym_newFile(*inotify).size(), 0);
}

// B4: isFirst=false → find new and deleted files
TEST_F(FileInotifyTest, GetAllPicture_IsFirstFalse_FindsDeletedFiles)
{
    // Arrange
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());

    auto inotify = createInotify();
    access_private_field::FileInotifym_currentDirs(*inotify).clear();
    access_private_field::FileInotifym_currentDirs(*inotify) << tmpDir.path();
    access_private_field::FileInotifym_currentUID(*inotify) = 1;

    if (!access_private_field::FileInotifym_Supported(*inotify).contains("JPG"))
        access_private_field::FileInotifym_Supported(*inotify) << "JPG";

    // Stub getAllFileInDir to return empty (all files deleted)
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getAllFileInDir),
        [](const QDir &, QFileInfoList &list) -> void { list.clear(); });

    // DBManager returns a path that no longer exists (deleted file)
    QString deletedPath = tmpDir.filePath("deleted.jpg");
    stub.set_lamda(ADDR(DBManager, getPathsByAlbum),
        [deletedPath](DBManager *, int) -> QStringList { return {deletedPath}; });

    access_private_field::FileInotifym_deleteFile(*inotify).clear();

    // Act
    inotify->getAllPicture(false);

    // Assert — deleted file should be in m_deleteFile
    EXPECT_FALSE(access_private_field::FileInotifym_deleteFile(*inotify).isEmpty());
    EXPECT_GT(access_private_field::FileInotifym_deleteFile(*inotify).size(), 0);
}

// ============ onNeedSendPictures ============

// B1: Empty m_newFile and m_deleteFile → no sigMonitorChanged emitted
TEST_F(FileInotifyTest, OnNeedSendPictures_EmptyLists_NoSignalEmitted)
{
    // Arrange
    auto inotify = createInotify();
    access_private_field::FileInotifym_newFile(*inotify).clear();
    access_private_field::FileInotifym_deleteFile(*inotify).clear();

    // Stub checkNewPath and getAllPicture to avoid side effects
    stub.set_lamda(get_private_fun::FileInotifycheckNewPath(),
        [](FileInotify *) -> void {});
    stub.set_lamda(ADDR(FileInotify, getAllPicture),
        [](FileInotify *, bool) -> void {});

    // Clear pending video files to avoid timer start
    access_private_field::FileInotifym_pendingFileSize(*inotify).clear();

    QSignalSpy spy(inotify.get(), &FileInotify::sigMonitorChanged);

    // Act
    inotify->onNeedSendPictures();

    // Assert
    EXPECT_EQ(spy.count(), 0);
    EXPECT_TRUE(access_private_field::FileInotifym_newFile(*inotify).isEmpty());
}

// B2: Non-empty m_newFile → emit sigMonitorChanged, clear m_newFile
TEST_F(FileInotifyTest, OnNeedSendPictures_NonEmptyNewFile_EmitsSigMonitorChanged)
{
    // Arrange
    QTemporaryDir tmpDir;
    auto inotify = createInotify();
    access_private_field::FileInotifym_newFile(*inotify).clear();
    access_private_field::FileInotifym_newFile(*inotify) << tmpDir.filePath("test_new.jpg");
    access_private_field::FileInotifym_deleteFile(*inotify).clear();

    stub.set_lamda(get_private_fun::FileInotifycheckNewPath(),
        [](FileInotify *) -> void {});
    stub.set_lamda(ADDR(FileInotify, getAllPicture),
        [](FileInotify *, bool) -> void {});

    access_private_field::FileInotifym_pendingFileSize(*inotify).clear();

    QSignalSpy spy(inotify.get(), &FileInotify::sigMonitorChanged);

    // Act
    inotify->onNeedSendPictures();

    // Assert
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(access_private_field::FileInotifym_newFile(*inotify).isEmpty());  // Cleared after emit
}

// B3: m_pendingFileSize has stable file → emit sigVideoFileStable
TEST_F(FileInotifyTest, OnNeedSendPictures_StableVideoFile_EmitsSigVideoFileStable)
{
    // Arrange
    QTemporaryFile tmpVideo;
    tmpVideo.open();
    tmpVideo.write("video");
    tmpVideo.flush();
    QString videoPath = tmpVideo.fileName();

    auto inotify = createInotify();
    access_private_field::FileInotifym_newFile(*inotify).clear();
    access_private_field::FileInotifym_deleteFile(*inotify).clear();

    stub.set_lamda(get_private_fun::FileInotifycheckNewPath(),
        [](FileInotify *) -> void {});
    stub.set_lamda(ADDR(FileInotify, getAllPicture),
        [](FileInotify *, bool) -> void {});

    // Set up pendingFileSize with a stable file (size already recorded)
    access_private_field::FileInotifym_pendingFileSize(*inotify).clear();
    qint64 fileSize = QFileInfo(videoPath).size();
    access_private_field::FileInotifym_pendingFileSize(*inotify)[videoPath] = fileSize;  // Same size = stable

    QSignalSpy spy(inotify.get(), &FileInotify::sigVideoFileStable);

    // Act
    inotify->onNeedSendPictures();

    // Assert
    EXPECT_EQ(spy.count(), 1);
    auto args = spy.takeFirst();
    QStringList stableFiles = args.at(0).toStringList();
    EXPECT_TRUE(stableFiles.contains(videoPath));
}

// ============ checkPendingDirectories ============

// B1: No matching pending dirs → no change to m_currentDirs
TEST_F(FileInotifyTest, CheckPendingDirectories_NoMatch_NoChange)
{
    // Arrange
    QTemporaryDir tmpDir;
    auto inotify = createInotify();
    access_private_field::FileInotifym_pendingDirs(*inotify).clear();
    access_private_field::FileInotifym_pendingDirs(*inotify) << tmpDir.filePath("pending_dir_1");
    access_private_field::FileInotifym_currentDirs(*inotify).clear();
    access_private_field::FileInotifym_currentDirs(*inotify) << tmpDir.filePath("existing_dir");

    // Act — changedPath doesn't match any pending dir's parent
    call_private_fun::FileInotifycheckPendingDirectories(*inotify, tmpDir.filePath("completely_different_path"));

    // Assert
    EXPECT_EQ(access_private_field::FileInotifym_currentDirs(*inotify).size(), 1);
    EXPECT_EQ(access_private_field::FileInotifym_pendingDirs(*inotify).size(), 1);
}

// B2: Matching pending dir → moved from m_pendingDirs to m_currentDirs
TEST_F(FileInotifyTest, CheckPendingDirectories_MatchingDir_MovedToCurrentDirs)
{
    // Arrange
    QTemporaryDir tmpParent;
    ASSERT_TRUE(tmpParent.isValid());
    // Create the pending directory
    QString pendingDir = tmpParent.filePath("subdir");
    QDir().mkpath(pendingDir);
    ASSERT_TRUE(QFileInfo(pendingDir).exists());

    auto inotify = createInotify();
    access_private_field::FileInotifym_pendingDirs(*inotify).clear();
    access_private_field::FileInotifym_pendingDirs(*inotify) << pendingDir;
    access_private_field::FileInotifym_currentDirs(*inotify).clear();

    // Act — changedPath is the parent of the pending dir
    call_private_fun::FileInotifycheckPendingDirectories(*inotify, tmpParent.path());

    // Assert
    EXPECT_TRUE(access_private_field::FileInotifym_pendingDirs(*inotify).isEmpty());
    EXPECT_EQ(access_private_field::FileInotifym_currentDirs(*inotify).size(), 1);
    EXPECT_TRUE(access_private_field::FileInotifym_currentDirs(*inotify).contains(pendingDir));
}

// B3: Parent dir has no remaining children → removed from m_parentDirs
TEST_F(FileInotifyTest, CheckPendingDirectories_ParentCleanup_RemovedFromParentDirs)
{
    // Arrange
    QTemporaryDir tmpParent;
    ASSERT_TRUE(tmpParent.isValid());
    QString pendingDir = tmpParent.filePath("subdir");
    QDir().mkpath(pendingDir);
    ASSERT_TRUE(QFileInfo(pendingDir).exists());

    auto inotify = createInotify();
    access_private_field::FileInotifym_pendingDirs(*inotify).clear();
    access_private_field::FileInotifym_pendingDirs(*inotify) << pendingDir;
    access_private_field::FileInotifym_currentDirs(*inotify).clear();
    access_private_field::FileInotifym_parentDirs(*inotify).clear();
    access_private_field::FileInotifym_parentDirs(*inotify) << tmpParent.path();
    access_private_field::FileInotifym_parentToChildren(*inotify).clear();
    access_private_field::FileInotifym_parentToChildren(*inotify)[tmpParent.path()] = QStringList{pendingDir};  // Only one child

    // Act
    call_private_fun::FileInotifycheckPendingDirectories(*inotify, tmpParent.path());

    // Assert — parent should be removed since no children remain
    EXPECT_EQ(access_private_field::FileInotifym_parentDirs(*inotify).size(), 0);
    EXPECT_FALSE(access_private_field::FileInotifym_parentToChildren(*inotify).contains(tmpParent.path()));
}
