// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

    signal sigDoRemoveAlbum(int type)

    property int deleteType: 0

    Text {
        id: deleteTitle
        anchors.top: parent.top
        anchors.topMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        font: DTK.fontManager.t5
        text: qsTr("Are you sure you want to delete this album?")
        verticalAlignment: Text.AlignBottom
        horizontalAlignment: Text.AlignHCenter
    }

    Button {
        id: cancelbtn
        anchors.top: deleteTitle.bottom
        anchors.topMargin: 50
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
        anchors.top: deleteTitle.bottom
        anchors.topMargin: 50
        anchors.left: cancelbtn.right
        anchors.leftMargin: 10
        text: qsTr("Delete")
        width: 185
        height: 36

        onClicked: {
            deleteDialog.visible = false
            sigDoRemoveAlbum(deleteType)
        }
    }

    onVisibleChanged: {
        setX(root.x + root.width / 2 - width / 2)
        setY(root.y + root.height / 2 - height / 2)
    }
}
