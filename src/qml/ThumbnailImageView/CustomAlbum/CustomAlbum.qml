import QtQuick 2.0
import org.deepin.dtk 1.0
import "../../Control"
import "../../Control/ListView"
import "../../"
import "../"

Rectangle {
    width: parent.width
    height: parent.height

    property int customAlbumUId: global.currentCustomAlbumUId
    property string customAlbumName: "" //相册名称显示内容
    property int totalPictrueAndVideos: 0
    property int filterType: filterCombo.currentIndex // 筛选类型，默认所有
    property string numLabelText: "" //总数标签显示内容
    property string selectedText: getSelectedText(selectedPaths)
    property alias selectedPaths: theView.selectedPaths
    property bool isCustom : albumControl.isCustomAlbum(customAlbumUId)
    property bool isSystemAutoImport: albumControl.isSystemAutoImportAlbum(customAlbumUId)
    property bool isNormalAutoImport: albumControl.isNormalAutoImportAlbum(customAlbumUId)

    onVisibleChanged: {
        if (visible)
            flushCustomAlbumView(global.currentCustomAlbumUId)
    }

    // 筛选类型改变处理事件
    onFilterTypeChanged: {
        flushCustomAlbumView(global.currentCustomAlbumUId)
    }

    // 我的收藏和相册视图之间切换，需要重载数据
    onCustomAlbumUIdChanged: {
        if (visible) {
            flushAlbumName(global.currentCustomAlbumUId, albumControl.getCustomAlbumByUid(global.currentCustomAlbumUId))
            flushCustomAlbumView(global.currentCustomAlbumUId)
        }
    }

    // 刷新自定义相册名称
    function flushAlbumName(UID, name) {
        if (UID === global.currentCustomAlbumUId) {
            customAlbumName = name
        }
}

    // 刷新自定义相册/我的收藏视图内容
    function flushCustomAlbumView(customAlbumUId) {
        if (customAlbumUId === global.currentCustomAlbumUId || customAlbumUId === -1) {
            loadCustomAlbumItems()
            global.selectedPaths = theView.selectedPaths
            getNumLabelText()
        }
    }

    // 刷新总数标签
    function getNumLabelText() {
        //QML的翻译不支持%n的特性，只能拆成这种代码

        var photoCountText = ""
        var photoCount = albumControl.getCustomAlbumInfoConut(customAlbumUId, 1)
        if(photoCount === 0) {
            photoCountText = ""
        } else if(photoCount === 1) {
            photoCountText = qsTr("1 photo")
        } else {
            photoCountText = qsTr("%1 photos").arg(photoCount)
        }

        var videoCountText = ""
        var videoCount = albumControl.getCustomAlbumInfoConut(customAlbumUId, 2)
        if(videoCount === 0) {
            videoCountText = ""
        } else if(videoCount === 1) {
            videoCountText = qsTr("1 video")
        } else {
            videoCountText = qsTr("%1 videos").arg(videoCount)
        }

        numLabelText = filterType == 0 ? (photoCountText + (videoCountText !== "" ? ((photoCountText !== "" ? " " : "") + videoCountText) : ""))
                                           : (filterType == 1 ? photoCountText : videoCountText)
        if (visible) {
            global.statusBarNumText = numLabelText
        }
    }

    // 刷新选中项目标签内容
    function getSelectedText(paths) {
        var selectedNumText = global.getSelectedNumText(paths, numLabelText)
        if (visible)
            global.statusBarNumText = selectedNumText
        return selectedNumText
    }

    // 加载自定义相册数据
    function loadCustomAlbumItems()
    {
        console.info("custom album model has refreshed... filterType:", filterType, "customAlbumUId:", customAlbumUId)
        theView.selectAll(false)
        theView.thumbnailListModel.clear();
        var customAlbumInfos = albumControl.getAlbumInfos(customAlbumUId, filterType);
        console.info("custom album model has refreshed... filterType:", filterType, "customAlbumUId:", customAlbumUId, " done...")
        for (var key in customAlbumInfos) {
            var customAlbumItems = customAlbumInfos[key]
            for (var i = 0; i < customAlbumItems.length; i++) {
                theView.thumbnailListModel.append(customAlbumItems[i])
            }
            break;
        }

        return true
    }

    Connections {
        target: albumControl
        onSigRepeatUrls: {
            if (visible && global.currentCustomAlbumUId !== 0) {
                theView.selectAll(false)
                theView.selectedPaths = urls
                global.selectedPaths = urls
            }
        }
    }

    // 自定义相册标题栏区域
    Rectangle {
        id: customAlbumTitleRect
        width: parent.width - global.verticalScrollBarWidth
        height: global.thumbnailViewTitleHieght - 10
        // 相册名称标签
        Label {
            id: customAlbumLabel
            anchors.top: parent.top
            anchors.topMargin: 12
            anchors.left: parent.left
            height: 30
            font: DTK.fontManager.t3
            text: qsTr(customAlbumName)
        }

        Label {
            id: customAlbumNumLabel
            anchors.top: customAlbumLabel.bottom
            anchors.topMargin: 10
            anchors.left: parent.left
            font: DTK.fontManager.t6
            text: numLabelText
        }

        // 筛选下拉框
        FilterComboBox {
            id: filterCombo
            anchors.top: customAlbumLabel.bottom
            anchors.topMargin: 4
            anchors.right: parent.right
            width: 130
            height: 30
            visible: !(numLabelText === "" && filterType === 0)
        }

        MouseArea {
            anchors.fill: parent
            onPressed: {
                theView.selectAll(false)
                mouse.accepted = false
            }
        }
    }

    // 缩略图列表控件
    ThumbnailListView {
        id: theView
        anchors.top: customAlbumTitleRect.bottom
        anchors.topMargin: 10
        width: parent.width
        height: parent.height - customAlbumTitleRect.height - m_topMargin - statusBar.height
        thumnailListType: (global.currentViewIndex === GlobalVar.ThumbnailViewType.CustomAlbum && global.currentCustomAlbumUId > 3) ? GlobalVar.ThumbnailType.CustomAlbum
                                                                                                                                    : GlobalVar.ThumbnailType.Normal
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

    // 仅在自动导入相册筛选无内容时，显示无结果
    Label {
        anchors.top: customAlbumTitleRect.bottom
        anchors.left: parent.left
        anchors.bottom: theView.bottom
        anchors.right: parent.right
        anchors.centerIn: parent
        visible: numLabelText === "" && filterType > 0 && (isSystemAutoImport || isNormalAutoImport)
        font: DTK.fontManager.t4
        color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        text: qsTr("No results")
    }

    // 仅在自动导入相册无内容时，显示没有图片或视频时显示
    Label {
        anchors.top: customAlbumTitleRect.bottom
        anchors.left: parent.left
        anchors.bottom: theView.bottom
        anchors.right: parent.right
        anchors.centerIn: parent
        visible: numLabelText === "" && filterType === 0 && (isSystemAutoImport || isNormalAutoImport)
        font: DTK.fontManager.t4
        color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        text: qsTr("No photos or videos found")
    }

    // 自定义相册，若没有数据，显示导入图片视图
    ImportView {
        anchors.fill: parent
        visible: global.currentViewIndex === 6 && numLabelText ==="" && isCustom
    }

    Component.onCompleted: {
        global.sigFlushCustomAlbumView.connect(flushCustomAlbumView)
        global.onSigCustomAlbumNameChaged.connect(flushAlbumName)
    }
}
