import QtQuick 2.0
import QtQuick.Controls 2.4
import org.deepin.dtk 1.0
import "../../Control"
import "../../Control/ListView"
Rectangle {
    width: parent.width
    height: parent.height

    property int customAlbumUId: 0
    property string devicePath: global.deviceCurrentPath
    property string deviceName: global.deviceCurrentName
    property int filterType: 0
    property int currentImportIndex: 0
    property var numLabelText: getNumLabelText(filterType)
    property string selectedText: getSelectedText(selectedPaths)
    property alias selectedPaths: theView.selectedPaths

    onVisibleChanged: {
        if (visible) {
            console.log("device show...")
            flushDeviceAlbumView()
        }
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        flushDeviceAlbumView()
    }

    // 设备之间切换，需要重载数据
    onDevicePathChanged: {
        console.log("onDevicePathChanged...")
        flushDeviceAlbumView()
    }

    // 刷新设备视图内容
    function flushDeviceAlbumView() {
        loadDeviceAlbumItems()
        global.selectedPaths = theView.selectedPaths
        getNumLabelText()
    }

    // 刷新总数标签
    function getNumLabelText() {
        var photoCountText = albumControl.getDeviceAlbumInfoConut(devicePath, 1) > 0 ? qsTr("%1 photos").arg(albumControl.getDeviceAlbumInfoConut(devicePath, 1)) : ""
        var videoCountText = albumControl.getDeviceAlbumInfoConut(devicePath, 2) > 0 ? qsTr("%1 videos").arg(albumControl.getDeviceAlbumInfoConut(devicePath, 2)) : ""
        var numLabelText = filterType == 0 ? (photoCountText + (videoCountText !== "" ? ((photoCountText !== "" ? " " : "") + videoCountText) : ""))
                                           : (filterType == 1 ? photoCountText : videoCountText)
        if (visible) {
            global.statusBarNumText = numLabelText
        }

        return numLabelText
    }

    // 刷新选中项数标签
    function getSelectedText(paths) {
        var selectedNumText = global.getSelectedNumText(paths, numLabelText)
        if (visible)
            global.statusBarNumText = selectedNumText
        return selectedNumText
    }

    // 加载设备相册数据
    function loadDeviceAlbumItems()
    {
        console.info("device album model has refreshed... filterType:", filterType)
        theView.selectAll(false)
        theView.thumbnailListModel.clear();
        var customAlbumInfos = albumControl.getDeviceAlbumInfos(devicePath, filterType);
        console.info("device album model has refreshed... filterType:", filterType, " done...")
        for (var key in customAlbumInfos) {
            var customAlbumItems = customAlbumInfos[key]
            for (var i = 0; i < customAlbumItems.length; i++) {
                theView.thumbnailListModel.append(customAlbumItems[i])
            }
            break;
        }

        return true
    }

    DeviceLoadDialog {
        id: devloaddig
    }

    // 新增挂载设备，显示正在加载对话框
    Connections {
        target: albumControl
        onSigAddDevice: {
            devloaddig.show()
        }
    }

    // 导入重复图片提示
    Connections {
        target: albumControl
        onSigRepeatUrls: {
            if (visible && global.currentCustomAlbumUId !== 0) {
                theView.selectedPaths = urls
                global.selectedPaths = selectedPaths
            }
        }
    }

    // 设备相册标题栏区域
    Rectangle {
        id: deviceAlbumTitleRect
        width: parent.width - global.verticalScrollBarWidth
        height: global.thumbnailViewTitleHieght - 10
        // 设备名称标签
        Label {
            id: deviceAlbumLabel
            anchors.top: parent.top
            anchors.topMargin: 12
            anchors.left: parent.left
            height: 30
            font: DTK.fontManager.t3
            text: qsTr(deviceName)
        }

        Label {
            id: deviceAlbumNumLabel
            anchors.top: deviceAlbumLabel.bottom
            anchors.topMargin: 10
            anchors.left: parent.left
            font: DTK.fontManager.t6
            text: numLabelText
        }
        Row{
            anchors.top: deviceAlbumLabel.bottom
            anchors.topMargin: 4
            anchors.right: parent.right
            spacing: 10
            // 筛选下拉框
            Rectangle{
                width: 80
                height: 36
                Label {
                    anchors.centerIn: parent
                    text :qsTr("Import to:")
                    width: 56
                    height: 20
                }
            }

            ComboBox {
                id: filterCombo
                width: 180
                height: 36
                displayText : currentIndex == 0?qsTr("Import") : currentIndex == 1? qsTr("New Album")  :albumControl.getAllCustomAlbumName(global.albumChangeList)[currentIndex-2]
                model: albumControl.getAllCustomAlbumId(global.albumChangeList).length+2
                delegate: MenuItem {
                    text: index == 0?qsTr("Import") : index == 1? qsTr("New Album")  :albumControl.getAllCustomAlbumName(global.albumChangeList)[index-2]
                    onTriggered:{
                        if(index == 0 ){
                            currentImportIndex = 0
                        }else if(index == 1){
                            var x = parent.mapToGlobal(0, 0).x + parent.width / 2 - 190
                            var y = parent.mapToGlobal(0, 0).y + parent.height / 2 - 89
                            newAlbum.setX(x)
                            newAlbum.setY(y)
                            newAlbum.isChangeView = false
                            newAlbum.setNormalEdit()
                            newAlbum.show()
                        }else {
                            currentImportIndex = index -2
                        }
                    }
                    highlighted: filterCombo.highlightedIndex === index

                }

            }
            RecommandButton{
                id: openPictureBtn
                font.capitalization: Font.MixedCase
                text: global.selectedPaths.length >0 ?qsTr("Import %1 items").arg(global.selectedPaths.length) :qsTr("Import All")
                onClicked:{
                    if(global.selectedPaths.length >0){
                        albumControl.importFromMountDevice(global.selectedPaths,albumControl.getAllCustomAlbumId(global.albumChangeList)[currentImportIndex])
                    }else{
                        albumControl.importFromMountDevice(theView.allOriginUrls(),albumControl.getAllCustomAlbumId(global.albumChangeList)[currentImportIndex])
                    }
                    DTK.sendMessage(thumbnailImage, qsTr("Import successful"), "checked")
                }
                width: 114
                height: 36
            }
        }
    }

    // 缩略图列表控件
    ThumbnailListView {
        id: theView
        anchors.top: deviceAlbumTitleRect.bottom
        anchors.topMargin: 10
        width: parent.width
        height: parent.height - deviceAlbumTitleRect.height - m_topMargin - statusBar.height
        visible: numLabelText !== ""
        property int m_topMargin: 10

        // 监听缩略图列表选中状态，一旦改变，更新globalVar所有选中路径
        Connections {
            target: theView
            onSelectedChanged: {
                var selectedPaths = []
                selectedPaths = theView.selectedPaths

                if (parent.visible)
                    global.selectedPaths = selectedPaths
            }
        }
    }

    Label {
        anchors.top: deviceAlbumTitleRect.bottom
        anchors.left: parent.left
        anchors.bottom: theView.bottom
        anchors.right: parent.right
        anchors.centerIn: parent
        visible: numLabelText === "" && filterType > 0
        font: DTK.fontManager.t4
        color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        text: qsTr("No results")
    }
}
