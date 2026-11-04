// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: GPL-3.0-or-later

#include <gtest/gtest.h>

#include "setwallpaperconfig.h"

#include "stubext.h"
#include "addr_pri.h"

#include <DConfig>
#include <type_traits>

using namespace stub_ext;

// SetWallpaperConfig is a static-local singleton. Once constructed, m_visible
// is immutable. In the test environment DConfig::create typically returns an
// invalid/nullptr object, so the constructor falls back to m_visible = true.

TEST(SetWallpaperConfigTest, InstanceReturnsNonNull)
{
    SetWallpaperConfig *ptr = SetWallpaperConfig::instance();
    EXPECT_NE(nullptr, ptr);
}

TEST(SetWallpaperConfigTest, InstanceReturnsSamePointer)
{
    SetWallpaperConfig *ptr1 = SetWallpaperConfig::instance();
    SetWallpaperConfig *ptr2 = SetWallpaperConfig::instance();
    EXPECT_EQ(ptr1, ptr2);
}

TEST(SetWallpaperConfigTest, VisibleReturnsBool)
{
    SetWallpaperConfig *ptr = SetWallpaperConfig::instance();
    bool result = ptr->visible();
    (void)result;
    SUCCEED();
}

TEST(SetWallpaperConfigTest, VisibleDefaultTrueWhenDConfigUnavailable)
{
    // In the test environment DConfig is typically unavailable (no schema
    // installed / no D-Bus), so the constructor falls back to m_visible = true.
    SetWallpaperConfig *ptr = SetWallpaperConfig::instance();
    EXPECT_TRUE(ptr->visible());
}

TEST(SetWallpaperConfigTest, CopyConstructorIsDeleted)
{
    static_assert(!std::is_copy_constructible<SetWallpaperConfig>::value,
                  "SetWallpaperConfig should not be copy constructible");
    SUCCEED();
}
