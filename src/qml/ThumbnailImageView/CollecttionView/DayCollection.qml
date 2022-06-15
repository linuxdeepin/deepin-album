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

    //dayToken: 日期令牌，用于获取其它数据
    ListModel {
        id: theModel
    }

    ListView {
        id: theView
        model: theModel
        anchors.fill: parent
        delegate: theDelegate
        //interactive: false
    }

    Component {
        id: theDelegate

        Rectangle {
            id: delegateRect
            width: theView.width
            //height: 500

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
                checked: false
                anchors.top: timeLineLabel.bottom
                anchors.left: timeLineLabel.left
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
                timeLineLabel.text = qsTr("%1年%2月%3日").arg(dates[0]).arg(Number(dates[1])).arg(Number(dates[2]))

                delegateRect.height = timeLineLabel.height + selectAllBox.height +
                        (Math.abs(Math.ceil(paths.length / Math.floor((delegateRect.width) / theSubView.itemWidth)) * theSubView.itemHeight))
            }
        }
    }

    Component.onCompleted: {
        //1.获取日期
        var days = albumControl.getDays()

        //2.构建model
        for (var i = 0;i !== days.length;++i) {
            theModel.append({dayToken: days[i]})
        }
    }
}
