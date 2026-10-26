// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 用例计数声明（self-check-structural 验证此块）：
// | method | level | factors | min | actual |
// |--------|-------|---------|-----|--------|
// | ImageEngineThreadObject::ctor | low | 1 | 1 | 1 |
// | ImageEngineThreadObject::needStop | mid | 2 | 2 | 2 |
// | ImageEngineThreadObject::ifCanStopThread | low | 1 | 1 | 1 |
// | ImageEngineThreadObject::run | mid | 1 | 1 | 1 |
// | ImportImagesThread::setData(QStringList) | mid | 2 | 1 | 1 |
// | ImportImagesThread::setData(QList<QUrl>) | mid | 2 | 1 | 1 |
// | ImportImagesThread::setNotifyUI | mid | 2 | 2 | 2 |
// | ImportImagesThread::ifCanStopThread | low | 1 | 1 | 1 |
// | ImportImagesThread::runDetail(edge) | high | 2 | 2 | 2 |
// | ImagesClassifyThread::setData | low | 1 | 1 | 1 |
// | ImagesClassifyThread::runDetail | high | 4 | 3 | 4 |
// ─── 生成后填入 actual 列，低于 min 即违规 ───

#include <gtest/gtest.h>
#include <QApplication>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QImage>
#include <QFile>
#include <QFileInfo>
#include <memory>

#include "imageengine/imageenginethread.h"
#include "dbmanager/dbmanager.h"
#include "albumControl.h"
#include "utils/classifyutils.h"
#include "unionimage/baseutils.h"
#include "unionimage/unionimage.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// ---------------------------------------------------------------------------
// Private field / function access declarations
// ---------------------------------------------------------------------------

ACCESS_PRIVATE_FIELD(ImportImagesThread, QStringList, m_paths)
ACCESS_PRIVATE_FIELD(ImportImagesThread, int, m_UID)
ACCESS_PRIVATE_FIELD(ImportImagesThread, bool, m_notifyUI)
ACCESS_PRIVATE_FIELD(ImportImagesThread, bool, m_checkRepeat)

// m_type's type is a private nested enum (ImportImagesThread::DataType), which
// ACCESS_PRIVATE_FIELD cannot name; read it via a friend injected through an
// explicit instantiation, exposed with an all-public signature. The underlying
// value is compared (NULL=0, String=1, Url=2).
namespace {
namespace private_access_detail {
struct TagImportImagesThreadm_type {};
int readImportImagesThreadm_type(TagImportImagesThreadm_type, const void *obj);

template <typename Ptr, Ptr PtrValue>
struct MTypeReader {
    friend int readImportImagesThreadm_type(TagImportImagesThreadm_type, const void *obj)
    {
        return static_cast<int>(static_cast<const ImportImagesThread *>(obj)->*PtrValue);
    }
};
template struct MTypeReader<decltype(&ImportImagesThread::m_type),
                            &ImportImagesThread::m_type>;
}
}
ACCESS_PRIVATE_FUN(ImportImagesThread, void(), runDetail)

ACCESS_PRIVATE_FIELD(ImagesClassifyThread, DBImgInfoList, m_infos)
ACCESS_PRIVATE_FUN(ImagesClassifyThread, void(), runDetail)

// ---------------------------------------------------------------------------
// Test subclasses to expose protected members
// ---------------------------------------------------------------------------

// Minimal concrete subclass for testing the abstract base class.
class TestableThreadObject : public ImageEngineThreadObject
{
public:
    using ImageEngineThreadObject::ifCanStopThread; // expose protected method
    using ImageEngineThreadObject::run; // expose protected run()
    bool runDetailCalled = false;
    void runDetail() override { runDetailCalled = true; }
    bool needStopFlag() const { return bneedstop; }
    bool backStopFlag() const { return bbackstop; }
};

// Subclass to expose protected ifCanStopThread on ImportImagesThread.
class TestableImportImagesThread : public ImportImagesThread
{
public:
    using ImportImagesThread::ifCanStopThread;
};

// ---------------------------------------------------------------------------
// Test fixture — shared QApplication
// ---------------------------------------------------------------------------

class ImageEngineThreadTest : public ::testing::Test
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

    void SetUp() override { stub.clear(); }
    void TearDown() override {}

    QString createTempImage(QTemporaryDir &dir, const QString &name, int w, int h)
    {
        QString filePath = dir.path() + "/" + name;
        QImage img(w, h, QImage::Format_RGB32);
        img.fill(Qt::red);
        EXPECT_TRUE(img.save(filePath, "PNG"));
        return filePath;
    }

    // Common DBManager stubs to avoid real database access.
    void stubDBManagerBasics()
    {
        stub.set_lamda(ADDR(DBManager, getPathsByAlbum),
                       [](DBManager *, const int) -> QStringList { return QStringList(); });
        stub.set_lamda(ADDR(DBManager, getExistingPaths),
                       [](DBManager *, const QStringList &) -> QSet<QString> { return QSet<QString>(); });
        stub.set_lamda(ADDR(DBManager, insertImgInfos),
                       [](DBManager *, const DBImgInfoList &) {});
    }
};

// ===========================================================================
// Part 1: ImageEngineThreadObject (base class)
// ===========================================================================

// ctor: setAutoDelete(false) → autoDelete() == false
TEST_F(ImageEngineThreadTest, Ctor_AutoDeleteIsFalse)
{
    TestableThreadObject obj;
    EXPECT_FALSE(obj.autoDelete());
}

// needStop(nullptr) → bneedstop / bbackstop set true
TEST_F(ImageEngineThreadTest, NeedStop_NullPtr_SetsFlags)
{
    TestableThreadObject obj;
    obj.needStop(nullptr);
    EXPECT_TRUE(obj.needStopFlag());
    EXPECT_TRUE(obj.backStopFlag());
}

// needStop(non-null) → via ifCanStopThread (base returns true) → flags set
TEST_F(ImageEngineThreadTest, NeedStop_NonNullPtr_SetsFlags)
{
    TestableThreadObject obj;
    int dummy = 0;
    obj.needStop(&dummy);
    EXPECT_TRUE(obj.needStopFlag());
    EXPECT_TRUE(obj.backStopFlag());
}

// ifCanStopThread (base) always returns true
TEST_F(ImageEngineThreadTest, IfCanStopThread_BaseReturnsTrue)
{
    TestableThreadObject obj;
    int dummy = 0;
    EXPECT_TRUE(obj.ifCanStopThread(&dummy));
    EXPECT_TRUE(obj.ifCanStopThread(nullptr));
}

// run(): calls runDetail → emits runFinished → schedules deleteLater
TEST_F(ImageEngineThreadTest, Run_CallsRunDetailAndEmitsFinished)
{
    auto *obj = new TestableThreadObject();
    QSignalSpy spy(obj, &ImageEngineThreadObject::runFinished);
    obj->run(); // synchronous: runDetail() + emit runFinished + deleteLater()
    EXPECT_EQ(spy.count(), 1);
    EXPECT_TRUE(obj->runDetailCalled);
    // deleteLater is deferred; process events to let it fire, then obj is gone.
    qApp->processEvents();
}

// ===========================================================================
// Part 2: ImportImagesThread — setData / setNotifyUI / ifCanStopThread
// ===========================================================================

// setData(QStringList, int): file:// paths converted to local, UID recorded
TEST_F(ImageEngineThreadTest, SetData_StringList_ConvertsAndStores)
{
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, localPath),
                   [](const QUrl &url) -> QString { return url.toLocalFile(); });

    ImportImagesThread thread;
    QStringList paths{"file:///tmp/a.png", "file:///home/u/img.jpg"};
    thread.setData(paths, 42);

    const auto &stored = access_private_field::ImportImagesThreadm_paths(thread);
    EXPECT_EQ(stored.size(), 2);
    EXPECT_EQ(stored[0], QString("/tmp/a.png"));
    EXPECT_EQ(stored[1], QString("/home/u/img.jpg"));
    EXPECT_EQ(access_private_field::ImportImagesThreadm_UID(thread), 42);
}

// setData(QList<QUrl>, int, bool): checkRepeat recorded, m_type = DataType_Url
TEST_F(ImageEngineThreadTest, SetData_UrlList_RecordsCheckRepeatAndType)
{
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, localPath),
                   [](const QUrl &url) -> QString { return url.toLocalFile(); });

    ImportImagesThread thread;
    QList<QUrl> urls{QUrl("file:///x/y.png"), QUrl("file:///z/w.jpg")};
    thread.setData(urls, 7, false);

    const auto &stored = access_private_field::ImportImagesThreadm_paths(thread);
    EXPECT_EQ(stored.size(), 2);
    EXPECT_EQ(access_private_field::ImportImagesThreadm_UID(thread), 7);
    EXPECT_FALSE(access_private_field::ImportImagesThreadm_checkRepeat(thread));
    // DataType_Url == 2
    EXPECT_EQ(private_access_detail::readImportImagesThreadm_type({}, &thread), 2);
}

// setNotifyUI(false) → runDetail with valid file does NOT emit sigImportFinished
TEST_F(ImageEngineThreadTest, SetNotifyUIFalse_NoImportFinishedSignal)
{
    stubDBManagerBasics();

    ImportImagesThread thread;
    thread.setNotifyUI(false);

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString filePath = createTempImage(dir, "img.png", 50, 50);
    access_private_field::ImportImagesThreadm_paths(thread) = QStringList{filePath};
    access_private_field::ImportImagesThreadm_UID(thread) = -1;

    QSignalSpy finishedSpy(&thread, &ImportImagesThread::sigImportFinished);
    call_private_fun::ImportImagesThreadrunDetail(thread);
    EXPECT_EQ(finishedSpy.count(), 0);
}

// setNotifyUI(true) (default) → runDetail with valid file emits sigImportFinished
TEST_F(ImageEngineThreadTest, SetNotifyUITrueDefault_EmitsImportFinished)
{
    stubDBManagerBasics();

    ImportImagesThread thread; // default m_notifyUI = true

    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString filePath = createTempImage(dir, "img.png", 50, 50);
    access_private_field::ImportImagesThreadm_paths(thread) = QStringList{filePath};
    access_private_field::ImportImagesThreadm_UID(thread) = -1;

    QSignalSpy finishedSpy(&thread, &ImportImagesThread::sigImportFinished);
    call_private_fun::ImportImagesThreadrunDetail(thread);
    EXPECT_EQ(finishedSpy.count(), 1);
}

// ifCanStopThread (override) always returns true
TEST_F(ImageEngineThreadTest, ImportImages_ifCanStopThread_AlwaysTrue)
{
    TestableImportImagesThread thread;
    int dummy = 0;
    EXPECT_TRUE(thread.ifCanStopThread(&dummy));
    EXPECT_TRUE(thread.ifCanStopThread(nullptr));
}

// runDetail edge: every path already imported into the current album →
// sigRepeatUrls with file:// paths, no failure/finish signals
// (imageenginethread.cpp:201-209 — distinct from the filePaths.isEmpty()
// branch already covered by RunDetail_CustomAlbumNoFiles_CallsRemoveCustomAutoImportPath)
TEST_F(ImageEngineThreadTest, RunDetail_AllInCurrentAlbum_EmitsRepeatUrls)
{
    const QString path = "/nonexistent/deepin/already_imported.png";

    ImportImagesThread thread;
    access_private_field::ImportImagesThreadm_paths(thread) = QStringList{path};
    access_private_field::ImportImagesThreadm_UID(thread) = 5;
    stub.set_lamda(ADDR(DBManager, getPathsByAlbum),
                   [&path](DBManager *, const int) -> QStringList { return QStringList{path}; });
    stub.set_lamda(ADDR(DBManager, getExistingPaths),
                   [](DBManager *, const QStringList &) -> QSet<QString> { return QSet<QString>(); });

    QSignalSpy repeatSpy(&thread, &ImportImagesThread::sigRepeatUrls);
    QSignalSpy failedSpy(&thread, &ImportImagesThread::sigImportFailed);
    QSignalSpy finishedSpy(&thread, &ImportImagesThread::sigImportFinished);
    call_private_fun::ImportImagesThreadrunDetail(thread);

    ASSERT_EQ(repeatSpy.count(), 1);
    ASSERT_EQ(repeatSpy.at(0).count(), 1);
    EXPECT_EQ(repeatSpy.at(0).at(0).toStringList(), QStringList{"file://" + path});
    EXPECT_EQ(failedSpy.count(), 0);
    EXPECT_EQ(finishedSpy.count(), 0);
}

// runDetail edge: UID >= u_CustomStart + no valid files → removeCustomAutoImportPath called
TEST_F(ImageEngineThreadTest, RunDetail_CustomAlbumNoFiles_CallsRemoveCustomAutoImportPath)
{
    stubDBManagerBasics();

    bool removeCalled = false;
    int capturedUID = -999;
    stub.set_lamda(ADDR(DBManager, removeCustomAutoImportPath),
                   [&removeCalled, &capturedUID](DBManager *, int uid) {
                       removeCalled = true;
                       capturedUID = uid;
                   });

    ImportImagesThread thread;
    access_private_field::ImportImagesThreadm_paths(thread) =
        QStringList{"/nonexistent/deepin/test_custom.png"};
    access_private_field::ImportImagesThreadm_UID(thread) =
        static_cast<int>(DBManager::u_CustomStart);

    QSignalSpy failedSpy(&thread, &ImportImagesThread::sigImportFailed);
    call_private_fun::ImportImagesThreadrunDetail(thread);

    EXPECT_TRUE(removeCalled);
    EXPECT_EQ(capturedUID, static_cast<int>(DBManager::u_CustomStart));
    ASSERT_EQ(failedSpy.count(), 1);
}

// ===========================================================================
// Part 3: ImagesClassifyThread
// ===========================================================================

// setData: info list is stored verbatim
TEST_F(ImageEngineThreadTest, Classify_SetData_StoresInfos)
{
    ImagesClassifyThread thread;
    DBImgInfoList infos;
    DBImgInfo info1;
    info1.filePath = "/tmp/a.png";
    info1.className = "";
    DBImgInfo info2;
    info2.filePath = "/tmp/b.jpg";
    info2.className = "Plant";
    infos << info1 << info2;

    thread.setData(infos);

    const auto &stored = access_private_field::ImagesClassifyThreadm_infos(thread);
    EXPECT_EQ(stored.size(), 2);
    EXPECT_EQ(stored[0].filePath, QString("/tmp/a.png"));
    EXPECT_EQ(stored[1].className, QString("Plant"));
}

// runDetail: className already non-empty → skipped; existing temp image → "Other";
// progress + finished signals emitted; updateClassName2DB called with results.
TEST_F(ImageEngineThreadTest, Classify_RunDetail_SkipsNonEmptyAndClassifies)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString imgPath = createTempImage(dir, "real.png", 30, 30);

    DBImgInfoList infos;
    // Item 0: className already set → should be skipped (no progress for it)
    DBImgInfo skipInfo;
    skipInfo.filePath = "/tmp/skip.png";
    skipInfo.className = "AlreadySet";
    // Item 1: real file, className empty, isSupportClassify false → "Other"
    DBImgInfo realInfo;
    realInfo.filePath = imgPath;
    realInfo.className = "";
    infos << skipInfo << realInfo;

    // Stub isSupportClassify to return false so imageClassify is not called
    stub.set_lamda(ADDR(Libutils::base, isSupportClassify),
                   [](const QString &) -> bool { return false; });

    // Stub DBManager::updateClassName2DB to capture the list
    DBImgInfoList captured;
    stub.set_lamda(ADDR(DBManager, updateClassName2DB),
                   [&captured](DBManager *, const DBImgInfoList &list) { captured = list; });

    ImagesClassifyThread thread;
    thread.setData(infos);

    auto *ac = AlbumControl::instance();
    QSignalSpy progressSpy(ac, &AlbumControl::progressOfImageClassify);
    QSignalSpy finishedSpy(ac, &AlbumControl::sigImageClassifyFinished);

    call_private_fun::ImagesClassifyThreadrunDetail(thread);

    // 2 progress signals: 1 initial + 1 for realInfo (skipInfo is skipped)
    EXPECT_EQ(progressSpy.count(), 2);
    ASSERT_EQ(progressSpy.at(0).count(), 2);
    EXPECT_EQ(progressSpy.at(0).at(0).toInt(), 2); // total
    EXPECT_EQ(progressSpy.at(0).at(1).toInt(), 0); // initial value
    ASSERT_EQ(progressSpy.at(1).count(), 2);
    EXPECT_EQ(progressSpy.at(1).at(0).toInt(), 2); // total
    EXPECT_EQ(progressSpy.at(1).at(1).toInt(), 0); // value after realInfo (i++ = 0)

    EXPECT_EQ(finishedSpy.count(), 1);

    ASSERT_EQ(captured.size(), 2);
    EXPECT_EQ(captured[0].className, QString("AlreadySet"));
    EXPECT_EQ(captured[1].className, QString("Other"));
}

// runDetail: file not exists → className stays "" (not readable → not "Other");
// missing files intentionally keep empty className
TEST_F(ImageEngineThreadTest, Classify_RunDetail_NonexistentFile_ClassNameEmpty)
{
    DBImgInfoList infos;
    DBImgInfo info;
    info.filePath = "/nonexistent/deepin/ghost.png";
    info.className = "";
    infos << info;

    stub.set_lamda(ADDR(Libutils::base, isSupportClassify),
                   [](const QString &) -> bool { return false; });

    DBImgInfoList captured;
    stub.set_lamda(ADDR(DBManager, updateClassName2DB),
                   [&captured](DBManager *, const DBImgInfoList &list) { captured = list; });

    ImagesClassifyThread thread;
    thread.setData(infos);

    auto *ac = AlbumControl::instance();
    QSignalSpy progressSpy(ac, &AlbumControl::progressOfImageClassify);
    QSignalSpy finishedSpy(ac, &AlbumControl::sigImageClassifyFinished);

    call_private_fun::ImagesClassifyThreadrunDetail(thread);

    // 2 progress signals: 1 initial + 1 for the item
    EXPECT_EQ(progressSpy.count(), 2);
    EXPECT_EQ(finishedSpy.count(), 1);
    ASSERT_EQ(captured.size(), 1);
    EXPECT_TRUE(captured[0].className.isEmpty());
}

// runDetail: empty info list → progress(total=0, 0) + finished, no crash
TEST_F(ImageEngineThreadTest, Classify_RunDetail_EmptyList_NoCrash)
{
    DBImgInfoList infos;

    DBImgInfoList captured;
    stub.set_lamda(ADDR(DBManager, updateClassName2DB),
                   [&captured](DBManager *, const DBImgInfoList &list) { captured = list; });

    ImagesClassifyThread thread;
    thread.setData(infos);

    auto *ac = AlbumControl::instance();
    QSignalSpy progressSpy(ac, &AlbumControl::progressOfImageClassify);
    QSignalSpy finishedSpy(ac, &AlbumControl::sigImageClassifyFinished);

    call_private_fun::ImagesClassifyThreadrunDetail(thread);

    EXPECT_EQ(progressSpy.count(), 1);
    EXPECT_EQ(finishedSpy.count(), 1);
    EXPECT_TRUE(captured.isEmpty());
}

// runDetail: file exists + supported + Classifyutils returns a label → label kept
TEST_F(ImageEngineThreadTest, Classify_RunDetail_SupportedFile_GetsClassLabel)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString imgPath = createTempImage(dir, "supported.png", 20, 20);

    DBImgInfoList infos;
    DBImgInfo info;
    info.filePath = imgPath;
    info.className = "";
    infos << info;

    stub.set_lamda(ADDR(Libutils::base, isSupportClassify),
                   [](const QString &) -> bool { return true; });

    // Stub imageClassify on the real singleton — no need to fake GetInstance.
    stub.set_lamda(ADDR(Classifyutils, imageClassify),
                   [](Classifyutils *, const QString &) -> QString { return "Animal"; });

    DBImgInfoList captured;
    stub.set_lamda(ADDR(DBManager, updateClassName2DB),
                   [&captured](DBManager *, const DBImgInfoList &list) { captured = list; });

    ImagesClassifyThread thread;
    thread.setData(infos);

    auto *ac = AlbumControl::instance();
    QSignalSpy progressSpy(ac, &AlbumControl::progressOfImageClassify);
    QSignalSpy finishedSpy(ac, &AlbumControl::sigImageClassifyFinished);

    call_private_fun::ImagesClassifyThreadrunDetail(thread);

    // 2 progress signals: 1 initial + 1 for the item
    EXPECT_EQ(progressSpy.count(), 2);
    EXPECT_EQ(finishedSpy.count(), 1);
    ASSERT_EQ(captured.size(), 1);
    EXPECT_EQ(captured[0].className, QString("Animal"));
}
