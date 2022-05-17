import QtQuick 2.0
import "./ImageViewer"
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
}
