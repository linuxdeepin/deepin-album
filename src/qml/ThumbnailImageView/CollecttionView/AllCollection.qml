// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import org.deepin.dtk 1.0

import org.deepin.album 1.0 as Album

import "../../Control"
import "../../Control/ListView"
import "../../Control/Animation"
import "../../"

SwitchViewAnimation {

    property int filterType: filterCombo.currentIndex // 筛选类型，默认所有
    property string numLabelText:""
    // Cached property: drop auto-binding so selectedPaths/numLabelText no longer each trigger a C++ count
    property string selectedText: ""
    property alias count: theView.count
    property int totalItemCount: 0
    // Coalesce flag: multiple schedules in the same frame run doUpdateSelectedText only once
    property bool updateSelectedTextPending: false

    function setDateRange(str) {
        dateRangeLabel.text = str
    }

    // Visibility gate: hidden views are not woken by selectedPathsChanged to do a type count
    function shouldUpdateSelectedText() {
        return visible && GStatus.currentCollecttionViewIndex === 3
    }

    // Schedule update: coalesce same-frame sources (selectedPathsChanged/flush/numLabelText change)
    function scheduleUpdateSelectedText() {
        if (!shouldUpdateSelectedText() || updateSelectedTextPending)
            return
        updateSelectedTextPending = true
        Qt.callLater(doUpdateSelectedText)
    }

    // Actual update: read latest selectedPaths + numLabelText, count only once
    function doUpdateSelectedText() {
        updateSelectedTextPending = false
        if (!shouldUpdateSelectedText())
            return
        var paths = GStatus.selectedPaths || []
        selectedText = paths.length === 0
                ? numLabelText
                : GStatus.getSelectedNumText(paths, numLabelText)
        GStatus.statusBarNumText = selectedText
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        filterRefreshTimer.restart()
    }

    // Filter changes can arrive in quick succession; coalesce them before rebuilding the proxy model.
    Timer {
        id: filterRefreshTimer
        interval: 100
        repeat: false
        onTriggered: flushAllCollectionView()
    }

    // 清空已选内容
    function clearSelecteds()
    {
        theView.selectAll(false)
        GStatus.selectedPaths = []
        scheduleUpdateSelectedText()
    }

    // 刷新所有项目视图内容
    function flushAllCollectionView() {
        theView.proxyModel.refresh(filterType)
        if (!visible)
            return
        GStatus.selectedPaths = theView.selectedUrls
        getNumLabelText()
        // numLabelText change does not auto-trigger an update, schedule explicitly
        scheduleUpdateSelectedText()
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
        totalItemCount = photoCount + videoCount
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

    // Unified selection entry: C++ setSelectedPaths also converges through this signal
    Connections {
        target: GStatus
        function onSelectedPathsChanged() {
            scheduleUpdateSelectedText()
        }
    }

    Connections {
        target: albumControl
        function onSigRepeatUrls(urls) {
            if (visible && collecttionView.currentViewIndex === 3) {
                theView.selectUrls(urls)
            }
        }
    }

    // 所有项目标题栏区域
    Item {
        id: allCollectionTitleRect
        width: parent.width - GStatus.verticalScrollBarWidth
        height: 60

        // 时间范围标签
        Label {
            id: dateRangeLabel
            anchors {
                top: parent.top
                topMargin: 16
                left: parent.left
            }
            height: 30
            font: DTK.fontManager.t3
            visible: numLabelText !== ""
        }

        // 筛选下拉框
        FilterComboBox {
            id: filterCombo
            anchors {
                top: dateRangeLabel.top
                topMargin: 4
                right: parent.right
            }
            width: 115
            height: 30
            visible: parent.visible && totalItemCount > 0
        }

        MouseArea {
            anchors.fill: parent
            onPressed: (mouse)=> {
                theView.selectAll(false)
                mouse.accepted = false
            }
        }
    }

    ThumbnailListViewAlbum {
        id: theView
        anchors {
            top: allCollectionTitleRect.bottom
            topMargin: m_topMargin
        }
        width: parent.width
        height: parent.height - allCollectionTitleRect.height - m_topMargin
        thumnailListType: Album.Types.ThumbnailAllCollection
        proxyModel.sourceModel: Album.ImageDataModel { modelType: Album.Types.AllCollection }

        visible: numLabelText !== ""
        property int m_topMargin: 10

    }

    // 仅在自动导入相册无内容时，显示没有图片或视频时显示
    Label {
        anchors {
            top: allCollectionTitleRect.bottom
            left: parent.left
            bottom: theView.bottom
            right: parent.right
            centerIn: parent
        }
        visible: numLabelText === "" && totalItemCount > 0
        font: DTK.fontManager.t4
        color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        text: qsTr("No results")
    }

    onVisibleChanged: {
        // 窗口显示时，重置显示内容
        if (visible && GStatus.currentCollecttionViewIndex === 3) {
            if (!GStatus.loading) {
                flushAllCollectionView()
                if (!GStatus.backingToMainAlbumView)
                    showAnimation.start()
            }
        }
    }

    NumberAnimation {
        id: showAnimation
        target: theView
        property: "anchors.topMargin"
        from: 10 + theView.height
        to: 10
        duration: GStatus.sidebarAnimationEnabled ? GStatus.animationDuration : 0
        easing.type: Easing.OutExpo
    }

    Component.onCompleted: {
        theView.timeChanged.connect(setDateRange)
        GStatus.sigFlushAllCollectionView.connect(flushAllCollectionView)
    }
}
