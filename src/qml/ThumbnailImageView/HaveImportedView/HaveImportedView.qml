import QtQuick 2.0
import org.deepin.dtk 1.0
import "../../Control"
import "../../Control/ListView"
Rectangle {
    width: parent.width
    height: parent.height
    Label {
        id:importedLabel
        anchors.top: parent.top
        anchors.left: parent.left
        height: 30
        font: DTK.fontManager.t3
        text: "已导入"

        color: Qt.rgba(0,0,0)

    }

    ImportedlListView {
        id: theView
        anchors.top: importedLabel.bottom
        anchors.topMargin: 10
        width: parent.width
        height: parent.height - importedLabel.height - m_topMargin - m_statsBarHeight

        property int m_statsBarHeight: 30
        property int m_topMargin: 10
    }
}
