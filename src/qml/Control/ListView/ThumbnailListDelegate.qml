import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQml.Models 2.11
import QtQml 2.11
import QtQuick.Shapes 1.10
import org.deepin.dtk 1.0

import "../"
import "../../"

Rectangle {
    //注意：在model里面加进去的变量，这边可以直接进行使用，只是部分位置不好拿到，需要使用变量
    property string m_index : index
    property string m_path : path

    onM_pathChanged: {
    }

    //选中后显示的阴影框
    Rectangle {
        id: selectShader
        anchors.fill: parent
        radius: 10
        color: "#AAAAAA"
        visible: theView.ism.indexOf(parent.m_index) !== -1
        opacity: 0.4
    }

    Image {
        source: "image://publisher/" + displayFlushHelper.toString() + theView.displayFlushHelper.toString() + "_" + path
        asynchronous: true
        anchors.centerIn: parent
        //sourceSize固定下来不要动，否则会疯狂闪烁
        sourceSize.width:  200
        sourceSize.height: 200
        width: parent.width - 14
        height: parent.height - 14
        //使用PreserveAspectFit确保在原始比例下不会变形
        fillMode: Image.PreserveAspectFit
        clip: true
    }
}
