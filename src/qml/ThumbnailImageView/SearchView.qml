// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import org.deepin.dtk 1.0

import org.deepin.album 1.0 as Album

import "../Control"
import "../Control/ListView"

Rectangle {
    width: parent.width
    height: parent.height

    property string currentKeyword: ""
    property var searchResults: new Array
    property string numLabelText: ""
    property string selectedText: getSelectedText(selectedPaths)
    property alias selectedPaths: view.selectedUrls

    onVisibleChanged: {
        if (visible) {
            global.statusBarNumText = searchResultLabel.text
        }
    }

    function searchFromKeyword(UID, keyword) {

        currentKeyword = keyword

        //1.调起C++，执行搜索，将结果刷入UI
        dataModel.albumId = UID
        dataModel.keyWord = keyword
        view.proxyModel.refresh()

        searchResults = view.allUrls()

        getStatusBarText()
    }

    function getStatusBarText() {
        var haveVideo = fileControl.haveVideo(searchResults)
        var havePicture = fileControl.haveImage(searchResults)

        if(havePicture && !haveVideo) {
            if(searchResults.length === 1) {
                searchResultLabel.text = qsTr("1 photo found")
            } else {
                searchResultLabel.text = qsTr("%1 photos found").arg(searchResults.length)
            }
        } else if(haveVideo && !havePicture) {
            if(searchResults.length === 1) {
                searchResultLabel.text = qsTr("1 video found")
            } else {
                searchResultLabel.text = qsTr("%1 videos found").arg(searchResults.length)
            }
        } else if (searchResults.length === 0) {
            searchResultLabel.text = ""
        } else {
            searchResultLabel.text = qsTr("%1 items found").arg(searchResults.length)
        }


        numLabelText = searchResultLabel.text

        if (visible) {
            global.statusBarNumText = numLabelText
        }

        global.selectedPaths = []
    }

    // 刷新选中项目标签内容
    function getSelectedText(paths) {
        var selectedNumText = global.getSelectedNumText(paths, numLabelText)

        if (visible)
            global.statusBarNumText = selectedNumText
        return selectedNumText
    }

    //有结果展示
    Rectangle {
        id: resultViewTitleRect

        width: parent.width - global.verticalScrollBarWidth
        height: global.thumbnailViewTitleHieght
	color: Qt.rgba(0,0,0,0)

        //搜索标题
        Label {
            id: searchTitle
            text: qsTr("Search results")
            anchors.top: parent.top
            anchors.left: parent.left
            font: DTK.fontManager.t3
            color: Qt.rgba(0,0,0)
        }

        //幻灯片放映按钮
        RecommandButton {
            id: sliderPlayButton
            visible: searchResults.length > 0
            text: qsTr("Slide Show")
            icon {
                name:"Combined_Shape"
                width: 20
                height: 20
            }
            enabled: fileControl.haveImage(searchResults)
            anchors.top: searchTitle.bottom
            anchors.left: searchTitle.left
            anchors.topMargin: 15

            onClicked: {
                stackControl.startMainSliderShow(searchResults, 0)
            }
        }

        //搜索结果label
        Label {
            id: searchResultLabel
            visible: searchResults.length > 0
            anchors.left: sliderPlayButton.right
            anchors.verticalCenter: sliderPlayButton.verticalCenter
            font: DTK.fontManager.t6
            anchors.leftMargin: 10
        }

        MouseArea {
            anchors.fill: parent
            onPressed: {
                view.selectAll(false)
                mouse.accepted = false
            }
        }
    }

    //缩略图视图
    ThumbnailListView2 {
        id: view
        anchors.top: resultViewTitleRect.bottom
        anchors.topMargin: 10
        width: parent.width
        height: parent.height - resultViewTitleRect.height - m_topMargin - statusBar.height

        proxyModel.sourceModel: Album.ImageDataModel { id: dataModel; modelType: Album.Types.SearchResult}

        visible: numLabelText !== ""
        property int m_topMargin: 10

        // 监听缩略图列表选中状态，一旦改变，更新globalVar所有选中路径
        Connections {
            target: view
            onSelectedChanged: {
                if (parent.visible)
                    global.selectedPaths = view.selectedUrls
            }
        }
    }

    //无结果展示
    Rectangle {
        id: noResultView
        visible: searchResults.length === 0
        anchors.top: parent.top
        anchors.topMargin: searchTitle.height
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        color: Qt.rgba(0,0,0,0)

        Label {
            id: noResultText
            text: qsTr("No search results")
            font: DTK.fontManager.t4
            anchors.centerIn: parent
            color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        }
    }

    Component.onCompleted: {
        global.sigRunSearch.connect(searchFromKeyword)
    }
}
