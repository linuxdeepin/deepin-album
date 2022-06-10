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
    function setDateRange(str) {
        dateRangeLabel.text = str
    }

    ListModel {
        id: theModel
    }

    Label {
        id: dateRangeLabel
        anchors.left: parent.left
        anchors.top: parent.top
        font: DTK.fontManager.t3
    }

    ThumbnailListView {
        id: view
        thumbnailListModel : theModel
        anchors.left: parent.left
        anchors.top: dateRangeLabel.bottom
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: 10
    }

    Component.onCompleted: {
        view.timeChanged.connect(setDateRange)

        var paths = albumControl.getAllPaths()
        for(var i = 0;i !== paths.length;++i) {
            theModel.append({url : paths[i]})
        }

        view.setType(GlobalVar.ThumbnailType.AllCollection)
    }
}
