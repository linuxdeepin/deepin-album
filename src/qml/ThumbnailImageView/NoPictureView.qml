import QtQuick 2.0
import org.deepin.dtk 1.0


Rectangle{
    color: Qt.rgba(0,0,0,0)
    Rectangle{
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        ActionButton {
            id: noImageIcon
            anchors.top: parent.top
            anchors.topMargin: -70
            anchors.left : parent.left
            anchors.leftMargin: -width/2

            icon {
                name:"nopicture1"
                width: 140
                height: 140
            }
        }
        Label{
            anchors.top:noImageIcon.bottom
            anchors.topMargin: 20
            anchors.left : parent.left
            anchors.leftMargin: -width/2
            text:qsTr("No photos or videos found")
        }
    }
}
