// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | FileInotifyGroup::FileInotifyGroup | high | 2 | 2 | 2 |
 * | FileInotifyGroup::~FileInotifyGroup | high | 2 | 2 | 2 |
 * | FileInotifyGroup::startWatch | high | 4 | 4 | 4 |
 *
 * 分支列表 (FileInotifyGroup):
 *   B1: 构造函数 — watchers 初始为空
 *   B2: 析构函数 — watchers 为空时不崩溃
 *   B3: 析构函数 — watchers 非空时对每个 watcher 调用 deleteLater
 *
 * 分支列表 (startWatch):
 *   B1: 单次 startWatch — watchers 列表增长为 1
 *   B2: 多次 startWatch — watchers 列表持续增长
 *   B3: startWatch 后 sigMonitorChanged 信号转发
 *   B4: startWatch 后 sigMonitorDestroyed 信号转发
 *   B5: startWatch 后 sigVideoFileStable 信号转发
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QSignalSpy>
#include <QTest>
#include <QMetaObject>
#include <QList>
#include <QStringList>
#include <memory>

#include "stubext.h"
#include "addr_pri.h"
#include "fileMonitor/fileinotifygroup.h"
#include "fileMonitor/fileinotify.h"

#include "dbmanager/dbmanager.h"
#include "unionimage/unionimage.h"

using namespace stub_ext;

// Access private members of FileInotifyGroup without "#define private public"
using FileInotifyPtrList = QList<FileInotify *>;
ACCESS_PRIVATE_FIELD(FileInotifyGroup, FileInotifyPtrList, watchers)

class FileInotifyGroupTest : public ::testing::Test
{
protected:
    StubExt stub;

    void SetUp() override
    {
        // Stub DBManager::instance() to return nullptr (sentinel)
        stub.set_lamda(ADDR(DBManager, instance),
            []() -> DBManager * { return nullptr; });

        // Stub FileInotify::addWather to prevent filesystem interaction
        // and timer start during startWatch
        stub.set_lamda(ADDR(FileInotify, addWather),
            [](FileInotify *, const QStringList &, const QString &, int) -> void {});
    }

    void TearDown() override
    {
        stub.clear();
    }

    // Helper: get reference to watchers list
    QList<FileInotify *> &getWatchers(FileInotifyGroup &group)
    {
        return access_private_field::FileInotifyGroupwatchers(group);
    }
};

// ============ Constructor ============

// B1: Constructor — watchers should be empty
TEST_F(FileInotifyGroupTest, Constructor_WatchersEmpty)
{
    FileInotifyGroup group;
    EXPECT_TRUE(getWatchers(group).isEmpty());
    EXPECT_EQ(getWatchers(group).size(), 0);
}

// B2: Constructor — does not crash, object is valid
TEST_F(FileInotifyGroupTest, Constructor_DoesNotCrash)
{
    auto group = std::make_unique<FileInotifyGroup>();
    ASSERT_NE(group, nullptr);
    EXPECT_TRUE(getWatchers(*group).isEmpty());
}

// ============ Destructor ============

// B3: Destructor — empty watchers, no crash
TEST_F(FileInotifyGroupTest, Destructor_EmptyWatchers_NoCrash)
{
    {
        FileInotifyGroup group;
        EXPECT_TRUE(getWatchers(group).isEmpty());
    } // destructor runs here
    SUCCEED(); // if we reach here, destructor didn't crash
}

// B4: Destructor — with watchers, calls deleteLater on each
TEST_F(FileInotifyGroupTest, Destructor_WithWatchers_CleansUp)
{
    {
        FileInotifyGroup group;
        group.startWatch(QStringList() << "/nonexistent/path", "testAlbum", 100);
        EXPECT_EQ(getWatchers(group).size(), 1);

        group.startWatch(QStringList() << "/another/nonexistent", "album2", 200);
        EXPECT_EQ(getWatchers(group).size(), 2);
    } // destructor runs, should call deleteLater on each watcher
    SUCCEED();
}

// ============ startWatch ============

// B1: Single startWatch — watchers list grows to 1
TEST_F(FileInotifyGroupTest, StartWatch_SingleCall_AddsOneWatcher)
{
    FileInotifyGroup group;
    QStringList paths = {"/test/path1"};
    group.startWatch(paths, "album1", 42);

    EXPECT_EQ(getWatchers(group).size(), 1);
    ASSERT_NE(getWatchers(group).at(0), nullptr);
}

// B2: Multiple startWatch — watchers list grows accordingly
TEST_F(FileInotifyGroupTest, StartWatch_MultipleCalls_WatchersGrow)
{
    FileInotifyGroup group;

    group.startWatch(QStringList() << "/path1", "album1", 1);
    EXPECT_EQ(getWatchers(group).size(), 1);

    group.startWatch(QStringList() << "/path2", "album2", 2);
    EXPECT_EQ(getWatchers(group).size(), 2);

    group.startWatch(QStringList() << "/path3", "album3", 3);
    EXPECT_EQ(getWatchers(group).size(), 3);
}

// B3: startWatch — addWather called with correct arguments
TEST_F(FileInotifyGroupTest, StartWatch_AddWatherCalledWithCorrectArgs)
{
    bool addWatherCalled = false;
    QStringList capturedPaths;
    QString capturedAlbum;
    int capturedUID = 0;

    stub.set_lamda(ADDR(FileInotify, addWather),
        [&](FileInotify *, const QStringList &paths, const QString &album, int UID) -> void {
            addWatherCalled = true;
            capturedPaths = paths;
            capturedAlbum = album;
            capturedUID = UID;
        });

    FileInotifyGroup group;
    QStringList paths = {"/test/album/path"};
    group.startWatch(paths, "myAlbum", 999);

    EXPECT_TRUE(addWatherCalled);
    EXPECT_EQ(capturedPaths, paths);
    EXPECT_EQ(capturedAlbum, QString("myAlbum"));
    EXPECT_EQ(capturedUID, 999);
}

// ============ Signal Forwarding ============
// Uses QMetaObject::invokeMethod with DirectConnection to emit
// FileInotify signals and verify FileInotifyGroup forwards them.

// B4: sigMonitorChanged forwarded from FileInotify to FileInotifyGroup
TEST_F(FileInotifyGroupTest, SignalForwarding_SigMonitorChanged)
{
    FileInotifyGroup group;
    group.startWatch(QStringList() << "/test", "album", 1);

    QSignalSpy spy(&group, &FileInotifyGroup::sigMonitorChanged);
    ASSERT_TRUE(spy.isValid());

    FileInotify *watcher = getWatchers(group).at(0);
    ASSERT_NE(watcher, nullptr);

    // Emit the signal on the watcher via meta-object system (DirectConnection)
    QStringList addFiles = {"file1.jpg", "file2.png"};
    QStringList delFiles = {"old.jpg"};
    bool ok = QMetaObject::invokeMethod(watcher, "sigMonitorChanged",
        Qt::DirectConnection,
        Q_ARG(QStringList, addFiles),
        Q_ARG(QStringList, delFiles),
        Q_ARG(QString, QString("album")),
        Q_ARG(int, 1));
    ASSERT_TRUE(ok) << "invokeMethod failed for sigMonitorChanged";

    EXPECT_EQ(spy.count(), 1);
    if (spy.count() == 1) {
        QList<QVariant> args = spy.takeFirst();
        EXPECT_EQ(args.at(0).toStringList(), addFiles);
        EXPECT_EQ(args.at(1).toStringList(), delFiles);
        EXPECT_EQ(args.at(2).toString(), QString("album"));
        EXPECT_EQ(args.at(3).toInt(), 1);
    }
}

// B5: sigMonitorDestroyed (pathDestroyed) forwarded
TEST_F(FileInotifyGroupTest, SignalForwarding_SigMonitorDestroyed)
{
    FileInotifyGroup group;
    group.startWatch(QStringList() << "/test", "album", 55);

    QSignalSpy spy(&group, &FileInotifyGroup::sigMonitorDestroyed);
    ASSERT_TRUE(spy.isValid());

    FileInotify *watcher = getWatchers(group).at(0);
    ASSERT_NE(watcher, nullptr);

    bool ok = QMetaObject::invokeMethod(watcher, "pathDestroyed",
        Qt::DirectConnection,
        Q_ARG(int, 55));
    ASSERT_TRUE(ok) << "invokeMethod failed for pathDestroyed";

    EXPECT_EQ(spy.count(), 1);
    if (spy.count() == 1) {
        EXPECT_EQ(spy.takeFirst().at(0).toInt(), 55);
    }
}

// B6: sigVideoFileStable forwarded
TEST_F(FileInotifyGroupTest, SignalForwarding_SigVideoFileStable)
{
    FileInotifyGroup group;
    group.startWatch(QStringList() << "/test", "album", 7);

    QSignalSpy spy(&group, &FileInotifyGroup::sigVideoFileStable);
    ASSERT_TRUE(spy.isValid());

    FileInotify *watcher = getWatchers(group).at(0);
    ASSERT_NE(watcher, nullptr);

    QStringList stableFiles = {"video1.mp4", "video2.avi"};
    bool ok = QMetaObject::invokeMethod(watcher, "sigVideoFileStable",
        Qt::DirectConnection,
        Q_ARG(QStringList, stableFiles));
    ASSERT_TRUE(ok) << "invokeMethod failed for sigVideoFileStable";

    EXPECT_EQ(spy.count(), 1);
    if (spy.count() == 1) {
        EXPECT_EQ(spy.takeFirst().at(0).toStringList(), stableFiles);
    }
}

// ============ Signal Forwarding with Multiple Watchers ============

// B7: Only the corresponding watcher's signal is forwarded
TEST_F(FileInotifyGroupTest, SignalForwarding_MultipleWatchers_Independent)
{
    FileInotifyGroup group;
    group.startWatch(QStringList() << "/path1", "album1", 1);
    group.startWatch(QStringList() << "/path2", "album2", 2);

    QSignalSpy spy(&group, &FileInotifyGroup::sigMonitorChanged);
    ASSERT_TRUE(spy.isValid());

    FileInotify *watcher2 = getWatchers(group).at(1);
    ASSERT_NE(watcher2, nullptr);

    // Emit only from watcher2
    bool ok = QMetaObject::invokeMethod(watcher2, "sigMonitorChanged",
        Qt::DirectConnection,
        Q_ARG(QStringList, QStringList() << "new.jpg"),
        Q_ARG(QStringList, QStringList()),
        Q_ARG(QString, QString("album2")),
        Q_ARG(int, 2));
    ASSERT_TRUE(ok) << "invokeMethod failed for sigMonitorChanged";

    // Should receive exactly 1 signal (from watcher2 only)
    EXPECT_EQ(spy.count(), 1);
    if (spy.count() == 1) {
        QList<QVariant> args = spy.takeFirst();
        EXPECT_EQ(args.at(2).toString(), QString("album2"));
        EXPECT_EQ(args.at(3).toInt(), 2);
    }
}
