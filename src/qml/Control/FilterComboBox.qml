import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQml 2.11
import org.deepin.dtk 1.0


ComboBox {
    textRole: "text"
    iconNameRole: "icon"
    flat: true

    model: ListModel {
        ListElement { text: qsTr("All"); icon: "selectall" }
        ListElement { text: qsTr("Photos"); icon: "images" }
        ListElement { text: qsTr("Videos"); icon: "videos" }
    }
}

