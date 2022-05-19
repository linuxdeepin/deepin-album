import QtQuick 2.0
import org.deepin.dtk 1.0
import "../../Control"
import "../../Control/ListView"
Rectangle {
    anchors.fill: parent

    ThumbnailListView {
        id: theView
        anchors.top: parent.top
        anchors.topMargin: 50
        width: parent.width
        height: parent.height - 50
    }
}
