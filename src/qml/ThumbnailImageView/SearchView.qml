import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import org.deepin.dtk 1.0
import "../Control"
import "../Control/ListView"

Rectangle {
    width: parent.width
    height: parent.height

    property string currentKeyword: ""
    property int searchResultCount: 0

    function searchFromKeyword(UID, keyword) {
        //1.调起C++，执行搜索
        var searchResult = albumControl.searchPicFromAlbum(UID, keyword, false);

        //2.将结果刷入UI
        currentKeyword = keyword
        flushResultToView(searchResult)
    }

    function flushResultToView(searchResult) {
        if(searchResult.length > 0) {
            noResultView.visible = false
            resultView.visible = true
            theModel.clear()
            for(var i = 0;i !== searchResult.length;++i) {
                theModel.append({url : searchResult[i], displayFlushHelper : 0})
            }
            searchResultCount = searchResult.length
        } else {
            noResultView.visible = true
            resultView.visible = false
        }
    }

    //搜索标题
    Label {
        id: searchTitle
        text: qsTr('"%1" 的搜索结果').arg(currentKeyword)
        anchors.top: parent.top
        anchors.left: parent.left
        font: DTK.fontManager.t3
        color: Qt.rgba(0,0,0)
    }

    //缩略图模型
    ListModel {
        id: theModel
    }

    //有结果展示
    Rectangle {
        id: resultView
        anchors.top: searchTitle.bottom
        anchors.left: searchTitle.left
        anchors.bottom: parent.bottom
        anchors.right: parent.right

        //幻灯片放映按钮
        RecommandButton {
            id: sliderPlayButton
            text: qsTr("幻灯片放映")
            icon {
                name:"Combined_Shape"
                width: 20
                height: 20
            }
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: 15

            onClicked: {
                console.debug("123123")
            }
        }

        //搜索结果label
        Label {
            id: searchResultLabel
            text: qsTr('搜到 %1 项').arg(searchResultCount)
            anchors.left: sliderPlayButton.right
            anchors.verticalCenter: sliderPlayButton.verticalCenter
            font: DTK.fontManager.t6
            anchors.leftMargin: 10
        }

        //缩略图视图
        ThumbnailListView {
            id: view
            thumbnailListModel : theModel
            anchors.left: parent.left
            anchors.top: sliderPlayButton.bottom
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.topMargin: 10
        }
    }

    //无结果展示
    Rectangle {
        id: noResultView
        anchors.top: searchTitle.bottom
        anchors.left: searchTitle.left
        anchors.bottom: parent.bottom
        anchors.right: parent.right

        Label {
            id: noResultText
            text: qsTr("无结果")
            font: DTK.fontManager.t3
            anchors.bottom: moreInfoText.top
            anchors.bottomMargin: 10
            anchors.horizontalCenter: moreInfoText.horizontalCenter
            color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        }
        Label {
            id: moreInfoText
            text: qsTr('没有 "%1" 的结果，请尝试搜索新词').arg(currentKeyword)
            anchors.centerIn: parent
            font: DTK.fontManager.t6
            color: Qt.rgba(85/255, 85/255, 85/255, 0.4)
        }
    }

    Component.onCompleted: {
        global.sigRunSearch.connect(searchFromKeyword)
    }
}
