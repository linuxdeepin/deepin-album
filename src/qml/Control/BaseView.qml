// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4

import "./Animation"
FadeInoutAnimation {
    anchors.fill: parent
    anchors.topMargin: 0
    anchors.leftMargin: 20
    anchors.bottomMargin: statusBar.height
}

