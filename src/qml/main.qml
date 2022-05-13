/*
 * Copyright (C) 2020 ~ 2020 Deepin Technology Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.4
import org.deepin.dtk 1.0 as D
import org.deepin.dtk 1.0
ApplicationWindow {
    GlobalVar{
        id: global
    }
    signal sigTitlePress

    // 设置 dtk 风格窗口
    D.DWindow.enabled: true
    id: root
    title: ""

    visible: true
    minimumHeight: global.minHeight
    minimumWidth: global.minWidth
    width: fileControl.getlastWidth()
    height: fileControl.getlastHeight()

    flags: Qt.Window | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint | Qt.WindowTitleHint
    Component.onCompleted: {
        setX(screen.width / 2 - width / 2);
        setY(screen.height / 2 - height / 2);
    }


    onWindowStateChanged: {
        global.sigWindowStateChange()
    }

    onWidthChanged: {
        if(root.visibility!=Window.FullScreen && root.visibility !=Window.Maximized){
            fileControl.setSettingWidth(width)
        }
    }

    onHeightChanged: {
        if(root.visibility!=Window.FullScreen &&root.visibility!=Window.Maximized){
            fileControl.setSettingHeight(height)
        }
    }

    //关闭的时候保存信息
    onClosing: {
        fileControl.saveSetting()
    }

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
    Control {
        id: backcontrol
        hoverEnabled: true // 开启 Hover 属性
        property Palette backgroundColor: Palette {
            normal: "#F8F8F8"
            normalDark:"#000000"
        }
    }

    Sidebar{
        id : leftSidebar
        width: visible ? 200 : 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        visible: true
        ActionButton {
            visible: leftSidebar.visible ? true : false
            id: appTitleIconLeft
            anchors.top:parent.top
            anchors.topMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            width :  leftSidebar.visible ? 50 : 0
            height : 50
            icon {
                name: "deepin-album"
                width: 36
                height: 36
            }
        }

        ActionButton {
            visible: leftSidebar.visible ? true : false
            id: showHideleftSidebarLeftButton
            anchors.top:parent.top
            anchors.topMargin: 0
            anchors.left: appTitleIconLeft.right
            anchors.leftMargin: 0
            width :  leftSidebar.visible ? 50 : 0
            height : 50
            icon {
                name: "topleft"
                width: 36
                height: 36
            }
            onClicked :{
                leftSidebar.visible=!leftSidebar.visible
            }
        }
    }

//    ImportView{
//        anchors.top: root.top
//        anchors.left: leftSidebar.right
//        anchors.leftMargin: 0
//        width: leftSidebar.visible ? parent.width-leftSidebar.width : root.width
//        height: root.height
//    }
    ThumbnailImage{
        anchors.top: root.top
        anchors.left: leftSidebar.right
        anchors.leftMargin: 0
        width: leftSidebar.visible ? parent.width - leftSidebar.width : root.width
        height: root.height
    }

    Rectangle {

        id:titleRect
        anchors.top : root.top
        anchors.left : leftSidebar.right
        anchors.leftMargin: 0
        width: leftSidebar.visible ? parent.width-leftSidebar.width : root.width
        height: 50
        //        color:titlecontrol.ColorSelector.backgroundColor


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
                root.setX(root.x+delta.x)
                root.setY(root.y+delta.y)
            }
        }



        TitleBar {
            id : title
            anchors.fill: parent
            width: parent.width
            aboutDialog: AboutDialog {
                icon: "deepin-album"
                width: 400
                modality: Qt.NonModal
                version: qsTr(String("Version: %1").arg(Qt.application.version))
                description: qsTr("Album is a fashion manager for viewing and organizing photos and videos.")
                productName: qsTr("deepin-album")
                websiteName: DTK.deepinWebsiteName
                websiteLink: DTK.deepinWebsitelLink
                license: qsTr(String("%1 is released under %2").arg(productName).arg("GPLV3"))
            }
            ActionButton {
                visible: leftSidebar.visible ? false : true
                id: appTitleIcon
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.left: parent.left
                anchors.leftMargin: 0
                width :  leftSidebar.visible ? 0 : 50
                height : 50
                icon {
                    name: "deepin-album"
                    width: 36
                    height: 36
                }
            }

            ActionButton {
                visible: leftSidebar.visible ? false : true
                id: showHideleftSidebarButton
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.left: appTitleIcon.right
                anchors.leftMargin: 0
                width :  leftSidebar.visible ? 0 : 50
                height : 50
                icon {
                    name: "topleft"
                    width: 36
                    height: 36
                }
                onClicked :{
                    leftSidebar.visible = !leftSidebar.visible
                }
            }

            ActionButton {
                id: range1Button
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.left: showHideleftSidebarButton.right
                anchors.leftMargin: 0
                width:50
                height:50
                icon {
                    name: "range1"
                    width: 36
                    height: 36
                }
            }
            ButtonBox {

                anchors.top: parent.top
                anchors.topMargin: 7
                anchors.left: range1Button.right
                anchors.leftMargin: 0
                height:36

                ToolButton {
                    Layout.preferredHeight: parent.height
                    checkable: true;
                    text: qsTr("Year") ;
                    checked: true
                }
                ToolButton {
                    Layout.preferredHeight: parent.height
                    checkable: true;
                    text: qsTr("Month")
                }
                ToolButton {
                    Layout.preferredHeight: parent.height
                    checkable: true;
                    text: qsTr("Day")
                }
                ToolButton {
                    Layout.preferredHeight: parent.height
                    checkable: true;
                    text: qsTr("All items")
                }
            }
            SearchEdit{
                placeholder: qsTr("Search")
                width: 240
                anchors.top: parent.top
                anchors.topMargin: 7
                anchors.left: parent.left
                anchors.leftMargin: ( parent.width - width )/2
            }

            ActionButton {

                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 4 * parent.height
                width: 50
                height: 50
                icon {
                    name: "import"
                    width: 36
                    height: 36
                }
            }

        }


    }

}
