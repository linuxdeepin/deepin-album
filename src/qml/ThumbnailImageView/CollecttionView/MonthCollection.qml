// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import QtGraphicalEffects 1.0
import org.deepin.dtk 1.0
import "../../Control"
import "../../Control/ListView"
import "../../"

Item {
    id: monthView

    signal monthClicked(string year, string month)

    function scrollToYear(year) {
        //搜索index
        for(var i = 0;i !== theModel.count;++i) {
            if(theModel.get(i).year === year) {
                break
            }
        }

        //计算偏移
        theView.contentY = (theView.height / 3 * 2 + theView.spacing) * i
    }

    function flushModel() {
        if (!visible)
            return
        //0.清理
        theModel.clear()

        //1.获取月份
        var monthArray = albumControl.getMonths()
        var yearMonthArray = new Array
        for(var j = 0;j !== monthArray.length;++j) {
            var data = monthArray[j].split("-")
            yearMonthArray.push(data)
        }

        //2.获取item count并构建model
        for(var i = 0;i !== monthArray.length;++i) {
            var itemCount = albumControl.getMonthCount(yearMonthArray[i][0], yearMonthArray[i][1])
            theModel.append({year: yearMonthArray[i][0], month: yearMonthArray[i][1], itemCount: itemCount})
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
        height: parent.height + GStatus.statusBarHeight - GStatus.collectionTopMargin
        anchors {
            top: parent.top
            topMargin: GStatus.collectionTopMargin
            horizontalCenter: parent.horizontalCenter
        }
    }

    Component {
        id: theDelegate

        Item {
            width: theView.width
            height: theView.height / 3 * 2

            //圆角遮罩Rectangle
            Rectangle {
                id: maskRec
                anchors.centerIn: parent
                width: image.width
                height: image.height

                color:"transparent"
                Rectangle {
                    anchors.centerIn: parent
                    width: image.width
                    height: image.height
                    color:"black"
                    radius: 18
                }
                visible: false
            }

            Image {
                id: image
                //因为新版本的Qt的图片缓存机制，导致相同路径的图片只会加载一次,source要改变才能刷新图片，所以尾部添加itemCount。如果需要数量相同也能刷新，则在尾部添加随机数
                source: "image://collectionPublisher/" + theView.displayFlushHelper.toString() + "_M_" + year + "_" + month + "_" + itemCount
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

            // 边框阴影立体效果
            DropShadow {
                anchors.fill: mask
                z: 0

                verticalOffset: 1

                radius: 5
                samples: radius * 2 + 1
                spread: 0.3

                color: "black"

                opacity: 0.3

                source: mask

                visible: true
            }

            Label {
                id: monthLabel
                font: DTK.fontManager.t3
                text: qsTr("%1/%2").arg(year).arg(Number(month))
                color: "#FFFFFF"
                anchors {
                    top: image.top
                    topMargin: 25
                    left: image.left
                    leftMargin: 25
                }
            }

            Rectangle {
                id: itemCountLabel
                visible: itemCount > 6
                color: "#000000"
                radius: 20
                opacity: 0.7
                width: 60
                height: 30

                Text {
                    anchors.centerIn: parent
                    text: itemCount - 6 < 99 ? itemCount - 6 : "99+"
                    color: "#FFFFFF"
                }

                anchors {
                    right: parent.right
                    rightMargin: 10
                    bottom: parent.bottom
                    bottomMargin: 10
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    monthClicked(year, month)
                    forceActiveFocus()
                }
            }
        }
    }

    Component.onCompleted: {
        GStatus.sigFlushAllCollectionView.connect(flushModel)
    }
}
