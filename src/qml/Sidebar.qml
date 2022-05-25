import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQml 2.11
import org.deepin.dtk 1.0
import "./Control"
Rectangle {
    width:200
    height:parent.height

    color: "#EEEEEE"

    Label {
        id:pictureLabel
        anchors.top: parent.top
        anchors.topMargin: 69
        anchors.left: parent.left
        anchors.leftMargin: 20
        height: 30
        text: "照片库"

        color: Qt.rgba(0,0,0,0.3)
        lineHeight: 20

    }
    ListView{
        id : pictureLeftlist
        anchors.top: pictureLabel.bottom
        anchors.topMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 10
        width: parent.width-20
        implicitHeight: contentHeight
        model: SidebarModel {}
        delegate:ItemDelegate {
            text: name
            width: parent.width
            icon.name: iconName
            checked: index == 0
            backgroundVisible: false
            onClicked: {
                global.currentViewIndex = index + 2
            }
        }
    }

    Label {
        id:albumLabel
        anchors.top: pictureLeftlist.bottom
        anchors.topMargin: 15
        anchors.left: parent.left
        anchors.leftMargin: 20

        height: 30
        text: "相册"

        color: Qt.rgba(0,0,0,0.3)

    }

    FloatingButton {
        id:addAlbumButton
        checked: false
        anchors.top: pictureLeftlist.bottom
        anchors.topMargin: 15
        anchors.right: parent.right
        anchors.rightMargin: 20
        width: 20
        height: 20
        icon{
            name: "add-xiangce"
            //            name :"import_left"
            width: 10
            height: 10
        }


        onClicked: {
            var x = parent.mapToGlobal(0, 0).x + parent.width / 2 - 190
            var y = parent.mapToGlobal(0, 0).y + parent.height / 2 - 89
            newAlbum.setX(x)
            newAlbum.setY(y)

            newAlbum.show()
        }
    }
    //rename窗口
    NewAlbumDialog {
        id: newAlbum
    }


}
