// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4
import org.deepin.dtk 1.0 as DTK

Rectangle {
    anchors.fill: parent
    color: backcontrol.DTK.ColorSelector.backgroundColor

    DTK.ActionButton {
        id: openWidgetImage

        anchors.centerIn: parent
        icon {
            name: "import_photo"
            width: 128
            height: 128
        }
    }

    DTK.RecommandButton {
        id: openFileBtn

        width: 300
        height: 35
        anchors {
            top: openWidgetImage.bottom
            topMargin: 10
            left: openWidgetImage.left
            leftMargin: -86
        }
        font.capitalization: Font.MixedCase
        text: qsTr("Open Image")

        onClicked: stackView.openImageDialog()
    }

    //打开看图查看图片
    function openAndImportImages(paths) {
        if (paths.length === 0)
            return

        var tempPath = ""

        if (paths.length > 0)
            tempPath = paths[0]

        var importPaths = []
        if (!fileControl.isAlbum())
            importPaths = fileControl.getDirImagePath(tempPath)
        else
            importPaths = paths

        GControl.setImageFiles(importPaths, tempPath)
        // 记录当前读取的图片信息，用于监控文件变更
        fileControl.resetImageFiles(importPaths)

        if(importPaths.length > 0 && tempPath !== ""){
            if (fileControl.isVideo(tempPath)){
                root.title = ""
                //TODO： V20右键打开视频文件，不会调影院播放，因此该功能同步V20，若V23要求使用影院播放，放开下面代码即可
                //albumControl.openDeepinMovie(tempPath)
            } else if (fileControl.isImage(tempPath)){
                stackView.switchImageView()
                GStatus.stackControlCurrent = 1
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
