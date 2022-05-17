import QtQuick 2.0
import org.deepin.dtk 1.0

Rectangle {
    anchors.fill: parent
    AlbumTitle{
        id:titleAlubmRect
        z:parent.z+1
    }

    Sidebar{
        id : leftSidebar
        width: visible ? 200 : 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        visible: true
        ActionButton {
            visible: leftSidebar.visible ? true : false
            id: appTitleIconLeft
            anchors.top:parent.top
            anchors.topMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            width :  leftSidebar.visible ? 50 : 0
            height : 50
            icon {
                name: "deepin-album"
                width: 36
                height: 36
            }
        }

        ActionButton {
            visible: leftSidebar.visible ? true : false
            id: showHideleftSidebarLeftButton
            anchors.top:parent.top
            anchors.topMargin: 0
            anchors.left: appTitleIconLeft.right
            anchors.leftMargin: 0
            width :  leftSidebar.visible ? 50 : 0
            height : 50
            icon {
                name: "topleft"
                width: 36
                height: 36
            }
            onClicked :{
                leftSidebar.visible=!leftSidebar.visible
            }
        }
    }

    ThumbnailImage{
        anchors.top: root.top
        anchors.left: leftSidebar.right
        anchors.leftMargin: 0
        width: leftSidebar.visible ? parent.width - leftSidebar.width : root.width
        height: root.height
    }
}
