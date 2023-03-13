// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 执行图片查看操作
function executeViewImage() {
    if (thumnailListType !== GlobalVar.ThumbnailType.Trash) {
        var indexes = dir.selectedIndexes
        if (indexes.length > 0) {
            if (fileControl.isVideo(dir.data(indexes[0], "url").toString())) {
                albumControl.openDeepinMovie(dir.data(indexes[0], url).toString())
            } else {
                var allUrls = dir.allUrls()
                mainStack.sourcePaths = allUrls
                mainStack.currentIndex = -1
                mainStack.currentIndex = indexes[0]
                mainStack.currentWidgetIndex = 1
                global.stackControlCurrent = 1
            }
        }
    }
}

// 执行图片删除操作
function executeDelete() {
    if ( thumnailListType !== GlobalVar.ThumbnailType.Trash ){
        albumControl.insertTrash(global.selectedPaths)
    } else {
        albumControl.deleteImgFromTrash(selectedPaths)
        selectAll(false)
        global.sigFlushRecentDelView()
    }
}

// 执行全屏预览
function executeFullScreen() {
    if (root.visibility !== Window.FullScreen && selectedUrls.length > 0) {
        showFullScreen()

        mainStack.sourcePaths = dir.allUrls()
        mainStack.currentIndex = -1
        mainStack.currentIndex = dir.allUrls().indexOf(selectedUrls[0])
        mainStack.currentWidgetIndex = 1
        global.stackControlLastCurrent = global.stackControlCurrent
        global.stackControlCurrent = 1

    }
}

// 执行图片打印
function executePrint() {
    fileControl.showPrintDialog(global.selectedPaths)
}

// 执行幻灯片放映
function excuteSlideShow() {
    if (selectedUrls.length > 0) {
        var allUrls = dir.allUrls()
        stackControl.startMainSliderShow(allUrls, allUrls.indexOf(selectedUrls[0]))
    }
}

// 执行导出图片
function excuteExport() {
    if (global.selectedPaths.length > 1) {
        var bRet = albumControl.getFolders(global.selectedPaths)
        if (bRet)
            DTK.sendMessage(thumbnailImage, qsTr("Export successful"), "notify_checked")
        else
            DTK.sendMessage(thumbnailImage, qsTr("Export failed"), "warning")
    } else {
        exportdig.setParameter(model.data(dir.selectedIndexes[0], "url").toString(), thumbnailImage)
        exportdig.show()
    }
}

// 执行图片复制
function executeCopy() {
    if (global.selectedPaths.length > 0)
        fileControl.copyImage(global.selectedPaths)
}

// 执行从相册移除
function executeRemoveFromAlbum() {
    if (selectedUrls.length > 0) {
        albumControl.removeFromAlbum(global.currentCustomAlbumUId, selectedUrls)
        global.sigFlushCustomAlbumView(global.currentCustomAlbumUId)
    }
}

// 执行收藏操作
function executeFavorite() {
    albumControl.insertIntoAlbum(0, global.selectedPaths)
    global.bRefreshFavoriteIconFlag = !global.bRefreshFavoriteIconFlag

    // 若当前视图为我的收藏，需要实时刷新我的收藏列表内容
    if (global.currentViewIndex === GlobalVar.ThumbnailViewType.Favorite && global.currentCustomAlbumUId === 0) {
        global.sigFlushCustomAlbumView(global.currentCustomAlbumUId)
    }
}

// 执行取消收藏操作
function executeUnFavorite() {
    albumControl.removeFromAlbum(0, global.selectedPaths)
    global.bRefreshFavoriteIconFlag = !global.bRefreshFavoriteIconFlag

    // 若当前视图为我的收藏，需要实时刷新我的收藏列表内容
    if (global.currentViewIndex === GlobalVar.ThumbnailViewType.Favorite && global.currentCustomAlbumUId === 0) {
        global.sigFlushCustomAlbumView(global.currentCustomAlbumUId)
    }
}

// 执行旋转操作
function executeRotate(angle) {
    if (global.selectedPaths.length > 0) {
        fileControl.rotateFile(global.selectedPaths, angle)
    }
}

// 执行设置壁纸操作
function executeSetWallpaper() {
    if (global.selectedPaths.length > 0)
        fileControl.setWallpaper(global.selectedPaths[0])
}

// 执行在文管中显示操作
function executeDisplayInFileManager() {
    if (global.selectedPaths.length > 0)
        fileControl.displayinFileManager(global.selectedPaths[0])
}

// 执行图片恢复操作
function executeRestore() {
    if (selectedPaths.length > 0) {
        albumControl.recoveryImgFromTrash(selectedPaths)
        selectAll(false)
        global.sigFlushRecentDelView()
    }
}

// 执行照片信息查看
function executeViewPhotoInfo() {
    albumInfomationDig.filePath = global.selectedPaths[0]
    albumInfomationDig.show()
}


// 执行视频信息查看
function executeViewVideoInfo() {
    videoInfomationDig.filePath = global.selectedPaths[0]
    videoInfomationDig.show()
}
