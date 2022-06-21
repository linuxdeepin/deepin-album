import QtQuick 2.0
import org.deepin.dtk 1.0
import "../../Control"
import "../../Control/ListView"
Rectangle {
    width: parent.width
    height: parent.height

    property int customAlbumUId: global.currentCustomAlbumUId
    property int filterType: filterCombo.currentIndex // 筛选类型，默认所有
    property string numLabelText: "" //总数标签显示内容
    property string selectedText: getSelectedText(selectedPaths)
    property alias selectedPaths: theView.selectedPaths

    onVisibleChanged: {
        if (visible)
            flushCustomAlbumView()
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        flushCustomAlbumView()
    }

    // 我的收藏和相册视图之间切换，需要重载数据
    onCustomAlbumUIdChanged: {
        flushCustomAlbumView()
    }

    // 刷新自定义相册/我的收藏视图内容
    function flushCustomAlbumView() {
        loadCustomAlbumItems()
        global.selectedPaths = theView.selectedPaths
        getNumLabelText()
    }

    // 刷新总数标签
    function getNumLabelText() {
        var photoCountText = albumControl.getCustomAlbumInfoConut(customAlbumUId, 1) > 0 ? qsTr("%1 photos").arg(albumControl.getCustomAlbumInfoConut(customAlbumUId, 1)) : ""
        var videoCountText = albumControl.getCustomAlbumInfoConut(customAlbumUId, 2) > 0 ? qsTr("%1 videos").arg(albumControl.getCustomAlbumInfoConut(customAlbumUId, 2)) : ""
        numLabelText = filterType == 0 ? (photoCountText + (videoCountText !== "" ? (" " + videoCountText) : ""))
                                           : (filterType == 1 ? photoCountText : videoCountText)
        if (visible) {
            global.statusBarNumText = numLabelText
        }
    }

    // 刷新选中项目标签内容
    function getSelectedText(paths) {
        var photoCount = fileControl.photoCount(paths)
        var videoCount = fileControl.videoCount(paths)
        var selectedNumText = ""
        if (photoCount && videoCount)
            selectedNumText = qsTr("%1 items selected").arg(photoCount + videoCount)
        else if (photoCount && videoCount === 0)
            selectedNumText = qsTr("%1 photos selected").arg(photoCount)
        else if (photoCount === 0 && videoCount)
            selectedNumText = qsTr("%1 videos selected").arg(videoCount)
        else
            selectedNumText = ""

        if (selectedNumText === "") {
            selectedNumText = numLabelText
        }

        if (visible)
            global.statusBarNumText = selectedNumText
        return selectedNumText
    }

    // 加载自定义相册数据
    function loadCustomAlbumItems()
    {
        console.info("custom album model has refreshed... filterType:", filterType)
        theView.selectAll(false)
        theView.thumbnailListModel.clear();
        var customAlbumInfos = albumControl.getAlbumInfos(customAlbumUId, filterType);
        console.info("custom album model has refreshed... filterType:", filterType, " done...")
        for (var key in customAlbumInfos) {
            var customAlbumItems = customAlbumInfos[key]
            for (var i = 0; i < customAlbumItems.length; i++) {
                theView.thumbnailListModel.append(customAlbumItems[i])
            }
            break;
        }

        return true
    }

    // 自定义相册标题栏区域
    Rectangle {
        id: customAlbumTitleRect
        width: parent.width - global.verticalScrollBarWidth
        height: global.thumbnailViewTitleHieght - 10
        // 相册名称标签
        Label {
            id: customAlbumLabel
            anchors.top: parent.top
            anchors.topMargin: 12
            anchors.left: parent.left
            height: 30
            font: DTK.fontManager.t3
            text: qsTr(albumControl.getCustomAlbumByUid(customAlbumUId))
        }

        Label {
            id: customAlbumNumLabel
            anchors.top: customAlbumLabel.bottom
            anchors.topMargin: 10
            anchors.left: parent.left
            font: DTK.fontManager.t6
            text: numLabelText
        }

        // 筛选下拉框
        FilterComboBox {
            id: filterCombo
            anchors.top: customAlbumLabel.bottom
            anchors.topMargin: 4
            anchors.right: parent.right
            width: 130
            height: 30
            visible: !(numLabelText === "" && filterType === 0)
        }
    }

    // 缩略图列表控件
    ThumbnailListView {
        id: theView
        anchors.top: customAlbumTitleRect.bottom
        anchors.topMargin: 10
        width: parent.width
        height: parent.height - customAlbumTitleRect.height - m_topMargin - statusBar.height
        visible: numLabelText !== ""
        property int m_topMargin: 10

        // 监听缩略图列表选中状态，一旦改变，更新globalVar所有选中路径
        Connections {
            target: theView
            onSelectedChanged: {
                var selectedPaths = []
                selectedPaths = theView.selectedPaths

                if (parent.visible)
                    global.selectedPaths = selectedPaths
            }
        }
    }

    Label {
        anchors.top: customAlbumTitleRect.bottom
        anchors.left: parent.left
        anchors.bottom: theView.bottom
        anchors.right: parent.right
        anchors.centerIn: parent
        visible: numLabelText === "" && filterType > 0
        font: DTK.fontManager.t4
        color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        text: qsTr("No results")
    }

    Component.onCompleted: {
        global.sigFlushCustomAlbumView.connect(flushCustomAlbumView)
    }
}
