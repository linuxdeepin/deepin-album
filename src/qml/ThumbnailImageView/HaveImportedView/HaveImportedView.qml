import QtQuick 2.0
import org.deepin.dtk 1.0
import "../../Control"
Rectangle {
    width: parent.width
    height: parent.height

    // 已导入标签
    Label {
        id:importedLabel
        anchors.top: parent.top
        anchors.left: parent.left
        height: 30
        font: DTK.fontManager.t3
        text: qsTr("Import")

        color: Qt.rgba(0,0,0)

    }

    // 筛选下拉框
    FilterComboBox {
        id: filterCombo
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.rightMargin: 20
        width: 130
        height: 30
    }

    // 已导入列表控件
    ImportedlListView {
        id: theView
        anchors.top: importedLabel.bottom
        anchors.topMargin: 10
        width: parent.width
        height: parent.height - importedLabel.height - m_topMargin - statusBar.height

        property int m_topMargin: 10
    }
}
