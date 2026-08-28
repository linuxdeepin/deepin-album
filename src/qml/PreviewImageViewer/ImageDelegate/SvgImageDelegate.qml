// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.deepin.image.viewer 1.0 as IV

import "../Utils"

BaseImageDelegate {
    Accessible.name: "SvgImageDelegate"
    Accessible.role: Accessible.ListItem
    id: delegate

    status: image.status
    targetImage: image
    inputHandler: imageInput

    Image {
        id: image

        height: delegate.height
        width: delegate.width
        asynchronous: true
        cache: false
        clip: true
        fillMode: Image.PreserveAspectFit
        mipmap: true
        smooth: true
        scale: 1.0
        source: delegate.source
        // 设置为当前图像的图片源，调整因缩放导致的模糊
        sourceSize: Qt.size(width, height)
    }

    ImageInputHandler {
        Accessible.name: "SvgImageInput"
        Accessible.role: Accessible.Client
        id: imageInput

        anchors.fill: parent
        targetImage: image.status === Image.Ready ? image : null
    }
}
