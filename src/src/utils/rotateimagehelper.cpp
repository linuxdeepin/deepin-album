// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "rotateimagehelper.h"
#include "imagedata/imagefilewatcher.h"
#include "unionimage/unionimage.h"

#include <QApplication>
#include <QQueue>
#include <QFutureWatcher>
#include <QTemporaryDir>
#include <QtConcurrent>

class RotateImageHelperData
{
public:
    explicit RotateImageHelperData();

    QString currentRotateImage;         // 当前操作的
    QHash<QString, int> rotationCache;  // 已缓存旋转文件列表 <文件路径，缓存旋转角度>
    QFutureWatcher<void> watcher;       // 异步处理监视器

    // 图片旋转处理队列
    QMutex queueMutex;
    QQueue<QPair<QString, int>> processQueue;  // 待处理的图片队列
    QTemporaryDir cacheDir;                    // 临时文件目录
};

RotateImageHelperData::RotateImageHelperData() 
{
    qDebug() << "Initializing RotateImageHelperData";
}

/**
   @class RotateImageHelper
   @brief 用于异步拷贝文本数据，防止阻塞界面(特别是在节能模式下)
    原始文件将被存放于临时目录，在程序退出时移除，降低频繁读写文件导致图像质量降低
 */
RotateImageHelper::RotateImageHelper(QObject *parent)
    : QObject { parent }
{
    qDebug() << "Initializing RotateImageHelper";
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() {
        qDebug() << "Application about to quit, cleaning up rotation tasks";
        // 等待拷贝文件结束
        if (data && data->watcher.isRunning()) {
            qDebug() << "Waiting for running rotation tasks to complete";
            data->watcher.waitForFinished();
            data->cacheDir.remove();
            qDebug() << "Rotation tasks completed and cache directory removed";
        }
    });
}

RotateImageHelper *RotateImageHelper::instance()
{
    qDebug() << "Getting RotateImageHelper instance";
    static RotateImageHelper ins;
    return &ins;
}

/**
   @brief 旋转图片文件 \a path 共 \a angle 度
 */
void RotateImageHelper::rotateImageFile(const QString &path, int angle)
{
    qDebug() << "Requesting image rotation:" << path << "angle:" << angle;
    angle = angle % 360;
    if (0 == angle) {
        qDebug() << "Skipping rotation - angle is 0";
        return;
    }

    // 20211019修改：特殊位置不执行写入操作
    imageViewerSpace::PathType pathType = LibUnionImage_NameSpace::getPathType(path);
    qDebug() << "Image path type:" << pathType;

    if (pathType == imageViewerSpace::PathTypeMTP || pathType == imageViewerSpace::PathTypePTP ||  // 安卓手机
        pathType == imageViewerSpace::PathTypeAPPLE ||                                             // 苹果手机
        pathType == imageViewerSpace::PathTypeSAFEBOX ||                                           // 保险箱
        pathType == imageViewerSpace::PathTypeRECYCLEBIN) {                                        // 回收站
        qDebug() << "Skipping rotation - unsupported path type:" << pathType;
        return;
    }

    // 构造数据结构
    checkDataValid();

    // 记录总旋转角度
    int &totalAngle = data->rotationCache[path];
    totalAngle += angle;
    totalAngle %= 360;
    qDebug() << "Total rotation angle for" << path << "is now" << totalAngle;

    if (data->watcher.isRunning()) {
        qDebug() << "Rotation task is running, adding to queue";
        QMutexLocker locker(&(data->queueMutex));
        for (auto &proc : data->processQueue) {
            if (proc.first == path) {
                qDebug() << "Updating existing rotation task for" << path;
                proc.second = totalAngle;
                // 查找数据，插入后退出
                return;
            }
        }
        // 未查找到数据，插入队列
        qDebug() << "Adding new rotation task to queue for" << path;
        data->processQueue.enqueue(qMakePair(path, totalAngle));

    } else {
        qDebug() << "Starting new rotation task for" << path;
        enqueueRotateTask(path, totalAngle);
    }
}

/**
   @brief 用于重置旋转记录信息，不会影响在处理中的文件
 */
void RotateImageHelper::resetRotateState()
{
    qDebug() << "Resetting rotation state";
    if (!data) {
        qDebug() << "No data to reset";
        return;
    }

    data->rotationCache.clear();
    if (!data->watcher.isRunning()) {
        qDebug() << "Removing cache directory";
        data->cacheDir.remove();
    } else {
        qDebug() << "Cache directory removal skipped - rotation task is running";
    }
}

/**
   @brief 缓存文件 \a cachePath 不存在时，拷贝文件 \a path 到缓存路径 \a cachePath .
    将缓存文件 \a cachePath 旋转 \a angle 度并保存到 \a path 中
 */
bool RotateImageHelper::rotateImageImpl(const QString &cachePath, const QString &path, int angle)
{
    qDebug() << "Implementing rotation for" << path << "angle:" << angle;
    // 拷贝文件到目录
    if (!QFile::exists(cachePath)) {
        qDebug() << "Copying file to cache:" << path << "->" << cachePath;
        if (!QFile::copy(path, cachePath)) {
            qWarning() << "Failed to copy file to cache:" << path;
            return false;
        }
    }

    // 操作前标记动作
    qDebug() << "Recording rotation start for" << path;
    Q_EMIT RotateImageHelper::instance()->recordRotateImage(path);

    QString errorMsg;
    qDebug() << "Performing rotation operation";
    bool ret = LibUnionImage_NameSpace::rotateImageFIle(angle, cachePath, errorMsg, path);

    // NOTE：处理结束，过滤旋转操作文件更新，旋转图像已在软件中缓存且旋转状态同步，不再从文件中更新读取
    // 保存文件后发送图片更新更新信号，通过监控文件变更触发。文件更新可能滞后，延时一定时间处理
    // 处于子线程中，慎用事件循环(没有初始化)
    QThread::msleep(10);
    qDebug() << "Clearing rotation status for" << path;
    Q_EMIT RotateImageHelper::instance()->clearRotateStatus(path);
    Q_EMIT RotateImageHelper::instance()->rotateImageFinished(path, ret);

    if (!ret) {
        qWarning() << "Rotation failed for" << path << "error:" << errorMsg;
    } else {
        qDebug() << "Rotation completed successfully for" << path;
    }
    return ret;
}

/**
   @brief 将文件 \a path 旋转 \a angle 角度任务压入队列中并启动任务
 */
void RotateImageHelper::enqueueRotateTask(const QString &path, int angle)
{
    qDebug() << "Enqueueing rotation task for" << path << "angle:" << angle;
    // 启动新的旋转处理线程
    QMutexLocker queueLocker(&(data->queueMutex));
    data->processQueue.enqueue(qMakePair(path, angle));
    queueLocker.unlock();

    auto rotateFuture = QtConcurrent::run([this]() {
        qDebug() << "Starting rotation task processing";
        int queueSize = 0;
        do {
            QPair<QString, int> currentData;

            QMutexLocker locker(&(data->queueMutex));
            if (data->processQueue.isEmpty()) {
                qDebug() << "Rotation queue is empty, stopping processing";
                break;
            }
            currentData = data->processQueue.dequeue();
            locker.unlock();

            // 执行拷贝文件及旋转
            QFileInfo file(currentData.first);
            QString cacheFile = data->cacheDir.filePath(file.fileName());
            if (!rotateImageImpl(cacheFile, currentData.first, currentData.second)) {
                qWarning() << "Rotation failed for" << currentData.first;
            }

            locker.relock();
            queueSize = data->processQueue.size();
            locker.unlock();
        } while (queueSize > 0);
        qDebug() << "Rotation task processing completed";
    });

    data->watcher.setFuture(rotateFuture);
}

/**
   @brief 数据字段仅在有需要时创建
 */
void RotateImageHelper::checkDataValid()
{
    qDebug() << "Checking data validity";
    if (!data) {
        qDebug() << "Creating new RotateImageHelperData";
        data.reset(new RotateImageHelperData);

        if (!data->cacheDir.isValid()) {
            qWarning() << "Failed to create cache directory:" << data->cacheDir.errorString();
        } else {
            qDebug() << "Cache directory created successfully:" << data->cacheDir.path();
        }

        qDebug() << "Setting up signal connections";
        connect(this,
                &RotateImageHelper::recordRotateImage,
                ImageFileWatcher::instance(),
                &ImageFileWatcher::recordRotateImage,
                Qt::QueuedConnection);
        connect(this,
                &RotateImageHelper::clearRotateStatus,
                ImageFileWatcher::instance(),
                &ImageFileWatcher::clearRotateStatus,
                Qt::QueuedConnection);
        qDebug() << "Signal connections established";
    }
}
