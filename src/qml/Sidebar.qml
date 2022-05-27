import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.4
import QtQuick 2.0
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.0
import org.deepin.dtk 1.0
import "./Control"

Rectangle {
    property bool changeList: false
    property ButtonGroup group: ButtonGroup {}
    width:200
    height:parent.height
    ScrollView {
        width:200
        height:parent.height
        //    color: "#EEEEEE"
        clip: true

        Column {
            width:parent.width
            id: colum
            spacing: 0
            //        height: 300
            ////    anchors.fill: parent
            //   anchors.centerIn: parent
            anchors.top: parent.top
            anchors.topMargin: 69
            anchors.left: parent.left
            anchors.leftMargin: 20
            Label {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 15
                id:pictureLabel
                height: 30
                text: "照片库"

                color: Qt.rgba(0,0,0,0.3)
                lineHeight: 20

            }

            ListView{
                id : pictureLeftlist
                width: parent.width-20
                implicitHeight: contentHeight
                model: SidebarModel {}
                delegate:ItemDelegate {
                    text: name
                    width: parent.width
                    icon.name: iconName
                    checked: index == 0
                    backgroundVisible: false
                    onClicked: {
                        global.currentViewIndex = index + 2
                    }
                    ButtonGroup.group: group
                }
            }
            RowLayout{
                Layout.alignment: Qt.AlignCenter
                spacing: 100
                Label {
                    id:albumLabel
                    //        anchors.top: pictureLeftlist.bottom
                    //        anchors.topMargin: 15
                    //        anchors.left: parent.left
                    //        anchors.leftMargin: 20

                    height: 30
                    text: "相册"
                    color: Qt.rgba(0,0,0,0.3)
                }

                FloatingButton {
                    id:addAlbumButton
                    checked: false
                    width: 20
                    height: 20

                    icon{
                        name: "add-xiangce"
                        width: 10
                        height: 10
                    }


                    onClicked: {
                        var x = parent.mapToGlobal(0, 0).x + parent.width / 2 - 190
                        var y = parent.mapToGlobal(0, 0).y + parent.height / 2 - 89
                        newAlbum.setX(x)
                        newAlbum.setY(y)

                        newAlbum.show()
                    }
                }
            }
            ListView{
                id : fixedList
                height:3 * 36
                width:parent.width
                clip: true
                visible: true
                interactive: false //禁用原有的交互逻辑，重新开始定制

                model: ListModel {
                    ListElement {
                        name: "Screen Capture"
                        number: "1"
                        iconName :"item"

                    }
                    ListElement {
                        name: "Camera"
                        number: "2"
                        iconName :"item"
                    }
                    ListElement {
                        name: "Draw"
                        number: "3"
                        iconName :"item"
                    }
                }
                delegate:    ItemDelegate {
                    text: name
                    width: parent.width - 20
                    height : 36
                    icon.name: iconName
                    checked: index == 0
                    backgroundVisible: false
                    onClicked: {
                        global.currentViewIndex = 6
                        global.currentCustomAlbumUId = number
                    }
                    ButtonGroup.group: group
                }

            }

            ListView{
                id : customList
                height:albumControl.getAllCustomAlbumId(changeList).length * 36
                width:parent.width
                clip: true
                visible: true
                interactive: false //禁用原有的交互逻辑，重新开始定制

                model :albumControl.getAllCustomAlbumId(changeList).length
                delegate:    ItemDelegate {
                    text: albumControl.getAllCustomAlbumName()[index]
                    width: parent.width - 20
                    height : 36
                    icon.name: "item"
                    checked: index == 0
                    backgroundVisible: false
                    onClicked: {
                        global.currentViewIndex = 6
                        global.currentCustomAlbumUId = index + 4
                    }
                    ButtonGroup.group: group
                }
            }
        }
        //rename窗口
        NewAlbumDialog {
            id: newAlbum
        }
    }
}
