// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4

Item {
    id: fadeInoutAnimation
    signal stoped()
    property bool show: true
    property string idName: ""

    onOpacityChanged: {
        if ((opacity === 1 || opacity === 0) && !GStatus.loading) {
            stoped()
        }
    }

    state: "hide"
    states: [
        State {
            name: "show"
            PropertyChanges {
                target: fadeInoutAnimation
                opacity: 1
                visible: true
            }
            when: show
        },
        State {
            name: "hide"
            PropertyChanges {
                target: fadeInoutAnimation
                opacity: 0
                visible: false
            }
            when: !show
        }
    ]

    transitions:
        Transition {
        enabled: GStatus.enableFadeInoutAnimation
        NumberAnimation{properties: "opacity,visible"; easing.type: Easing.OutExpo; duration: GStatus.animationDuration
        }
    }
}
