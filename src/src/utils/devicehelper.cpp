// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "devicehelper.h"
#include "unionimage/baseutils.h"
#include <dfm-mount/base/dmount_global.h>
#include <DSysInfo>

#include <QDBusReply>
#include <QDebug>
#include <QRegularExpression>

DCORE_USE_NAMESPACE

DeviceHelper *DeviceHelper::m_instance = nullptr;
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
    qDebug() << "Initializing DeviceHelper";
    // DFM 设备管理接口，访问文件挂载信息
    if (DSysInfo::majorVersion() == "25") {
        qDebug() << "Using V25 file manager daemon interface";
        m_dfmDeviceManager.reset(new QDBusInterface(QStringLiteral(V25_FILEMANAGER_DAEMON_SERVICE),
                                                    QStringLiteral(V25_FILEMANAGER_DAEMON_PATH),
                                                    QStringLiteral(V25_FILEMANAGER_DAEMON_INTERFACE)));
    } else {
        qDebug() << "Using V23 file manager daemon interface";
        m_dfmDeviceManager.reset(new QDBusInterface(QStringLiteral(V23_FILEMANAGER_DAEMON_SERVICE),
                                                    QStringLiteral(V23_FILEMANAGER_DAEMON_PATH),
                                                    QStringLiteral(V23_FILEMANAGER_DAEMON_INTERFACE)));
    }

    qInfo() << "DeviceHelper initialized - majorVersion:" << DSysInfo::majorVersion()
             << "dbus service:" << m_dfmDeviceManager.data()->service()
             << "interface:" << m_dfmDeviceManager.data()->interface()
             << "object:" << m_dfmDeviceManager.data()->objectName()
             << "path:" << m_dfmDeviceManager.data()->path();
}

DeviceHelper::~DeviceHelper()
{
    qDebug() << "Destroying DeviceHelper";
}

DeviceHelper *DeviceHelper::instance()
{
    qDebug() << "Getting DeviceHelper instance";
    if (!m_instance) {
        qDebug() << "Creating new DeviceHelper instance";
        m_instance = new DeviceHelper();
    }
    return m_instance;
}

QStringList DeviceHelper::getAllMountPoints()
{
    qDebug() << "Getting all mount points";
    loadAllDeviceInfos();
    QStringList allMountPoints;
    for (const auto &deviceId : m_mapDevicesInfos.keys()) {
        QString mountPoint = m_mapDevicesInfos.value(deviceId).value("MountPoint").toString();
        if (!mountPoint.isEmpty()) {
            qDebug() << "Found mount point:" << mountPoint << "for device:" << deviceId;
            allMountPoints.push_back(mountPoint);
        }
    }
    qDebug() << "Total mount points found:" << allMountPoints.size();
    return allMountPoints;
}

QString DeviceHelper::getMountPointByDeviceId(const QString &deviceId)
{
    qDebug() << "Getting mount point for device ID:" << deviceId;
    if (deviceId.isEmpty()) {
        qWarning() << "Empty device ID provided";
        return QString("");
    }

    QVariantMap infos = loadDeviceInfo(deviceId);
    if (!infos.isEmpty()) {
        QString mountPoint = infos.value("MountPoint").toString();
        qDebug() << "Found mount point:" << mountPoint << "for device:" << deviceId;
        return mountPoint;
    }

    qWarning() << "No mount point found for device:" << deviceId;
    return QString("");
}

QString DeviceHelper::getDeviceIdByMountPoint(const QString &mnp)
{
    qDebug() << "Getting device ID for mount point:" << mnp;
    if (mnp.isEmpty()) {
        qWarning() << "Empty mount point provided";
        return QString("");
    }

    for (const auto &deviceId : m_mapDevicesInfos.keys()) {
        QString mountPoint = m_mapDevicesInfos.value(deviceId).value("MountPoint").toString();
        if (!mountPoint.isEmpty() && mnp == mountPoint) {
            qDebug() << "Found device ID:" << deviceId << "for mount point:" << mnp;
            return deviceId;
        }
    }

    qWarning() << "No device ID found for mount point:" << mnp;
    return QString("");
}

void DeviceHelper::loadAllDeviceInfos()
{
    qDebug() << "Loading all device information";
    m_mapDevicesInfos.clear();
    QStringList blockDeviceIds = getBlockDeviceIds();
    QStringList protocalDeviceIds = getProtocalDeviceIds();
    QStringList deviceIds;
    deviceIds << blockDeviceIds << protocalDeviceIds;
    qDebug() << "Found" << deviceIds.size() << "devices (" 
             << blockDeviceIds.size() << "block devices,"
             << protocalDeviceIds.size() << "protocol devices)";

    for (auto id : deviceIds) {
        loadDeviceInfo(id, true);
    }
}

QStringList DeviceHelper::getAllDeviceIds()
{
    qDebug() << "Getting all device IDs";
    QStringList ids = m_mapDevicesInfos.keys();
    qDebug() << "Found" << ids.size() << "device IDs";
    return ids;
}

QStringList DeviceHelper::getBlockDeviceIds()
{
    qDebug() << "Getting block device IDs";
    QDBusReply<QStringList> deviceListReply = m_dfmDeviceManager->call("GetBlockDevicesIdList", kRemovable);
    if (!deviceListReply.isValid()) {
        qWarning() << "DBus call GetBlockDevicesIdList failed:" << deviceListReply.error().message();
        return QStringList();
    }

    QStringList devices = deviceListReply.value();
    qDebug() << "Found" << devices.size() << "block devices";
    return devices;
}

QStringList DeviceHelper::getProtocalDeviceIds()
{
    qDebug() << "Getting protocol device IDs";
    QDBusReply<QStringList> protocalDeviceListReply = m_dfmDeviceManager->call("GetProtocolDevicesIdList");
    if (!protocalDeviceListReply.isValid()) {
        qWarning() << "DBus call GetProtocolDevicesIdList failed:" << protocalDeviceListReply.error().message();
        return QStringList();
    }

    QStringList devices = protocalDeviceListReply.value();
    qDebug() << "Found" << devices.size() << "protocol devices";
    return devices;
}

QVariantMap DeviceHelper::loadDeviceInfo(const QString &deviceId, bool reload)
{
    qDebug() << "Loading device info for ID:" << deviceId << "reload:" << reload;
    if (deviceId.isEmpty()) {
        qWarning() << "Empty device ID provided";
        return QVariantMap();
    }

    // 过滤samba网络路径
    if (isSamba(deviceId)) {
        qDebug() << "Device is Samba, skipping:" << deviceId;
        return QVariantMap();
    }

    if (m_mapDevicesInfos.find(deviceId) != m_mapDevicesInfos.end() && !reload) {
        qDebug() << "Using cached device info for:" << deviceId;
        return m_mapDevicesInfos[deviceId];
    }

    QDBusReply<QVariantMap> deviceReply;
    QVariantMap deviceInfo;
    if (deviceId.startsWith("/org/freedesktop/")) {
        qDebug() << "Querying block device info for:" << deviceId;
        deviceReply = m_dfmDeviceManager->call("QueryBlockDeviceInfo", deviceId, false);
        if (!deviceReply.isValid()) {
            qWarning() << "DBus call QueryBlockDeviceInfo failed:" << deviceReply.error().message();
            return QVariantMap();
        }

        deviceInfo = deviceReply.value();
        if (QString("usb") != deviceInfo.value("ConnectionBus").toString()) {
            qWarning() << "Device is not USB:" << deviceId;
            return QVariantMap();
        }
    } else {
        qDebug() << "Querying protocol device info for:" << deviceId;
        deviceReply = m_dfmDeviceManager->call("QueryProtocolDeviceInfo", deviceId, false);
        if (!deviceReply.isValid()) {
            qWarning() << "DBus call QueryProtocolDeviceInfo failed:" << deviceReply.error().message();
            return QVariantMap();
        }
        deviceInfo = deviceReply.value();
    }

    if (deviceInfo.isEmpty()) {
        qWarning() << "Empty device info received for:" << deviceId;
        if (m_mapDevicesInfos.find(deviceId) != m_mapDevicesInfos.end())
            m_mapDevicesInfos.remove(deviceId);
        return QVariantMap();
    }

    // 过滤为空的挂载点
    QString mountPoint = deviceInfo.value("MountPoint").toString();
    if (mountPoint.isEmpty()) {
        qDebug() << "Empty mount point for device:" << deviceId;
        return QVariantMap();
    }

    // 过滤光驱
    if (deviceInfo.contains("Optical") && deviceInfo.value("Optical").toBool()) {
        qDebug() << "Device is optical drive, skipping:" << deviceId;
        return QVariantMap();
    }

    if (deviceId.startsWith("/org/freedesktop/"))
        deviceInfo["DeviceType"] = static_cast<int>(DeviceType::kBlockDevice);
    else
        deviceInfo["DeviceType"] = static_cast<int>(DeviceType::kProtocolDevice);

    m_mapDevicesInfos[deviceId] = deviceInfo;
    qDebug() << "Successfully loaded device info for:" << deviceId 
             << "mount point:" << mountPoint
             << "type:" << deviceInfo["DeviceType"].toInt();

    return deviceInfo;
}

bool DeviceHelper::detachDevice(const QString &deviceId)
{
    qDebug() << "Detaching device:" << deviceId;
    if (deviceId.isEmpty()) {
        qWarning() << "Cannot detach device - empty device ID";
        return false;
    }

    QList<QVariant> argumentList;
    argumentList << QVariant::fromValue(deviceId);
    if (deviceId.startsWith("/org/freedesktop/")) {
        qDebug() << "Detaching block device:" << deviceId;
        m_dfmDeviceManager->asyncCallWithArgumentList(QStringLiteral("DetachBlockDevice"), argumentList);
    } else {
        qDebug() << "Detaching protocol device:" << deviceId;
        m_dfmDeviceManager->asyncCallWithArgumentList(QStringLiteral("DetachProtocolDevice"), argumentList);
    }
    return true;
}

bool DeviceHelper::isExist(const QString &deviceId)
{
    qDebug() << "Checking if device exists:" << deviceId;
    if (deviceId.isEmpty()) {
        qDebug() << "Empty device ID provided";
        return false;
    }

    bool exists = m_mapDevicesInfos.find(deviceId) != m_mapDevicesInfos.end();
    qDebug() << "Device" << deviceId << (exists ? "exists" : "does not exist");
    return exists;
}

/*!
   \return 判断 \a url 是否为远程挂载路径
 */
bool DeviceHelper::isSamba(const QUrl &url)
{
    qDebug() << "Checking if URL is Samba:" << url;
    if (!url.isValid()) {
        qDebug() << "Invalid URL provided";
        return false;
    }

    const QString &path = url.toLocalFile();
    static const QString gvfsMatch { "(^/run/user/\\d+/gvfs/|^/root/.gvfs/|^(/run)?/media/[\\s\\S]*/smbmounts)" };
    QRegularExpression re { gvfsMatch };
    QRegularExpressionMatch match { re.match(path) };
    bool isSamba = match.hasMatch();
    qDebug() << "Path" << path << (isSamba ? "is" : "is not") << "Samba";
    return isSamba;
}
