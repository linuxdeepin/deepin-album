// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQml.Models 2.11
import QtQml 2.11
import QtQuick.Shapes 1.10
import org.deepin.dtk 1.0
import QtGraphicalEffects 1.0

import org.deepin.album 1.0 as Album

import "../"
import "../../"

Item {
    id: main
    //注意：在model里面加进去的变量，这边可以直接进行使用，只是部分位置不好拿到，需要使用变量
    property string m_index
    property string m_url
    property string m_displayFlushHelper
    property var m_favoriteBtn: itemFavoriteBtn
    property string remainDays
    property bool bShowDamageIcon: image.bLoadError
    property bool bSelected: theView.ism.indexOf(parent.m_index) !== -1 || GStatus.selectedPaths.indexOf(m_url) !== -1
    property bool bHovered: false //属性：是否hover
    property bool bFavorited: albumControl.photoHaveFavorited(model.url, GStatus.bRefreshFavoriteIconFlag)
    property bool bShowRemainDays: GStatus.currentViewType === Album.Types.ViewRecentlyDeleted
    property bool bShowVideoLabel: fileControl.isVideo(m_url)
    property Item selectIcon: null
    property Item selectFrame: null
    property Item favoriteBtn: null
    property Item remainDaysLbl: null
    property Item videoLabel: null

    //缩略图本体
    Image {
        id: image
        source: m_url !== "" ? "image://asynImageProvider/" + m_displayFlushHelper + theView.displayFlushHelper.toString() + "_" + m_url : ""
        asynchronous: false
        anchors.centerIn: parent
        width: parent.width - 14
        height: parent.height - 14
        //使用PreserveAspectFit确保在原始比例下不变形
        fillMode: Image.PreserveAspectFit
        visible: false

        //由于qml图片加载状态值在使用QQuickAsyncImageProvider异步加载方式后有异常（图片加载错误没有设置错误状态），暂时自定义类型进行判断错误状态
        property bool bLoadError: false

        onStatusChanged: {
            if (status === Image.Ready && sourceSize === Qt.size(0, 0)) {
                bLoadError = true;
            } else {
                bLoadError = false;
            }
        }
    }

    Loader {
        id: damageIconLoader
        anchors.centerIn: parent

        // 判断是否加载错误图片状态组件
        active: bShowDamageIcon
        sourceComponent: ActionButton {
            anchors.centerIn: parent
            ColorSelector.hovered: false
            icon {
                name: "photo_breach"
                width: image.width
                height: image.height
            }
        }
    }

    // 图片保存完成，缩略图区域重新加载当前图片
    Connections {
        target: fileControl
        function onCallSavePicDone() {
            if (path === m_url) {
                m_displayFlushHelper = Math.random()
            }
        }
    }

    //圆角遮罩Rectangle
    Rectangle {
        id: maskRec
        anchors.centerIn: parent
        width: image.width
        height: image.height

        color:"transparent"
        Rectangle {
            anchors.centerIn: parent
            width: image.paintedWidth
            height: image.paintedHeight
            color:"black"
            radius: 10
        }
        visible: false
    }

    OpacityMask{
        id: opacityMask
        anchors.fill: image
        source: image
        maskSource: mask
    }

    FastBlur {
        anchors.top: opacityMask.top; anchors.topMargin: 6
        anchors.left: opacityMask.left; anchors.leftMargin: 1
        width: opacityMask.width - 2; height: opacityMask.width - 6
        source: opacityMask
        radius: 10
        transparentBorder: true
    }

    //遮罩执行
    OpacityMask {
        id: mask
        anchors.fill: image
        source: image
        maskSource: maskRec
        antialiasing: true
        smooth: true
    }

    //border and shadow
    Rectangle {
        id: borderRect
        anchors.fill: image
        color: Qt.rgba(0, 0, 0, 0)
        border.color: Qt.rgba(0, 0, 0, 0.1)
        border.width: 1
        visible: true
        radius: 10
    }

    MouseArea {
        id:mouseAreaTopParentRect
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true

        //属性：是否hover
        property bool bHovered: false

        onClicked: {
            //允许鼠标事件传递给子控件处理,否则鼠标点击缩略图收藏图标不能正常工作
            mouse.accepted = false
        }

        onEntered: {
            bHovered = true
        }

        onExited: {
            bHovered = false
        }
    }

    //收藏图标
    ActionButton {
        id: itemFavoriteBtn
        visible: albumControl.photoHaveFavorited(m_url, GStatus.bRefreshFavoriteIconFlag) || mouseAreaTopParentRect.bHovered
        anchors {
            bottom: image.bottom
            left: image.left
            leftMargin : (image.width - image.paintedWidth) / 2 + 5
            bottomMargin : (image.height - image.paintedHeight) / 2 + 5
        }
        hoverEnabled: false  //设置为false，可以解决鼠标移动到图标附近时，图标闪烁问题

        icon {
            name: albumControl.photoHaveFavorited(m_url, GStatus.bRefreshFavoriteIconFlag) ? "collected" : "collection2"
        }

        MouseArea {
            id:mouseAreaFavoriteBtn
            anchors.fill: itemFavoriteBtn
            propagateComposedEvents: true

            onClicked: {
                var paths = []
                paths.push(m_url)

                if (albumControl.photoHaveFavorited(m_url, GStatus.bRefreshFavoriteIconFlag)) {
                    //取消收藏
                    albumControl.removeFromAlbum(0, paths)
                } else {
                    //收藏
                    albumControl.insertIntoAlbum(0, paths)
                }

                GStatus.bRefreshFavoriteIconFlag = !GStatus.bRefreshFavoriteIconFlag
                // 若当前视图为我的收藏，需要实时刷新我的收藏列表内容
                if (GStatus.currentViewType === Album.Types.ViewFavorite && GStatus.currentCustomAlbumUId === 0) {
                    GStatus.sigFlushCustomAlbumView(GStatus.currentCustomAlbumUId)
                }

                mouse.accepted = true
            }

        }
    }

    onBSelectedChanged: {
        if (bSelected) {
            if (selectIcon == null)
                selectIcon = selectedIconComponent.createObject(main)
            if (selectFrame == null)
                selectFrame = selectedFrameComponent.createObject(main)
        } else {
            selectIcon.destroy()
            selectIcon = null
            selectFrame.destroy()
            selectFrame = null
        }
    }

    onBFavoritedChanged: {
        if (bFavorited) {
            if (favoriteBtn == null) {
                favoriteBtn = favoriteComponent.createObject(main)
            }
        } else {
            if (favoriteBtn && !bHovered) {
                favoriteBtn.destroy()
                favoriteBtn = null
            }
        }
    }

    onBShowRemainDaysChanged: {
        if (bShowRemainDays) {
            if (remainDaysLbl == null) {
                remainDaysLbl = remainDaysComponent.createObject(main)
            }
        } else {
            remainDaysLbl.destroy()
            remainDaysLbl = null
        }
    }

    onBShowVideoLabelChanged: {
        if (bShowVideoLabel) {
            if (videoLabel == null) {
                videoLabel = videoTimeComponent.createObject(main)
            }
        } else {
            videoLabel.destroy()
            videoLabel = null
        }
    }

    // 选中图标组件
    Component {
        id: selectedIconComponent
        Item {
            id: imageArea
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.topMargin: 2
            anchors.rightMargin: 2
            width: 20
            height: 20

            //选中后显示的图标
            DciIcon {
                name: "select_active_1"
                visible: true
                anchors.centerIn: parent
            }

            DciIcon {
                name: "Inner_shadow"
                visible: true
                anchors.centerIn: parent
            }

            DciIcon {
                name: "shadow"
                visible: true
                anchors.centerIn: parent
            }

            DciIcon {
                name: "yes"
                visible: true
                anchors.centerIn: parent
            }
        }
    }

    // 选中框组件
    Component {
        id: selectedFrameComponent
        Item {
            anchors.fill: parent

            z: -1
            // 计算图片区域的位置
            Rectangle {
                id: imageArea
                anchors.centerIn: parent
                width: parent.width - 14
                height: parent.height - 14
                visible: false
            }

            //选中后显示的阴影框
            Rectangle {
                id: selectShader
                anchors.centerIn: parent
                width: parent.width
                height: parent.height
                radius: 10
                color: "#AAAAAA"
                visible: true
                opacity: 0.4

                border.color: Qt.rgba(0,0,0,0.1)
                border.width: 1
            }

            //遮罩执行
            OpacityMask {
                id: mask
                anchors.fill: imageArea
                source: imageArea
                maskSource: selectShader
                antialiasing: true
                smooth: true
            }

        }
    }

    // 收藏图标组件
    Component {
        id: favoriteComponent
        Item {
            anchors.fill: parent

            // 计算图片区域的位置
            Rectangle {
                id: imageArea
                anchors.fill: parent
                width: parent.width - 14
                height: parent.height - 14
                visible: false
            }

            //收藏图标
            ActionButton {
                id: itemFavoriteBtn
                anchors {
                    bottom: imageArea.bottom
                    left: imageArea.left
                    leftMargin : (imageArea.width - image.paintedWidth) / 2 + 5
                    bottomMargin : (imageArea.height - image.paintedHeight) / 2 + 5
                }

                hoverEnabled: false  //设置为false，可以解决鼠标移动到图标附近时，图标闪烁问题

                icon {
                    name: bFavorited ? "collected" : "collection2"
                }

                MouseArea {
                    id:mouseAreaFavoriteBtn
                    anchors.fill: itemFavoriteBtn
                    propagateComposedEvents: true

                    onClicked: {
                        var paths = []
                        paths.push(m_url)

                        if (bFavorited) {
                            //取消收藏
                            albumControl.removeFromAlbum(0, paths)
                        } else {
                            //收藏
                            albumControl.insertIntoAlbum(0, paths)
                        }

                        GStatus.bRefreshFavoriteIconFlag = !GStatus.bRefreshFavoriteIconFlag
                        // 若当前视图为我的收藏，需要实时刷新我的收藏列表内容
                        if (GStatus.currentViewType === Album.Types.ViewFavorite && GStatus.currentCustomAlbumUId === 0) {
                            GStatus.sigFlushCustomAlbumView(GStatus.currentCustomAlbumUId)
                        }

                        mouse.accepted = true
                    }
                }
            }
        }
    }

    //剩余天数标签组件
    Component {
        id: remainDaysComponent
        Item {
            anchors.fill: parent

            // 计算图片区域的位置
            Rectangle {
                id: imageArea
                anchors.centerIn: parent
                width: parent.width - 14
                height: parent.height - 14
                visible: false
            }

            VideoLabel {
                id: labelRemainDays
                visible: true
                anchors {
                    bottom: imageArea.bottom
                    left: imageArea.left
                    leftMargin : 9
                    bottomMargin : 5
                }
                opacity: 0.7
                displayStr: remainDays > 1 ? (remainDays + qsTr("days")) : (remainDays + qsTr("day"))
            }
        }
    }

    // 视频时长组件
    Component {
        id: videoTimeComponent
        Item {
            anchors.fill: parent

            // 计算图片区域的位置
            Rectangle {
                id: imageArea
                anchors.centerIn: parent
                width: parent.width - 14
                height: parent.height - 14
                visible: false
            }

            VideoLabel {
                id: videoLabel
                visible: bShowVideoLabel
                anchors {
                    bottom: imageArea.bottom
                    right: imageArea.right
                    rightMargin : 9
                    bottomMargin : 5
                }
                opacity: 0.7
                displayStr: fileControl.isVideo(m_url) ? albumControl.getVideoTime(m_url) : "00:00"

                Connections {
                    target: albumControl
                    function onSigRefreashVideoTime(url, videoTimeStr) {
                        if (url === m_url) {
                            videoLabel.displayStr = videoTimeStr
                        }
                    }
                }
            }
        }
    }
}
