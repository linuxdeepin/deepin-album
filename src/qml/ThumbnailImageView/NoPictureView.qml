// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import org.deepin.dtk 1.0


Item {
    anchors.fill: parent
    Item {
        anchors.centerIn: parent
        ActionButton {
            id: noImageIcon
            anchors {
                top: parent.top
                topMargin: -70
                left : parent.left
                leftMargin: -width / 2
            }

            icon {
                name:"nopicture1"
                width: 140
                height: 140
            }
        }
        Label{
            anchors {
                top:noImageIcon.bottom
                topMargin: 20
                left : parent.left
                leftMargin: -width/2
            }
            text:qsTr("No photos or videos found")
        }
    }
}
