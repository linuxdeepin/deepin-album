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
import "./"

Rectangle {
    color: Qt.rgba(0,0,0,0)
    //注意：在model里面加进去的变量，这边可以直接进行使用，只是部分位置不好拿到，需要使用变量
    property string m_index
    property string m_displayFlushHelper
    property QtObject modelData

    property int index: model.index
    property bool blank: model.blank
    property bool bSelected: model.selected !== undefined ? model.selected : false
    property bool bHovered: false //属性：是否hover
    property bool bFavorited: albumControl.photoHaveFavorited(model.url, global.bRefreshFavoriteIconFlag)
    property Item imageItem: image
    property Item favoriteBtn: null

    onBFavoritedChanged: {
        if (bFavorited) {
            if (favoriteBtn == null) {
                favoriteBtn = favoriteComponent.createObject(buttons)
            }
        } else {
            if (favoriteBtn && !bHovered) {
                favoriteBtn.destroy()
                favoriteBtn = null
            }
        }
    }

    //选中后显示的阴影框
    Rectangle {
        id: selectShader
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        radius: 10
        color: "#AAAAAA"
        visible: bSelected
        opacity: 0.4
    }

    Album.QImageItem {
        id: image
        anchors.centerIn: parent
        width: parent.width - 14
        height: parent.height -14
        smooth: true
        image: {
            gridView.bRefresh
            modelData.thumbnail
        }
        fillMode: Album.QImageItem.PreserveAspectFit
        visible: false
    }

    Loader {
        id: damageIconLoader
        anchors.centerIn: parent

        active: image.null
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
        onCallSavePicDone: {
            if (path === model.url) {
                model.reloadThumbnail
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

    //遮罩执行
    OpacityMask {
        id: mask
        anchors.fill: image
        source: image
        maskSource: maskRec
    }

    // 边框阴影立体效果
    DropShadow {
        anchors.fill: mask
        z: 0

        verticalOffset: 1

        radius: 5
        samples: radius * 2 + 1
        spread: 0.3

        color: "black"

        opacity: 0.3

        source: mask

        visible: true
    }

    MouseArea {
        id:mouseAreaTopParentRect
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true

        onClicked: {
            //允许鼠标事件传递给子控件处理,否则鼠标点击缩略图收藏图标不能正常工作
            mouse.accepted = false
        }

        onEntered: {
            if (model.blank)
                return;

            bHovered = true
            if (favoriteBtn == null && model.modelType !== Album.Types.Device && model.modelType !== Album.Types.RecentlyDeleted)
                favoriteBtn = favoriteComponent.createObject(buttons)
        }

        onExited: {
            bHovered = false
            if (favoriteBtn && !bFavorited) {
                favoriteBtn.destroy()
                favoriteBtn = null
            }
        }
    }

    Component {
        id: favoriteComponent
        //收藏图标
        ActionButton {
            id: itemFavoriteBtn
            hoverEnabled: false  //设置为false，可以解决鼠标移动到图标附近时，图标闪烁问题
            enabled: !image.null
            icon {
                name: bFavorited ? "collected" : "collection2"
            }

            MouseArea {
                id:mouseAreaFavoriteBtn
                anchors.fill: itemFavoriteBtn
                propagateComposedEvents: true

                onClicked: {
                    var paths = []
                    paths.push(modelData.url)

                    if (bFavorited) {
                        //取消收藏
                        albumControl.removeFromAlbum(0, paths)
                    } else {
                        //收藏
                        albumControl.insertIntoAlbum(0, paths)
                    }

                    global.bRefreshFavoriteIconFlag = !global.bRefreshFavoriteIconFlag
                    // 若当前视图为我的收藏，需要实时刷新我的收藏列表内容
                    if (global.currentViewIndex === GlobalVar.ThumbnailViewType.Favorite && global.currentCustomAlbumUId === 0) {
                        global.sigFlushCustomAlbumView(global.currentCustomAlbumUId)
                    }

                    mouse.accepted = true
                }

            }
        }
    }

    Column {
        id: buttons

        visible: true

        anchors {
            bottom: image.bottom
            left: image.left
            leftMargin : (image.width - image.paintedWidth) / 2 + 5
            bottomMargin : (image.height - image.paintedHeight) / 2 + 5
        }
    }

    //选中后显示的图标
    DciIcon {
        name: "select_active_1"
        visible: selectShader.visible
        anchors.top: image.top
        anchors.right: image.right
        anchors.topMargin: 5
        anchors.rightMargin : 5
    }

    DciIcon {
        name: "Inner_shadow"
        visible: selectShader.visible
        anchors.top: image.top
        anchors.right: image.right
        anchors.topMargin: 5
        anchors.rightMargin : 5
    }

    DciIcon {
        name: "shadow"
        visible: selectShader.visible
        anchors.top: image.top
        anchors.right: image.right
        anchors.topMargin: 5
        anchors.rightMargin : 5
    }

    DciIcon {
        name: "yes"
        visible: selectShader.visible
        anchors.top: image.top
        anchors.right: image.right
        anchors.topMargin: 5
        anchors.rightMargin : 5
    }

    //剩余天数标签
    VideoLabel {
        id: labelRemainDays
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.RecentlyDeleted && !model.blank
        anchors.bottom: image.bottom
        anchors.left: image.left
        anchors.leftMargin : 5
        anchors.bottomMargin : 5
        opacity: 0.7
        displayStr: model.remainDays > 1 ? (model.remainDays + qsTr("days")) : (model.remainDays + qsTr("day"))
        height: 22
        width: 44
    }

    //视频时长标签
    VideoLabel {
        id: videoLabel
        visible: fileControl.isVideo(model.url) && !model.blank
        anchors.bottom: image.bottom
        anchors.right: image.right
        anchors.rightMargin : 5
        anchors.bottomMargin : 5
        opacity: 0.7
        displayStr: fileControl.isVideo(model.url) ? albumControl.getVideoTime(model.url) : "00:00"
        height: 22
        width: displayStr.length === 5 ? 44 : 64

        Connections {
            target: albumControl
            onSigRefreashVideoTime: {
                if (url === model.url) {
                    videoLabel.displayStr = videoTimeStr
                }
            }
        }
    }
}
