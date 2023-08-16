// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Window 2.10
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11

import org.deepin.dtk 1.0 as D
import org.deepin.dtk 1.0

DialogWindow {
    id: emptyWarningDlg
    modality: Qt.WindowModal
    width: 400
    icon: "deepin-album"

    ColumnLayout {
        spacing: 10
        width: parent.width
        Label {
            Layout.alignment: Qt.AlignHCenter
            horizontalAlignment: Text.AlignHCenter
            font: DTK.fontManager.t5
            text: qsTr("File name cannot be empty!")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        Button {
            Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
            Layout.bottomMargin: 10
            Layout.fillWidth: true
            text: qsTr("OK")

            onClicked: {
                emptyWarningDlg.visible = false
            }
        }
    }

    onVisibleChanged: {
        setX(window.x  + window.width / 2 - width / 2)
        setY(window.y  + window.height / 2 - height / 2)
    }
}

