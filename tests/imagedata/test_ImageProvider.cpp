// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <QThreadPool>
#include <QUrl>
#include <memory>

#include "imagedata/imageprovider.h"

template <typename Provider>
class SeededImageProvider : public Provider
{
public:
    void seed(const QString &path, const QImage &image)
    {
        this->imageCache.add(path, 0, image);
    }

    QImage cached(const QString &path)
    {
        return this->imageCache.get(path, 0);
    }
};

class ImageProviderSizeTest : public ::testing::TestWithParam<bool>
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char name[] = "test_imageprovider";
            static char *argv[] = {name, nullptr};
            static QGuiApplication app(argc, argv);
        }
    }

    void SetUp() override
    {
        ASSERT_TRUE(directory.isValid());
        path = directory.filePath("image.png");
        id = QUrl::fromLocalFile(path).toString() + "#frame_0";
    }

    static QImage makeImage(const QSize &size, Qt::GlobalColor color)
    {
        QImage image(size, QImage::Format_ARGB32);
        image.fill(color);
        return image;
    }

    void seed(const QImage &image)
    {
        if (GetParam()) {
            asyncProvider.seed(path, image);
        } else {
            syncProvider.seed(path, image);
        }
    }

    QImage request(const QSize &requestedSize)
    {
        if (!GetParam()) {
            return syncProvider.requestImage(id, nullptr, requestedSize);
        }

        std::unique_ptr<QQuickImageResponse> response(asyncProvider.requestImageResponse(id, requestedSize));
        // Wait for the worker to finish before reading or destroying its response.
        QThreadPool::globalInstance()->waitForDone();
        QCoreApplication::processEvents();
        std::unique_ptr<QQuickTextureFactory> factory(response->textureFactory());
        return factory->image();
    }

    QImage cached()
    {
        return GetParam() ? asyncProvider.cached(path) : syncProvider.cached(path);
    }

    QTemporaryDir directory;
    QString path;
    QString id;
    SeededImageProvider<ImageProvider> syncProvider;
    SeededImageProvider<AsyncImageProvider> asyncProvider;
};

TEST_P(ImageProviderSizeTest, PortraitCacheInLandscapeBoxDoesNotReloadOldDirection)
{
    // The file still has its old direction; only the cache has been rotated.
    ASSERT_TRUE(makeImage(QSize(1920, 1080), Qt::red).save(path));
    seed(makeImage(QSize(1080, 1920), Qt::green));

    const QImage result = request(QSize(1150, 700));

    ASSERT_FALSE(result.isNull());
    EXPECT_LT(result.width(), result.height());
    EXPECT_EQ(result.pixelColor(0, 0), QColor(Qt::green));
    EXPECT_EQ(cached().size(), QSize(1080, 1920));
}

TEST_P(ImageProviderSizeTest, LandscapeCacheInPortraitBoxDoesNotReloadOldDirection)
{
    ASSERT_TRUE(makeImage(QSize(1080, 1920), Qt::red).save(path));
    seed(makeImage(QSize(1920, 1080), Qt::green));

    const QImage result = request(QSize(700, 1150));

    ASSERT_FALSE(result.isNull());
    EXPECT_GT(result.width(), result.height());
    EXPECT_EQ(result.pixelColor(0, 0), QColor(Qt::green));
    EXPECT_EQ(cached().size(), QSize(1920, 1080));
}

TEST_P(ImageProviderSizeTest, EnlargedBoxStillKeepsSufficientRotatedCache)
{
    ASSERT_TRUE(makeImage(QSize(1920, 1080), Qt::red).save(path));
    seed(makeImage(QSize(1080, 1920), Qt::green));

    request(QSize(1150, 700));
    const QImage result = request(QSize(1500, 1000));

    ASSERT_FALSE(result.isNull());
    EXPECT_LT(result.width(), result.height());
    EXPECT_EQ(result.pixelColor(0, 0), QColor(Qt::green));
    EXPECT_EQ(cached().size(), QSize(1080, 1920));
}

TEST_P(ImageProviderSizeTest, InsufficientCacheStillLoadsHigherResolution)
{
    ASSERT_TRUE(makeImage(QSize(1080, 1920), Qt::red).save(path));
    seed(makeImage(QSize(108, 192), Qt::green));

    const QImage result = request(QSize(1150, 700));

    ASSERT_FALSE(result.isNull());
    EXPECT_GT(result.height(), 192);
    EXPECT_EQ(result.pixelColor(0, 0), QColor(Qt::red));
}

TEST_P(ImageProviderSizeTest, InvalidRequestKeepsCachedImage)
{
    seed(makeImage(QSize(1080, 1920), Qt::green));

    const QImage result = request(QSize());

    EXPECT_EQ(result.size(), QSize(1080, 1920));
    EXPECT_EQ(result.pixelColor(0, 0), QColor(Qt::green));
}

INSTANTIATE_TEST_SUITE_P(SyncAndAsync, ImageProviderSizeTest, ::testing::Bool());
