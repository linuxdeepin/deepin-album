// SPDX-FileCopyrightText: 2023 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQml
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs

import org.deepin.dtk 1.0
import org.deepin.dtk.style 1.0 as DS
import org.deepin.album 1.0 as Album

import "./Control/Animation"

ApplicationWindow {
    id: window

    property bool isFullScreen: window.visibility === Window.FullScreen
    property bool backgroundBlurReady: false
    color: "transparent"

    // Bug fix: 使用 ListView 替换 PathView 时，出现内部的 mouseArea 鼠标操作会被 DWindow 截取
    // 导致 flicking 时拖动窗口，此处使用此标志禁用此行为
    DWindow.enableSystemMove: !GStatus.viewFlicking

    // 设置 dtk 风格窗口
    DWindow.enabled: true
    DWindow.alphaBufferSize: 8
    DWindow.enableBlurWindow: true
    title: ""
    header: AlbumTitle {
        id: titleAlubmRect

        onForceExit: {
            saveAndTerminate()
        }
    }

    background: Rectangle {
        anchors.fill: parent
        color: "transparent"

        // uos-design: right content area opaque base — must not extend under the
        // blur sidebar or the frosted area degrades into a solid panel.
        Rectangle {
            id: rightContentBg
            visible: GStatus.stackControlCurrent === 0
            anchors {
                top: parent.top
                bottom: parent.bottom
                left: parent.left
                leftMargin: leftBgArea.width
                right: parent.right
            }
            color: DTK.themeType === ApplicationHelper.LightType ? "#f8f8f8" : "#202020"
        }

        // Opaque base for image viewer / slideshow so a transparent window
        // surface doesn't show the desktop behind content.
        Rectangle {
            visible: GStatus.stackControlCurrent !== 0
            anchors.fill: parent
            color: DTK.themeType === ApplicationHelper.LightType ? "#f8f8f8" : "#202020"
        }

        Row {
            anchors.fill: parent
            Loader {
                id: leftBgArea
                width: GStatus.sideBarWidth
                height: parent.height
                anchors.top: parent.top
                active: window.backgroundBlurReady && GStatus.stackControlCurrent === 0
                sourceComponent: StyledBehindWindowBlur {
                    // uos-design: sidebar frosted-glass surface via the verified DTK
                    // compositor behind-window blur path (dde-control-center baseline).
                    control: window
                    anchors.fill: parent
                    blendColor: {
                        // Compositor blur available: translucent tint so the blur reads through.
                        if (valid) {
                            return DS.Style.control.selectColor(undefined,
                                Qt.rgba(238 / 255, 238 / 255, 238 / 255, 0.8),
                                Qt.rgba(20 / 255, 20 / 255, 20 / 255, 0.8))
                        }
                        // No compositor blur: solid panel via the system no-blur token.
                        return DS.Style.control.selectColor(undefined,
                            DS.Style.behindWindowBlur.lightNoBlurColor,
                            DS.Style.behindWindowBlur.darkNoBlurColor)
                    }
                    Rectangle {
                        width: 1
                        height: parent.height
                        anchors.right: parent.right
                        color: DTK.themeType === ApplicationHelper.LightType ? "#eee7e7e7"
                                                                             : "#11a2a2a2"
                    }
                }
            }
            Loader {
                active: window.backgroundBlurReady
                visible: GStatus.stackControlCurrent === 0
                width: parent.width
                height: 50
                sourceComponent: Rectangle {
                    anchors.fill: parent
                    color: Qt.rgba(0, 0, 0, 0.01)
                    BoxShadow {
                        anchors.fill: parent
                        shadowOffsetX: 0
                        shadowOffsetY: 4
                        shadowColor: Qt.rgba(0, 0, 0, 0.05)
                        shadowBlur: 10
                        cornerRadius: parent.radius
                        spread: 0
                        hollow: true
                    }
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
    width: FileControl.getlastWidth()
    height: FileControl.getlastHeight()
    // Bind centered coordinates at declaration time so first frame map is already centered,
    // avoiding position jump/flicker from mapping at (0,0) then setX/setY in onCompleted.
    x: Screen.width / 2 - width / 2
    y: Screen.height / 2 - height / 2

    flags: Qt.Window | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    Component.onCompleted: {
        // 合集-所有项视图延迟刷新，解决其加载时会闪烁显示一张缩略图的问题
        GStatus.currentViewType = Album.Types.ViewCollecttion
        GStatus.currentDeviceName = albumControl.getDeviceName(GStatus.currentDevicePath)
        Qt.callLater(function() {
            backgroundBlurReady = true
        })
    }

    onActiveChanged: {
        // 记录应用主窗口是否被置灰过
        if (!window.active)
            GStatus.windowDisActived = true
    }

    onWidthChanged: {
        if(window.visibility!=Window.FullScreen && window.visibility !=Window.Maximized){
            FileControl.setSettingWidth(width)
        }

        GStatus.enableRatioAnimation = false
    }

    onHeightChanged: {
        if(window.visibility!=Window.FullScreen &&window.visibility!=Window.Maximized){
            FileControl.setSettingHeight(height)
        }
    }

    //关闭的时候保存信息
    onClosing: {
        saveAndTerminate()
    }

    FileDialog {
        id: importDialog
        title: qsTr("All photos and videos")
        currentFolder: FileControl.standardPicturesPath()
        fileMode: FileDialog.OpenFiles
        nameFilters: albumControl.getAllFilters()
        onAccepted: {
            var bIsCustomAlbumImport = GStatus.currentViewType === Album.Types.ViewCustomAlbum && albumControl.isCustomAlbum(GStatus.currentCustomAlbumUId)
            //自定义相册不需要判重
            albumControl.importAllImagesAndVideosUrl(importDialog.selectedFiles, GStatus.currentCustomAlbumUId, !bIsCustomAlbumImport)
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
        function onSigActiveApplicationWindow() {
            window.requestActivate()
        }
    }

    // 保存并退出程序
    function saveAndTerminate() {
        FileControl.saveSetting()
        FileControl.terminateShortcutPanelProcess() //结束快捷键面板进程
        GControl.forceExit();
    }
}
