// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "importfilehelper.h"

namespace {

class ImportFileHelperTest : public testing::Test
{
protected:
    void SetUp() override
    {
        ASSERT_TRUE(temporaryDir.isValid());
        mountDir = temporaryDir.path() + "/usb";
        destinationDir = temporaryDir.path() + "/album/2026-11-24";
        ASSERT_TRUE(QDir().mkpath(mountDir));
    }

    QString writeFile(const QString &relativePath, const QByteArray &contents)
    {
        const QString path = temporaryDir.path() + '/' + relativePath;
        EXPECT_TRUE(QDir().mkpath(QFileInfo(path).absolutePath()));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            ADD_FAILURE() << "Cannot write temporary fixture file";
            return {};
        }
        EXPECT_EQ(contents.size(), file.write(contents));
        file.close();
        return path;
    }

    QByteArray readFile(const QString &path)
    {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            ADD_FAILURE() << "Cannot read result file: " << path.toStdString();
            return {};
        }
        return file.readAll();
    }

    QString expectedName(const QString &source, const QByteArray &contents)
    {
        const QFileInfo info(source);
        return info.completeBaseName() + '-'
                + QString::fromLatin1(QCryptographicHash::hash(contents,
                                               QCryptographicHash::Sha256).toHex())
                + '.' + info.suffix();
    }

    QString prepare(const QString &source, const QStringList &imported = {})
    {
        return ImportFileHelper::prepareFile(source, {mountDir}, destinationDir, imported);
    }

    void expectCopy(const QString &result, const QString &source, const QByteArray &contents)
    {
        ASSERT_FALSE(result.isEmpty());
        EXPECT_NE(source, result);
        EXPECT_EQ(QDir(destinationDir).absolutePath(), QFileInfo(result).absolutePath());
        EXPECT_EQ(expectedName(source, contents), QFileInfo(result).fileName());
        EXPECT_EQ(contents, readFile(result));
        EXPECT_EQ(contents, readFile(source));
    }

    QTemporaryDir temporaryDir {QStringLiteral("/tmp/album-importfilehelper-test-XXXXXX")};
    QString mountDir;
    QString destinationDir;
};

TEST_F(ImportFileHelperTest, MountedSubdirectoryIsCopiedWithSha256Name)
{
    const QByteArray contents("photo\0binary\xff", 13);
    const QString source = writeFile("usb/DCIM/camera/photo.jpg", contents);
    expectCopy(prepare(source), source, contents);
}

TEST_F(ImportFileHelperTest, SimilarPrefixIsNotAMountMatch)
{
    const QString source = writeFile("usb-backup/photo.jpg", "local");
    EXPECT_EQ(source, prepare(source));
    EXPECT_FALSE(QFileInfo::exists(destinationDir));
}

TEST_F(ImportFileHelperTest, MountWithTrailingSlashesMatches)
{
    const QByteArray contents("external");
    const QString source = writeFile("usb/DCIM/photo.jpg", contents);
    expectCopy(ImportFileHelper::prepareFile(source, {mountDir + "///"},
                                           destinationDir, {}), source, contents);
}

TEST_F(ImportFileHelperTest, RootAndEmptyMountEntriesAreIgnored)
{
    const QString source = writeFile("local/photo.jpg", "local");
    EXPECT_EQ(source, ImportFileHelper::prepareFile(source, {"", "/", "///"},
                                                  destinationDir, {}));
    EXPECT_FALSE(QFileInfo::exists(destinationDir));
}

TEST_F(ImportFileHelperTest, LaterMountEntryStillMatches)
{
    const QByteArray contents("external");
    const QString source = writeFile("usb/photo.jpg", contents);
    expectCopy(ImportFileHelper::prepareFile(source,
                                           {"", "/", temporaryDir.path() + "/other", mountDir},
                                           destinationDir, {}), source, contents);
}

TEST_F(ImportFileHelperTest, CanonicalMountPathResolvesDotComponents)
{
    ASSERT_TRUE(QDir().mkpath(mountDir + "/DCIM"));
    const QByteArray contents("canonical");
    const QString source = writeFile("usb/photo.jpg", contents);
    expectCopy(ImportFileHelper::prepareFile(source, {mountDir + "/DCIM/.."},
                                           destinationDir, {}), source, contents);
}

TEST_F(ImportFileHelperTest, LocalThenExternalCallsDoNotShareMountState)
{
    const QString local = writeFile("local/photo.jpg", "local");
    const QByteArray contents("external");
    const QString external = writeFile("usb/photo.jpg", contents);
    EXPECT_EQ(local, prepare(local));
    expectCopy(prepare(external), external, contents);
    EXPECT_EQ(local, prepare(local));
}

TEST_F(ImportFileHelperTest, ExternalThenLocalCallsDoNotShareMountState)
{
    const QString local = writeFile("local/photo.jpg", "local");
    const QByteArray contents("external");
    const QString external = writeFile("usb/photo.jpg", contents);
    expectCopy(prepare(external), external, contents);
    EXPECT_EQ(local, prepare(local));
    expectCopy(prepare(external), external, contents);
}

TEST_F(ImportFileHelperTest, RepeatedSourceReusesExistingDestination)
{
    const QString source = writeFile("usb/photo.jpg", "same photo");
    const QString first = prepare(source);
    ASSERT_FALSE(first.isEmpty());
    EXPECT_EQ(first, prepare(source));
    EXPECT_EQ(first, prepare(source, {first}));
    EXPECT_EQ(1, QDir(destinationDir).entryList(QDir::Files | QDir::Hidden).size());
}

TEST_F(ImportFileHelperTest, SameContentDestinationIsReusedWithoutImportedList)
{
    const QByteArray contents("already copied");
    const QString source = writeFile("usb/photo.jpg", contents);
    const QString existing = writeFile("album/2026-11-24/" + expectedName(source, contents), contents);
    EXPECT_EQ(existing, prepare(source));
    EXPECT_EQ(contents, readFile(existing));
}

TEST_F(ImportFileHelperTest, ImportedFileFromPreviousDateIsReused)
{
    const QByteArray contents("previous date");
    const QString source = writeFile("usb/photo.jpg", contents);
    const QString previous = writeFile("album/2026-11-23/" + expectedName(source, contents), contents);
    EXPECT_EQ(previous, prepare(source, {previous}));
    EXPECT_EQ(contents, readFile(previous));
    EXPECT_TRUE(QDir(destinationDir).entryList(QDir::Files | QDir::Hidden).isEmpty());
}

TEST_F(ImportFileHelperTest, ImportedContentWithDifferentFilenameIsNotReused)
{
    const QByteArray contents("identical bytes but a different photo name");
    const QString source = writeFile("usb/photo.jpg", contents);
    const QString different = writeFile("album/2026-11-23/another.jpg", contents);
    const QString result = prepare(source, {different});
    EXPECT_NE(different, result);
    expectCopy(result, source, contents);
}

TEST_F(ImportFileHelperTest, SourceContentChangeCreatesNewCopy)
{
    const QString source = writeFile("usb/photo.jpg", "original");
    const QString first = prepare(source);
    ASSERT_FALSE(first.isEmpty());
    ASSERT_EQ(source, writeFile("usb/photo.jpg", "modified"));
    const QString second = prepare(source, {first});
    EXPECT_NE(first, second);
    expectCopy(second, source, "modified");
    EXPECT_EQ(QByteArray("original"), readFile(first));
}

TEST_F(ImportFileHelperTest, ModifiedDestinationIsNeitherReusedNorOverwritten)
{
    const QByteArray contents("original");
    const QString source = writeFile("usb/photo.jpg", contents);
    const QString occupied = writeFile("album/2026-11-24/" + expectedName(source, contents), "edited");
    const QString result = prepare(source, {occupied});
    ASSERT_FALSE(result.isEmpty());
    EXPECT_NE(occupied, result);
    EXPECT_EQ(QFileInfo(occupied).absolutePath(), QFileInfo(result).absolutePath());
    EXPECT_EQ(QByteArray("edited"), readFile(occupied));
    EXPECT_EQ(contents, readFile(result));
    EXPECT_EQ(contents, readFile(source));
    EXPECT_EQ(2, QDir(destinationDir).entryList(QDir::Files | QDir::Hidden).size());
    EXPECT_EQ(result, prepare(source, {result}));
    EXPECT_EQ(2, QDir(destinationDir).entryList(QDir::Files | QDir::Hidden).size());
}

TEST_F(ImportFileHelperTest, ModifiedPreviousDateFileIsNotReused)
{
    const QByteArray contents("original");
    const QString source = writeFile("usb/photo.jpg", contents);
    const QString previous = writeFile("album/2026-11-23/" + expectedName(source, contents), "edited");
    const QString result = prepare(source, {previous});
    EXPECT_NE(previous, result);
    expectCopy(result, source, contents);
    EXPECT_EQ(QByteArray("edited"), readFile(previous));
}

TEST_F(ImportFileHelperTest, MissingImportedCandidateDoesNotPreventCopy)
{
    const QByteArray contents("original");
    const QString source = writeFile("usb/photo.jpg", contents);
    const QString missing = temporaryDir.path() + "/old/" + expectedName(source, contents);
    expectCopy(prepare(source, {missing}), source, contents);
}

TEST_F(ImportFileHelperTest, MultiDotFilenamePreservesCompleteBaseName)
{
    const QByteArray contents("multi-dot");
    const QString source = writeFile("usb/holiday.edited.final.jpg", contents);
    const QString result = prepare(source);
    expectCopy(result, source, contents);
    EXPECT_TRUE(QFileInfo(result).fileName().startsWith("holiday.edited.final-"));
    EXPECT_TRUE(result.endsWith(".jpg"));
}

TEST_F(ImportFileHelperTest, MissingMountedSourceFails)
{
    EXPECT_TRUE(prepare(mountDir + "/missing.jpg").isEmpty());
}

TEST_F(ImportFileHelperTest, DirectorySourceFails)
{
    ASSERT_TRUE(QDir().mkpath(mountDir + "/DCIM"));
    EXPECT_TRUE(prepare(mountDir + "/DCIM").isEmpty());
}

TEST_F(ImportFileHelperTest, DestinationThatIsARegularFileFailsWithoutChangingIt)
{
    const QString source = writeFile("usb/photo.jpg", "external");
    const QString destinationFile = writeFile("not-a-directory", "keep me");
    EXPECT_TRUE(ImportFileHelper::prepareFile(source, {mountDir}, destinationFile, {}).isEmpty());
    EXPECT_EQ(QByteArray("keep me"), readFile(destinationFile));
    EXPECT_EQ(QByteArray("external"), readFile(source));
}

TEST_F(ImportFileHelperTest, EmptyMountedFileIsCopiedAndReused)
{
    const QString source = writeFile("usb/empty.jpg", {});
    const QString first = prepare(source);
    expectCopy(first, source, {});
    EXPECT_EQ(first, prepare(source));
}

TEST_F(ImportFileHelperTest, LargeBinaryFileIsCopiedCompletely)
{
    QByteArray contents(9 * 1024 * 1024 + 17, '\0');
    for (int i = 0; i < contents.size(); ++i)
        contents[i] = static_cast<char>(i % 251);
    const QString source = writeFile("usb/large.jpg", contents);
    expectCopy(prepare(source), source, contents);
}

TEST_F(ImportFileHelperTest, LocalSourceIsReturnedVerbatimWithoutCreatingDestination)
{
    writeFile("local/photo.jpg", "local");
    const QString source = temporaryDir.path() + "/local/../local/photo.jpg";
    EXPECT_EQ(source, prepare(source));
    EXPECT_FALSE(QFileInfo::exists(destinationDir));
}

TEST_F(ImportFileHelperTest, EmptyMountListLeavesLocalSourceUntouched)
{
    const QString source = writeFile("local/photo.jpg", "local");
    EXPECT_EQ(source, ImportFileHelper::prepareFile(source, {}, destinationDir, {}));
    EXPECT_FALSE(QFileInfo::exists(destinationDir));
}

TEST_F(ImportFileHelperTest, SymlinkToMountedFileUsesCanonicalMountMembership)
{
    const QByteArray contents("external through symlink");
    const QString source = writeFile("usb/photo.jpg", contents);
    const QString link = temporaryDir.path() + "/photo.jpg";
    ASSERT_TRUE(QFile::link(source, link));
    expectCopy(prepare(link), link, contents);
}

TEST_F(ImportFileHelperTest, SymlinkOutsideMountIsNotCopiedJustBecauseLinkIsInside)
{
    const QString source = writeFile("local/photo.jpg", "local through symlink");
    const QString link = mountDir + "/photo.jpg";
    ASSERT_TRUE(QFile::link(source, link));
    EXPECT_EQ(link, prepare(link));
    EXPECT_FALSE(QFileInfo::exists(destinationDir));
}

TEST_F(ImportFileHelperTest, LongAsciiBasenameIsCopiedWithinByteLimitAndReused)
{
    const QByteArray contents("long ASCII photo\0complete\xff", 26);
    const QString basename(200, QLatin1Char('a'));
    const QString filename = basename + ".jpg";
    ASSERT_LE(QFile::encodeName(filename).size(), 255);
    const QString source = writeFile("usb/" + filename, contents);
    ASSERT_FALSE(source.isEmpty());

    const QString first = prepare(source);
    ASSERT_FALSE(first.isEmpty());
    EXPECT_NE(source, first);
    EXPECT_EQ(QDir(destinationDir).absolutePath(), QFileInfo(first).absolutePath());
    EXPECT_TRUE(QFileInfo(first).isFile());
    EXPECT_FALSE(QFileInfo(first).isSymLink());
    EXPECT_LE(QFile::encodeName(QFileInfo(first).fileName()).size(), 255);
    EXPECT_TRUE(first.endsWith(".jpg"));
    EXPECT_EQ(contents, readFile(first));
    EXPECT_EQ(contents, readFile(source));
    EXPECT_EQ(first, prepare(source));
    EXPECT_EQ(first, prepare(source, {first}));
    EXPECT_EQ(1, QDir(destinationDir).entryList(QDir::Files | QDir::Hidden).size());
}

TEST_F(ImportFileHelperTest, LongChineseBasenameIsCopiedWithinByteLimitAndReused)
{
    const QByteArray contents("long Chinese photo\0complete\xff", 28);
    const QString basename = QString::fromUtf8("图").repeated(70);
    const QString filename = basename + ".jpg";
    ASSERT_LE(QFile::encodeName(filename).size(), 255);
    const QString source = writeFile("usb/" + filename, contents);
    ASSERT_FALSE(source.isEmpty());

    const QString first = prepare(source);
    ASSERT_FALSE(first.isEmpty());
    EXPECT_NE(source, first);
    EXPECT_EQ(QDir(destinationDir).absolutePath(), QFileInfo(first).absolutePath());
    EXPECT_TRUE(QFileInfo(first).isFile());
    EXPECT_FALSE(QFileInfo(first).isSymLink());
    EXPECT_LE(QFile::encodeName(QFileInfo(first).fileName()).size(), 255);
    EXPECT_TRUE(first.endsWith(".jpg"));
    EXPECT_EQ(contents, readFile(first));
    EXPECT_EQ(contents, readFile(source));
    EXPECT_EQ(first, prepare(source));
    EXPECT_EQ(first, prepare(source, {first}));
    EXPECT_EQ(1, QDir(destinationDir).entryList(QDir::Files | QDir::Hidden).size());
}

TEST_F(ImportFileHelperTest, EditedLongFilenameDestinationIsPreservedAndNewCopyFitsByteLimit)
{
    const QByteArray contents("original long-name photo\0binary", 31);
    const QByteArray edited("user edited photo");
    const QStringList basenames {QString(200, QLatin1Char('a')),
                                 QString::fromUtf8("图").repeated(70)};
    for (const QString &basename : basenames) {
        SCOPED_TRACE(basename.toStdString());
        const QString source = writeFile("usb/" + basename + ".jpg", contents);
        ASSERT_FALSE(source.isEmpty());
        const QString occupied = prepare(source);
        ASSERT_FALSE(occupied.isEmpty());
        ASSERT_EQ(occupied, writeFile("album/2026-11-24/"
                                     + QFileInfo(occupied).fileName(), edited));

        const QString result = prepare(source, {occupied});
        ASSERT_FALSE(result.isEmpty());
        EXPECT_NE(source, result);
        EXPECT_NE(occupied, result);
        EXPECT_EQ(QDir(destinationDir).absolutePath(), QFileInfo(result).absolutePath());
        EXPECT_TRUE(QFileInfo(result).isFile());
        EXPECT_FALSE(QFileInfo(result).isSymLink());
        EXPECT_LE(QFile::encodeName(QFileInfo(occupied).fileName()).size(), 255);
        EXPECT_LE(QFile::encodeName(QFileInfo(result).fileName()).size(), 255);
        EXPECT_TRUE(result.endsWith(".jpg"));
        EXPECT_EQ(edited, readFile(occupied));
        EXPECT_EQ(contents, readFile(result));
        EXPECT_EQ(contents, readFile(source));
        EXPECT_EQ(result, prepare(source, {occupied, result}));
        EXPECT_EQ(edited, readFile(occupied));
    }
    EXPECT_EQ(4, QDir(destinationDir).entryList(QDir::Files | QDir::Hidden).size());
}

TEST_F(ImportFileHelperTest, ExpectedDestinationSymlinkToUsbIsNotReusedOrOverwritten)
{
    const QByteArray contents("external photo\0binary", 21);
    const QString source = writeFile("usb/photo.jpg", contents);
    ASSERT_TRUE(QDir().mkpath(destinationDir));
    const QString link = QDir(destinationDir).filePath(expectedName(source, contents));
    ASSERT_TRUE(QFile::link(source, link));
    ASSERT_TRUE(QFileInfo(link).isSymLink());
    const QString linkTarget = QFileInfo(link).symLinkTarget();

    const QString result = prepare(source);
    ASSERT_FALSE(result.isEmpty());
    EXPECT_NE(link, result);
    EXPECT_NE(source, result);
    EXPECT_EQ(QDir(destinationDir).absolutePath(), QFileInfo(result).absolutePath());
    EXPECT_TRUE(QFileInfo(result).isFile());
    EXPECT_FALSE(QFileInfo(result).isSymLink());
    EXPECT_FALSE(QFileInfo(result).canonicalFilePath().startsWith(mountDir + '/'));
    EXPECT_LE(QFile::encodeName(QFileInfo(result).fileName()).size(), 255);
    EXPECT_EQ(contents, readFile(result));
    EXPECT_TRUE(QFileInfo(link).isSymLink());
    EXPECT_EQ(linkTarget, QFileInfo(link).symLinkTarget());
    EXPECT_EQ(source, QFileInfo(link).canonicalFilePath());
    EXPECT_TRUE(QFileInfo(source).isFile());
    EXPECT_FALSE(QFileInfo(source).isSymLink());
    EXPECT_EQ(contents, readFile(link));
    EXPECT_EQ(contents, readFile(source));
    EXPECT_EQ(result, prepare(source, {link, result}));
}

TEST_F(ImportFileHelperTest, ImportedPreviousDateSymlinkToUsbIsNotReused)
{
    const QByteArray contents("previous date external photo");
    const QString source = writeFile("usb/photo.jpg", contents);
    const QString previousDir = temporaryDir.path() + "/album/2026-11-23";
    ASSERT_TRUE(QDir().mkpath(previousDir));
    const QString link = QDir(previousDir).filePath(expectedName(source, contents));
    ASSERT_TRUE(QFile::link(source, link));
    ASSERT_TRUE(QFileInfo(link).isSymLink());
    const QString linkTarget = QFileInfo(link).symLinkTarget();

    const QString result = prepare(source, {link});
    ASSERT_FALSE(result.isEmpty());
    EXPECT_NE(link, result);
    expectCopy(result, source, contents);
    EXPECT_TRUE(QFileInfo(result).isFile());
    EXPECT_FALSE(QFileInfo(result).isSymLink());
    EXPECT_FALSE(QFileInfo(result).canonicalFilePath().startsWith(mountDir + '/'));
    EXPECT_TRUE(QFileInfo(link).isSymLink());
    EXPECT_EQ(linkTarget, QFileInfo(link).symLinkTarget());
    EXPECT_EQ(source, QFileInfo(link).canonicalFilePath());
    EXPECT_EQ(contents, readFile(link));
    EXPECT_TRUE(QFileInfo(source).isFile());
    EXPECT_FALSE(QFileInfo(source).isSymLink());
    EXPECT_EQ(contents, readFile(source));
    EXPECT_EQ(result, prepare(source, {link, result}));
}

TEST_F(ImportFileHelperTest, DanglingSymlinkAtExpectedDestinationIsPreservedAndCopyUsesNewName)
{
    const QByteArray contents("external photo");
    const QString source = writeFile("usb/photo.jpg", contents);
    ASSERT_TRUE(QDir().mkpath(destinationDir));
    const QString missing = mountDir + "/missing.jpg";
    const QString link = QDir(destinationDir).filePath(expectedName(source, contents));
    ASSERT_TRUE(QFile::link(missing, link));
    ASSERT_TRUE(QFileInfo(link).isSymLink());
    ASSERT_FALSE(QFileInfo::exists(link));
    const QString linkTarget = QFileInfo(link).symLinkTarget();

    const QString result = prepare(source, {link});
    ASSERT_FALSE(result.isEmpty());
    EXPECT_NE(link, result);
    EXPECT_NE(source, result);
    EXPECT_EQ(QDir(destinationDir).absolutePath(), QFileInfo(result).absolutePath());
    EXPECT_TRUE(QFileInfo(result).isFile());
    EXPECT_FALSE(QFileInfo(result).isSymLink());
    EXPECT_LE(QFile::encodeName(QFileInfo(result).fileName()).size(), 255);
    EXPECT_EQ(contents, readFile(result));
    EXPECT_EQ(contents, readFile(source));
    EXPECT_TRUE(QFileInfo(link).isSymLink());
    EXPECT_FALSE(QFileInfo::exists(link));
    EXPECT_EQ(linkTarget, QFileInfo(link).symLinkTarget());
    EXPECT_FALSE(QFileInfo::exists(missing));
    EXPECT_EQ(result, prepare(source, {link, result}));
}

TEST_F(ImportFileHelperTest, PreviousDateCandidateUnderSymlinkedUsbDirectoryIsNotReused)
{
    const QByteArray contents("external photo through parent symlink");
    const QString source = writeFile("usb/photo.jpg", contents);
    const QString externalCopy = writeFile("usb/previous/"
                                           + expectedName(source, contents), contents);
    ASSERT_TRUE(QDir().mkpath(temporaryDir.path() + "/album"));
    const QString parentLink = temporaryDir.path() + "/album/2026-11-23";
    ASSERT_TRUE(QFile::link(mountDir + "/previous", parentLink));
    ASSERT_TRUE(QFileInfo(parentLink).isSymLink());
    const QString linkTarget = QFileInfo(parentLink).symLinkTarget();
    const QString candidate = QDir(parentLink).filePath(expectedName(source, contents));
    ASSERT_TRUE(QFileInfo(candidate).isFile());
    ASSERT_FALSE(QFileInfo(candidate).isSymLink());
    ASSERT_EQ(externalCopy, QFileInfo(candidate).canonicalFilePath());

    const QString result = prepare(source, {candidate});
    ASSERT_FALSE(result.isEmpty());
    EXPECT_NE(candidate, result);
    EXPECT_NE(externalCopy, result);
    expectCopy(result, source, contents);
    EXPECT_TRUE(QFileInfo(result).isFile());
    EXPECT_FALSE(QFileInfo(result).isSymLink());
    EXPECT_FALSE(QFileInfo(result).canonicalFilePath().startsWith(mountDir + '/'));
    EXPECT_TRUE(QFileInfo(parentLink).isSymLink());
    EXPECT_EQ(linkTarget, QFileInfo(parentLink).symLinkTarget());
    EXPECT_EQ(externalCopy, QFileInfo(candidate).canonicalFilePath());
    EXPECT_EQ(contents, readFile(candidate));
    EXPECT_EQ(contents, readFile(externalCopy));
    EXPECT_TRUE(QFileInfo(source).isFile());
    EXPECT_FALSE(QFileInfo(source).isSymLink());
    EXPECT_EQ(contents, readFile(source));
}

} // namespace
