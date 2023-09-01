// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Controls 2.4

import org.deepin.album 1.0 as Album

Item {
    property bool isInTrash: GStatus.currentViewType === Album.Types.ViewRecentlyDeleted
    property bool isInDevice: GStatus.currentViewType === Album.Types.ViewDevice
    //菜单项状态检查
    property bool haveImage: false
    property bool haveVideo: false
    property bool canFullScreen: false
    property bool canSlideShow: false
    property bool canView: false
    property bool canExport: false
    property bool canCopy: false
    property bool canDelete: false
    property bool canViewPhotoInfo: false
    property bool canViewVideoInfo: false
    property bool canWallpaper: false
    property bool canFavorite: albumControl.canFavorite(GStatus.selectedPaths, GStatus.bRefreshFavoriteIconFlag) && !isInTrash
    property bool canRotate: true
    property bool canDisplayInFolder: false
    property bool canPrint: true

    //已选的图片状态检查
    function updateMenuItemStates() {
        haveImage = fileControl.haveImage(GStatus.selectedPaths)
        haveVideo = fileControl.haveVideo(GStatus.selectedPaths)
        canFullScreen = (GStatus.selectedPaths.length === 1 && fileControl.pathExists(GStatus.selectedPaths[0]))
                && fileControl.isImage(GStatus.selectedPaths[0]) && !isInTrash
        canSlideShow = ((GStatus.selectedPaths.length === 1 && fileControl.pathExists(GStatus.selectedPaths[0])) || haveImage)
                && fileControl.isImage(GStatus.selectedPaths[0]) && !isInTrash
        canView = GStatus.selectedPaths.length === 1 && !isInTrash
        canExport= GStatus.selectedPaths.length >= 1 && fileControl.pathExists(GStatus.selectedPaths[0]) && haveImage && !haveVideo && !isInTrash
        canCopy = GStatus.selectedPaths.length >= 1 && !isInTrash
        canDelete = fileControl.isCanDelete(GStatus.selectedPaths)
        canViewPhotoInfo = GStatus.selectedPaths.length === 1 && fileControl.isImage(GStatus.selectedPaths[0])
        canViewVideoInfo = GStatus.selectedPaths.length === 1 && !fileControl.isImage(GStatus.selectedPaths[0])
        canWallpaper = GStatus.selectedPaths.length === 1 && fileControl.isSupportSetWallpaper(GStatus.selectedPaths[0]) && !isInTrash
        canRotate = fileControl.isRotatable(GStatus.selectedPaths)
        canDisplayInFolder = GStatus.selectedPaths.length === 1 && fileControl.pathExists(GStatus.selectedPaths[0]) && !isInTrash
        canPrint = fileControl.isCanPrint(GStatus.selectedPaths) && GStatus.selectedPaths.length === 1 && !isInTrash
    }
}
