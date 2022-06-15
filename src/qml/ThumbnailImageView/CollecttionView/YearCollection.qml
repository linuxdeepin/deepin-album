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
        height: parent.height
        anchors.top: parent.top
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
                source: "image://collectionPublisher/" + theView.displayFlushHelper.toString() + "_Y_" + year + "_0"
                asynchronous: true
                anchors.fill: parent
                width: parent.width
                height: parent.height
                //使用PreserveAspectFit确保在原始比例下不变形
                fillMode: Image.PreserveAspectFit
                clip: true
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
                text: year + qsTr("年")
                color: "#FFFFFF"
                anchors.top: image.top
                anchors.topMargin: 25
                anchors.left: image.left
                anchors.leftMargin: 25
            }

            Label {
                id: itemCountLabel
                font: DTK.fontManager.t6
                text: itemCount + qsTr("项")
                color: yearLabel.color
                anchors.top: yearLabel.bottom
                anchors.topMargin: 5
                anchors.left: yearLabel.left
            }
        }
    }

    Component.onCompleted: {
        var yearArray = new Array
        var countArray = new Array
        //1.获取年份
        yearArray = albumControl.getYears()

        //2.获取item count并构建model
        for(var i = 0;i != yearArray.length;++i) {
            var itemCount = albumControl.getYearCount(yearArray[i])
            theModel.append({year: yearArray[i], itemCount: itemCount})
        }
    }
}
