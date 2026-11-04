// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | ApplicationAdaptor::ApplicationAdaptor | high | 2 | 1 | 2 |
 * | ApplicationAdaptor::openImageFile      | high | 6 | 4 | 6 |
 *
 * 分支列表 (Constructor):
 *   B1: Valid FileControl* → QDBusAbstractAdaptor constructed, fileControl non-null
 *   B2: Constructor does not crash with valid parent
 *
 * 分支列表 (openImageFile):
 *   B1: fileControl valid, isCanReadable returns true → emit openImageFile(urlPath), return true
 *   B2: fileControl valid, isCanReadable returns false → return false, no signal emitted
 *   B3: Empty string fileName → URL-encoded empty path, isCanReadable returns false → return false
 *   B4: Path with spaces and Chinese chars → QUrl::fromLocalFile encoding, isCanReadable true → emit signal
 *   B5: Non-existent path, isCanReadable returns false → return false
 *   B6: Verify urlPath equals QUrl::fromLocalFile(fileName).toString()
 */

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QUrl>
#include <QString>
#include <QTemporaryFile>

#include "dbus/applicationadpator.h"
#include "filecontrol.h"
#include "unionimage/unionimage.h"
#include "stubext.h"

using namespace stub_ext;

class ApplicationAdaptorTest : public ::testing::Test
{
protected:
    StubExt stub;

    FileControl *fileControl = nullptr;
    bool isCanReadableResult = true;

    void SetUp() override
    {
        isCanReadableResult = true;

        // Stub FileControl::isCanReadable to control return value
        stub.set_lamda(ADDR(FileControl, isCanReadable),
            [this](FileControl *, const QString &) -> bool { return isCanReadableResult; });

        // Stub LibUnionImage_NameSpace::localPath to avoid real file access
        stub.set_lamda(ADDR(LibUnionImage_NameSpace, localPath),
            [](const QUrl &url) -> QString { return url.toLocalFile(); });

        // Create a real FileControl (its constructor creates OcrInterface, etc.
        // but those are DBus proxies that won't crash in test env)
        fileControl = new FileControl();
    }

    void TearDown() override
    {
        stub.clear();
        delete fileControl;
        fileControl = nullptr;
    }
};

// ============ Constructor ============

// B1: Valid FileControl* → QDBusAbstractAdaptor constructed, fileControl non-null
TEST_F(ApplicationAdaptorTest, Constructor_ValidFileControl_FileControlSet)
{
    ApplicationAdaptor adaptor(fileControl);
    // The adaptor should be constructed without crashing.
    // fileControl pointer is stored internally; verify by calling openImageFile.
    isCanReadableResult = true;
    QSignalSpy spy(fileControl, &FileControl::openImageFile);
    bool result = adaptor.openImageFile("/tmp/test.png");
    EXPECT_TRUE(result);
    EXPECT_EQ(spy.count(), 1);
}

// B2: Constructor does not crash with valid parent
TEST_F(ApplicationAdaptorTest, Constructor_ValidParent_NoCrash)
{
    ApplicationAdaptor adaptor(fileControl);
    // If we got here without crashing, the constructor succeeded
    SUCCEED();
}

// ============ openImageFile ============

// B1: fileControl valid, isCanReadable returns true → emit openImageFile(urlPath), return true
TEST_F(ApplicationAdaptorTest, OpenImageFile_CanReadable_EmitsSignal_ReturnsTrue)
{
    ApplicationAdaptor adaptor(fileControl);
    isCanReadableResult = true;

    QSignalSpy spy(fileControl, &FileControl::openImageFile);
    bool result = adaptor.openImageFile("/tmp/test_image.png");

    EXPECT_TRUE(result);
    EXPECT_EQ(spy.count(), 1);

    // Verify the emitted URL matches QUrl::fromLocalFile
    QString expectedUrl = QUrl::fromLocalFile("/tmp/test_image.png").toString();
    QList<QVariant> args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), expectedUrl);
}

// B2: fileControl valid, isCanReadable returns false → return false, no signal emitted
TEST_F(ApplicationAdaptorTest, OpenImageFile_CannotReadable_NoSignal_ReturnsFalse)
{
    ApplicationAdaptor adaptor(fileControl);
    isCanReadableResult = false;

    QSignalSpy spy(fileControl, &FileControl::openImageFile);
    bool result = adaptor.openImageFile("/tmp/test_image.png");

    EXPECT_FALSE(result);
    EXPECT_EQ(spy.count(), 0);
}

// B3: Empty string fileName → isCanReadable returns false → return false
TEST_F(ApplicationAdaptorTest, OpenImageFile_EmptyString_ReturnsFalse)
{
    ApplicationAdaptor adaptor(fileControl);
    isCanReadableResult = false;

    QSignalSpy spy(fileControl, &FileControl::openImageFile);
    bool result = adaptor.openImageFile("");

    EXPECT_FALSE(result);
    EXPECT_EQ(spy.count(), 0);
}

// B4: Path with spaces and Chinese chars → QUrl::fromLocalFile encoding, isCanReadable true → emit signal
TEST_F(ApplicationAdaptorTest, OpenImageFile_SpacesAndChinese_EmitsSignal)
{
    ApplicationAdaptor adaptor(fileControl);
    isCanReadableResult = true;

    QString path = "/tmp/测试 目录/图片 file.png";
    QSignalSpy spy(fileControl, &FileControl::openImageFile);
    bool result = adaptor.openImageFile(path);

    EXPECT_TRUE(result);
    EXPECT_EQ(spy.count(), 1);

    // Verify the emitted URL matches QUrl::fromLocalFile encoding
    QString expectedUrl = QUrl::fromLocalFile(path).toString();
    QList<QVariant> args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), expectedUrl);
}

// B5: Non-existent path, isCanReadable returns false → return false
TEST_F(ApplicationAdaptorTest, OpenImageFile_NonExistentPath_ReturnsFalse)
{
    ApplicationAdaptor adaptor(fileControl);
    isCanReadableResult = false;

    QSignalSpy spy(fileControl, &FileControl::openImageFile);
    bool result = adaptor.openImageFile("/nonexistent/path/to/image.png");

    EXPECT_FALSE(result);
    EXPECT_EQ(spy.count(), 0);
}

// B6: Verify urlPath equals QUrl::fromLocalFile(fileName).toString()
TEST_F(ApplicationAdaptorTest, OpenImageFile_UrlPath_MatchesQUrlFromLocalFile)
{
    ApplicationAdaptor adaptor(fileControl);
    isCanReadableResult = true;

    QString fileName = "/home/user/photos/vacation.jpg";
    QSignalSpy spy(fileControl, &FileControl::openImageFile);

    adaptor.openImageFile(fileName);

    QString expectedUrl = QUrl::fromLocalFile(fileName).toString();
    QList<QVariant> args = spy.takeFirst();
    EXPECT_EQ(args.at(0).toString(), expectedUrl);
}
