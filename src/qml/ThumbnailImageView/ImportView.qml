// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.0
import org.deepin.dtk 1.0


Item {
    anchors.fill: parent
    Item {
        anchors.centerIn: parent
        ActionButton {
            id: openViewImageIcon
            anchors {
                top: parent.top
                topMargin: -70
                left : parent.left
                leftMargin: -width / 2
            }

            icon {
                name:"nopicture2"
                width: 140
                height: 140
            }
        }
        RecommandButton{
            id: openPictureBtn
            font.capitalization: Font.MixedCase
            text: qsTr("Import Photos and Videos") 
            width: 302
            height: 36
            anchors {
                top:openViewImageIcon.bottom
                topMargin:10
                left : parent.left
                leftMargin: -width/2
            }

            onClicked:{
                importDialog.open()
            }
        }
        Label{
            anchors {
                top:openPictureBtn.bottom
                topMargin: 20
                left : parent.left
                leftMargin: -width/2
            }
            text:qsTr("Or drag them here")
        }
    }
}
