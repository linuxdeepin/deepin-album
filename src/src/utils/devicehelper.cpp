// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "devicehelper.h"
#include "ddlog.h"
#include "unionimage/baseutils.h"
#include <dfm-mount/base/dmount_global.h>
#include <DSysInfo>

#include <QDBusReply>
#include <QFileInfo>
#include <QDebug>
#include <QElapsedTimer>

#include <algorithm>
#include <QRegularExpression>

DCORE_USE_NAMESPACE

DeviceHelper *DeviceHelper::m_instance = nullptr;
QMutex DeviceHelper::s_unmountMutex;
QWaitCondition DeviceHelper::s_readCondition;
QStringList DeviceHelper::s_unmountingPaths;
QStringList DeviceHelper::s_activeReads;
using namespace dfmmount;

/*!
   \class DeviceHelper::DeviceHelper
   \brief 设备管理辅助类
   \details 用于管理可移除设备，包括U盘、手机等外界硬件设备
   \note 强关联文管DBus接口，需注意对应接口更新
 */
DeviceHelper::DeviceHelper(QObject *parent)
    : QObject(parent)
{
    qCDebug(albumLog) << "Initializing DeviceHelper";
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

    qCInfo(albumLog) << "DeviceHelper initialized - majorVersion:" << DSysInfo::majorVersion()
             << "dbus service:" << m_dfmDeviceManager.data()->service()
             << "interface:" << m_dfmDeviceManager.data()->interface()
             << "object:" << m_dfmDeviceManager.data()->objectName()
             << "path:" << m_dfmDeviceManager.data()->path();
}

DeviceHelper::~DeviceHelper()
{
    qCDebug(albumLog) << "Destroying DeviceHelper";
}

namespace {
QString normalizedPath(const QString &path)
{
    if (path.isEmpty()) {
        return {};
    }
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

bool pathUnderMountPoint(const QString &path, const QString &mountPoint)
{
    return path == mountPoint || path.startsWith(mountPoint + QLatin1Char('/'));
}
}

void DeviceHelper::markUnmounting(const QString &mountPoint, bool unmounting)
{
    const QString normalizedMountPoint = normalizedPath(mountPoint);
    if (normalizedMountPoint.isEmpty()) {
        return;
    }

    QMutexLocker locker(&s_unmountMutex);
    if (unmounting) {
        if (!s_unmountingPaths.contains(normalizedMountPoint)) {
            s_unmountingPaths.append(normalizedMountPoint);
        }
    } else {
        s_unmountingPaths.removeAll(normalizedMountPoint);
    }
}

bool DeviceHelper::tryBeginRead(const QString &path)
{
    const QString normalized = normalizedPath(path);
    if (normalized.isEmpty()) {
        return false;
    }

    QMutexLocker locker(&s_unmountMutex);
    const bool unmounting = std::any_of(
        s_unmountingPaths.cbegin(), s_unmountingPaths.cend(),
        [&normalized](const QString &root) { return pathUnderMountPoint(normalized, root); });
    if (unmounting) {
        return false;
    }

    s_activeReads.append(normalized);
    return true;
}

void DeviceHelper::endRead(const QString &path)
{
    const QString normalized = normalizedPath(path);
    QMutexLocker locker(&s_unmountMutex);
    s_activeReads.removeOne(normalized);
    s_readCondition.wakeAll();
}

bool DeviceHelper::waitForReadsDone(const QString &mountPoint, int timeoutMs)
{
    const QString normalizedRoot = normalizedPath(mountPoint);
    if (normalizedRoot.isEmpty()) {
        return true;
    }

    auto hasActiveRead = [&normalizedRoot]() {
        return std::any_of(s_activeReads.cbegin(), s_activeReads.cend(),
                           [&normalizedRoot](const QString &readPath) {
                               return pathUnderMountPoint(readPath, normalizedRoot);
                           });
    };

    QMutexLocker locker(&s_unmountMutex);
    QElapsedTimer timer;
    timer.start();
    while (hasActiveRead()) {
        const int remaining = timeoutMs - static_cast<int>(timer.elapsed());
        if (remaining <= 0 || !s_readCondition.wait(&s_unmountMutex, remaining)) {
            qCWarning(albumLog) << "DeviceHelper::waitForReadsDone timeout, remaining reads:"
                       << s_activeReads.size();
            return false;
        }
    }
    return true;
}

DeviceHelper *DeviceHelper::instance()
{
    qCDebug(albumLog) << "Getting DeviceHelper instance";
    if (!m_instance) {
        qCDebug(albumLog) << "Creating new DeviceHelper instance";
        m_instance = new DeviceHelper();
    }
    return m_instance;
}

QStringList DeviceHelper::getAllMountPoints()
{
    qCDebug(albumLog) << "Getting all mount points";
    loadAllDeviceInfos();
    QStringList allMountPoints;
    for (const auto &deviceId : m_mapDevicesInfos.keys()) {
        QString mountPoint = m_mapDevicesInfos.value(deviceId).value("MountPoint").toString();
        if (!mountPoint.isEmpty()) {
            qCDebug(albumLog) << "Found mount point:" << mountPoint << "for device:" << deviceId;
            allMountPoints.push_back(mountPoint);
        }
    }
    qCDebug(albumLog) << "Total mount points found:" << allMountPoints.size();
    return allMountPoints;
}

QString DeviceHelper::getMountPointByDeviceId(const QString &deviceId)
{
    qCDebug(albumLog) << "Getting mount point for device ID:" << deviceId;
    if (deviceId.isEmpty()) {
        qCWarning(albumLog) << "Empty device ID provided";
        return QString("");
    }

    QVariantMap infos = loadDeviceInfo(deviceId);
    if (!infos.isEmpty()) {
        QString mountPoint = infos.value("MountPoint").toString();
        qCDebug(albumLog) << "Found mount point:" << mountPoint << "for device:" << deviceId;
        return mountPoint;
    }

    qCWarning(albumLog) << "No mount point found for device:" << deviceId;
    return QString("");
}

QString DeviceHelper::getDeviceIdByMountPoint(const QString &mnp)
{
    qCDebug(albumLog) << "Getting device ID for mount point:" << mnp;
    if (mnp.isEmpty()) {
        qCWarning(albumLog) << "Empty mount point provided";
        return QString("");
    }

    for (const auto &deviceId : m_mapDevicesInfos.keys()) {
        QString mountPoint = m_mapDevicesInfos.value(deviceId).value("MountPoint").toString();
        if (!mountPoint.isEmpty() && mnp == mountPoint) {
            qCDebug(albumLog) << "Found device ID:" << deviceId << "for mount point:" << mnp;
            return deviceId;
        }
    }

    qCWarning(albumLog) << "No device ID found for mount point:" << mnp;
    return QString("");
}

void DeviceHelper::loadAllDeviceInfos()
{
    qCDebug(albumLog) << "Loading all device information";
    m_mapDevicesInfos.clear();
    QStringList blockDeviceIds = getBlockDeviceIds();
    QStringList protocalDeviceIds = getProtocalDeviceIds();
    QStringList deviceIds;
    deviceIds << blockDeviceIds << protocalDeviceIds;
    qCDebug(albumLog) << "Found" << deviceIds.size() << "devices (" 
             << blockDeviceIds.size() << "block devices,"
             << protocalDeviceIds.size() << "protocol devices)";

    for (auto id : deviceIds) {
        loadDeviceInfo(id, true);
    }
}

QStringList DeviceHelper::getAllDeviceIds()
{
    qCDebug(albumLog) << "Getting all device IDs";
    QStringList ids = m_mapDevicesInfos.keys();
    qCDebug(albumLog) << "Found" << ids.size() << "device IDs";
    return ids;
}

QStringList DeviceHelper::getBlockDeviceIds()
{
    qCDebug(albumLog) << "Getting block device IDs";
    QDBusReply<QStringList> deviceListReply = m_dfmDeviceManager->call("GetBlockDevicesIdList", kRemovable);
    if (!deviceListReply.isValid()) {
        qCWarning(albumLog) << "DBus call GetBlockDevicesIdList failed:" << deviceListReply.error().message();
        return QStringList();
    }

    QStringList devices = deviceListReply.value();
    qCDebug(albumLog) << "Found" << devices.size() << "block devices";
    return devices;
}

QStringList DeviceHelper::getProtocalDeviceIds()
{
    qCDebug(albumLog) << "Getting protocol device IDs";
    QDBusReply<QStringList> protocalDeviceListReply = m_dfmDeviceManager->call("GetProtocolDevicesIdList");
    if (!protocalDeviceListReply.isValid()) {
        qCWarning(albumLog) << "DBus call GetProtocolDevicesIdList failed:" << protocalDeviceListReply.error().message();
        return QStringList();
    }

    QStringList devices = protocalDeviceListReply.value();
    qCDebug(albumLog) << "Found" << devices.size() << "protocol devices";
    return devices;
}

QVariantMap DeviceHelper::loadDeviceInfo(const QString &deviceId, bool reload)
{
    qCDebug(albumLog) << "Loading device info for ID:" << deviceId << "reload:" << reload;
    if (deviceId.isEmpty()) {
        qCWarning(albumLog) << "Empty device ID provided";
        return QVariantMap();
    }

    // 过滤samba网络路径
    if (isSamba(deviceId)) {
        qCDebug(albumLog) << "Device is Samba, skipping:" << deviceId;
        return QVariantMap();
    }

    if (m_mapDevicesInfos.find(deviceId) != m_mapDevicesInfos.end() && !reload) {
        qCDebug(albumLog) << "Using cached device info for:" << deviceId;
        return m_mapDevicesInfos[deviceId];
    }

    QDBusReply<QVariantMap> deviceReply;
    QVariantMap deviceInfo;
    if (deviceId.startsWith("/org/freedesktop/")) {
        qCDebug(albumLog) << "Querying block device info for:" << deviceId;
        deviceReply = m_dfmDeviceManager->call("QueryBlockDeviceInfo", deviceId, false);
        if (!deviceReply.isValid()) {
            qCWarning(albumLog) << "DBus call QueryBlockDeviceInfo failed:" << deviceReply.error().message();
            return QVariantMap();
        }

        deviceInfo = deviceReply.value();
        if (QString("usb") != deviceInfo.value("ConnectionBus").toString()) {
            qCWarning(albumLog) << "Device is not USB:" << deviceId;
            return QVariantMap();
        }
    } else {
        qCDebug(albumLog) << "Querying protocol device info for:" << deviceId;
        deviceReply = m_dfmDeviceManager->call("QueryProtocolDeviceInfo", deviceId, false);
        if (!deviceReply.isValid()) {
            qCWarning(albumLog) << "DBus call QueryProtocolDeviceInfo failed:" << deviceReply.error().message();
            return QVariantMap();
        }
        deviceInfo = deviceReply.value();
    }

    if (deviceInfo.isEmpty()) {
        qCWarning(albumLog) << "Empty device info received for:" << deviceId;
        if (m_mapDevicesInfos.find(deviceId) != m_mapDevicesInfos.end())
            m_mapDevicesInfos.remove(deviceId);
        return QVariantMap();
    }

    // 过滤为空的挂载点
    QString mountPoint = deviceInfo.value("MountPoint").toString();
    if (mountPoint.isEmpty()) {
        qCDebug(albumLog) << "Empty mount point for device:" << deviceId;
        return QVariantMap();
    }

    // 过滤光驱
    if (deviceInfo.contains("Optical") && deviceInfo.value("Optical").toBool()) {
        qCDebug(albumLog) << "Device is optical drive, skipping:" << deviceId;
        return QVariantMap();
    }

    if (deviceId.startsWith("/org/freedesktop/"))
        deviceInfo["DeviceType"] = static_cast<int>(DeviceType::kBlockDevice);
    else
        deviceInfo["DeviceType"] = static_cast<int>(DeviceType::kProtocolDevice);

    m_mapDevicesInfos[deviceId] = deviceInfo;
    qCDebug(albumLog) << "Successfully loaded device info for:" << deviceId 
             << "mount point:" << mountPoint
             << "type:" << deviceInfo["DeviceType"].toInt();

    return deviceInfo;
}

bool DeviceHelper::detachDevice(const QString &deviceId)
{
    qCDebug(albumLog) << "Detaching device:" << deviceId;
    if (deviceId.isEmpty()) {
        qCWarning(albumLog) << "Cannot detach device - empty device ID";
        return false;
    }

    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(deviceId);
    if (deviceId.startsWith("/org/freedesktop/")) {
        qCDebug(albumLog) << "Detaching block device:" << deviceId;
        m_dfmDeviceManager->asyncCallWithArgumentList(QStringLiteral("DetachBlockDevice"), argumentList);
    } else {
        qCDebug(albumLog) << "Detaching protocol device:" << deviceId;
        m_dfmDeviceManager->asyncCallWithArgumentList(QStringLiteral("DetachProtocolDevice"), argumentList);
    }
    return true;
}

bool DeviceHelper::isExist(const QString &deviceId)
{
    qCDebug(albumLog) << "Checking if device exists:" << deviceId;
    if (deviceId.isEmpty()) {
        qCDebug(albumLog) << "Empty device ID provided";
        return false;
    }

    bool exists = m_mapDevicesInfos.find(deviceId) != m_mapDevicesInfos.end();
    qCDebug(albumLog) << "Device" << deviceId << (exists ? "exists" : "does not exist");
    return exists;
}

/*!
   \return 判断 \a url 是否为远程挂载路径
 */
bool DeviceHelper::isSamba(const QUrl &url)
{
    qCDebug(albumLog) << "Checking if URL is Samba:" << url;
    if (!url.isValid()) {
        qCDebug(albumLog) << "Invalid URL provided";
        return false;
    }

    const QString &path = url.toLocalFile();
    static const QString gvfsMatch { "(^/run/user/\\d+/gvfs/|^/root/.gvfs/|^(/run)?/media/[\\s\\S]*/smbmounts)" };
    QRegularExpression re { gvfsMatch };
    QRegularExpressionMatch match { re.match(path) };
    bool isSamba = match.hasMatch();
    qCDebug(albumLog) << "Path" << path << (isSamba ? "is" : "is not") << "Samba";
    return isSamba;
}
