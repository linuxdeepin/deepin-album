// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.1
import QtQml 2.11

//视频时长标签

Item {
    id: root
    visible: false
    width: 60
    height: 30

    property string displayStr: ""

    Rectangle {
        color: "#EEEEEE"
        radius: 20
        anchors.fill: parent

        Text {
            anchors.centerIn: parent
            text: displayStr
        }
    }
}
