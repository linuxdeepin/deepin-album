// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "setwallpaperconfig.h"

#include <QDebug>
#include <DConfig>

DCORE_USE_NAMESPACE

static const char *const DCONFIG_APPID = "org.deepin.album";
static const char *const DCONFIG_NAME = "org.deepin.album";
static const char *const DCONFIG_KEY_SET_WALLPAPER_VISIBLE = "setWallpaperVisible";

SetWallpaperConfig::SetWallpaperConfig()
{
    m_dconfig = DConfig::create(DCONFIG_APPID, DCONFIG_NAME, QString());
    if (m_dconfig && m_dconfig->isValid()) {
        m_visible = m_dconfig->value(DCONFIG_KEY_SET_WALLPAPER_VISIBLE, true).toBool();
    } else {
        qWarning() << "DConfig is invalid, use default setWallpaperVisible = true";
    }
}

SetWallpaperConfig::~SetWallpaperConfig()
{
    delete m_dconfig;
    m_dconfig = nullptr;
}

SetWallpaperConfig *SetWallpaperConfig::instance()
{
    static SetWallpaperConfig ins;
    return &ins;
}

bool SetWallpaperConfig::visible() const
{
    return m_visible;
}
