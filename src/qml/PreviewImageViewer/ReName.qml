// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
import org.deepin.dtk 1.0
import org.deepin.image.viewer 1.0 as IV

DialogWindow {
    id: renamedialog

    property string filesuffix: ".jpg"

    function renameFile() {
        FileControl.slotFileReName(nameedit.text, GControl.currentSource);
        renamedialog.visible = false;
    }

    function setFileName(name) {
        nameedit.text = name;
    }

    function setFileSuffix(suffix) {
        filesuffix = suffix;
    }

    flags: Qt.Window | Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
    height: 180
    maximumHeight: 180
    maximumWidth: 400
    minimumHeight: 180
    minimumWidth: 400
    modality: Qt.WindowModal
    visible: false
    width: 400

    // 调整默认的 titlebar
    header: DialogTitleBar {
        // BugFix: 暂时屏蔽 Blur 效果，待 DTK 修复 D.InWindowBlur 后恢复
        enableInWindowBlendBlur: false
        // 仅保留默认状态，否则 hover 上会有变化效果
        icon.mode: DTK.NormalState
        icon.name: "deepin-image-viewer"
    }

    onVisibleChanged: {
        if (visible) {
            setFileName(FileControl.slotGetFileName(GControl.currentSource));
            setFileSuffix(FileControl.slotFileSuffix(GControl.currentSource));
            setX(window.x + window.width / 2 - width / 2);
            setY(window.y + window.height / 2 - height / 2);
        }
    }

    Text {
        id: renametitle


        color: DTK.themeType === ApplicationHelper.LightType ? "black" : "white"
        font.pixelSize: 16
        height: 24
        horizontalAlignment: Text.AlignHCenter
        text: qsTr("Input a new name")
        textFormat: Text.PlainText
        verticalAlignment: Text.AlignBottom
        width: 308

        anchors {
            left: parent.left
            leftMargin: 46
            top: parent.top
        }
    }

    LineEdit {
        id: nameedit

        alertText: qsTr("The file already exists, please use another name")
        focus: true
        font: DTK.fontManager.t5
        height: 36
        maximumLength: 255 - filesuffix.length
        selectByMouse: true
        showAlert: FileControl.isShowToolTip(GControl.currentSource, nameedit.text)
        width: 380

        validator: RegularExpressionValidator {
            regularExpression: /^[^ \\.\\\\/\':\\*\\?\"<>|%&][^\\\\/\':\\*\\?\"<>|%&]*/
        }

        Keys.onPressed: (event)=> {
            switch (event.key) {
            case Qt.Key_Return:
            case Qt.Key_Enter:
                renameFile();
                break;
            case Qt.Key_Escape:
                renamedialog.visible = false;
                break;
            default:
                break;
            }
        }

        anchors {
            top: renametitle.bottom
            topMargin: 16
        }
    }

    RecommandButton {
        id: enterbtn

        enabled: !FileControl.isShowToolTip(GControl.currentSource, nameedit.text) && nameedit.text.length > 0
        height: 36
        text: qsTr("Confirm")
        width: 185

        onClicked: {
            renameFile();
        }

        anchors {
            right: nameedit.right
            top: nameedit.bottom
            topMargin: 10
        }
    }

    Button {
        id: cancelbtn

        height: 36
        text: qsTr("Cancel")
        width: 185

        onClicked: {
            renamedialog.visible = false;
        }

        anchors {
            left: nameedit.left
            top: nameedit.bottom
            topMargin: 10
        }
    }
}
