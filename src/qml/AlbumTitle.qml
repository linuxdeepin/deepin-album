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

import "./Control"

TitleBar {
    id : title

    signal sigDeleteClicked()

    anchors {
        top : window.top
        left : parent.left
        leftMargin: 0
    }
    width:  window.width
    height: 50

    background: Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: GStatus.sideBarIsVisible ? 200 : 0
        width: parent.width - (GStatus.sideBarIsVisible ? 200 : 0)
        height: parent.height

        color: DTK.themeType === ApplicationHelper.LightType ? "#FDFDFD"
                                                             : "#202020"
    }

    signal collectionBtnClicked(int nIndex)
    signal showHideSideBar(bool bShow)
    signal showNewAlbumDialog()

    property int minSearchEditWidth : 100 //搜索框最小尺寸
    property int normalSearchEditWidth : 240 //搜索框最大尺寸
    property int iconSize: 36 //图片操作按钮尺寸
    property int iconSpacing: 5 //图片操作按钮间隙
    property int showCollComboWidth: 884 //需要显示年月日下拉框时，主界面宽度
    property int layoutLeftMargin_AlignLeft: showHideleftSidebarButton.x + showHideleftSidebarButton.width // 显示比例按钮向标题左侧对齐时的布局留白宽度
    property int layoutLeftMargin_AlignRight: GStatus.sideBarWidth + 10 // 显示比例按钮向标题右侧对齐时的布局留白宽度

    property int lastWidth: 0
    onWidthChanged: {

        // 合集视图下，宽度变化，控制年月日控件显示类型
        if (GStatus.currentViewType === Album.Types.ViewCollecttion) {
            if (albumControl.getYears().length !== 0) {
                collectionBtnBox.refreshVisilbe = !collectionBtnBox.refreshVisilbe
                collectionCombo.refreshVisible = !collectionCombo.refreshVisible
            }
        }

        // 标题栏缩放比例按钮跟随窗口尺寸左对齐/右对齐
        if (width <= GStatus.needHideSideBarWidth) {
            if (rightLayout.anchors.leftMargin === layoutLeftMargin_AlignRight && (lastWidth > width)) {
                layoutMoveToLeftAnimation.start()
            }
        } else {
            if (rightLayout.anchors.leftMargin === layoutLeftMargin_AlignLeft && (lastWidth < width)) {
                if (GStatus.loading)
                    rightLayout.anchors.leftMargin = layoutLeftMargin_AlignRight
                else
                    layoutMoveToRightAnimation.start()
            }
        }

        lastWidth = width
    }

    // 显示比例按钮向左对齐动画
    NumberAnimation {
        id: layoutMoveToRightAnimation
        target: rightLayout
        from: layoutLeftMargin_AlignLeft
        to: layoutLeftMargin_AlignRight
        property: "anchors.leftMargin"
        duration: 200
        easing.type: Easing.InOutQuad
    }

    // 显示比例按钮向右对齐动画
    NumberAnimation {
        id: layoutMoveToLeftAnimation
        target: rightLayout
        from: layoutLeftMargin_AlignRight
        to: layoutLeftMargin_AlignLeft
        property: "anchors.leftMargin"
        duration: 200
        easing.type: Easing.InOutQuad
    }



    menu: Menu {
        x: 0; y: 0
        Action {
            id: equalizerControl
            text: qsTr("New album")
            onTriggered: {
                showNewAlbumDialog()
            }
        }
        MenuItem {
            id: settingsControl
            text: qsTr("Import folders")
            onTriggered: {
                albumControl.createNewCustomAutoImportAlbum()
            }
        }
        MenuSeparator { }
        ThemeMenu { }
        MenuSeparator { }
        HelpAction { }
        AboutAction {
            aboutDialog: AboutDialog {
                width: 360
                height: 362
                productName: qsTr("Album")
                productIcon: "deepin-album"
                version: qsTr("Version:") + Qt.application.version
                description: qsTr("Album is a stylish management tool for viewing and organizing photos and videos.")
                websiteName: DTK.deepinWebsiteName
                websiteLink: DTK.deepinWebsiteLink
                license: qsTr("%1 is released under %2").arg(productName).arg("GPLV3")
            }
        }
        QuitAction { }
    }
    ActionButton {
        id: appTitleIcon
        anchors {
            top: parent.top
            topMargin: 0
            left: parent.left
            leftMargin: 0
        }
        width :  50
        height : 50
        icon {
            name: "deepin-album"
            width: 36
            height: 36
        }
    }

    ToolButton {
        id: showHideleftSidebarButton
        anchors {
            top: parent.top
            topMargin: 7
            left: appTitleIcon.right
            leftMargin: 0
        }
        width : iconSize
        height : iconSize
        icon {
            name: "topleft"
            width: iconSize
            height: iconSize
        }
        ToolTip.visible: hovered
        ToolTip.text: GStatus.sideBarX !== 0 ? qsTr("Show side pane") : qsTr("Hide side pane")
        onClicked :{
            if(GStatus.sideBarX !== 0 ){
                if (title.width < GStatus.needHideSideBarWidth) {
                    window.setWidth(GStatus.needHideSideBarWidth + 1)
                }
                showHideSideBar(true)
                //showSliderAnimation.start()
                layoutMoveToRightAnimation.start()
            }
            else{
                showHideSideBar(false)
                //hideSliderAnimation.start()
                layoutMoveToLeftAnimation.start()
            }
        }
    }

    // 显示比例按钮+年月日控件+搜索框+图片操作按钮+退出按钮整体布局容器
    RowLayout {
        id: rightLayout
        anchors {
            fill: parent
            left: parent.left
            right: parent.right
            leftMargin: layoutLeftMargin_AlignLeft
            rightMargin: 4 * parent.height
        }

        spacing: 10

        // 比例+年月日按钮组/下拉框
        RowLayout {
            spacing: 5

            // 比例按钮
            ToolButton {
                id: range1Button
                Layout.preferredWidth: iconSize
                Layout.preferredHeight: iconSize
                enabled: GStatus.statusBarNumText !== "" && !(GStatus.currentViewType === Album.Types.ViewCollecttion &&  collectionCombo.currentIndex === 0)
                ToolTip.visible: hovered
                ToolTip.text: icon.name === "range1" ? qsTr("Original ratio") : qsTr("Square thumbnails")
                icon {
                    name: asynImageProvider.getLoadMode() == 0 ? "range1" : "range2"
                    width: iconSize
                    height: iconSize
                }
                onClicked: {
                    //1.图片推送器切换
                    asynImageProvider.switchLoadMode()
                    imageDataService.switchLoadMode()

                    //切换图标
                    if(icon.name === "range1"){
                        icon.name = "range2"
                    }else{
                        icon.name = "range1"
                    }

                    //2.发送全局信号，所有的缩略图强制刷新
                    GStatus.sigThumbnailStateChange()
                }
            }

            // 年月日按钮组
            ButtonBox {
                id: collectionBtnBox
                Layout.preferredHeight: iconSize
                property bool refreshVisilbe: false
                visible: GStatus.currentViewType === Album.Types.ViewCollecttion && albumControl.getYears(refreshVisilbe).length !== 0 && window.width > showCollComboWidth

                ToolButton {
                    id:yButton
                    Layout.preferredHeight: parent.height
                    checkable: true
                    text: qsTr("Y")
                    checked: true
                    onClicked: {
                        collectionBtnClicked(0)
                        collectionCombo.setCurrentIndex()
                    }
                }
                ToolButton {
                    id:mButton
                    Layout.preferredHeight: parent.height
                    checkable: true
                    text: qsTr("M")
                    onClicked: {
                        collectionBtnClicked(1)
                        collectionCombo.setCurrentIndex(1)
                    }
                }
                ToolButton {
                    id:dButton
                    Layout.preferredHeight: parent.height
                    checkable: true
                    text: qsTr("D")
                    onClicked: {
                        collectionBtnClicked(2)
                        collectionCombo.setCurrentIndex(2)
                    }
                }
                ToolButton {
                    id:allButton
                    Layout.preferredHeight: parent.height
                    checkable: true
                    checked: true
                    text: qsTr("All")
                    onClicked: {
                        collectionBtnClicked(3)
                        collectionCombo.setCurrentIndex(3)
                    }
                }

                function setChecked(index) {
                    switch (index) {
                    case 0:
                        yButton.checked = true
                        break
                    case 1:
                        mButton.checked = true
                        break
                    case 2:
                        dButton.checked = true
                        break
                    case 3:
                        allButton.checked = true
                        break
                    }
                }

                Component.onCompleted: {
                    GStatus.sigCollectionViewIndexChanged.connect(setChecked)
                }
            }

            // 年月日下拉框
            ComboBox {
                id: collectionCombo
                Layout.minimumWidth: 100
                Layout.maximumWidth: 120
                Layout.fillWidth: true
                textRole: "text"
                iconNameRole: "icon"
                currentIndex: 3
                flat: false
                visible: GStatus.currentViewType === Album.Types.ViewCollecttion && albumControl.getYears(refreshVisible).length !== 0 && window.width <= showCollComboWidth

                property bool refreshVisible: false

                model: ListModel {
                    ListElement { text: qsTr("Y"); icon: "" }
                    ListElement { text: qsTr("M"); icon: "" }
                    ListElement { text: qsTr("D"); icon: "" }
                    ListElement { text: qsTr("All"); icon: "" }
                }

                function setCurrentIndex(index) {
                    collectionCombo.currentIndex = index
                }

                onCurrentIndexChanged: {
                    collectionBtnClicked(currentIndex)
                    collectionBtnBox.setChecked(currentIndex)
                }

                Component.onCompleted: {
                    GStatus.sigCollectionViewIndexChanged.connect(setCurrentIndex)
                }
            }
        }

        SearchEdit{
            id: searchEdit
            Layout.minimumWidth: minSearchEditWidth
            Layout.maximumWidth: normalSearchEditWidth
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignCenter
            placeholder: qsTr("Search")
            text: GStatus.searchEditText

            property string searchKey: ""
            property int   beforeView: -1

            //先用这个顶上吧，以前的returnPressed不支持
            onAccepted: {
                executeSearch(false)
            }

            function executeSearch(bForce) {
                if(GStatus.currentViewType !== Album.Types.ViewSearchResult) {
                    beforeView = GStatus.currentViewType
                }

                //判重
                if(text == searchKey && GStatus.currentViewType === Album.Types.ViewSearchResult && !bForce) {
                    return
                }
                searchKey = text

                //空白的时候执行退出
                if(text == "") {
                    GStatus.currentViewType = beforeView
                    return
                }

                //执行搜索并切出画面
                //1.搜索
                var UID = -1
                switch(beforeView) {
                default: //搜索全库
                    break
                case 4: //搜索我的收藏
                    UID = 0
                    break
                case 5: //搜索最近删除
                    UID = -2
                    break
                case 6: //搜索指定相册
                    UID = GStatus.currentCustomAlbumUId
                    break;
                }
                GStatus.sigRunSearch(UID, text)

                //2.切出画面
                GStatus.currentViewType = Album.Types.ViewSearchResult
            }
        }

        RowLayout {
            spacing: iconSpacing
            Layout.alignment: Qt.AlignRight
            Layout.minimumWidth: iconSize * 3 + iconSpacing * (3 - 1)
            ToolButton {
                visible: GStatus.selectedPaths.length === 0 || GStatus.currentViewType === Album.Types.ViewDevice
                id: titleImportBtn
                Layout.preferredWidth: iconSize
                Layout.preferredHeight: iconSize
                Layout.alignment: Qt.AlignRight
                ToolTip.delay: 500
                ToolTip.timeout: 5000
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Import Photos and Videos")
                icon {
                    name: "import"
                    width: iconSize
                    height: iconSize
                }
                onClicked :{
                    importDialog.open()
                }

                Shortcut {
                    enabled: GStatus.stackControlCurrent === 0
                    autoRepeat: false
                    sequence : "Ctrl+O"
                    onActivated : {
                        importDialog.open()
                    }
                }
            }
            ToolButton {
                id: titleCollectionBtn
                property bool canFavorite: albumControl.canFavorite(GStatus.selectedPaths,GStatus.bRefreshFavoriteIconFlag)
                visible: !titleImportBtn.visible && GStatus.currentViewType !== Album.Types.ViewDevice
                Layout.preferredWidth: iconSize
                Layout.preferredHeight: iconSize
                ToolTip.delay: 500
                ToolTip.timeout: 5000
                ToolTip.visible: hovered
                ToolTip.text: canFavorite ? qsTr("Favorite") : qsTr("Unfavorite")
                DciIcon.mode: DTK.HoveredState
                icon {
                    name: canFavorite ? "toolbar-collection" : "toolbar-collection2"
                    width: iconSize
                    height: iconSize
                }
                onClicked: {
                    if (canFavorite)
                        albumControl.insertIntoAlbum(0, GStatus.selectedPaths)
                    else {
                        albumControl.removeFromAlbum(0, GStatus.selectedPaths)
                        // 当前处于我的收藏视图，点击图片操作-取消收藏，需要重载我的收藏列表内容
                        if (GStatus.currentViewType === Album.Types.ViewFavorite && GStatus.currentCustomAlbumUId === 0) {
                            GStatus.sigFlushCustomAlbumView(GStatus.currentCustomAlbumUId)
                        }
                    }

                    GStatus.bRefreshFavoriteIconFlag = !GStatus.bRefreshFavoriteIconFlag
                }
            }

            ToolButton {
                id: titleRotateBtn
                visible: (titleImportBtn.visible ? false : true) && GStatus.currentViewType !== Album.Types.ViewDevice
                enabled: fileControl.isRotatable(GStatus.selectedPaths)
                ColorSelector.disabled: !fileControl.isRotatable(GStatus.selectedPaths)
                Layout.preferredWidth: iconSize
                Layout.preferredHeight: iconSize
                ToolTip.delay: 500
                ToolTip.timeout: 5000
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Rotate")
                icon {
                    name: "felete"
                    width: iconSize
                    height: iconSize
                }
                onClicked: {
                    fileControl.rotateFile(GStatus.selectedPaths, -90)
                }
            }
            ToolButton {
                id: titleTrashBtn
                visible: (titleImportBtn.visible ? false : true) && GStatus.currentViewType !== Album.Types.ViewDevice
                enabled: fileControl.isCanDelete(GStatus.selectedPaths)
                ColorSelector.disabled: !fileControl.isCanDelete(GStatus.selectedPaths)
                Layout.preferredWidth: iconSize
                Layout.preferredHeight: iconSize
                ToolTip.delay: 500
                ToolTip.timeout: 5000
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Delete")
                icon {
                    name: "delete"
                    width: iconSize
                    height: iconSize
                }
                onClicked: {
                    sigDeleteClicked()
                }
            }
        }

        Component.onCompleted: {
            rightLayout.anchors.leftMargin = title.width <= GStatus.needHideSideBarWidth ? layoutLeftMargin_AlignLeft : layoutLeftMargin_AlignRight
        }
    }


    Connections {
        target: GStatus
        onSigFlushSearchView: {
            searchEdit.executeSearch(true)
        }
    }

    Connections {
        target: albumControl
        onSigRefreshAllCollection: {
            if (GStatus.currentViewType === Album.Types.ViewCollecttion) {
                collectionBtnBox.refreshVisilbe = !collectionBtnBox.refreshVisilbe
            }
        }
    }
}
