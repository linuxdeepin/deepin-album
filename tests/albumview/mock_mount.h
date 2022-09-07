// SPDX-FileCopyrightText: 2020 - 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QSharedData>

#include <dgiofile.h>

class Stub;

class Mock_Mount
{
public:
    Mock_Mount(const QString &mountPoint);
    ~Mock_Mount();

    //初始化挂载测试
    void initMountMock();

    //为卸载设备准备
    void setToUmountDevice();

    //DGioMount需要的桩函数
    QString stub_DGioMount_name();
    QExplicitlySharedDataPointer<DGioFile> stub_DGioMount_getRootFile();
    bool stub_DGioMount_canEject() const;
    QExplicitlySharedDataPointer<DGioFile> stub_DGioMount_getDefaultLocationFile();

    //DGioFile需要的桩函数
    QString stub_DGioFile_uri();
    QString stub_DGioFile_path();

    //DGioVolumeManager需要的桩函数
    const QList<QExplicitlySharedDataPointer<DGioMount> > stub_DGioVolumeManager_getMounts(); //这个是静态函数，打桩有点难度

private:
    //打桩工具
    Stub *stub;

    //文件保存位置
    static QString mountPoint;
};
