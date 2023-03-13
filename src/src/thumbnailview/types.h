// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ITEMTYPES_H
#define ITEMTYPES_H
#include <QObject>

#include "unionimage/unionimage_global.h"
class Types : public QObject
{
    Q_OBJECT
    Q_ENUMS(ItemType)
    Q_ENUMS(ModelType)
public:
    using QObject::QObject;
    ~Types() = default;

    enum ItemType { All = 0, Picture, Video };
    enum ModelType {
        Normal = 0,       // 常规视图model （大多缩略图使用该model）
        DayCollecttion,  // 合集日视图model
        AllCollection,   // 合集所有项目视图model
        HaveImported,    // 已导入视图数据model
        RecentlyDeleted, // 最近删除视图model
        CustomAlbum,     // 自定义相册视图model
        SearchResult,    // 搜索结果视图model
        Device           // 设备视图mode
    };
};

#endif
