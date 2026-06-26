// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import "./PreviewImageViewer"
import "./PreviewImageViewer/InformationDialog"
import "./Control"

import org.deepin.album 1.0 as Album

Item {
    anchors.fill: parent
    //本文件用于替代stackwidget的作用，通过改变GStatus的0-n来切换窗口
    MainAlbumView{
        id: mainAlbumView
        idName: "mainAlbumView"
        show: GStatus.stackControlCurrent === 0
    }
    MainStack{
        id: mainStack
        idName: "mainStack"
        anchors.fill: parent
        show: GStatus.stackControlCurrent === 1
        iconName: "deepin-album"
    }
    SliderShow{
        id: mainSliderShow
        idName: "albumview"
        anchors.fill: parent
        visible: GStatus.stackControlCurrent === 2
    }

    //全屏动画
    PropertyAnimation {
        id :showfullAnimation
        target: window
        from: 0
        to: 1
        property: "opacity"
        duration: 200
        easing.type: Easing.InExpo
    }

    GlobalVar{
        id: global
    }

    MenuItemStates {
        id: menuItemStates
    }

    Loader {
        id: emptyWarningDig
        active: false
        sourceComponent: EmptyWarningDialog {
        }

        function show() {
            active = true
            if (item)
                item.show()
        }
    }

    //delete窗口
    Loader {
        id: deleteDialog
        active: false
        sourceComponent: DeleteDialog {
            onSigDoDeleteImg: deleteDialog.sigDoDeleteImg()
            onSigDoAllDeleteImg: deleteDialog.sigDoAllDeleteImg()
        }

        signal sigDoDeleteImg()
        signal sigDoAllDeleteImg()

        function ensureDialog() {
            active = true
            return item
        }

        function setDisplay(deltype, count) {
            var dialog = ensureDialog()
            if (dialog)
                dialog.setDisplay(deltype, count)
        }

        function show() {
            var dialog = ensureDialog()
            if (dialog)
                dialog.show()
        }

        function deleteDirectly() {
            var dialog = ensureDialog()
            if (dialog)
                dialog.deleteDirectly()
        }
    }

    //export窗口
    Loader {
        id: exportdig
        active: false
        sourceComponent: ExportDialog {
        }

        function ensureDialog() {
            active = true
            return item
        }

        function setParameter(path, toId) {
            var dialog = ensureDialog()
            if (dialog)
                dialog.setParameter(path, toId)
        }

        function show() {
            var dialog = ensureDialog()
            if (dialog)
                dialog.show()
        }
    }

    //info的窗口
    Component {
        id: informationDialogComponent
        InformationDialog {
        }
    }

    Loader {
        id: infomationDig
        sourceComponent: informationDialogComponent
        active: GStatus.showImageInfo
        asynchronous: true

        function show() {
            GStatus.showImageInfo = true;
        }
    }

    //视频info窗口
    Loader {
        id: videoInfomationDig
        active: false
        sourceComponent: VideoInfoDialog {
        }

        function show() {
            active = true
            if (item)
                item.show()
        }
    }

    //相册界面启动幻灯片（和看图界面启动有区别）
    //images: 图片路径 startIndex: 启动后一个图的下标索引
    function startMainSliderShow(images, startIndex) {
        showfullAnimation.start()
        showFullScreen()
        GControl.setImageFiles(images, images[startIndex])
        FileControl.resetImageFiles(images);
        mainStack.switchImageView()
        mainSliderShow.autoRun = true
        mainSliderShow.source = "image://ImageLoad/" + GControl.currentSource + "#frame_" + GControl.currentFrameIndex;
        mainSliderShow.restart()
        GStatus.stackControlCurrent = 2
        mainAlbumView.visible = false
    }
}
