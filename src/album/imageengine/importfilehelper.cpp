// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "importfilehelper.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QUuid>

namespace {
QString canonicalPath(const QString &path)
{
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    return canonical.isEmpty() ? QDir::cleanPath(info.absoluteFilePath()) : canonical;
}

bool isMountedPath(const QString &path, const QStringList &mountPaths)
{
    const QString canonical = canonicalPath(path);
    for (const QString &mountPath : mountPaths) {
        if (mountPath.isEmpty())
            continue;
        const QString root = canonicalPath(mountPath);
        if (root != QDir::rootPath() && (canonical == root || canonical.startsWith(root + '/')))
            return true;
    }
    return false;
}

bool hasDigest(const QString &path, const QByteArray &digest)
{
    QFile file(path);
    QCryptographicHash hash(QCryptographicHash::Sha256);
    return file.open(QIODevice::ReadOnly) && hash.addData(&file)
            && file.error() == QFileDevice::NoError && hash.result() == digest;
}

bool canReuse(const QString &path, const QByteArray &digest, const QStringList &mountPaths)
{
    const QFileInfo info(path);
    return !info.isSymLink() && info.isFile() && !isMountedPath(path, mountPaths)
            && hasDigest(path, digest);
}

QString shortenedBaseName(QString name, int byteLimit)
{
    while (!name.isEmpty() && QFile::encodeName(name).size() > byteLimit) {
        const bool surrogatePair = name.size() > 1 && name.at(name.size() - 1).isLowSurrogate()
                && name.at(name.size() - 2).isHighSurrogate();
        name.chop(surrogatePair ? 2 : 1);
    }
    return name;
}
}

QString ImportFileHelper::prepareFile(const QString &sourcePath, const QStringList &mountPaths,
                                     const QString &destinationDir, const QStringList &importedPaths)
{
    const QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.isFile())
        return QString();

    if (!isMountedPath(sourcePath, mountPaths))
        return sourcePath;

    QFile input(sourcePath);
    if (!input.open(QIODevice::ReadOnly) || !QDir().mkpath(destinationDir)
            || isMountedPath(destinationDir, mountPaths))
        return QString();

    QTemporaryFile copy(QDir(destinationDir).filePath(".album-import-XXXXXX"));
    if (!copy.open())
        return QString();

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!input.atEnd()) {
        const QByteArray data = input.read(4 * 1024 * 1024);
        if (input.error() != QFileDevice::NoError || data.isEmpty()
                || copy.write(data) != data.size())
            return QString();
        hash.addData(data);
    }
    if (!copy.flush())
        return QString();
    copy.close();

    const QByteArray digest = hash.result();
    const QString suffix = sourceInfo.suffix().isEmpty() ? QString() : '.' + sourceInfo.suffix();
    const QString hashPart = '-' + QString::fromLatin1(digest.toHex());
    // Reserve room for a conflict UUID within the usual 255-byte filename limit.
    const int baseNameLimit = 255 - QFile::encodeName(hashPart + suffix).size() - 37;
    if (baseNameLimit < 0)
        return QString();
    const QString stem = shortenedBaseName(sourceInfo.completeBaseName(), baseNameLimit) + hashPart;
    const QString fileName = stem + suffix;

    // Registered copies may live in a previous date's import directory.
    for (const QString &path : importedPaths) {
        const QFileInfo info(path);
        if ((info.fileName() == fileName || info.completeBaseName().startsWith(stem + '-'))
                && info.suffix() == sourceInfo.suffix() && canReuse(path, digest, mountPaths))
            return path;
    }

    QString destination = QDir(destinationDir).filePath(fileName);
    for (;;) {
        const QFileInfo occupied(destination);
        // QFile::rename can fall back to copying through a dangling symlink.
        if (!occupied.isSymLink() && QFile::rename(copy.fileName(), destination))
            break;
        if (canReuse(destination, digest, mountPaths))
            return destination;
        if (!occupied.exists() && !occupied.isSymLink())
            return QString();
        destination = QDir(destinationDir).filePath(stem + '-'
                      + QUuid::createUuid().toString(QUuid::WithoutBraces) + suffix);
    }
    copy.setAutoRemove(false);
    return destination;
}
