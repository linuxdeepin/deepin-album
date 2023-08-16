// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import org.deepin.dtk 1.0
import org.deepin.image.viewer 1.0 as IV

Rectangle {
    property string iconName: "deepin-image-viewer"
    property bool animationShow: true

    onAnimationShowChanged: {
        y = animationShow ? 0 : -GStatus.titleHeight
    }

    anchors.top: window.top
    width: window.width
    height: GStatus.titleHeight
    visible: !window.isFullScreen
    color: titlecontrol.ColorSelector.backgroundColor
    gradient: Gradient {
        GradientStop {
            position: 0.0
            color: titlecontrol.ColorSelector.backgroundColor1
        }
        GradientStop {
            position: 1.0
            color: titlecontrol.ColorSelector.backgroundColor2
        }
    }

    Control {
        id: titlecontrol
        hoverEnabled: true // 开启 Hover 属性
        property Palette backgroundColor1: Palette {
            normal: Qt.rgba(255 / 255, 255 / 255, 255 / 255, 0.6)
            normalDark: Qt.rgba(26 / 255, 26 / 255, 26 / 255, 0.6)
        }
        property Palette backgroundColor2: Palette {
            normal: Qt.rgba(255 / 255, 255 / 255, 255 / 255, 0.02)
            normalDark: Qt.rgba(26 / 255, 26 / 255, 26 / 255, 0.02)
        }
    }

    ActionButton {
        anchors.top: parent.top
        anchors.topMargin: GStatus.actionMargin
        anchors.left: parent.left
        anchors.leftMargin: GStatus.actionMargin
        icon {
            name: iconName
            width: 32
            height: 32
        }
    }

    // 捕获标题栏部分鼠标事件，部分事件将穿透，由底层 ApplicationWindow 处理
    IV.MouseTrackItem {
        id: trackItem
        anchors.fill: parent

        onPressedChanged: {
            // 点击标题栏时屏蔽动画计算效果
            GStatus.animationBlock = pressed
        }

        onDoubleClicked: {
            // 切换窗口最大化状态
            title.toggleWindowState()
        }
    }

    TitleBar {
        id: title
        anchors.fill: parent

        windowButtonGroup: WindowButtonGroupEx {
            Layout.alignment: Qt.AlignRight
            Layout.fillHeight: true
            embedMode: title.embedMode
            textColor: title.textColor
            fullScreenButtonVisible: title.fullScreenButtonVisible
            Component.onCompleted: {
                title.toggleWindowState.connect(maxOrWinded)
            }
        }

        menu: Menu {
            ThemeMenu {
            }
            MenuSeparator {
            }
            HelpAction {
            }
            AboutAction {
                aboutDialog: AboutDialog {
                    icon: !fileControl.isAlbum() ? "deepin-image-viewer" : "deepin-album"
                    width:400
                    modality:Qt.NonModal
                    version:qsTr(String("Version: %1").arg(Qt.application.version))
                    description: !fileControl.isAlbum() ? "Image Viewer is an image viewing tool with fashion interface and smooth performance."
                                                        : qsTr("Album is a stylish management tool for viewing and organizing photos and videos.")
                    productName: !fileControl.isAlbum() ? "Image Viewer"
                                                        : qsTr("Album")
                    websiteName:DTK.deepinWebsiteName
                    websiteLink:DTK.deepinWebsitelLink
                    license:qsTr(String("%1 is released under %2.").arg(productName).arg("GPLV3"))
                }
            }

            onVisibleChanged: {
                GStatus.animationBlock = visible
            }
        }

        // 使用自定的文本
        content: Loader {
            active: true

            sourceComponent: Label {
                textFormat: Text.PlainText
                text: title.title
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                // 自动隐藏多余文本
                elide: Text.ElideRight
            }
        }
    }

    Behavior on y {
        enabled: visible
        NumberAnimation {
            duration: 200
            easing.type: Easing.InOutQuad
        }
    }
}
