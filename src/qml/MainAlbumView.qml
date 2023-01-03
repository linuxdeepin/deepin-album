import QtQuick 2.11
import QtQuick.Window 2.11
import org.deepin.dtk 1.0
import "./ThumbnailImageView"
import "./Control"
import "./SideBar"
import "./PopProgress"
Rectangle {
    color: Qt.rgba(0,0,0,0)
    anchors.fill: parent
    property int lastWidth: 0

    //rename窗口
    NewAlbumDialog {
        id: newAlbum
    }
    //delete窗口
    DeleteDialog {
        id: deleteDialog
    }

    // 侧边导航栏
    Sidebar{
        id : leftSidebar
        width: visible ? global.sideBarWidth : 0
        anchors.top: parent.top
        anchors.topMargin: 69
        anchors.bottom: parent.bottom

        visible: true
        z: thumbnailImage.z + 1

        Component.onCompleted: {
            x =  parent.width <= global.needHideSideBarWidth ? -global.sideBarWidth : 0
        }
    }

    // 侧边栏跟随窗口尺寸展开/收起
    onWidthChanged: {
        if (width <= global.needHideSideBarWidth) {
            if (leftSidebar.x === 0 && (lastWidth > width)) {
                hideSliderAnimation.start()
            }
        } else {
            if (leftSidebar.x < 0 && (lastWidth < width)) {
                showSliderAnimation.start()
            }
        }

        lastWidth = width
    }

    Connections {
        target: titleAlubmRect
        onShowHideSideBar: {
            if (bShow)
                showSliderAnimation.start()
            else
                hideSliderAnimation.start()
        }
    }

    //左右按钮隐藏动画
    NumberAnimation {
        id :hideSliderAnimation
        target: leftSidebar
        from: leftSidebar.x
        to: -global.sideBarWidth
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
        width: parent.width - leftSidebar.x - global.sideBarWidth
        height: root.height - titleAlubmRect.height
    }

    StatusBar {
        id: statusBar
        anchors.bottom: parent.bottom
        anchors.left: leftSidebar.right
//        width: leftSidebar.x == 0 ? parent.width - leftSidebar.width : root.width
        width: parent.width - leftSidebar.x - global.sideBarWidth
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

    //标准弹出式进度条窗口
    StandardProgressDialog {
        id: idStandardProgressDialog
        z: leftSidebar.z + 1
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
                    albumControl.importAllImagesAndVideosUrl(drop.urls, global.currentCustomAlbumUId, false)
                    albumControl.addCustomAlbumInfos(global.currentCustomAlbumUId,drop.urls)
                }
            } else {
                albumControl.importAllImagesAndVideosUrl(drop.urls, global.currentCustomAlbumUId, true)
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

    Connections {
        target: albumControl
        //收到导入开始消息
        onSigImportStart: {
            idStandardProgressDialog.clear()
            idStandardProgressDialog.setTitle(qsTr("Importing..."))
            var prevS = qsTr("Imported:")
            var suffixS = "0"
            idStandardProgressDialog.setContent(prevS + suffixS)
            idStandardProgressDialog.setProgress(0, 100)
            idStandardProgressDialog.show()
            leftSidebar.enabled = false
            thumbnailImage.enabled = false
            titleAlubmRect.enabled = false
        }

        //收到导入进度消息
        onSigImportProgress: {
            var prevS = qsTr("Imported:")
            var suffixS = qsTr("%1/%2").arg(value).arg(max)
            idStandardProgressDialog.setContent(prevS + suffixS)
            idStandardProgressDialog.setProgress(value, max)
        }

        //收到导入完成消息
        onSigImportFinished: {
            idStandardProgressDialog.close()
            leftSidebar.enabled = true
            thumbnailImage.enabled = true
            titleAlubmRect.enabled = true
            DTK.sendMessage(stackControl, qsTr("Import successful"), "notify_checked")
        }

        //收到导入完成消息
        onSigRepeatUrls: {
            idStandardProgressDialog.close()
            leftSidebar.enabled = true
            thumbnailImage.enabled = true
            titleAlubmRect.enabled = true
            DTK.sendMessage(stackControl, qsTr("Import failed"), "warning")
        }
    }
}
