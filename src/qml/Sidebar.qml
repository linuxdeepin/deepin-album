import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.4
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.0
import org.deepin.dtk 1.0
import "./Control"
import "./PreviewImageViewer"

Rectangle {

    property int currentCustomIndex: 0
    signal sigRename
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
            anchors.top: parent.top
            anchors.topMargin: 69
            anchors.left: parent.left
            anchors.leftMargin: 10
            Label {
                Layout.alignment: Qt.AlignTop
                Layout.topMargin: 15
                id:pictureLabel
                height: 30
                text: qsTr("Gallery")

                color: Qt.rgba(0,0,0,0.3)
                lineHeight: 20

            }

            ListView{
                id : pictureLeftlist
                width: parent.width-20
                implicitHeight: contentHeight
                model: SidebarModel {}
                delegate:ItemDelegate {
                    id :picItem
                    width: 180
                    height: 36
                    checked: index == 0
                    backgroundVisible: false
                    DciIcon {
                        id: siderIconPic
                        anchors.left: picItem.left; anchors.leftMargin: 10
                        anchors.verticalCenter: picItem.verticalCenter
                        name: iconName
                        sourceSize: Qt.size(20, 20)
                    }
                    Label {
                        id: songNamePic
                        width: 100
                        anchors.left: siderIconPic.right; anchors.leftMargin: 10
                        anchors.verticalCenter: picItem.verticalCenter
                        text: name
                    }
                    onClicked: {
                        global.currentViewIndex = index + 2
                        // 导航页选中我的收藏时，设定自定相册索引为0，使用CutomAlbum控件按自定义相册界面逻辑显示我的收藏内容
                        if (global.currentViewIndex == 4) {
                            global.currentCustomAlbumUId = 0
                        }
                        global.searchEditText = ""

                        forceActiveFocus()
                    }
                    ButtonGroup.group: global.siderGroup
                }
            }

            RowLayout{
                Layout.alignment: Qt.AlignCenter
                spacing: 100

                visible: albumControl.getDevicePaths(global.deviceChangeList).length>0 ?true :false
                Label {
                    id:deviceLabel
                    height: 30
                    text: qsTr("Device")
                    color: Qt.rgba(0,0,0,0.3)
                }
            }

            ListView{
                id : deviceList
                height:albumControl.getDevicePaths(global.deviceChangeList).length *42
                width:parent.width
                visible: true
                interactive: false //禁用原有的交互逻辑，重新开始定制

                model :albumControl.getDevicePaths(global.deviceChangeList).length
                delegate:    ItemDelegate {
                    id: deviccItem
                    width: 180
                    height : 36
                    backgroundVisible: false
                    checked: index == 0
                    DciIcon {
                        id: deviceIcon
                        anchors.left: deviccItem.left; anchors.leftMargin: 10
                        anchors.verticalCenter: deviccItem.verticalCenter
                        name: "iphone"
                        sourceSize: Qt.size(20, 20)
                    }
                    Label {
                        id: deviceName
                        width: 100
                        anchors.left: deviceIcon.right; anchors.leftMargin: 10
                        anchors.verticalCenter: deviccItem.verticalCenter
                        text: albumControl.getDeviceNames(global.deviceChangeList)[index]
                    }
                    ActionButton {
                        anchors.left: deviceName.right; anchors.leftMargin: 10
                        anchors.verticalCenter: deviccItem.verticalCenter
                        width: 10
                        height:  10
                        icon.name: "arrow"
                        icon.width: 10
                        icon.height: 10
                        onClicked:{
                            albumControl.unMountDevice(albumControl.getDevicePaths(global.deviceChangeList)[index])

                        }
                    }

                    onClicked: {
                        global.currentViewIndex = 8
                        global.deviceCurrentPath=albumControl.getDevicePaths(global.deviceChangeList)[index]
                        forceActiveFocus()
                    }
                    ButtonGroup.group: global.siderGroup
                }

            }

            RowLayout{
                Layout.alignment: Qt.AlignCenter
                spacing: 100
                Label {
                    id:albumLabel
                    height: 30
                    text: qsTr("Albums")
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
                        newAlbum.setNormalEdit()
                        newAlbum.show()
                        forceActiveFocus()
                    }
                }
            }
            ListView{
                id : fixedList
                height:3 * 36
                width:parent.width
                visible: true
                interactive: false //禁用原有的交互逻辑，重新开始定制

                model: ListModel {
                    ListElement {
                        name: qsTr("Screen Capture")
                        number: "1"
                        iconName :"screenshot"

                    }
                    ListElement {
                        name: qsTr("Camera")
                        number: "2"
                        iconName :"camera"
                    }
                    ListElement {
                        name: qsTr("Draw")
                        number: "3"
                        iconName :"draw"
                    }
                }
                delegate:    ItemDelegate {
                    id:sysitem
                    width: 180
                    height : 36
                    backgroundVisible: false
                    checked: index == 0
                    DciIcon {
                        id: siderIcon1
                        anchors.left: sysitem.left; anchors.leftMargin: 10
                        anchors.verticalCenter: sysitem.verticalCenter
                        name: iconName
                        sourceSize: Qt.size(20, 20)
                    }
                    Label {
                        id: songName1
                        width: 100
                        anchors.left: siderIcon1.right; anchors.leftMargin: 10
                        anchors.verticalCenter: sysitem.verticalCenter
                        text: name
                    }

                    onClicked: {
                        global.currentViewIndex = 6
                        global.currentCustomAlbumUId = number
                        global.searchEditText = ""
                        forceActiveFocus()
                    }
                    ButtonGroup.group: global.siderGroup
                    MouseArea {

                        anchors.fill: parent
                        acceptedButtons:Qt.RightButton

                        enabled: true;

                        onClicked: {
                            if(mouse.button == Qt.RightButton) {
                                sysItem.checked=true
                                global.currentViewIndex = 6
                                global.currentCustomAlbumUId = number
                                global.searchEditText = ""
                                systemMenu.popup()
                            }

                            forceActiveFocus()
                        }
                    }
                }

            }

            ListView{
                id : importList
                height:albumControl.getImportAlubumCount(global.albumChangeList) * 36
                width:parent.width
                visible: true
                interactive: false //禁用原有的交互逻辑，重新开始定制

                model: albumControl.getImportAlubumCount(global.albumChangeList)
                delegate:    ItemDelegate {
                    id:importitem
                    width: 180
                    height : 36
                    backgroundVisible: false
                    checked: index == 0
                    DciIcon {
                        id: imIcon
                        anchors.left: importitem.left; anchors.leftMargin: 10
                        anchors.verticalCenter: importitem.verticalCenter
                        name: "custom"
                        sourceSize: Qt.size(20, 20)
                    }
                    Label {
                        id: songNameIm
                        width: 100
                        anchors.left: imIcon.right; anchors.leftMargin: 10
                        anchors.verticalCenter: importitem.verticalCenter
                        text:albumControl.getImportAlubumAllNames(global.albumChangeList)[index]
                    }

                    onClicked: {
                        global.currentViewIndex = 6
                        global.currentCustomAlbumUId = albumControl.getImportAlubumAllId(global.albumChangeList)[index]
                        global.searchEditText = ""
                        forceActiveFocus()
                    }
                    ButtonGroup.group: global.siderGroup
                    MouseArea {

                        anchors.fill: parent
                        acceptedButtons:Qt.RightButton

                        enabled: true;

                        onClicked: {

                            if(mouse.button == Qt.RightButton) {
                                importMenu.popup()
                            }

                            forceActiveFocus()
                        }
                    }
                }

            }


            ListView{
                id : customList
                height:albumControl.getAllCustomAlbumId(global.albumChangeList).length * 42
                width:parent.width
                visible: true
                interactive: false //禁用原有的交互逻辑，重新开始定制

                model :albumControl.getAllCustomAlbumId(global.albumChangeList).length
                delegate:    ItemDelegate {
                    id:item
                    width: 180
                    height : 36
                    backgroundVisible: false
                    DciIcon {
                        id: siderIcon
                        anchors.left: item.left; anchors.leftMargin: 10
                        anchors.verticalCenter: item.verticalCenter
                        name: "item"
                        sourceSize: Qt.size(20, 20)
                    }
                    Label {
                        id: songName
                        width: 100
                        anchors.left: siderIcon.right; anchors.leftMargin: 10
                        anchors.verticalCenter: item.verticalCenter
                        text: albumControl.getAllCustomAlbumName(global.albumChangeList)[index]
                    }
                    LineEdit {
                        id :keyLineEdit
                        visible: false

                        height :36
                        width :180
                        text:albumControl.getAllCustomAlbumName(global.albumChangeList)[index]

                        onEditingFinished: {
                            item.checked = true;
                            songName.visible = true;
                            siderIcon.visible = true;
                            keyLineEdit.visible = false;
                            albumControl.renameAlbum( global.currentCustomAlbumUId, keyLineEdit.text)
                            global.albumChangeList=!global.albumChangeList
                        }
                        onActiveFocusChanged: {
//                            EventsFilter.setEnabled(!activeFocus)
                        }
                    }
                    //刷新自定义相册
                    Connections {
                        target: leftSidebar
                        onSigRename: {
                           if (currentCustomIndex ==index){
                               item.rename();
                           }
                        }
                    }

                    function enableRename(){
//
                        item.checked = true;
                        keyLineEdit.text = songName.text;
                        keyLineEdit.forceActiveFocus()
                        songName.visible = false;
                        siderIcon.visible = false;
                        keyLineEdit.visible = true;
                        item.checked = false;
                    }
                    function rename(){
                        enableRename();
                        keyLineEdit.selectAll();
                    }
                    // 屏蔽空格响应
                    Keys.onSpacePressed: { event.accepted=false; }
                    Keys.onReleased: { event.accepted=(event.key===Qt.Key_Space); }

                    ButtonGroup.group: global.siderGroup
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons:  Qt.LeftButton | Qt.RightButton

                        onClicked: {
                            currentCustomIndex=index
                            item.checked=true
                            global.currentViewIndex = 6
                            global.currentCustomAlbumUId = albumControl.getAllCustomAlbumId(global.albumChangeList)[index]
                            item.forceActiveFocus();
                            if(mouse.button == Qt.RightButton) {
                                customMenu.popup()
                            }
                            forceActiveFocus()
                        }
                        onDoubleClicked:{
                           item.rename();
                        }
                    }
                }

            }
        }
        Menu {
            id: systemMenu

            //显示大图预览
            RightMenuItem {
                text: qsTr("Slide show")
                visible: albumControl.getAlbumPaths(global.currentCustomAlbumUId).length >0
                onTriggered: {
                    stackControl.startMainSliderShow(albumControl.getAlbumPaths(global.currentCustomAlbumUId), 0)
                }
            }

            MenuSeparator {
            }

            RightMenuItem {
                text: qsTr("Export")
                visible:  albumControl.getAlbumPaths(global.currentCustomAlbumUId).length >0
                onTriggered: {
                    albumControl.getFolders(albumControl.getAlbumPaths(global.currentCustomAlbumUId))
                }
            }
        }

        Menu {
            id: importMenu

            //显示大图预览
            RightMenuItem {
                text: qsTr("Slide show")
                visible: albumControl.getAlbumPaths(global.currentCustomAlbumUId).length >0
                onTriggered: {
                    stackControl.startMainSliderShow(albumControl.getAlbumPaths(global.currentCustomAlbumUId), 0)
                }
            }

            MenuSeparator {
            }

            RightMenuItem {
                text: qsTr("Export")
                visible: albumControl.getAlbumPaths(global.currentCustomAlbumUId).length >0
                onTriggered: {
                    albumControl.getFolders(albumControl.getAlbumPaths(global.currentCustomAlbumUId))
                }
            }
            RightMenuItem {
                text: qsTr("Delete")
                onTriggered: {
                    albumControl.removeAlbum(global.currentCustomAlbumUId)
                    albumControl.removeCustomAutoImportPath(global.currentCustomAlbumUId)
                    //前往已导入
                    global.currentViewIndex == 3
                    global.albumChangeList=!global.albumChangeList
                }
            }
        }


        Menu {
            id: customMenu

            //显示大图预览
            RightMenuItem {
                text: qsTr("Slide show")
                visible: albumControl.getAlbumPaths(global.currentCustomAlbumUId).length >0
                onTriggered: {
                    stackControl.startMainSliderShow(albumControl.getAlbumPaths(global.currentCustomAlbumUId), 0)
                }
            }

            RightMenuItem {
                text: qsTr("New album")
                onTriggered: {
                    var x = parent.mapToGlobal(0, 0).x + parent.width / 2 - 190
                    var y = parent.mapToGlobal(0, 0).y + parent.height / 2 - 89
                    newAlbum.setX(x)
                    newAlbum.setY(y)
                    newAlbum.setNormalEdit()
                    newAlbum.show()
                }
            }
            RightMenuItem {
                text: qsTr("Rename")
                onTriggered: {
                    sigRename();
                }
            }

            MenuSeparator {
            }

            RightMenuItem {
                text: qsTr("Export")
                visible: albumControl.getAlbumPaths(global.currentCustomAlbumUId).length >0
                onTriggered: {
                    albumControl.getFolders(albumControl.getAlbumPaths(global.currentCustomAlbumUId))
                }
            }
            RightMenuItem {
                text: qsTr("Delete")
                onTriggered: {
                    albumControl.removeAlbum(global.currentCustomAlbumUId)
                    global.albumChangeList=!global.albumChangeList
                }
            }
        }

    }
}
