import QtQuick 2.0
import org.deepin.dtk 1.0
import "../../Control"
Rectangle {
    width: parent.width
    height: parent.height

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

        z:2
    }
}
