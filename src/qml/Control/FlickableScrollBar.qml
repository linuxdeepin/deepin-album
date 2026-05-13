// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

// 适用于纯 QML Flickable/ListView 的 ScrollBar 封装
// 内置绑定循环防护和拖拽冲突处理
ScrollBar {
    id: control

    required property Flickable flickable

    orientation: Qt.Vertical
    policy: ScrollBar.AsNeeded
    height: flickable.height
    anchors.right: flickable.parent.right
    anchors.top: flickable.parent.top
    active: flickable.moving || control.hovered || control.pressed
    size: flickable.visibleArea.heightRatio
    visible: flickable.contentHeight > flickable.height
    snapMode: ScrollBar.NoSnap
    minimumSize: 0.1

    property real viewPos: flickable.visibleArea.yPosition
    onViewPosChanged: { if (!pressed) position = viewPos }

    onPressedChanged: {
        if (pressed) {
            flickable.cancelFlick()
            flickable.interactive = false
        } else {
            flickable.interactive = true
        }
    }

    onPositionChanged: {
        if (pressed) {
            var maxScroll = flickable.contentHeight - flickable.height
            if (maxScroll > 0)
                flickable.contentY = Math.max(0, Math.min(position * maxScroll, maxScroll))
        }
    }
}
