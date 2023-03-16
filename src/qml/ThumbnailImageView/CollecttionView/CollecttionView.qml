// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4
import "../"
import "../../"

Rectangle {
    id: root
    property int currentViewIndex: 3

    // 通知日视图刷新状态栏提示信息
    signal flushDayViewStatusText()

    function setIndex(index) {

        currentViewIndex = index

        if (currentViewIndex === 2)
            flushDayViewStatusText()
    }

    onVisibleChanged: {
        allCollection.clearSelecteds()

        if (visible) {
            global.selectedPaths = []

            //年月视图不显示底栏数量
            if (currentViewIndex === 0 || currentViewIndex ===1) {
                global.statusBarNumText = ""
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
            global.sigCollectionViewIndexChanged(currentIndex)

            // 保证日聚合和所有照片视图互斥显示，以便列表控件全选逻辑只在显示的视图中生效
            dayCollection.visible = currentViewIndex === 2
            allCollection.clearSelecteds()
            allCollection.visible = currentViewIndex === 3

            //年月视图不显示底栏数量
            if (currentViewIndex === 0 || currentViewIndex ===1) {
                global.statusBarNumText = ""
            }
        }
    }

    // 若没有数据，显示导入图片视图
    ImportView {
        anchors.fill: parent
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.Collecttion && allCollection.numLabelText === "" && albumControl.getAllCount() === 0

        onVisibleChanged: {
            if (visible) {
                global.statusBarNumText = ""
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
