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

    property int scrollDelta: 60
    property int timeLineLblHeight: 36
    property int timeLineLblMargin: 10
    property int selAllCheckBoxHeight: 22
    property int rowSizeHint: (width - global.thumbnailListRightMargin) / global.cellBaseWidth
    property real realCellWidth : (width - global.thumbnailListRightMargin) / rowSizeHint
    property var dayHeights: []

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
        dayHeights = []
        //1.获取日期
        var days = albumControl.getDays()

        //2.构建model
        var dayHeight = 0
        var listHeight = 0
        var dayPaths
        for (var i = 0;i !== days.length;++i) {
            theModel.append({dayToken: days[i]})

            // 计算每个日期列表高度
            dayPaths = albumControl.getDayPaths(days[i])
            listHeight = Math.abs(Math.ceil(dayPaths.length / Math.floor(width / realCellWidth)) * realCellWidth)
            dayHeight = timeLineLblHeight + timeLineLblMargin + selAllCheckBoxHeight + listHeight
            dayHeights.push(dayHeight)
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

    //dayToken: 日期令牌，用于获取其它数据
    ListModel {
        id: theModel
    }

    ListView {
        id: theView
        model: theModel
        width: parent.width
        height: parent.height
        delegate: theDelegate
        interactive: false

        ScrollBar.vertical: ScrollBar {
            id: vbar
            active: false
        }

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            onWheel: {
                var datla = wheel.angleDelta.y / 2
                executeScrollBar(datla)
            }
        }
    }

    Connections {
        target: global
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

        Rectangle {
            //color: index%2 === 0 ? Qt.rgba(0.9,0.8,0.3,0.1) : Qt.rgba(0.9,0.0,0.0,0.1)
            id: delegateRect
            width: theView.width
            height: timeLineLblHeight + timeLineLblMargin + selAllCheckBoxHeight + theSubView.height

            property string m_dayToken: dayToken

            Label {
                id: timeLineLabel
                font: DTK.fontManager.t3
                height: timeLineLblHeight
                anchors.top: parent.top
                anchors.topMargin: timeLineLblMargin
                anchors.left: parent.left
                anchors.leftMargin: timeLineLblMargin
            }

            CheckBox {
                id: selectAllBox
                height: selAllCheckBoxHeight
                anchors.top: timeLineLabel.bottom
                anchors.left: timeLineLabel.left
                checked: theSubView.haveSelectAll
                onClicked: {
                    if(checked) {
                        theSubView.selectAll(true)
                    } else {
                        theSubView.selectAll(false)
                    }
                }
            }

            ListModel {
                id: viewModel
            }

            ThumbnailListView {
                id: theSubView
                thumbnailListModel: viewModel
                anchors.top: selectAllBox.bottom
                anchors.left: selectAllBox.left
                enableWheel: false
                width: parent.width
                height: Math.abs(Math.ceil(theSubView.count() / Math.floor((parent.width) / itemWidth)) * itemHeight)
            }

            function flushView() {
                var picTotal = 0
                var videoTotal = 0
                //1.刷新图片显示
                var paths = albumControl.getDayPaths(m_dayToken)
                viewModel.clear()
                for (var i = 0;i !== paths.length;++i) {
                    viewModel.append({url: paths[i]})

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

                selectAllBox.text = str

                //3.刷新标题
                var dates = m_dayToken.split("-")
                timeLineLabel.text = qsTr("%1Year%2Month%3Day").arg(dates[0]).arg(Number(dates[1])).arg(Number(dates[2]))
            }

            Component.onCompleted: {
                flushView()
                global.sigThumbnailSizeLevelChanged.connect(flushView)
            }
        }
    }

    Component.onCompleted: {
        flushModel()
        global.sigFlushAllCollectionView.connect(flushModel)
    }
}
