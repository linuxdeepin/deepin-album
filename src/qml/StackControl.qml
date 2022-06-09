import QtQuick 2.0
import "./PreviewImageViewer"
Rectangle {
    anchors.fill: parent
    //本文件用于替代stackwidget的作用，通过改变global的0-n来切换窗口
    MainAlbumView{
        id: mainAlbumView
        anchors.fill: parent
        visible: global.stackControlCurrent == 0 ?true :false
    }
    MainStack{
        id: mainStack
        anchors.fill: parent
        visible: global.stackControlCurrent == 1 ?true :false
        iconName: "deepin-album"
    }
    SliderShow{
        id: mainSliderShow
        anchors.fill: parent
        visible: global.stackControlCurrent == 2 ?true :false
    }

    //全局幻灯片信号槽
    Connections {
        target: mainSliderShow
        onBacktrack: {
            global.stackControlCurrent = 0
        }
    }

    //全屏动画
    PropertyAnimation {
        id :showfullAnimation
        target: root
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
        global.stackControlCurrent = 2
        showfullAnimation.start()
    }
}
