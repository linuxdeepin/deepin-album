// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.0
import "./PreviewImageViewer"
Item {
    anchors.fill: parent
    //本文件用于替代stackwidget的作用，通过改变global的0-n来切换窗口
    MainAlbumView{
        id: mainAlbumView
        visible: global.stackControlCurrent == 0 ?true :false
    }
    MainStack{
        id: mainStack
        anchors.fill: parent
        visible: global.stackControlCurrent == 1 ?true :false
        iconName: "deepin-album"
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
        GControl.setImageFiles(images, images[startIndex])
        fileControl.resetImageFiles(images)
        mainStack.switchSliderShow()
        global.stackControlCurrent = 1
        showfullAnimation.start()
    }
}
