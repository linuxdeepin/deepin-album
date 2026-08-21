// SPDX-FileCopyrightText: 2024-2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DEVICEHELPER_H
#define DEVICEHELPER_H

#include <QDBusInterface>
#include <QDir>
#include <QMutex>
#include <QWaitCondition>
#include <QObject>
#include <QUrl>

// 设备管理辅助类
class DeviceHelper : public QObject
{
    Q_OBJECT

public:
    explicit DeviceHelper(QObject *parent = nullptr);
    ~DeviceHelper();

    static DeviceHelper *instance();

    // 更新所有设备信息
    void loadAllDeviceInfos();

    // 获取所有设备的挂载点
    QStringList getAllMountPoints();

    // 根据设备Id获取设备信息
    QString getMountPointByDeviceId(const QString &deviceId);

    // 根据挂载点获取设备Id
    QString getDeviceIdByMountPoint(const QString &mnp);

    // 获取所有设备Id,包括块设备和协议设备
    QStringList getAllDeviceIds();

    // 获取所有块设备Id
    QStringList getBlockDeviceIds();

    // 加载设备信息
    QVariantMap loadDeviceInfo(const QString &deviceId, bool reload = false);

    // 卸载设备
    bool detachDevice(const QString &deviceId);

    // 判断设备是否存在
    bool isExist(const QString &deviceId);

    // 判断url是否为smb网络路径
    static bool isSamba(const QUrl &url);

    // Mark/clear a mount point as unmounting; while marked, parsing of files on that device is blocked
    static void markUnmounting(const QString &mountPoint, bool unmounting);

    // Atomically check a path and register one in-flight file read
    static bool tryBeginRead(const QString &path);
    // Unregister one in-flight read; must be given the same path passed to tryBeginRead
    static void endRead(const QString &path);
    // Wait for all in-flight reads under the given mount point to finish; returns false on timeout
    // Note: the blocking wait processes no events; calling it on the main thread briefly freezes the UI (bounded by timeoutMs)
    static bool waitForReadsDone(const QString &mountPoint, int timeoutMs);

private:
    static DeviceHelper *m_instance;
    static QMutex s_unmountMutex;
    static QWaitCondition s_readCondition;
    static QStringList s_unmountingPaths;
    static QStringList s_activeReads; // normalized paths of in-flight reads (the same path may appear more than once)

    QStringList getProtocalDeviceIds();
private:
    QScopedPointer<QDBusInterface> m_dfmDeviceManager; // 文管设备管理DBus服务接口
    QMap<QString, QVariantMap> m_mapDevicesInfos; // 记录所有可插拔设备信息，设备id-设备信息map表
};

// In-flight file read guard: registers/unregisters the read on construction/destruction,
// so unmount can drain reads by mount point
class DeviceReadGuard
{
    Q_DISABLE_COPY(DeviceReadGuard)

public:
    explicit DeviceReadGuard(const QString &path)
        : m_path(path)
        , m_active(DeviceHelper::tryBeginRead(path))
    {
    }

    ~DeviceReadGuard()
    {
        if (m_active) {
            DeviceHelper::endRead(m_path);
        }
    }

    bool isActive() const { return m_active; }

private:
    QString m_path;
    bool m_active = false;
};

#endif  // DEVICEHELPER_H
