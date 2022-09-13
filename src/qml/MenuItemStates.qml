import QtQuick 2.11
import QtQuick.Controls 2.4

Item {
    property bool isInTrash: global.currentViewIndex === GlobalVar.ThumbnailViewType.RecentlyDeleted
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
    property bool canFavorite: albumControl.canFavorite(global.selectedPaths, global.bRefreshFavoriteIconFlag) && !isInTrash
    property bool canRotate: true
    property bool canDisplayInFolder: false
    property bool canPrint: true

    //已选的图片状态检查
    function updateMenuItemStates() {
        haveImage = fileControl.haveImage(global.selectedPaths)
        haveVideo = fileControl.haveVideo(global.selectedPaths)
        canFullScreen = (global.selectedPaths.length === 1 && fileControl.pathExists(global.selectedPaths[0]))
                && fileControl.isImage(global.selectedPaths[0]) && !isInTrash
        canSlideShow = ((global.selectedPaths.length === 1 && fileControl.pathExists(global.selectedPaths[0])) || haveImage)
                && fileControl.isImage(global.selectedPaths[0]) && !isInTrash
        canView = global.selectedPaths.length === 1 && !isInTrash
        canExport= global.selectedPaths.length >= 1 && fileControl.pathExists(global.selectedPaths[0]) && haveImage && !haveVideo && !isInTrash
        canCopy = global.selectedPaths.length >= 1 && !isInTrash
        canDelete = fileControl.isCanDelete(global.selectedPaths)
        canViewPhotoInfo = global.selectedPaths.length === 1 && fileControl.isImage(global.selectedPaths[0])
        canViewVideoInfo = global.selectedPaths.length === 1 && !fileControl.isImage(global.selectedPaths[0])
        canWallpaper = global.selectedPaths.length === 1 && fileControl.isSupportSetWallpaper(global.selectedPaths[0]) && !isInTrash
        canFavorite = albumControl.canFavorite(global.selectedPaths) && !isInTrash
        canRotate = fileControl.isRotatable(global.selectedPaths)
        canDisplayInFolder = global.selectedPaths.length === 1 && fileControl.pathExists(global.selectedPaths[0]) && !isInTrash
        canPrint = fileControl.isCanPrint(global.selectedPaths) && global.selectedPaths.length === 1 && !isInTrash
    }
}
