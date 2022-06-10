import QtQuick 2.0

Item {
    id: root
    property int currentViewIndex: 0

    function setIndex(index) {
        currentViewIndex = index
    }

    YearCollection {
        id: yearCollection
        visible: currentViewIndex == 0
        anchors.fill: parent
    }

    MonthCollection {
        id: monthCollection
        visible: currentViewIndex == 1
        anchors.fill: parent
    }

    DayCollection {
        id: dayCollection
        visible: currentViewIndex == 2
        anchors.fill: parent
    }

    AllCollection {
        id: allCollection
        visible: currentViewIndex == 3
        anchors.fill: parent
    }
}
