import QtQuick 2.0
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import org.deepin.dtk 1.0 as D
import org.deepin.dtk 1.0

ListModel {
    id: thumbnailListModel
    property var haveImportedPaths :global.haveImportedPaths
    function loadHaveImportedDatas()
    {
        // TODO：后续可换为直接从后台获取数据，获得的数据类型为按Json串组成的QMap
        thumbnailListModel.clear();
        for(var i = 0; i < haveImportedPaths.length; i++){
            var data = global.haveImportedPaths[i];

            thumbnailListModel.append({"path": data, "displayFlushHelper" : 0});
        }
        thumbnailListModel = haveImportedPaths
    }

    // 监听已导入文件列表是否有改变
    onHaveImportedPathsChanged: {
        loadHaveImportedDatas()
    }
    Component.onCompleted: {
        thumbnailListModel.loadHaveImportedDatas();
    }
}
