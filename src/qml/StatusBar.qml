// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQml.Models 2.11
import QtQml 2.11
import QtQuick.Shapes 1.10
import org.deepin.dtk 1.0

Item {
    id: statusBar

    property int sliderValue: slider.value

    function setSliderWidgetValue(value) {
        slider.value = value
    }

    Label {
        height: parent.height
        anchors.centerIn: parent
        text: global.statusBarNumText
    }

    Slider {
        id: slider
        width: 160
        height: parent.height
        anchors {
            right: parent.right
            rightMargin: 50
        }
    }
}
