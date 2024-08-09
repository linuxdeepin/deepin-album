// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import "./PreviewImageViewer"

import org.deepin.album 1.0 as Album

Item {
    anchors.fill: parent
    //本文件用于替代stackwidget的作用，通过改变GStatus的0-n来切换窗口
    MainAlbumView{
        id: mainAlbumView
        visible: GStatus.stackControlCurrent === 0
    }
    MainStack{
        id: mainStack
        anchors.fill: parent
        show: GStatus.stackControlCurrent === 1
        iconName: "deepin-album"
    }
    SliderShow{
        id: mainSliderShow
        anchors.fill: parent
        visible: GStatus.stackControlCurrent === 2
    }

    //全局幻灯片信号槽
    Connections {
        target: mainSliderShow
        onBacktrack: {
            GStatus.stackControlCurrent = 0
        }
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

    //相册界面启动幻灯片（和看图界面启动有区别）
    //images: 图片路径 startIndex: 启动后一个图的下标索引
    function startMainSliderShow(images, startIndex) {
        showFullScreen()
        mainSliderShow.images = images
        mainSliderShow.modelCount = images.length
        mainSliderShow.autoRun = true
        mainSliderShow.indexImg = startIndex
        GStatus.stackControlCurrent = 2
        showfullAnimation.start()
    }
}
