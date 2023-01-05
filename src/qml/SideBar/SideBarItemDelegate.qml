// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.0
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import QtQml.Models 2.11
import org.deepin.dtk 1.0


ItemDelegate {
    id: item
    MouseArea {
        anchors.fill: item
        acceptedButtons: Qt.RightButton | Qt.LeftButton
        onDoubleClicked:{
            item.rename()
        }
        onClicked: {
            sideListView.currentIndex = index
            item.forceActiveFocus()
            control.itemClicked(model.uuid, model.displayName)
            item.checked = true
            if(mouse.button === Qt.RightButton){
                control.itemRightClicked(model.uuid, model.displayName)
            }
        }
    }
    DciIcon {
        id: siderIcon
        anchors.left: item.left; anchors.leftMargin: 10
        anchors.verticalCenter: item.verticalCenter
        name: model.icon
        sourceSize: Qt.size(20, 20)
    }
    Label {
        id: songName
        width: 100;
        anchors.left: siderIcon.right; anchors.leftMargin: 10
        anchors.verticalCenter: item.verticalCenter
        text: "%1".arg(model.displayName)
        elide: Text.ElideRight
    }
    // 移除设备按钮，仅在设备列表显示
    ActionButton {
        visible: control.showRemoveDeviceBtn
        anchors.left: songName.right; anchors.leftMargin: 10
        anchors.verticalCenter: item.verticalCenter
        width: 10
        height:  10
        icon.name: "arrow"
        icon.width: 10
        icon.height: 10
        onClicked:{
            control.removeDeviceBtnClicked(index)
        }
    }
    LineEdit {
        id: keyLineEdit
        width: 180
        visible: false;
        maximumLength: 30
        validator: RegExpValidator {regExp: /^[^\\.\\\\/\':\\*\\?\"<>|%&][^\\\\/\':\\*\\?\"<>|%&]*/ }
        onEditingFinished: {
            item.checked = true;
            songName.visible = true;
            siderIcon.visible = true;
            keyLineEdit.visible = false;
            if (keyLineEdit.text !== "" && albumControl.renameAlbum(albumControl.getAllCustomAlbumId()[index], keyLineEdit.text)) {
                songName.text = keyLineEdit.text

                // 通知自定义相册视图刷新相册名称
                global.sigCustomAlbumNameChaged(global.currentCustomAlbumUId, songName.text)
            }
        }
        onActiveFocusChanged: {
            //EventsFilter.setEnabled(!activeFocus)
        }
    }

    onCheckedChanged: {
        control.itemCheckedChanged(index, item.checked)
    }

    function enableRename(){
        item.checked = true;
        keyLineEdit.text = songName.text;
        keyLineEdit.forceActiveFocus()
        songName.visible = false;
        siderIcon.visible = false;
        keyLineEdit.visible = true;
        item.checked = false;
    }
    function rename(){
        if(!model.editable)
            return;
        enableRename();
        keyLineEdit.selectAll();
    }
    function switchToPrevious(){
        item.forceActiveFocus();
        control.itemClicked(model.uuid, model.displayName);
        item.checked = true
    }

    // 屏蔽空格响应
    Keys.onSpacePressed: { event.accepted=false; }
    Keys.onReleased: { event.accepted=(event.key===Qt.Key_Space); }
}
