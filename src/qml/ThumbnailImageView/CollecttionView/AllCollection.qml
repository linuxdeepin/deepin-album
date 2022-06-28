import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import org.deepin.dtk 1.0
import "../../Control"
import "../../Control/ListView"
import "../../"

Item {
    property int filterType: filterCombo.currentIndex // 筛选类型，默认所有
    property string numLabelText:""
    property string selectedText: getSelectedNum(selectedPaths)
    property var selectedPaths: []

    function setDateRange(str) {
        dateRangeLabel.text = str
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        flushAllCollectionView()
    }

    // 刷新所有项目视图内容
    function flushAllCollectionView() {
        loadAllCollectionItems()
        global.selectedPaths = theView.selectedPaths
        getNumLabelText()
        theView.totalTimeScope()
    }

    function flushTotalTimeScope() {
        theView.totalTimeScope()
    }

    // 刷新总数标签
    function getNumLabelText() {
        var photoCountText = albumControl.getAllCount(1) > 0 ? qsTr("%1 photos").arg(albumControl.getAllCount(1)) : ""
        var videoCountText = albumControl.getAllCount(2) > 0 ? qsTr("%1 videos").arg(albumControl.getAllCount(2)) : ""
        numLabelText = filterType == 0 ? (photoCountText + (videoCountText !== "" ? (", " + videoCountText) : ""))
                                           : (filterType == 1 ? photoCountText : videoCountText)
        if (visible) {
            global.statusBarNumText = numLabelText
        }
    }

    // 刷新选择项数标签
    function getSelectedNum(paths) {
        console.log("selected count:", selectedPaths.length)
        var selectedNumText = selectedPaths.length == 0 ? numLabelText : qsTr("%1 items selected (%2)").arg(selectedPaths.length).arg(numLabelText)
        if (visible)
            global.statusBarNumText = selectedNumText
        return selectedNumText
    }

    // 加载所有项目数据
    function loadAllCollectionItems()
    {
        console.info("all collection model has refreshed... filterType:", filterType)
        theView.selectAll(false)
        theView.thumbnailListModel.clear();
        var allCollections = albumControl.getAlbumAllInfos(filterType);
        console.info("all collection model has refreshed... filterType:", filterType, " done...")
        for (var i = 0; i < allCollections.length; i++) {
            theView.thumbnailListModel.append(allCollections[i])
        }

        return true
    }


    // 所有项目标题栏区域
    Rectangle {
        id: allCollectionTitleRect
        width: parent.width - global.verticalScrollBarWidth
        height: 60
        // 时间范围标签
        Label {
            id: dateRangeLabel
            anchors.top: parent.top
            anchors.topMargin: 16
            anchors.left: parent.left
            height: 30
            font: DTK.fontManager.t3
            //text: qsTr(albumControl.getCustomAlbumByUid(customAlbumUId))
        }

        // 筛选下拉框
        FilterComboBox {
            id: filterCombo
            anchors.top: dateRangeLabel.top
            anchors.topMargin: 4
            anchors.right: parent.right
            width: 130
            height: 30
            //visible: !(numLabelText === "" && filterType === 0)
        }
    }

    ThumbnailListView {
        id: theView
        anchors.top: allCollectionTitleRect.bottom
        anchors.topMargin: m_topMargin
        width: parent.width
        height: parent.height - allCollectionTitleRect.height - m_topMargin - statusBar.height
        visible: true
        property int m_topMargin: 10

        // 监听缩略图列表选中状态，一旦改变，更新globalVar所有选中路径
        Connections {
            target: theView
            onSelectedChanged: {
                selectedPaths = []
                selectedPaths = theView.selectedPaths

                if (parent.visible)
                    global.selectedPaths = selectedPaths
            }
        }
    }

    Component.onCompleted: {
        theView.timeChanged.connect(setDateRange)
        global.sigFlushAllCollectionView.connect(flushAllCollectionView)

        theView.setType(GlobalVar.ThumbnailType.AllCollection)
    }
}
