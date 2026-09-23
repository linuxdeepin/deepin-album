# SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
# SPDX-License-Identifier: GPL-3.0-or-later

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
