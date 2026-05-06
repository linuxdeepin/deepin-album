// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.deepin.image.viewer 1.0 as IV
import org.deepin.dtk 1.0 as DTK

Item {
    id: thumbnailImage

    property alias frameCount: imageInfo.frameCount
    property int frameIndex: 0
    property alias image: contentImage
    property url source
    property int status: imageInfo.status
    property int type

    function reset() {
        var temp = contentImage.source;
        contentImage.source = "";
        contentImage.source = temp;
        contentImage.rotation = 0;
    }

    clip: true

    Image {
        id: contentImage

        anchors.centerIn: thumbnailImage
        asynchronous: true
        cache: false
        fillMode: Image.PreserveAspectCrop
        height: thumbnailImage.height
        // 用于在旋转过程中不显示白边，图片大小比例 1 -> sqrt(2) -> 1
        scale: {
            if (0 === rotation) {
                return 1;
            }
            var normalRotation = (Math.abs(rotation) % 90);
            var ratio = (normalRotation < 45) ? (normalRotation / 45) : ((90 - normalRotation) / 45);
            return 1 + ((Math.sqrt(2) - 1) * ratio);
        }
        smooth: true
        width: thumbnailImage.width
        visible: !damagedIcon.visible
    }

    // 使用 DCI 图标显示损坏图片，自动适应主题
    DTK.DciIcon {
        id: damagedIcon
        anchors.centerIn: thumbnailImage
        width: Math.min(thumbnailImage.width, thumbnailImage.height) * 0.6
        height: width
        name: "photo_breach"
        theme: DTK.themeType
        visible: imageInfo.status === IV.ImageInfo.Error && !imageInfo.hasCachedThumbnail
    }

    // 使用 ImageInfo 控制加载状态
    IV.ImageInfo {
        id: imageInfo

        frameIndex: thumbnailImage.frameIndex
        source: thumbnailImage.source

        onStatusChanged: {
            var imageSource = "image://ThumbnailLoad/" + thumbnailImage.source + "#frame_" + thumbnailImage.frameIndex;
            if (IV.ImageInfo.Error === imageInfo.status) {
                if (imageInfo.hasCachedThumbnail) {
                    contentImage.source = imageSource;
                }
            } else if (IV.ImageInfo.Ready === imageInfo.status) {
                contentImage.source = imageSource;
            }

            // 更新类型，不直接绑定
            thumbnailImage.type = imageInfo.type;
        }
    }
}
