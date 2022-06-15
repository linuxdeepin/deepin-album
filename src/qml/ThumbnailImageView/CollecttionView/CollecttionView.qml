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
    }
}
