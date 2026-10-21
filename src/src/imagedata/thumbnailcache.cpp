// SPDX-FileCopyrightText: 2023-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumbnailcache.h"
#include "ddlog.h"
#include <QDebug>

ThumbnailCache::ThumbnailCache()
{
    // 设置默认缓存为240
    cache.setMaxCost(240);
    qCDebug(albumLog) << "ThumbnailCache initialized with max cost:" << 240;
}

ThumbnailCache::~ThumbnailCache() 
{
    // qCDebug(albumLog) << "ThumbnailCache destroyed";
}

ThumbnailCache *ThumbnailCache::instance()
{
    // qCDebug(albumLog) << "ThumbnailCache::instance - Entry";
    static ThumbnailCache ins;
    return &ins;
}

/**
   @return 返回缓存中是否存在文件路径为 \a path 和图片帧索引为 \a frameIndex 的缩略图
 */
bool ThumbnailCache::contains(const QString &path, int frameIndex)
{
    // qCDebug(albumLog) << "ThumbnailCache::contains - Entry";
    QMutexLocker _locker(&mutex);
    bool result = cache.contains(toFindKey(path, frameIndex));
    qCDebug(albumLog) << "Checking thumbnail cache for path:" << path << "frame:" << frameIndex << "result:" << result;
    return result;
}

/**
   @return 返回缓存中文件路径为 \a path 和图片帧索引为 \a frameIndex 的缩略图
    QImage内部使用引用计数降低拷贝次数
 */
QImage ThumbnailCache::get(const QString &path, int frameIndex)
{
    // qCDebug(albumLog) << "ThumbnailCache::get - Entry";
    QMutexLocker _locker(&mutex);
    QImage *image = cache.object(toFindKey(path, frameIndex));
    if (image) {
        qCDebug(albumLog) << "Retrieved thumbnail from cache for path:" << path << "frame:" << frameIndex;
        return *image;
    } else {
        qCDebug(albumLog) << "Thumbnail not found in cache for path:" << path << "frame:" << frameIndex;
        return QImage();
    }
}

/**
   @brief 添加文件路径为 \a path 和图片帧索引为 \a frameIndex 的缩略图
 */
void ThumbnailCache::add(const QString &path, int frameIndex, const QImage &image)
{
    // qCDebug(albumLog) << "ThumbnailCache::add - Entry";
    QMutexLocker _locker(&mutex);
    cache.insert(toFindKey(path, frameIndex), new QImage(image));
    qCDebug(albumLog) << "Added thumbnail to cache for path:" << path << "frame:" << frameIndex << "size:" << image.size();
}

/**
   @brief 移除文件路径为 \a path 和图片帧索引为 \a frameIndex 的缩略图
 */
void ThumbnailCache::remove(const QString &path, int frameIndex)
{
    // qCDebug(albumLog) << "ThumbnailCache::remove - Entry";
    QMutexLocker _locker(&mutex);
    cache.remove(toFindKey(path, frameIndex));
    qCDebug(albumLog) << "Removed thumbnail from cache for path:" << path << "frame:" << frameIndex;
}

/**
   @brief 设置当前缓存的最大容量为 \a maxCost
 */
void ThumbnailCache::setMaxCost(int maxCost)
{
    // qCDebug(albumLog) << "ThumbnailCache::setMaxCost - Entry";
    QMutexLocker _locker(&mutex);
    cache.setMaxCost(maxCost);
    qCDebug(albumLog) << "Set thumbnail cache max cost to:" << maxCost;
}

/**
   @brief 清空缩略图信息
 */
void ThumbnailCache::clear()
{
    // qCDebug(albumLog) << "ThumbnailCache::clear - Entry";
    QMutexLocker _locker(&mutex);
    cache.clear();
    qCDebug(albumLog) << "Cleared all thumbnails from cache";
}

/**
   @return 返回图片的
 */
QList<ThumbnailCache::Key> ThumbnailCache::keys()
{
    // qCDebug(albumLog) << "ThumbnailCache::keys - Entry";
    return cache.keys();
}

/**
   @return 组合图像文件路径 \a path 和图像帧索引 \a frameIndex 为缩略图缓存处理的 key
 */
ThumbnailCache::Key ThumbnailCache::toFindKey(const QString &path, int frameIndex)
{
    // qCDebug(albumLog) << "ThumbnailCache::toFindKey - Entry";
    return qMakePair(path, frameIndex);
}
