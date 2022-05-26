import QtQuick 2.0
import org.deepin.dtk 1.0
import "../../Control"
import "../../Control/ListView"
Rectangle {
    width: parent.width
    height: parent.height

    property int filterType : filterCombo.currentIndex // 筛选类型，默认所有
    // 我的收藏标题栏区域
    Rectangle {
        id: favoriteTitleRect
        width: parent.width - global.verticalScrollBarWidth
        height: global.thumbnailViewTitleHieght - 10
        // 我的收藏标签
        Label {
            id: myFavoriteLabel
            anchors.top: parent.top
            anchors.topMargin: 12
            anchors.left: parent.left
            height: 30
            font: DTK.fontManager.t3
            text: qsTr("Favorites")
        }

        Label {
            id: favoriteNumLabel
            anchors.top: myFavoriteLabel.bottom
            anchors.topMargin: 10
            anchors.left: parent.left
            font: DTK.fontManager.t6
            text: 0 + qsTr(" photos") + " " + 0 + qsTr(" videos")
        }

        // 筛选下拉框
        FilterComboBox {
            id: filterCombo
            anchors.top: myFavoriteLabel.bottom
            anchors.topMargin: 4
            anchors.right: parent.right
            width: 130
            height: 30
        }
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        if (filterType >= 0)
            loadFavoriteItems()
    }

    // 加载我的收藏图片数据
    function loadFavoriteItems()
    {
        console.info("favorites model has refreshed... filterType:", filterType)
        theView.thumbnailListModel.clear();
        var favoriteInfos = albumControl.getAlbumInfos(0, filterType);
        console.info("favorites model has refreshed... filterType:", filterType, " done...")
        for (var key in favoriteInfos) {
            var favoriteItems = favoriteInfos[key]
            for (var i = 0; i < favoriteItems.length; i++) {
                theView.thumbnailListModel.append(favoriteItems[i])
            }
            break;
        }
    }

    // 缩略图列表控件
    ThumbnailListView {
        id: theView
        anchors.top: favoriteTitleRect.bottom
        anchors.topMargin: 10
        width: parent.width
        height: parent.height - favoriteTitleRect.height - m_topMargin - statusBar.height

        property int m_topMargin: 10
    }
}
