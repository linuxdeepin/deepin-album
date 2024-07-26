// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.1
import QtQml 2.11

import org.deepin.dtk 1.0
//视频时长标签
Label {
    property string displayStr: ""

    text: displayStr

    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter

    property Palette textColor: Palette {
        normal: ("black")
        normalDark: ("white")
    }
    color: ColorSelector.textColor
    background: Rectangle {
        color: DTK.themeType === ApplicationHelper.LightType ? "#EEEEEE" : "#111111"
        radius: 20
        anchors.centerIn: parent
        width: parent.width + 10
        height: parent.height
    }
}


