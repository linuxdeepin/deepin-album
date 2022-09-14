import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQml.Models 2.11
import QtQml 2.11
import QtQuick.Shapes 1.10
import org.deepin.dtk 1.0

import "../../Control/ListView"
import "../../Control"
import "../../"
Item {
    id : importedListView
    signal rectSelTitleChanged(rect rt)
    signal sigUnSelectAll()
    property var selectedPaths: []
    property int filterComboOffsetY: 5
    property int spaceCtrlHeight: filterCombo.y + filterComboOffsetY
    property int importCheckboxHeight: 26
    property int listMargin: 10 // 已导入列表子项上、下边距
    property int rowSizeHint: (width - global.thumbnailListRightMargin) / global.cellBaseWidth
    property real realCellWidth : (width - global.thumbnailListRightMargin) / rowSizeHint

    //view依赖的model管理器
    property ListModel importedListModel: ListModel {
        id: theModel
        property var selectedPathObj: {"id":0, "paths":[]}
        property var selectedPathObjs: []
        property var dayHeights: []
        function loadImportedInfos() {
            console.log("imported model has refreshed.. filterType:", filterCombo.currentIndex)
            theModel.clear()
            theModel.selectedPathObjs = []
            theModel.dayHeights = []
            // 从后台获取所有已导入数据
            var titleInfos = albumControl.getImportTimelinesTitleInfos(filterCombo.currentIndex);
            console.log("imported model has refreshed.. filterType:", filterCombo.currentIndex, " done...")
            var tmpPath = []
            var i = 0
            var dayHeight = 0
            var listHeight = 0
            theView.listContentHeight = 0
            for (var key in titleInfos) {
                theModel.append({"title":key, "items":titleInfos[key]})
                selectedPathObj = {"id": i, "paths":tmpPath}
                theModel.selectedPathObjs.push(selectedPathObj)

                // 计算每个日期列表高度
                listHeight = Math.abs(Math.ceil(titleInfos[key].length / Math.floor(importedListView.width / realCellWidth)) * realCellWidth)
                dayHeight = listHeight + listMargin * 2 + importCheckboxHeight + (i === 0 ? spaceCtrlHeight : 0)
                dayHeights.push(dayHeight)
                theView.listContentHeight += dayHeight
                i++
            }
        }
    }

    // 刷新已导入列表已选路径
    function updateSelectedPaths()
    {
        var tmpPaths = []
        for (var i = 0; i < theModel.selectedPathObjs.length; i++) {
            if (theModel.selectedPathObjs[i].paths.length > 0) {
                for (var j = 0; j < theModel.selectedPathObjs[i].paths.length; j++)
                    tmpPaths.push(theModel.selectedPathObjs[i].paths[j])
            }
        }

        selectedPaths = tmpPaths
        if (importedListView.visible) {
            global.selectedPaths = selectedPaths
        }
    }

    // 通知已导入视图标题栏区域，调整色差校正框选框大小
    Connections {
        target: rubberBand
        onRectSelChanged: {
            var pos1 = theView.contentItem.mapToItem(importedListView, rubberBand.left(), rubberBand.top())
            var pos2 = theView.contentItem.mapToItem(importedListView, rubberBand.right(), rubberBand.bottom())
            rectSelTitleChanged(albumControl.rect(pos1, pos2))
        }
    }

    //已导入列表本体
    ListView {
        id: theView
        clip: true
        interactive: false //禁用原有的交互逻辑，重新开始定制
        model: theModel
        width: parent.width
        height: parent.height
        delegate: importedListDelegate
        //鼠标正在按下状态
        property bool inPress: false
        //框选滚动方向
        property var scrollDirType: GlobalVar.RectScrollDirType.NoType
        property var listContentHeight
        property int rectSelScrollOffset: global.rectSelScrollStep
        //激活滚动条
        ScrollBar.vertical: ScrollBar {
            id: vbar
            active: true
        }

        MouseArea {
            anchors.fill: parent.contentItem
            acceptedButtons: Qt.LeftButton //仅激活左键

            id: theMouseArea

            onPressed: {
                if(mouse.button == Qt.RightButton) {
                    mouse.accepted = false
                    return
                }

                theView.scrollDirType = GlobalVar.RectScrollDirType.NoType
                parent.inPress = true
                rubberBand.x1 = mouse.x
                rubberBand.y1 = mouse.y
                rubberBand.x2 = mouse.x
                rubberBand.y2 = mouse.y
                mouse.accepted = true
            }
            onMouseXChanged: {
                if(mouse.button == Qt.RightButton) {
                    mouse.accepted = false
                    return
                }

                rubberBand.x2 = mouse.x

                mouse.accepted = true
            }
            onMouseYChanged: {
                if(mouse.button == Qt.RightButton) {
                    mouse.accepted = false
                    return
                }

                // 刷新矩形第二锚点，内部触发updateRect，保证y2值标记为矩形底部坐标
                rubberBand.y2 = mouse.y

                // 确定滚动延展方向（向上还是向下）
                var parentY = mapToItem(theView, mouse.x, mouse.y).y
                if (parentY > theView.height) {
                    // 选择框超出ListView底部，ListView准备向下滚动
                    theView.scrollDirType = GlobalVar.RectScrollDirType.ToBottom
                } else if (parentY < 0) {
                    // 选择框超出ListView顶部，ListView准备向上滚动
                    theView.scrollDirType = GlobalVar.RectScrollDirType.ToTop
                } else {
                    if (rectScrollTimer.running)
                        rectScrollTimer.stop()
                }

                mouse.accepted = true
            }
            onReleased: {
                if(mouse.button == Qt.RightButton) {
                    mouse.accepted = false
                    return
                }

                theView.scrollDirType = GlobalVar.RectScrollDirType.NoType
                parent.inPress = false
                rubberBand.clearRect()

                // 清除标题栏色差矫校正框选框
                rectSelTitleChanged(albumControl.rect(Qt.point(0, 0), Qt.point(0, 0)))

                mouse.accepted = true
            }
            onWheel: {
                // 滚动时，激活滚动条显示
                vbar.active = true
                var datla = wheel.angleDelta.y
                console.log("onWhell, delta:", datla)
                if( datla > 0 ) {
                    vbar.decrease()
                } else {
                    vbar.increase()
                }
                if (theView.atYEnd) {
                    console.log("at end..")
                }
            }

            //橡皮筋控件
            RubberBand {
                id: rubberBand
                visible: theView.inPress
            }
        }

        Timer {
            id: rectScrollTimer
            interval: 100
            running: theView.scrollDirType !== GlobalVar.RectScrollDirType.NoType
            repeat: true
            onTriggered: {
                // 选择框向下延展滚动
                if (theView.scrollDirType === GlobalVar.RectScrollDirType.ToBottom) {
                    var newY2 = rubberBand.y2 + theView.rectSelScrollOffset
                    if (newY2 <= theView.listContentHeight) {
                        rubberBand.y2 = newY2
                        theView.contentY = theView.contentY + theView.rectSelScrollOffset + theView.originY
                    } else {
                        // 选择框底部最大值为内容区域底部
                        theView.contentY = theView.listContentHeight - theView.height
                        rubberBand.y2 = theView.listContentHeight
                        rectScrollTimer.stop()
                    }
                } else if (theView.scrollDirType === GlobalVar.RectScrollDirType.ToTop) {
                    if (rubberBand.top() < 0) {
                        rectScrollTimer.stop()
                        return
                    }

                    // 矩形顶部向上延展
                    if (theView.contentY <= rubberBand.bottom() || rubberBand.bottom() === rubberBand.top()) {
                        var newTop = rubberBand.top() - theView.rectSelScrollOffset
                        if (newTop > 0) {
                            rubberBand.y2 = newTop
                            theView.contentY = theView.contentY - theView.rectSelScrollOffset + theView.originY
                        } else {
                            // 选择框顶部最小值为内容区域顶部
                            rubberBand.y2 = 0
                            theView.contentY = 0 + theView.originY

                            rectScrollTimer.stop()
                        }
                    } else {
                        // 矩形框底部向上收缩
                        var newBottom = rubberBand.bottom() - theView.rectSelScrollOffset
                        if (newBottom > rubberBand.top()) {
                            rubberBand.y2 = newBottom
                            theView.contentY = theView.contentY - theView.rectSelScrollOffset + theView.originY
                        } else {
                            var srcollOffset = Math.abs(rubberBand.y1 - rubberBand.y2)
                            rubberBand.y2 = rubberBand.y1
                            theView.contentY = theView.contentY - srcollOffset + theView.originY
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: global
        onSigPageUp: {
            if (visible) {
                vbar.active = true
                vbar.decrease()
            }
        }

        onSigPageDown: {
            if (visible) {
                vbar.active = true
                vbar.increase()
            }
        }
    }

    //已导入列表代理控件
    Component {
        id: importedListDelegate

        Control {
            id :importControl
            z: 2
            width: theView.width
            height: importedGridView.height + importedListView.listMargin * 2 + importedListView.importCheckboxHeight + spaceRect.height
            property string m_index: index
            property var theViewTitle: global.objIsEmpty(theModel.get(index)) ? "" : theModel.get(index).title //日期标题文本内容
            property var theViewItems: global.objIsEmpty(theModel.get(index)) ? "" : theModel.get(index).items //日期标题对应图片信息链表

            Rectangle {
                id: spaceRect
                width: parent.width
                height: index == 0 ? importedListView.spaceCtrlHeight : 0
            }

            CheckBox {
                id: importedCheckBox
                height: importedListView.importCheckboxHeight
                visible: importedGridView.haveSelect
                checked: importedGridView.haveSelectAll
                font: DTK.fontManager.t6
                anchors.top: (index == 0 ? spaceRect.bottom : spaceRect.top)
                onClicked: {
                    if(checked) {
                        importedGridView.selectAll(true)
                    } else {
                        importedGridView.selectAll(false)
                    }
                }

            }
            Label {
                anchors.left :importedCheckBox.visible ? importedCheckBox.right : parent.left
                anchors.top :importedCheckBox.top
                font: DTK.fontManager.t6
                id: importedLabel
                text: theViewTitle + " " + (importedGridView.count() === 1 ? qsTr("1 item") : qsTr("%1 items").arg(importedGridView.count()))

            }

            //缩略图网格表
            ThumbnailListView {
                id: importedGridView
                viewTitle: theViewTitle
                anchors.left: parent.left
                anchors.top: importedCheckBox.bottom
                anchors.topMargin: importedListView.listMargin
                anchors.bottomMargin: importedListView.listMargin
                width: parent.width
                height: Math.abs(Math.ceil(importedGridView.count() / Math.floor((parent.width) / itemWidth)) * itemHeight)

                enableWheel: false

                // 装载数据
                thumbnailListModel: {
                    theViewItems
                }

                Connections {
                    target: rubberBand
                    onRectSelChanged: {
                        var pos1 = theMouseArea.mapToItem(importedGridView, rubberBand.left(), rubberBand.top())
                        var pos2 = theMouseArea.mapToItem(importedGridView, rubberBand.right(), rubberBand.bottom())
                        var rectsel = albumControl.rect(pos1, pos2)
                        var rectList = Qt.rect(0, 0, importedGridView.width, importedGridView.height)
                        var rect = albumControl.intersected(rectList, rectsel)
                        importedGridView.flushRectSel(rect.x, rect.y, rect.width, rect.height)
                    }
                }

                // 监听缩略图子控件选中状态，一旦改变，更新已导入视图所有选中路径
                Connections {
                    target: importedGridView
                    onSelectedChanged: {
                        theModel.selectedPathObjs[m_index].paths = importedGridView.selectedPaths
                        updateSelectedPaths()
                    }
                }

                Connections {
                    target: importedListView
                    onSigUnSelectAll: {
                        importedGridView.selectAll(false)
                    }
                }
            }
        }
    }
}
