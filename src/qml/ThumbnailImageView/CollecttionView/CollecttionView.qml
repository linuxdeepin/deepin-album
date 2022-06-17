import QtQuick 2.11
import QtQuick.Controls 2.4

Item {
    id: root
    property int currentViewIndex: 0

    function setIndex(index) {
        currentViewIndex = index
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
        yearCollection.yearClicked.connect(onYearClicked)
        monthCollection.monthClicked.connect(onMonthClicked)
    }
}
