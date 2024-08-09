// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

// 执行图片查看操作
function executeViewImageCutSwitch() {
    if (thumnailListType !== Album.Types.ThumbnailTrash) {
        var indexes = thumbnailModel.selectedIndexes
        if (indexes.length > 0) {
            if (fileControl.isVideo(thumbnailModel.data(indexes[0], "url").toString())) {
                albumControl.openDeepinMovie(thumbnailModel.data(indexes[0], "url").toString())
            } else {
                var allUrls = thumbnailModel.allUrls()
                mainStack.sourcePaths = allUrls
                mainStack.currentIndex = -1
                mainStack.currentIndex = indexes[0]
                mainStack.currentWidgetIndex = 1
                GStatus.stackControlCurrent = 1
            }
        }
    }
}

// 执行图片查看操作
function executeViewImage(x, y, w, h) {
    if (thumnailListType !== Album.Types.ThumbnailTrash) {
        var indexes = thumbnailModel.selectedIndexes
        if (indexes.length > 0) {
            if (fileControl.isVideo(thumbnailModel.data(indexes[0], "url").toString())) {
                albumControl.openDeepinMovie(thumbnailModel.data(indexes[0], "url").toString())
            } else {
                var allUrls = thumbnailModel.allUrls()
                mainStack.sourcePaths = allUrls
                mainStack.currentIndex = -1
                mainStack.currentIndex = indexes[0]
                mainStack.currentWidgetIndex = 1
                GStatus.stackControlCurrent = 1
                window.sigMoveCenter(x, y, w, h)
                window.sigShowToolBar()
            }
        }
    }
}
// 执行图片删除操作
function executeDelete() {
    if ( thumnailListType !== Album.Types.ThumbnailTrash ){
        albumControl.insertTrash(GStatus.selectedPaths)
        thumbnailModel.clearSelection()
    } else {
        albumControl.deleteImgFromTrash(selectedPaths)
        selectAll(false)
        GStatus.sigFlushRecentDelView()
    }
}

// 执行全屏预览
function executeFullScreen() {
    if (window.visibility !== Window.FullScreen && selectedUrls.length > 0) {
        showFullScreen()

        mainStack.sourcePaths = thumbnailModel.allUrls()
        mainStack.currentIndex = -1
        mainStack.currentIndex = thumbnailModel.allUrls().indexOf(selectedUrls[0])
        mainStack.currentWidgetIndex = 1
        GStatus.stackControlLastCurrent = GStatus.stackControlCurrent
        GStatus.stackControlCurrent = 1
    }
}

// 执行图片打印
function executePrint() {
    fileControl.showPrintDialog(GStatus.selectedPaths)
}

// 执行幻灯片放映
function excuteSlideShow() {
    if (selectedUrls.length > 0) {
        var allUrls = thumbnailModel.allUrls()
        stackControl.startMainSliderShow(allUrls, allUrls.indexOf(selectedUrls[0]))
    }
}

// 执行导出图片
function excuteExport() {
    if (GStatus.selectedPaths.length > 1) {
        var bRet = albumControl.getFolders(GStatus.selectedPaths)
        if (bRet)
            DTK.sendMessage(thumbnailImage, qsTr("Export successful"), "notify_checked")
        else
            DTK.sendMessage(thumbnailImage, qsTr("Export failed"), "warning")
    } else {
        exportdig.setParameter(model.data(thumbnailModel.selectedIndexes[0], "url").toString(), thumbnailImage)
        exportdig.show()
    }
}

// 执行图片复制
function executeCopy() {
    if (GStatus.selectedPaths.length > 0)
        fileControl.copyImage(GStatus.selectedPaths)
}

// 执行从相册移除
function executeRemoveFromAlbum() {
    if (selectedUrls.length > 0) {
        albumControl.removeFromAlbum(GStatus.currentCustomAlbumUId, selectedUrls)
        GStatus.sigFlushCustomAlbumView(GStatus.currentCustomAlbumUId)
    }
}

// 执行收藏操作
function executeFavorite() {
    albumControl.insertIntoAlbum(0, GStatus.selectedPaths)
    GStatus.bRefreshFavoriteIconFlag = !GStatus.bRefreshFavoriteIconFlag

    // 若当前视图为我的收藏，需要实时刷新我的收藏列表内容
    if (GStatus.currentViewType === Album.Types.ViewFavorite && GStatus.currentCustomAlbumUId === 0) {
        GStatus.sigFlushCustomAlbumView(GStatus.currentCustomAlbumUId)
    }
}

// 执行取消收藏操作
function executeUnFavorite() {
    albumControl.removeFromAlbum(0, GStatus.selectedPaths)
    GStatus.bRefreshFavoriteIconFlag = !GStatus.bRefreshFavoriteIconFlag

    // 若当前视图为我的收藏，需要实时刷新我的收藏列表内容
    if (GStatus.currentViewType === Album.Types.ViewFavorite && GStatus.currentCustomAlbumUId === 0) {
        GStatus.sigFlushCustomAlbumView(GStatus.currentCustomAlbumUId)
    }
}

// 执行旋转操作
function executeRotate(angle) {
    if (GStatus.selectedPaths.length > 0) {
        fileControl.rotateFile(GStatus.selectedPaths, angle)
    }
}

// 执行设置壁纸操作
function executeSetWallpaper() {
    if (GStatus.selectedPaths.length > 0)
        fileControl.setWallpaper(GStatus.selectedPaths[0])
}

// 执行在文管中显示操作
function executeDisplayInFileManager() {
    if (GStatus.selectedPaths.length > 0)
        fileControl.displayinFileManager(GStatus.selectedPaths[0])
}

// 执行图片恢复操作
function executeRestore() {
    if (selectedPaths.length > 0) {
        albumControl.recoveryImgFromTrash(selectedPaths)
        selectAll(false)
        GStatus.sigFlushRecentDelView()
    }
}

// 执行照片信息查看
function executeViewPhotoInfo() {
    albumInfomationDig.filePath = GStatus.selectedPaths[0]
    albumInfomationDig.show()
}


// 执行视频信息查看
function executeViewVideoInfo() {
    videoInfomationDig.filePath = GStatus.selectedPaths[0]
    videoInfomationDig.show()
}
