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
import QtQuick.Dialogs 1.3
import Qt.labs.folderlistmodel 2.11

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

    onActiveChanged: {
        // 记录应用主窗口是否被置灰过
        if (!root.active)
            global.windowDisActived = true
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

    FileDialog {
        id: importDialog
        title: qsTr("Select pictures")
        folder: shortcuts.pictures
        selectMultiple: true
        nameFilters: ["Image files (*.jpg *.png *.bmp *.gif *.ico *.jpe *.jps *.jpeg *.jng *.koala *.koa *.lbm *.iff *.mng *.pbm *.pbmraw *.pcd *.pcx *.pgm *.pgmraw *.ppm *.ppmraw *.ras *.tga *.targa *.tiff *.tif *.wbmp *.psd *.cut *.xbm *.xpm *.dds *.fax *.g3 *.sgi *.exr *.pct *.pic *.pict *.webp *.jxr *.mrw *.raf *.mef *.raw *.orf *.djvu *.or2 *.icns *.dng *.svg *.nef *.pef *.pxm *.pnm)"]
        onAccepted: {
//            mainView.sourcePaths = fileControl.getDirImagePath(fileDialog.fileUrls[0]);
//            mainView.source = fileDialog.fileUrls[0]
//            mainView.currentIndex=mainView.sourcePaths.indexOf(mainView.source)
//            if(mainView.sourcePaths.length >0){

//                mainView.setThumbnailCurrentIndex(mainView.sourcePaths.indexOf(mainView.source))
//                console.log( "test",mainView.source)
//                stackView.currentWidgetIndex= 1
//            }
            albumControl.importAllImagesAndVideos(importDialog.fileUrls)
        }
    }

    StackControl{
        id: stackControl
    }

}
