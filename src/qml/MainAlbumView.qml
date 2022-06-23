import QtQuick 2.11
import QtQuick.Window 2.11
import org.deepin.dtk 1.0
import "./ThumbnailImageView"
import "./Control"
Rectangle {
    anchors.fill: parent
    AlbumTitle{
        id:titleAlubmRect
        z:100
    }
    //rename窗口
    NewAlbumDialog {
        id: newAlbum
    }
    Sidebar{
        id : leftSidebar
        width: visible ? 200 : 0
//        anchors.left: parent.left
//        anchors.leftMargin: 0
        visible: true
        z: thumbnailImage.z + 1
//        ActionButton {
//            visible: leftSidebar.x == 0 ? true : false
//            id: appTitleIconLeft
//            anchors.top:parent.top
//            anchors.topMargin: 0
//            anchors.left: parent.left
//            anchors.leftMargin: 0
//            width :  leftSidebar.x == 0 ? 50 : 0
//            height : 50
//            icon {
//                name: "deepin-album"
//                width: 36
//                height: 36
//            }
//        }

//        ActionButton {
//            visible: leftSidebar.x == 0 ? true : false
//            id: showHideleftSidebarLeftButton
//            anchors.top:parent.top
//            anchors.topMargin: 0
//            anchors.left: appTitleIconLeft.right
//            anchors.leftMargin: 0
//            width :  leftSidebar.x == 0 ? 50 : 0
//            height : 50
//            icon {
//                name: "topleft"
//                width: 36
//                height: 36
//            }
//            onClicked :{
////                leftSidebar.visible = !leftSidebar.visible
//                if(!leftSidebar.x == 0 ){
//                    showSliderAnimation.start()
//                }
//                else{
//                    hideSliderAnimation.start()
//                }
//            }
//        }
    }
    //左右按钮隐藏动画
    NumberAnimation {
        id :hideSliderAnimation
        target: leftSidebar
        from: leftSidebar.x
        to: -200
        property: "x"
        duration: 200
        easing.type: Easing.InOutQuad
    }
    //左右按钮隐藏动画
    NumberAnimation {
        id :showSliderAnimation
        target: leftSidebar
        from: leftSidebar.x
        to: 0
        property: "x"
        duration: 200
        easing.type: Easing.InOutQuad
    }

    ThumbnailImage{
        id: thumbnailImage
        anchors.top: titleAlubmRect.bottom
        anchors.left: leftSidebar.right
        anchors.leftMargin: 0
        width: parent.width - leftSidebar.x - 200
        height: root.height - titleAlubmRect.height
        signal escKeyPressed()

        Shortcut {
            enabled: true
            sequence: "Esc"
            onActivated: {
                thumbnailImage.escKeyPressed()
            }
        }
    }

    StatusBar {
        id: statusBar
        anchors.bottom: parent.bottom
        anchors.left: leftSidebar.right
//        width: leftSidebar.x == 0 ? parent.width - leftSidebar.width : root.width
        width: parent.width - leftSidebar.x - 200
        height: 30

        onSliderValueChanged: {
            global.thumbnailSizeLevel = sliderValue
            fileControl.setConfigValue("", "album-zoomratio", sliderValue)
        }

        Component.onCompleted: {
            var oldSliderValue = Number(fileControl.getConfigValue("", "album-zoomratio", 4))
            setSliderWidgetValue(oldSliderValue)
        }
    }

    //拖拽导入
    DropArea {
        anchors.fill: parent

        onDropped: {
            albumControl.importAllImagesAndVideosUrl(drop.urls)
            if(global.currentViewIndex == 6 && albumControl.isCustomAlbum(global.currentCustomAlbumUId)){
                albumControl.addCustomAlbumInfos(global.currentCustomAlbumUId,drop.urls)
            }
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
