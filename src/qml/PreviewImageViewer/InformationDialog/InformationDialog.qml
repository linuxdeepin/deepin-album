// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.image.viewer 1.0 as IV

DialogWindow {
    id: dialog

    property string fileName: FileControl.slotGetFileNameSuffix(filePath)
    property url filePath: GControl.currentSource
    property string dimensionsStr: imageInfo.width + "x" + imageInfo.height
    property int leftX: 20
    property int propFullWidth: 260
    property int propLeftWidth: 66
    property int propMidWidth: 106
    property int propRightWidth: 86
    property int topY: 70
    property bool dismiss: false

    flags: Qt.Dialog | Qt.WindowCloseButtonHint | Qt.MSWindowsFixedSizeDialogHint | Qt.WindowStaysOnTopHint
    maximumWidth: 280
    minimumWidth: 280
    visible: true
    width: 280

    header: DialogTitleBar {
        property string title: fileName

        enableInWindowBlendBlur: false

        content: Loader {
            sourceComponent: Label {
                anchors.centerIn: parent
                elide: Text.ElideMiddle
                font: DTK.fontManager.t6
                horizontalAlignment: Text.AlignHCenter
                text: title
                textFormat: Text.PlainText
                verticalAlignment: Text.AlignVCenter
            }
        }
    }

    Component.onCompleted: {
        x = window.x + window.width - width - leftX;
        y = window.y + topY;
    }

    // 窗口关闭时复位组件状态
    onClosing: {
        fileNameProp.reset();
        GStatus.showImageInfo = false;
    }

    onActiveChanged: {
        if (dismiss)
            close();
        dismiss = !dismiss
    }

    // 图片变更时复位组件状态(切换时关闭重命名框)
    onFileNameChanged: {
        fileNameProp.reset();
    }

    ColumnLayout {
        id: contentHeight

        width: 260

        anchors {
            horizontalCenter: parent.horizontalCenter
            margins: 10
        }

        PropertyItem {
            Accessible.name: "BasicInfo"
            Accessible.role: Accessible.List
            title: qsTr("Basic info")

            ColumnLayout {
                spacing: 1

                PropertyActionItemDelegate {
                    Accessible.name: "FileNameProp"
                    Accessible.role: Accessible.ListItem
                    id: fileNameProp

                    Layout.fillWidth: true
                    corners: RoundRectangle.TopCorner
                    description: fileName
                    editable: !FileControl.isTrashPath(filePath)
                    iconName: "action_edit"
                    implicitWidth: propFullWidth
                    title: qsTr("File name")
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 1

                    PropertyItemDelegate {
                        Accessible.name: "Size"
                        Accessible.role: Accessible.ListItem
                        contrlImplicitWidth: propLeftWidth
                        corners: RoundRectangle.BottomLeftCorner
                        description: FileControl.slotGetInfo("FileSize", filePath)
                        title: qsTr("Size")
                    }

                    PropertyItemDelegate {
                        Accessible.name: "Dimensions"
                        Accessible.role: Accessible.ListItem
                        Layout.fillWidth: true
                        contrlImplicitWidth: propMidWidth
                        description: {
                            var originalDim = FileControl.slotGetInfo("OriginalDimension", filePath);
                            if (originalDim && originalDim !== "-") {
                                return originalDim;
                            } else {
                                return dimensionsStr;
                            }
                        }
                        title: qsTr("Dimensions")
                    }

                    PropertyItemDelegate {
                        Accessible.name: "Type"
                        Accessible.role: Accessible.ListItem
                        contrlImplicitWidth: propRightWidth
                        corners: RoundRectangle.BottomRightCorner
                        description: FileControl.slotFileSuffix(filePath, false)
                        title: qsTr("Type")
                    }
                }
            }

            ColumnLayout {
                spacing: 1

                PropertyItemDelegate {
                    Accessible.name: "DateCaptured"
                    Accessible.role: Accessible.ListItem
                    Layout.fillWidth: true
                    corners: RoundRectangle.TopCorner
                    description: FileControl.slotGetInfo("DateTimeOriginal", filePath)
                    title: qsTr("Date captured")
                }

                PropertyItemDelegate {
                    Accessible.name: "DateModified"
                    Accessible.role: Accessible.ListItem
                    Layout.fillWidth: true
                    corners: RoundRectangle.BottomCorner
                    description: FileControl.slotGetInfo("DateTimeDigitized", filePath)
                    title: qsTr("Date modified")
                }
            }
        }

        PropertyItem {
            Accessible.name: "DetailInfoItem"
            Accessible.role: Accessible.List
            id: detailInfoItem

            // 详细信息默认不显示，会影响自动布局效果，因此目前设置为固定布局
            showProperty: false
            title: qsTr("Details")

            GridLayout {
                Layout.fillWidth: true
                columnSpacing: 1
                columns: 3
                rowSpacing: 1
                rows: 4

                PropertyItemDelegate {
                    Accessible.name: "Aperture"
                    Accessible.role: Accessible.ListItem
                    contrlImplicitWidth: propLeftWidth
                    corners: RoundRectangle.TopLeftCorner
                    description: FileControl.slotGetInfo("ApertureValue", filePath)
                    title: qsTr("Aperture")
                }

                PropertyItemDelegate {
                    Accessible.name: "ExposureProgram"
                    Accessible.role: Accessible.ListItem
                    Layout.fillWidth: true
                    Layout.minimumWidth: propMidWidth
                    contrlImplicitWidth: propMidWidth
                    description: FileControl.slotGetInfo("ExposureProgram", filePath)
                    title: qsTr("Exposure program")
                }

                PropertyItemDelegate {
                    Accessible.name: "FocalLength"
                    Accessible.role: Accessible.ListItem
                    contrlImplicitWidth: propRightWidth
                    corners: RoundRectangle.TopRightCorner
                    description: FileControl.slotGetInfo("FocalLength", filePath)
                    title: qsTr("Focal length")
                }

                PropertyItemDelegate {
                    Accessible.name: "Iso"
                    Accessible.role: Accessible.ListItem
                    contrlImplicitWidth: propLeftWidth
                    description: FileControl.slotGetInfo("ISOSpeedRatings", filePath)
                    title: qsTr("ISO")
                }

                PropertyItemDelegate {
                    Accessible.name: "ExposureMode"
                    Accessible.role: Accessible.ListItem
                    Layout.fillWidth: true
                    contrlImplicitWidth: propMidWidth
                    description: FileControl.slotGetInfo("ExposureMode", filePath)
                    title: qsTr("Exposure mode")
                }

                PropertyItemDelegate {
                    Accessible.name: "ExposureTime"
                    Accessible.role: Accessible.ListItem
                    contrlImplicitWidth: propRightWidth
                    description: FileControl.slotGetInfo("ExposureTime", filePath)
                    title: qsTr("Exposure time")
                }

                PropertyItemDelegate {
                    Accessible.name: "Flash"
                    Accessible.role: Accessible.ListItem
                    contrlImplicitWidth: propLeftWidth
                    description: FileControl.slotGetInfo("Flash", filePath)
                    title: qsTr("Flash")
                }

                PropertyItemDelegate {
                    Accessible.name: "FlashCompensation"
                    Accessible.role: Accessible.ListItem
                    Layout.fillWidth: true
                    contrlImplicitWidth: propMidWidth
                    description: FileControl.slotGetInfo("FlashExposureComp", filePath)
                    title: qsTr("Flash compensation")
                }

                PropertyItemDelegate {
                    Accessible.name: "MaxAperture"
                    Accessible.role: Accessible.ListItem
                    contrlImplicitWidth: propRightWidth
                    description: FileControl.slotGetInfo("MaxApertureValue", filePath)
                    title: qsTr("Max aperture")
                }

                PropertyItemDelegate {
                    Accessible.name: "Colorspace"
                    Accessible.role: Accessible.ListItem
                    contrlImplicitWidth: propLeftWidth
                    corners: RoundRectangle.BottomLeftCorner
                    description: FileControl.slotGetInfo("ColorSpace", filePath)
                    title: qsTr("Colorspace")
                }

                PropertyItemDelegate {
                    Accessible.name: "MeteringMode"
                    Accessible.role: Accessible.ListItem
                    Layout.fillWidth: true
                    contrlImplicitWidth: propMidWidth
                    description: FileControl.slotGetInfo("MeteringMode", filePath)
                    title: qsTr("Metering mode")
                }

                PropertyItemDelegate {
                    Accessible.name: "WhiteBalance"
                    Accessible.role: Accessible.ListItem
                    contrlImplicitWidth: propRightWidth
                    corners: RoundRectangle.BottomRightCorner
                    description: FileControl.slotGetInfo("WhiteBalance", filePath)
                    title: qsTr("White balance")
                }
            }

            ColumnLayout {
                spacing: 1

                PropertyItemDelegate {
                    Accessible.name: "DeviceModel"
                    Accessible.role: Accessible.ListItem
                    contrlImplicitWidth: propFullWidth
                    corners: RoundRectangle.AllCorner
                    description: FileControl.slotGetInfo("Model", filePath)
                    title: qsTr("Device model")
                }

                PropertyItemDelegate {
                    Accessible.name: "LensModel"
                    Accessible.role: Accessible.ListItem
                    contrlImplicitWidth: propFullWidth
                    corners: RoundRectangle.AllCorner
                    description: FileControl.slotGetInfo("LensType", filePath)
                    title: qsTr("Lens model")
                }
            }
        }

        // 隐藏占位组件
        Item {
            id: footer

            Layout.preferredHeight: detailInfoItem.showProperty ? 5 : 10
            width: 10
        }
    }

    // DialogWindow 无法直接包含 ImageInfo
    Item {
        IV.ImageInfo {
            id: imageInfo

            frameIndex: GControl.currentFrameIndex
            source: filePath

            onStatusChanged: {
                if (status === IV.ImageInfo.Ready) {
                    dimensionsStr = imageInfo.width + "x" + imageInfo.height
                }
            }
        }
    }
}
