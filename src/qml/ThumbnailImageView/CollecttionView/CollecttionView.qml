// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4

import org.deepin.album 1.0 as Album

import "../"
import "../../"
import "../../Control"

BaseView {
    id: collecttView

    property int currentViewIndex: 3
    property int rollingWidth: collecttView.width + 20

    // 通知日视图刷新状态栏提示信息
    signal flushDayViewStatusText()

    function setIndex(index) {
        if (currentViewIndex === index)
            return

        if (index === 0) {
            yearCollection.flushModel()
            if (yearCollection.x < 0)
                yearCollection.x = rollingWidth
        } else if (index === 1) {
            monthCollection.flushModel()
            if (monthCollection.x < 0)
                monthCollection.x = rollingWidth
        } else if (index === 2) {
            dayCollection.flushModel()
            if (dayCollection.x < 0)
                dayCollection.x = rollingWidth
            if (dayCollection.x !== 0) {
                dayCollection.visible = true
            }

            GStatus.selectedPaths = []
            flushDayViewStatusText()
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

    onWidthChanged: {
        if (yearCollection.x < 0)
            yearCollection.x = -rollingWidth
        if (monthCollection.x < 0)
            monthCollection.x = -rollingWidth
        if (dayCollection.x < 0)
            dayCollection.x = -rollingWidth
        if (allCollection.x < 0)
            allCollection.x = -rollingWidth
    }

    onVisibleChanged: {
        allCollection.clearSelecteds()

        if (visible) {
            GStatus.selectedPaths = []

            //年月视图不显示底栏数量
            if (currentViewIndex === 0 || currentViewIndex ===1) {
                GStatus.statusBarNumText = ""
            }
        }
    }

    YearCollection {
        id: yearCollection
        x: rollingWidth
        width: collecttView.width
        height: collecttView.height
        show: currentViewIndex === 0
    }

    MonthCollection {
        id: monthCollection
        x: rollingWidth
        width: collecttView.width
        height: collecttView.height
        show: currentViewIndex === 1
    }

    DayCollection {
        id: dayCollection
        visible: false
        x: rollingWidth
        width: collecttView.width
        height: collecttView.height
        show: currentViewIndex === 2
    }

    AllCollection {
        id: allCollection
        x: 0
        width: collecttView.width
        height: collecttView.height
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
        setIndex(1)
        monthCollection.scrollToYear(year)
    }

    function onMonthClicked(year, month) {
        setIndex(2)
        dayCollection.scrollToMonth(year, month)
    }

    Component.onCompleted: {
        // 保证日聚合和所有照片视图互斥显示，以便列表控件全选逻辑只在显示的视图中生效
        dayCollection.visible = currentViewIndex === 2
        allCollection.visible = currentViewIndex === 3

        yearCollection.yearClicked.connect(onYearClicked)
        monthCollection.monthClicked.connect(onMonthClicked)
    }
}
