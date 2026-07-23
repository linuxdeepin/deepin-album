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

    // The default all-items page is constructed with show=true.  Do not let
    // UnknownSwitchType fade it in before the parent collection view appears.
    property bool initialStateApplied: false
    switchType: initialStateApplied ? GStatus.currentSwitchType : Album.Types.HardCut

    property int filterType: filterCombo.currentIndex // 筛选类型，默认所有
    property string numLabelText:""
    // Cached property: drop auto-binding so selectedPaths/numLabelText no longer each trigger a C++ count
    property string selectedText: ""
    property int count: theViewLoader.item ? theViewLoader.item.count : 0
    property int totalItemCount: 0
    property bool initialContentRefreshPending: true
    property bool initialContentActivationPending: false
    property bool pendingModelRefresh: false
    property bool collectionModelLoaded: false
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
        // The initial currentIndex binding may change before the deferred
        // thumbnail view is created. initialTitleTimer will consume the final
        // value, so do not schedule a second full model refresh at startup.
        if (initialContentRefreshPending) {
            scheduleInitialTitle()
            return
        }
        if (!updateTitle(false, false))
            scheduleInitialTitle()
        filterRefreshTimer.restart()
    }

    onWidthChanged: scheduleInitialTitle()
    onHeightChanged: scheduleInitialTitle()

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
        if (theViewLoader.item)
            theViewLoader.item.selectAll(false)
        GStatus.selectedPaths = []
        scheduleUpdateSelectedText()
    }

    // 刷新所有项目视图内容
    function flushAllCollectionView(refreshTitleModel) {
        if (refreshTitleModel === undefined)
            refreshTitleModel = true
        if (initialContentRefreshPending || !theViewLoader.item) {
            if (refreshTitleModel)
                pendingModelRefresh = true
            return
        }

        if (refreshTitleModel || pendingModelRefresh) {
            theViewLoader.item.proxyModel.refresh(filterType)
            pendingModelRefresh = false
        }
        if (!visible)
            return
        GStatus.selectedPaths = theViewLoader.item.selectedUrls
        updateTitle(false, false)
        // numLabelText change does not auto-trigger an update, schedule explicitly
        scheduleUpdateSelectedText()
        totalTimepScopeTimer.start()
    }

    // Wait for QML to settle the parent geometry before calculating the first
    // visible grid range. This still runs before the first frame is presented.
    Timer {
        id: initialTitleTimer
        interval: 0
        repeat: false
        onTriggered: updateTitle()
    }

    function scheduleInitialTitle() {
        if (initialContentRefreshPending && visible)
            initialTitleTimer.restart()
    }

    // The title and grid share allCollectionDataModel, so constructing the
    // initial grid does not trigger a second all-items database query.
    function startInitialContent() {
        if (GStatus.currentViewType === Album.Types.ViewCollecttion
                && GStatus.currentCollecttionViewIndex === 3
                && !theViewLoader.active) {
            initialContentActivationPending = true
        }
    }

    function activateInitialContent() {
        if (!initialContentActivationPending)
            return

        initialContentActivationPending = false
        if (GStatus.currentViewType === Album.Types.ViewCollecttion
                && GStatus.currentCollecttionViewIndex === 3) {
            theViewLoader.active = true
        }
    }

    // 筛选相册内容后，使用定时器延迟刷新时间范围标签内容
    Timer {
        id: totalTimepScopeTimer
        interval: 100
        repeat: false
        onTriggered: {
            if (theViewLoader.item)
                theViewLoader.item.totalTimeScope()
        }
    }

    // Match ThumbnailListViewAlbum's GridView.rectIndexes(0, 0, width, height)
    // without constructing the thumbnail view during the first frame.
    function initialTimeScopeEndIndex() {
        var availableWidth = theViewLoader.width - GStatus.thumbnailListRightMargin
        var rowSizeHint = parseInt(availableWidth / GStatus.cellBaseWidth)
        if (!(rowSizeHint > 0))
            return -1

        var cellWidth = availableWidth / rowSizeHint
        var columns = Math.floor(theViewLoader.width / cellWidth)
        var rows = Math.ceil((theViewLoader.height - theViewLoader.contentTopMargin) / cellWidth)
        if (!(columns > 0) || !(rows > 0))
            return -1

        return columns * rows - 1
    }

    function updateTitle(refreshTitleModel, updateDateRange) {
        if (refreshTitleModel === undefined)
            refreshTitleModel = true
        if (updateDateRange === undefined)
            updateDateRange = true
        if (!(theViewLoader.width > 0)
                || !(theViewLoader.height > theViewLoader.contentTopMargin))
            return false

        getNumLabelText()
        if (refreshTitleModel) {
            allCollectionDataModel.loadData(filterType)
            collectionModelLoaded = true
        }
        if (!collectionModelLoaded)
            return false

        var itemCount = allCollectionDataModel.itemCount()
        if (itemCount === 0) {
            setDateRange("")
            return true
        }
        if (!updateDateRange)
            return true

        var initialEndIndex = initialTimeScopeEndIndex()
        if (initialEndIndex < 0)
            return false

        var lastIndex = Math.min(initialEndIndex, itemCount - 1)
        var firstUrl = allCollectionDataModel.urlAt(0)
        var lastUrl = allCollectionDataModel.urlAt(lastIndex)
        setDateRange(albumControl.getFileTime(firstUrl, lastUrl))
        return true
    }

    // Load title data before the first frame; the deferred grid uses this
    // source directly and does not perform a second all-items query.
    Album.ImageDataModel {
        id: allCollectionDataModel
        modelType: Album.Types.AllCollection
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
        if (show && GStatus.currentViewType === Album.Types.ViewCollecttion
                && GStatus.currentCollecttionViewIndex === 3) {
            GStatus.statusBarNumText = numLabelText
            GStatus.allCollectionContentResolved = true
        }
    }

    // Unified selection entry: C++ setSelectedPaths also converges through this signal
    Connections {
        target: GStatus
        function onSelectedPathsChanged() {
            scheduleUpdateSelectedText()
        }
        function onCurrentViewTypeChanged() {
            startInitialContent()
        }
        function onCurrentCollecttionViewIndexChanged() {
            startInitialContent()
        }
        function onCellBaseWidthChanged() {
            scheduleInitialTitle()
        }
    }

    Connections {
        target: window
        function onFrameSwapped() {
            activateInitialContent()
        }
    }

    Connections {
        target: albumControl
        function onSigRepeatUrls(urls) {
            if (visible && collecttionView.currentViewIndex === 3) {
                if (theViewLoader.item)
                    theViewLoader.item.selectUrls(urls)
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
                if (theViewLoader.item)
                    theViewLoader.item.selectAll(false)
                mouse.accepted = false
            }
        }
    }

    Loader {
        id: theViewLoader
        anchors.top: allCollectionTitleRect.bottom
        width: parent.width
        height: parent.height - allCollectionTitleRect.height
        property int contentTopMargin: 10
        active: false
        asynchronous: false
        sourceComponent: ThumbnailListViewAlbum {
            anchors {
                top: parent.top
                topMargin: theViewLoader.contentTopMargin
            }
            width: parent.width
            height: parent.height - theViewLoader.contentTopMargin
            thumnailListType: Album.Types.ThumbnailAllCollection
            proxyModel.sourceModel: allCollectionDataModel
            visible: numLabelText !== ""
        }
        onLoaded: {
            item.timeChanged.connect(setDateRange)
            initialContentRefreshPending = false
            flushAllCollectionView(pendingModelRefresh)
            if (!GStatus.backingToMainAlbumView)
                showAnimation.start()
        }
    }

    // 同步延迟创建的缩略图列表选中项，供右键菜单更新状态。
    Connections {
        target: theViewLoader.item
        function onSelectedChanged() {
            if (parent.visible && theViewLoader.item)
                GStatus.selectedPaths = theViewLoader.item.selectedUrls
        }
    }

    // 仅在自动导入相册无内容时，显示没有图片或视频时显示
    Label {
        anchors {
            top: allCollectionTitleRect.bottom
            left: parent.left
            bottom: theViewLoader.bottom
            right: parent.right
            centerIn: parent
        }
        visible: numLabelText === "" && totalItemCount > 0
        font: DTK.fontManager.t4
        color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        text: qsTr("No results")
    }

    onVisibleChanged: {
        scheduleInitialTitle()
        // 窗口显示时，重置显示内容
        if (visible && GStatus.currentCollecttionViewIndex === 3) {
            startInitialContent()
            if (!GStatus.loading) {
                flushAllCollectionView()
            }
        }
    }

    NumberAnimation {
        id: showAnimation
        target: theViewLoader.item
        property: "anchors.topMargin"
        from: theViewLoader.contentTopMargin + theViewLoader.height
        to: theViewLoader.contentTopMargin
        duration: GStatus.sidebarAnimationEnabled ? GStatus.animationDuration : 0
        easing.type: Easing.OutExpo
    }

    Component.onCompleted: {
        GStatus.sigFlushAllCollectionView.connect(flushAllCollectionView)

        // Populate the shared source model before creating the initial grid.
        GStatus.selectedPaths = []
        if (updateTitle())
            initialTitleTimer.stop()
        else
            scheduleInitialTitle()
        initialStateApplied = true
        startInitialContent()
    }
}
