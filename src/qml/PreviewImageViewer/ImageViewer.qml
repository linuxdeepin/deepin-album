// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQuick.Shapes 1.11
import org.deepin.dtk 1.0
import org.deepin.image.viewer 1.0 as IV

import "./ImageDelegate"
import "../Control"

Item {
    id: imageViewer

    // Note: 对于SVG、动图等特殊类型图片，使用 targeImage 获取的图片 sourceSize 存在差异，
    // 可能为零或导致缩放模糊，调整为使用从文件中读取的原始大小计算。
    // 图片旋转后同样会交换宽度和高度，更新缓存的图片源宽高信息
    property alias targetImageInfo: currentImageInfo
    // Image 类型的对象，空图片、错误图片、消失图片等异常为 null
    property alias targetImage: view.currentImage
    property bool targetImageReady: (null !== view.currentImage)
                                    && (Image.Ready === view.currentImage.status)

    // current rotate
    property int currentRotate: 0

    // 记录图像缩放，用于在窗口缩放时，根据前后窗口变化保持图片缩放比例
    property bool enableChangeDisplay: true
    property real lastDisplayScaleWidth: 0

    // 窗口拖拽大小变更时保持图片的显示缩放比例
    function keepImageDisplayScale() {
        if (!targetImageReady) {
            return
        }

        // 当前缩放比例与匹配窗口的图片缩放比例比较，不一致则保持缩放比例
        if (Math.abs(targetImage.scale - 1.0) > Number.EPSILON) {
            if (0 !== lastDisplayScaleWidth) {
                // Note: 拖拽窗口时将保持 scale ，但 paintedWidth / paintedHeight 将变更
                // 因此在此处设置缩放比例时屏蔽重复设置，以保留缩放比例
                enableChangeDisplay = false
                targetImage.scale = lastDisplayScaleWidth / targetImage.paintedWidth
                enableChangeDisplay = true
            } else {
                lastDisplayScaleWidth = targetImage.paintedWidth * targetImage.scale
            }
        } else {
            // 一致则保持匹配窗口
            fitWindow()
        }
    }

    function showScaleFloatLabel() {
        // 不存在的图片不弹出缩放提示框
        if (!targetImageReady) {
            return
        }

        // 图片实际缩放比值 绘制像素宽度 / 图片原始像素宽度
        var readableScale = targetImage.paintedWidth * targetImage.scale
                / targetImageInfo.width * 100
        if (readableScale.toFixed(0) > 2000 && readableScale.toFixed(0) <= 3000) {
            floatLabel.displayStr = "2000%"
        } else if (readableScale.toFixed(0) < 2 && readableScale.toFixed(0) >= 0) {
            floatLabel.displayStr = "2%"
        } else if (readableScale.toFixed(0) >= 2 && readableScale.toFixed(0) <= 2000) {
            floatLabel.displayStr = readableScale.toFixed(0) + "%"
        }

        floatLabel.visible = true
    }

    function fitImage() {
        if (targetImageReady) {
            // 按图片原始大小执行缩放
            targetImage.scale = targetImageInfo.width / targetImage.paintedWidth
        }
    }

    function fitWindow() {
        // 默认状态的图片即适应窗口大小(使用 Image.PreserveAspectFit)
        if (targetImageReady) {
            targetImage.scale = 1.0
        }
    }

    function rotateImage(angle) {
        if (targetImageReady) {
            GControl.currentRotation += angle
        }
    }

    // 触发全屏展示图片
    function showPanelFullScreen() {
        GStatus.showImageInfo = false

        showFullScreen()
        view.contentItem.forceActiveFocus()
        showfullAnimation.start()
    }

    // 退出全屏展示图片
    function escBack() {
        GStatus.showImageInfo = false

        showNormal()

        // 在相册主界面进入全屏，按Esc需要回到相册主界面
        if (GStatus.stackControlLastCurrent === 0) {
            GStatus.stackControlCurrent = GStatus.stackControlLastCurrent
            GStatus.stackControlLastCurrent = -1
            // 强制刷新一次图片
            var urls = []
            GControl.setImageFiles(urls, "")
            fileControl.resetImageFiles(urls)
            GControl.currentSource = ""
            window.title = ""
            return
        }

        showfullAnimation.start()

//        if (GStatus.stackControlCurrent === 2) {
//            mainSliderShow.outSliderShow()
//        }
    }

    onWidthChanged: keepImageDisplayScale()
    onHeightChanged: keepImageDisplayScale()

    // 图片状态变更时触发
    onTargetImageReadyChanged: {
        showScaleFloatLabel()

        // 重置保留的缩放状态
        lastDisplayScaleWidth = 0
    }

    Connections {
        enabled: targetImageReady
        target: targetImage
        ignoreUnknownSignals: true

        onScaleChanged: {
            // 图片实际缩放比值 绘制像素宽度 / 图片原始像素宽度
            var readableScale = targetImage.paintedWidth * targetImage.scale
                    / targetImageInfo.width * 100
            // 缩放限制在 2% ~ 2000% ，变更后再次进入此函数处理
            if (readableScale < 2) {
                targetImage.scale = targetImageInfo.width * 0.02 / targetImage.paintedWidth
                return
            } else if (readableScale > 2000) {
                targetImage.scale = targetImageInfo.width * 20 / targetImage.paintedWidth
                return
            }

            // 处于保持效果缩放状态时，保留之前的缩放比例
            if (enableChangeDisplay) {
                lastDisplayScaleWidth = targetImage.paintedWidth * targetImage.scale
                // 显示缩放框
                showScaleFloatLabel()
            }
        }
    }

    // 触发切换全屏状态
    Connections {
        target: GStatus

        onShowFullScreenChanged: {
            if (window.isFullScreen !== GStatus.showFullScreen) {
                // 关闭详细信息窗口
                GStatus.showImageInfo = false

                GStatus.showFullScreen ? showPanelFullScreen() : escBack()
            }
        }
    }

    PropertyAnimation {
        id: showfullAnimation

        target: parent.Window.window
        from: 0
        to: 1
        property: "opacity"
        duration: 200
        easing.type: Easing.InExpo

        onRunningChanged: {
            GStatus.fullScreenAnimating = running
            // 动画结束时，重置缩放状态
            if (!running && targetImageReady) {
                // 匹配缩放处理
                if (targetImageInfo.height < targetImage.height) {
                    targetImage.scale = targetImageInfo.width / targetImage.paintedWidth
                } else {
                    targetImage.scale = 1.0
                }
            }
        }
    }

    // 执行收藏操作
    function executeFavorite() {
        albumControl.insertIntoAlbum(0, GControl.currentSource.toString())
        GStatus.bRefreshFavoriteIconFlag = !GStatus.bRefreshFavoriteIconFlag
    }

    // 执行取消收藏操作
    function executeUnFavorite() {
        albumControl.removeFromAlbum(0, GControl.currentSource.toString())
        GStatus.bRefreshFavoriteIconFlag = !GStatus.bRefreshFavoriteIconFlag
    }

    //收藏/取消收藏
    Shortcut {
        enabled: visible
        sequence: "."
        onActivated: {
            if (!menuItemStates.isInTrash && fileControl.isAlbum()) {
                if (canFavorite)
                    executeFavorite()
                else
                    executeUnFavorite()
            }
        }
    }

    //缩放快捷键
    Shortcut {
        sequence: "Ctrl+="
        onActivated: {
            targetImage.scale = targetImage.scale / 0.9
        }
    }

    Shortcut {
        sequence: "Ctrl+-"
        onActivated: {
            targetImage.scale = targetImage.scale * 0.9
        }
    }

    Shortcut {
        sequence: "Up"
        onActivated: {
            targetImage.scale = targetImage.scale / 0.9
        }
    }

    Shortcut {
        sequence: "Down"
        onActivated: {
            targetImage.scale = targetImage.scale * 0.9
        }
    }

    Shortcut {
        sequence: "Ctrl+Shift+/"
        onActivated: {
            var screenPos = mapToGlobal(parent.x, parent.y)
            fileControl.showShortcutPanel(
                        screenPos.x + parent.Window.width / 2,
                        screenPos.y + parent.Window.height / 2)
        }
    }

    // 图片滑动视图的上层组件
    Item {
        id: viewBackground
        anchors.fill: parent
    }

    // 图片滑动视图
    ListView {
        id: view

        // 当前展示的 Image 图片对象，空图片、错误图片、消失图片等异常为 undefined
        // 此图片信息用于外部交互缩放、导航窗口等，已标识类型，使用 null !== currentImage 判断
        property Image currentImage: {
            if (view.currentItem) {
                if (view.currentItem.item) {
                    return view.currentItem.item.targetImage
                }
            }
            return null
        }

        // 设置滑动视图的父组件以获取完整的OCR图片信息
        parent: viewBackground
        // WARNING: 目前 ListView 组件屏蔽输入处理，窗口拖拽依赖底层的 ApplicationWindow
        // 因此不允许 ListView 的区域超过标题栏，图片缩放超过显示区域无妨。
        // 显示图片上下边界距边框 50px (标题栏宽度)，若上下间隔不一致时，进行拖拽、导航定位或需减去(间隔差/2)
        // 在全屏时无上下边框
        anchors.horizontalCenter: parent.horizontalCenter
        y: window.isFullScreen ? 0 : GStatus.titleHeight
        height: window.isFullScreen ? parent.height : (parent.height - (GStatus.titleHeight * 2))
        width: parent.width
        cacheBuffer: 200
        interactive: !GStatus.fullScreenAnimating && GStatus.viewInteractive
        preferredHighlightBegin: 0
        preferredHighlightEnd: 0
        highlightRangeMode: ListView.StrictlyEnforceRange
        highlightMoveDuration: 0
        orientation: ListView.Horizontal
        snapMode: ListView.SnapOneItem
        flickDeceleration: 500
        boundsMovement: Flickable.FollowBoundsBehavior
        boundsBehavior: Flickable.StopAtBounds

        currentIndex: GControl.currentIndex
        model: GControl.globalModel
        delegate: Loader {
            id: swipeViewItemLoader

            property url source: model.imageUrl
            property alias frameCount: imageInfo.frameCount

            active: {
                if (ListView.isCurrentItem) {
                    return true
                }
                if (view.currentIndex - 1 === index
                        || view.currentIndex + 1 === index) {
                    return true
                }
                return false
            }
            visible: active
            asynchronous: true
            width: view.width
            height: view.height

            onActiveChanged: {
                if (active && imageInfo.delegateSource) {
                    setSource(imageInfo.delegateSource, {
                                  "source": swipeViewItemLoader.source,
                                  "type": imageInfo.type
                              })
                }
            }

            IV.ImageInfo {
                id: imageInfo

                property url delegateSource
                property bool isCurrentItem: swipeViewItemLoader.ListView.isCurrentItem

                function checkDelegateSource() {
                    if (IV.ImageInfo.Ready !== status
                            && IV.ImageInfo.Error !== status) {
                        return
                    }

                    if (!imageInfo.exists) {
                        delegateSource = "qrc:/qml/PreviewImageViewer/ImageDelegate/NonexistImageDelegate.qml"
                        return
                    }

                    switch (type) {
                    case IV.Types.NormalImage:
                        delegateSource = "qrc:/qml/PreviewImageViewer/ImageDelegate/NormalImageDelegate.qml"
                        return
                    case IV.Types.DynamicImage:
                        delegateSource = "qrc:/qml/PreviewImageViewer/ImageDelegate/DynamicImageDelegate.qml"
                        return
                    case IV.Types.SvgImage:
                        delegateSource = "qrc:/qml/PreviewImageViewer/ImageDelegate/SvgImageDelegate.qml"
                        return
                    case IV.Types.MultiImage:
                        delegateSource = "qrc:/qml/PreviewImageViewer/ImageDelegate/MultiImageDelegate.qml"
                        return
                    default:
                        // Default is damaged image.
                        delegateSource = "qrc:/qml/PreviewImageViewer/ImageDelegate/DamagedImageDelegate.qml"
                        return
                    }
                }

                source: swipeViewItemLoader.source

                onDelegateSourceChanged: {
                    if (swipeViewItemLoader.active && delegateSource) {
                        setSource(delegateSource, {
                                      "source": swipeViewItemLoader.source,
                                      "type": imageInfo.type
                                  })
                    }
                }
                onStatusChanged: checkDelegateSource()
                onIsCurrentItemChanged: checkDelegateSource()

                // InfoChange 在图片文件变更时触发，此时图片文件路径不变，文件内容被替换、删除
                onInfoChanged: {
                    if (isCurrentItem) {
                        GControl.currentFrameIndex = 0
                    }

                    checkDelegateSource()
                    var temp = delegateSource
                    delegateSource = ""
                    delegateSource = temp
                }
            }
        }

        onCurrentIndexChanged: {
            // 当通过界面拖拽导致索引变更，需要调整多页图索引范围
            if (view.currentIndex < GControl.currentIndex) {
                GControl.previousImage()
            } else if (view.currentIndex > GControl.currentIndex) {
                GControl.nextImage()
            }
        }

        onMovementStarted: {
            GStatus.viewFlicking = true
        }

        onMovementEnded: {
            GStatus.viewFlicking = false
        }

        BusyIndicator {
            anchors.centerIn: parent
            width: 48
            height: 48
            running: visible
            visible: {
                if (view.currentItem.status === Loader.Loading) {
                    return true
                } else if (view.currentItem.item) {
                    return view.currentItem.item.status === Image.Loading
                }
                return false
            }
        }
    }

    IV.ImageInfo {
        id: currentImageInfo

        frameIndex: GControl.currentFrameIndex
        source: GControl.currentSource
    }

    FloatingButton {
        id: highlightTextButton

        property bool isHighlight: false

        checked: isHighlight
        width: 50
        height: 50
        visible: false
        parent: imageViewerArea
        z: parent.z + 100
        anchors {
            right: parent.right
            rightMargin: 100
            bottom: parent.bottom
            bottomMargin: thumbnailViewBackGround.height + 20
        }

        DciIcon {
            width: 45
            height: 45
            Layout.alignment: Qt.AlignHCenter | Qt.AlignBottom
            name: "icon_recognition_highlight"
        }

        onClicked: {
            isHighlight = !isHighlight
        }

        // 高亮时不弹出工具栏栏以方便选取
        onCheckedChanged: {
            GStatus.animationBlock = checked
        }
        onVisibleChanged: {
            if (!visible) {
                GStatus.animationBlock = false
            }
        }
    }

    //rename窗口
    ReName {
        id: renamedialog
    }

    // 右键菜单
    ViewRightMenu {
        id: rightMenu

        onClosed: {
            GStatus.showRightMenu = false
            imageViewer.forceActiveFocus()
        }

        Connections {
            target: GStatus
            onShowRightMenuChanged: {
                if (GStatus.showRightMenu) {
                    rightMenu.popup(cursorTool.currentCursorPos())
                    rightMenu.focus = true
                }
            }
        }
    }

    // 图片信息窗口
    Loader {
        id: infomationDig

        function show() {
            GStatus.showImageInfo = true
        }

        active: GStatus.showImageInfo
        asynchronous: true
        // 图片属性信息窗口
        source: "qrc:/qml/PreviewImageViewer/InformationDialog/InformationDialog.qml"
    }

    // 导出窗口
    ExportDialog {
        id: exportdialog
    }

    //导航窗口
    Loader {
        id: naviLoader

        active: GStatus.enableNavigation && null !== targetImage
                && targetImage.scale > 1
        anchors {
            bottom: parent.bottom
            bottomMargin: 109
            left: parent.left
            leftMargin: 15
        }
        width: 150
        height: 112
        sourceComponent: NavigationWidget {
            anchors.fill: parent
            targetImage: view.currentImage
        }
    }
}
