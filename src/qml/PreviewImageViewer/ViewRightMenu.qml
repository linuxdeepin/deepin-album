// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import org.deepin.dtk 1.0
import org.deepin.image.viewer 1.0 as IV

import "./Utils"

Menu {
    id: optionMenu

    property url imageSource: GControl.currentSource
    property bool isNullImage: imageInfo.type === IV.Types.NullImage
    property bool readable: !isNullImage && fileControl.isCanReadable(imageSource)
    property bool renamable: !isNullImage && fileControl.isCanRename(imageSource)
    property bool rotatable: !isNullImage && fileControl.isRotatable(imageSource)
    property bool deletable: !isNullImage && fileControl.isCanDelete(imageSource)
    property bool supportOcr: !isNullImage && fileControl.isCanSupportOcr(imageSource)
    property bool supportWallpaper: !isNullImage && fileControl.isSupportSetWallpaper(imageSource)
    property bool canExport: fileControl.pathExists(imageSource.toString()) && fileControl.isImage(imageSource.toString()) && !fileControl.isVideo(imageSource.toString())
    property bool canFavorite: albumControl.canFavorite((imageSource.toString()), GStatus.bRefreshFavoriteIconFlag)

    x: 250
    y: 600
    maxVisibleItems: 20

    RightMenuItem {
        id: rightFullscreen

        function switchFullScreen() {
            GStatus.showFullScreen = !GStatus.showFullScreen
        }

        text: !window.isFullScreen ? qsTr("Fullscreen") : qsTr("Exit fullscreen")
        onTriggered: switchFullScreen()

        Shortcut {
            enabled: stackView.visible
            sequence: "F11"
            onActivated: rightFullscreen.switchFullScreen()
        }

        Shortcut {
            enabled: window.isFullScreen && stackView.visible && GStatus.stackPage !== Number(IV.Types.SliderShowPage)
            sequence: "Esc"
            onActivated: rightFullscreen.switchFullScreen()
        }
    }

    RightMenuItem {
        text: qsTr("Print")
        visible: !isNullImage

        onTriggered: {
            optionMenu.close()
            fileControl.showPrintDialog(imageSource)
        }

        Shortcut {
            sequence: "Ctrl+P"
            enabled: !isNullImage && stackView.visible
            onActivated: {
                optionMenu.close()
                if (parent.visible && GStatus.stackPage === Number(IV.Types.ImageViewPage)) {
                    fileControl.showPrintDialog(imageSource.toString())
                }
            }
        }
    }

    RightMenuItem {
        text: qsTr("Extract text")
        visible: supportOcr && !isNullImage

        onTriggered: {
            GControl.submitImageChangeImmediately()
            fileControl.ocrImage(imageSource, GControl.currentFrameIndex)
        }

        Shortcut {
            sequence: "Alt+O"
            enabled: supportOcr && !isNullImage
            onActivated: {
                if (parent.visible && GStatus.stackPage === Number(IV.Types.ImageViewPage)) {
                    GControl.submitImageChangeImmediately()
                    fileControl.ocrImage(imageSource, GControl.currentFrameIndex)
                }
            }
        }
    }

    RightMenuItem {
        text: qsTr("Slide show")

        onTriggered: {
            stackView.switchSliderShow()
        }

        Shortcut {
            enabled: stackView.visible
            sequence: "F5"
            onActivated: {
                if (parent.visible && GStatus.stackPage !== Number(IV.Types.OpenImagePage)) {
                    stackView.switchSliderShow()
                }
            }
        }
    }

    MenuSeparator {
        id: firstSeparator
    }

    //导出图片为其它格式
    RightMenuItem {
        text: qsTr("Export")
        visible: canExport && !menuItemStates.isInTrash && fileControl.isAlbum()
        onTriggered: {
            excuteExport()
        }

        Shortcut {
            enabled: stackView.visible && canExport && !menuItemStates.isInTrash && fileControl.isAlbum()
            sequence : "Ctrl+E"
            onActivated : {
                excuteExport()
            }
        }
    }

    RightMenuItem {
        text: qsTr("Copy")
        visible: readable

        onTriggered: {
            if (parent.visible) {
                GControl.submitImageChangeImmediately()
                fileControl.copyImage(imageSource.toString())
            }
        }

        Shortcut {
            sequence: "Ctrl+C"
            enabled: readable && stackView.visible
            onActivated: {
                if (parent.visible && GStatus.stackPage === Number(IV.Types.ImageViewPage)) {
                    GControl.submitImageChangeImmediately()
                    fileControl.copyImage(imageSource.toString())
                }
            }
        }
    }

    RightMenuItem {
        text: qsTr("Rename")
        visible: renamable

        onTriggered: {
            renamedialog.show()
        }

        Shortcut {
            sequence: "F2"
            enabled: renamable
            onActivated: {
                if (parent.visible && GStatus.stackPage === Number(IV.Types.ImageViewPage)) {
                    renamedialog.show()
                }
            }
        }
    }

    RightMenuItem {
        text: qsTr("Delete")
        enabled: deletable
        visible: deletable

        onTriggered: {
            toolBarThumbnailListView.deleteCurrentImage()
        }

        Shortcut {
            sequence: "Delete"
            enabled: deletable && stackView.visible
            onActivated: {
                if (parent.visible && GStatus.stackPage === Number(IV.Types.ImageViewPage)) {
                    toolBarThumbnailListView.deleteCurrentImage()
                }
            }
        }
    }

    //分割条-收藏
    MenuSeparator {
        visible: !menuItemStates.isInTrash && fileControl.isAlbum()
        // 不显示分割条时调整高度，防止菜单项间距不齐
        height: visible ? firstSeparator.height : 0
    }

    //添加到我的收藏
    RightMenuItem {
        id: favoriteAction
        text: qsTr("Favorite")
        visible: !menuItemStates.isInTrash && canFavorite && fileControl.isAlbum()
        onTriggered: {
            imageViewer.executeFavorite()
        }
    }

    //从我的收藏中移除
    RightMenuItem {
        id: unFavoriteAction
        text: qsTr("Unfavorite")
        visible: !menuItemStates.isInTrash && !canFavorite && fileControl.isAlbum()
        onTriggered: {
            imageViewer.executeUnFavorite()
        }
    }

    // 不允许无读写权限时上方选项已屏蔽，不展示此分割条
    MenuSeparator {
        // 不显示分割条时调整高度，防止菜单项间距不齐
        height: visible ? firstSeparator.height : 0
        visible: fileControl.isCanReadable(imageSource) || fileControl.isCanDelete(imageSource)
    }

    RightMenuItem {
        text: qsTr("Rotate clockwise")
        visible: rotatable

        onTriggered: {
            imageViewer.rotateImage(90)
        }

        Shortcut {
            sequence: "Ctrl+R"
            enabled: rotatable && stackView.visible
            onActivated: {
                if (parent.visible && GStatus.stackPage === Number(IV.Types.ImageViewPage)) {
                    imageViewer.rotateImage(90)
                }
            }
        }
    }

    RightMenuItem {
        text: qsTr("Rotate counterclockwise")
        visible: rotatable

        onTriggered: {
            imageViewer.rotateImage(-90)
        }

        Shortcut {
            sequence: "Ctrl+Shift+R"
            enabled: rotatable && stackView.visible
            onActivated: {
                if (parent.visible && GStatus.stackPage === Number(IV.Types.ImageViewPage)) {
                    imageViewer.rotateImage(-90)
                }
            }
        }
    }

    RightMenuItem {
        id: enableNavigation

        visible: !isNullImage
                 && window.height > GStatus.minHideHeight
                 && window.width > GStatus.minWidth
        text: !GStatus.enableNavigation ? qsTr("Show navigation window") : qsTr("Hide navigation window")

        onTriggered: {
            if (!parent.visible) {
                return
            }

            GStatus.enableNavigation = !GStatus.enableNavigation
        }
    }

    RightMenuItem {
        text: qsTr("Set as wallpaper")
        visible: supportWallpaper

        onTriggered: {
            GControl.submitImageChangeImmediately()
            fileControl.setWallpaper(imageSource)
        }

        Shortcut {
            sequence: "Ctrl+F9"
            enabled: supportWallpaper && stackView.visible
            onActivated: {
                if (parent.visible && GStatus.stackPage === Number(IV.Types.ImageViewPage)) {
                    GControl.submitImageChangeImmediately()
                    fileControl.setWallpaper(imageSource)
                }
            }
        }
    }

    RightMenuItem {
        text: qsTr("Display in file manager")

        onTriggered: {
            fileControl.displayinFileManager(imageSource)
        }

        Shortcut {
            sequence: "Alt+D"
            enabled: parent.visible
            onActivated: {
                if (parent.visible && GStatus.stackPage === Number(IV.Types.ImageViewPage)) {
                    fileControl.displayinFileManager(imageSource)
                }
            }
        }
    }

    RightMenuItem {
        text: qsTr("Image info")

        onTriggered: {
            infomationDig.show()
        }

        Shortcut {
            sequence: "Ctrl+I"
            enabled: parent.visible
            onActivated: {
                if (parent.visible && GStatus.stackPage === Number(IV.Types.ImageViewPage)) {
                    infomationDig.show()
                }
            }
        }
    }

    IV.ImageInfo {
        id: imageInfo
        source: imageSource

        onStatusChanged: {
            if (IV.ImageInfo.Ready === imageInfo.status) {
                isNullImage = (imageInfo.type === IV.Types.NullImage)
            } else {
                isNullImage = true
            }
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
        } else{
            exportdialog.setParameter(imageSource.toString(), imageViewer)
            exportdialog.show()
        }
    }
}
