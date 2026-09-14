# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.13)

# ---- 确保依赖包在测试作用域可见 ----
find_package(GTest REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} CONFIG REQUIRED COMPONENTS
    Core Gui DBus Concurrent Sql Svg PrintSupport Test
)
find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget Gui Declarative)
find_package(PkgConfig REQUIRED)
pkg_check_modules(3rd_lib REQUIRED libavformat)
pkg_check_modules(dfmmount REQUIRED dfm${DTK_VERSION_MAJOR}-mount)

# ---- 收集生产源码（排除 main.cpp） ----
file(GLOB_RECURSE ALBUM_SRCS
    "${CMAKE_SOURCE_DIR}/src/src/*.cpp"
)
list(FILTER ALBUM_SRCS EXCLUDE REGEX ".*main\\.cpp$")

# ---- stub-shadow 源 ----
set(STUB_SHADOW "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/stub-shadow.cpp")

# ---- 测试目标 ----
add_executable(test_thumbnaillistview
    ${ALBUM_SRCS}
    ${STUB_SHADOW}
    test_ThumbnailListView.cpp
)

target_include_directories(test_thumbnaillistview PRIVATE
    "${CMAKE_SOURCE_DIR}/src/src"
    "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub"
    ${3rd_lib_INCLUDE_DIRS}
    ${dfmmount_INCLUDE_DIRS}
)

target_link_libraries(test_thumbnaillistview PRIVATE
    GTest::gtest
    GTest::gtest_main
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::DBus
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::PrintSupport
    Qt${QT_VERSION_MAJOR}::Test
    Dtk${DTK_VERSION_MAJOR}::Widget
    Dtk${DTK_VERSION_MAJOR}::Gui
    Dtk${DTK_VERSION_MAJOR}::Declarative
    ${3rd_lib_LIBRARIES}
    ${dfmmount_LIBRARIES}
    GL
    pthread
)

# ---- GTest 发现 ----
include(GoogleTest)
gtest_discover_tests(test_thumbnaillistview
    PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
)
