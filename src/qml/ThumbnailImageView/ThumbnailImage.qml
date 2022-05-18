import QtQuick 2.0
import org.deepin.dtk 1.0
import QtQuick.Controls 2.4
import QtQml 2.11
import "./CollecttionView"
import "./HaveImportedView"
import "./MyFavoriteView"
import "./RecentlyDeletedView"
import "./CustomAlbum"
import "./"
//本文件用于替代stackwidget的作用，通过改变global的0-n来切换窗口
Rectangle{

    ImportView{
        visible: global.currentViewIndex == 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
    NoPictureView{
        visible: global.currentViewIndex == 1
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
    HaveImportedView{
        visible: global.currentViewIndex == 2
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
}
