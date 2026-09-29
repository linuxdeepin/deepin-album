// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QImage>
#include <QString>
#include <QList>
#include <QPair>

#include "imagedata/thumbnailcache.h"

class ThumbnailCacheTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        cache = ThumbnailCache::instance();
        ASSERT_NE(cache, nullptr);
        cache->clear();
    }

    void TearDown() override
    {
        cache->clear();
    }

    ThumbnailCache *cache = nullptr;

    static QImage makeImage(int width, int height)
    {
        QImage img(width, height, QImage::Format_ARGB32);
        img.fill(Qt::red);
        return img;
    }
};

// ---- add + get ----

TEST_F(ThumbnailCacheTest, AddAndGet_ReturnsSameImage)
{
    QImage img = makeImage(32, 32);
    cache->add("/test/photo.jpg", 0, img);

    QImage result = cache->get("/test/photo.jpg", 0);
    EXPECT_FALSE(result.isNull());
    EXPECT_EQ(result.size(), QSize(32, 32));
}

TEST_F(ThumbnailCacheTest, Add_WithDifferentFrameIndices)
{
    QImage img0 = makeImage(10, 10);
    QImage img1 = makeImage(20, 20);
    cache->add("/video.mp4", 0, img0);
    cache->add("/video.mp4", 1, img1);

    EXPECT_EQ(cache->get("/video.mp4", 0).size(), QSize(10, 10));
    EXPECT_EQ(cache->get("/video.mp4", 1).size(), QSize(20, 20));
}

TEST_F(ThumbnailCacheTest, Add_OverwritesExistingKey)
{
    QImage img1 = makeImage(16, 16);
    QImage img2 = makeImage(48, 48);
    cache->add("/path/img.png", 0, img1);
    cache->add("/path/img.png", 0, img2);

    QImage result = cache->get("/path/img.png", 0);
    EXPECT_EQ(result.size(), QSize(48, 48));
}

// ---- get ----

TEST_F(ThumbnailCacheTest, Get_NonExistentReturnsEmptyImage)
{
    QImage result = cache->get("/nonexistent.jpg", 0);
    EXPECT_TRUE(result.isNull());
}

TEST_F(ThumbnailCacheTest, Get_EmptyPathReturnsEmptyImage)
{
    QImage result = cache->get("", 0);
    EXPECT_TRUE(result.isNull());
}

TEST_F(ThumbnailCacheTest, Get_NonExistentFrameIndexReturnsEmpty)
{
    cache->add("/photo.jpg", 0, makeImage(10, 10));
    QImage result = cache->get("/photo.jpg", 99);
    EXPECT_TRUE(result.isNull());
}

// ---- contains ----

TEST_F(ThumbnailCacheTest, Contains_ReturnsTrueForExisting)
{
    cache->add("/photo.jpg", 0, makeImage(10, 10));
    EXPECT_TRUE(cache->contains("/photo.jpg", 0));
}

TEST_F(ThumbnailCacheTest, Contains_ReturnsFalseForNonExistent)
{
    EXPECT_FALSE(cache->contains("/nonexistent.jpg", 0));
}

TEST_F(ThumbnailCacheTest, Contains_ReturnsFalseForWrongFrameIndex)
{
    cache->add("/video.mp4", 0, makeImage(10, 10));
    EXPECT_TRUE(cache->contains("/video.mp4", 0));
    EXPECT_FALSE(cache->contains("/video.mp4", 5));
}

// ---- remove ----

TEST_F(ThumbnailCacheTest, Remove_RemovesExistingEntry)
{
    cache->add("/photo.jpg", 0, makeImage(10, 10));
    EXPECT_TRUE(cache->contains("/photo.jpg", 0));

    cache->remove("/photo.jpg", 0);
    EXPECT_FALSE(cache->contains("/photo.jpg", 0));
}

TEST_F(ThumbnailCacheTest, Remove_NonExistentDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(cache->remove("/nonexistent.jpg", 0));
}

TEST_F(ThumbnailCacheTest, Remove_OnlyRemovesSpecifiedFrameIndex)
{
    cache->add("/video.mp4", 0, makeImage(10, 10));
    cache->add("/video.mp4", 1, makeImage(20, 20));

    cache->remove("/video.mp4", 0);
    EXPECT_FALSE(cache->contains("/video.mp4", 0));
    EXPECT_TRUE(cache->contains("/video.mp4", 1));
}

// ---- clear ----

TEST_F(ThumbnailCacheTest, Clear_RemovesAllEntries)
{
    cache->add("/a.jpg", 0, makeImage(10, 10));
    cache->add("/b.jpg", 0, makeImage(10, 10));
    cache->add("/c.mp4", 1, makeImage(10, 10));
    ASSERT_EQ(cache->keys().size(), 3);

    cache->clear();
    EXPECT_EQ(cache->keys().size(), 0);
}

TEST_F(ThumbnailCacheTest, Clear_OnEmptyCacheDoesNotCrash)
{
    EXPECT_NO_FATAL_FAILURE(cache->clear());
    EXPECT_EQ(cache->keys().size(), 0);
}

// ---- keys ----

TEST_F(ThumbnailCacheTest, Keys_EmptyCacheReturnsEmptyList)
{
    EXPECT_TRUE(cache->keys().isEmpty());
}

TEST_F(ThumbnailCacheTest, Keys_ReturnsAllKeysAfterMultipleAdds)
{
    cache->add("/a.jpg", 0, makeImage(10, 10));
    cache->add("/b.jpg", 0, makeImage(10, 10));
    cache->add("/c.mp4", 3, makeImage(10, 10));

    QList<ThumbnailCache::Key> allKeys = cache->keys();
    EXPECT_EQ(allKeys.size(), 3);
}

TEST_F(ThumbnailCacheTest, Keys_ReturnsCorrectKeyValues)
{
    cache->add("/photo.jpg", 5, makeImage(10, 10));

    QList<ThumbnailCache::Key> allKeys = cache->keys();
    ASSERT_EQ(allKeys.size(), 1);
    EXPECT_EQ(allKeys.first().first, QString("/photo.jpg"));
    EXPECT_EQ(allKeys.first().second, 5);
}
