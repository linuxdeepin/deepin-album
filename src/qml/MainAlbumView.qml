import QtQuick 2.11
import QtQuick.Window 2.11
import org.deepin.dtk 1.0
import "./ThumbnailImageView"
import "./Control"
import "./SideBar"
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
    // 侧边导航栏
    Sidebar{
        id : leftSidebar
        width: visible ? 200 : 0
        anchors.top: parent.top
        anchors.topMargin: 69
        anchors.bottom: parent.bottom

        visible: true
        z: thumbnailImage.z + 1

//        Rectangle {
//            anchors.fill: parent
//            color: Qt.rgba(0.3,0.3,0.3,0.3)
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
        clip: true
        anchors.top: titleAlubmRect.bottom
        anchors.left: leftSidebar.right
        anchors.leftMargin: 0
        width: parent.width - leftSidebar.x - 200
        height: root.height - titleAlubmRect.height
    }

    StatusBar {
        id: statusBar
        anchors.bottom: parent.bottom
        anchors.left: leftSidebar.right
//        width: leftSidebar.x == 0 ? parent.width - leftSidebar.width : root.width
        width: parent.width - leftSidebar.x - 200
        height: global.statusBarHeight

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
            if(global.currentViewIndex == 6 && albumControl.isCustomAlbum(global.currentCustomAlbumUId)) {
                var albumPaths = albumControl.getAlbumPaths(global.currentCustomAlbumUId)
                var urls = []
                for (var i = 0; i < drop.urls.length; i++) {
                    urls.push(drop.urls[i])
                }
                if (!albumControl.checkRepeatUrls(albumPaths, urls)) {
                    albumControl.importAllImagesAndVideosUrl(drop.urls, false)
                    albumControl.addCustomAlbumInfos(global.currentCustomAlbumUId,drop.urls)
                }
            } else {
                if(albumControl.importAllImagesAndVideosUrl(drop.urls, true)) {
                    DTK.sendMessage(stackControl, qsTr("Import successful"), "checked")
                }
            }
        }

        onEntered: {
            if(drag.hasUrls) {
                var urls = drag.urls
                if(fileControl.checkMimeUrls(urls)) {
                    if (global.currentViewIndex === GlobalVar.ThumbnailViewType.CustomAlbum) {
                        // 仅自定义相册允许拖拽导入
                        drag.accepted = albumControl.isCustomAlbum(global.currentCustomAlbumUId)
                    } else if (global.currentViewIndex === GlobalVar.ThumbnailViewType.Import
                               || global.currentViewIndex === GlobalVar.ThumbnailViewType.Collecttion
                               || global.currentViewIndex === GlobalVar.ThumbnailViewType.HaveImported) {
                        drag.accepted = true
                    } else {
                        drag.accepted = false
                    }
                } else {
                    drag.accepted = false
                }
            } else {
                drag.accepted = false
            }
        }
    }
}
