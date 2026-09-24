# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later

# ---- 通用 GTest 目标构建函数 ----
# 用法: add_album_gtest(<target> <test sources...>)
function(add_album_gtest target)
    # 确保依赖包在测试作用域可见
    find_package(GTest REQUIRED)
    find_package(Qt${QT_VERSION_MAJOR} CONFIG REQUIRED COMPONENTS
        Core Gui DBus Concurrent Sql Svg PrintSupport Test
    )
    find_package(Dtk${DTK_VERSION_MAJOR} REQUIRED COMPONENTS Widget Gui Declarative)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(3rd_lib REQUIRED libavformat)
    pkg_check_modules(dfmmount REQUIRED dfm${DTK_VERSION_MAJOR}-mount)

    # 收集生产源码（排除 main.cpp）
    file(GLOB_RECURSE ALBUM_SRCS
        "${CMAKE_SOURCE_DIR}/src/src/*.cpp"
    )
    list(FILTER ALBUM_SRCS EXCLUDE REGEX ".*main\\.cpp$")

    # stub-shadow 源
    set(STUB_SHADOW "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub/stub-shadow.cpp")

    add_executable(${target}
        ${ALBUM_SRCS}
        ${STUB_SHADOW}
        ${ARGN}
    )

    target_compile_definitions(${target} PRIVATE BUILD_TESTING)

    target_include_directories(${target} PRIVATE
        "${CMAKE_SOURCE_DIR}/src/src"
        "${CMAKE_SOURCE_DIR}/tests/3rdparty/stub"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        ${3rd_lib_INCLUDE_DIRS}
        ${dfmmount_INCLUDE_DIRS}
    )

    target_link_libraries(${target} PRIVATE
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

    include(GoogleTest)
    gtest_discover_tests(${target}
        PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endfunction()

# gtest: 使用 DAppLoader 加载本项目生成的 LIB
add_subdirectory(dapploader)

# AlbumControl 单元测试
add_subdirectory(albumcontrol)

# ThumbnailListView 单元测试
add_subdirectory(thumbnaillistview)

# ThumbnailModel / Positioner 单元测试
add_subdirectory(thumbnailview)

# DBManager 单元测试
add_subdirectory(dbmanager)

# UnionImage 单元测试
add_subdirectory(unionimage)

# ImageSourceModel / PathViewProxyModel 单元测试
add_subdirectory(imagedata)

# ImageDataModel 单元测试
add_subdirectory(imagedatamodel)

# GlobalStatus 单元测试
add_subdirectory(globalstatus)

# MouseEventListener 单元测试
add_subdirectory(mouseevent)

# ReadThumbnailManager 单元测试
add_subdirectory(readthumbnail)

# DeviceHelper 单元测试
add_subdirectory(devicehelper)

# FileTrashHelper 单元测试
add_subdirectory(filetrashhelper)

# FileControl 单元测试
add_subdirectory(filecontrol)

# FileInotify 单元测试
add_subdirectory(fileinotify)

# MovieService 单元测试
add_subdirectory(movieservice)
