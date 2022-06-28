import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Window 2.10
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11

import org.deepin.dtk 1.0

DialogWindow {
    id: deleteDialog
    modality: Qt.WindowModal
    flags: Qt.Window | Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
    title: " "
    visible: false

    minimumWidth: 400
    maximumWidth: 400
    minimumHeight: 190
    maximumHeight: 190

    width: 400
    height: 190

    icon : "deepin-album"

    signal sigDoDeleteImg()

    function setDisplay(isTrash, count) {
        if(isTrash) {
            if(count === 1) {
                deleteTitle.text = qsTr("Are you sure you want to permanently delete this file?")
                deleteTips.text  = qsTr("You cannot restore it any longer")
            } else {
                deleteTitle.text = qsTr("Are you sure you want to permanently delete %1 files?").arg(count)
                deleteTips.text  = qsTr("You cannot restore them any longer")
            }
        } else {
            if(count === 1) {
                deleteTitle.text = qsTr("Are you sure you want to delete this file locally?")
                deleteTips.text  = qsTr("You can restore it in the trash")
            } else {
                deleteTitle.text = qsTr("Are you sure you want to delete %1 file locally?").arg(count)
                deleteTips.text  = qsTr("You can restore them in the trash")
            }
        }
    }

    Text {
        id: deleteTitle
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        font: DTK.fontManager.t6
        verticalAlignment: Text.AlignBottom
        horizontalAlignment: Text.AlignHCenter
    }

    Text {
        id: deleteTips
        opacity: 0.7
        anchors.top: deleteTitle.bottom
        anchors.horizontalCenter: deleteTitle.horizontalCenter
        font: DTK.fontManager.t6
        verticalAlignment: Text.AlignBottom
        horizontalAlignment: Text.AlignHCenter
    }

    Button {
        id: cancelbtn
        anchors.top: deleteTips.bottom
        anchors.topMargin: 30
        anchors.left: parent.left
        anchors.leftMargin: 0
        text: qsTr("Cancel")
        width: 185
        height: 36
        font.pixelSize: 16
        onClicked: {
            deleteDialog.visible = false
        }
    }

    WarningButton {
        id: enterbtn
        anchors.top: deleteTips.bottom
        anchors.topMargin: 30
        anchors.left: cancelbtn.right
        anchors.leftMargin: 10
        text: qsTr("Delete")
        enabled: nameedit.text !== "" ? true : false
        width: 185
        height: 36

        onClicked: {
            deleteDialog.visible = false
            sigDoDeleteImg()
        }
    }

    onVisibleChanged: {
        setX(root.x + root.width / 2 - width / 2)
        setY(root.y + root.height / 2 - height / 2)
    }
}
