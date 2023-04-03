// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import org.deepin.dtk 1.0

import org.deepin.album 1.0 as Album

import "../../Control"
import "../../Control/ListView"
import "../../"

Item {
    property int filterType: filterCombo.currentIndex // 筛选类型，默认所有
    property string numLabelText:""
    property string selectedText: getSelectedNum(selectedPaths)
    property var selectedPaths: []

    function setDateRange(str) {
        dateRangeLabel.text = str
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        flushAllCollectionView()
    }

    // 清空已选内容
    function clearSelecteds()
    {
        theView.selectAll(false)
        selectedPaths = []
        global.selectedPaths = selectedPaths
    }

    // 刷新所有项目视图内容
    function flushAllCollectionView() {
        theView.proxyModel.refresh(filterType)
        global.selectedPaths = theView.selectedUrls
        getNumLabelText()
        totalTimepScopeTimer.start()
    }

    // 筛选相册内容后，使用定时器延迟刷新时间范围标签内容
    Timer {
        id: totalTimepScopeTimer
        interval: 100
        repeat: false
        onTriggered: {
            theView.totalTimeScope()
        }
    }

    // 刷新总数标签
    function getNumLabelText() {
        //QML的翻译不支持%n的特性，只能拆成这种代码

        var photoCountText = ""
        var photoCount = albumControl.getAllCount(1)
        if(photoCount === 0) {
            photoCountText = ""
        } else if(photoCount === 1) {
            photoCountText = qsTr("1 photo")
        } else {
            photoCountText = qsTr("%1 photos").arg(photoCount)
        }

        var videoCountText = ""
        var videoCount = albumControl.getAllCount(2)
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

    // 刷新选择项数标签
    function getSelectedNum(paths) {
        var selectedNumText = global.getSelectedNumText(paths, numLabelText)
        if (visible)
            global.statusBarNumText = selectedNumText
        return selectedNumText
    }

    Connections {
        target: albumControl
        onSigRepeatUrls: {
            if (visible && collecttionView.currentViewIndex === 3) {
                theView.selectUrls(urls)
            }
        }
    }

    // 所有项目标题栏区域
    Rectangle {
        id: allCollectionTitleRect
        width: parent.width - global.verticalScrollBarWidth
        height: 60
        color: Qt.rgba(0,0,0,0)
        // 时间范围标签
        Label {
            id: dateRangeLabel
            anchors.top: parent.top
            anchors.topMargin: 16
            anchors.left: parent.left
            height: 30
            font: DTK.fontManager.t3
            visible: numLabelText !== ""
        }

        // 筛选下拉框
        FilterComboBox {
            id: filterCombo
            anchors.top: dateRangeLabel.top
            anchors.topMargin: 4
            anchors.right: parent.right
            width: 130
            height: 30
            visible: parent.visible && albumControl.getAllCount() !== 0
            //visible: !(numLabelText === "" && filterType === 0)
        }

        MouseArea {
            anchors.fill: parent
            onPressed: {
                theView.selectAll(false)
                mouse.accepted = false
            }
        }
    }

    ThumbnailListView2 {
        id: theView
        anchors.top: allCollectionTitleRect.bottom
        anchors.topMargin: m_topMargin
        width: parent.width
        height: parent.height - allCollectionTitleRect.height - m_topMargin
        thumnailListType: GlobalVar.ThumbnailType.AllCollection
        proxyModel.sourceModel: Album.ImageDataModel { modelType: Album.Types.AllCollection }

        visible: numLabelText !== ""
        property int m_topMargin: 10

        // 监听缩略图列表选中状态，一旦改变，更新globalVar所有选中路径
        Connections {
            target: theView
            onSelectedChanged: {
                selectedPaths = []
                selectedPaths = theView.selectedUrls

                if (parent.visible)
                    global.selectedPaths = selectedPaths
            }
        }
    }

    // 仅在自动导入相册无内容时，显示没有图片或视频时显示
    Label {
        anchors.top: allCollectionTitleRect.bottom
        anchors.left: parent.left
        anchors.bottom: theView.bottom
        anchors.right: parent.right
        anchors.centerIn: parent
        visible: numLabelText === "" && albumControl.getAllCount() !== 0
        font: DTK.fontManager.t4
        color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        text: qsTr("No results")
    }

    onVisibleChanged: {
        // 窗口显示时，重置显示内容
        if (visible) {
            flushAllCollectionView()
        }
    }

    Component.onCompleted: {
        theView.timeChanged.connect(setDateRange)
        global.sigFlushAllCollectionView.connect(flushAllCollectionView)
    }
}
