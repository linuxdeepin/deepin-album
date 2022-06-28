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
    property int currentImportCustomIndex :0
    signal sigRename
    signal backCollection
    signal changeDevice
    signal sigDeleteItem
    signal sigDeleteCustomItem
    signal todoDraw
    property var devicePath : albumControl.getDevicePaths(global.deviceChangeList)
    property var albumPaths : albumControl.getAlbumPaths(global.currentCustomAlbumUId)
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
                        if (global.currentViewIndex === GlobalVar.ThumbnailViewType.Favorite) {
                            global.currentCustomAlbumUId = 0
                        }
                        global.searchEditText = ""

                        forceActiveFocus()
                    }
                    ButtonGroup.group: global.siderGroup

                    //刷新自定义相册
                    Connections {
                        target: leftSidebar
                        onBackCollection: {
                            if(number == "0"){
                                console.log(name)
                                picItem.checked=true
                                global.currentViewIndex = GlobalVar.ThumbnailViewType.Collecttion
                                global.searchEditText = ""
                                forceActiveFocus()
                            }
                        }
                    }
                }
            }

            RowLayout{
                Layout.alignment: Qt.AlignCenter
                spacing: 100

                visible: devicePath.length>0 ?true :false
                Label {
                    id:deviceLabel
                    height: 30
                    text: qsTr("Device")
                    color: Qt.rgba(0,0,0,0.3)
                }
            }

            ListView{
                id : deviceList
                height:devicePath.length *42
                width:parent.width
                visible: true
                interactive: false //禁用原有的交互逻辑，重新开始定制

                model :devicePath.length
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
                            if (devicePath.length == 1){
                                backCollection();
                            }else{
                                if(index -1>= 0){
                                    global.deviceCurrentPath=devicePath[index-1]
                                }else{

                                }

                               forceActiveFocus()
                            }
                            albumControl.unMountDevice(devicePath[index])
                        }
                    }
                    // 图片保存完成，缩略图区域重新加载当前图片
                    Connections {
                        target: albumControl
                        onSigAddDevice: {
                            console.log(path)
                            if (path === albumControl.getDevicePaths(global.deviceChangeList)[index]) {
                                deviccItem.checked =true
                            }
                        }
                    }
                    onCheckedChanged: {
                        if(deviccItem.checked ==true){
                            global.currentViewIndex = GlobalVar.ThumbnailViewType.Device
                            global.deviceCurrentPath=devicePath[index]
                            forceActiveFocus()
                        }
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
                        newAlbum.isChangeView = true
                        newAlbum.show()
                        forceActiveFocus()
                    }
                }
            }
            ListView{
                id : fixedList
                height:fixedListModel.count * 36
                width:parent.width
                visible: true
                interactive: false //禁用原有的交互逻辑，重新开始定制

                ListModel {
                    id: fixedListModel
                }

                model: fixedListModel

                delegate:    ItemDelegate {
                    id:sysitem
                    width: 180
                    height : 36
                    backgroundVisible: false
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
                        global.currentViewIndex = GlobalVar.ThumbnailViewType.CustomAlbum
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
                            sysitem.checked=true
                            global.currentViewIndex = GlobalVar.ThumbnailViewType.CustomAlbum
                            global.currentCustomAlbumUId = number
                            global.searchEditText = ""
                            if(mouse.button == Qt.RightButton) {
                                customMenu.popup()
                            }

                            forceActiveFocus()
                        }
                    }

                    //刷新自定义相册
                    Connections {
                        target: leftSidebar
                        onTodoDraw: {
                           if (number == 3){
                               sysitem.checked=true
                               global.currentViewIndex = GlobalVar.ThumbnailViewType.CustomAlbum
                               global.currentCustomAlbumUId = number
                               global.searchEditText = ""
                               forceActiveFocus()
                           }
                        }
                    }
                }

                Component.onCompleted: {
                    //根据文件夹情况刷新当前的默认路径相册显示
                    //1: 截图，2: 相机，3: 画板
                    if(albumControl.isDefaultPathExists(1)) {
                        fixedListModel.append({name: qsTr("Screen Capture"), number: "1", iconName :"screenshot"})
                    }

                    if(albumControl.isDefaultPathExists(2)) {
                        fixedListModel.append({name: qsTr("Camera"), number: "2", iconName :"camera"})
                    }

                    if(albumControl.isDefaultPathExists(3)) {
                        fixedListModel.append({name: qsTr("Draw"), number: "3", iconName :"draw"})
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
                    Connections {
                        target: albumControl
                        onSigAddCustomAlbum: {
                            if (UID === albumControl.getImportAlubumAllId(global.deviceChangeList)[index]) {
                                importitem.checked =true
                            }
                        }
                    }
                    onCheckedChanged: {
                        if(importitem.checked ==true){
                            global.currentViewIndex = GlobalVar.ThumbnailViewType.CustomAlbum
                            global.currentCustomAlbumUId = albumControl.getImportAlubumAllId(global.albumChangeList)[index]
                            global.searchEditText = ""
                            forceActiveFocus()
                        }
                    }
                    Connections {
                        target: leftSidebar
                        onSigDeleteCustomItem: {
                            console.log(currentImportCustomIndex)
                            console.log(albumControl.getImportAlubumAllId(global.albumChangeList).length)
                            if(currentImportCustomIndex >= albumControl.getImportAlubumAllId(global.albumChangeList).length ){
                                currentImportCustomIndex = albumControl.getImportAlubumAllId(global.albumChangeList).length -1
                            }
                            if (currentImportCustomIndex === index){
                                importitem.checked = true
                                global.currentViewIndex = GlobalVar.ThumbnailViewType.CustomAlbum
                                global.currentCustomAlbumUId = albumControl.getImportAlubumAllId(global.albumChangeList)[currentCustomIndex]
                                importitem.forceActiveFocus();
                                forceActiveFocus()
                            }
                        }
                    }

                    ButtonGroup.group: global.siderGroup
                    MouseArea {

                        anchors.fill: parent
                        acceptedButtons:Qt.RightButton

                        enabled: true;

                        onClicked: {
                            currentImportCustomIndex = index
                            importitem.checked =true

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
                            albumControl.renameAlbum( albumControl.getAllCustomAlbumId(global.albumChangeList)[index], keyLineEdit.text)
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

                    Connections {
                        target: leftSidebar
                        onSigDeleteItem: {
                            if(currentCustomIndex >= albumControl.getAllCustomAlbumId(global.albumChangeList).length ){
                                currentCustomIndex = albumControl.getAllCustomAlbumId(global.albumChangeList).length -1
                            }
                            if (currentCustomIndex === index){
                                item.checked=true
                                global.currentViewIndex = GlobalVar.ThumbnailViewType.CustomAlbum
                                global.currentCustomAlbumUId = albumControl.getAllCustomAlbumId(global.albumChangeList)[currentCustomIndex]
                                item.forceActiveFocus();
                                forceActiveFocus()
                            }
                        }
                    }
                    Connections {
                        target: newAlbum
                        onSigCreateAlbum: {
                            console.log(global.currentViewIndex)
                            if(global.currentViewIndex === GlobalVar.ThumbnailViewType.CustomAlbum){
                                console.log(global.currentViewIndex)
                                for(var i=0 ; i < albumControl.getAllCustomAlbumId(global.albumChangeList).length; i++) {
                                    if(albumControl.getAllCustomAlbumId(global.albumChangeList)[i] === global.currentCustomAlbumUId ){
                                        if(i == index){
                                            item.checked=true
                                            item.forceActiveFocus();
                                            forceActiveFocus()
                                        }
                                    }
                                }
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
                            global.currentViewIndex = GlobalVar.ThumbnailViewType.CustomAlbum
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
                visible: albumPaths.length >0
                onTriggered: {
                    stackControl.startMainSliderShow(albumPaths, 0)
                }
            }

            MenuSeparator {
            }

            RightMenuItem {
                text: qsTr("Export")
                visible:  albumPaths.length >0
                onTriggered: {
                    albumControl.exportFolders(albumPaths,albumControl.getCustomAlbumByUid(global.currentCustomAlbumUId))
                }
            }
        }

        Menu {
            id: importMenu

            //显示大图预览
            RightMenuItem {
                text: qsTr("Slide show")
                visible: albumPaths.length >0
                onTriggered: {
                    stackControl.startMainSliderShow(albumPaths, 0)
                }
            }

            MenuSeparator {
            }

            RightMenuItem {
                text: qsTr("Export")
                visible: albumPaths.length >0
                onTriggered: {
                    albumControl.exportFolders(albumPaths,albumControl.getCustomAlbumByUid(global.currentCustomAlbumUId))
                }
            }

            RightMenuItem {
                text: qsTr("Delete")
                onTriggered: {
                    removeAlbumDialog.deleteType = 1
                    removeAlbumDialog.show()
                }
            }
        }

        //删除相册窗口
        RemoveAlbumDialog {
            id: removeAlbumDialog
        }

        //删除执行函数，type由调用位置设置，用于模拟回调函数
        function doDeleteAlbum(type) {
            if(type === 0) {
                albumControl.removeAlbum(global.currentCustomAlbumUId)
                global.albumChangeList=!global.albumChangeList
                sigDeleteItem()
                if(albumControl.getAllCustomAlbumId(global.albumChangeList).length === 0){
                    todoDraw()
                }
            } else if(type === 1) {
                albumControl.removeAlbum(global.currentCustomAlbumUId)
                albumControl.removeCustomAutoImportPath(global.currentCustomAlbumUId)
                global.albumChangeList=!global.albumChangeList
                sigDeleteCustomItem()
                if(albumControl.getImportAlubumCount(global.albumChangeList) === 0){
                    todoDraw()
                }
            }

            //删完以后要执行界面跳转
            global.siderGroup.buttons[0].checked = true;
            global.currentViewIndex = 2
        }

        Component.onCompleted: {
            removeAlbumDialog.sigDoRemoveAlbum.connect(doDeleteAlbum)
        }

        Menu {
            id: customMenu

            //显示大图预览
            RightMenuItem {
                text: qsTr("Slide show")
                visible: albumPaths.length >0
                onTriggered: {
                    stackControl.startMainSliderShow(albumPaths, 0)
                }
            }

            RightMenuItem {
                text: qsTr("New album")
                visible: global.currentCustomAlbumUId > 3 ?true : false
                onTriggered: {
                    var x = parent.mapToGlobal(0, 0).x + parent.width / 2 - 190
                    var y = parent.mapToGlobal(0, 0).y + parent.height / 2 - 89
                    newAlbum.setX(x)
                    newAlbum.setY(y)
                    newAlbum.isChangeView = true
                    newAlbum.setNormalEdit()
                    newAlbum.show()
                }
            }

            RightMenuItem {
                text: qsTr("Rename")
                visible: global.currentCustomAlbumUId > 3 ?true : false
                onTriggered: {
                    sigRename();
                }
            }

            MenuSeparator {
            }

            RightMenuItem {
                text: qsTr("Export")
                visible: albumPaths.length >0
                onTriggered: {
                    albumControl.exportFolders(albumPaths,albumControl.getCustomAlbumByUid(global.currentCustomAlbumUId))
                }
            }

            RightMenuItem {
                text: qsTr("Delete")
                visible: global.currentCustomAlbumUId > 3 ?true : false
                onTriggered: {
                    removeAlbumDialog.deleteType = 0
                    removeAlbumDialog.show()
                }
            }
        }
    }
}
