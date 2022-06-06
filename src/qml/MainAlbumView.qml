import QtQuick 2.0
import org.deepin.dtk 1.0
import "./ThumbnailImageView"
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
                leftSidebar.visible = !leftSidebar.visible
            }
        }
    }

    ThumbnailImage{
        id: thumbnailImage
        anchors.top: titleAlubmRect.bottom
        anchors.left: leftSidebar.right
        anchors.leftMargin: 0
        width: leftSidebar.visible ? parent.width - leftSidebar.width : root.width
        height: root.height - titleAlubmRect.height
    }

    StatusBar {
        id: statusBar
        anchors.bottom: parent.bottom
        anchors.left: leftSidebar.right
        width: leftSidebar.visible ? parent.width - leftSidebar.width : root.width
        height: 30

        onSliderValueChanged: {
            global.thumbnailSizeLevel = sliderValue
            fileControl.setConfigValue("StatusBar", "sliderValue", sliderValue)
        }

        Component.onCompleted: {
            var oldSliderValue = Number(fileControl.getConfigValue("StatusBar", "sliderValue", 0))
            setSliderWidgetValue(oldSliderValue)
        }
    }

    //拖拽导入
    DropArea {
        anchors.fill: parent

        onDropped: {
            albumControl.importAllImagesAndVideos(drop.urls)
        }

        onEntered: {
            if(drag.hasUrls) {
                var urls = drag.urls
                if(fileControl.checkMimeUrls(urls)) {
                    drag.accepted = true
                } else {
                    drag.accepted = false
                }
            } else {
                drag.accepted = false
            }
        }
    }
}
