// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4

import org.deepin.album 1.0 as Album

Item {
    id: switchViewAnimation

    property bool show: false
    property real showOpacity: 1
    property real showX: 0

    property real hideOpacity: 0
    property real hideX: -width - 20

    property int switchType: GStatus.currentSwitchType
    property string switchPropertys: "x,opacity"

    onSwitchTypeChanged: {
        if (switchType === Album.Types.FlipScroll) {
            x = width + 20
            switchPropertys = "x,opacity"
        } else if (switchType === Album.Types.FadeInOut) {
            x = 0
            switchPropertys = "opacity"
        }
    }

    state: "hide"
    states: [
        State {
            name: "show"
            PropertyChanges {
                target: switchViewAnimation
                opacity: showOpacity
                x: showX
            }
            when: show
        },
        State {
            name: "hide"
            PropertyChanges {
                target: switchViewAnimation
                opacity: hideOpacity
                x: hideX
            }
            when: !show
        }
    ]

    transitions:
        Transition {
        enabled: switchType !== Album.Types.HardCut
        NumberAnimation{properties: switchPropertys; easing.type: Easing.OutExpo; duration: 400
        }
    }
}
