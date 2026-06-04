// SPDX-FileCopyrightText: 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.deepin.album 1.0 as Album

// View Loader deferred until after window map (or when this view becomes current), stays loaded once created.
// Delay logic unified in one place to avoid repetition across views.
//   active = ready (after map) || current view is this view
Loader {
    // Set to true by host to signal deferral can start (after map).
    property bool ready: false
    // The Album.Types.View* type this Loader carries.
    property int viewType: -1

    anchors.fill: parent
    asynchronous: false
    active: ready || GStatus.currentViewType === viewType
}
