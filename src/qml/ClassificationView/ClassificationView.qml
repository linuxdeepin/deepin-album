// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import org.deepin.dtk 1.0

import org.deepin.album 1.0 as Album

import "../Control"

/**
 * @brief 分类视图
 * 以网格形式显示图片分类结果，支持延迟加载
 */
BaseView {
    id: classificationView

    //property bool dataLoaded: false

    // 信号：要求显示分类详情
    signal showClassificationDetail(string name, string className)

    // 分类数据模型 - 参考现有的侧边栏模式使用ListModel
    ListModel {
        id: classificationModel
    }

    onVisibleChanged: {
        if (visible) {
            loadClassificationData()
        }
    }

    // 监听分类完成信号，刷新数据
    Connections {
        target: albumControl
        function onSigImageClassifyFinished() {
            loadClassificationData()
        }
    }

    /**
     * @brief 加载分类数据
     */
    function loadClassificationData() {
        var rawData = albumControl.getClassificationData()

        // 清空现有模型
        classificationModel.clear()

        // 按照现有架构模式，将C++返回的QVariantList转换为ListModel
        for (var i = 0; i < rawData.length; i++) {
            var item = rawData[i]
            // 参考侧边栏模式，创建标准化的数据项
            classificationModel.append({
                "name": item.className || "",
                "count": item.count || 0,
                "className": item.className || "",
                "thumbnail": item.thumbnail || "",
            })
        }
    }

    // 标题区域
    Item {
        id: classificationTitleRect
        width: parent.width
        height: 80
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

        Label {
            id: titleLabel
            anchors.centerIn: parent
            font: DTK.fontManager.t3
            text: qsTr("Image Classification")
            color: DTK.themeType === ApplicationHelper.DarkType ? "#FFFFFF" : "#000000"
        }
    }

    ScrollView {
        id: scrollView
        anchors {
            top: classificationTitleRect.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        clip: true

        GridView {
            id: classificationGrid
            anchors.fill: parent
            cellWidth: 280
            cellHeight: 200
            model: classificationModel  // 使用ListModel而不是QVariantList

            delegate: ClassificationItem {
                width: classificationGrid.cellWidth - 10
                height: classificationGrid.cellHeight - 10

                classificationName: model.name || ""
                imageCount: model.count || 0
                thumbnailPath: model.thumbnail || ""

                onClicked: {
                    // 获取完整的分类数据
                    var classificationData = classificationModel.get(index)
                    // 发射信号，请求显示分类详情
                    classificationView.showClassificationDetail(
                        classificationData.name || "",
                        classificationData.className || ""
                    )
                }
            }
        }
    }
}
