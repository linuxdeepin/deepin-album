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

    //搬运自缩略图控件内部，width多减个10是因为timeLineLabel的leftMargin
    /*property real cellBaseWidth: global.thumbnailSizeLevel >= 0 && global.thumbnailSizeLevel <= 9 ? 80 + global.thumbnailSizeLevel * 10 : 80
    property int  rowSizeHint: (theView.width - 10 - 10) / cellBaseWidth
    property real realCellWidth: (theView.width - 10 - 10) / rowSizeHint*/

    //风险：月视图切日视图无法正常定位
    //屏蔽部分代码已验证计算结果和theDelegate内部计算的值一致，未知原因导致无法滚动到指定位置
    function scrollToMonth(year, month) {
        vbar.position = 0
        /*var distance = 0
        for(var i = 0;i != theModel.count;++i) {
            var modelObj = theModel.get(i)
            var token = modelObj.dayToken
            var dates = token.split("-")
            if(year === dates[0] && month === dates[1]) {
                break
            }

            var paths = albumControl.getDayPaths(token)

            var currentLength = timeLineLabel_invisible.height + selectAllBox_invisible.height +
                    (Math.abs(Math.ceil(paths.length / Math.floor((theView.width) / realCellWidth)) * realCellWidth))

            distance += currentLength

            //console.debug("scroll", i, currentLength)
        }
        //theView.contentY = distance
        vbar.position = distance / theView.contentY*/
    }

    /*Label {
        id: timeLineLabel_invisible
        font: DTK.fontManager.t3
        visible: false
        text: "123123"
    }

    CheckBox {
        id: selectAllBox_invisible
        visible: false
        text: "123123"
    }*/

    function flushModel() {
        //0.清理
        theModel.clear()

        //1.获取日期
        var days = albumControl.getDays()

        //2.构建model
        for (var i = 0;i !== days.length;++i) {
            theModel.append({dayToken: days[i]})
        }
    }

    //dayToken: 日期令牌，用于获取其它数据
    ListModel {
        id: theModel
    }

    ListView {
        id: theView
        model: theModel
        anchors.fill: parent
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
                if (parent.contentHeight <= parent.height)
                    return
                vbar.active = true
                var datla = wheel.angleDelta.y / 2
                parent.contentY -= datla

                if(vbar.position < 0) {
                    vbar.position = 0
                } else if(vbar.position > 1 - parent.height / parent.contentHeight) {
                    vbar.position = 1 - parent.height / parent.contentHeight
                }
            }
        }
    }

    Component {
        id: theDelegate

        Rectangle {
            id: delegateRect
            width: theView.width

            property string m_dayToken: dayToken

            Label {
                id: timeLineLabel
                font: DTK.fontManager.t3
                anchors.top: parent.top
                anchors.topMargin: 10
                anchors.left: parent.left
                anchors.leftMargin: 10
            }

            CheckBox {
                id: selectAllBox
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
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                enableWheel: false

                width: parent.width
                height: Math.abs(Math.ceil(theSubView.count() / Math.floor((parent.width) / itemWidth)) * itemHeight)
            }

            Component.onCompleted: {
                var picTotal = 0
                var videoTotal = 0
                //1.刷新图片显示
                var paths = albumControl.getDayPaths(m_dayToken)
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

                delegateRect.height = timeLineLabel.height + selectAllBox.height +
                        (Math.abs(Math.ceil(paths.length / Math.floor((delegateRect.width) / theSubView.itemWidth)) * theSubView.itemHeight))

                //console.debug("delegate", index, delegateRect.height)

            }
        }
    }

    Component.onCompleted: {
        flushModel()
        global.sigFlushAllCollectionView.connect(flushModel)
    }
}
