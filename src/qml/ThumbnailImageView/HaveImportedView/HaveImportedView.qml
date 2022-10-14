import QtQuick 2.0
import org.deepin.dtk 1.0
import "../../Control"
import "../../"
import "../"

Rectangle {
    width: parent.width
    height: parent.height

    property int filterType : filterCombo.currentIndex // 筛选类型，默认所有
    property string numLabelText: "" //总数标签显示内容
    property string selectedText: getSelectedText(selectedPaths)
    property alias selectedPaths: theView.selectedPaths

    property real titleOpacity: 0.7

    Connections {
        target: albumControl
        onSigRepeatUrls: {
            if (visible) {
                theView.sigUnSelectAll()
                theView.selectedPaths = urls
                global.selectedPaths = selectedPaths
            }
        }
    }

    onVisibleChanged: {
        if (visible)
            flushHaveImportedView()
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        flushHaveImportedView()
    }

    // 刷新已导入视图内容
    function flushHaveImportedView() {
        theView.importedListModel.loadImportedInfos()
        theView.updateSelectedPaths()
        getNumLabelText()

        filterCombo.visible = albumControl.getAllInfoConut(1) > 0 || albumControl.getAllInfoConut(2)
    }

    // 刷新总数标签
    function getNumLabelText() {
        var photoCountText = albumControl.getAllInfoConut(1) > 0 ? qsTr("%1 photos").arg(albumControl.getAllInfoConut(1)) : ""
        var videoCountText = albumControl.getAllInfoConut(2) > 0 ? qsTr("%1 videos").arg(albumControl.getAllInfoConut(2)) : ""
        numLabelText = filterType == 0 ? (photoCountText + (videoCountText !== "" ? ((photoCountText !== "" ? " " : "") + videoCountText) : ""))
                                           : (filterType == 1 ? photoCountText : videoCountText)
        if (visible) {
            global.statusBarNumText = numLabelText
        }
    }

    // 刷新选中项数标签
    function getSelectedText(paths) {
        var selectedNumText = global.getSelectedNumText(paths, numLabelText)
        if (visible)
            global.statusBarNumText = selectedNumText
        return selectedNumText
    }

    // 已导入视图标题栏区域
    Rectangle {
        id: importedTitleRect
        width: parent.width - global.verticalScrollBarWidth
        height: global.thumbnailViewTitleHieght
        color: Qt.rgba(255, 255, 255, titleOpacity)
        z:3

        // 已导入标签
        Label {
            id: importedLabel
            anchors.top: parent.top
            anchors.topMargin: 12
            anchors.left: parent.left
            height: 30
            font: DTK.fontManager.t3
            text: qsTr("Import")

            color: Qt.rgba(0,0,0)
        }

        // 筛选下拉框
        FilterComboBox {
            id: filterCombo
            anchors.top: importedLabel.bottom
            anchors.topMargin: 4
            anchors.right: parent.right
            width: 130
            height: 30
        }
    }

    // 增加标题栏色差校正框选框，用于解决标题栏吸顶导致的框选框色差问题
    Rectangle {
        id: rectTitleSelArea
        color: "#1E90FF"
        opacity: 0.4 * (titleOpacity + 0.002)
        z: 4
        Connections {
            target: theView
            onRectSelTitleChanged: {
                var rectTitle = Qt.rect(0, 0, importedTitleRect.width, importedTitleRect.height)
                var rectTopSel = albumControl.intersected(rt, rectTitle)
                rectTitleSelArea.x = rectTopSel.x
                rectTitleSelArea.y = rectTopSel.y
                rectTitleSelArea.width = rectTopSel.width - 1
                rectTitleSelArea.height = rectTopSel.height
            }
        }
    }

    // 已导入列表控件
    ImportedlListView {
        id: theView
        anchors.top: parent.top
        width: parent.width
        height: parent.height - statusBar.height
    }

    // 若没有数据，显示导入图片视图
    ImportView {
        anchors.fill: parent
        visible: global.currentViewIndex === GlobalVar.ThumbnailViewType.HaveImported && numLabelText === "" && filterType === 0
    }

    Component.onCompleted: {
        global.sigFlushHaveImportedView.connect(flushHaveImportedView)
    }
}
