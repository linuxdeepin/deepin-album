// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filetrashhelper.h"
#include "ddlog.h"
#include "imagedata/imagefilewatcher.h"
#include "unionimage/baseutils.h"

#include <DSysInfo>

#include <QApplication>
#include <QDBusReply>
#include <QDebug>
#include <QFile>
#include <QFileSystemWatcher>
#include <QRegularExpression>

DCORE_USE_NAMESPACE
/*!
   \brief Result type for moving a file to the Trash via DBus.
 */
enum DeletionResult {
    kTrashTimeout,  // default error
    kTrashInterfaceError,
    kTrashInvalidUrl,
    kTrashSuccess,
};

/*!
   \class FileTrashHelper::FileTrashHelper
   \brief 文件回收站辅助类
   \details 用于判断文件是否可被移动到回收站中，以及移动文件到回收站
   \note 强关联文管DBus接口，需注意对应接口更新
 */
FileTrashHelper::FileTrashHelper(QObject *parent)
    : QObject(parent)
{
    qCDebug(albumLog) << "Initializing FileTrashHelper";
    // DFM 设备管理接口，访问文件挂载信息
    if (DSysInfo::majorVersion() == "25") {
        qCDebug(albumLog) << "Using V25 file manager daemon interface";
        m_dfmDeviceManager.reset(new QDBusInterface(QStringLiteral(V25_FILEMANAGER_DAEMON_SERVICE),
                                                    QStringLiteral(V25_FILEMANAGER_DAEMON_PATH),
                                                    QStringLiteral(V25_FILEMANAGER_DAEMON_INTERFACE)));
    } else {
        qCDebug(albumLog) << "Using V23 file manager daemon interface";
        m_dfmDeviceManager.reset(new QDBusInterface(QStringLiteral(V23_FILEMANAGER_DAEMON_SERVICE),
                                                    QStringLiteral(V23_FILEMANAGER_DAEMON_PATH),
                                                    QStringLiteral(V23_FILEMANAGER_DAEMON_INTERFACE)));
    }

    qCInfo(albumLog) << "FileTrashHelper initialized - majorVersion:" << DSysInfo::majorVersion()
            << "dbus service:" << m_dfmDeviceManager.data()->service() 
            << "interface:" << m_dfmDeviceManager.data()->interface()
            << "object:" << m_dfmDeviceManager.data()->objectName() 
            << "path:" << m_dfmDeviceManager.data()->path();
}

/*!
   \return 返回文件 \a url 是否可被移动到回收站中
 */
bool FileTrashHelper::fileCanTrash(const QUrl &url)
{
    qCDebug(albumLog) << "Checking if file can be moved to trash:" << url;
    // 检测当前文件和上次是否相同
    QDir currentDir(url.path());
    if (lastDir != currentDir) {
        qCDebug(albumLog) << "Directory changed from" << lastDir.path() << "to" << currentDir.path();
        lastDir = currentDir;
        resetMountInfo();
    }

    queryMountInfo();

    if (isGvfsFile(url)) {
        qCDebug(albumLog) << "File is GVFS, cannot be moved to trash:" << url;
        return false;
    }

    if (isExternalDevice(url.path())) {
        qCDebug(albumLog) << "File is on external device, cannot be moved to trash:" << url;
        return false;
    }

    qCDebug(albumLog) << "File can be moved to trash:" << url;
    return true;
}

/*!
   \return 移动文件 \a url 到回收站
 */
bool FileTrashHelper::moveFileToTrash(const QUrl &url)
{
    qCDebug(albumLog) << "Attempting to move file to trash:" << url;
    // 优先采用文管后端 DBus 服务
    int ret = moveFileToTrashWithDBus(url);

    if (kTrashInterfaceError == ret) {
        qCInfo(albumLog) << "Move file to trash DBus interface failed, rolling back to v20 version:" << url;
        // rollback v20 interface
        if (Libutils::base::trashFile(url.path())) {
            qCDebug(albumLog) << "Successfully moved file to trash using v20 interface:" << url;
            return true;
        }
    }

    if (kTrashSuccess != ret) {
        qCWarning(albumLog) << "Failed to move file to trash:" << url << "error code:" << ret;
        return false;
    }

    qCDebug(albumLog) << "Successfully moved file to trash:" << url;
    return true;
}

/*!
   \return 返回删除文件 \a url 的结果
 */
bool FileTrashHelper::removeFile(const QUrl &url)
{
    qCDebug(albumLog) << "Attempting to remove file:" << url;
    QFile file(url.path());
    bool ret = file.remove();
    if (!ret) {
        qCWarning(albumLog) << "Failed to remove file:" << url << "error:" << file.errorString();
    } else {
        qCDebug(albumLog) << "Successfully removed file:" << url;
    }

    return ret;
}

/*!
   \brief 重置文件挂载信息，仅在需要获取时重新取得挂载数据
    对于看图应用的特殊处理，在打开新的图片后，仅需在首次触发查询挂载设备信息时查询，
    对于中途取消挂载的设备，文件监控会提示文件不存在，因此无需处理;
    同时，看图只会访问同一目录内容，挂载信息仅需查询单次，无需重复查询或监控设备
 */
void FileTrashHelper::resetMountInfo()
{
    qCDebug(albumLog) << "Resetting mount information";
    initData = false;
    mountDevices.clear();
}

/*!
   \brief 查询当前挂载设备信息
   \note 注意依赖文官接口的变动
 */
void FileTrashHelper::queryMountInfo()
{
    qCDebug(albumLog) << "Querying mount information";
    if (initData) {
        qCDebug(albumLog) << "Mount information already initialized, skipping query";
        return;
    }

    initData = true;

    // 调用 DBus 接口查询可被卸载设备信息
    QDBusReply<QStringList> deviceListReply = m_dfmDeviceManager->call("GetBlockDevicesIdList", kRemovable);
    if (!deviceListReply.isValid()) {
        qCWarning(albumLog) << "DBus call GetBlockDevicesIdList failed:" << deviceListReply.error().message();
        return;
    }

    qCDebug(albumLog) << "Found" << deviceListReply.value().size() << "removable devices";
    for (const QString &id : deviceListReply.value()) {
        qCDebug(albumLog) << "Querying device info for:" << id;
        QDBusReply<QVariantMap> deviceReply = m_dfmDeviceManager->call("QueryBlockDeviceInfo", id, false);
        if (!deviceReply.isValid()) {
            qCWarning(albumLog) << "DBus call QueryBlockDeviceInfo failed for device" << id 
                       << "error:" << deviceReply.error().message();
            continue;
        }

        const QVariantMap deviceInfo = deviceReply.value();
        if (QString("usb") == deviceInfo.value("ConnectionBus").toString()) {
            qCDebug(albumLog) << "Found USB device:" << id;
            const QStringList mountPaths = deviceInfo.value("MountPoints").toStringList();
            if (mountPaths.isEmpty()) {
                const QString mountPath = deviceInfo.value("MountPoint").toString();

                if (!mountPath.isEmpty()) {
                    qCDebug(albumLog) << "Adding mount point for device" << id << ":" << mountPath;
                    mountDevices.insert(id, mountPath);
                }
            } else {
                for (const QString &mount : mountPaths) {
                    if (!mount.isEmpty()) {
                        qCDebug(albumLog) << "Adding mount point for device" << id << ":" << mount;
                        mountDevices.insert(id, mount);
                    }
                }
            }
        }
    }
    qCDebug(albumLog) << "Found" << mountDevices.size() << "mount points";
}

/*!
   \return 返回文件路径 \a path 是否指向外部设备
 */
bool FileTrashHelper::isExternalDevice(const QString &path)
{
    qCDebug(albumLog) << "Checking if path is on external device:" << path;
    for (auto itr = mountDevices.begin(); itr != mountDevices.end(); ++itr) {
        if (path.startsWith(itr.value())) {
            qCDebug(albumLog) << "Path is on external device" << itr.key() << "mounted at" << itr.value();
            return true;
        }
    }

    qCDebug(albumLog) << "Path is not on any external device:" << path;
    return false;
}

/*!
   \return 判断 \a url 是否为远程挂载路径，此路径下同样无法恢复文件
 */
bool FileTrashHelper::isGvfsFile(const QUrl &url) const
{
    qCDebug(albumLog) << "Checking if URL is GVFS:" << url;
    if (!url.isValid()) {
        qCDebug(albumLog) << "Invalid URL provided";
        return false;
    }

    const QString &path = url.toLocalFile();
    static const QString gvfsMatch { "(^/run/user/\\d+/gvfs/|^/root/.gvfs/|^(/run)?/media/[\\s\\S]*/smbmounts)" };
    QRegularExpression re { gvfsMatch };
    QRegularExpressionMatch match { re.match(path) };
    bool isGvfs = match.hasMatch();
    qCDebug(albumLog) << "Path" << path << (isGvfs ? "is" : "is not") << "GVFS";
    return isGvfs;
}

/*!
   \brief 通过后端文管接口将文件 \a url 移动到回收站
    注意文管接口是异步接口且没有返回值，此处通过文件变更信号判断文件是否被移动。
   \return 移动文件到回收站是否成功
 */
int FileTrashHelper::moveFileToTrashWithDBus(const QUrl &url)
{
    qCDebug(albumLog) << "Attempting to move file to trash via DBus:" << url;
    if (!url.isValid()) {
        qCWarning(albumLog) << "Invalid URL provided for trash operation:" << url;
        return kTrashInvalidUrl;
    }

    QStringList list;
    list << url.toString();
    qCDebug(albumLog) << "Creating DBus interface for trash operation";
    // 优先采用文管后端的DBus服务
    QDBusInterface interface(QStringLiteral("org.freedesktop.FileManager1"),
                             QStringLiteral("/org/freedesktop/FileManager1"),
                             QStringLiteral("org.freedesktop.FileManager1"));

    QEventLoop loop;
    int waitRet = kTrashTimeout;
    // wait file deleted signal
    QString filePath = url.path();
    qCDebug(albumLog) << "Setting up file change watcher for:" << filePath;
    auto conn = QObject::connect(ImageFileWatcher::instance(),
                                 &ImageFileWatcher::imageFileChanged,
                                 this,
                                 [&filePath, &loop, &waitRet](const QString &imagePath) {
                                     // 删除信息未通过 DBus 返回，直接判断文件是否已被删除
                                     if ((imagePath == filePath) && !QFile::exists(filePath)) {
                                         qCDebug(albumLog) << "File change detected, file moved to trash:" << filePath;
                                         waitRet = kTrashSuccess;
                                         loop.quit();
                                     };
                                 });

    qCDebug(albumLog) << "Calling DBus Trash method";
    auto pendingCall = interface.asyncCall("Trash", list);
    // will return soon
    pendingCall.waitForFinished();

    if (pendingCall.isError()) {
        auto error = pendingCall.error();
        qCWarning(albumLog) << "DBus Trash call failed:" << error.name() << error.message();
        QObject::disconnect(conn);
        return kTrashInterfaceError;
    }

    if (kTrashSuccess != waitRet) {
        // FIX-273813 Wait up to 10 seconds
        static constexpr int kDelTimeout = 1000 * 10;
        QTimer::singleShot(kDelTimeout, &loop, &QEventLoop::quit);
        loop.exec();
    }

    QObject::disconnect(conn);
    qCDebug(albumLog) << "Trash operation completed with result:" << waitRet;
    return waitRet;
}
