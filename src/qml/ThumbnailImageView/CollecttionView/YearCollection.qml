// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import org.deepin.dtk 1.0
import "../../Control"
import "../../Control/ListView"
import "../../"

Item {
    id: root

    signal yearClicked(string year)

    function flushModel() {
        //0.清理
        theModel.clear()

        //1.获取年份
        var yearArray = albumControl.getYears()

        //2.获取item count并构建model
        for(var i = 0;i !== yearArray.length;++i) {
            var itemCount = albumControl.getYearCount(yearArray[i])
            theModel.append({year: yearArray[i], itemCount: itemCount})
        }
    }

    ListModel {
        id: theModel
    }

    ListView {
        property double displayFlushHelper: 0

        id: theView
        model: theModel
        clip: true
        delegate: theDelegate
        spacing: 20

        width: parent.width / 3 * 2
        height: parent.height + global.statusBarHeight - global.collectionTopMargin
        anchors.top: parent.top
        anchors.topMargin: global.collectionTopMargin
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Component {
        id: theDelegate

        Rectangle {
            width: theView.width
            height: theView.height / 3 * 2
            radius: 18

            //圆角遮罩Rectangle
            Rectangle {
                id: maskRec
                anchors.centerIn: parent
                width: image.width
                height: image.height

                color:"transparent"
                Rectangle {
                    anchors.centerIn: parent
                    width: image.paintedWidth
                    height: image.paintedHeight
                    color:"black"
                    radius: 18
                }
                visible: false
            }

            Image {
                id: image
                //因为新版本的Qt的图片缓存机制，导致相同路径的图片只会加载一次,source要改变才能刷新图片，所以尾部添加itemCount。如果需要数量相同也能刷新，则在尾部添加随机数
                source: "image://collectionPublisher/" + theView.displayFlushHelper.toString() + "_Y_" + year + "_0" + "_" + itemCount
                asynchronous: true
                anchors.fill: parent
                width: parent.width
                height: parent.height
                fillMode: Image.PreserveAspectCrop
                visible: false
            }

            //遮罩执行
            OpacityMask {
                id: mask
                anchors.fill: image
                source: image
                maskSource: maskRec
            }

            //渐变阴影
            //颜色格式为ARGB
            Rectangle {
                anchors.top: image.top
                anchors.left: image.left
                radius: 18
                width: image.width
                height: yearLabel.height + yearLabel.anchors.topMargin + itemCountLabel.height + itemCountLabel.anchors.topMargin + 5
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "#61000000" }
                    GradientStop { position: 1.0; color: "#00000000" }
                }
            }

            Label {
                id: yearLabel
                font: DTK.fontManager.t3
                text: qsTr("%1").arg(year)
                color: "#FFFFFF"
                anchors.top: image.top
                anchors.topMargin: 25
                anchors.left: image.left
                anchors.leftMargin: 25
            }

            Label {
                id: itemCountLabel
                font: DTK.fontManager.t6
                text: itemCount > 1 ? qsTr("%1 items").arg(itemCount) : qsTr("1 item") //itemCount为0的时候不会显示出来
                color: yearLabel.color
                anchors.top: yearLabel.bottom
                anchors.topMargin: 5
                anchors.left: yearLabel.left
            }

            MouseArea {
                anchors.fill: parent
                onClicked: { //double click 切换动画不生效
                    yearClicked(year)
                    forceActiveFocus()
                }
            }
        }
    }

    Component.onCompleted: {
        flushModel()
        global.sigFlushAllCollectionView.connect(flushModel)
    }
}
