// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SETWALLPAPERCONFIG_H
#define SETWALLPAPERCONFIG_H

namespace Dtk { namespace Core { class DConfig; } }

// 读取 DConfig 配置，控制右键菜单“设置为壁纸”项的显示/隐藏（默认显示）
class SetWallpaperConfig
{
public:
    static SetWallpaperConfig *instance();

    // true 显示，false 隐藏
    bool visible() const;

private:
    SetWallpaperConfig();
    ~SetWallpaperConfig();
    SetWallpaperConfig(const SetWallpaperConfig &) = delete;
    SetWallpaperConfig &operator=(const SetWallpaperConfig &) = delete;

    Dtk::Core::DConfig *m_dconfig = nullptr;
    bool m_visible = true;
};

#endif // SETWALLPAPERCONFIG_H
