// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4

Item {
    id: switchViewAnimation

    property bool show: false
    property real showOpacity: 1
    property real showX: 0

    property real hideOpacity: 0
    property real hideX: -width - 20

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

    transitions: Transition {
        NumberAnimation{properties: "x,opacity"; easing.type: Easing.OutExpo; duration: 400}
    }
}
