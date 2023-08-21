// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Window 2.10
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11

import org.deepin.dtk 1.0
import org.deepin.album 1.0 as Album

import "../"

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

    property int type: 0

    signal sigDoDeleteImg()
    signal sigDoAllDeleteImg()

    function setDisplay(deltype, count) {
        type = deltype
        if(deltype === Album.Types.TrashNormal) {
            if(count === 1) {
                deleteTitle.text = qsTr("Are you sure you want to delete this file locally?")
                deleteTips.text  = qsTr("You can restore it in the trash")
            } else {
                deleteTitle.text = qsTr("Are you sure you want to delete %1 files locally?").arg(count)
                deleteTips.text  = qsTr("You can restore them in the trash")
            }
        } else {
            if(count === 1) {
                deleteTitle.text = qsTr("Are you sure you want to permanently delete this file?")
                deleteTips.text  = qsTr("You cannot restore it any longer")
            } else {
                deleteTitle.text = qsTr("Are you sure you want to permanently delete %1 files?").arg(count)
                deleteTips.text  = qsTr("You cannot restore them any longer")
            }
        }
    }

    //不显示弹窗直接删除
    function deleteDirectly() {
        sigDoDeleteImg()
    }

    Text {
        id: deleteTitle
        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
        }
        font: DTK.fontManager.t6
        verticalAlignment: Text.AlignBottom
        horizontalAlignment: Text.AlignHCenter
    }

    Text {
        id: deleteTips
        opacity: 0.7
        anchors {
            top: deleteTitle.bottom
            horizontalCenter: deleteTitle.horizontalCenter
        }
        font: DTK.fontManager.t6
        verticalAlignment: Text.AlignBottom
        horizontalAlignment: Text.AlignHCenter
    }

    Button {
        id: cancelbtn
        anchors {
            top: deleteTips.bottom
            topMargin: 30
            left: parent.left
            leftMargin: 0
        }
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
        anchors {
            top: deleteTips.bottom
            topMargin: 30
            left: cancelbtn.right
            leftMargin: 10
        }
        text: qsTr("Delete")
        width: 185
        height: 36

        onClicked: {
            deleteDialog.visible = false
            switch (type) {
                case Album.Types.TrashNormal:
                case Album.Types.TrashSel:
                    sigDoDeleteImg()
                    break
                case Album.Types.TrashAll:
                    sigDoAllDeleteImg()
                    break
            }

        }
    }

    onVisibleChanged: {
        setX(window.x + window.width / 2 - width / 2)
        setY(window.y + window.height / 2 - height / 2)
    }
}
