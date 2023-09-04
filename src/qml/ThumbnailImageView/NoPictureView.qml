// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Layouts 1.11
import org.deepin.dtk 1.0


Item {
    anchors.fill: parent

    property bool bShowImportBtn: false
    property string iconName: "nopicture4"

    ColumnLayout {
        anchors {
            centerIn: parent
        }

        spacing: 15

        DciIcon {
            id: openViewImageIcon
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 140
            Layout.preferredHeight: 100
            sourceSize: Qt.size(140, 100)
            name: iconName
            palette: DTK.makeIconPalette(window.palette)
        }

        RecommandButton {
            id: openPictureBtn

            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 302
            Layout.preferredHeight: 36
            font.capitalization: Font.MixedCase
            text: qsTr("Import Photos and Videos")
            visible: bShowImportBtn
            onClicked:{
                importDialog.open()
            }
        }
        Label {
            Layout.alignment: Qt.AlignHCenter
            color: "#7A7A7A"
            text: bShowImportBtn ? qsTr("Or drag them here") : qsTr("No photos or videos found")
        }
    }
}
