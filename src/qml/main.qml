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
    id: window

    property bool isFullScreen: window.visibility === Window.FullScreen

    GlobalVar{
        id: global
    }
    MenuItemStates {
        id: menuItemStates
    }

    signal sigTitlePress
    // 设置 dtk 风格窗口
    DWindow.enabled: true
    //DWindow.alphaBufferSize: 8
    title: ""
    header: AlbumTitle {id: titleAlubmRect}

    background: Rectangle {
        anchors.fill: parent
        color: "transparent"

        Row {
            anchors.fill: parent
            Rectangle {
                id: leftBgArea
                width: GStatus.sideBarWidth
                height: parent.height
                anchors.top: parent.top
                color: DTK.themeType === ApplicationHelper.LightType ? Qt.rgba(1, 1, 1, 0.9)
                                                                          : Qt.rgba(16.0/255.0, 16.0/255.0, 16.0/255.0, 0.85)
                Rectangle {
                    width: 1
                    height: parent.height
                    anchors.right: parent.right
                    color: DTK.themeType === ApplicationHelper.LightType ? "#eee7e7e7"
                                                                         : "#ee252525"
                }
            }
            Rectangle {
                id: rightBgArea
                width: parent.width - leftBgArea.width
                height: 50
                anchors.top: parent.top
                color: Qt.rgba(0, 0, 0, 0.01)
                BoxShadow {
                    anchors.fill: rightBgArea
                    shadowOffsetX: 0
                    shadowOffsetY: 4
                    shadowColor: Qt.rgba(0, 0, 0, 0.05)
                    shadowBlur: 10
                    cornerRadius: rightBgArea.radius
                    spread: 0
                    hollow: true
                }
            }
        }
    }

    MessageManager.layout: Column {
        anchors {
            bottom: parent.bottom
            bottomMargin: GStatus.statusBarHeight + 5
            horizontalCenter: parent.horizontalCenter
        }
    }

    visible: true
    minimumHeight: GStatus.minHeight
    minimumWidth: GStatus.minWidth
    width: fileControl.getlastWidth()
    height: fileControl.getlastHeight()

    flags: Qt.Window | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    Component.onCompleted: {
        setX(screen.width / 2 - width / 2);
        setY(screen.height / 2 - height / 2);

        // 合集-所有项视图延迟刷新，解决其加载时会闪烁显示一张缩略图的问题
        GStatus.currentViewType = Album.Types.ViewCollecttion

        GStatus.loading = false
        GStatus.currentDeviceName = albumControl.getDeviceName(GStatus.currentDevicePath)
    }

    onActiveChanged: {
        // 记录应用主窗口是否被置灰过
        if (!window.active)
            GStatus.windowDisActived = true
    }

    onWidthChanged: {
        if(window.visibility!=Window.FullScreen && window.visibility !=Window.Maximized){
            fileControl.setSettingWidth(width)
        }
    }

    onHeightChanged: {
        if(window.visibility!=Window.FullScreen &&window.visibility!=Window.Maximized){
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
            var bIsCustomAlbumImport = GStatus.currentViewType === Album.Types.ViewCustomAlbum && albumControl.isCustomAlbum(GStatus.currentCustomAlbumUId)
            //自定义相册不需要判重
            albumControl.importAllImagesAndVideosUrl(importDialog.fileUrls, GStatus.currentCustomAlbumUId, !bIsCustomAlbumImport)
        }
    }

    StackControl{
        id: stackControl
    }

    Album.EventGenerator {
        id: eventGenerator
    }

    Connections {
        target: albumControl
        onSigActiveApplicationWindow: {
            window.requestActivate()
        }
    }
}
