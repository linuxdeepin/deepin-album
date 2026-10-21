// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imagefilewatcher.h"
#include "ddlog.h"
#include "imageinfo.h"

#include <QDir>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QDebug>

ImageFileWatcher::ImageFileWatcher(QObject *parent)
    : QObject(parent)
    , fileWatcher(new QFileSystemWatcher(this))
{
    qCDebug(albumLog) << "Initializing ImageFileWatcher";
    connect(fileWatcher, &QFileSystemWatcher::fileChanged, this, &ImageFileWatcher::onImageFileChanged);
    connect(fileWatcher, &QFileSystemWatcher::directoryChanged, this, &ImageFileWatcher::onImageDirChanged);
}

ImageFileWatcher::~ImageFileWatcher() 
{
    qCDebug(albumLog) << "Cleaning up ImageFileWatcher";
}

ImageFileWatcher *ImageFileWatcher::instance()
{
    static ImageFileWatcher ins;
    return &ins;
}

/**
   @brief 重置监控文件列表为 \a filePaths , 若监控的文件路重复，则不执行重置
 */
void ImageFileWatcher::resetImageFiles(const QStringList &filePaths)
{
    qCDebug(albumLog) << "Resetting image files watch list, count:" << filePaths.size();
    
    // 重置时清理缓存记录
    cacheFileInfo.clear();
    removedFile.clear();
    rotateImagePathSet.clear();

    if (filePaths.isEmpty()) {
        qCInfo(albumLog) << "Clearing all watched files and directories";
        auto files = fileWatcher->files();
        if (!files.isEmpty()) {
            qCDebug(albumLog) << "Removing" << files.size() << "watched files";
            fileWatcher->removePaths(fileWatcher->files());
        }
        auto directories = fileWatcher->directories();
        if (!directories.isEmpty()) {
            qCDebug(albumLog) << "Removing" << directories.size() << "watched directories";
            fileWatcher->removePaths(fileWatcher->directories());
        }
        return;
    }

    // 目前仅会处理单个文件夹路径，重复追加将忽略
    if (isCurrentDir(filePaths.first())) {
        qCDebug(albumLog) << "Directory already being watched:" << filePaths.first();
        return;
    }

    qCDebug(albumLog) << "Removing existing watched paths";
    fileWatcher->removePaths(fileWatcher->files());
    fileWatcher->removePaths(fileWatcher->directories());

    for (const QString &filePath : filePaths) {
        QString tempPath = QUrl(filePath).toLocalFile();
        QFileInfo info(tempPath);
        // 若文件存在
        if (info.exists()) {
            qCDebug(albumLog) << "Adding file to watch:" << tempPath;
            // 记录文件的最后修改时间
            cacheFileInfo.insert(tempPath, filePath);
            // 将文件追加到记录中
            fileWatcher->addPath(tempPath);
        } else {
            qCWarning(albumLog) << "File does not exist, skipping:" << tempPath;
        }
    }

    QStringList fileList = fileWatcher->files();
    if (!fileList.isEmpty()) {
        // 观察文件夹变更
        QFileInfo info(fileList.first());
        QString dirPath = info.absolutePath();
        qCInfo(albumLog) << "Adding directory to watch:" << dirPath;
        fileWatcher->addPath(dirPath);
    }
}

/**
   @brief 监控的文件 \a oldPath 重命名为 \a newPath 更新监控列表
 */
void ImageFileWatcher::fileRename(const QString &oldPath, const QString &newPath)
{
    qCInfo(albumLog) << "File renamed from:" << oldPath << "to:" << newPath;
    if (cacheFileInfo.contains(oldPath)) {
        qCDebug(albumLog) << "Updating watch for renamed file";
        fileWatcher->removePath(oldPath);

        cacheFileInfo.insert(newPath, newPath);
        fileWatcher->addPath(newPath);
    } else {
        qCDebug(albumLog) << "Renamed file was not being watched:" << oldPath;
    }
}

/**
   @return 返回文件路径 \a filePath 所在的文件夹是否为当前监控的文件夹
 */
bool ImageFileWatcher::isCurrentDir(const QString &filePath)
{
    QString dir = QFileInfo(filePath).absolutePath();
    bool isWatched = fileWatcher->directories().contains(dir);
    qCDebug(albumLog) << "Checking if directory is watched:" << dir << "result:" << isWatched;
    return isWatched;
}

/**
   @brief 设置当前旋转的图片文件路径为 \a targetPath ，在执行旋转操作前调用，
    旋转覆写文件时将不会发送变更信号(文件的旋转状态已在缓存中记录)
    文件旋转操作现移至子线程处理，和文件更新信号到达时间不定，因此在拷贝前记录操作文件，
    若旋转操作成功，通过文件更新处理重置；旋转操作失败，通过 cleareRotateStatus() 重置
 */
void ImageFileWatcher::recordRotateImage(const QString &targetPath)
{
    qCDebug(albumLog) << "Recording rotate operation for:" << targetPath;
    rotateImagePathSet.insert(targetPath);
}

/**
   @brief 若 \a targetPath 和当前缓存的旋转记录文件一致，则清除记录。
 */
void ImageFileWatcher::clearRotateStatus(const QString &targetPath)
{
    if (rotateImagePathSet.contains(targetPath)) {
        qCDebug(albumLog) << "Clearing rotate status for:" << targetPath;
        rotateImagePathSet.remove(targetPath);
    } else {
        qCDebug(albumLog) << "No rotate status found for:" << targetPath;
    }
}

/**
   @brief 监控的文件路\a file 变更
 */
void ImageFileWatcher::onImageFileChanged(const QString &file)
{
    qCDebug(albumLog) << "File changed:" << file;
    
    // 若为当前旋转处理的文件，不触发更新，状态在图像缓存中已同步
    if (rotateImagePathSet.contains(file)) {
        qCDebug(albumLog) << "File is being rotated, skipping update:" << file;
        return;
    }

    // 文件移动、删除或替换后触发，旋转操作时不触发更新，使用缓存中的旋转图像
    if (cacheFileInfo.contains(file)) {
        QUrl url = cacheFileInfo.value(file);
        bool isExist = QFile::exists(file);
        // 文件移动或删除，缓存记录
        if (!isExist) {
            qCWarning(albumLog) << "File no longer exists, caching for recovery:" << file;
            removedFile.insert(file, url);
        } else {
            qCDebug(albumLog) << "File exists, processing change:" << file;
        }

        Q_EMIT imageFileChanged(file);

        // 请求重新加载缓存，外部使用 ImageInfo 获取文件状态变更，使用 clearCurrentCache() 清理多页图缓存信息
        ImageInfo info;
        info.setSource(url);
        info.clearCurrentCache();
        info.reloadData();
    } else {
        qCDebug(albumLog) << "Changed file was not being watched:" << file;
    }
}

/**
   @brief 监控的文件路径 \a dir 变更
 */
void ImageFileWatcher::onImageDirChanged(const QString &dir)
{
    qCDebug(albumLog) << "Directory changed:" << dir;
    
    // 文件夹变更，判断是否存在新增已移除的文件
    QDir imageDir(dir);
    QStringList dirFiles = imageDir.entryList();
    qCDebug(albumLog) << "Directory contents changed, current files:" << dirFiles;

    for (auto itr = removedFile.begin(); itr != removedFile.end();) {
        QFileInfo info(itr.key());
        if (dirFiles.contains(info.fileName())) {
            qCInfo(albumLog) << "Recovered file found:" << itr.key();
            // 重新追加到文件观察中
            fileWatcher->addPath(itr.key());
            // 文件恢复或替换，发布文件变更信息
            onImageFileChanged(itr.key());

            // 从缓存信息中移除
            itr = removedFile.erase(itr);
        } else {
            qCDebug(albumLog) << "File still not found in directory:" << itr.key();
            ++itr;
        }
    }
}
