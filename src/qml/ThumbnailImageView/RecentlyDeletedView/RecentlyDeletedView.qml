import QtQuick 2.0
import org.deepin.dtk 1.0
import "../../Control"
import "../../Control/ListView"
import "../../"
import "../"

Rectangle {
    width: parent.width
    height: parent.height

    property int filterType : filterCombo.currentIndex // 筛选类型，默认所有
    property string numLabelText: ""
    property string selectedText: getSelectedText(selectedPaths)
    property alias selectedPaths: theView.selectedPaths
    property var selectedOriginPaths
    property int totalCount: 0

    onVisibleChanged: {
        if (visible) {
            flushRecentDelView()
        }
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        flushRecentDelView()
    }

    // 刷新最近删除视图内容
    function flushRecentDelView() {
        loadRecentDelItems()
        global.selectedPaths = theView.selectedPaths
        getNumLabelText()
        totalCount = albumControl.getTrashInfoConut(1) +  albumControl.getTrashInfoConut(2)
    }

    // 刷新总数标签
    function getNumLabelText() {
        var photoCountText = albumControl.getTrashInfoConut(1) > 0 ? qsTr("%1 photos").arg(albumControl.getTrashInfoConut(1)) : ""
        var videoCountText = albumControl.getTrashInfoConut(2) > 0 ? qsTr("%1 videos").arg(albumControl.getTrashInfoConut(2)) : ""
        numLabelText = filterType == 0 ? (photoCountText + (videoCountText !== "" ? ((photoCountText !== "" ? " " : "") + videoCountText) : ""))
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

    // 加载最近删除图片数据
    function loadRecentDelItems()
    {
        console.info("recently delete model has refreshed... filterType:", filterType)
        theView.thumbnailListModel.clear();
        var delInfos = albumControl.getTrashAlbumInfos(filterType);
        console.info("recently delete model has refreshed... filterType:", filterType, " done...")
        for (var key in delInfos) {
            var delItems = delInfos[key]
            for (var i = 0; i < delItems.length; i++) {
                theView.thumbnailListModel.append(delItems[i])
            }
            break;
        }
    }

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
            visible: !theView.haveSelect && theView.count()
            onClicked: {
                albumControl.deleteImgFromTrash(theView.allOriginPaths())
                theView.selectAll(false)
                global.sigFlushRecentDelView()
            }
        }

        // 筛选下拉框
        FilterComboBox {
            id: filterCombo
            anchors.top: recentDelLabel.bottom
            anchors.topMargin: 4
            anchors.right: parent.right
            width: 130
            height: 30
            visible: !theView.haveSelect && totalCount > 0
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

            onClicked: {
                albumControl.deleteImgFromTrash(selectedOriginPaths)
                theView.selectAll(false)
                global.sigFlushRecentDelView()
            }
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

            onClicked: {
                albumControl.recoveryImgFromTrash(selectedOriginPaths)
                theView.selectAll(false)
                global.sigFlushRecentDelView()
            }
        }
    }

    // 缩略图列表控件
    ThumbnailListView {
        id: theView
        anchors.top: recentDelTitleRect.bottom
        anchors.topMargin: 10
        width: parent.width
        height: parent.height - recentDelTitleRect.height - m_topMargin - statusBar.height
        thumnailListType: GlobalVar.ThumbnailType.Trash

        property int m_topMargin: 10

        // 监听缩略图列表选中状态，一旦改变，更新globalVar所有选中路径
        Connections {
            target: theView
            onSelectedChanged: {
                selectedOriginPaths = []
                selectedOriginPaths = theView.selectedOriginPaths
                global.selectedPaths = theView.selectedPaths
            }
        }
    }

    // 若没有数据，显示无图片视图
    NoPictureView {
        anchors.fill: parent
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.RecentlyDeleted && numLabelText === "" && filterType === 0
    }

    Component.onCompleted: {
        global.sigFlushRecentDelView.connect(flushRecentDelView)
    }
}
