// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import org.deepin.dtk 1.0

import org.deepin.album 1.0 as Album

import "../../Control"
import "../../Control/ListView"
import "../../Control/Animation"
import "../../"

SwitchViewAnimation {
    id: dayView

    signal sigListViewPressed(int x, int y)
    signal sigListViewReleased(int x, int y)
    property int scrollDelta: 60
    property int timeLineLblHeight: 36
    property int timeLineLblMargin: 10
    property int selAllCheckBoxHeight: 22
    property int rowSizeHint: (width - GStatus.thumbnailListRightMargin) / GStatus.cellBaseWidth
    property real realCellWidth : (width - GStatus.thumbnailListRightMargin) / rowSizeHint
    property var dayHeights: []

    property var selectedPaths: []
    property string numLabelText: "" //总数标签显示内容
    property string selectedText: getSelectedText(selectedPaths)

    property int currentColletionIndex: collecttionView.currentViewIndex

    property bool checkBoxClicked: false

    Connections {
        target: collecttionView
        onFlushDayViewStatusText: {
            if (visible) {
                if (selectedPaths.length > 0)
                    getSelectedText(selectedPaths)
                else
                    getNumLabelText()
            }
        }
    }

    Connections {
        target: albumControl
        onSigRepeatUrls: {
            if (visible && collecttionView.currentViewIndex === 2) {
                theView.sigUnSelectAll()
                selectedPaths = urls
                if (selectedPaths.length > 0)
                    getSelectedText(selectedPaths)
                else
                    getNumLabelText()
                GStatus.selectedPaths = selectedPaths
            }
        }

        //收到导入完成消息
        onSigImportFinished: {
            if (visible) {
                //刷新数量显示
                getNumLabelText()
            }
        }
    }

    // 刷新总数标签
    function getNumLabelText() {
        //QML的翻译不支持%n的特性，只能拆成这种代码

        var photoCountText = ""
        var photoCount = albumControl.getAllInfoConut(1)
        if(photoCount === 0) {
            photoCountText = ""
        } else if(photoCount === 1) {
            photoCountText = qsTr("1 photo")
        } else {
            photoCountText = qsTr("%1 photos").arg(photoCount)
        }

        var videoCountText = ""
        var videoCount = albumControl.getAllInfoConut(2)
        if(videoCount === 0) {
            videoCountText = ""
        } else if(videoCount === 1) {
            videoCountText = qsTr("1 video")
        } else {
            videoCountText = qsTr("%1 videos").arg(videoCount)
        }

        numLabelText = photoCountText + (videoCountText !== "" ? ((photoCountText !== "" ? " " : "") + videoCountText) : "")

        if (visible) {
            GStatus.statusBarNumText = numLabelText
        }
    }

    // 刷新选中项目标签内容
    function getSelectedText(paths) {
        var selectedNumText = GStatus.getSelectedNumText(paths, numLabelText)
        if (visible)
            GStatus.statusBarNumText = selectedNumText
        return selectedNumText
    }

    //月视图切日视图
    function scrollToMonth(year, month) {
        vbar.active = true
        var targetY = 0
        theView.contentY = 0
        for (var i = 0; i < theModel.count; i++) {
            var modelObj = theModel.get(i)
            var token = modelObj.dayToken
            var dates = token.split("-")
            if(year === dates[0] && month === dates[1]) {
                break
            }
            targetY += dayHeights[i]
        }

        // 当日视图滚到底部时，theview的originY会发生改变，将会导致contentY的定位值异常
        // 因此实际滚动值需要通过originY来修正
        theView.contentY = targetY + theView.originY
    }

    function flushModel() {
        //0.清理
        theModel.clear()
        theModel.selectedPathObjs = []
        selectedPaths = []
        dayHeights = []
        //1.获取日期
        var days = []
        if (Number(fileControl.getConfigValue("", "loadDayView", 1)))
            days = albumControl.getDays()

        //2.构建model
        var dayHeight = 0
        var listHeight = 0
        theView.listContentHeight = 0
        var dayPaths
        for (var i = 0;i !== days.length;++i) {
            theModel.append({dayToken: days[i]})

            // 当前日期列表选中数据初始化
            dayPaths = []
            theModel.selectedPathObj = {"id": i, "paths": dayPaths}
            theModel.selectedPathObjs.push(theModel.selectedPathObj)

            // 计算每个日期列表高度
            dayPaths = albumControl.getDayPaths(days[i])
            listHeight = Math.abs(Math.ceil(dayPaths.length / Math.floor(width / realCellWidth)) * realCellWidth)
            dayHeight = timeLineLblHeight + timeLineLblMargin + selAllCheckBoxHeight + listHeight
            dayHeights.push(dayHeight)
            theView.listContentHeight += dayHeight
        }
    }

    function executeScrollBar(delta) {
        if (theView.contentHeight <= theView.height)
            return

        vbar.active = true
        theView.contentY -= delta

        if(vbar.position < 0) {
            vbar.position = 0
        } else if(vbar.position > 1 - theView.height / theView.contentHeight) {
            vbar.position = 1 - theView.height / theView.contentHeight
        }
    }

    // 刷新日视图列表已选路径
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
        if (visible) {
            GStatus.selectedPaths = selectedPaths
        }
    }

    //dayToken: 日期令牌，用于获取其它数据
    ListModel {
        id: theModel
        property var selectedPathObj: {"id":0, "paths":[]}
        property var selectedPathObjs: []
    }

    ListView {
        id: theView
        clip: true
        model: theModel
        width: parent.width
        height: parent.height
        delegate: theDelegate
        interactive: false
        //鼠标正在按下状态
        property bool inPress: false
        //框选滚动方向
        property var scrollDirType: Album.Types.NoType
        property var listContentHeight
        property int rectSelScrollOffset: GStatus.rectSelScrollStep

        signal sigUnSelectAll()
        signal dbClicked(string url)

        //激活滚动条
        ScrollBar.vertical: ScrollBar {
            id: vbar
            active: false
        }

        MouseArea {
            // 鼠标区域需要包含空白区域，否则点击空白区域会拖动相册应用
            anchors.fill: parent.contentHeight > parent.height ? parent.contentItem : parent
            acceptedButtons: Qt.LeftButton
            propagateComposedEvents: true

            property bool ctrlPressed: false //记录ctrl是否按下

            id: theMouseArea

            onClicked: {
                //允许鼠标事件传递给子控件处理,否则鼠标点击缩略图收藏图标不能正常工作
                //同时propagateComposedEvents需设置为true
                //注意：不能传递onPressed、onReleased等基础事件，会有bug；合成事件onClicked等可以传递
                mouse.accepted = false
            }

            onPressed: {
                if(mouse.button == Qt.RightButton) {
                    mouse.accepted = false
                    return
                }

                var gPos = theMouseArea.mapToGlobal(mouse.x, mouse.y)
                sigListViewPressed(gPos.x, gPos.y)
                if (checkBoxClicked) {
                    mouse.accepted = false
                    return
                }

                ctrlPressed = Qt.ControlModifier & mouse.modifiers

                theView.scrollDirType = Album.Types.NoType
                parent.inPress = true
                rubberBand.x1 = mouse.x
                rubberBand.y1 = mouse.y
                rubberBand.x2 = mouse.x
                rubberBand.y2 = mouse.y
                mouse.accepted = true
            }
            onDoubleClicked: {
                if (GStatus.selectedPaths.length > 0)
                    theView.dbClicked(GStatus.selectedPaths[0])

                parent.inPress = false
                rubberBand.clearRect()

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
                    if (parent.contentHeight > parent.height)
                        theView.scrollDirType = Album.Types.ToBottom
                } else if (parentY < 0) {
                    // 选择框超出ListView顶部，ListView准备向上滚动
                    theView.scrollDirType = Album.Types.ToTop
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

                parent.inPress = false

                // ctrl按下，鼠标点击事件释放时，需要再发送一次框选改变信号，用来在鼠标释放时实现ctrl取消选中的功能
                if ((Qt.ControlModifier & mouse.modifiers) && rubberBand.width < 3 & rubberBand.height < 3) {
                    rubberBand.rectSelChanged()
                }

                ctrlPressed = false

                theView.scrollDirType = Album.Types.NoType
                rubberBand.clearRect()

                var gPos = theMouseArea.mapToGlobal(mouse.x, mouse.y)
                sigListViewReleased(gPos.x, gPos.y)

                mouse.accepted = true
            }

            onWheel: {
                var datla = wheel.angleDelta.y / 2
                if (Qt.ControlModifier & wheel.modifiers) {
                    // 按住ctrl，缩放缩略图
                    var curValue = statusBar.sliderValue
                    if (datla > 0)
                        statusBar.setSliderWidgetValue(curValue + 1)
                    else
                        statusBar.setSliderWidgetValue(curValue - 1)
                } else {
                    // 正常滚动显示缩略图内容
                    executeScrollBar(datla)
                }
            }

            //橡皮筋控件
            RubberBand {
                id: rubberBand
                visible: theView.inPress
            }

            Timer {
                id: rectScrollTimer
                interval: 100
                running: theView.scrollDirType !== Album.Types.NoType
                repeat: true
                onTriggered: {
                    // 选择框向下延展滚动
                    if (theView.scrollDirType === Album.Types.ToBottom) {
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
                    } else if (theView.scrollDirType === Album.Types.ToTop) {
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
    }
    Connections {
        target: GStatus
        onSigPageUp: {
            if (visible) {
                executeScrollBar(scrollDelta)
            }
        }

        onSigPageDown: {
            if (visible) {
                executeScrollBar(-scrollDelta)
            }
        }
    }

    Component {
        id: theDelegate

        Item {
            id: delegateRect
            width: theView.width
            height: timeLineLblHeight + timeLineLblMargin + selAllCheckBoxHeight + theSubView.height

            property string m_dayToken: dayToken

            Label {
                id: timeLineLabel
                font: DTK.fontManager.t3
                height: timeLineLblHeight
                anchors {
                    top: parent.top
                    topMargin: timeLineLblMargin
                    left: parent.left
                    leftMargin: timeLineLblMargin
                }
            }

            CheckBox {
                id: selectAllBox
                height: selAllCheckBoxHeight
                anchors {
                    top: timeLineLabel.bottom
                    left: timeLineLabel.left
                }
                checked: theSubView.haveSelectAll
                visible: selectedPaths.length > 0
                onClicked: {
                    if(checked) {
                        theSubView.selectAll(true)
                    } else {
                        theSubView.selectAll(false)
                    }
                }
            }

            Connections {
                target: dayView
                function onSigListViewPressed(x, y) {
                    var object = selectAllBox.mapFromGlobal(x,y)
                    if (selectAllBox.contains(object)) {
                        checkBoxClicked = true
                        if (selectAllBox.checkState === Qt.Checked) {
                            selectAllBox.checkState = Qt.Unchecked
                            theSubView.selectAll(false)
                        } else {
                            selectAllBox.checkState = Qt.Checked
                            theSubView.selectAll(true)
                        }
                    }
                }

                function onSigListViewReleased(x, y) {
                    checkBoxClicked = false
                }
            }

            Label {
                id: numLabelTitle
                height: selAllCheckBoxHeight
                anchors {
                    top: timeLineLabel.bottom
                    left: selectAllBox.visible ? selectAllBox.right : timeLineLabel.left
                }
                topPadding: -1
            }

            ListModel {
                id: viewModel
            }

            ThumbnailListView {
                id: theSubView
                thumbnailListModel: viewModel
                anchors {
                    top: selectAllBox.bottom
                    left: selectAllBox.left
                }
                enableWheel: false
                width: parent.width
                height: Math.abs(Math.ceil(theSubView.count() / Math.floor((parent.width) / itemWidth)) * itemHeight)

                Connections {
                    target: rubberBand
                    function onRectSelChanged() {
                        var pos1 = theMouseArea.mapToItem(theSubView, rubberBand.left(), rubberBand.top())
                        var pos2 = theMouseArea.mapToItem(theSubView, rubberBand.right(), rubberBand.bottom())
                        var rectsel = albumControl.rect(pos1, pos2)
                        var rectList = Qt.rect(0, 0, theSubView.width, theSubView.height)
                        var rect = albumControl.intersected(rectList, rectsel)
                        var bDetectMousePrees = albumControl.manhattanLength(pos1, pos2) < 3 // 识别此次框选事件是否为鼠标点击事件，以便在列表控件处理ctrl按键相关的操作
                        theSubView.flushRectSel(rect.x, rect.y, rect.width, rect.height, theMouseArea.ctrlPressed, bDetectMousePrees, theView.inPress)
                    }
                }

                // 监听缩略图子控件选中状态，一旦改变，更新日视图所有选中路径
                Connections {
                    target: theSubView
                    function onSelectedChanged() {
                        if (index > -1) {
                            theModel.selectedPathObjs[index].paths = theSubView.selectedPaths
                        }
                        updateSelectedPaths()
                    }
                }

                Connections {
                    target: theView
                    function onSigUnSelectAll() {
                        theSubView.selectAll(false)
                    }
                }

                Connections {
                    target: theView
                    function onDbClicked(url) {
                        var openPaths = theSubView.allOriginUrls()
                        if (openPaths.indexOf(url) !== -1)
                            theSubView.executeViewImage()
                    }
                }
            }

            function flushView() {
                var picTotal = 0
                var videoTotal = 0
                //1.刷新图片显示
                var paths = albumControl.getDayPaths(m_dayToken)
                viewModel.clear()
                for (var i = 0;i !== paths.length;++i) {
                    viewModel.append({url: paths[i], filePath: albumControl.url2localPath(paths[i])})

                    //顺便统计下图片和视频的数量
                    if(fileControl.isImage(paths[i])) {
                        picTotal++
                    } else {
                        videoTotal++
                    }
                }

                //2.刷新checkbox
                var str = ""
                if(picTotal == 1) {
                    str += qsTr("1 photo ")
                } else if(picTotal > 1) {
                    str += qsTr("%1 photos ").arg(picTotal)
                }

                if(videoTotal == 1) {
                    str += qsTr("1 video")
                } else if(videoTotal > 1) {
                    str += qsTr("%1 videos").arg(videoTotal)
                }

                numLabelTitle.text = str

                //3.刷新标题
                var dates = m_dayToken.split("-")
                timeLineLabel.text = qsTr("%1/%2/%3").arg(dates[0]).arg(Number(dates[1])).arg(Number(dates[2]))
            }

            Component.onCompleted: {
                flushView()
            }
        }
    }

    onVisibleChanged: {
        // 窗口显示时，重置显示内容
        if (visible) {
            //清除选中状态
            theView.sigUnSelectAll()
            selectedPaths = []
            GStatus.selectedPaths = []
            flushModel()
        }
    }

    Component.onCompleted: {
        GStatus.sigFlushAllCollectionView.connect(flushModel)
    }
}
