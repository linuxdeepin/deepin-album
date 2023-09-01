// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import org.deepin.dtk 1.0

Rectangle {
    Control {
        id: titlecontrol
        hoverEnabled: true // 开启 Hover 属性
        property Palette backgroundColor1: Palette {
            normal: Qt.rgba(255/255, 255/255, 255/255, 0.6)
            normalDark:Qt.rgba(26/255, 26/255, 26/255, 0.6)
        }
        property Palette backgroundColor2: Palette {
            normal: Qt.rgba(255/255, 255/255, 255/255, 0.02)
            normalDark:Qt.rgba(26/255, 26/255, 26/255, 0.02)
        }
    }

    property string iconName :"deepin-image-viewer"
    anchors.top:window.top

//        anchors.topMargin: 10
    width: parent.width
    height: 50
    visible: window.visibility === 5 ? false:true
    color:titlecontrol.ColorSelector.backgroundColor
    gradient: Gradient {
           GradientStop { position: 0.0; color: titlecontrol.ColorSelector.backgroundColor1 }
           GradientStop { position: 1.0; color: titlecontrol.ColorSelector.backgroundColor2 }
       }
    //opacity: 1
    ActionButton {
        anchors.top:parent.top
        anchors.topMargin:GStatus.actionMargin
        anchors.left:parent.left
        anchors.leftMargin:GStatus.actionMargin
        icon {
            name: iconName
            width: 32
            height: 32
        }
    }

    MouseArea { //为窗口添加鼠标事件
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton //只处理鼠标左键
        property point clickPos: "0,0"
        onPressed: { //接收鼠标按下事件
            clickPos  = Qt.point(mouse.x,mouse.y)
            sigTitlePress()
        }
        onPositionChanged: { //鼠标按下后改变位置
            //鼠标偏移量
            var delta = Qt.point(mouse.x-clickPos.x, mouse.y-clickPos.y)

            //如果mainwindow继承自QWidget,用setPos
            window.setX(window.x+delta.x)
            window.setY(window.y+delta.y)
            //               rect1.x = rect1.x + delta.x
            //               rect1.y = rect1.y + delta.y
        }
    }
    TitleBar {
        id :title
        anchors.fill:parent
        windowButtonGroup: WindowButtonGroupEx {
            Layout.alignment: Qt.AlignRight
            Layout.fillHeight: true
            embedMode: title.embedMode
            textColor: title.textColor
            fullScreenButtonVisible: title.fullScreenButtonVisible
            Component.onCompleted: {
                title.toggleWindowState.connect(maxOrWinded)
            }
        }

        aboutDialog: AboutDialog{
            icon: !fileControl.isAlbum() ? "deepin-image-viewer" : "deepin-album"
            width:400
            modality:Qt.NonModal
            version:qsTr(String("Version: %1").arg(Qt.application.version))
            description: !fileControl.isAlbum() ? "Image Viewer is an image viewing tool with fashion interface and smooth performance."
                                                      : qsTr("Album is a stylish management tool for viewing and organizing photos and videos.")
            productName: !fileControl.isAlbum() ? "Image Viewer"
                                                     : qsTr("Album")
            websiteName:DTK.deepinWebsiteName
            websiteLink:DTK.deepinWebsitelLink
            license:qsTr(String("%1 is released under %2.").arg(productName).arg("GPLV3"))
        }

        // 使用自定的文本
        title: ""
        Text {
            anchors.centerIn: parent
            width: parent.width
            leftPadding: 300
            rightPadding: 300
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            // 显示窗口的标题(文件名)
            text: Window.window.title
            // 自动隐藏多余文本
            elide: Text.ElideRight

            textFormat: Text.PlainText
            color: title.textColor
        }
    }
}
