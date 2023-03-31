// Copyright (C) 2020 ~ 2020 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import Qt.labs.folderlistmodel 2.11

import org.deepin.dtk 1.0
import org.deepin.album 1.0 as Album

ApplicationWindow {
    GlobalVar{
        id: global
    }
    MenuItemStates {
        id: menuItemStates
    }

    signal sigTitlePress
    // 设置 dtk 风格窗口
    DWindow.enabled: true
    id: root
    title: ""
    header: AlbumTitle {id: titleAlubmRect}
    MessageManager.layout: Column {
        anchors {
            bottom: parent.bottom
            bottomMargin: global.statusBarHeight + 5
            horizontalCenter: parent.horizontalCenter
        }
    }

    visible: true
    minimumHeight: global.minHeight
    minimumWidth: global.minWidth
    width: fileControl.getlastWidth()
    height: fileControl.getlastHeight()

    flags: Qt.Window | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    Component.onCompleted: {
        setX(screen.width / 2 - width / 2);
        setY(screen.height / 2 - height / 2);
    }

    onWindowStateChanged: {
        global.sigWindowStateChange()
    }

    onActiveChanged: {
        // 记录应用主窗口是否被置灰过
        if (!root.active)
            global.windowDisActived = true
    }

    onWidthChanged: {
        if(root.visibility!=Window.FullScreen && root.visibility !=Window.Maximized){
            fileControl.setSettingWidth(width)
        }
    }

    onHeightChanged: {
        if(root.visibility!=Window.FullScreen &&root.visibility!=Window.Maximized){
            fileControl.setSettingHeight(height)
        }
    }

    //关闭的时候保存信息
    onClosing: {
        fileControl.saveSetting()
        fileControl.terminateShortcutPanelProcess() //结束快捷键面板进程
    }

    function showTitleBar(bShow) {
        titleAlubmRect.visible = bShow
    }

    FileDialog {
        id: importDialog
        title: qsTr("All photos and videos")
        folder: shortcuts.pictures
        selectMultiple: true
        nameFilters: albumControl.getAllFilters()
        onAccepted: {
            var bIsCustomAlbumImport = global.currentViewIndex == 6 && albumControl.isCustomAlbum(global.currentCustomAlbumUId)
            //自定义相册不需要判重
            albumControl.importAllImagesAndVideosUrl(importDialog.fileUrls, global.currentCustomAlbumUId, !bIsCustomAlbumImport)
        }
    }

    StackControl{
        id: stackControl
    }

    Album.EventGenerator {
        id: eventGenerator
    }
}
