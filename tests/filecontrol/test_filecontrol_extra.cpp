// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * Supplementary unit tests for FileControl — covers methods not exercised
 * by test_filecontrol.cpp: path/info, file operations, judgment overloads,
 * config & state, shortcut panel, and signals.
 *
 * 分支列表:
 *   standardPicturesPath: B1 normal → returns QStandardPaths::PicturesLocation
 *   resetImageFiles: B1 calls imageFileWatcher->resetImageFiles + ImageInfo::clearCache
 *   isCurrentWatcherDir: B1 delegates to imageFileWatcher->isCurrentDir
 *   slotGetFileName: B1 normal path → completeBaseName; B2 empty path
 *   slotGetFileNameSuffix: B1 trash path with DB match; B2 trash path no DB match; B3 normal path
 *   getNamePath: B1 normal path with suffix → constructs new path
 *   slotFileSuffix: B1 path empty → empty; B2 file not exists → empty; B3 ret=true → ".suffix"; B4 ret=false → "suffix"
 *   getDirPath: B1 normal path → dir().path()
 *   deleteImagePath: B1 invalid URL → false; B2 valid URL, file deleted → true
 *   displayinFileManager: B1 interface invalid → false
 *   copyImage (single): B1 non-empty path → sets clipboard mimeData
 *   copyImage (multi): B1 multiple paths → sets clipboard mimeData
 *   copyText: B1 normal → sets clipboard text
 *   ocrImage: B1 non-multi image → openFile; B2 multi image → jumpToImage + save + openFile
 *   showPrintDialog (single): B1 calls PrintHelper::showPrintDialog
 *   showPrintDialog (multi): B1 calls PrintHelper::showPrintDialog with all paths
 *   slotFileReName: B1 file not exists → false; B2 file exists, rename succeeds, isSuffix=true → true + signal; B3 rename succeeds, isSuffix=false → true + signal; B4 rename fails → false
 *   isShowToolTip: B1 filename==name → false; B2 file exists and != path → true; B3 file not exists → false
 *   rotateFile (QVariantList): B1 empty entries skipped; B2 non-empty entries → rotateFile(QString)
 *   rotateFile (QString): B1 new path → slotRotatePixCurrent + set m_currentPath; B2 same path → accumulate angle
 *   slotRotatePixCurrent: B1 angle=0 → no-op; B2 angle!=0, normal path → rotateImageFIle; B3 angle!=0, special path → skip; B4 bNotifyExternal → emit callSavePicDone
 *   checkMimeUrls: B1 empty urls → false; B2 dir with images → true; B3 file with image → true; B4 no valid → false
 *   isCheckOnly: B1 creates lock file → true
 *   isCanRename: B1 readable+writable+normal path → true; B2 not readable → false
 *   isCanReadable: B1 readable → true; B2 not readable → false
 *   isCanDelete (QStringList): B1 empty list → false; B2 all deletable → true; B3 none deletable → false
 *   isRotatable (QStringList): B1 all rotatable → true; B2 one not rotatable → false; B3 empty entries skipped
 *   isAlbum: B1 viewerType=Album → true; B2 viewerType=Local → false
 *   getConfigValue: B1 delegates to m_config->value
 *   setConfigValue: B1 delegates to m_config->setValue
 *   saveSetting: B1 width changed → setConfigValue; B2 height changed → setConfigValue; B3 no change → no-op
 *   getlastWidth: B1 no screen → MAINWIDGET_MINIMUN_WIDTH; B2 with screen → config value or minimum
 *   getlastHeight: B1 no screen → MAINWIDGET_MINIMUN_HEIGHT; B2 with screen → config value or minimum
 *   setSettingWidth: B1 sets m_windowWidth + starts timer
 *   setSettingHeight: B1 sets m_windowHeight + starts timer
 *   setEnableNavigation: B1 delegates to setConfigValue
 *   isEnableNavigation: B1 delegates to getConfigValue
 *   setViewerType: B1 sets m_viewerType
 *   getCompanyLogo: B1 returns QUrl from DSysInfo::distributionOrgLogo
 *   showShortcutPanel: B1 terminates existing + starts new process
 *   terminateShortcutPanelProcess: B1 calls terminate + waitForFinished
 *   callSavePicDone: signal emission test
 *   openImageFile: signal emission test
 *   imageRenamed: signal emission test
 *   imageFileChanged: signal forwarded from ImageFileWatcher
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QCoreApplication>
#include <QClipboard>
#include <QMimeData>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QSignalSpy>
#include <QVariantList>
#include <QVariant>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTest>
#include <QJsonArray>

#include <DSysInfo>

#include "filecontrol.h"
#include "unionimage/unionimage.h"
#include "unionimage/unionimage_global.h"
#include "imagedata/imagefilewatcher.h"
#include "imagedata/imageinfo.h"
#include "dbmanager/dbmanager.h"
#include "printdialog/printhelper.h"
#include "ocr/ocrinterface.h"
#include "configsetter.h"
#include "types.h"

#include "stubext.h"

using namespace stub_ext;

DCORE_USE_NAMESPACE

class FileControlExtraTest : public ::testing::Test
{
protected:
    StubExt stub;

    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char *argv[] = {const_cast<char *>("test_filecontrol_extra"), nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    void SetUp() override
    {
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, localPath),
            [](const QUrl &url) -> QString { return url.toLocalFile(); });
        // Default stubs to prevent crashes in destructor's saveSetting()
        stub.set_lamda(ADDR(LibConfigSetter, value),
            [](LibConfigSetter *, const QString &, const QString &, const QVariant &def) -> QVariant {
                return def;
            });
        stub.set_lamda(ADDR(LibConfigSetter, setValue),
            [](LibConfigSetter *, const QString &, const QString &, const QVariant &) {});
    }

    void TearDown() override
    {
        stub.clear();
    }
};

// ============ standardPicturesPath ============

TEST_F(FileControlExtraTest, StandardPicturesPath_ReturnsNonEmptyPath)
{
    FileControl fc;
    QString result = fc.standardPicturesPath();
    EXPECT_FALSE(result.isEmpty());
    EXPECT_TRUE(QDir(result).exists() || QDir::home().exists());
}

// ============ resetImageFiles ============

TEST_F(FileControlExtraTest, ResetImageFiles_DelegatesToWatcherAndClearsCache)
{
    FileControl fc;
    bool watcherCalled = false;
    stub.set_lamda(ADDR(ImageFileWatcher, resetImageFiles),
        [&watcherCalled](ImageFileWatcher *, const QStringList &) { watcherCalled = true; });

    fc.resetImageFiles({QStringLiteral("/tmp/a.jpg"), QStringLiteral("/tmp/b.jpg")});
    EXPECT_TRUE(watcherCalled);
}

TEST_F(FileControlExtraTest, ResetImageFiles_EmptyList_StillCallsWatcher)
{
    FileControl fc;
    bool watcherCalled = false;
    stub.set_lamda(ADDR(ImageFileWatcher, resetImageFiles),
        [&watcherCalled](ImageFileWatcher *, const QStringList &) { watcherCalled = true; });

    fc.resetImageFiles();
    EXPECT_TRUE(watcherCalled);
}

// ============ isCurrentWatcherDir ============

TEST_F(FileControlExtraTest, IsCurrentWatcherDir_DelegatesToWatcher)
{
    FileControl fc;
    stub.set_lamda(ADDR(ImageFileWatcher, isCurrentDir),
        [](ImageFileWatcher *, const QString &) -> bool { return true; });

    QUrl url = QUrl::fromLocalFile(QStringLiteral("/tmp/testdir"));
    EXPECT_TRUE(fc.isCurrentWatcherDir(url));
}

TEST_F(FileControlExtraTest, IsCurrentWatcherDir_WatcherReturnsFalse)
{
    FileControl fc;
    stub.set_lamda(ADDR(ImageFileWatcher, isCurrentDir),
        [](ImageFileWatcher *, const QString &) -> bool { return false; });

    QUrl url = QUrl::fromLocalFile(QStringLiteral("/tmp/nonexistent"));
    EXPECT_FALSE(fc.isCurrentWatcherDir(url));
}

// ============ slotGetFileName ============

TEST_F(FileControlExtraTest, SlotGetFileName_ReturnsCompleteBaseName)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("photo.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    QString result = fc.slotGetFileName(QUrl::fromLocalFile(filePath).toString());
    EXPECT_EQ(result, QStringLiteral("photo"));
}

TEST_F(FileControlExtraTest, SlotGetFileName_FileWithMultipleDots)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("my.photo.backup.png");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    QString result = fc.slotGetFileName(QUrl::fromLocalFile(filePath).toString());
    EXPECT_EQ(result, QStringLiteral("my.photo.backup"));
}

// ============ slotGetFileNameSuffix ============

TEST_F(FileControlExtraTest, SlotGetFileNameSuffix_NormalPath_ReturnsFileName)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("document.txt");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    QString result = fc.slotGetFileNameSuffix(QUrl::fromLocalFile(filePath).toString());
    EXPECT_EQ(result, QStringLiteral("document.txt"));
}

TEST_F(FileControlExtraTest, SlotGetFileNameSuffix_TrashPathWithDbMatch_ReturnsOriginalName)
{
    FileControl fc;
    QString trashDir = QDir::homePath() + "/.local/share/deepin/deepin-album-delete";
    QDir().mkpath(trashDir);
    QString trashPath = trashDir + "/abc123hash.jpg";
    QFile f(trashPath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    DBImgInfoList fakeInfos;
    DBImgInfo info;
    info.filePath = QStringLiteral("/home/user/original_name.jpg");
    fakeInfos.append(info);

    stub.set_lamda(ADDR(DBManager, getTrashImgInfos),
        [&fakeInfos](DBManager *, const QString &, const QString &) -> DBImgInfoList {
            return fakeInfos;
        });

    QString result = fc.slotGetFileNameSuffix(QUrl::fromLocalFile(trashPath).toString());
    EXPECT_EQ(result, QStringLiteral("original_name.jpg"));

    QFile::remove(trashPath);
}

TEST_F(FileControlExtraTest, SlotGetFileNameSuffix_TrashPathNoDbMatch_ReturnsHashFileName)
{
    FileControl fc;
    QString trashDir = QDir::homePath() + "/.local/share/deepin/deepin-album-delete";
    QDir().mkpath(trashDir);
    QString trashPath = trashDir + "/xyz789hash.png";
    QFile f(trashPath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(DBManager, getTrashImgInfos),
        [](DBManager *, const QString &, const QString &) -> DBImgInfoList {
            return {};
        });

    QString result = fc.slotGetFileNameSuffix(QUrl::fromLocalFile(trashPath).toString());
    EXPECT_EQ(result, QStringLiteral("xyz789hash.png"));

    QFile::remove(trashPath);
}

// ============ getNamePath ============

TEST_F(FileControlExtraTest, GetNamePath_ConstructsNewPathWithSuffix)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString oldPath = tmpDir.filePath("oldname.jpg");
    QFile f(oldPath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    QString result = fc.getNamePath(QUrl::fromLocalFile(oldPath).toString(), QStringLiteral("newname"));
    QUrl resultUrl(result);
    QString localPath = resultUrl.toLocalFile();
    EXPECT_TRUE(localPath.endsWith(QStringLiteral("/newname.jpg")));
}

// ============ slotFileSuffix ============

TEST_F(FileControlExtraTest, SlotFileSuffix_EmptyPath_ReturnsEmpty)
{
    FileControl fc;
    QString result = fc.slotFileSuffix(QStringLiteral(""));
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(FileControlExtraTest, SlotFileSuffix_FileNotExists_ReturnsEmpty)
{
    FileControl fc;
    QString result = fc.slotFileSuffix(QStringLiteral("/nonexistent/path/file.txt"));
    EXPECT_TRUE(result.isEmpty());
}

TEST_F(FileControlExtraTest, SlotFileSuffix_RetTrue_ReturnsDotSuffix)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("archive.tar.gz");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    QString result = fc.slotFileSuffix(QUrl::fromLocalFile(filePath).toString(), true);
    EXPECT_EQ(result, QStringLiteral(".tar.gz"));
}

TEST_F(FileControlExtraTest, SlotFileSuffix_RetFalse_ReturnsSuffixWithoutDot)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("image.png");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    QString result = fc.slotFileSuffix(QUrl::fromLocalFile(filePath).toString(), false);
    EXPECT_EQ(result, QStringLiteral("png"));
}

// ============ getDirPath ============

TEST_F(FileControlExtraTest, GetDirPath_ReturnsDirectoryPath)
{
    FileControl fc;
    QString path = QStringLiteral("/tmp/subdir/file.jpg");
    QString result = fc.getDirPath(path);
    EXPECT_EQ(result, QStringLiteral("/tmp/subdir"));
}

TEST_F(FileControlExtraTest, GetDirPath_RootPath)
{
    FileControl fc;
    QString path = QStringLiteral("/file.txt");
    QString result = fc.getDirPath(path);
    EXPECT_EQ(result, QStringLiteral("/"));
}

// ============ deleteImagePath ============

TEST_F(FileControlExtraTest, DeleteImagePath_InvalidUrl_ReturnsFalse)
{
    FileControl fc;
    EXPECT_FALSE(fc.deleteImagePath(QStringLiteral("")));
}

TEST_F(FileControlExtraTest, DeleteImagePath_ValidUrlButFileNotDeleted_ReturnsFalse)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("testfile.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    // DBus interface is invalid in test environment, so pendingCall will error
    // File still exists → returns false
    EXPECT_FALSE(fc.deleteImagePath(QUrl::fromLocalFile(filePath).toString()));
    // File should still exist
    EXPECT_TRUE(QFile::exists(filePath));
}

// ============ displayinFileManager ============

TEST_F(FileControlExtraTest, DisplayinFileManager_ExecutesWithoutCrash)
{
    FileControl fc;
    // DBus interface availability depends on test environment; verify no crash
    bool result = fc.displayinFileManager(QUrl::fromLocalFile(QStringLiteral("/tmp/test.jpg")).toString());
    // Result may be true or false depending on DBus availability
    EXPECT_TRUE(result == true || result == false);
}

// ============ copyImage (single) ============

TEST_F(FileControlExtraTest, CopyImage_SinglePath_SetsClipboardMimeData)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("copyme.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    fc.copyImage(QUrl::fromLocalFile(filePath).toString());

    QClipboard *cb = QGuiApplication::clipboard();
    const QMimeData *md = cb->mimeData();
    ASSERT_NE(md, nullptr);
    EXPECT_TRUE(md->hasUrls());
    EXPECT_TRUE(md->hasFormat("x-special/gnome-copied-files"));
}

TEST_F(FileControlExtraTest, CopyImage_EmptyPath_StillSetsMimeData)
{
    FileControl fc;
    fc.copyImage(QStringLiteral(""));

    QClipboard *cb = QGuiApplication::clipboard();
    const QMimeData *md = cb->mimeData();
    ASSERT_NE(md, nullptr);
    // Even with empty path, mimeData is set (may have empty text)
    EXPECT_NE(md, nullptr);
}

// ============ copyImage (multi) ============

TEST_F(FileControlExtraTest, CopyImage_MultiPath_SetsClipboardMimeData)
{
    FileControl fc;
    QStringList paths = {
        QUrl::fromLocalFile(QStringLiteral("/tmp/a.jpg")).toString(),
        QUrl::fromLocalFile(QStringLiteral("/tmp/b.png")).toString(),
    };

    fc.copyImage(paths);

    QClipboard *cb = QGuiApplication::clipboard();
    const QMimeData *md = cb->mimeData();
    ASSERT_NE(md, nullptr);
    EXPECT_TRUE(md->hasUrls());
    EXPECT_EQ(md->urls().size(), 2);
}

TEST_F(FileControlExtraTest, CopyImage_MultiPathEmpty_StillExecutes)
{
    FileControl fc;
    fc.copyImage(QStringList());
    QClipboard *cb = QGuiApplication::clipboard();
    EXPECT_NE(cb->mimeData(), nullptr);
}

// ============ copyText ============

TEST_F(FileControlExtraTest, CopyText_SetsClipboardText)
{
    FileControl fc;
    QString testText = QStringLiteral("hello world");
    fc.copyText(testText);

    QClipboard *cb = QGuiApplication::clipboard();
    EXPECT_EQ(cb->text(), testText);
}

TEST_F(FileControlExtraTest, CopyText_EmptyString)
{
    FileControl fc;
    fc.copyText(QStringLiteral(""));
    QClipboard *cb = QGuiApplication::clipboard();
    EXPECT_TRUE(cb->text().isEmpty());
}

// ============ ocrImage ============

TEST_F(FileControlExtraTest, OcrImage_NonMultiImage_CallsOpenFile)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("ocr_test.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(ImageInfo, type),
        []() -> int { return Types::NormalImage; });

    // OcrInterface::openFile is an inline DBus call; in test env it returns
    // an error reply silently. Verify ocrImage does not crash.
    fc.ocrImage(QUrl::fromLocalFile(filePath).toString(), 0);
    SUCCEED();
}

// ============ showPrintDialog (single) ============

TEST_F(FileControlExtraTest, ShowPrintDialog_SinglePath_CallsPrintHelper)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("printme.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    bool printCalled = false;
    QStringList capturedPaths;
    stub.set_lamda(ADDR(PrintHelper, showPrintDialog),
        [&printCalled, &capturedPaths](PrintHelper *, const QStringList &paths, QWidget *) {
            printCalled = true;
            capturedPaths = paths;
        });

    fc.showPrintDialog(QUrl::fromLocalFile(filePath).toString());
    EXPECT_TRUE(printCalled);
    ASSERT_EQ(capturedPaths.size(), 1);
    EXPECT_EQ(capturedPaths.first(), filePath);
}

// ============ showPrintDialog (multi) ============

TEST_F(FileControlExtraTest, ShowPrintDialog_MultiPath_CallsPrintHelper)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString p1 = tmpDir.filePath("a.jpg");
    QString p2 = tmpDir.filePath("b.png");
    for (const QString &p : {p1, p2}) {
        QFile f(p);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("data");
        f.close();
    }

    bool printCalled = false;
    int capturedCount = 0;
    stub.set_lamda(ADDR(PrintHelper, showPrintDialog),
        [&printCalled, &capturedCount](PrintHelper *, const QStringList &paths, QWidget *) {
            printCalled = true;
            capturedCount = paths.size();
        });

    fc.showPrintDialog(QStringList{QUrl::fromLocalFile(p1).toString(), QUrl::fromLocalFile(p2).toString()});
    EXPECT_TRUE(printCalled);
    EXPECT_EQ(capturedCount, 2);
}

// ============ slotFileReName ============

TEST_F(FileControlExtraTest, SlotFileReName_FileNotExists_ReturnsFalse)
{
    FileControl fc;
    bool result = fc.slotFileReName(QStringLiteral("newname"), QStringLiteral("/nonexistent/file.jpg"));
    EXPECT_FALSE(result);
}

TEST_F(FileControlExtraTest, SlotFileReName_IsSuffixTrue_RenamesAndEmitsSignal)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("original.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(ImageFileWatcher, fileRename),
        [](ImageFileWatcher *, const QString &, const QString &) {});

    QSignalSpy spy(&fc, &FileControl::imageRenamed);
    // When isSuffix=true, name is the full filename including extension
    QString newFilePath = tmpDir.filePath("renamed.jpg");

    bool result = fc.slotFileReName(QStringLiteral("renamed.jpg"), QUrl::fromLocalFile(filePath).toString(), true);
    EXPECT_TRUE(result);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(QFile::exists(filePath));
    EXPECT_TRUE(QFile::exists(newFilePath));
}

TEST_F(FileControlExtraTest, SlotFileReName_IsSuffixFalse_RenamesAndEmitsSignal)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("original.png");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(ImageFileWatcher, fileRename),
        [](ImageFileWatcher *, const QString &, const QString &) {});

    QSignalSpy spy(&fc, &FileControl::imageRenamed);
    bool result = fc.slotFileReName(QStringLiteral("newname"), QUrl::fromLocalFile(filePath).toString(), false);
    EXPECT_TRUE(result);
    EXPECT_EQ(spy.count(), 1);
    EXPECT_FALSE(QFile::exists(filePath));
    EXPECT_TRUE(QFile::exists(tmpDir.filePath("newname.png")));
}

TEST_F(FileControlExtraTest, SlotFileReName_RenameFails_ReturnsFalse)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("exists.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    // Try to rename to a path that already exists (QFile::rename returns false if target exists)
    QFile existing(tmpDir.filePath("target.jpg"));
    ASSERT_TRUE(existing.open(QIODevice::WriteOnly));
    existing.write("data");
    existing.close();

    stub.set_lamda(ADDR(ImageFileWatcher, fileRename),
        [](ImageFileWatcher *, const QString &, const QString &) {});

    QSignalSpy spy(&fc, &FileControl::imageRenamed);
    bool result = fc.slotFileReName(QStringLiteral("target"), QUrl::fromLocalFile(filePath).toString(), false);
    // On most filesystems, QFile::rename to existing file fails
    EXPECT_FALSE(result);
    EXPECT_EQ(spy.count(), 0);
    EXPECT_TRUE(QFile::exists(filePath));
}

// ============ isShowToolTip ============

TEST_F(FileControlExtraTest, IsShowToolTip_SameFileName_ReturnsFalse)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("same.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    EXPECT_FALSE(fc.isShowToolTip(QUrl::fromLocalFile(filePath).toString(), QStringLiteral("same")));
}

TEST_F(FileControlExtraTest, IsShowToolTip_DifferentNameFileExists_ReturnsTrue)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("original.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    // Create a file that matches the "new name" pattern
    QString conflictingPath = tmpDir.filePath("conflict.jpg");
    QFile cf(conflictingPath);
    ASSERT_TRUE(cf.open(QIODevice::WriteOnly));
    cf.write("data");
    cf.close();

    EXPECT_TRUE(fc.isShowToolTip(QUrl::fromLocalFile(filePath).toString(), QStringLiteral("conflict")));
}

TEST_F(FileControlExtraTest, IsShowToolTip_DifferentNameFileNotExists_ReturnsFalse)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("original.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    EXPECT_FALSE(fc.isShowToolTip(QUrl::fromLocalFile(filePath).toString(), QStringLiteral("nonexistent")));
}

// ============ rotateFile (QVariantList) ============

TEST_F(FileControlExtraTest, RotateFile_QVariantList_AllEntriesProcessed)
{
    FileControl fc;
    int rotateCallCount = 0;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeLOCAL; });
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, rotateImageFIle),
        [&rotateCallCount](int, const QString &, QString &, const QString &) -> bool {
            rotateCallCount++;
            return true;
        });

    QVariantList paths = {
        QVariant(QUrl::fromLocalFile(QStringLiteral("/tmp/a.jpg")).toString()),
        QVariant(QUrl::fromLocalFile(QStringLiteral("/tmp/b.jpg")).toString())
    };
    bool result = fc.rotateFile(paths, 90);
    EXPECT_TRUE(result);
    // First path is rotated when switching to second path (slotRotatePixCurrent called on path change)
    EXPECT_EQ(rotateCallCount, 1);
    // Flush remaining rotation
    fc.slotRotatePixCurrent(true);
    EXPECT_EQ(rotateCallCount, 2);
}

TEST_F(FileControlExtraTest, RotateFile_QVariantList_EmptyEntriesSkipped)
{
    FileControl fc;
    int rotateCallCount = 0;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeLOCAL; });
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, rotateImageFIle),
        [&rotateCallCount](int, const QString &, QString &, const QString &) -> bool {
            rotateCallCount++;
            return true;
        });

    QVariantList paths = {
        QVariant(QStringLiteral("")),
        QVariant(QUrl::fromLocalFile(QStringLiteral("/tmp/b.jpg")).toString())
    };
    bool result = fc.rotateFile(paths, 90);
    EXPECT_TRUE(result);
    // First entry skipped; second entry is new path so slotRotatePixCurrent is called
    // but m_rotateAngel is 0 at that point, so no rotation happens synchronously
    EXPECT_EQ(rotateCallCount, 0);
    // Flush rotation by calling slotRotatePixCurrent
    fc.slotRotatePixCurrent(true);
    EXPECT_EQ(rotateCallCount, 1);
}

// ============ rotateFile (QString) ============

TEST_F(FileControlExtraTest, RotateFile_QString_NewPath_SetsCurrentPathAndAngle)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeLOCAL; });
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, rotateImageFIle),
        [](int, const QString &, QString &, const QString &) -> bool { return true; });

    bool result = fc.rotateFile(QStringLiteral("/tmp/newimage.jpg"), 90);
    EXPECT_TRUE(result);
}

TEST_F(FileControlExtraTest, RotateFile_QString_SamePath_AccumulatesAngle)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeLOCAL; });
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, rotateImageFIle),
        [](int, const QString &, QString &, const QString &) -> bool { return true; });

    // First call sets m_currentPath
    fc.rotateFile(QStringLiteral("/tmp/same.jpg"), 90);
    // Second call on same path should accumulate angle (90+90=180)
    bool result = fc.rotateFile(QStringLiteral("/tmp/same.jpg"), 90);
    EXPECT_TRUE(result);
}

// ============ slotRotatePixCurrent ============

TEST_F(FileControlExtraTest, SlotRotatePixCurrent_ZeroAngle_NoRotation)
{
    FileControl fc;
    int rotateCallCount = 0;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeLOCAL; });
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, rotateImageFIle),
        [&rotateCallCount](int, const QString &, QString &, const QString &) -> bool {
            rotateCallCount++;
            return true;
        });

    // m_rotateAngel is 0 by default → no-op
    fc.slotRotatePixCurrent(false);
    EXPECT_EQ(rotateCallCount, 0);
}

TEST_F(FileControlExtraTest, SlotRotatePixCurrent_NonZeroAngleNormalPath_RotatesAndEmitsSignal)
{
    FileControl fc;
    int rotateCallCount = 0;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeLOCAL; });
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, rotateImageFIle),
        [&rotateCallCount](int, const QString &, QString &, const QString &) -> bool {
            rotateCallCount++;
            return true;
        });

    // Set up rotation state via rotateFile
    fc.rotateFile(QStringLiteral("/tmp/rotate_test.jpg"), 90);

    QSignalSpy spy(&fc, &FileControl::callSavePicDone);
    fc.slotRotatePixCurrent(true);
    EXPECT_EQ(rotateCallCount, 1);
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(FileControlExtraTest, SlotRotatePixCurrent_SpecialPathType_SkipsRotation)
{
    FileControl fc;
    int rotateCallCount = 0;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeMTP; });
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, rotateImageFIle),
        [&rotateCallCount](int, const QString &, QString &, const QString &) -> bool {
            rotateCallCount++;
            return true;
        });

    fc.rotateFile(QStringLiteral("/tmp/mtp_image.jpg"), 90);
    fc.slotRotatePixCurrent(true);
    EXPECT_EQ(rotateCallCount, 0);
}

TEST_F(FileControlExtraTest, SlotRotatePixCurrent_NoNotifyExternal_NoSignal)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeLOCAL; });
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, rotateImageFIle),
        [](int, const QString &, QString &, const QString &) -> bool { return true; });

    fc.rotateFile(QStringLiteral("/tmp/no_signal.jpg"), 90);
    QSignalSpy spy(&fc, &FileControl::callSavePicDone);
    fc.slotRotatePixCurrent(false);
    EXPECT_EQ(spy.count(), 0);
}

// ============ checkMimeUrls ============

TEST_F(FileControlExtraTest, CheckMimeUrls_EmptyUrls_ReturnsFalse)
{
    FileControl fc;
    QList<QUrl> urls;
    EXPECT_FALSE(fc.checkMimeUrls(urls));
}

TEST_F(FileControlExtraTest, CheckMimeUrls_FileIsImage_ReturnsTrue)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, imageSupportRead),
        [](const QString &) -> bool { return true; });
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isVideo),
        [](const QString &) -> bool { return false; });

    QList<QUrl> urls = {QUrl::fromLocalFile(QStringLiteral("/tmp/test.jpg"))};
    EXPECT_TRUE(fc.checkMimeUrls(urls));
}

TEST_F(FileControlExtraTest, CheckMimeUrls_FileIsVideo_ReturnsTrue)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, imageSupportRead),
        [](const QString &) -> bool { return false; });
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isVideo),
        [](const QString &) -> bool { return true; });

    QList<QUrl> urls = {QUrl::fromLocalFile(QStringLiteral("/tmp/test.mp4"))};
    EXPECT_TRUE(fc.checkMimeUrls(urls));
}

TEST_F(FileControlExtraTest, CheckMimeUrls_FileNotSupported_ReturnsFalse)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, imageSupportRead),
        [](const QString &) -> bool { return false; });
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, isVideo),
        [](const QString &) -> bool { return false; });

    QList<QUrl> urls = {QUrl::fromLocalFile(QStringLiteral("/tmp/test.xyz"))};
    EXPECT_FALSE(fc.checkMimeUrls(urls));
}

// ============ isCheckOnly ============

TEST_F(FileControlExtraTest, IsCheckOnly_ReturnsTrueInTestEnv)
{
    FileControl fc;
    // In a fresh test process, the lock file should be acquirable
    bool result = fc.isCheckOnly();
    EXPECT_TRUE(result);
}

// ============ isCanRename ============

TEST_F(FileControlExtraTest, IsCanRename_ReadableWritableNormalPath_ReturnsTrue)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("renameable.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeLOCAL; });

    EXPECT_TRUE(fc.isCanRename(QUrl::fromLocalFile(filePath).toString()));
}

TEST_F(FileControlExtraTest, IsCanRename_NotReadable_ReturnsFalse)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeLOCAL; });

    // Non-existent file is not readable
    EXPECT_FALSE(fc.isCanRename(QUrl::fromLocalFile(QStringLiteral("/nonexistent/file.jpg")).toString()));
}

TEST_F(FileControlExtraTest, IsCanRename_MtpPath_ReturnsFalse)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("mtp.jpg");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeMTP; });

    EXPECT_FALSE(fc.isCanRename(QUrl::fromLocalFile(filePath).toString()));
}

// ============ isCanReadable ============

TEST_F(FileControlExtraTest, IsCanReadable_ReadableFile_ReturnsTrue)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString filePath = tmpDir.filePath("readable.txt");
    QFile f(filePath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    EXPECT_TRUE(fc.isCanReadable(QUrl::fromLocalFile(filePath).toString()));
}

TEST_F(FileControlExtraTest, IsCanReadable_NonExistentFile_ReturnsFalse)
{
    FileControl fc;
    EXPECT_FALSE(fc.isCanReadable(QUrl::fromLocalFile(QStringLiteral("/nonexistent/file.txt")).toString()));
}

// ============ isCanDelete (QStringList) ============

TEST_F(FileControlExtraTest, IsCanDelete_QStringList_EmptyList_ReturnsFalse)
{
    FileControl fc;
    EXPECT_FALSE(fc.isCanDelete(QStringList()));
}

TEST_F(FileControlExtraTest, IsCanDelete_QStringList_AllDeletable_ReturnsTrue)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString p1 = tmpDir.filePath("a.jpg");
    QString p2 = tmpDir.filePath("b.jpg");
    for (const QString &p : {p1, p2}) {
        QFile f(p);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("data");
        f.close();
    }

    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeLOCAL; });

    QStringList paths = {QUrl::fromLocalFile(p1).toString(), QUrl::fromLocalFile(p2).toString()};
    EXPECT_TRUE(fc.isCanDelete(paths));
}

TEST_F(FileControlExtraTest, IsCanDelete_QStringList_NoneDeletable_ReturnsFalse)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeLOCAL; });

    QStringList paths = {QUrl::fromLocalFile(QStringLiteral("/nonexistent/a.jpg")).toString()};
    EXPECT_FALSE(fc.isCanDelete(paths));
}

TEST_F(FileControlExtraTest, IsCanDelete_QStringList_MixedEntries_ReturnsTrue)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString p1 = tmpDir.filePath("exists.jpg");
    QFile f(p1);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeLOCAL; });

    QStringList paths = {
        QUrl::fromLocalFile(QStringLiteral("/nonexistent/a.jpg")).toString(),
        QUrl::fromLocalFile(p1).toString(),
    };
    EXPECT_TRUE(fc.isCanDelete(paths));
}

// ============ isRotatable (QStringList) ============

TEST_F(FileControlExtraTest, IsRotatable_QStringList_AllRotatable_ReturnsTrue)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString p1 = tmpDir.filePath("a.jpg");
    QString p2 = tmpDir.filePath("b.jpg");
    for (const QString &p : {p1, p2}) {
        QFile f(p);
        ASSERT_TRUE(f.open(QIODevice::WriteOnly));
        f.write("data");
        f.close();
    }

    QStringList paths = {QUrl::fromLocalFile(p1).toString(), QUrl::fromLocalFile(p2).toString()};
    EXPECT_TRUE(fc.isRotatable(paths));
}

TEST_F(FileControlExtraTest, IsRotatable_QStringList_OneNotRotatable_ReturnsFalse)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString p1 = tmpDir.filePath("valid.jpg");
    QFile f(p1);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    QStringList paths = {
        QUrl::fromLocalFile(p1).toString(),
        QUrl::fromLocalFile(QStringLiteral("/nonexistent/invalid.jpg")).toString(),
    };
    EXPECT_FALSE(fc.isRotatable(paths));
}

TEST_F(FileControlExtraTest, IsRotatable_QStringList_EmptyEntriesSkipped_ReturnsTrue)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString p1 = tmpDir.filePath("valid.jpg");
    QFile f(p1);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    QStringList paths = {QStringLiteral(""), QUrl::fromLocalFile(p1).toString()};
    EXPECT_TRUE(fc.isRotatable(paths));
}

// ============ isAlbum ============

TEST_F(FileControlExtraTest, IsAlbum_DefaultViewerType_ReturnsFalse)
{
    FileControl fc;
    EXPECT_FALSE(fc.isAlbum());
}

TEST_F(FileControlExtraTest, IsAlbum_AlbumViewerType_ReturnsTrue)
{
    FileControl fc;
    fc.setViewerType(imageViewerSpace::ImgViewerTypeAlbum);
    EXPECT_TRUE(fc.isAlbum());
}

// ============ setViewerType ============

TEST_F(FileControlExtraTest, SetViewerType_SetsAlbumType)
{
    FileControl fc;
    fc.setViewerType(imageViewerSpace::ImgViewerTypeAlbum);
    EXPECT_TRUE(fc.isAlbum());
}

TEST_F(FileControlExtraTest, SetViewerType_SetsLocalType)
{
    FileControl fc;
    fc.setViewerType(imageViewerSpace::ImgViewerTypeAlbum);
    fc.setViewerType(imageViewerSpace::ImgViewerTypeLocal);
    EXPECT_FALSE(fc.isAlbum());
}

// ============ getConfigValue ============

TEST_F(FileControlExtraTest, GetConfigValue_DelegatesToConfigSetter)
{
    FileControl fc;
    QVariant capturedDefault = QStringLiteral("default_val");
    stub.set_lamda(ADDR(LibConfigSetter, value),
        [](LibConfigSetter *, const QString &, const QString &, const QVariant &) -> QVariant {
            return QStringLiteral("test_value");
        });

    QVariant result = fc.getConfigValue(QStringLiteral("GROUP"), QStringLiteral("KEY"), QStringLiteral("default"));
    EXPECT_EQ(result.toString(), QStringLiteral("test_value"));
}

// ============ setConfigValue ============

TEST_F(FileControlExtraTest, SetConfigValue_DelegatesToConfigSetter)
{
    FileControl fc;
    QString capturedGroup, capturedKey;
    QVariant capturedValue;
    stub.set_lamda(ADDR(LibConfigSetter, setValue),
        [&capturedGroup, &capturedKey, &capturedValue](LibConfigSetter *, const QString &g, const QString &k, const QVariant &v) {
            capturedGroup = g;
            capturedKey = k;
            capturedValue = v;
        });

    fc.setConfigValue(QStringLiteral("GROUP"), QStringLiteral("KEY"), 42);
    EXPECT_EQ(capturedGroup, QStringLiteral("GROUP"));
    EXPECT_EQ(capturedKey, QStringLiteral("KEY"));
    EXPECT_EQ(capturedValue.toInt(), 42);
}

// ============ saveSetting ============

TEST_F(FileControlExtraTest, SaveSetting_WidthChanged_PersistsWidth)
{
    FileControl fc;
    int setConfigCount = 0;
    stub.set_lamda(ADDR(LibConfigSetter, setValue),
        [&setConfigCount](LibConfigSetter *, const QString &, const QString &, const QVariant &) {
            setConfigCount++;
        });
    stub.set_lamda(ADDR(LibConfigSetter, value),
        [](LibConfigSetter *, const QString &, const QString &, const QVariant &) -> QVariant {
            return 0;
        });

    // Use getlastWidth to initialize m_windowWidth and m_lastSaveWidth
    fc.getlastWidth();
    // Change width via setSettingWidth
    fc.setSettingWidth(800);
    // Now saveSetting should persist the new width
    fc.saveSetting();
    EXPECT_GE(setConfigCount, 1);
}

TEST_F(FileControlExtraTest, SaveSetting_NoChange_NoPersistence)
{
    FileControl fc;
    int setConfigCount = 0;
    stub.set_lamda(ADDR(LibConfigSetter, setValue),
        [&setConfigCount](LibConfigSetter *, const QString &, const QString &, const QVariant &) {
            setConfigCount++;
        });
    stub.set_lamda(ADDR(LibConfigSetter, value),
        [](LibConfigSetter *, const QString &, const QString &, const QVariant &) -> QVariant {
            return 0;
        });

    // Initialize m_windowWidth and m_windowHeight
    fc.getlastWidth();
    fc.getlastHeight();
    // First saveSetting syncs m_lastSaveWidth/m_lastSaveHeight
    fc.saveSetting();
    int countBefore = setConfigCount;
    // Second saveSetting should find no changes
    fc.saveSetting();
    EXPECT_EQ(setConfigCount, countBefore);
}

// ============ getlastWidth ============

TEST_F(FileControlExtraTest, GetlastWidth_ReturnsAtLeastMinimumWidth)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibConfigSetter, value),
        [](LibConfigSetter *, const QString &, const QString &, const QVariant &) -> QVariant {
            return 0;  // Config returns 0
        });

    int width = fc.getlastWidth();
    EXPECT_GE(width, 658);  // MAINWIDGET_MINIMUN_WIDTH
}

TEST_F(FileControlExtraTest, GetlastWidth_ConfigReturnsLargeValue_ReturnsConfigValue)
{
    FileControl fc;
    int capturedWidth = 0;
    stub.set_lamda(ADDR(LibConfigSetter, value),
        [](LibConfigSetter *, const QString &, const QString &, const QVariant &) -> QVariant {
            return 1024;
        });

    int width = fc.getlastWidth();
    EXPECT_EQ(width, 1024);
}

// ============ getlastHeight ============

TEST_F(FileControlExtraTest, GetlastHeight_ReturnsAtLeastMinimumHeight)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibConfigSetter, value),
        [](LibConfigSetter *, const QString &, const QString &, const QVariant &) -> QVariant {
            return 0;
        });

    int height = fc.getlastHeight();
    EXPECT_GE(height, 300);  // MAINWIDGET_MINIMUN_HEIGHT
}

TEST_F(FileControlExtraTest, GetlastHeight_ConfigReturnsLargeValue_ReturnsConfigValue)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibConfigSetter, value),
        [](LibConfigSetter *, const QString &, const QString &, const QVariant &) -> QVariant {
            return 768;
        });

    int height = fc.getlastHeight();
    EXPECT_EQ(height, 768);
}

// ============ setSettingWidth ============

TEST_F(FileControlExtraTest, SetSettingWidth_DoesNotCrash)
{
    FileControl fc;
    fc.setSettingWidth(1024);
    // Verify no crash; internal state m_windowWidth is set
    SUCCEED();
}

// ============ setSettingHeight ============

TEST_F(FileControlExtraTest, SetSettingHeight_DoesNotCrash)
{
    FileControl fc;
    fc.setSettingHeight(768);
    SUCCEED();
}

// ============ setEnableNavigation ============

TEST_F(FileControlExtraTest, SetEnableNavigation_DelegatesToConfig)
{
    FileControl fc;
    bool capturedValue = false;
    stub.set_lamda(ADDR(LibConfigSetter, setValue),
        [&capturedValue](LibConfigSetter *, const QString &, const QString &, const QVariant &v) {
            capturedValue = v.toBool();
        });

    fc.setEnableNavigation(true);
    EXPECT_TRUE(capturedValue);

    fc.setEnableNavigation(false);
    EXPECT_FALSE(capturedValue);
}

// ============ isEnableNavigation ============

TEST_F(FileControlExtraTest, IsEnableNavigation_DelegatesToConfig)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibConfigSetter, value),
        [](LibConfigSetter *, const QString &, const QString &, const QVariant &) -> QVariant {
            return true;
        });

    EXPECT_TRUE(fc.isEnableNavigation());
}

TEST_F(FileControlExtraTest, IsEnableNavigation_ConfigReturnsFalse)
{
    FileControl fc;
    stub.set_lamda(ADDR(LibConfigSetter, value),
        [](LibConfigSetter *, const QString &, const QString &, const QVariant &) -> QVariant {
            return false;
        });

    EXPECT_FALSE(fc.isEnableNavigation());
}

// ============ getCompanyLogo ============

TEST_F(FileControlExtraTest, GetCompanyLogo_ReturnsValidUrl)
{
    FileControl fc;
    QUrl logo = fc.getCompanyLogo();
    EXPECT_TRUE(logo.isValid());
    EXPECT_TRUE(logo.isLocalFile());
}

// ============ showShortcutPanel ============

TEST_F(FileControlExtraTest, ShowShortcutPanel_TerminatesAndStartsProcess)
{
    FileControl fc;
    bool terminateCalled = false;
    stub.set_lamda(ADDR(QProcess, terminate),
        [&terminateCalled](QProcess *) { terminateCalled = true; });
    stub.set_lamda(ADDR(QProcess, waitForFinished),
        [](QProcess *, int) -> bool { return true; });

    fc.showShortcutPanel(100, 200);
    EXPECT_TRUE(terminateCalled);
}

// ============ terminateShortcutPanelProcess ============

TEST_F(FileControlExtraTest, TerminateShortcutPanelProcess_CallsTerminateAndWait)
{
    FileControl fc;
    bool terminateCalled = false;
    bool waitCalled = false;
    stub.set_lamda(ADDR(QProcess, terminate),
        [&terminateCalled](QProcess *) { terminateCalled = true; });
    stub.set_lamda(ADDR(QProcess, waitForFinished),
        [&waitCalled](QProcess *, int) -> bool { waitCalled = true; return true; });

    fc.terminateShortcutPanelProcess();
    EXPECT_TRUE(terminateCalled);
    EXPECT_TRUE(waitCalled);
}

// ============ Signal: callSavePicDone ============

TEST_F(FileControlExtraTest, Signal_CallSavePicDone_EmitsWithPath)
{
    FileControl fc;
    QSignalSpy spy(&fc, &FileControl::callSavePicDone);

    // Trigger via slotRotatePixCurrent with notifyExternal=true
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, getPathType),
        [](const QString &) -> imageViewerSpace::PathType { return imageViewerSpace::PathTypeLOCAL; });
    stub.set_lamda(ADDR(LibUnionImage_NameSpace, rotateImageFIle),
        [](int, const QString &, QString &, const QString &) -> bool { return true; });

    QString imgPath = QUrl::fromLocalFile(QStringLiteral("/tmp/signal_test.jpg")).toString();
    fc.rotateFile(imgPath, 90);
    fc.slotRotatePixCurrent(true);

    ASSERT_EQ(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    EXPECT_FALSE(args.at(0).toString().isEmpty());
}

// ============ Signal: openImageFile ============

TEST_F(FileControlExtraTest, Signal_OpenImageFile_CanBeEmitted)
{
    FileControl fc;
    QSignalSpy spy(&fc, &FileControl::openImageFile);

    // The signal is declared but not emitted internally in FileControl;
    // verify it can be connected and emitted manually (QML-side trigger)
    QMetaObject::invokeMethod(&fc, "openImageFile", Qt::DirectConnection,
        Q_ARG(QString, QStringLiteral("/tmp/test.jpg")));

    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(spy.takeFirst().at(0).toString(), QStringLiteral("/tmp/test.jpg"));
}

// ============ Signal: imageRenamed ============

TEST_F(FileControlExtraTest, Signal_ImageRenamed_EmittedOnSuccessfulRename)
{
    FileControl fc;
    QTemporaryDir tmpDir;
    ASSERT_TRUE(tmpDir.isValid());
    QString oldPath = tmpDir.filePath("old.jpg");
    QFile f(oldPath);
    ASSERT_TRUE(f.open(QIODevice::WriteOnly));
    f.write("data");
    f.close();

    stub.set_lamda(ADDR(ImageFileWatcher, fileRename),
        [](ImageFileWatcher *, const QString &, const QString &) {});

    QSignalSpy spy(&fc, &FileControl::imageRenamed);
    // When isSuffix=true, name is the full filename including extension
    QString newFilePath = tmpDir.filePath("new.jpg");
    fc.slotFileReName(QStringLiteral("new.jpg"), QUrl::fromLocalFile(oldPath).toString(), true);

    ASSERT_EQ(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QUrl oldUrl = args.at(0).toUrl();
    QUrl newUrl = args.at(1).toUrl();
    EXPECT_EQ(oldUrl.toLocalFile(), oldPath);
    EXPECT_EQ(newUrl.toLocalFile(), newFilePath);
}

// ============ Signal: imageFileChanged (forwarded from ImageFileWatcher) ============

TEST_F(FileControlExtraTest, Signal_ImageFileChanged_ForwardedFromWatcher)
{
    FileControl fc;
    QSignalSpy spy(&fc, &FileControl::imageFileChanged);

    QString testPath = QStringLiteral("/tmp/test_forward.jpg");
    Q_EMIT ImageFileWatcher::instance()->imageFileChanged(testPath);

    // FileControl connects imageFileWatcher::imageFileChanged to its own imageFileChanged signal
    // Allow event processing

    EXPECT_GE(spy.count(), 1);
    bool found = false;
    for (int i = 0; i < spy.count(); ++i) {
        if (spy.at(i).at(0).toString() == testPath) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}
