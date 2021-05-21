/*
 * Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co., Ltd.
 *
 * Author:     WangZhengYang <wangzhengyang@uniontech.com>
 *
 * Maintainer: ZhangYong <ZhangYong@uniontech.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
