// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TYPES_H
#define TYPES_H

#include <QObject>

#include "unionimage/unionimage_global.h"
class Types : public QObject
{
    Q_OBJECT
    Q_ENUMS(ItemType)
    Q_ENUMS(ModelType)

    Q_ENUMS(ItemRole)
    Q_ENUMS(ImageType)
    Q_ENUMS(StackPage)
public:
    explicit Types(QObject *parent = nullptr);
    ~Types() override;

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

    enum ItemRole {
        ImageUrlRole = Qt::UserRole + 1,  ///< 图片路径
        FrameIndexRole,                   ///< 图片帧索引
        ImageAngleRole,                   ///< 图片旋转角度
    };

    /**
       @brief 图片文件类型
     */
    enum ImageType {
        NullImage,      ///< 无图片信息，文件为空
        NormalImage,    ///< 静态图片
        DynamicImage,   ///< 动态图
        SvgImage,       ///< SVG图片
        MultiImage,     ///< 多页图
        DamagedImage,   ///< 损坏图片
        NonexistImage,  ///< 图片文件不存在，删除、移动等
    };

    /**
       @brief 界面类型
     */
    enum StackPage {
        OpenImagePage,   ///< 默认打开窗口界面
        ImageViewPage,   ///< 图片展示界面(含缩略图栏)
        SliderShowPage,  ///< 图片动画展示界面
    };
};

#endif  // TYPES_H
