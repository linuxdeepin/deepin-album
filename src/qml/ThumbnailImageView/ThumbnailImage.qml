import QtQuick 2.0
import org.deepin.dtk 1.0
import QtQuick.Controls 2.4
import QtQml 2.11
import "./CollecttionView"
import "./HaveImportedView"
import "./RecentlyDeletedView"
import "./CustomAlbum"
import "./DeviceAlbum"
import "./"

import "./../Control"
import "./../PreviewImageViewer"
//本文件用于替代stackwidget的作用，通过改变global的0-n来切换窗口

Rectangle{
    property int m_topMargin: 0
    property int m_leftMargin: 20

    function setCollecttionViewIndex(index) {
        collecttionView.setIndex(index)
    }

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
    CollecttionView{
        id: collecttionView
        visible: global.currentViewIndex == 2
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
        anchors.bottomMargin: statusBar.height
    }
    HaveImportedView{
        visible: global.currentViewIndex == 3
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    CustomAlbum{
        visible: global.currentViewIndex == 4
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    RecentlyDeletedView{
        visible: global.currentViewIndex == 5
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    CustomAlbum{
        visible: global.currentViewIndex == 6
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    SearchView{
        visible: global.currentViewIndex == 7
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    DeviceAlbum{
        visible: global.currentViewIndex == 8
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }

    //rename窗口
    ExportDialog {
        id: exportdig
    }
    //info的窗口
    InfomationDialog{
        id: albumInfomationDig
    }

    //视频info窗口
    VideoInfoDialog{
        id: videoInfomationDig
    }

    Connections {
        target: newAlbum
        onSigCreateAlbum:
        {
            var index = albumControl.getAllCustomAlbumId(global.albumChangeList).length - 1
            if(newAlbum.isChangeView){
                global.currentViewIndex = 6
            }
            else{
                albumControl.insertIntoAlbum(albumControl.getAllCustomAlbumId(global.albumChangeList)[index] , global.selectedPaths)
            }

            global.currentCustomAlbumUId = albumControl.getAllCustomAlbumId(global.albumChangeList)[index]
        }
    }
}
