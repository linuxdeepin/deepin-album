// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQml.Models
import QtQuick.Window
import QtQuick.Controls
import org.deepin.dtk 1.0 as D
import org.deepin.album 1.0 as Album

import "./CollecttionView"
import "./HaveImportedView"
import "./RecentlyDeletedView"
import "./CustomAlbum"
import "./DeviceAlbum"
import "./"
import "../ClassificationView"

import "./../Control"
import "./../"
//本文件用于替代stackwidget的作用，通过改变global的0-n来切换窗口

Item {
    id: thumbView

    // Window must map quickly: slow map triggers dock launcher timeout, causing icon flicker.
    // Only default CollecttionView is created sync during engine.load; other 8 views all embed
    // QWidgets (can't use async Loader), so they're deferred by viewDelayTimer until after map.
    // interval is minimal (next event loop), just yields current frame to let window map first.
    property bool delayedViewsReady: false
    Timer {
        id: viewDelayTimer
        interval: 1
        running: true
        repeat: false
        onTriggered: thumbView.delayedViewsReady = true
    }

    // Default Collection view created sync (active: true) so window map shows real thumbnails
    // (AllCollection) immediately, eliminating "shell->content" pop-in. Its heavy DayCollection child
    // is already split into Loader in CollecttionView.qml, keeping this light. Other 8 views still
    // deferred by viewDelayTimer. If user clicks year/month/day before this view loads, setIndex
    // would be lost (item is null), so use pendingCollectionIndex to stash and replay onLoaded.
    property int pendingCollectionIndex: -1
    Loader {
        id: collecttionViewLoader
        anchors.fill: parent
        asynchronous: false
        active: true
        sourceComponent: CollecttionView {
            id: collecttionView
            show: GStatus.currentViewType === Album.Types.ViewCollecttion
        }
        onLoaded: {
            if (thumbView.pendingCollectionIndex >= 0) {
                item.setIndex(thumbView.pendingCollectionIndex)
                thumbView.pendingCollectionIndex = -1
            }
        }
    }
    // Other views deferred until after map (or when selected), stay loaded once created.
    // Delay logic unified in DeferredView, not repeated per view.
    DeferredView {
        ready: delayedViewsReady
        viewType: Album.Types.ViewHaveImported
        sourceComponent: HaveImportedView { show: GStatus.currentViewType === Album.Types.ViewHaveImported }
    }
    DeferredView {
        ready: delayedViewsReady
        viewType: Album.Types.ViewFavorite
        sourceComponent: CustomAlbum { show: GStatus.currentViewType === Album.Types.ViewFavorite }
    }
    DeferredView {
        ready: delayedViewsReady
        viewType: Album.Types.ViewRecentlyDeleted
        sourceComponent: RecentlyDeletedView { show: GStatus.currentViewType === Album.Types.ViewRecentlyDeleted }
    }
    DeferredView {
        ready: delayedViewsReady
        viewType: Album.Types.ViewCustomAlbum
        sourceComponent: CustomAlbum { show: GStatus.currentViewType === Album.Types.ViewCustomAlbum }
    }
    DeferredView {
        ready: delayedViewsReady
        viewType: Album.Types.ViewSearchResult
        sourceComponent: SearchView { show: GStatus.currentViewType === Album.Types.ViewSearchResult }
    }
    DeferredView {
        ready: delayedViewsReady
        viewType: Album.Types.ViewDevice
        sourceComponent: DeviceAlbum { show: GStatus.currentViewType === Album.Types.ViewDevice }
    }
    DeferredView {
        id: classificationViewLoader
        ready: delayedViewsReady
        viewType: Album.Types.ViewClassification
        sourceComponent: ClassificationView {
            id: classificationView
            show: GStatus.currentViewType === Album.Types.ViewClassification
            onShowClassificationDetail: function(name, className) {
                classificationDetailViewLoader.active = true
                // Loader sync-loads (asynchronous:false); guard against load failure (item null)
                if (!classificationDetailViewLoader.item)
                    return
                classificationDetailViewLoader.item.setClassificationData(name, className)
                GStatus.currentViewType = Album.Types.ViewClassificationDetail
            }
        }
    }
    DeferredView {
        id: classificationDetailViewLoader
        ready: delayedViewsReady
        viewType: Album.Types.ViewClassificationDetail
        sourceComponent: ClassificationDetailView {
            id: classificationDetailView
            show: GStatus.currentViewType === Album.Types.ViewClassificationDetail
        }
    }

    Loader {
        id: emptyWarningDig
        active: false
        sourceComponent: EmptyWarningDialog {
        }

        function show() {
            active = true
            if (item)
                item.show()
        }
    }

    Connections {
        target: titleAlubmRect
        function onCollectionBtnClicked(index) {
            if (GStatus.currentCollecttionViewIndex === index)
                return
            // 点击按钮，动画切换类型设定为翻页滚动
            GStatus.currentSwitchType = Album.Types.FlipScroll
            if (collecttionViewLoader.item) {
                collecttionViewLoader.item.setIndex(index)
            } else {
                // View not created yet: stash index, replay onLoaded once ready
                thumbView.pendingCollectionIndex = index
            }
        }
    }

    Connections {
        target: GStatus
        function onSigZoomInOutFromQWidget(delta) {
            var curValue = statusBar.sliderValue
            if (delta > 0)
                statusBar.setSliderWidgetValue(curValue + 1)
            else
                statusBar.setSliderWidgetValue(curValue - 1)
        }
    }

    Shortcut {
        enabled: GStatus.stackControlCurrent === 0
        autoRepeat: false
        sequence: "Esc"
        onActivated: {
            GStatus.sigSelectAll(false)
        }
    }

    Shortcut {
        enabled: true
        autoRepeat: false
        sequence : "Ctrl+A"
        onActivated : {
            GStatus.sigSelectAll(true)
        }
    }

    Shortcut {
        enabled: visible && menuItemStates.canCopy
        autoRepeat: false
        sequence : "Ctrl+C"
        onActivated : {
            FileControl.copyImage(GStatus.selectedPaths)
        }
    }

    Shortcut {
        enabled: visible && GStatus.currentViewType !== Album.Types.ViewDevice
        autoRepeat: false
        sequence : "Delete"
        onActivated : {
            if (menuItemStates.isInTrash) {
                deleteDialog.setDisplay(menuItemStates.isInTrash ? Album.Types.TrashSel : Album.Types.TrashNormal, GStatus.selectedPaths.length)
                deleteDialog.show()
            } else {
                if (menuItemStates.canDelete) {
                    deleteDialog.deleteDirectly()
                } else {
                    // 源文件已不存在，从相册移除裂图
                    menuItemStates.executeRemoveFromAlbum()
                }
            }
        }
    }

    Shortcut {
        enabled: visible && menuItemStates.canRotate && GStatus.currentViewType !== Album.Types.ViewDevice
        autoRepeat: false
        sequence : "Ctrl+R"
        onActivated : {
            FileControl.rotateFile(GStatus.selectedPaths, 90)
        }
    }

    Shortcut {
        enabled: visible && menuItemStates.canRotate && GStatus.currentViewType !== Album.Types.ViewDevice
        autoRepeat: false
        sequence : "Ctrl+Shift+R"
        onActivated : {
            FileControl.rotateFile(GStatus.selectedPaths, -90)
        }
    }

    Shortcut {
        enabled: visible
        autoRepeat: true
        sequence : "Page Up"
        onActivated : {
            GStatus.sigPageUp()
        }
    }

    Shortcut {
        enabled: visible
        autoRepeat: true
        sequence : "Page Down"
        onActivated : {
            GStatus.sigPageDown()
        }
    }

    Shortcut {
        enabled: visible
        autoRepeat: false
        sequence : "Ctrl+="
        onActivated : {
            var curValue = statusBar.sliderValue
            statusBar.setSliderWidgetValue(curValue + 1)
        }
    }

    Shortcut {
        enabled: visible
        autoRepeat: false
        sequence : "Ctrl+-"
        onActivated : {
            var curValue = statusBar.sliderValue
            statusBar.setSliderWidgetValue(curValue - 1)
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
        enabled: visible
        autoRepeat: false
        sequence: "F1"
        onActivated: {
            D.ApplicationHelper.handleHelpAction()
        }
    }

    Shortcut {
        enabled: visible
        autoRepeat: false
        sequence: "Ctrl+Shift+/"
        onActivated: {
            albumControl.ctrlShiftSlashShortcut(window.x, window.y, window.width, window.height)
        }
    }
}
