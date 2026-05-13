// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.deepin.dtk 1.0

// 适用于 QmlWidget 嵌入 QWidget 的滚动条组件
// 样式参数与 DTK ScrollBar 一致（FlowStyle.qml）
Rectangle {
    id: root

    anchors.right: parent.right
    anchors.top: parent.top
    anchors.bottom: parent.bottom
    width: GStatus.verticalScrollBarWidth
    color: "transparent"

    // 由 QmlWidget 暴露的属性驱动
    required property real contentRatio
    required property real scrollPosition

    // 可配置样式参数（与 DTK FlowStyle.qml 一致）
    property int trackMargin: 2
    property int handleWidthNormal: 4
    property int handleWidthActive: 12
    property int handleMinHeight: 30
    property int hideDelay: 450
    property int fadeOutDuration: 1500
    property color handleColorLight: Qt.rgba(0, 0, 0, 0.5)
    property color handleColorDark: Qt.rgba(1, 1, 1, 0.3)

    property bool needScroll: contentRatio < 0.99
    property real trackHeight: height - trackMargin * 2
    property bool showHandle: false
    property bool hovered: trackMouse.containsMouse
    property bool pressed: trackMouse.drag.active
    property int handleWidth: (hovered || pressed) ? handleWidthActive : handleWidthNormal
    property real _lastDragPos: 0

    property real handleHeight: Math.max(handleMinHeight, trackHeight * contentRatio)
    property real handleY: trackMargin + scrollPosition * (trackHeight - handleHeight)
    onHandleYChanged: { if (!trackMouse.drag.active) scrollHandle.y = handleY }

    onScrollPositionChanged: {
        if (needScroll && !trackMouse.drag.active) {
            showHandle = true
        }
    }

    Timer {
        id: hideTimer
        interval: root.hideDelay
        onTriggered: {
            if (!trackMouse.containsMouse && !trackMouse.drag.active)
                root.showHandle = false
        }
    }

    onShowHandleChanged: {
        if (showHandle) {
            hideTimer.stop()
            hideTimer.start()
        }
    }

    MouseArea {
        id: trackMouse
        anchors.fill: parent
        hoverEnabled: true
        drag.target: scrollHandle
        drag.axis: Drag.YAxis
        drag.minimumY: root.trackMargin
        drag.maximumY: root.trackHeight - scrollHandle.height + root.trackMargin

        onContainsMouseChanged: {
            if (containsMouse) {
                root.showHandle = true
                hideTimer.stop()
            } else if (!drag.active) {
                hideTimer.restart()
            }
        }

        onPositionChanged: {
            if (drag.active) {
                var maxTravel = root.trackHeight - scrollHandle.height
                var newPos = maxTravel > 0 ? (scrollHandle.y - root.trackMargin) / maxTravel : 0
                if (Math.abs(newPos - root._lastDragPos) > 0.001) {
                    root._lastDragPos = newPos
                    root.scrollPositionChangedFromDrag(newPos)
                }
            }
        }
    }

    Rectangle {
        id: scrollHandle
        opacity: 0.0
        visible: root.needScroll && scrollHandle.opacity > 0
        anchors.right: parent.right
        anchors.rightMargin: root.trackMargin
        width: root.handleWidth
        height: root.handleHeight
        radius: width / 2

        color: DTK.themeType === ApplicationHelper.LightType
               ? root.handleColorLight
               : root.handleColorDark

        Connections {
            target: root
            function onShowHandleChanged() {
                if (root.showHandle) {
                    scrollHandle.opacity = 1.0
                    fadeOutAnim.stop()
                } else {
                    fadeOutAnim.start()
                }
            }
        }

        NumberAnimation {
            id: fadeOutAnim
            target: scrollHandle
            property: "opacity"
            from: 1.0
            to: 0.0
            duration: root.fadeOutDuration
        }

        Behavior on width { NumberAnimation { duration: 150 } }
    }

    signal scrollPositionChangedFromDrag(real pos)
}
