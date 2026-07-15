// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.deepin.album 1.0 as Album

// View Loader created only when this view becomes current, then stays loaded.
Loader {
    // The Album.Types.View* type this Loader carries.
    property int viewType: -1
    property bool loadedOnce: false

    anchors.fill: parent
    asynchronous: false
    active: loadedOnce || GStatus.currentViewType === viewType
    onLoaded: loadedOnce = true
}
