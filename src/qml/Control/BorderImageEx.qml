// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11

Image {
    id: boerderImageEx
    smooth: true
    antialiasing: true
    fillMode: Image.PreserveAspectCrop
    property string url: ""
    //border and shadow
    Rectangle {
        id: borderRect
        anchors.fill: parent
        color: "transparent"
        border.color: Qt.rgba(0, 0, 0, 0.2)
        border.width: 1
        visible: true
    }
}

