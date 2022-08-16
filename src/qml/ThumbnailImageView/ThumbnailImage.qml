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
import "./../"
//本文件用于替代stackwidget的作用，通过改变global的0-n来切换窗口

Rectangle{
    property int m_topMargin: 0
    property int m_leftMargin: 20
    property int m_CollecttionCurrentViewIndex: collecttionView.currentViewIndex

    function setCollecttionViewIndex(index) {
        collecttionView.setIndex(index)
    }

    ImportView{
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.Import
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
    NoPictureView{
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.NoPicture
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
    CollecttionView{
        id: collecttionView
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.Collecttion
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
        anchors.bottomMargin: statusBar.height
    }
    HaveImportedView{
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.HaveImported
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    CustomAlbum{
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.Favorite
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    RecentlyDeletedView{
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.RecentlyDeleted
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    CustomAlbum{
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.CustomAlbum
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    SearchView{
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.SearchResult
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    DeviceAlbum{
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.Device
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }

    //delete窗口
    DeleteDialog {
        id: deleteDialog
    }

    //export窗口
    ExportDialog {
        id: exportdig
    }

    EmptyWarningDialog {
        id: emptyWarningDig
    }

    //info的窗口
    InfomationDialog{
        id: albumInfomationDig
    }

    //视频info窗口
    VideoInfoDialog{
        id: videoInfomationDig
    }
}
