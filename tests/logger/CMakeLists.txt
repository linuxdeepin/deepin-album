# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later

# logger.cpp lives in src/ (not src/src/), so it is not picked up by the
# GLOB_RECURSE in add_album_gtest.  Add it explicitly.
add_album_gtest(test_logger
    ${CMAKE_SOURCE_DIR}/src/logger.cpp
    test_logger.cpp
)
