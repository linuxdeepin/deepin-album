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

    // 通知日视图刷新状态栏提示信息
    signal flushDayViewStatusText()

    function setIndex(index) {

        if (currentViewIndex === index)
            return

        currentViewIndex = index
        if (currentViewIndex === 0)
            yearCollection.flushModel()
        else if (currentViewIndex === 1)
            monthCollection.flushModel()
        else if (currentViewIndex === 2) {
            dayCollection.flushModel()
            flushDayViewStatusText()
        }
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

    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: currentViewIndex
        interactive: false

        YearCollection {
            id: yearCollection
        }

        MonthCollection {
            id: monthCollection
        }

        DayCollection {
            id: dayCollection
        }

        AllCollection {
            id: allCollection
        }

        onCurrentIndexChanged: {
            currentViewIndex = currentIndex
            GStatus.sigCollectionViewIndexChanged(currentIndex)

            // 保证日聚合和所有照片视图互斥显示，以便列表控件全选逻辑只在显示的视图中生效
            dayCollection.visible = currentViewIndex === 2
            allCollection.clearSelecteds()
            allCollection.visible = currentViewIndex === 3

            //年月视图不显示底栏数量
            if (currentViewIndex === 0 || currentViewIndex ===1) {
                GStatus.statusBarNumText = ""
            }
        }
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
