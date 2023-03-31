// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs 1.3
import Qt.labs.folderlistmodel 2.11
import org.deepin.dtk 1.0


Item {
    id: openwidget
    property string file
    //    anchors.fill: parent

    Rectangle{
        id:openRec
        color:backcontrol.ColorSelector.backgroundColor
        anchors.fill: parent
        anchors.centerIn: openwidget

        ActionButton {
            id: openWidgetImage
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            icon {
                name:"import_photo"
                width: 128
                height: 128
            }
        }

        RecommandButton{

            id: openFileBtn
            font.capitalization: Font.MixedCase
            //text: qsTr("Open Image")

            onClicked: fileDialog.open()
            width: 300
            height: 35
            anchors.top:openWidgetImage.bottom
            anchors.topMargin:10

            anchors.left : openWidgetImage.left
            anchors.leftMargin: -86

        }
    }
    Shortcut {
        enabled: stackView.currentWidgetIndex != 2 && stackView.visible
        sequence: "Ctrl+O"
        onActivated:{
            if(stackView.currentWidgetIndex!= 2){
                fileDialog.open()
            }
        }
    }

    FileDialog {
        id: fileDialog
        title: qsTr("Select pictures")
        folder: shortcuts.pictures
        selectMultiple: true
//        nameFilters: ["Image files (*.jpg *.png *.bmp *.gif)"]

        nameFilters: ["Image files (*.jpg *.png *.bmp *.gif *.ico *.jpe *.jps *.jpeg *.jng *.koala *.koa *.lbm *.iff *.mng *.pbm *.pbmraw *.pcd *.pcx *.pgm *.pgmraw *.ppm *.ppmraw *.ras *.tga *.targa *.tiff *.tif *.wbmp *.psd *.cut *.xbm *.xpm *.dds *.fax *.g3 *.sgi *.exr *.pct *.pic *.pict *.webp *.jxr *.mrw *.raf *.mef *.raw *.orf *.djvu *.or2 *.icns *.dng *.svg *.nef *.pef *.pxm *.pnm)"]
        onAccepted: {
            mainView.sourcePaths = fileControl.getDirImagePath(fileDialog.fileUrls[0]);
            mainView.source = fileDialog.fileUrls[0]
            mainView.currentIndex=mainView.sourcePaths.indexOf(mainView.source)
            if(mainView.sourcePaths.length >0){
                // 记录当前读取的图片信息
                fileControl.resetImageFiles(mainView.sourcePaths)

                mainView.setThumbnailCurrentIndex(mainView.sourcePaths.indexOf(mainView.source))
                console.log( "test",mainView.source)
                stackView.currentWidgetIndex= 1
            }

            // 将选择的图片导入相册中
            if (fileControl.isAlbum() && mainView.source !== "") {
                albumControl.importAllImagesAndVideos(mainView.sourcePaths)
            }
        }
    }

    FolderListModel
    {
        id: foldermodel
        folder: "file://" + escape(platform.picturesLocation()) //不明确这个模块的作用,暂时使用这种方式来转换字符串和URL
        showDirs: false
        showDotAndDotDot: false
        nameFilters: ["*.dng", "*.nef", "*.bmp", "*.gif", "*.ico", "*.jpeg", "*.jpg", "*.pbm", "*.pgm","*.png",  "*.pnm", "*.ppm",
            "*.svg", "*.tga", "*.tif", "*.tiff", "*.wbmp", "*.webp", "*.xbm", "*.xpm", "*.gif"]
        sortField: FolderListModel.Type
        showOnlyReadable: true
        sortReversed: false

        onCountChanged: {
            //           if(!root.run){
            //              root.fileMonitor()
            //           }
        }
    }

    //打开看图查看图片
    function openAndImportImages(paths) {
        if (paths.length === 0)
            return

        var tempPath = ""

        if (paths.length > 0)
            tempPath = paths[0]

        if (!fileControl.isAlbum())
            mainView.sourcePaths = fileControl.getDirImagePath(tempPath);
        else
            mainView.sourcePaths = paths

        // 记录当前读取的图片信息，用于监控文件变更
        fileControl.resetImageFiles(mainView.sourcePaths)

        mainView.source = tempPath

        if(mainView.sourcePaths.length > 0 && tempPath !== ""){
            mainView.setThumbnailCurrentIndex(mainView.sourcePaths.indexOf(mainView.source))
            if (fileControl.isVideo(tempPath)){
                root.title = ""
                //TODO： V20右键打开视频文件，不会调影院播放，因此该功能同步V20，若V23要求使用影院播放，放开下面代码即可
                //albumControl.openDeepinMovie(tempPath)
            } else if (fileControl.isImage(tempPath)){
                mainStack.currentWidgetIndex = 1
                global.stackControlCurrent = 1
            }
        }

        // 若在文管菜单使用相册打开图片文件，并且数据库中未导入该图片，应该将选择的图片导入相册中
        if (fileControl.isAlbum() && tempPath !== "" && !albumControl.checkRepeatUrls(albumControl.getAllUrlPaths(), paths, false)) {
            albumControl.importAllImagesAndVideos(paths)
        }
    }

    Connections {
        target: albumControl
        onSigOpenImageFromFiles: {
            openAndImportImages(paths)
        }
    }

    Component.onCompleted: {
        var tempPaths = []
        tempPaths = fileControl.parseCommandlineGetPaths()

        openAndImportImages(tempPaths)
    }
}
