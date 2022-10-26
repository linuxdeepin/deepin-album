import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.4
//import QtQuick.Controls 1.4
import QtQuick.Layouts 1.11
import org.deepin.dtk 1.0

Menu {
    x: 250; y: 600
    id: option_menu

    maxVisibleItems: 20

    property bool canExport: fileControl.pathExists(source) && fileControl.isImage(source) && !fileControl.isVideo(source)

    RightMenuItem {
        id : right_fullscreen
        text: root.visibility != Window.FullScreen ? qsTr("Fullscreen") : qsTr("Exit fullscreen")

        onTriggered : showFulltimer.start()
        Shortcut {
            enabled: stackView.visible
            sequence : "F11"
            onActivated : {
                root.visibility !== Window.FullScreen ? imageViewer.showPanelFullScreen() : imageViewer.escBack()
            }
        }
        Shortcut {
            enabled: (root.visibility === Window.FullScreen && stackView.visible && stackView.currentWidgetIndex !== 2) ? true : false
            sequence :  "Esc"
            onActivated : root.visibility !== Window.FullScreen ? imageViewer.showPanelFullScreen() : imageViewer.escBack()
        }
    }


    RightMenuItem {
        text: qsTr("Print")
        visible: !CodeImage.imageIsNull(source)
        onTriggered: {
            fileControl.showPrintDialog(mainView.source)
        }
        Shortcut {
            sequence: "Ctrl+P"
            enabled: !CodeImage.imageIsNull(source) && stackView.visible
            onActivated:  {
                if (parent.visible && stackView.currentWidgetIndex == 1)
                {
                    fileControl.showPrintDialog(mainView.source)
                }
            }
        }
    }

    RightMenuItem {
        text: qsTr("Extract text")
        visible: fileControl.isCanSupportOcr(source) && !CodeImage.imageIsNull(source)
        onTriggered: {
            fileControl.ocrImage(source)
        }
        Shortcut {
            sequence: "Alt+O"
            enabled: fileControl.isCanSupportOcr(source) && !CodeImage.imageIsNull(source)
            onActivated: {
                if (parent.visible && stackView.currentWidgetIndex == 1)
                {
                    fileControl.ocrImage(source)
                }
            }
        }
    }

    RightMenuItem {

        text: qsTr("Slide show")
        onTriggered: {
            imageViewer.startSliderShow()
            showfullAnimation.start()
        }
        Shortcut {
            enabled: stackView.visible
            sequence: "F5"
            onActivated: {
                if (parent.visible && stackView.currentWidgetIndex != 0)
                {
                    imageViewer.startSliderShow()
                    showfullAnimation.start()
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
        visible: fileControl.isCanReadable(source)
        onTriggered: {
            if( parent.visible ){
                fileControl.copyImage(source)
            }
        }
        Shortcut {
            sequence: "Ctrl+C"
            enabled: fileControl.isCanReadable(source) && stackView.visible
            onActivated: {
                if (parent.visible && stackView.currentWidgetIndex == 1)
                {
                    fileControl.copyImage(source)
                }
            }
        }
    }

    RightMenuItem {

        text: qsTr("Rename")
        visible: fileControl.isCanRename(source)
        onTriggered: {
            var x = parent.mapToGlobal(0, 0).x + parent.width / 2 - 190
            var y = parent.mapToGlobal(0, 0).y + parent.height / 2 - 89
            renamedialog.setX(x)
            renamedialog.setY(y)
            renamedialog.getFileName(fileControl.slotGetFileName(source))
            renamedialog.getFileSuffix(fileControl.slotFileSuffix(source))
            renamedialog.show()
        }
        Shortcut {
            sequence: "F2"
            // 判断文件是否允许重命名
            enabled: fileControl.isCanRename(source)
            onActivated: {
                if (parent.visible && stackView.currentWidgetIndex == 1)
                {
                    var x = parent.mapToGlobal(0, 0).x + parent.width / 2 - 190
                    var y = parent.mapToGlobal(0, 0).y + parent.height / 2 - 89
                    renamedialog.setX(x)
                    renamedialog.setY(y)
                    renamedialog.getFileName(fileControl.slotGetFileName(source))
                    renamedialog.getFileSuffix(fileControl.slotFileSuffix(source))
                    renamedialog.show()
                }
            }
        }
    }

    RightMenuItem {
        text: qsTr("Delete")
        enabled: fileControl.isCanDelete(source)
        visible: fileControl.isCanDelete(source)
        onTriggered: {
            toolBarthumbnailListView.deleteCurrentImage()
        }
        Shortcut {
            sequence: "Delete"
	    enabled: fileControl.isCanDelete(source) && stackView.visible
            onActivated: {
                if (parent.visible && stackView.currentWidgetIndex == 1)
                {
                    toolBarthumbnailListView.deleteCurrentImage()
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
        visible: !menuItemStates.isInTrash && imageViewer.canFavorite && fileControl.isAlbum()
        onTriggered: {
            imageViewer.executeFavorite()
        }
    }

    //从我的收藏中移除
    RightMenuItem {
        id: unFavoriteAction
        text: qsTr("Unfavorite")
        visible: !menuItemStates.isInTrash && !imageViewer.canFavorite && fileControl.isAlbum()
        onTriggered: {
            imageViewer.executeUnFavorite()
        }
    }

    // 不允许无读写权限时上方选项已屏蔽，不展示此分割条
    MenuSeparator {
        // 不显示分割条时调整高度，防止菜单项间距不齐
        height: visible ? firstSeparator.height : 0
        visible: fileControl.isCanReadable(source)
                 || fileControl.isCanDelete(source)
    }

    RightMenuItem {
        text: qsTr("Rotate clockwise")
        visible: !CodeImage.imageIsNull(imageViewer.source) && fileControl.isRotatable(imageViewer.source)
        onTriggered: {
            imageViewer.rotateImage(90)
        }

        Shortcut {
            sequence: "Ctrl+R"
            enabled: !CodeImage.imageIsNull(imageViewer.source) && fileControl.isRotatable(imageViewer.source) && stackView.visible
            onActivated: {
                if (parent.visible && stackView.currentWidgetIndex == 1)
                {
                    imageViewer.rotateImage(90)
                }
            }
        }
    }

    RightMenuItem {
        text: qsTr("Rotate counterclockwise")
        visible: !CodeImage.imageIsNull(imageViewer.source) && fileControl.isRotatable(imageViewer.source)
        onTriggered: {
            imageViewer.rotateImage(-90)
        }
        Shortcut {
            sequence: "Ctrl+Shift+R"
            enabled: !CodeImage.imageIsNull(imageViewer.source) && fileControl.isRotatable(imageViewer.source) && stackView.visible
            onActivated: {
                if (parent.visible && stackView.currentWidgetIndex == 1)
                {
                    imageViewer.rotateImage(-90)
                }
            }
        }
    }

    RightMenuItem {

        id : showNavigation
        visible: !CodeImage.imageIsNull(source) && currentScale >1 && root.height > global.minHideHeight && root.width > global.minWidth
        text: !imageViewer.isNavShow ? qsTr("Show navigation window") : qsTr("Hide navigation window")
        onTriggered : {
            if(!parent.visible){
                return
            }

            if (imageViewer.isNavShow) {
                imageViewer.isNavShow = false
                idNavWidget.visible = false
            } else {
                imageViewer.isNavShow = true
                idNavWidget.visible = true
                if(m_NavX === 0 && m_NavY === 0) {
                    // 设置蒙皮信息
                    idNavWidget.setRectPec(view.currentItem.scale, imageViewer.viewImageWidthRatio, imageViewer.viewImageHeightRatio)
                } else {
                    idNavWidget.setRectLocation(m_NavX, m_NavY)
                }
            }
        }
    }

    RightMenuItem {

        text: qsTr("Set as wallpaper")
        visible: fileControl.isSupportSetWallpaper(source)
        onTriggered: {
            fileControl.setWallpaper(source)
        }
        Shortcut {
            sequence: "Ctrl+F9"
            enabled: fileControl.isSupportSetWallpaper(source) && stackView.visible
            onActivated: {
                if (parent.visible && stackView.currentWidgetIndex == 1)
                {
                    fileControl.setWallpaper(source)
                }
            }
        }
    }

    RightMenuItem {

        text: qsTr("Display in file manager")
        onTriggered: {
            fileControl.displayinFileManager(source)
        }
        Shortcut {
            enabled: stackView.visible
            sequence: "Alt+D"
            onActivated: {
                if (parent.visible && stackView.currentWidgetIndex == 1)
                {
                    fileControl.displayinFileManager(source)
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
            enabled: stackView.visible
            sequence: "Ctrl+I"
            onActivated: {
                if (parent.visible && stackView.currentWidgetIndex == 1)
                {
                    infomationDig.show()
                }
            }
        }
    }

    // 执行导出图片
    function excuteExport() {
        if (global.selectedPaths.length > 1) {
            var bRet = albumControl.getFolders(global.selectedPaths)
            if (bRet)
                DTK.sendMessage(thumbnailImage, qsTr("Export successful"), "checked")
            else
                DTK.sendMessage(thumbnailImage, qsTr("Export failed"), "warning")
        } else{
            exportdig.setParameter(imageViewer.source, imageViewer)
            exportdig.show()
        }
    }
}
