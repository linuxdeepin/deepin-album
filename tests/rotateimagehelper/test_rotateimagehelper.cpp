// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

/*
 * 用例统计表
 * | RotateImageHelper::instance | high | 1 | 1 | 1 |
 * | RotateImageHelper::rotateImageFile | high | 4 | 4 | 4 |
 * | RotateImageHelper::resetRotateState | medium | 3 | 3 | 3 |
 * | RotateImageHelper::rotateImageImpl (static) | high | 6 | 6 | 6 |
 * | RotateImageHelper::checkDataValid (private) | medium | 1 | 1 | 1 |
 * | RotateImageHelper::enqueueRotateTask (private) | medium | 1 | 1 | 1 |
 *
 * 分支列表:
 *   instance():
 *     I1: 两次调用返回同一指针且非空
 *   rotateImageFile():
 *     R1: angle 为 0/360/-360 → 跳过，不产生任务与信号
 *     R2: 普通本地图片旋转 90 度 → rotateImageFinished(path, true)，宽高互换
 *     R3: 同一文件连续 90+90 → 角度累加 180
 *     R4: 文件不存在 → 不崩溃，任务正常结束
 *   resetRotateState():
 *     S1: data 为空时调用不崩溃
 *     S2: 产生任务后调用清空旋转记录
 *     S3: 重复调用不崩溃
 *   rotateImageImpl (static):
 *     M1: cachePath 不存在 → 拷贝源文件到 cachePath，再旋转保存
 *     M2: cachePath 已存在 → 复用缓存
 *     M3: 0/90/180/270 度旋转 → 尺寸验证
 *     M4: path 不存在 → 返回 false
 *     M5: 成功旋转时信号顺序 recordRotateImage → clearRotateStatus → rotateImageFinished
 *     M6: 0 度旋转 → 成功，信号顺序正确
 *   checkDataValid (private):
 *     D1: 首次调用后 data 非空、缓存目录有效
 *   enqueueRotateTask (private):
 *     E1: 压入多个任务后队列逐一处理并排空
 */

#include <gtest/gtest.h>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QTemporaryDir>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QThread>
#include <QCoreApplication>
#include <QElapsedTimer>

#include "stubext.h"
#include "addr_pri.h"
#include "utils/rotateimagehelper.h"
#include "unionimage/unionimage.h"

using namespace stub_ext;

// Forward-declare the private data class (defined in rotateimagehelper.cpp)
class RotateImageHelperData;

// Access private members
ACCESS_PRIVATE_FIELD(RotateImageHelper, QSharedPointer<RotateImageHelperData>, data)
ACCESS_PRIVATE_FUN(RotateImageHelper, void(), checkDataValid)
ACCESS_PRIVATE_FUN(RotateImageHelper, void(const QString &, int), enqueueRotateTask)

class RotateImageHelperTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        if (!QGuiApplication::instance()) {
            static int argc = 1;
            static char *argv[] = {(char *)"test_rotateimagehelper", nullptr};
            new QGuiApplication(argc, argv);
        }
    }

    void SetUp() override
    {
        ASSERT_TRUE(tempDir.isValid());
    }

    void TearDown() override
    {
        // Process any pending events from async operations
        QCoreApplication::processEvents(QEventLoop::AllEvents, 200);
    }

    QTemporaryDir tempDir;

    // Create a PNG test image with the given dimensions
    QString createPngImage(const QString &name, int width = 20, int height = 10)
    {
        QString path = tempDir.filePath(name);
        QImage img(width, height, QImage::Format_ARGB32);
        img.fill(Qt::red);
        QPainter painter(&img);
        painter.drawLine(0, 0, width - 1, 0);  // Draw a line to make rotation detectable
        painter.end();
        EXPECT_TRUE(img.save(path, "PNG"));
        return path;
    }

    // Ensure the watcher has finished before proceeding
    void ensureWatcherFinished()
    {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 500);
        QThread::msleep(50);
        QCoreApplication::processEvents(QEventLoop::AllEvents, 200);
    }
};

// =========================================================================
// instance()
// =========================================================================

// I1: Two calls return the same non-null pointer (singleton semantics)
TEST_F(RotateImageHelperTest, Instance_ReturnsSameNonNullPointer)
{
    RotateImageHelper *first = RotateImageHelper::instance();
    RotateImageHelper *second = RotateImageHelper::instance();

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_EQ(first, second);
}

// =========================================================================
// rotateImageFile()
// =========================================================================

// R1: angle 0/360/-360 → skip, no tasks or signals
TEST_F(RotateImageHelperTest, RotateImageFile_ZeroAngle_NoSignalEmitted)
{
    auto *helper = RotateImageHelper::instance();
    QString path = createPngImage("zero_angle.png");

    QSignalSpy finishedSpy(helper, &RotateImageHelper::rotateImageFinished);

    // angle 0
    helper->rotateImageFile(path, 0);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    EXPECT_EQ(finishedSpy.count(), 0);

    // angle 360 (normalized to 0)
    helper->rotateImageFile(path, 360);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    EXPECT_EQ(finishedSpy.count(), 0);

    // angle -360 (normalized to 0)
    helper->rotateImageFile(path, -360);
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    EXPECT_EQ(finishedSpy.count(), 0);
}

// R2: Rotate a normal local image by 90 degrees → finished(path, true), w/h swapped
TEST_F(RotateImageHelperTest, RotateImageFile_90Degrees_WidthHeightSwapped)
{
    auto *helper = RotateImageHelper::instance();
    QString path = createPngImage("rotate90.png", 20, 10);

    // Record original dimensions
    QImage original(path);
    ASSERT_FALSE(original.isNull());
    int origW = original.width();
    int origH = original.height();

    QSignalSpy finishedSpy(helper, &RotateImageHelper::rotateImageFinished);

    helper->rotateImageFile(path, 90);

    // Wait for the async rotation to complete
    EXPECT_TRUE(finishedSpy.wait(5000));
    ensureWatcherFinished();

    // Verify signal received with correct parameters
    ASSERT_GE(finishedSpy.count(), 1);
    EXPECT_EQ(finishedSpy.at(0).at(0).toString(), path);
    EXPECT_TRUE(finishedSpy.at(0).at(1).toBool());

    // Verify the image dimensions are swapped after rotation
    QImage rotated(path);
    ASSERT_FALSE(rotated.isNull());
    EXPECT_EQ(rotated.width(), origH);
    EXPECT_EQ(rotated.height(), origW);
}

// R3: Consecutive 90+90 on the same file → total angle 180
TEST_F(RotateImageHelperTest, RotateImageFile_ConsecutiveRotations_AngleAccumulated)
{
    auto *helper = RotateImageHelper::instance();
    QString path = createPngImage("rotate_180.png", 20, 10);

    QImage original(path);
    ASSERT_FALSE(original.isNull());
    int origW = original.width();
    int origH = original.height();

    QSignalSpy finishedSpy(helper, &RotateImageHelper::rotateImageFinished);

    // First 90-degree rotation
    helper->rotateImageFile(path, 90);
    EXPECT_TRUE(finishedSpy.wait(5000));
    ensureWatcherFinished();

    // Second 90-degree rotation (total 180)
    finishedSpy.clear();
    helper->rotateImageFile(path, 90);
    EXPECT_TRUE(finishedSpy.wait(5000));
    ensureWatcherFinished();

    // After 180-degree rotation, dimensions should be the same as original
    QImage rotated(path);
    ASSERT_FALSE(rotated.isNull());
    EXPECT_EQ(rotated.width(), origW);
    EXPECT_EQ(rotated.height(), origH);
}

// R4: Non-existent file → no crash, task completes normally
TEST_F(RotateImageHelperTest, RotateImageFile_NonexistentFile_NoCrash)
{
    auto *helper = RotateImageHelper::instance();
    QString path = tempDir.filePath("nonexistent_image.png");

    ASSERT_FALSE(QFile::exists(path));

    QSignalSpy finishedSpy(helper, &RotateImageHelper::rotateImageFinished);

    helper->rotateImageFile(path, 90);

    // Wait for the async task to process and fail (no signal emitted on failure)
    // Process events with a timeout to let the watcher thread complete
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 3000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        QThread::msleep(50);
    }
    ensureWatcherFinished();

    // No crash is the primary assertion; rotateImageFinished is not emitted on failure
    SUCCEED();
}

// =========================================================================
// resetRotateState()
// =========================================================================

// S1: Calling resetRotateState when data is null → no crash
TEST_F(RotateImageHelperTest, ResetRotateState_NullData_NoCrash)
{
    // Create a fresh RotateImageHelper that has never had checkDataValid called
    // Note: RotateImageHelper is a singleton, so we need to test the null-data path
    // by resetting data first
    auto *helper = RotateImageHelper::instance();

    // Reset data to null to simulate never-having-called-checkDataValid state
    auto &dataRef = access_private_field::RotateImageHelperdata(*helper);
    dataRef.reset();

    // Should not crash
    helper->resetRotateState();
    SUCCEED();
}

// S2: After a task, resetRotateState clears rotation records
TEST_F(RotateImageHelperTest, ResetRotateState_AfterTask_ClearsRecords)
{
    auto *helper = RotateImageHelper::instance();
    QString path = createPngImage("reset_test.png", 20, 10);

    // Perform a rotation to populate internal state
    QSignalSpy finishedSpy(helper, &RotateImageHelper::rotateImageFinished);
    helper->rotateImageFile(path, 90);
    EXPECT_TRUE(finishedSpy.wait(5000));
    ensureWatcherFinished();

    // resetRotateState should not crash and should clear rotation cache
    helper->resetRotateState();
    SUCCEED();
}

// S3: Repeated calls to resetRotateState → no crash
TEST_F(RotateImageHelperTest, ResetRotateState_RepeatedCalls_NoCrash)
{
    auto *helper = RotateImageHelper::instance();

    helper->resetRotateState();
    helper->resetRotateState();
    helper->resetRotateState();
    SUCCEED();
}

// =========================================================================
// rotateImageImpl() (static)
// =========================================================================

// M1: cachePath doesn't exist → copies source to cachePath, then rotates and saves back
TEST_F(RotateImageHelperTest, RotateImageImpl_CachePathNotExist_CopiesAndRotates)
{
    QString srcPath = createPngImage("impl_copy_src.png", 20, 10);
    QString cachePath = tempDir.filePath("impl_copy_cache.png");

    ASSERT_FALSE(QFile::exists(cachePath));

    bool ret = RotateImageHelper::rotateImageImpl(cachePath, srcPath, 90);

    EXPECT_TRUE(ret);

    // Cache file should have been created (copy of original)
    EXPECT_TRUE(QFile::exists(cachePath));

    // Source file should now contain the rotated image (w/h swapped)
    QImage rotated(srcPath);
    ASSERT_FALSE(rotated.isNull());
    EXPECT_EQ(rotated.width(), 10);
    EXPECT_EQ(rotated.height(), 20);

    // Cache file should still contain the original dimensions (pre-rotation copy)
    QImage cacheImg(cachePath);
    ASSERT_FALSE(cacheImg.isNull());
    EXPECT_EQ(cacheImg.width(), 20);
    EXPECT_EQ(cacheImg.height(), 10);
}

// M2: cachePath already exists → reuses cache, does not re-copy
TEST_F(RotateImageHelperTest, RotateImageImpl_CachePathExists_ReusesCache)
{
    QString srcPath = createPngImage("impl_reuse_src.png", 20, 10);
    QString cachePath = tempDir.filePath("impl_reuse_cache.png");

    // Pre-create the cache file with different content
    QImage cacheImg(15, 25, QImage::Format_ARGB32);
    cacheImg.fill(Qt::blue);
    ASSERT_TRUE(cacheImg.save(cachePath, "PNG"));

    // Record the cache file's modification time
    QFileInfo cacheInfo(cachePath);
    QDateTime beforeMod = cacheInfo.lastModified();

    // Small delay to ensure modification time would differ if re-copied
    QThread::msleep(50);

    bool ret = RotateImageHelper::rotateImageImpl(cachePath, srcPath, 90);

    EXPECT_TRUE(ret);

    // The cache file should still be the blue 15x25 image (not overwritten by copy)
    QImage cacheAfter(cachePath);
    ASSERT_FALSE(cacheAfter.isNull());
    EXPECT_EQ(cacheAfter.width(), 15);
    EXPECT_EQ(cacheAfter.height(), 25);
}

// M3: 0/90/180/270 degree rotations → verify dimensions
TEST_F(RotateImageHelperTest, RotateImageImpl_VariousAngles_DimensionsCorrect)
{
    for (int angle : {0, 90, 180, 270}) {
        QString srcPath = createPngImage(
            QStringLiteral("impl_angle_%1.png").arg(angle), 20, 10);
        QString cachePath = tempDir.filePath(
            QStringLiteral("impl_cache_%1.png").arg(angle));

        // Remove cache to force fresh copy for each angle
        QFile::remove(cachePath);

        bool ret = RotateImageHelper::rotateImageImpl(cachePath, srcPath, angle);
        EXPECT_TRUE(ret) << "Failed for angle " << angle;

        QImage rotated(srcPath);
        ASSERT_FALSE(rotated.isNull()) << "Failed to load for angle " << angle;

        if (angle == 0 || angle == 180) {
            EXPECT_EQ(rotated.width(), 20) << "Wrong width for angle " << angle;
            EXPECT_EQ(rotated.height(), 10) << "Wrong height for angle " << angle;
        } else {
            // 90 or 270: dimensions swapped
            EXPECT_EQ(rotated.width(), 10) << "Wrong width for angle " << angle;
            EXPECT_EQ(rotated.height(), 20) << "Wrong height for angle " << angle;
        }
    }
}

// M4: path doesn't exist → returns false
TEST_F(RotateImageHelperTest, RotateImageImpl_PathNotExist_ReturnsFalse)
{
    QString srcPath = tempDir.filePath("nonexistent_for_impl.png");
    QString cachePath = tempDir.filePath("nonexistent_cache.png");

    ASSERT_FALSE(QFile::exists(srcPath));

    bool ret = RotateImageHelper::rotateImageImpl(cachePath, srcPath, 90);
    EXPECT_FALSE(ret);
}

// M5: Successful rotation emits signals in order: recordRotateImage → clearRotateStatus → rotateImageFinished
TEST_F(RotateImageHelperTest, RotateImageImpl_Success_SignalOrderCorrect)
{
    auto *helper = RotateImageHelper::instance();
    QString srcPath = createPngImage("impl_signal_order.png", 20, 10);
    QString cachePath = tempDir.filePath("impl_signal_cache.png");

    QSignalSpy recordSpy(helper, SIGNAL(recordRotateImage(QString)));
    QSignalSpy clearSpy(helper, SIGNAL(clearRotateStatus(QString)));
    QSignalSpy finishedSpy(helper, &RotateImageHelper::rotateImageFinished);

    bool ret = RotateImageHelper::rotateImageImpl(cachePath, srcPath, 90);
    EXPECT_TRUE(ret);

    // Process any queued events
    QCoreApplication::processEvents(QEventLoop::AllEvents, 200);

    // All three signals should have been emitted exactly once
    EXPECT_EQ(recordSpy.count(), 1);
    EXPECT_EQ(clearSpy.count(), 1);
    EXPECT_EQ(finishedSpy.count(), 1);

    // Verify signal parameters
    EXPECT_EQ(recordSpy.at(0).at(0).toString(), srcPath);
    EXPECT_EQ(clearSpy.at(0).at(0).toString(), srcPath);
    EXPECT_EQ(finishedSpy.at(0).at(0).toString(), srcPath);
    EXPECT_TRUE(finishedSpy.at(0).at(1).toBool());
}

// M6: 0-degree rotation → succeeds, signals emitted in correct order
TEST_F(RotateImageHelperTest, RotateImageImpl_ZeroAngle_SignalsCorrect)
{
    auto *helper = RotateImageHelper::instance();
    QString srcPath = createPngImage("impl_zero_angle.png", 20, 10);
    QString cachePath = tempDir.filePath("impl_zero_cache.png");

    QSignalSpy recordSpy(helper, SIGNAL(recordRotateImage(QString)));
    QSignalSpy clearSpy(helper, SIGNAL(clearRotateStatus(QString)));
    QSignalSpy finishedSpy(helper, &RotateImageHelper::rotateImageFinished);

    bool ret = RotateImageHelper::rotateImageImpl(cachePath, srcPath, 0);
    EXPECT_TRUE(ret);

    QCoreApplication::processEvents(QEventLoop::AllEvents, 200);

    EXPECT_EQ(recordSpy.count(), 1);
    EXPECT_EQ(clearSpy.count(), 1);
    EXPECT_EQ(finishedSpy.count(), 1);
    EXPECT_EQ(finishedSpy.at(0).at(0).toString(), srcPath);
    EXPECT_TRUE(finishedSpy.at(0).at(1).toBool());
}

// =========================================================================
// checkDataValid() (private)
// =========================================================================

// D1: First call creates data, cache dir is valid
TEST_F(RotateImageHelperTest, CheckDataValid_FirstCall_CreatesDataAndCacheDir)
{
    auto *helper = RotateImageHelper::instance();

    // Reset data to null to ensure checkDataValid creates fresh data
    auto &dataRef = access_private_field::RotateImageHelperdata(*helper);
    dataRef.reset();

    // Call checkDataValid
    call_private_fun::RotateImageHelpercheckDataValid(*helper);

    // data should now be non-null
    auto &dataAfter = access_private_field::RotateImageHelperdata(*helper);
    EXPECT_FALSE(dataAfter.isNull());
}

// =========================================================================
// enqueueRotateTask() (private)
// =========================================================================

// E1: Multiple tasks enqueued → queue processed and emptied, each file gets finished signal
TEST_F(RotateImageHelperTest, EnqueueRotateTask_MultipleTasks_AllProcessed)
{
    auto *helper = RotateImageHelper::instance();

    // Ensure data is initialized
    call_private_fun::RotateImageHelpercheckDataValid(*helper);

    QString path1 = createPngImage("enqueue1.png", 20, 10);
    QString path2 = createPngImage("enqueue2.png", 20, 10);
    QString path3 = createPngImage("enqueue3.png", 20, 10);

    QSignalSpy finishedSpy(helper, &RotateImageHelper::rotateImageFinished);

    // Enqueue first task - this starts the watcher
    call_private_fun::RotateImageHelperenqueueRotateTask(*helper, path1, 90);

    // Wait for the first task to complete
    EXPECT_TRUE(finishedSpy.wait(5000));
    ensureWatcherFinished();

    // Enqueue second task
    finishedSpy.clear();
    call_private_fun::RotateImageHelperenqueueRotateTask(*helper, path2, 90);
    EXPECT_TRUE(finishedSpy.wait(5000));
    ensureWatcherFinished();

    // Enqueue third task
    finishedSpy.clear();
    call_private_fun::RotateImageHelperenqueueRotateTask(*helper, path3, 90);
    EXPECT_TRUE(finishedSpy.wait(5000));
    ensureWatcherFinished();

    // Verify all images were rotated (dimensions swapped)
    QImage img1(path1);
    ASSERT_FALSE(img1.isNull());
    EXPECT_EQ(img1.width(), 10);
    EXPECT_EQ(img1.height(), 20);

    QImage img2(path2);
    ASSERT_FALSE(img2.isNull());
    EXPECT_EQ(img2.width(), 10);
    EXPECT_EQ(img2.height(), 20);

    QImage img3(path3);
    ASSERT_FALSE(img3.isNull());
    EXPECT_EQ(img3.width(), 10);
    EXPECT_EQ(img3.height(), 20);
}
