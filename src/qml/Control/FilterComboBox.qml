// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import QtQml
import org.deepin.dtk 1.0
import org.deepin.dtk 1.0 as D
import org.deepin.dtk.style 1.0 as DS

ComboBox {
    id: comboBox
    Accessible.name: "FilterComboBox"
    Accessible.role: Accessible.ComboBox
    textRole: "text"
    iconNameRole: "icon"
    flat: true

    // 隐藏文本，用于测量各选项文字宽度
    Text { id: _measure; visible: false }

    // 选中项：图标 16x16、文字左对齐、超长省略号截断并显示 Tooltip
    contentItem: RowLayout {
        spacing: 6
        D.DciIcon {
            sourceSize: Qt.size(16, 16)
            name: comboBox.model.get(comboBox.currentIndex)
                  ? comboBox.model.get(comboBox.currentIndex)[comboBox.iconNameRole]
                  : ""
            palette: comboBox.D.DTK.makeIconPalette(comboBox.palette)
            mode: comboBox.D.ColorSelector.controlState
            theme: comboBox.D.ColorSelector.controlTheme
            Layout.alignment: Qt.AlignVCenter
        }
        Label {
            text: comboBox.currentText
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Qt.AlignVCenter
            font: comboBox.font
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            ToolTip.text: text
            ToolTip.visible: truncated && comboBox.hovered
            ToolTip.delay: 500
        }
    }

    // 以最长选项确定按钮宽度，切换选项时宽度保持一致，上限 140px
    Component.onCompleted: {
        _measure.font = comboBox.font
        var maxTextW = 0
        for (var i = 0; i < count; i++) {
            _measure.text = model.get(i)[textRole]
            maxTextW = Math.max(maxTextW, _measure.implicitWidth)
        }
        width = Math.max(width, Math.min(16 + 6 + maxTextW + leftPadding + rightPadding, 140))
    }

    model: ListModel {
        ListElement { text: qsTr("All"); icon: "selectall" }
        ListElement { text: qsTr("Photos"); icon: "images" }
        ListElement { text: qsTr("Videos"); icon: "videos" }
    }

    delegate: MenuItem {
        useIndicatorPadding: true
        width: parent.width
        text: comboBox.textRole ? (Array.isArray(comboBox.model) ? modelData[comboBox.textRole] : model[comboBox.textRole]) : modelData
        // 下拉项图标与选中项保持一致 16x16
        icon.width: 16
        icon.height: 16
        icon.name: (comboBox.iconNameRole && model[comboBox.iconNameRole] !== undefined) ? model[comboBox.iconNameRole] : null
        highlighted: comboBox.highlightedIndex === index
        hoverEnabled: comboBox.hoverEnabled
        autoExclusive: true
        checked: comboBox.currentIndex === index
    }
}
