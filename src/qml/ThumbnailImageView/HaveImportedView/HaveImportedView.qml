// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import org.deepin.dtk 1.0

import org.deepin.album 1.0 as Album

import "../../Control"
import "../../"
import "../"

BaseView {
    property int filterType : filterCombo.currentIndex // 筛选类型，默认所有
    property string numLabelText: "" //总数标签显示内容
    property string selectedText: getSelectedText(selectedPaths)
    property alias selectedPaths: theView.selectedPaths
    property real titleOpacity: 0.7
    property bool bShowImportTips: GStatus.currentViewType === Album.Types.ViewHaveImported && numLabelText === "" && filterType === 0

    Connections {
        target: albumControl
        onSigRepeatUrls: {
            if (visible) {
                theView.sigUnSelectAll()
                theView.selectedPaths = urls
                GStatus.selectedPaths = selectedPaths
            }
        }
    }

    onVisibleChanged: {
        if (visible)
            flushHaveImportedView()
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        if (visible)
            flushHaveImportedView()
    }

    // 刷新已导入视图内容
    function flushHaveImportedView() {
        theView.importedListModel.loadImportedInfos()
        theView.updateSelectedPaths()
        getNumLabelText()
    }

    // 刷新总数标签
    function getNumLabelText() {
        //QML的翻译不支持%n的特性，只能拆成这种代码

        var photoCountText = ""
        var photoCount = albumControl.getAllInfoConut(1)
        if(photoCount === 0) {
            photoCountText = ""
        } else if(photoCount === 1) {
            photoCountText = qsTr("1 photo")
        } else {
            photoCountText = qsTr("%1 photos").arg(photoCount)
        }

        var videoCountText = ""
        var videoCount = albumControl.getAllInfoConut(2)
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
            GStatus.statusBarNumText = numLabelText
        }
    }

    // 刷新选中项数标签
    function getSelectedText(paths) {
        var selectedNumText = GStatus.getSelectedNumText(paths, numLabelText)
        if (visible)
            GStatus.statusBarNumText = selectedNumText
        return selectedNumText
    }

    // 已导入视图标题栏区域
    Item {
        id: importedTitleRect
        width: parent.width - GStatus.verticalScrollBarWidth
        height: GStatus.thumbnailViewTitleHieght
        z:3

        // 已导入标签
        Label {
            id: importedLabel
            anchors {
                top: parent.top
                topMargin: 12
                left: parent.left
            }
            height: 30
            font: DTK.fontManager.t3
            text: qsTr("Import")
        }

        Label {
            anchors {
                top: importedLabel.bottom
                topMargin: 10
                left: parent.left
            }
            visible: false
            font: DTK.fontManager.t6
            text: qsTr("0 item")
        }

        // 筛选下拉框
        FilterComboBox {
            id: filterCombo
            anchors {
                top: importedLabel.bottom
                topMargin: 4
                right: parent.right
            }
            width: 115
            height: 30
        }
    }

    // 已导入列表控件
    ImportedlListView {
        id: theView
        anchors.fill: parent
    }

    // 若没有数据，显示导入图片视图
    NoPictureView {
        visible: bShowImportTips
        bShowImportBtn: true
        iconName: "nopicture1"
    }

    Component.onCompleted: {
        GStatus.sigFlushHaveImportedView.connect(flushHaveImportedView)
    }
}
