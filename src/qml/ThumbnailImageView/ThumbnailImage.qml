import QtQuick 2.9
import QtQml.Models 2.11
import QtQuick.Window 2.2
import QtQuick.Controls 2.4
import org.deepin.dtk.impl 1.0 as D

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
    signal escKeyPressed()

    function setCollecttionViewIndex(index) {
        collecttionView.setIndex(index)
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
        id: haveImportedView
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

    Shortcut {
        enabled: global.stackControlCurrent === 0
        autoRepeat: false
        sequence: "Esc"
        onActivated: {
            escKeyPressed()
        }
    }

    Shortcut {
        enabled: true
        autoRepeat: false
        sequence : "Ctrl+A"
        onActivated : {
            global.sigSelectAll(true)
        }
    }

    Shortcut {
        enabled: visible && menuItemStates.canCopy
        autoRepeat: false
        sequence : "Ctrl+C"
        onActivated : {
            fileControl.copyImage(global.selectedPaths)
        }
    }

    Shortcut {
        enabled: visible && menuItemStates.canDelete
        autoRepeat: false
        sequence : "Delete"
        onActivated : {          
            if (menuItemStates.isInTrash) {
                deleteDialog.setDisplay(menuItemStates.isInTrash ? GlobalVar.FileDeleteType.TrashSel : GlobalVar.FileDeleteType.Normal, global.selectedPaths.length)
                deleteDialog.show()
            } else {
                deleteDialog.deleteDirectly()
            }
        }
    }

    Shortcut {
        enabled: visible && menuItemStates.canRotate
        autoRepeat: false
        sequence : "Ctrl+R"
        onActivated : {
            fileControl.rotateFile(global.selectedPaths, 90)
        }
    }

    Shortcut {
        enabled: visible && menuItemStates.canRotate
        autoRepeat: false
        sequence : "Ctrl+Shift+R"
        onActivated : {
            fileControl.rotateFile(global.selectedPaths, -90)
        }
    }

    Shortcut {
        enabled: visible
        autoRepeat: true
        sequence : "Page Up"
        onActivated : {
            global.sigPageUp()
        }
    }

    Shortcut {
        enabled: visible
        autoRepeat: true
        sequence : "Page Down"
        onActivated : {
            global.sigPageDown()
        }
    }

    Shortcut {
        enabled: visible && leftSidebar.visible
        autoRepeat: false
        sequence: "Ctrl+Shift+N"
        onActivated: {
            newAlbum.setNormalEdit()
            newAlbum.isChangeView = true
            newAlbum.show()
            forceActiveFocus()
        }
    }

    Shortcut {
        enabled: true
        autoRepeat: false
        sequence: "F1"
        onActivated: {
            D.ApplicationHelper.handleHelpAction()
        }
    }

    Shortcut {
        enabled: true
        autoRepeat: false
        sequence: "Ctrl+Shift+/"
        onActivated: {
            albumControl.ctrlShiftSlashShortcut(root.x, root.y, root.width, root.height)
        }
    }
}
