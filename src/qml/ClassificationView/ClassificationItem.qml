// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import org.deepin.dtk 1.0
import org.deepin.image.viewer 1.0 as IV

import org.deepin.album 1.0 as Album

/**
 * @brief 分类项组件
 * 显示单个分类的缩略图、名称和图片数量
 */
Control {
    id: classificationItem

    property string classificationName: ""
    property int imageCount: 0
    property string thumbnailPath: ""
    property bool isHovered: mouseArea.containsMouse

    signal clicked()

    background: Rectangle {
        radius: 8
        color: classificationItem.isHovered ? Qt.rgba(0.5, 0.5, 0.5, 0.1) : "transparent"
        border.color: Qt.rgba(0.8, 0.8, 0.8, 0.3)
        border.width: classificationItem.isHovered ? 1 : 0

        Behavior on color {
            ColorAnimation { duration: 200 }
        }

        Behavior on border.width {
            NumberAnimation { duration: 200 }
        }
    }

    contentItem: Item {
        // 背景图片
        Rectangle {
            id: imageBackground
            anchors {
                fill: parent
                margins: 8
            }
            radius: 6
            color: Qt.rgba(0.95, 0.95, 0.95, 1.0)
            clip: true

            Image {
                id: thumbnailImage
                anchors.fill: parent
                asynchronous: true
                cache: true
                fillMode: Image.PreserveAspectCrop
                source: classificationItem.thumbnailPath ? "file://" + classificationItem.thumbnailPath : ""
                visible: status === Image.Ready

                onStatusChanged: {
                    if (status === Image.Error) {
                        console.warn("Failed to load thumbnail for classification:", classificationItem.classificationName, "path:", source);
                        // Try using ThumbnailLoad provider as fallback
                        if (classificationItem.thumbnailPath) {
                            source = "image://ThumbnailLoad/" + classificationItem.thumbnailPath;
                        }
                    } else if (status === Image.Ready) {
                        console.debug("Successfully loaded thumbnail for classification:", classificationItem.classificationName);
                    }
                }

                /**
                 * @brief Reset image source to trigger reload
                 * @details Useful for handling rotation or source changes
                 */
                function reset() {
                    var temp = source;
                    source = "";
                    source = temp;
                }
            }

            /**
             * @brief Fallback icon when thumbnail loading fails
             * @details Displayed when image loading fails completely
             */
            DciIcon {
                id: fallbackIcon
                anchors.centerIn: parent
                name: "image-missing"
                sourceSize: Qt.size(48, 48)
                theme: DTK.themeType
                palette: DTK.makeIconPalette(parent.palette)
                visible: thumbnailImage.status === Image.Error && thumbnailImage.source != ""
            }

            // 渐变遮罩用于文字显示
            Rectangle {
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                height: 60
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.6) }
                }
                radius: 6
            }
        }

        // 文字信息
        Column {
            anchors {
                left: parent.left
                right: parent.right
                bottom: parent.bottom
                margins: 16
            }
            spacing: 4

            Text {
                id: nameText
                text: classificationItem.classificationName
                font.pixelSize: 14
                font.weight: Font.Medium
                color: "white"
                elide: Text.ElideRight
                width: parent.width
            }

            Text {
                id: countText
                text: qsTr("%1 photos").arg(classificationItem.imageCount)
                font.pixelSize: 12
                color: Qt.rgba(1, 1, 1, 0.8)
                elide: Text.ElideRight
                width: parent.width
            }
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onClicked: {
            classificationItem.clicked()
        }
    }
}
