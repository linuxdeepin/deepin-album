// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

import org.deepin.album 1.0 as Album

import "../"
import "../../"
import "../../Control"

BaseView {
    id: collecttView

    property int currentViewIndex: 3
    property int rollingWidth: collecttView.width + 20

    // DayCollection contains heavy QWidget(TimeLineView) that slows window map.
    // Defer creation via Loader until after map or when switching to day view (index===2).
    // Once dayReady is true, loading is locked (active implies status===Loader.Ready).
    property bool dayReady: false
    // Stash day/month switch requests before Loader is ready, replay in onLoaded.
    property bool pendingDaySwitch: false
    property var pendingMonthArgs: undefined

    // 通知日视图刷新状态栏提示信息
    signal flushDayViewStatusText()

    function setIndex(index) {
        if (currentViewIndex === index)
            return

        if (index === 0) {
            if (yearCollection.count === 0)
                yearCollection.flushModel()
            if (yearCollection.x < 0)
                yearCollection.x = rollingWidth
        } else if (index === 1) {
            if (monthCollection.count === 0)
                monthCollection.flushModel()
            if (monthCollection.x < 0)
                monthCollection.x = rollingWidth
        } else if (index === 2) {
            var dayItem = dayCollectionLoader.item
            if (!dayItem) {
                // Day view not created yet: stash request, activate Loader, replay onLoaded
                pendingDaySwitch = true
                dayCollectionLoader.active = true
            } else {
                applyDayViewState(dayItem)
            }
        } else if (index === 3) {
            if (allCollection.x < 0)
                allCollection.x = rollingWidth
            allCollection.clearSelecteds()
        }

        currentViewIndex = index
        GStatus.currentCollecttionViewIndex = index

        //年月视图不显示底栏数量
        if (currentViewIndex === 0 || currentViewIndex ===1) {
            GStatus.statusBarNumText = ""
        }
    }

    // Apply day view state when switching to it (requires dayItem already created).
    function applyDayViewState(dayItem) {
        if (dayItem.count === 0)
            dayItem.flushView()
        if (dayItem.x < 0)
            dayItem.x = rollingWidth
        if (!dayItem.visible) {
            dayItem.visible = true
        }
        dayItem.unSelectAll()
        flushDayViewStatusText()
    }

    onWidthChanged: {
        if (yearCollection.x < 0)
            yearCollection.x = -rollingWidth
        if (monthCollection.x < 0)
            monthCollection.x = -rollingWidth
        if (dayCollectionLoader.item && dayCollectionLoader.item.x < 0)
            dayCollectionLoader.item.x = -rollingWidth
        if (allCollection.x < 0)
            allCollection.x = -rollingWidth
    }

    onVisibleChanged: {
        allCollection.clearSelecteds()
        if (dayCollectionLoader.item)
            dayCollectionLoader.item.unSelectAll()

        if (visible) {
            GStatus.selectedPaths = []

            //年月视图不显示底栏数量
            if (currentViewIndex === 0 || currentViewIndex === 1) {
                GStatus.statusBarNumText = ""
            }
        }
    }

    YearCollection {
        id: yearCollection
        width: collecttView.width
        height: collecttView.height
        viewType: 0
        show: currentViewIndex === 0
    }

    MonthCollection {
        id: monthCollection
        width: collecttView.width
        height: collecttView.height
        viewType: 1
        show: currentViewIndex === 1
    }

    // DayCollection deferred/lazy-loaded (heavy QWidget, not involved in startup map).
    Loader {
        id: dayCollectionLoader
        x: 0
        width: collecttView.width
        height: collecttView.height
        asynchronous: false
        active: dayReady || currentViewIndex === 2
        sourceComponent: DayCollection {
            id: dayCollection
            visible: false
            width: collecttView.width
            height: collecttView.height
            viewType: 2
            show: collecttView.currentViewIndex === 2
        }
        onLoaded: {
            // Exclusive visibility: only show when current view is actually day view
            item.visible = collecttView.currentViewIndex === 2
            if (collecttView.pendingDaySwitch) {
                collecttView.pendingDaySwitch = false
                collecttView.applyDayViewState(item)
            }
            if (collecttView.pendingMonthArgs !== undefined) {
                var args = collecttView.pendingMonthArgs
                collecttView.pendingMonthArgs = undefined
                item.scrollToMonth(args.year, args.month)
            }
        }
    }

    // Create DayCollection after map so its embedded QWidget doesn't slow startup.
    Timer {
        id: dayDelayTimer
        interval: 1
        running: true
        repeat: false
        onTriggered: collecttView.dayReady = true
    }

    AllCollection {
        id: allCollection
        x: 0
        width: collecttView.width
        height: collecttView.height
        viewType: 3
        show: currentViewIndex === 3
    }

    // 若没有数据，显示导入图片视图
    NoPictureView {
        visible: GStatus.currentViewType === Album.Types.ViewCollecttion && allCollection.numLabelText === "" && albumControl.getAllCount() === 0
        bShowImportBtn: true
        iconName: "nopicture1"
        onVisibleChanged: {
            if (visible) {
                GStatus.statusBarNumText = ""
            }
        }
    }

    function onYearClicked(year) {
        // 点击在图片上，动画切换类型为渐显渐隐
        GStatus.currentSwitchType = Album.Types.ScaleInOUt
        setIndex(1)
        allCollection.x = -rollingWidth
        if (dayCollectionLoader.item)
            dayCollectionLoader.item.x = -rollingWidth
        monthCollection.scrollToYear(year)
    }

    function onMonthClicked(year, month) {
        // 点击在图片上，动画切换类型为渐显渐隐
        GStatus.currentSwitchType = Album.Types.ScaleInOUt
        setIndex(2)
        allCollection.x = -rollingWidth
        var dayItem = dayCollectionLoader.item
        if (dayItem) {
            dayItem.scrollToMonth(year, month)
        } else {
            // Day view not created yet: stash jump params; setIndex(2) already activated Loader, replay onLoaded
            pendingMonthArgs = { "year": year, "month": month }
        }
    }

    Component.onCompleted: {
        // dayCollection deferred; its exclusive visibility is set in Loader.onLoaded
        allCollection.visible = currentViewIndex === 3

        yearCollection.yearClicked.connect(onYearClicked)
        monthCollection.monthClicked.connect(onMonthClicked)
    }
}
