import QtQuick 2.11
import QtQuick.Controls 2.4

Item {
    id: root
    property int currentViewIndex: 0

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
            flushTimeScopetimer.start(10)
            global.selectedPaths = []
        }
    }

    Timer {
        id: flushTimeScopetimer
        running: false
        repeat: false
        onTriggered: {
            allCollection.flushTotalTimeScope()
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

            allCollection.flushTotalTimeScope()
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
