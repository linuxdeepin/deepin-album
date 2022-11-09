import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import Qt.labs.folderlistmodel 2.11
import org.deepin.dtk 1.0

import "./Control"

Rectangle {

    anchors.top : root.top
    anchors.left : parent.left
    anchors.leftMargin: 0
    width:  root.width
    height: 50

    property int minSearchEditWidth : 100 //搜索框最小尺寸
    property int normalSearchEditWidth : 240 //搜索框最大尺寸
    property int showCollComboWidth: 884 //需要显示年月日下拉框时，主界面宽度
    property int layoutLeftMargin_AlignLeft: showHideleftSidebarButton.x + showHideleftSidebarButton.width // 显示比例按钮向标题左侧对齐时的布局留白宽度
    property int layoutLeftMargin_AlignRight: leftSidebar.width + 10 // 显示比例按钮向标题右侧对齐时的布局留白宽度

    property int lastWidth: 0
    onWidthChanged: {

        // 合集视图下，宽度变化，控制年月日控件显示类型
        if (global.currentViewIndex === GlobalVar.ThumbnailViewType.Collecttion) {
            if (albumControl.getYears().length !== 0) {
                collectionBtnBox.refreshVisilbe = !collectionBtnBox.refreshVisilbe
                collectionCombo.refreshVisible = !collectionCombo.refreshVisible
            }
        }

        // 标题栏缩放比例按钮跟随窗口尺寸左对齐/右对齐
        if (width <= global.needHideSideBarWidth) {
            if (rightLayout.anchors.leftMargin === layoutLeftMargin_AlignRight && (lastWidth > width)) {
                layoutMoveToLeftAnimation.start()
            }
        } else {
            if (rightLayout.anchors.leftMargin === layoutLeftMargin_AlignLeft && (lastWidth < width)) {
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

    TitleBar {
        id : title
        anchors.fill: parent
        width: parent.width
        menu: Menu {
               x: 0; y: 0
               Action {
                   id: equalizerControl
                   text: qsTr("New album")
                   onTriggered: {
                       var x = parent.mapToGlobal(0, 0).x + parent.width / 2 - 190
                       var y = parent.mapToGlobal(0, 0).y + parent.height / 2 - 89
                       newAlbum.setX(x)
                       newAlbum.setY(y)
                       newAlbum.setNormalEdit()
                       newAlbum.show()

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
                       maximumWidth: 360
                       maximumHeight: 362
                       minimumWidth: 360
                       minimumHeight: 362
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
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
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
            anchors.top: parent.top
            anchors.topMargin: 7
            anchors.left: appTitleIcon.right
            anchors.leftMargin: 0
            width : 36
            height : 36
            icon {
                name: "topleft"
                width: 36
                height: 36
            }
            ToolTip.visible: hovered
            ToolTip.text: leftSidebar.x !== 0 ? qsTr("Show side pane") : qsTr("Hide side pane")
            onClicked :{
                if(leftSidebar.x !== 0 ){
                    if (title.width < global.needHideSideBarWidth) {
                        root.setWidth(global.needHideSideBarWidth + 1)
                    }
                    showSliderAnimation.start()
                    layoutMoveToRightAnimation.start()
                }
                else{
                    hideSliderAnimation.start()
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
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    enabled: global.statusBarNumText !== ""
                    ToolTip.visible: hovered
                    ToolTip.text: icon.name === "range1" ? qsTr("Original ratio") : qsTr("Square thumbnails")
                    icon {
                        name: asynImageProvider.getLoadMode() == 0 ? "range1" : "range2"
                        width: 36
                        height: 36
                    }
                    onClicked: {
                        //1.图片推送器切换
                        asynImageProvider.switchLoadMode()

                        //切换图标
                        if(icon.name === "range1"){
                            icon.name = "range2"
                        }else{
                            icon.name = "range1"
                        }

                        //2.发送全局信号，所有的缩略图强制刷新
                        global.sigThumbnailStateChange()
                    }
                }

                // 年月日按钮组
                ButtonBox {
                    id: collectionBtnBox
                    Layout.preferredHeight: 36
                    property bool refreshVisilbe: false
                    visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.Collecttion && albumControl.getYears(refreshVisilbe).length !== 0 && root.width > showCollComboWidth

                    ToolButton {
                        id:yButton
                        Layout.preferredHeight: parent.height
                        checkable: true
                        text: qsTr("Y")
                        checked: true
                        onClicked: {
                            thumbnailImage.setCollecttionViewIndex(0)
                            collectionCombo.setCurrentIndex(0)
                        }
                    }
                    ToolButton {
                        id:mButton
                        Layout.preferredHeight: parent.height
                        checkable: true
                        text: qsTr("M")
                        onClicked: {
                            thumbnailImage.setCollecttionViewIndex(1)
                            collectionCombo.setCurrentIndex(1)
                        }
                    }
                    ToolButton {
                        id:dButton
                        Layout.preferredHeight: parent.height
                        checkable: true
                        text: qsTr("D")
                        onClicked: {
                            thumbnailImage.setCollecttionViewIndex(2)
                            collectionCombo.setCurrentIndex(2)
                        }
                    }
                    ToolButton {
                        id:allButton
                        Layout.preferredHeight: parent.height
                        checkable: true
                        text: qsTr("All")
                        onClicked: {
                            thumbnailImage.setCollecttionViewIndex(3)
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
                        global.sigCollectionViewIndexChanged.connect(setChecked)
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
                    flat: false
                    visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.Collecttion && albumControl.getYears(refreshVisible).length !== 0 && root.width <= showCollComboWidth

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
                        thumbnailImage.setCollecttionViewIndex(currentIndex)
                        collectionBtnBox.setChecked(currentIndex)
                    }

                    Component.onCompleted: {
                        global.sigCollectionViewIndexChanged.connect(setCurrentIndex)
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
                text: global.searchEditText

                property string searchKey: ""
                property int   beforeView: -1

                //先用这个顶上吧，以前的returnPressed不支持
                onAccepted: {
                    executeSearch(false)
                }

                function executeSearch(bForce) {
                    if(global.currentViewIndex !== GlobalVar.ThumbnailViewType.SearchResult) {
                        beforeView = global.currentViewIndex
                    }

                    //判重
                    if(text == searchKey && global.currentViewIndex === GlobalVar.ThumbnailViewType.SearchResult && !bForce) {
                        return
                    }
                    searchKey = text

                    //空白的时候执行退出
                    if(text == "") {
                        global.currentViewIndex = beforeView
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
                        UID = global.currentCustomAlbumUId
                        break;
                    }
                    global.sigRunSearch(UID, text)

                    //2.切出画面
                    global.currentViewIndex = GlobalVar.ThumbnailViewType.SearchResult
                }
            }

            RowLayout {
                spacing: 5
                Layout.alignment: Qt.AlignRight
                ToolButton {
                    visible: global.selectedPaths.length === 0
                    id: titleImportBtn
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    icon {
                        name: "import"
                        width: 36
                        height: 36
                    }
                    onClicked :{
                        importDialog.open()
                    }

                    Shortcut {
                        enabled: global.stackControlCurrent === 0
                        autoRepeat: false
                        sequence : "Ctrl+O"
                        onActivated : {
                           importDialog.open()
                        }
                    }
                }
                ToolButton {
                    id: titleCollectionBtn
                    property bool canFavorite: albumControl.canFavorite(global.selectedPaths,global.bRefreshFavoriteIconFlag)
                    visible: !titleImportBtn.visible && global.currentViewIndex !== GlobalVar.ThumbnailViewType.Device
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    ToolTip.delay: 500
                    ToolTip.timeout: 5000
                    ToolTip.visible: hovered
                    ToolTip.text: canFavorite ? qsTr("Favorite") : qsTr("Unfavorite")
                    DciIcon.mode: DTK.ControlState.HoveredState
                    icon {
                        name: canFavorite ? "toolbar-collection" : "toolbar-collection2"
                        width: 36
                        height: 36
                    }
                    onClicked: {
                        if (canFavorite)
                            albumControl.insertIntoAlbum(0, global.selectedPaths)
                        else {
                            albumControl.removeFromAlbum(0, global.selectedPaths)
                            // 当前处于我的收藏视图，点击图片操作-取消收藏，需要重载我的收藏列表内容
                            if (global.currentViewIndex === GlobalVar.ThumbnailViewType.Favorite && global.currentCustomAlbumUId === 0) {
                                global.sigFlushCustomAlbumView(global.currentCustomAlbumUId)
                            }
                        }

                        global.bRefreshFavoriteIconFlag = !global.bRefreshFavoriteIconFlag
                    }
                }

                ToolButton {
                    id: titleRotateBtn
                    visible: (titleImportBtn.visible ? false : true)
                    enabled: fileControl.isRotatable(global.selectedPaths)
                    ColorSelector.disabled: !fileControl.isRotatable(global.selectedPaths)
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    ToolTip.delay: 500
                    ToolTip.timeout: 5000
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Rotate")
                    icon {
                        name: "felete"
                        width: 36
                        height: 36
                    }
                    onClicked: {
                        fileControl.rotateFile(global.selectedPaths, -90)
                    }
                }
                ToolButton {
                    id: titleTrashBtn
                    visible: (titleImportBtn.visible ? false : true)
                    enabled: fileControl.isCanDelete(global.selectedPaths)
                    ColorSelector.disabled: !fileControl.isCanDelete(global.selectedPaths)
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 36
                    ToolTip.delay: 500
                    ToolTip.timeout: 5000
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("Delete")
                    icon {
                        name: "delete"
                        width: 36
                        height: 36
                    }
                    onClicked: {
                        deleteDialog.setDisplay(GlobalVar.FileDeleteType.TrashSel, global.selectedPaths.length)
                        deleteDialog.show()
                    }
                }
            }

            Component.onCompleted: {
                rightLayout.anchors.leftMargin = title.width <= global.needHideSideBarWidth ? layoutLeftMargin_AlignLeft : layoutLeftMargin_AlignRight
            }
        }
    }

    Connections {
        target: global
        onSigFlushSearchView: {
            searchEdit.executeSearch(true)
        }
    }

    Connections {
        target: albumControl
        onSigRefreshAllCollection: {
            if (global.currentViewIndex === GlobalVar.ThumbnailViewType.Collecttion) {
                collectionBtnBox.refreshVisilbe = !collectionBtnBox.refreshVisilbe
            }
        }
    }
}
