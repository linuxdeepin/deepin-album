// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QUrl>
#include <QStringList>
#include <QString>
#include <QVariant>
#include <memory>

#include "globalcontrol.h"
#include "types.h"
#include "imagedata/imagesourcemodel.h"
#include "imagedata/pathviewproxymodel.h"
#include "utils/rotateimagehelper.h"

#include "stubext.h"
#include "addr_pri.h"

using namespace stub_ext;

// Typedefs for pointer types to avoid macro issues
using ImageSourceModelPtr = ImageSourceModel *;
using PathViewProxyModelPtr = PathViewProxyModel *;

// Private field accessors for GlobalControl
ACCESS_PRIVATE_FIELD(GlobalControl, int, curIndex)
ACCESS_PRIVATE_FIELD(GlobalControl, int, curFrameIndex)
ACCESS_PRIVATE_FIELD(GlobalControl, ImageInfo, currentImage)
ACCESS_PRIVATE_FIELD(GlobalControl, ImageSourceModelPtr, sourceModel)
ACCESS_PRIVATE_FIELD(GlobalControl, PathViewProxyModelPtr, viewSourceModel)
ACCESS_PRIVATE_FIELD(GlobalControl, bool, hasPrevious)
ACCESS_PRIVATE_FIELD(GlobalControl, bool, hasNext)
ACCESS_PRIVATE_FIELD(GlobalControl, int, imageRotation)

// Private field accessor for ImageInfo
ACCESS_PRIVATE_FIELD(ImageInfo, QUrl, imageUrl)

class GlobalControlTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char arg0[] = "test";
            static char *argv[] = {arg0, nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    static void TearDownTestSuite() {}

    void SetUp() override
    {
        // Stub ImageInfo::setSource to avoid loading image data from disk
        stub.set_lamda(ADDR(ImageInfo, setSource), [](ImageInfo *, const QUrl &) {});
        gc = std::make_unique<GlobalControl>();
    }

    void TearDown() override
    {
        gc.reset();
    }

    StubExt stub;
    std::unique_ptr<GlobalControl> gc;
};

// ============ currentSource ============

TEST_F(GlobalControlTest, CurrentSource_DefaultEmpty)
{
    EXPECT_EQ(gc->currentSource(), QUrl());
}

TEST_F(GlobalControlTest, CurrentSource_ReturnsImageUrl)
{
    auto &img = access_private_field::GlobalControlcurrentImage(*gc);
    access_private_field::ImageInfoimageUrl(img) = QUrl("file:///test/image.jpg");
    EXPECT_EQ(gc->currentSource(), QUrl("file:///test/image.jpg"));
}

// ============ currentIndex ============

TEST_F(GlobalControlTest, CurrentIndex_DefaultZero)
{
    EXPECT_EQ(gc->currentIndex(), 0);
}

TEST_F(GlobalControlTest, CurrentIndex_ReturnsSetValue)
{
    access_private_field::GlobalControlcurIndex(*gc) = 5;
    EXPECT_EQ(gc->currentIndex(), 5);
}

// ============ imageCount ============

TEST_F(GlobalControlTest, ImageCount_EmptyModel_ReturnsZero)
{
    EXPECT_EQ(gc->imageCount(), 0);
}

TEST_F(GlobalControlTest, ImageCount_WithFiles_ReturnsCount)
{
    QStringList files = {"file:///a.jpg", "file:///b.jpg", "file:///c.jpg"};
    gc->setImageFiles(files, "file:///a.jpg");
    EXPECT_EQ(gc->imageCount(), 3);
}

// ============ currentRotation ============

TEST_F(GlobalControlTest, CurrentRotation_DefaultZero)
{
    EXPECT_EQ(gc->currentRotation(), 0);
}

TEST_F(GlobalControlTest, CurrentRotation_ReturnsSetValue)
{
    access_private_field::GlobalControlimageRotation(*gc) = 90;
    EXPECT_EQ(gc->currentRotation(), 90);
}

// ============ setCurrentRotation ============

TEST_F(GlobalControlTest, SetCurrentRotation_SameAngle_NoChange)
{
    access_private_field::GlobalControlimageRotation(*gc) = 90;
    gc->setCurrentRotation(90);
    EXPECT_EQ(access_private_field::GlobalControlimageRotation(*gc), 90);
}

TEST_F(GlobalControlTest, SetCurrentRotation_InvalidAngle_Rejected)
{
    access_private_field::GlobalControlimageRotation(*gc) = 0;
    gc->setCurrentRotation(45);
    EXPECT_EQ(access_private_field::GlobalControlimageRotation(*gc), 0);
}

TEST_F(GlobalControlTest, SetCurrentRotation_InvalidAngleNegative_Rejected)
{
    access_private_field::GlobalControlimageRotation(*gc) = 0;
    gc->setCurrentRotation(-45);
    EXPECT_EQ(access_private_field::GlobalControlimageRotation(*gc), 0);
}

TEST_F(GlobalControlTest, SetCurrentRotation_ValidAngle180_NoSwap)
{
    stub.set_lamda(ADDR(RotateImageHelper, rotateImageFile),
                   [](RotateImageHelper *, const QString &, int) {});
    access_private_field::GlobalControlimageRotation(*gc) = 0;
    gc->setCurrentRotation(180);
    EXPECT_EQ(access_private_field::GlobalControlimageRotation(*gc), 180);
}

TEST_F(GlobalControlTest, SetCurrentRotation_ValidAngle90_WithSwap)
{
    stub.set_lamda(ADDR(RotateImageHelper, rotateImageFile),
                   [](RotateImageHelper *, const QString &, int) {});
    stub.set_lamda(ADDR(ImageInfo, swapWidthAndHeight), [](ImageInfo *) {});
    access_private_field::GlobalControlimageRotation(*gc) = 0;
    gc->setCurrentRotation(90);
    EXPECT_EQ(access_private_field::GlobalControlimageRotation(*gc), 90);
}

TEST_F(GlobalControlTest, SetCurrentRotation_ValidAngle270_WithSwap)
{
    stub.set_lamda(ADDR(RotateImageHelper, rotateImageFile),
                   [](RotateImageHelper *, const QString &, int) {});
    stub.set_lamda(ADDR(ImageInfo, swapWidthAndHeight), [](ImageInfo *) {});
    access_private_field::GlobalControlimageRotation(*gc) = 0;
    gc->setCurrentRotation(270);
    EXPECT_EQ(access_private_field::GlobalControlimageRotation(*gc), 270);
}

// ============ hasPreviousImage ============

TEST_F(GlobalControlTest, HasPreviousImage_DefaultFalse)
{
    EXPECT_FALSE(gc->hasPreviousImage());
}

TEST_F(GlobalControlTest, HasPreviousImage_ReturnsTrue)
{
    access_private_field::GlobalControlhasPrevious(*gc) = true;
    EXPECT_TRUE(gc->hasPreviousImage());
}

// ============ hasNextImage ============

TEST_F(GlobalControlTest, HasNextImage_DefaultFalse)
{
    EXPECT_FALSE(gc->hasNextImage());
}

TEST_F(GlobalControlTest, HasNextImage_ReturnsTrue)
{
    access_private_field::GlobalControlhasNext(*gc) = true;
    EXPECT_TRUE(gc->hasNextImage());
}

// ============ hasMultipleImages ============

TEST_F(GlobalControlTest, HasMultipleImages_NoImages_ReturnsFalse)
{
    EXPECT_FALSE(gc->hasMultipleImages());
}

TEST_F(GlobalControlTest, HasMultipleImages_OneImage_ReturnsFalse)
{
    gc->setImageFiles({"file:///a.jpg"}, "file:///a.jpg");
    EXPECT_FALSE(gc->hasMultipleImages());
}

TEST_F(GlobalControlTest, HasMultipleImages_MultipleImagesWithHasNext_ReturnsTrue)
{
    gc->setImageFiles({"file:///a.jpg", "file:///b.jpg"}, "file:///a.jpg");
    // After setImageFiles with 2 files at index 0, checkSwitchEnable sets hasNext=true
    EXPECT_TRUE(gc->hasMultipleImages());
}

TEST_F(GlobalControlTest, HasMultipleImages_MultipleImagesNoPrevNoNext_ReturnsFalse)
{
    gc->setImageFiles({"file:///a.jpg", "file:///b.jpg"}, "file:///a.jpg");
    access_private_field::GlobalControlhasPrevious(*gc) = false;
    access_private_field::GlobalControlhasNext(*gc) = false;
    EXPECT_FALSE(gc->hasMultipleImages());
}

// ============ setIndexAndFrameIndex ============

TEST_F(GlobalControlTest, SetIndexAndFrameIndex_ValidIndex_UpdatesCurIndex)
{
    gc->setImageFiles({"file:///a.jpg", "file:///b.jpg", "file:///c.jpg"}, "file:///a.jpg");
    EXPECT_EQ(gc->currentIndex(), 0);
    gc->setIndexAndFrameIndex(2, 0);
    EXPECT_EQ(gc->currentIndex(), 2);
}

TEST_F(GlobalControlTest, SetIndexAndFrameIndex_SameIndex_NoChange)
{
    gc->setImageFiles({"file:///a.jpg", "file:///b.jpg"}, "file:///a.jpg");
    gc->setIndexAndFrameIndex(0, 0);
    EXPECT_EQ(gc->currentIndex(), 0);
}

TEST_F(GlobalControlTest, SetIndexAndFrameIndex_OutOfRangeHigh_Clamped)
{
    gc->setImageFiles({"file:///a.jpg", "file:///b.jpg"}, "file:///a.jpg");
    gc->setIndexAndFrameIndex(100, 0);
    // validIndex = qBound(0, 100, 1) = 1
    EXPECT_EQ(gc->currentIndex(), 1);
}

TEST_F(GlobalControlTest, SetIndexAndFrameIndex_NegativeIndex_Clamped)
{
    gc->setImageFiles({"file:///a.jpg", "file:///b.jpg"}, "file:///b.jpg");
    // curIndex = 1 after setImageFiles
    gc->setIndexAndFrameIndex(-5, 0);
    // validIndex = qBound(0, -5, 1) = 0
    EXPECT_EQ(gc->currentIndex(), 0);
}

// ============ previousImage ============

TEST_F(GlobalControlTest, PreviousImage_NoPrevious_ReturnsFalse)
{
    gc->setImageFiles({"file:///a.jpg", "file:///b.jpg"}, "file:///a.jpg");
    // curIndex=0, hasPrevious=false
    EXPECT_FALSE(gc->previousImage());
}

TEST_F(GlobalControlTest, PreviousImage_HasPrevious_DecrementsIndex)
{
    gc->setImageFiles({"file:///a.jpg", "file:///b.jpg", "file:///c.jpg"}, "file:///b.jpg");
    EXPECT_EQ(gc->currentIndex(), 1);
    EXPECT_TRUE(gc->previousImage());
    EXPECT_EQ(gc->currentIndex(), 0);
}

TEST_F(GlobalControlTest, PreviousImage_FromMiddle_NavigatesCorrectly)
{
    gc->setImageFiles({"file:///a.jpg", "file:///b.jpg", "file:///c.jpg"}, "file:///c.jpg");
    EXPECT_EQ(gc->currentIndex(), 2);
    EXPECT_TRUE(gc->previousImage());
    EXPECT_EQ(gc->currentIndex(), 1);
}

// ============ nextImage ============

TEST_F(GlobalControlTest, NextImage_NoNext_ReturnsFalse)
{
    gc->setImageFiles({"file:///a.jpg", "file:///b.jpg"}, "file:///b.jpg");
    // curIndex=1 (last), hasNext=false
    EXPECT_FALSE(gc->nextImage());
}

TEST_F(GlobalControlTest, NextImage_HasNext_IncrementsIndex)
{
    gc->setImageFiles({"file:///a.jpg", "file:///b.jpg", "file:///c.jpg"}, "file:///a.jpg");
    EXPECT_EQ(gc->currentIndex(), 0);
    EXPECT_TRUE(gc->nextImage());
    EXPECT_EQ(gc->currentIndex(), 1);
}

TEST_F(GlobalControlTest, NextImage_FromMiddle_NavigatesCorrectly)
{
    gc->setImageFiles({"file:///a.jpg", "file:///b.jpg", "file:///c.jpg"}, "file:///b.jpg");
    EXPECT_EQ(gc->currentIndex(), 1);
    EXPECT_TRUE(gc->nextImage());
    EXPECT_EQ(gc->currentIndex(), 2);
}

// ============ setImageFiles ============

TEST_F(GlobalControlTest, SetImageFiles_MultipleFiles_SetsCountAndIndex)
{
    QStringList files = {"file:///a.jpg", "file:///b.jpg", "file:///c.jpg"};
    gc->setImageFiles(files, "file:///b.jpg");
    EXPECT_EQ(gc->imageCount(), 3);
    EXPECT_EQ(gc->currentIndex(), 1);
}

TEST_F(GlobalControlTest, SetImageFiles_OpenFileNotInList_DefaultsToZero)
{
    QStringList files = {"file:///a.jpg", "file:///b.jpg"};
    gc->setImageFiles(files, "file:///nonexistent.jpg");
    EXPECT_EQ(gc->currentIndex(), 0);
    EXPECT_EQ(gc->imageCount(), 2);
}

TEST_F(GlobalControlTest, SetImageFiles_EmptyList_IndexZero)
{
    gc->setImageFiles({}, "");
    EXPECT_EQ(gc->imageCount(), 0);
    EXPECT_EQ(gc->currentIndex(), 0);
}

TEST_F(GlobalControlTest, SetImageFiles_SingleFile_NoMultipleImages)
{
    gc->setImageFiles({"file:///a.jpg"}, "file:///a.jpg");
    EXPECT_EQ(gc->imageCount(), 1);
    EXPECT_FALSE(gc->hasMultipleImages());
}
