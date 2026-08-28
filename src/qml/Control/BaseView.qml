// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls

import "./Animation"
FadeInoutAnimation {
    Accessible.name: "BaseView_FadeInoutAnimation"
    Accessible.role: Accessible.Client
    anchors.fill: parent
    anchors.topMargin: 0
    anchors.leftMargin: 20
    anchors.bottomMargin: statusBar.height
}

