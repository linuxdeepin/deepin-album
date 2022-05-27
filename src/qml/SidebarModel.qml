import QtQuick 2.0

ListModel {
    ListElement {
        name: qsTr("Collection")
        number: "0"
        iconName :"images"

    }
    ListElement {
        name: qsTr("Import")
        number: "1"
        iconName :"import_left"
    }
    ListElement {
        name: qsTr("Favorites")
        number: "2"
        iconName :"collection"
    }
    ListElement {
        name: qsTr("Trash")
        number: "3"
        iconName :"trash"
    }
}
