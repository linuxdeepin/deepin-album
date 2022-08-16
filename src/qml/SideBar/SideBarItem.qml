/*
 * Copyright (C) 2022 UnionTech Technology Co., Ltd.
 *
 * Author:     yeshanshan <yeshanshan@uniontech.com>
 *
 * Maintainer: yeshanshan <yeshanshan@uniontech.com>
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
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Layouts 1.11
import QtQuick.Controls 2.0
import org.deepin.dtk 1.0
import "../Control"

ColumnLayout {
    id: control
    property string title
    property ListModel sideModel
    property alias view: sideListView
    property Component action
    signal itemClicked(string uuid, string text)
    signal itemRightClicked(string uuid, string text)
    signal itemCheckedChanged(int index, bool checked)
    signal removeDeviceBtnClicked(string uuid)
    signal sigRemoveItem()
    property ButtonGroup group: ButtonGroup {}
    property bool showTitle : true
    property bool showRemoveDeviceBtn : false

    // 根据uuid获取所在行索引
    function indexFromUuid(uuid) {
        for (var i = 0; i < sideModel.count; i++) {
            if (Number(sideModel.get(i).uuid) === Number(uuid)) {
                return i
            }
        }

        return -1
    }

    RowLayout {
        id: siderTitle
        visible: showTitle
        Layout.preferredWidth: 180; Layout.preferredHeight: 20
        Label {
            id: viewLabel
            width: 42; height: 20
            text: title
            Layout.alignment: Qt.AlignLeft; Layout.leftMargin: 20
            horizontalAlignment: Qt.AlignLeft

            color: Qt.rgba(0,0,0,0.3)
        }
        Loader {
            Layout.alignment: Qt.AlignRight; Layout.rightMargin: 5
            sourceComponent: control.action
        }
    }
    spacing: 10
    ListView {
        id: sideListView
        implicitHeight: contentHeight
        width: 180
        Layout.alignment: Qt.AlignLeft; Layout.leftMargin: 10
        model: sideModel
        interactive: false
        delegate: SideBarItemDelegate{
            id: sidebarItem
            width: 180
            height: 36
            checked: sideModel.get(index).checked
            backgroundVisible: false
            ButtonGroup.group: group
        }
        Keys.onPressed: {
            switch (event.key){
            case Qt.Key_F2:
                sideListView.currentItem.rename();
                break;
            case Qt.Key_Delete:
                if(sideModel.get(currentIndex).deleteable){
                    sigRemoveItem()
                }
                break
            default:
                break;
            }
            event.accepted = true;
        }
    }

}
