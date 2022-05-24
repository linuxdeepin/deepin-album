import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQml.Models 2.11
import QtQml 2.11
import QtQuick.Shapes 1.10
import org.deepin.dtk 1.0

import "../../Control/ListView"

Item {
    id : importedListView

    //view依赖的model管理器
//    ListModel {
//        id: importedListModel
//    }

    //已导入列表本体
    ListView {
        id: theView
        clip: true
        interactive: false //禁用原有的交互逻辑，重新开始定制
        model: 3
        width: parent.width
        height: parent.height
        delegate: importedListDelegate

        //激活滚动条
        ScrollBar.vertical: ScrollBar {
            id: vbar
            active: true
        }

        MouseArea {
            //按下时的起始坐标
            property int pressedXAxis: -1
            property int pressedYAxis: -1

            anchors.fill: parent
            acceptedButtons: Qt.LeftButton //仅激活左键

            // 已导入列表不处理鼠标，穿透到缩略图列表处理鼠标事件，以便框选生效
            onPressed: {
                mouse.accepted = false
            }
            onReleased: {
                mouse.accepted = false
            }
            onMouseXChanged: {
                mouse.accepted = false
            }
            onMouseYChanged: {
                mouse.accepted = false
            }

            onWheel: {
                // 滚动时，激活滚动条显示
                vbar.active = true
                //滚动事件
                var datla = wheel.angleDelta.y / 2
                parent.contentY -= datla
                if(parent.contentY < 0)
                {
                    parent.contentY = 0
                }
                else if(parent.contentY > parent.contentHeight - parent.height)
                {
                    parent.contentY = parent.contentHeight - parent.height
                }
            }
        }
    }

    //已导入列表代理控件
    Component {
        id: importedListDelegate

        Control {
            width: theView.width
            height: importedGridView.height + m_topMarign + m_bottomMarign + importedCheckBox.height
            property string m_index: index
            property int m_topMarign: 10 // 已导入列表子项上边距
            property int m_bottomMarign: 10 // 已导入列表子项下边距
            CheckBox {
                id: importedCheckBox
                font: DTK.fontManager.t6
                text: "导入于2022年5月20日11:11 共46项"

                onCheckStateChanged: {
                    if(checked) {
                        importedGridView.selectAll(true)
                    } else {
                        importedGridView.selectAll(false)
                    }
                }
            }

            //缩略图网格表
            ThumbnailListView {
                id: importedGridView

                anchors.left: parent.left
                anchors.top: importedCheckBox.bottom
                anchors.topMargin: m_topMarign
                anchors.bottomMargin: m_bottomMarign
                width: parent.width
                height: Math.ceil(thumbnailListModel.count / Math.floor((parent.width) / itemWidth)) * itemHeight
                enableWheel: false
            }
        }
    }
}
