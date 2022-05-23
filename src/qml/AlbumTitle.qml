import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import Qt.labs.folderlistmodel 2.11
import org.deepin.dtk 1.0

Rectangle {

    anchors.top : root.top
    anchors.left : leftSidebar.right
    anchors.leftMargin: 0
    width: leftSidebar.visible ? parent.width-leftSidebar.width : root.width
    height: 50
    //        color:titlecontrol.ColorSelector.backgroundColor


//    MouseArea { //为窗口添加鼠标事件
//        anchors.fill: parent
//        acceptedButtons: Qt.LeftButton //只处理鼠标左键
//        property point clickPos: "0,0"
//        onPressed: { //接收鼠标按下事件
//            clickPos  = Qt.point(mouse.x,mouse.y)
//            sigTitlePress()
//        }
//        onPositionChanged: { //鼠标按下后改变位置
//            //鼠标偏移量
//            var delta = Qt.point(mouse.x-clickPos.x, mouse.y-clickPos.y)

//            //如果mainwindow继承自QWidget,用setPos
//            root.setX(root.x+delta.x)
//            root.setY(root.y+delta.y)
//        }
//    }



    TitleBar {
        id : title
        anchors.fill: parent
        width: parent.width
        aboutDialog: AboutDialog {
            icon: "deepin-album"
            width: 400
            modality: Qt.NonModal
            version: qsTr(String("Version: %1").arg(Qt.application.version))
            description: qsTr("Album is a fashion manager for viewing and organizing photos and videos.")
            productName: qsTr("deepin-album")
            websiteName: DTK.deepinWebsiteName
            websiteLink: DTK.deepinWebsitelLink
            license: qsTr(String("%1 is released under %2").arg(productName).arg("GPLV3"))
        }
        ActionButton {
            visible: leftSidebar.visible ? false : true
            id: appTitleIcon
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            width :  leftSidebar.visible ? 0 : 50
            height : 50
            icon {
                name: "deepin-album"
                width: 36
                height: 36
            }
        }

        ActionButton {
            visible: leftSidebar.visible ? false : true
            id: showHideleftSidebarButton
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.left: appTitleIcon.right
            anchors.leftMargin: 0
            width :  leftSidebar.visible ? 0 : 50
            height : 50
            icon {
                name: "topleft"
                width: 36
                height: 36
            }
            onClicked :{
                leftSidebar.visible = !leftSidebar.visible
            }
        }

        ActionButton {
            id: range1Button
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.left: showHideleftSidebarButton.right
            anchors.leftMargin: 0
            width:50
            height:50
            icon {
                name: "range1"
                width: 36
                height: 36
            }
            onClicked: {
                //1.图片推送器切换
                publisher.switchLoadMode()

                //2.发送全局信号，所有的缩略图强制刷新
                global.sigThumbnailStateChange()
            }
        }
        ButtonBox {

            anchors.top: parent.top
            anchors.topMargin: 7
            anchors.left: range1Button.right
            anchors.leftMargin: 0
            height:36

            ToolButton {
                Layout.preferredHeight: parent.height
                checkable: true;
                text: qsTr("Year") ;
                checked: true
            }
            ToolButton {
                Layout.preferredHeight: parent.height
                checkable: true;
                text: qsTr("Month")
            }
            ToolButton {
                Layout.preferredHeight: parent.height
                checkable: true;
                text: qsTr("Day")
            }
            ToolButton {
                Layout.preferredHeight: parent.height
                checkable: true;
                text: qsTr("All items")
            }
        }
        SearchEdit{
            placeholder: qsTr("Search")
            width: 240
            anchors.top: parent.top
            anchors.topMargin: 7
            anchors.left: parent.left
            anchors.leftMargin: ( parent.width - width )/2
        }

        ActionButton {

            visible: true
            id: titleImportBtn
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.right: parent.right
            anchors.rightMargin: 4 * parent.height
            width: 50
            height: 50
            icon {
                name: "import"
                width: 36
                height: 36
            }
            onClicked :{
                importDialog.open()
            }
        }
        ActionButton {
            id: titleCollectionBtn
            visible: titleImportBtn.visible?false : true
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.right: titleRotateBtn.left
            width: 50
            height: 50
            icon {
                name: "toolbar-collection"
                width: 36
                height: 36
            }
        }

        ActionButton {
            id: titleRotateBtn
            visible: titleImportBtn.visible?false : true
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.right: titleTrashBtn.left
            width: 50
            height: 50
            icon {
                name: "felete"
                width: 36
                height: 36
            }
        }
        ActionButton {
            id: titleTrashBtn
            visible: titleImportBtn.visible?false : true
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.right: parent.right
            anchors.rightMargin: 4 * parent.height
            width: 50
            height: 50
            icon {
                name: "delete"
                width: 36
                height: 36
            }
        }
    }

}
