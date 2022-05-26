import QtQuick 2.0
import org.deepin.dtk 1.0
import "../../Control"
import "../../Control/ListView"
Rectangle {
    width: parent.width
    height: parent.height

    property int filterType : filterCombo.currentIndex // 筛选类型，默认所有
    // 最近删除标题栏区域
    Rectangle {
        id: recentDelTitleRect
        width: parent.width - global.verticalScrollBarWidth
        height: global.thumbnailViewTitleHieght - 10
        // 最近删除标签
        Label {
            id: recentDelLabel
            anchors.top: parent.top
            anchors.topMargin: 12
            anchors.left: parent.left
            height: 30
            font: DTK.fontManager.t3
            text: qsTr("Trash")
        }

        Label {
            id: recentDelTipLabel
            anchors.top: recentDelLabel.bottom
            anchors.topMargin: 10
            anchors.left: parent.left
            font: DTK.fontManager.t6
            text: qsTr("The files will be permanently deleted after the days shown on them")
        }

        WarningButton {
            id: delAllBtn
            anchors.top: parent.top
            anchors.topMargin: 12
            anchors.right: parent.right
            width: 140
            height: 36
            flat: true
            font: DTK.fontManager.t6
            text: qsTr("Delete All")
            visible: !theView.haveSelect
        }

        // 筛选下拉框
        FilterComboBox {
            id: filterCombo
            anchors.top: recentDelLabel.bottom
            anchors.topMargin: 4
            anchors.right: parent.right
            width: 130
            height: 30
            visible: !theView.haveSelect
        }

        WarningButton {
            id: delSelectedBtn
            anchors.top: recentDelLabel.bottom
            anchors.topMargin: 4
            anchors.right: parent.right
            width: 140
            height: 36
            font: DTK.fontManager.t6
            text: qsTr("Delete Selected (%1)").arg(theView.haveSelectCount)
            visible: theView.haveSelect
        }

        Button {
            id: restoreSelectedBtn
            anchors.top: recentDelLabel.bottom
            anchors.topMargin: 4
            anchors.right: delSelectedBtn.left
            anchors.rightMargin: 10
            width: 140
            height: 36
            font: DTK.fontManager.t6
            text: qsTr("Restore Selected (%1)").arg(theView.haveSelectCount)
            visible: theView.haveSelect
        }
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        if (filterType >= 0)
            loadRecentDelItems()
    }

    // 加载最近删除图片数据
    function loadRecentDelItems()
    {
        console.info("recently delete model has refreshed... filterType:", filterType)
        theView.thumbnailListModel.clear();
        var delInfos = albumControl.getTrashAlbumInfos(filterType);
        for (var key in delInfos) {
            var delItems = delInfos[key]
            for (var i = 0; i < delItems.length; i++) {
                theView.thumbnailListModel.append(delItems[i])
            }
            break;
        }
    }

    // 缩略图列表控件
    ThumbnailListView {
        id: theView
        anchors.top: recentDelTitleRect.bottom
        anchors.topMargin: 10
        width: parent.width
        height: parent.height - recentDelTitleRect.height - m_topMargin - statusBar.height

        property int m_topMargin: 10
    }
}
