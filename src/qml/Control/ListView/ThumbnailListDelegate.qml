import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQml.Models 2.11
import QtQml 2.11
import QtQuick.Shapes 1.10
import org.deepin.dtk 1.0
import QtGraphicalEffects 1.0

import "../"
import "../../"

Rectangle {
    //注意：在model里面加进去的变量，这边可以直接进行使用，只是部分位置不好拿到，需要使用变量
    property string m_index
    property string m_url
    property string m_displayFlushHelper
    property var m_favoriteBtn: itemFavoriteBtn

    //选中后显示的阴影框
    Rectangle {
        id: selectShader
        anchors.centerIn: parent
        width: image.paintedWidth + 14
        height: image.paintedHeight + 14
        radius: 10
        color: "#AAAAAA"
        visible: theView.ism.indexOf(parent.m_index) !== -1 || global.selectedPaths.indexOf(m_url) !== -1
        opacity: 0.4
    }

    //缩略图本体
    Image {
        id: image
        source: m_url !== "" ? "image://publisher/" + m_displayFlushHelper + theView.displayFlushHelper.toString() + "_" + m_url : ""
        asynchronous: true
        anchors.centerIn: parent
        width: parent.width - 14
        height: parent.height - 14
        //使用PreserveAspectFit确保在原始比例下不变形
        fillMode: Image.PreserveAspectFit
        visible: false
    }

    // 图片保存完成，缩略图区域重新加载当前图片
    Connections {
        target: fileControl
        onCallSavePicDone: {
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

    //遮罩执行
    OpacityMask {
        id: mask
        anchors.fill: image
        source: image
        maskSource: maskRec
    }

    //收藏图标
    ActionButton {
        id: itemFavoriteBtn
        visible: albumControl.photoHaveFavorited(m_url, global.bRefreshFavoriteIconFlag)
        anchors.bottom: image.bottom
        anchors.left: image.left
        anchors.leftMargin : (image.width - image.paintedWidth) / 2 + 5
        anchors.bottomMargin : (image.height - image.paintedHeight) / 2 + 5

        icon {
            name: "collected"
        }

        onClicked: {
            var paths = []
            paths.push(m_url)
            albumControl.removeFromAlbum(0, paths)
            global.bRefreshFavoriteIconFlag = !global.bRefreshFavoriteIconFlag
            // 若当前视图为我的收藏，需要实时刷新我的收藏列表内容
            if (global.currentViewIndex === GlobalVar.ThumbnailViewType.Favorite && global.currentCustomAlbumUId === 0) {
                global.sigFlushCustomAlbumView(global.currentCustomAlbumUId)
            }
        }
    }

    //选中后显示的图标
    DciIcon {
        name: "select_active_1"
        visible: selectShader.visible
        anchors.top: image.top
        anchors.right: image.right
        anchors.topMargin: (image.height - image.paintedHeight) / 2 + 5
        anchors.rightMargin : (image.width - image.paintedWidth) / 2 + 5
    }

    DciIcon {
        name: "Inner_shadow"
        visible: selectShader.visible
        anchors.top: image.top
        anchors.right: image.right
        anchors.topMargin: (image.height - image.paintedHeight) / 2 + 5
        anchors.rightMargin : (image.width - image.paintedWidth) / 2 + 5
    }

    DciIcon {
        name: "shadow"
        visible: selectShader.visible
        anchors.top: image.top
        anchors.right: image.right
        anchors.topMargin: (image.height - image.paintedHeight) / 2 + 5
        anchors.rightMargin : (image.width - image.paintedWidth) / 2 + 5
    }

    DciIcon {
        name: "yes"
        visible: selectShader.visible
        anchors.top: image.top
        anchors.right: image.right
        anchors.topMargin: (image.height - image.paintedHeight) / 2 + 5
        anchors.rightMargin : (image.width - image.paintedWidth) / 2 + 5
    }

    //视频时长标签
    VideoLabel {
        id: videoLabel
        visible: fileControl.isVideo(m_url)
        anchors.bottom: image.bottom
        anchors.right: image.right
        anchors.rightMargin : 5
        anchors.bottomMargin : 5
        opacity: 0.7
        displayStr: fileControl.isVideo(m_url) ? albumControl.getVideoTime(m_url) : "00:00:00"
        Connections {
            target: albumControl
            onSigRefreashVideoTime: {
                if (url === m_url) {
                    videoLabel.displayStr = videoTimeStr
                }
            }
        }
    }
}
