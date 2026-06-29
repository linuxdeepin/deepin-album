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
    id: dayView

    signal sigListViewPressed(int x, int y)
    signal sigListViewReleased(int x, int y)
    property int scrollDelta: 60
    property int timeLineLblHeight: 36
    property int timeLineLblMargin: 10
    property int selAllCheckBoxHeight: 22
    // property int rowSizeHint: (width - GStatus.thumbnailListRightMargin) / GStatus.cellBaseWidth
    // property real realCellWidth : (width - GStatus.thumbnailListRightMargin) / rowSizeHint
    // property var dayHeights: []

    property int filterType: timeline.filterType
    property string numLabelText: "" //总数标签显示内容
    // Cached property: drop auto-binding so selectedPaths/numLabelText no longer each trigger a C++ count
    property string selectedText: ""
    // Coalesce flag: multiple schedules in the same frame run doUpdateSelectedText only once
    property bool updateSelectedTextPending: false
    property bool bShowImportTips: GStatus.currentViewType === Album.Types.ViewCollecttion
                                   && GStatus.currentCollecttionViewIndex === 2
                                   && numLabelText === ""
                                   && albumControl.getAllCount() === 0

    // property alias count: theModel.count

    Rectangle {
        id: dayContainer
        anchors.fill : parent
        color: DTK.themeType === ApplicationHelper.LightType ? "#f8f8f8"
                                                              : "#202020"
        visible: !bShowImportTips
        Album.QmlWidget{
            id: timeline
            anchors.fill: parent
            anchors.rightMargin: GStatus.verticalScrollBarWidth
            focus: false
            viewType: Album.Types.WidgetDayView
            hideBuiltinFilter: true
        }

        // QML FilterComboBox overlay, replaces the builtin C++ filter widget
        FilterComboBox {
            id: filterCombo
            anchors {
                top: timeline.top
                topMargin: 36
                right: timeline.right
                rightMargin: 6
            }
            width: 115
            height: 30
            currentIndex: timeline.filterType
            onActivated: timeline.filterType = currentIndex
        }

        WidgetScrollBar {
            contentRatio: timeline.contentRatio
            scrollPosition: timeline.scrollPosition
            onScrollPositionChangedFromDrag: (pos) => timeline.setScrollPosition(pos)
        }
    }

    Connections {
        target: GStatus
        function onSigDoubleClickedFromQWidget(url) {
            if (!dayView.visible)
                return
            if (url !== undefined) {
                var allUrls = timeline.allUrls()
                menuItemStates.executeViewImageCutSwitch(url, allUrls)
            }
        }

        function onSigMenuItemClickedFromQWidget(id, uid) {
            if (!dayView.visible)
                return
            var sels = GStatus.selectedPaths
            var url = ""
            if (sels.length > 0)
                url = sels[0]
            var allUrls = timeline.allUrls()
            if (id === Album.Types.IdView) {
                menuItemStates.executeViewImageCutSwitch(url, allUrls)
            } else if (id === Album.Types.IdMoveToTrash) {
                deleteDialog.sigDoDeleteImg.connect(runDeleteImg)
                deleteDialog.setDisplay(Album.Types.TrashNormal, GStatus.selectedPaths.length)
                deleteDialog.show()
            } else if (id === Album.Types.IdFullScreen) {
                menuItemStates.executeFullScreen(url, allUrls)
            } else if (id === Album.Types.IdPrint) {
                menuItemStates.executePrint()
            } else if (id === Album.Types.IdStartSlideShow) {
                menuItemStates.excuteSlideShow(allUrls)
            } else if (id === Album.Types.IdExport) {
                menuItemStates.excuteExport()
            } else if (id === Album.Types.IdCopyToClipboard) {
                menuItemStates.executeCopy()
            } else if (id === Album.Types.IdAddToFavorites) {
                menuItemStates.executeFavorite()
            } else if (id === Album.Types.IdRemoveFromFavorites) {
                menuItemStates.executeUnFavorite()
            } else if (id === Album.Types.IdRotateClockwise) {
                menuItemStates.executeRotate(90)
            } else if (id === Album.Types.IdRotateCounterclockwise) {
                menuItemStates.executeRotate(-90)
            } else if (id === Album.Types.IdSetAsWallpaper) {
                menuItemStates.executeSetWallpaper()
            } else if (id === Album.Types.IdDisplayInFileManager) {
                menuItemStates.executeDisplayInFileManager()
            } else if (id === Album.Types.IdImageInfo) {
                menuItemStates.executeViewPhotoInfo()
            } else if (id === Album.Types.IdVideoInfo) {
                menuItemStates.executeViewVideoInfo()
            } else if (id === Album.Types.IdNewAlbum) {
                newAlbum.isChangeView = true
                newAlbum.importSelected = true
                newAlbum.setNormalEdit()
                newAlbum.show()
            } else if (id === Album.Types.IdAddToAlbum) {
                // 获取所选自定义相册的Id，根据Id添加到对应自定义相册
                albumControl.insertIntoAlbum(uid , GStatus.selectedPaths)
                DTK.sendMessage(thumbnailImage, qsTr("Successfully added to “%1”").arg(albumControl.getCustomAlbumByUid(uid)), "notify_checked")
            }
        }
    }

    // Visibility gate: hidden views are not woken by selectedPathsChanged to do a type count
    function shouldUpdateSelectedText() {
        return visible && GStatus.currentCollecttionViewIndex === 2
    }

    // Schedule update: coalesce same-frame sources (selectedPathsChanged/filter/flush/external refresh)
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

    onVisibleChanged: {
        // 窗口显示时，重置显示内容
        if (visible) {
            flushView()
        }
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        if (visible) {
            getNumLabelText()
            // numLabelText change does not auto-trigger an update, schedule explicitly
            scheduleUpdateSelectedText()
        }
    }

    function flushView() {
        if (!visible)
            return
        timeline.refresh()
        getNumLabelText()
        scheduleUpdateSelectedText()
    }

    function unSelectAll() {
        timeline.unSelectAll()
    }

    function runDeleteImg() {
        deleteDialog.sigDoDeleteImg.disconnect(runDeleteImg)
        menuItemStates.executeDelete()
        getNumLabelText()
    }

    Connections {
        target: collecttionView
        function onFlushDayViewStatusText() {
            if (visible) {
                if (GStatus.selectedPaths.length > 0)
                    getSelectedText(GStatus.selectedPaths)
                else {
                    getNumLabelText()
                    scheduleUpdateSelectedText()
                }
            }
        }
    }

    Connections {
        target: albumControl
        function onSigRepeatUrls(urls) {
            if (visible && collecttionView.currentViewIndex === 2) {
                timeline.selectUrls(urls)
            }
        }
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

    // Refresh selected-item label (wrapper: forwards to the coalesced entry; param kept for call compat)
    function getSelectedText(paths) {
        scheduleUpdateSelectedText()
        return selectedText
    }

    //月视图切日视图
    function scrollToMonth(year, month) {
        timeline.navigateToMonth(year+"/"+month)
    }

    Connections {
        target: GStatus

        // Unified selection entry: C++ setSelectedPaths also converges through this signal
        function onSelectedPathsChanged() {
            scheduleUpdateSelectedText()
        }

        function onSigSelectAll(sel) {
            if (visible) {
                if (sel)
                    GStatus.sigKeyPressFromQml("Ctrl+A")
                else
                    GStatus.sigKeyPressFromQml("Esc")
            }
        }

        function onSigPageUp() {
            if (visible) {
                GStatus.sigKeyPressFromQml("Page Up")
            }
        }

        function onSigPageDown() {
            if (visible) {
                GStatus.sigKeyPressFromQml("Page Down")
            }
        }
    }

    Component.onCompleted: {
        GStatus.sigFlushAllCollectionView.connect(flushView)
    }
}
