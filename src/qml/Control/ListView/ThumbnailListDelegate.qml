import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQml.Models 2.11
import QtQml 2.11
import QtQuick.Shapes 1.10

Rectangle  {
    width: 100
    height: 100

    //注意：在model里面加进去的变量，这边可以直接进行使用，只是部分位置不好拿到，需要使用变量
    property string m_index
    property string m_path

    onM_pathChanged: {
    }

    //选中后显示的阴影框
    Rectangle {
        id: selectShader
        anchors.fill: parent
        radius: 5 * 2
        color: "#AAAAAA"
        visible: theView.ism.indexOf(parent.m_index) != -1
        opacity: 0.4
    }

    Image{
        source: "image://publisher/" + displayFlushHelper.toString() + theView.displayFlushHelper.toString() + "_" + path
        asynchronous: true
        anchors.centerIn: parent
        sourceSize.width: parent.width - 14
        sourceSize.height: parent.height - 14
        width:parent.width - 14
        height:parent.height - 14
        fillMode:Image.Pad
        clip: true
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton //仅激活右键，激活左键会和view的冲突
        onClicked: {
            console.log(theView.ism)
            if(theView.ism.indexOf(parent.m_index) == -1)
            {
                theView.ism = [parent.m_index]
            }

            thumbnailMenu.popup()
        }
    }

    //缩略图菜单
    Menu {
        id: thumbnailMenu

        //局部刷新例程
        MenuItem {
            text: qsTr("顺时针旋转")
            onTriggered: {

            }
        }

        //删除部分例程
        MenuItem {
            text: qsTr("删除")
            onTriggered: {

            }
        }

        //调起文管例程
        MenuItem {
            text: qsTr("在文件管理器中显示")
            onTriggered: {

            }
        }
    }
}

