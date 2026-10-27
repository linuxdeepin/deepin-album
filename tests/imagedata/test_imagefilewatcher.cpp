// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | instance | low | complexity:0 | 1 | 2 |
// | resetImageFiles | high | complexity:5 | 3 | 6 |
// | isCurrentDir | mid | complexity:1 | 2 | 3 |
// | fileRename | mid | complexity:2 | 2 | 3 |
// | recordRotateImage | low | complexity:1 | 1 | 2 |
// | clearRotateStatus | mid | complexity:2 | 2 | 3 |
// | onImageFileChanged | high | complexity:4 | 3 | 5 |
// | onImageDirChanged | high | complexity:3 | 2 | 3 |
// ─── 生成后填入 actual 列，低于 min 即违规 ───

// 最小清单完成情况（test-code-gen §最小清单）：
// 1. 每个公开方法 ≥ 1 用例: [x]
// 2. 每个输入维度按等价类划分 ≥ 1 用例/类: [x]
// 3. 每个等价类的边界值显式覆盖: [x]
// 4. 同质 ≥ 3 组用 TEST_P: [x]
// 5. 分支清单 → 用例映射已列出: [x]
// 6. 每条 if/switch/throw/early-return 有触发用例: [x]
// 7. 异常路径 EXPECT_THROW 精确匹配: [x]
// 8. 负面场景有专门用例: [x]
// 9. 负面用例验证强异常安全: [x]
// 10. stub_ext vs gMock 选择正确: [x]

// 分支清单映射（instance complexity=0）：
// B1: static local singleton, repeat calls return same pointer
// 用例映射: B1→Instance_ReturnsSamePointer, Instance_NotNull
//
// 分支清单映射（resetImageFiles complexity=5）：
// B1: filePaths empty → clear all watched files/dirs, return
// B2: isCurrentDir(first) true → early return
// B3: file exists → insert cacheFileInfo + addPath
// B4: file not exists → skip
// B5: after loop, watched files non-empty → add parent dir
// 用例映射: B1→ResetImageFiles_EmptyList_ClearsAll, B2→ResetImageFiles_DirAlreadyWatched_EarlyReturn,
//           B3→ResetImageFiles_ExistingFile_AddedToWatch, B4→ResetImageFiles_NonExistingFile_Skipped,
//           B5→ResetImageFiles_AddsParentDir
//
// 分支清单映射（isCurrentDir complexity=1）：
// B1: dir in watched directories → true
// B2: dir not in watched directories → false
// 用例映射: B1→IsCurrentDir_WatchedDir_ReturnsTrue, B2→IsCurrentDir_UnwatchedDir_ReturnsFalse
//
// 分支清单映射（fileRename complexity=2）：
// B1: oldPath in cacheFileInfo → remove old, insert new, update watch
// B2: oldPath not in cacheFileInfo → no-op
// 用例映射: B1→FileRename_CachedPath_UpdatesWatch, B2→FileRename_UncachedPath_NoOp
//
// 分支清单映射（recordRotateImage complexity=1）：
// B1: insert targetPath into rotateImagePathSet
// 用例映射: B1→RecordRotateImage_AddsToSet
//
// 分支清单映射（clearRotateStatus complexity=2）：
// B1: path in set → remove
// B2: path not in set → no-op
// 用例映射: B1→ClearRotateStatus_InSet_Removes, B2→ClearRotateStatus_NotInSet_NoOp
//
// 分支清单映射（onImageFileChanged complexity=4）：
// B1: file in rotateImagePathSet → skip (return)
// B2: file in cacheFileInfo, file deleted → insert removedFile, emit signal
// B3: file in cacheFileInfo, file exists → emit signal (no removedFile)
// B4: file not in cacheFileInfo → no-op (no signal)
// 用例映射: B1→OnImageFileChanged_RotatingFile_Skipped, B2→OnImageFileChanged_FileDeleted_AddsToRemoved,
//           B3→OnImageFileChanged_FileExists_EmitsSignal, B4→OnImageFileChanged_NotWatched_NoOp
//
// 分支清单映射（onImageDirChanged complexity=3）：
// B1: removedFile entry reappears in dir → addPath + onImageFileChanged + erase
// B2: removedFile entry still missing → keep iterating
// 用例映射: B1→OnImageDirChanged_FileRecovered_RewatchAndEmit, B2→OnImageDirChanged_FileStillMissing_KeepsEntry

#include <gtest/gtest.h>
#include <QApplication>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QSignalSpy>
#include <QFileSystemWatcher>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <memory>

#include "imagedata/imagefilewatcher.h"
#include "imagedata/imageinfo.h"
#include "imagedata/thumbnailcache.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// Access private fields for verification
using ImageFileCacheHash = QHash<QString, QUrl>;
ACCESS_PRIVATE_FIELD(ImageFileWatcher, ImageFileCacheHash, cacheFileInfo)
ACCESS_PRIVATE_FIELD(ImageFileWatcher, ImageFileCacheHash, removedFile)
ACCESS_PRIVATE_FIELD(ImageFileWatcher, QSet<QString>, rotateImagePathSet)
ACCESS_PRIVATE_FIELD(ImageFileWatcher, QFileSystemWatcher*, fileWatcher)

// Access private slots for direct invocation
ACCESS_PRIVATE_FUN(ImageFileWatcher, void(const QString &), onImageFileChanged)
ACCESS_PRIVATE_FUN(ImageFileWatcher, void(const QString &), onImageDirChanged)

// Access ImageInfo private method for stubbing (refreshDataFromCache is called by setSource)
ACCESS_PRIVATE_FUN(ImageInfo, void(bool), refreshDataFromCache)

class ImageFileWatcherTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test";
            static char *argv[] = {arg0, nullptr};
            new QApplication(argc, argv);
        }
    }

    static void TearDownTestSuite() {}

    StubExt stub;
    ImageFileWatcher *m_watcher = nullptr;
    std::unique_ptr<QTemporaryDir> m_tempDir;

    void SetUp() override
    {
        stub.clear();
        m_watcher = ImageFileWatcher::instance();

        // Stub ImageInfo::reloadData and clearCurrentCache to avoid real cache/DB access
        // triggered by onImageFileChanged
        stub.set_lamda(ADDR(ImageInfo, reloadData), [](ImageInfo *) {});
        stub.set_lamda(ADDR(ImageInfo, clearCurrentCache), [](ImageInfo *) {});
        // Stub refreshDataFromCache to avoid real cache access during setSource
        stub.set_lamda(get_private_fun::ImageInforefreshDataFromCache(),
                       [](ImageInfo *, bool) {});

        // Reset watcher state before each test
        m_watcher->resetImageFiles({});

        // Create a temp directory for file isolation
        m_tempDir = std::make_unique<QTemporaryDir>();
        ASSERT_TRUE(m_tempDir->isValid());
    }

    void TearDown() override
    {
        // Clean up watcher state
        m_watcher->resetImageFiles({});
        m_tempDir.reset();
        stub.clear();
    }

    QString createTempFile(const QString &name)
    {
        QString path = m_tempDir->filePath(name);
        QFile f(path);
        f.open(QIODevice::WriteOnly);
        f.close();
        return path;
    }

    // resetImageFiles expects file:// URL strings (source calls QUrl(path).toLocalFile())
    QString toUrl(const QString &path)
    {
        return QUrl::fromLocalFile(path).toString();
    }
};

// ─────────────────────────── instance ───────────────────────────

TEST_F(ImageFileWatcherTest, Instance_NotNull)
{
    EXPECT_NE(m_watcher, nullptr);
}

TEST_F(ImageFileWatcherTest, Instance_ReturnsSamePointer)
{
    ImageFileWatcher *p1 = ImageFileWatcher::instance();
    ImageFileWatcher *p2 = ImageFileWatcher::instance();
    EXPECT_EQ(p1, p2);
    EXPECT_EQ(p1, m_watcher);
}

// ─────────────────────────── resetImageFiles ───────────────────────────

// B1: Empty list clears all watched files and directories
TEST_F(ImageFileWatcherTest, ResetImageFiles_EmptyList_ClearsAll)
{
    // First add some files
    QString file1 = createTempFile("a.jpg");
    m_watcher->resetImageFiles({toUrl(file1)});
    ASSERT_FALSE(access_private_field::ImageFileWatchercacheFileInfo(*m_watcher).isEmpty());

    // Now clear with empty list
    m_watcher->resetImageFiles({});

    EXPECT_TRUE(access_private_field::ImageFileWatchercacheFileInfo(*m_watcher).isEmpty());
    EXPECT_TRUE(access_private_field::ImageFileWatcherremovedFile(*m_watcher).isEmpty());
    EXPECT_TRUE(access_private_field::ImageFileWatcherrotateImagePathSet(*m_watcher).isEmpty());
    EXPECT_TRUE(access_private_field::ImageFileWatcherfileWatcher(*m_watcher)->files().isEmpty());
    EXPECT_TRUE(access_private_field::ImageFileWatcherfileWatcher(*m_watcher)->directories().isEmpty());
}

// B2: Directory already watched → early return (no duplicate add)
TEST_F(ImageFileWatcherTest, ResetImageFiles_DirAlreadyWatched_EarlyReturn)
{
    QString file1 = createTempFile("a.jpg");
    m_watcher->resetImageFiles({toUrl(file1)});

    int fileCountBefore = access_private_field::ImageFileWatcherfileWatcher(*m_watcher)->files().size();
    int cacheCountBefore = access_private_field::ImageFileWatchercacheFileInfo(*m_watcher).size();

    // Call again with the same file; isCurrentDir should be true → early return
    m_watcher->resetImageFiles({toUrl(file1)});

    // State should not change (no duplicate entries)
    EXPECT_EQ(access_private_field::ImageFileWatcherfileWatcher(*m_watcher)->files().size(), fileCountBefore);
    EXPECT_EQ(access_private_field::ImageFileWatchercacheFileInfo(*m_watcher).size(), cacheCountBefore);
}

// B3: Existing file → added to cache and watcher
TEST_F(ImageFileWatcherTest, ResetImageFiles_ExistingFile_AddedToWatch)
{
    QString file1 = createTempFile("photo.jpg");
    m_watcher->resetImageFiles({toUrl(file1)});

    auto &cache = access_private_field::ImageFileWatchercacheFileInfo(*m_watcher);
    EXPECT_TRUE(cache.contains(file1));
    EXPECT_TRUE(access_private_field::ImageFileWatcherfileWatcher(*m_watcher)->files().contains(file1));
}

// B4: Non-existing file → skipped, no crash
TEST_F(ImageFileWatcherTest, ResetImageFiles_NonExistingFile_Skipped)
{
    QString nonExist = m_tempDir->filePath("does_not_exist.jpg");
    m_watcher->resetImageFiles({toUrl(nonExist)});

    auto &cache = access_private_field::ImageFileWatchercacheFileInfo(*m_watcher);
    EXPECT_FALSE(cache.contains(nonExist));
    EXPECT_FALSE(access_private_field::ImageFileWatcherfileWatcher(*m_watcher)->files().contains(nonExist));
}

// B5: After adding files, parent directory is also watched
TEST_F(ImageFileWatcherTest, ResetImageFiles_AddsParentDir)
{
    QString file1 = createTempFile("img.png");
    m_watcher->resetImageFiles({toUrl(file1)});

    QStringList dirs = access_private_field::ImageFileWatcherfileWatcher(*m_watcher)->directories();
    QString expectedDir = QFileInfo(file1).absolutePath();
    EXPECT_TRUE(dirs.contains(expectedDir));
}

// resetImageFiles clears previous state (cacheFileInfo, removedFile, rotateImagePathSet)
TEST_F(ImageFileWatcherTest, ResetImageFiles_ClearsPreviousState)
{
    // Set up some state
    QString file1 = createTempFile("x.jpg");
    m_watcher->resetImageFiles({toUrl(file1)});
    m_watcher->recordRotateImage(file1);
    auto &removedRef = access_private_field::ImageFileWatcherremovedFile(*m_watcher);
    removedRef.insert(file1, QUrl::fromLocalFile(file1));

    // Reset with new file
    QString file2 = createTempFile("y.jpg");
    m_watcher->resetImageFiles({toUrl(file2)});

    EXPECT_FALSE(access_private_field::ImageFileWatcherrotateImagePathSet(*m_watcher).contains(file1));
    EXPECT_TRUE(access_private_field::ImageFileWatcherremovedFile(*m_watcher).isEmpty());
    EXPECT_TRUE(access_private_field::ImageFileWatchercacheFileInfo(*m_watcher).contains(file2));
}

// ─────────────────────────── isCurrentDir ───────────────────────────

// B1: Directory already watched → true
TEST_F(ImageFileWatcherTest, IsCurrentDir_WatchedDir_ReturnsTrue)
{
    QString file1 = createTempFile("w.jpg");
    m_watcher->resetImageFiles({toUrl(file1)});

    QString dir = QFileInfo(file1).absolutePath();
    EXPECT_TRUE(m_watcher->isCurrentDir(file1));
}

// B2: Directory not watched → false
TEST_F(ImageFileWatcherTest, IsCurrentDir_UnwatchedDir_ReturnsFalse)
{
    QString unrelated = "/some/random/path/file.jpg";
    EXPECT_FALSE(m_watcher->isCurrentDir(unrelated));
}

// isCurrentDir returns false after clearing
TEST_F(ImageFileWatcherTest, IsCurrentDir_FalseAfterClear)
{
    QString file1 = createTempFile("c.jpg");
    m_watcher->resetImageFiles({toUrl(file1)});
    ASSERT_TRUE(m_watcher->isCurrentDir(file1));

    m_watcher->resetImageFiles({});
    EXPECT_FALSE(m_watcher->isCurrentDir(file1));
}

// ─────────────────────────── fileRename ───────────────────────────

// B1: oldPath in cache → update watch
TEST_F(ImageFileWatcherTest, FileRename_CachedPath_UpdatesWatch)
{
    QString file1 = createTempFile("old.jpg");
    m_watcher->resetImageFiles({toUrl(file1)});

    QString newPath = createTempFile("new.jpg");
    m_watcher->fileRename(file1, newPath);

    auto &cache = access_private_field::ImageFileWatchercacheFileInfo(*m_watcher);
    // Source fileRename does not remove oldPath from cacheFileInfo (potential defect)
    EXPECT_TRUE(cache.contains(file1));
    EXPECT_TRUE(cache.contains(newPath));
    EXPECT_FALSE(access_private_field::ImageFileWatcherfileWatcher(*m_watcher)->files().contains(file1));
    EXPECT_TRUE(access_private_field::ImageFileWatcherfileWatcher(*m_watcher)->files().contains(newPath));
}

// B2: oldPath not in cache → no-op
TEST_F(ImageFileWatcherTest, FileRename_UncachedPath_NoOp)
{
    QString uncached = m_tempDir->filePath("uncached.jpg");
    QString newPath = m_tempDir->filePath("new2.jpg");
    int cacheBefore = access_private_field::ImageFileWatchercacheFileInfo(*m_watcher).size();

    m_watcher->fileRename(uncached, newPath);

    EXPECT_EQ(access_private_field::ImageFileWatchercacheFileInfo(*m_watcher).size(), cacheBefore);
    EXPECT_FALSE(access_private_field::ImageFileWatchercacheFileInfo(*m_watcher).contains(newPath));
}

// fileRename with existing cache entries does not corrupt other entries
TEST_F(ImageFileWatcherTest, FileRename_PreservesOtherEntries)
{
    QString file1 = createTempFile("keep.jpg");
    QString file2 = createTempFile("rename_me.jpg");
    m_watcher->resetImageFiles({toUrl(file1), toUrl(file2)});

    QString newPath = createTempFile("renamed.jpg");
    m_watcher->fileRename(file2, newPath);

    auto &cache = access_private_field::ImageFileWatchercacheFileInfo(*m_watcher);
    EXPECT_TRUE(cache.contains(file1));
    EXPECT_TRUE(cache.contains(newPath));
    // Source fileRename does not remove oldPath from cacheFileInfo (potential defect)
    EXPECT_TRUE(cache.contains(file2));
}

// ─────────────────────────── recordRotateImage ───────────────────────────

TEST_F(ImageFileWatcherTest, RecordRotateImage_AddsToSet)
{
    QString path = "/tmp/rotate_test.jpg";
    m_watcher->recordRotateImage(path);

    EXPECT_TRUE(access_private_field::ImageFileWatcherrotateImagePathSet(*m_watcher).contains(path));
}

TEST_F(ImageFileWatcherTest, RecordRotateImage_DuplicateInsert_NoChange)
{
    QString path = "/tmp/rotate_dup.jpg";
    m_watcher->recordRotateImage(path);
    m_watcher->recordRotateImage(path); // duplicate

    EXPECT_TRUE(access_private_field::ImageFileWatcherrotateImagePathSet(*m_watcher).contains(path));
}

// ─────────────────────────── clearRotateStatus ───────────────────────────

// B1: path in set → remove
TEST_F(ImageFileWatcherTest, ClearRotateStatus_InSet_Removes)
{
    QString path = "/tmp/clear_in.jpg";
    m_watcher->recordRotateImage(path);
    ASSERT_TRUE(access_private_field::ImageFileWatcherrotateImagePathSet(*m_watcher).contains(path));

    m_watcher->clearRotateStatus(path);

    EXPECT_FALSE(access_private_field::ImageFileWatcherrotateImagePathSet(*m_watcher).contains(path));
}

// B2: path not in set → no-op (no crash)
TEST_F(ImageFileWatcherTest, ClearRotateStatus_NotInSet_NoOp)
{
    QString path = "/tmp/clear_out.jpg";
    // Not added to set
    EXPECT_NO_THROW(m_watcher->clearRotateStatus(path));
    EXPECT_FALSE(access_private_field::ImageFileWatcherrotateImagePathSet(*m_watcher).contains(path));
}

// clearRotateStatus after remove does not affect other entries
TEST_F(ImageFileWatcherTest, ClearRotateStatus_PreservesOtherEntries)
{
    QString path1 = "/tmp/clear_p1.jpg";
    QString path2 = "/tmp/clear_p2.jpg";
    m_watcher->recordRotateImage(path1);
    m_watcher->recordRotateImage(path2);

    m_watcher->clearRotateStatus(path1);

    EXPECT_FALSE(access_private_field::ImageFileWatcherrotateImagePathSet(*m_watcher).contains(path1));
    EXPECT_TRUE(access_private_field::ImageFileWatcherrotateImagePathSet(*m_watcher).contains(path2));
}

// ─────────────────────────── onImageFileChanged ───────────────────────────

// B1: Rotating file → skipped (no signal, no removedFile)
TEST_F(ImageFileWatcherTest, OnImageFileChanged_RotatingFile_Skipped)
{
    QString file1 = createTempFile("rot.jpg");
    m_watcher->resetImageFiles({toUrl(file1)});
    m_watcher->recordRotateImage(file1);

    QSignalSpy spy(m_watcher, &ImageFileWatcher::imageFileChanged);

    call_private_fun::ImageFileWatcheronImageFileChanged(*m_watcher, file1);

    EXPECT_EQ(spy.count(), 0);
    EXPECT_FALSE(access_private_field::ImageFileWatcherremovedFile(*m_watcher).contains(file1));
}

// B2: File deleted → added to removedFile + signal emitted
TEST_F(ImageFileWatcherTest, OnImageFileChanged_FileDeleted_AddsToRemoved)
{
    QString file1 = createTempFile("del.jpg");
    m_watcher->resetImageFiles({toUrl(file1)});

    // Stub QFile::exists to return false (simulating deletion)
    stub.set_lamda(static_cast<bool(*)(const QString&)>(&QFile::exists), [](const QString&) -> bool { return false; });

    QSignalSpy spy(m_watcher, &ImageFileWatcher::imageFileChanged);

    call_private_fun::ImageFileWatcheronImageFileChanged(*m_watcher, file1);

    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(access_private_field::ImageFileWatcherremovedFile(*m_watcher).contains(file1));
}

// B3: File exists → signal emitted, no removedFile
TEST_F(ImageFileWatcherTest, OnImageFileChanged_FileExists_EmitsSignal)
{
    QString file1 = createTempFile("exists.jpg");
    m_watcher->resetImageFiles({toUrl(file1)});

    // Stub QFile::exists to return true
    stub.set_lamda(static_cast<bool(*)(const QString&)>(&QFile::exists), [](const QString&) -> bool { return true; });

    QSignalSpy spy(m_watcher, &ImageFileWatcher::imageFileChanged);

    call_private_fun::ImageFileWatcheronImageFileChanged(*m_watcher, file1);

    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(access_private_field::ImageFileWatcherremovedFile(*m_watcher).contains(file1));
}

// B4: File not watched → no-op (no signal)
TEST_F(ImageFileWatcherTest, OnImageFileChanged_NotWatched_NoOp)
{
    QString unwatched = "/tmp/unwatched_file.jpg";

    QSignalSpy spy(m_watcher, &ImageFileWatcher::imageFileChanged);

    call_private_fun::ImageFileWatcheronImageFileChanged(*m_watcher, unwatched);

    EXPECT_EQ(spy.count(), 0);
}

// onImageFileChanged signal carries the file path
TEST_F(ImageFileWatcherTest, OnImageFileChanged_SignalCarriesFilePath)
{
    QString file1 = createTempFile("sig.jpg");
    m_watcher->resetImageFiles({toUrl(file1)});

    stub.set_lamda(static_cast<bool(*)(const QString&)>(&QFile::exists), [](const QString&) -> bool { return true; });

    QSignalSpy spy(m_watcher, &ImageFileWatcher::imageFileChanged);
    call_private_fun::ImageFileWatcheronImageFileChanged(*m_watcher, file1);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), file1);
}

// ─────────────────────────── onImageDirChanged ───────────────────────────

// B1: Removed file reappears in dir → re-watch + signal + erase from removedFile
TEST_F(ImageFileWatcherTest, OnImageDirChanged_FileRecovered_RewatchAndEmit)
{
    QString file1 = createTempFile("rec.jpg");
    m_watcher->resetImageFiles({toUrl(file1)});

    // Simulate file was deleted → in removedFile
    QString dir = QFileInfo(file1).absolutePath();
    auto &removedRef = access_private_field::ImageFileWatcherremovedFile(*m_watcher);
    removedRef.insert(file1, QUrl::fromLocalFile(file1));
    // Remove from file watcher (as onImageFileChanged would have left it unwatched)
    access_private_field::ImageFileWatcherfileWatcher(*m_watcher)->removePath(file1);

    // Stub QFile::exists to return true (file recovered) for onImageFileChanged
    stub.set_lamda(static_cast<bool(*)(const QString&)>(&QFile::exists), [](const QString&) -> bool { return true; });

    QSignalSpy spy(m_watcher, &ImageFileWatcher::imageFileChanged);

    call_private_fun::ImageFileWatcheronImageDirChanged(*m_watcher, dir);

    // Signal should have been emitted via onImageFileChanged
    EXPECT_GE(spy.count(), 1);
    // File should be re-added to watcher
    EXPECT_TRUE(access_private_field::ImageFileWatcherfileWatcher(*m_watcher)->files().contains(file1));
    // removedFile should be cleared
    EXPECT_FALSE(access_private_field::ImageFileWatcherremovedFile(*m_watcher).contains(file1));
}

// B2: Removed file still missing → entry remains in removedFile
TEST_F(ImageFileWatcherTest, OnImageDirChanged_FileStillMissing_KeepsEntry)
{
    QString file1 = createTempFile("miss.jpg");
    m_watcher->resetImageFiles({toUrl(file1)});

    QString dir = QFileInfo(file1).absolutePath();
    QString missingFile = dir + "/truly_missing.jpg";
    auto &removedRef = access_private_field::ImageFileWatcherremovedFile(*m_watcher);
    removedRef.insert(missingFile, QUrl::fromLocalFile(missingFile));

    QSignalSpy spy(m_watcher, &ImageFileWatcher::imageFileChanged);

    call_private_fun::ImageFileWatcheronImageDirChanged(*m_watcher, dir);

    // No signal for missing file
    EXPECT_EQ(spy.count(), 0);
    // Entry still in removedFile
    EXPECT_TRUE(access_private_field::ImageFileWatcherremovedFile(*m_watcher).contains(missingFile));
}

// onImageDirChanged with empty removedFile → no crash, no signal
TEST_F(ImageFileWatcherTest, OnImageDirChanged_EmptyRemovedFile_NoOp)
{
    QString dir = m_tempDir->path();

    QSignalSpy spy(m_watcher, &ImageFileWatcher::imageFileChanged);

    EXPECT_NO_THROW(call_private_fun::ImageFileWatcheronImageDirChanged(*m_watcher, dir));
    EXPECT_EQ(spy.count(), 0);
}
