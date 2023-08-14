// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import org.deepin.dtk 1.0

import org.deepin.album 1.0 as Album

import "../../Control"
import "../../Control/ListView"
import "../../"
import "../"

Rectangle {
    width: parent.width
    height: parent.height

    property int filterType : filterCombo.currentIndex // 筛选类型，默认所有
    property string numLabelText: ""
    property string selectedText: getSelectedText(selectedPaths)
    property alias selectedPaths: theView.selectedUrls
    property int totalCount: 0

    onVisibleChanged: {
        if (visible) {
            flushRecentDelView()
        }
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        flushRecentDelView()
    }

    // 刷新最近删除视图内容
    function flushRecentDelView() {
        theView.proxyModel.refresh(filterType)
        global.selectedPaths = theView.selectedUrls
        getNumLabelText()
        totalCount = albumControl.getTrashInfoConut(0)
    }

    // 刷新总数标签
    function getNumLabelText() {
        //QML的翻译不支持%n的特性，只能拆成这种代码

        var photoCountText = ""
        var photoCount = albumControl.getTrashInfoConut(1)
        if(photoCount === 0) {
            photoCountText = ""
        } else if(photoCount === 1) {
            photoCountText = qsTr("1 photo")
        } else {
            photoCountText = qsTr("%1 photos").arg(photoCount)
        }

        var videoCountText = ""
        var videoCount = albumControl.getTrashInfoConut(2)
        if(videoCount === 0) {
            videoCountText = ""
        } else if(videoCount === 1) {
            videoCountText = qsTr("1 video")
        } else {
            videoCountText = qsTr("%1 videos").arg(videoCount)
        }

        numLabelText = filterType == 0 ? (photoCountText + (videoCountText !== "" ? ((photoCountText !== "" ? " " : "") + videoCountText) : ""))
                                           : (filterType == 1 ? photoCountText : videoCountText)
        if (visible) {
            global.statusBarNumText = numLabelText
        }
    }

    // 刷新选中项目标签内容
    function getSelectedText(paths) {
        var selectedNumText = global.getSelectedNumText(paths, numLabelText)
        if (visible)
            global.statusBarNumText = selectedNumText
        return selectedNumText
    }

    // 加载最近删除图片数据
    function loadRecentDelItems()
    {
        console.info("recently delete model has refreshed... filterType:", filterType)
        theView.thumbnailListModel.clear();
        var delInfos = albumControl.getTrashAlbumInfos(filterType);
        console.info("recently delete model has refreshed... filterType:", filterType, " done...")
        for (var key in delInfos) {
            var delItems = delInfos[key]
            for (var i = 0; i < delItems.length; i++) {
                theView.thumbnailListModel.append(delItems[i])
            }
            break;
        }
    }

    //执行图片删除操作
    function runAllDeleteImg() {
        albumControl.deleteImgFromTrash(theView.allPaths())
        theView.selectAll(false)
        global.sigFlushRecentDelView()
    }

    // 最近删除标题栏区域
    Item {
        id: recentDelTitleRect
        width: parent.width - global.verticalScrollBarWidth
        height: global.thumbnailViewTitleHieght - 10
        // 最近删除标签
        Label {
            id: recentDelLabel
            anchors {
                top: parent.top
                topMargin: 12
                left: parent.left
            }
            height: 30
            font: DTK.fontManager.t3
            text: qsTr("Trash")
        }

        Label {
            id: recentDelTipLabel
            anchors {
                top: recentDelLabel.bottom
                topMargin: 10
                left: parent.left
            }
            font: DTK.fontManager.t6
            text: qsTr("The files will be permanently deleted after the days shown on them")
        }

        //删除全部按钮
        Label {
            id: delAllBtn
            anchors {
                top: parent.top
                topMargin: 12
                right: parent.right
            }
            width: 56
            height: 36
            font: DTK.fontManager.t6
            text: qsTr("Delete All")
            visible: !theView.haveSelect && totalCount
            color: mouseAreaDelAllBtn.containsMouse ? "#FF5736" : "#FD6E52"


            MouseArea {
                id: mouseAreaDelAllBtn
                anchors.fill: parent
                hoverEnabled: true

                onClicked: {
                    deleteDialog.setDisplay(GlobalVar.FileDeleteType.TrashAll, theView.allPaths().length)
                    deleteDialog.show()
                }
            }
        }

        // 筛选下拉框
        FilterComboBox {
            id: filterCombo
            anchors {
                top: recentDelLabel.bottom
                topMargin: 4
                right: parent.right
            }
            width: 130
            height: 30
            visible: !theView.haveSelect && totalCount > 0
        }

        WarningButton {
            id: delSelectedBtn
            anchors {
                top: recentDelLabel.bottom
                topMargin: 4
                right: parent.right
            }
            width: 140
            height: 36
            font: DTK.fontManager.t6
            text: qsTr("Delete Selected (%1)").arg(theView.haveSelectCount)
            visible: theView.haveSelect

            onClicked: {
                deleteDialog.setDisplay(GlobalVar.FileDeleteType.TrashSel, theView.selectedPaths.length)
                deleteDialog.show()
            }
        }

        Button {
            id: restoreSelectedBtn
            anchors {
                top: recentDelLabel.bottom
                topMargin: 4
                right: delSelectedBtn.left
                rightMargin: 10
            }
            width: 140
            height: 36
            font: DTK.fontManager.t6
            text: qsTr("Restore Selected (%1)").arg(theView.haveSelectCount)
            visible: theView.haveSelect

            onClicked: {
                albumControl.recoveryImgFromTrash(theView.selectedPaths)
                theView.selectAll(false)
                global.sigFlushRecentDelView()
            }
        }

        MouseArea {
            id: theMouseArea
            anchors.fill: parent
            onPressed: {
                var gPos = theMouseArea.mapToGlobal(mouse.x, mouse.y)
                if (!restoreSelectedBtn.contains(restoreSelectedBtn.mapFromGlobal(gPos.x, gPos.y))
                        && !delSelectedBtn.contains(delSelectedBtn.mapFromGlobal(gPos.x, gPos.y))) {
                    theView.selectAll(false)
                }
                mouse.accepted = false
            }
        }
    }

    // 缩略图列表控件
    ThumbnailListView2 {
        id: theView
        anchors {
            top: recentDelTitleRect.bottom
            topMargin: 10
        }
        width: parent.width
        height: parent.height - recentDelTitleRect.height - m_topMargin - statusBar.height
        thumnailListType: GlobalVar.ThumbnailType.Trash

        proxyModel.sourceModel: Album.ImageDataModel { id: dataModel; modelType: Album.Types.RecentlyDeleted}

        visible: theView.count
        property int m_topMargin: 10

        // 监听缩略图列表选中状态，一旦改变，更新globalVar所有选中路径
        Connections {
            target: theView
            onSelectedChanged: {
                if (parent.visible)
                    global.selectedPaths = theView.selectedUrls
            }
        }
    }

    // 若没有数据，显示无图片视图
    NoPictureView {
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.RecentlyDeleted && numLabelText === "" && filterType === 0
    }

    Component.onCompleted: {
        global.sigFlushRecentDelView.connect(flushRecentDelView)
        deleteDialog.sigDoAllDeleteImg.connect(runAllDeleteImg)
    }
}
