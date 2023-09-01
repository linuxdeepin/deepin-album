// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import org.deepin.dtk 1.0

import org.deepin.album 1.0 as Album

import "../../Control"
import "../../Control/ListView"
import "../../"
import "../"

BaseView {

    property int customAlbumUId: GStatus.currentCustomAlbumUId
    property string customAlbumName: "" //相册名称显示内容
    property int totalPictrueAndVideos: 0
    property int filterType: filterCombo.currentIndex // 筛选类型，默认所有
    property string numLabelText: "" //总数标签显示内容
    property string selectedText: getSelectedText(selectedPaths)
    property alias selectedPaths: theView.selectedUrls
    property bool isCustom : albumControl.isCustomAlbum(customAlbumUId)
    property bool isSystemAutoImport: albumControl.isSystemAutoImportAlbum(customAlbumUId)
    property bool isNormalAutoImport: albumControl.isNormalAutoImportAlbum(customAlbumUId)

    onVisibleChanged: {
        if (visible) {
            flushAlbumName(GStatus.currentCustomAlbumUId, albumControl.getCustomAlbumByUid(GStatus.currentCustomAlbumUId))
            flushCustomAlbumView(GStatus.currentCustomAlbumUId)
        }
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        if (visible)
            flushCustomAlbumView(GStatus.currentCustomAlbumUId)
    }

    // 我的收藏和相册视图之间切换，需要重载数据
    onCustomAlbumUIdChanged: {
        if (visible) {
            flushAlbumName(GStatus.currentCustomAlbumUId, albumControl.getCustomAlbumByUid(GStatus.currentCustomAlbumUId))
            flushCustomAlbumView(GStatus.currentCustomAlbumUId)
        }
    }

    // 刷新自定义相册名称
    function flushAlbumName(UID, name) {
        if (UID === GStatus.currentCustomAlbumUId) {
            customAlbumName = name
        }
    }

    // 刷新自定义相册/我的收藏视图内容
    function flushCustomAlbumView(UID) {
        if (UID === GStatus.currentCustomAlbumUId || UID === -1) {
            dataModel.albumId = customAlbumUId
            theView.proxyModel.refresh(filterType)
            GStatus.selectedPaths = theView.selectedUrls
            getNumLabelText()
        }
    }

    // 刷新总数标签
    function getNumLabelText() {
        //QML的翻译不支持%n的特性，只能拆成这种代码

        var photoCountText = ""
        var photoCount = albumControl.getCustomAlbumInfoConut(customAlbumUId, 1)
        if(photoCount === 0) {
            photoCountText = ""
        } else if(photoCount === 1) {
            photoCountText = qsTr("1 photo")
        } else {
            photoCountText = qsTr("%1 photos").arg(photoCount)
        }

        var videoCountText = ""
        var videoCount = albumControl.getCustomAlbumInfoConut(customAlbumUId, 2)
        if(videoCount === 0) {
            videoCountText = ""
        } else if(videoCount === 1) {
            videoCountText = qsTr("1 video")
        } else {
            videoCountText = qsTr("%1 videos").arg(videoCount)
        }

        numLabelText = filterType == 0 ? (photoCountText + (videoCountText !== "" ? ((photoCountText !== "" ? " " : "") + videoCountText) : ""))
                                           : (filterType == 1 ? photoCountText : videoCountText)
        if (visible) {
            GStatus.statusBarNumText = numLabelText
        }
    }

    // 刷新选中项目标签内容
    function getSelectedText(paths) {
        var selectedNumText = GStatus.getSelectedNumText(paths, numLabelText)
        if (visible)
            GStatus.statusBarNumText = selectedNumText
        return selectedNumText
    }

    Connections {
        target: albumControl
        onSigRepeatUrls: {
            if (visible && GStatus.currentCustomAlbumUId !== 0) {
                theView.selectUrls(urls)
            }
        }
    }

    // 自定义相册标题栏区域
    Item {
        id: customAlbumTitleRect
        width: parent.width - GStatus.verticalScrollBarWidth
        height: GStatus.thumbnailViewTitleHieght - 10
        // 相册名称标签
        Label {
            id: customAlbumLabel
            anchors {
                top: parent.top
                topMargin: 12
                left: parent.left
            }
            height: 30
            font: DTK.fontManager.t3
            text: qsTr(customAlbumName)
        }

        Label {
            id: customAlbumNumLabel
            anchors {
                top: customAlbumLabel.bottom
                topMargin: 10
                left: parent.left
            }
            font: DTK.fontManager.t6
            text: numLabelText
        }

        // 筛选下拉框
        FilterComboBox {
            id: filterCombo
            anchors {
                top: customAlbumLabel.bottom
                topMargin: 4
                right: parent.right
            }
            width: 130
            height: 30
            visible: !(numLabelText === "" && filterType === 0)
        }

        MouseArea {
            anchors.fill: parent
            onPressed: {
                theView.selectAll(false)
                mouse.accepted = false
            }
        }
    }

    // 缩略图列表控件
    ThumbnailListView2 {
        id: theView
        anchors {
            top: customAlbumTitleRect.bottom
            topMargin: 10
        }
        width: parent.width
        height: parent.height - customAlbumTitleRect.height - m_topMargin
        thumnailListType: (GStatus.currentViewType === Album.Types.ViewCustomAlbum && GStatus.currentCustomAlbumUId > 3) ? Album.Types.ThumbnailCustomAlbum
                                                                                                                                    : Album.Types.ThumbnailNormal
        proxyModel.sourceModel: Album.ImageDataModel { id: dataModel; modelType: Album.Types.CustomAlbum}

        visible: numLabelText !== ""
        property int m_topMargin: 10

        // 监听缩略图列表选中状态，一旦改变，更新globalVar所有选中路径
        Connections {
            target: theView
            onSelectedChanged: {
                if (parent.visible)
                    GStatus.selectedPaths = theView.selectedUrls
            }
        }
    }

    // 仅在自动导入相册筛选无内容时，显示无结果
    Label {
        anchors {
            top: customAlbumTitleRect.bottom
            left: parent.left
            bottom: theView.bottom
            right: parent.right
            centerIn: parent
        }
        visible: numLabelText === "" && filterType > 0 && (isSystemAutoImport || isNormalAutoImport)
        font: DTK.fontManager.t4
        color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        text: qsTr("No results")
    }

    // 仅在自动导入相册无内容时，显示没有图片或视频时显示
    Label {
        anchors {
            top: customAlbumTitleRect.bottom
            left: parent.left
            bottom: theView.bottom
            right: parent.right
            centerIn: parent
        }
        visible: numLabelText === "" && filterType === 0 && (isSystemAutoImport || isNormalAutoImport)
        font: DTK.fontManager.t4
        color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        text: qsTr("No photos or videos found")
    }

    // 自定义相册，若没有数据，显示导入图片视图
    ImportView {
        visible: GStatus.currentViewType === Album.Types.ViewCustomAlbum && numLabelText === "" && isCustom
    }

    Component.onCompleted: {
        GStatus.sigFlushCustomAlbumView.connect(flushCustomAlbumView)
        GStatus.sigCustomAlbumNameChaged.connect(flushAlbumName)
    }
}
