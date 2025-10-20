// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import org.deepin.dtk 1.0

import org.deepin.album 1.0 as Album

import "../Control"
import "../Control/ListView"
import "../"
import "../ThumbnailImageView"

/**
 * @brief 分类详情视图
 * 展示指定分类下的所有图片，参考自定义相册的实现方式
 */
BaseView {
    id: classificationDetailView

    property string classificationName: ""  // 分类名称
    property string className: ""           // 分类的类别名
    property int totalImages: 0             // 总图片数
    property int filterType: 0              // 筛选类型，默认所有
    property string numLabelText: ""        // 总数标签显示内容
    property string selectedText: getSelectedText(selectedPaths)
    property alias selectedPaths: theView.selectedUrls

    /**
     * @brief 设置分类数据
     */
    function setClassificationData(name, className) {
        classificationName = name || ""
        classificationDetailView.className = className || ""        
    }

    /**
     * @brief 刷新分类详情视图内容
     */
    function flushClassificationDetailView() {
        // 如果分类名为空，不执行数据加载
        if (!classificationDetailView.className || classificationDetailView.className === "") {
            return
        }

        // 触发数据加载
        dataModel.className = className
        theView.proxyModel.refresh(filterType)
        GStatus.selectedPaths = theView.selectedUrls
        getNumLabelText()
    }

    /**
     * @brief 清理视图状态数据
     */
    function clearViewState() {
        // 清空数据属性
        classificationName = ""
        className = ""
        totalImages = 0
        numLabelText = ""

        // 清空选中状态
        if (theView && theView.selectedUrls) {
            theView.selectedUrls = []
        }

        // 清空状态栏显示
        GStatus.statusBarNumText = ""
    }

    /**
     * @brief 刷新总数标签
     */
    function getNumLabelText() {
        var photoCount = dataModel.rowCount()

        if(photoCount === 0) {
            numLabelText = ""
        } else if(photoCount === 1) {
            numLabelText = qsTr("1 photo")
        } else {
            numLabelText = qsTr("%1 photos").arg(photoCount)
        }

        if (show) {
            GStatus.statusBarNumText = numLabelText
        }
    }

    /**
     * @brief 刷新选中项目标签内容
     */
    function getSelectedText(paths) {
        if (!show)
            return "";

        var selectedNumText = GStatus.getSelectedNumText(paths, numLabelText)
        if (show)
            GStatus.statusBarNumText = selectedNumText
        return selectedNumText
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        if (show)
            flushClassificationDetailView()
    }

    onShowChanged: {
        if (show && classificationDetailView.className !== "") {
            flushClassificationDetailView()
            showAnimation.start()
        } else if (!show) {
            // 视图隐藏时清理状态数据，避免下次显示时出现异常
            clearViewState()
        }
    }

    // 分类详情标题栏区域
    Item {
        id: classificationDetailTitleRect
        width: parent.width - GStatus.verticalScrollBarWidth
        height: GStatus.thumbnailViewTitleHieght - 10
        visible: classificationDetailView.className !== ""

        // 返回按钮
        IconButton {
            id: backButton
            anchors {
                left: parent.left
                leftMargin: 20
                top: parent.top
                topMargin: 12
            }
            width: 30
            height: 30
            icon.name: "go-previous"
            icon.width: 16
            icon.height: 16

            onClicked: {
                // 返回分类视图
                GStatus.currentViewType = Album.Types.ViewClassification
            }
        }

        // 分类名称标签
        Label {
            id: classificationLabel
            anchors {
                top: parent.top
                topMargin: 12
                left: backButton.right
                leftMargin: 8
            }
            height: 30
            font: DTK.fontManager.t3
            text: qsTr(classificationName)
        }

        Label {
            id: classificationNumLabel
            anchors {
                top: classificationLabel.bottom
                topMargin: 10
                left: backButton.right
                leftMargin: 8
            }
            font: DTK.fontManager.t6
            text: numLabelText !== "" ? numLabelText : qsTr("0 item")
        }

        MouseArea {
            anchors.fill: parent
            onPressed: (mouse)=> {
                theView.selectAll(false)
                mouse.accepted = false
            }
        }
    }

    // 缩略图列表控件
    ThumbnailListViewAlbum {
        id: theView
        anchors {
            top: classificationDetailTitleRect.bottom
            topMargin: 10
        }
        width: parent.width
        height: parent.height - classificationDetailTitleRect.height - m_topMargin
        thumnailListType: Album.Types.ThumbnailNormal
        proxyModel.sourceModel: Album.ImageDataModel {
            id: dataModel;
            modelType: Album.Types.ClassificationDetail
        }

        visible: numLabelText !== "" && classificationDetailView.className !== ""
        property int m_topMargin: 10
    }

    // 监听选中路径变化，更新全局选中状态
    onSelectedPathsChanged: {
        if (show) {
            GStatus.selectedPaths = selectedPaths
        }
    }

    // 筛选无内容时，显示无结果
    Label {
        anchors {
            top: classificationDetailTitleRect.bottom
            left: parent.left
            bottom: theView.bottom
            right: parent.right
            centerIn: parent
        }
        visible: numLabelText === "" && filterType > 0
        font: DTK.fontManager.t4
        color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        text: qsTr("No results")
    }

    // 无内容时显示
    Label {
        anchors {
            top: classificationDetailTitleRect.bottom
            left: parent.left
            bottom: theView.bottom
            right: parent.right
            centerIn: parent
        }
        visible: numLabelText === "" && filterType === 0
        font: DTK.fontManager.t4
        color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        text: qsTr("No photos in this classification")
    }

    NumberAnimation {
        id: showAnimation
        target: theView
        property: "anchors.topMargin"
        from: 10 + theView.height
        to: 10
        duration: GStatus.sidebarAnimationEnabled ? GStatus.animationDuration : 0
        easing.type: Easing.OutExpo
    }
}
