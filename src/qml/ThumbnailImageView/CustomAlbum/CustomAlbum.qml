import QtQuick 2.0
import org.deepin.dtk 1.0
import "../../Control"
import "../../Control/ListView"
Rectangle {
    width: parent.width
    height: parent.height

    property int customAlbumUId: global.currentCustomAlbumUId
    property int filterType: filterCombo.currentIndex // 筛选类型，默认所有
    property var photoCountText: albumControl.getCustomAlbumInfoConut(customAlbumUId, 1) > 0 ? qsTr("%1 photos").arg(albumControl.getCustomAlbumInfoConut(customAlbumUId, 1)) : ""
    property var videoCountText: albumControl.getCustomAlbumInfoConut(customAlbumUId, 2) > 0 ? qsTr("%1 videos").arg(albumControl.getCustomAlbumInfoConut(customAlbumUId, 2)) : ""
    property var numLabelText: filterType == 0 ? (photoCountText + (videoCountText !== "" ? (" " + videoCountText) : "")) : (filterType == 1 ? photoCountText : videoCountText)
    onVisibleChanged: {
        if (visible) {
            global.statusBarNumText = numLabelText
        }
    }
    onNumLabelTextChanged: {
        if (visible) {
            global.statusBarNumText = numLabelText
        }
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

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        if (filterType >= 0)
            loadCustomAlbumItems()
    }

    onCustomAlbumUIdChanged: {
        loadCustomAlbumItems()
    }

    // 加载自定义相册数据
    function loadCustomAlbumItems()
    {
        console.info("custom album model has refreshed... filterType:", filterType)
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
}
