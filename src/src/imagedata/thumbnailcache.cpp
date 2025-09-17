// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumbnailcache.h"
#include <QDebug>

ThumbnailCache::ThumbnailCache()
{
    // 设置默认缓存为240
    cache.setMaxCost(240);
    qDebug() << "ThumbnailCache initialized with max cost:" << 240;
}

ThumbnailCache::~ThumbnailCache() 
{
    // qDebug() << "ThumbnailCache destroyed";
}

ThumbnailCache *ThumbnailCache::instance()
{
    // qDebug() << "ThumbnailCache::instance - Entry";
    static ThumbnailCache ins;
    return &ins;
}

/**
   @return 返回缓存中是否存在文件路径为 \a path 和图片帧索引为 \a frameIndex 的缩略图
 */
bool ThumbnailCache::contains(const QString &path, int frameIndex)
{
    // qDebug() << "ThumbnailCache::contains - Entry";
    QMutexLocker _locker(&mutex);
    bool result = cache.contains(toFindKey(path, frameIndex));
    qDebug() << "Checking thumbnail cache for path:" << path << "frame:" << frameIndex << "result:" << result;
    return result;
}

/**
   @return 返回缓存中文件路径为 \a path 和图片帧索引为 \a frameIndex 的缩略图
    QImage内部使用引用计数降低拷贝次数
 */
QImage ThumbnailCache::get(const QString &path, int frameIndex)
{
    // qDebug() << "ThumbnailCache::get - Entry";
    QMutexLocker _locker(&mutex);
    QImage *image = cache.object(toFindKey(path, frameIndex));
    if (image) {
        qDebug() << "Retrieved thumbnail from cache for path:" << path << "frame:" << frameIndex;
        return *image;
    } else {
        qDebug() << "Thumbnail not found in cache for path:" << path << "frame:" << frameIndex;
        return QImage();
    }
}

/**
   @brief 添加文件路径为 \a path 和图片帧索引为 \a frameIndex 的缩略图
 */
void ThumbnailCache::add(const QString &path, int frameIndex, const QImage &image)
{
    // qDebug() << "ThumbnailCache::add - Entry";
    QMutexLocker _locker(&mutex);
    cache.insert(toFindKey(path, frameIndex), new QImage(image));
    qDebug() << "Added thumbnail to cache for path:" << path << "frame:" << frameIndex << "size:" << image.size();
}

/**
   @brief 移除文件路径为 \a path 和图片帧索引为 \a frameIndex 的缩略图
 */
void ThumbnailCache::remove(const QString &path, int frameIndex)
{
    // qDebug() << "ThumbnailCache::remove - Entry";
    QMutexLocker _locker(&mutex);
    cache.remove(toFindKey(path, frameIndex));
    qDebug() << "Removed thumbnail from cache for path:" << path << "frame:" << frameIndex;
}

/**
   @brief 设置当前缓存的最大容量为 \a maxCost
 */
void ThumbnailCache::setMaxCost(int maxCost)
{
    // qDebug() << "ThumbnailCache::setMaxCost - Entry";
    QMutexLocker _locker(&mutex);
    cache.setMaxCost(maxCost);
    qDebug() << "Set thumbnail cache max cost to:" << maxCost;
}

/**
   @brief 清空缩略图信息
 */
void ThumbnailCache::clear()
{
    // qDebug() << "ThumbnailCache::clear - Entry";
    QMutexLocker _locker(&mutex);
    cache.clear();
    qDebug() << "Cleared all thumbnails from cache";
}

/**
   @return 返回图片的
 */
QList<ThumbnailCache::Key> ThumbnailCache::keys()
{
    // qDebug() << "ThumbnailCache::keys - Entry";
    return cache.keys();
}

/**
   @return 组合图像文件路径 \a path 和图像帧索引 \a frameIndex 为缩略图缓存处理的 key
 */
ThumbnailCache::Key ThumbnailCache::toFindKey(const QString &path, int frameIndex)
{
    // qDebug() << "ThumbnailCache::toFindKey - Entry";
    return qMakePair(path, frameIndex);
}
