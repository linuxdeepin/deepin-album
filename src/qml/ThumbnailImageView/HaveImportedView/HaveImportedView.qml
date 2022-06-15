import QtQuick 2.0
import org.deepin.dtk 1.0
import "../../Control"
Rectangle {
    width: parent.width
    height: parent.height

    property int filterType : filterCombo.currentIndex // 筛选类型，默认所有
    property var numLabelText: getNumLabelText(filterType) //总数标签显示内容
    onVisibleChanged: {
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
    }

    // 刷新总数标签
    function getNumLabelText() {
        var photoCountText = albumControl.getAllInfoConut(1) > 0 ? qsTr("%1 photos").arg(albumControl.getAllInfoConut(1)) : ""
        var videoCountText = albumControl.getAllInfoConut(2) > 0 ? qsTr("%1 videos").arg(albumControl.getAllInfoConut(2)) : ""
        var numLabelText = filterType == 0 ? (photoCountText + (videoCountText !== "" ? (" " + videoCountText) : ""))
                                           : (filterType == 1 ? photoCountText : videoCountText)
        if (visible) {
            global.statusBarNumText = numLabelText
        }

        return numLabelText
    }

    // 已导入视图标题栏区域
    Rectangle {
        id: importedTitleRect
        width: parent.width - global.verticalScrollBarWidth
        height: global.thumbnailViewTitleHieght
        color: Qt.rgba(255,255,255,0.7)
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

    // 已导入列表控件
    ImportedlListView {
        id: theView
        anchors.top: parent.top
        width: parent.width
        height: parent.height - statusBar.height
    }

    Component.onCompleted: {
        global.sigFlushHaveImportedView.connect(flushHaveImportedView)
    }
}
