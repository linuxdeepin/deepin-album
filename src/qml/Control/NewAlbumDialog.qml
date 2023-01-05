// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Window 2.10
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11

import org.deepin.dtk 1.0 as D
import org.deepin.dtk 1.0

import "../"

DialogWindow {
    id: renamedialog
    modality: Qt.WindowModal
    flags: Qt.Window | Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
    title: " "
    visible: false
    property bool isChangeView: false
    property bool importSelected: false
    minimumWidth: 400
    maximumWidth: 400
    minimumHeight: 190
    maximumHeight: 190

    width: 400
    height: 190

    icon : "deepin-album"

    signal sigCreateAlbumDone() //创建相册完成信号

    function setNormalEdit()
    {
        //重新设置焦点和名称
        nameedit.focus=true
        nameedit.text=qsTr("Unnamed")
    }

    Text {
        id: renametitle
        width: 308
        height: 24
        anchors.left: parent.left
        anchors.leftMargin: 46
        anchors.top: parent.top
        font.pixelSize: 16
        text: qsTr("New Album")
        verticalAlignment: Text.AlignBottom
        horizontalAlignment: Text.AlignHCenter
    }
    Label{
        id:nameLabel
        width:42
        height: 20
        font.pixelSize: 14
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: nameedit.top
        anchors.topMargin:5
        text:qsTr("Name:")
    }
    LineEdit {
        id: nameedit
        anchors.top: renametitle.bottom
        anchors.topMargin: 16
        anchors.left: parent.left
        anchors.leftMargin: 52
        width: 328
        height: 36
        font: DTK.fontManager.t5
        focus: true
        maximumLength: 255
        validator: RegExpValidator {regExp: /^[^\\.\\\\/\':\\*\\?\"<>|%&][^\\\\/\':\\*\\?\"<>|%&]*/ }
        text: qsTr("Unnamed")
        selectByMouse: true
//        alertText: qsTr("The file already exists, please use another name")
//        showAlert: fileControl.isShowToolTip(source,nameedit.text)
    }


    Button {
        id: cancelbtn
        anchors.top: nameedit.bottom
        anchors.topMargin: 20
        anchors.left: parent.left
        anchors.leftMargin: 0
        text: qsTr("Cancel")
        width: 185
        height: 36
        font.pixelSize: 16
        onClicked: {
            renamedialog.visible = false
        }
    }

    Button {
        id: enterbtn
        anchors.top: nameedit.bottom
        anchors.topMargin: 20
        anchors.left: cancelbtn.right
        anchors.leftMargin: 10
        text: qsTr("Confirm")
        enabled: nameedit.text !== "" ? true : false
        width: 185
        height: 36

        onClicked: {
            albumControl.createAlbum(nameedit.text )
            global.albumChangeList = !global.albumChangeList
            renamedialog.visible = false

            // 获取新相册index
            var index = albumControl.getAllCustomAlbumId().length - 1

            // 导入已选图片
            if (importSelected) {
                albumControl.insertIntoAlbum(albumControl.getAllCustomAlbumId()[index] , global.selectedPaths)
            }

            // 切换到对应相册视图
            if (isChangeView) {
                global.currentViewIndex = GlobalVar.ThumbnailViewType.CustomAlbum
                global.currentCustomAlbumUId = albumControl.getAllCustomAlbumId()[index]
                sigCreateAlbumDone()
            }

            //侧边栏如果是关闭状态，侧边栏会自动打开
            if (leftSidebar.x !== 0) {
                showSliderAnimation.start()
            }
        }
    }

    onVisibleChanged: {
        setX(root.x  + root.width / 2 - width / 2)
        setY(root.y  + root.height / 2 - height / 2)
    }
}
