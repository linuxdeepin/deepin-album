// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQml.Models 2.11
import QtQml 2.11
import QtQuick.Shapes 1.10
import org.deepin.dtk 1.0
import "../"
import "../../"
import "../../PreviewImageViewer"
Item {
    id : thumnailListView
    property var viewTitle

    //view依赖的model管理器
    property ListModel thumbnailListModel: ListModel { }

    // 缩略图Item尺寸
    property real itemWidth: realCellWidth
    property real itemHeight: realCellWidth
    // 是否开启滚轮
    property bool enableWheel: true
    //缩略图显示类型，默认为普通模式
    property int thumnailListType: GlobalVar.ThumbnailType.Normal
    //存在框选项
    property bool haveSelect: theView.ism.length > 0
    //已框选全部缩略图
    property bool haveSelectAll: theView.ism.length === count()
    // 已选项个数
    property int haveSelectCount: theView.ism.length
    // 已选路径
    property var selectedPaths: new Array
    // 已选原始路径
    property var selectedOriginPaths: new Array
    //缩略图动态变化
    property int  rowSizeHint: (width - global.thumbnailListRightMargin) / global.cellBaseWidth
    property real realCellWidth: (width - global.thumbnailListRightMargin) / rowSizeHint

    //当前区域时间区间改变信号，字符串可以直接刷新在界面上
    signal timeChanged(string str)
    signal selectedChanged()

    //设置图片缩放等级，传入的参数为外部滑动条的值
    function setPicZoomLevel(level) {
        if(level >= 0 && level <= 9) {
            theView.cellBaseWidth = 80 + level * 10
        } else {
            theView.cellBaseWidth = 80
        }
    }

    //设置全选/全取消缩略图
    function selectAll(doSelect) {
        if (theView.visible) {
            if(doSelect) {
                var len = thumbnailListModel.count
                var tempArray = []
                for(var i = 0;i != len;++i) {
                    tempArray.push(i.toString()) //数据类型必须和代理里面的m_index对齐，否则会判断失败
                }
                theView.ism = tempArray
            } else {
                theView.ism = []
            }
        }
        selectedChanged()
    }

    //初始化选中ism
    function initIsm(paths) {
        var tempArray = []
        for(var i = 0; i != thumbnailListModel.count; ++i) {
            var path = thumbnailListModel.get(i).url
            for(var j = 0; j !== paths.length; ++j) {
                if (paths[j] === path) {
                    tempArray.push(i.toString()) //数据类型必须和代理里面的m_index对齐，否则会判断失败
                    break;
                }
            }
        }
        theView.ism = tempArray
    }

    onVisibleChanged: {
        if (visible) {
            theView.displayFlushHelper = asynImageProvider.getLoadMode()
        }
    }

    //强制重新刷新整个缩略图界面
    function fouceUpdate() {
        if (thumnailListView && visible) {
            theView.displayFlushHelper = asynImageProvider.getLoadMode()
        }
    }

    // 获取列表中项的个数
    function count()
    {
        return global.objIsEmpty(thumbnailListModel) ? 0 : thumbnailListModel.count
    }

    // 获取列表中所有原始url
    function allUrls() {
        var originPaths = []
        for(var i=0 ; i < count(); i++) {
            originPaths.push(thumbnailListModel.get(i).url.toString())
        }
        return originPaths
    }

    // 获取列表中所有原始url
    function allOriginUrls() {
        var originPaths = []
        for(var i=0 ; i < count(); i++) {
            originPaths.push(thumbnailListModel.get(i).url.toString())
        }
        return originPaths
    }

    //统计当前页面的缩略图时间范围
    function totalTimeScope() {
        if(thumnailListType === GlobalVar.ThumbnailType.AllCollection &&
                global.currentViewIndex === GlobalVar.ThumbnailViewType.Collecttion &&
                collecttionView.currentViewIndex === 3) { //仅在合集模式的时候激活计算，以此节省性能
            var visilbeIndexs = theView.flushRectSel(0, theView.contentY, theView.width, theView.height)
            if (visilbeIndexs.length > 0 && visilbeIndexs[0] !== "-1") {
                var url1 = thumbnailListModel.get(visilbeIndexs[0]).url
                var url2 = thumbnailListModel.get(visilbeIndexs[visilbeIndexs.length - 1]).url
                var str = albumControl.getFileTime(url1, url2)
                timeChanged(str)
            } else {
                timeChanged("")
            }
        }
    }

    //执行图片删除操作
    function runDeleteImg() {
       if (!visible)
           return
       if ( thumnailListType !== GlobalVar.ThumbnailType.Trash ){
           albumControl.insertTrash(global.selectedPaths)
       } else {
           albumControl.deleteImgFromTrash(selectedOriginPaths)
           selectAll(false)
           global.sigFlushRecentDelView()
       }
    }

    // 查看图片
    function executeViewImage() {
        if (thumnailListType !== GlobalVar.ThumbnailType.Trash
                && (theView.ism.length === 1)) {
            //如果是影视，则采用打开视频
            if (fileControl.isVideo(thumbnailListModel.get(theView.ism[0]).url.toString())){
                albumControl.openDeepinMovie(thumbnailListModel.get(theView.ism[0]).url.toString())
            } else {
                var openPaths = new Array
                for(var i = 0 ; i< thumbnailListModel.count ; i++){
                    openPaths.push(thumbnailListModel.get(i).url.toString())
                }
                mainStack.sourcePaths = openPaths
                mainStack.currentIndex = theView.ism[0]
                mainStack.currentWidgetIndex = 1
                global.stackControlCurrent = 1
            }
        }
    }

    // 执行全屏预览
    function executeFullScreen() {
        if (root.visibility !== Window.FullScreen && selectedPaths.length !== 0) {
            showFullScreen()
            var openPaths = []
            for(var i=0 ; i< thumbnailListModel.count; i++){
                openPaths.push(thumbnailListModel.get(i).url.toString())
            }
            mainStack.sourcePaths = openPaths
            mainStack.currentIndex = -1
            mainStack.currentIndex = theView.ism[0]
            mainStack.currentWidgetIndex = 1
            global.stackControlLastCurrent = global.stackControlCurrent
            global.stackControlCurrent = 1
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
        } else{
            exportdig.setParameter(global.objIsEmpty(thumbnailListModel) ? "" : (global.objIsEmpty(thumbnailListModel.get(theView.ism[0])) ? "" : thumbnailListModel.get(theView.ism[0]).url.toString()), thumbnailImage)
            exportdig.show()
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

    // 提供给合集日视图和已导入视图使用，用来刷新
    function flushRectSel(x,y,w,h,ctrl,mousePress, inPress) {
        if (theView.contains(x,y) && theView.contains(x+w, y+h) && w !== 0 && h !== 0) {
            // 按住Ctrl，鼠标点击事件释放时，处理点选逻辑
            if (ctrl && mousePress) {
                if (!inPress) {
                    var rectSel = theView.flushRectSel(x, y + theView.contentY, w, h)
                    if (rectSel.length === 1) {
			//不直接往theView.ism里面push，是为了触发onIsmChanged
                        var tempIsm = theView.ism
                        if (theView.ism.indexOf(rectSel[0]) === -1) {
                            tempIsm.push(rectSel[0])
                        } else {      
                            tempIsm.splice(tempIsm.indexOf(rectSel[0]), 1)
                        }
                        theView.ism = tempIsm
                    }
                }
            } else {
                theView.ism = theView.flushRectSel(x, y + theView.contentY, w, h)
            }
        } else {
            // 1.按住ctrl，鼠标垮区域框选时，需要清空框选区域外的已选项
            // 2.不按住ctrl，鼠标单选，需要清空其他列表的已选项
            if (!ctrl && mousePress || ctrl && !mousePress)
                theView.ism = []
        }

        // 跨区域框选后，需要手动激活列表焦点，这样快捷键才能生效
        if (theView.ism.length > 0) {
            theView.forceActiveFocus()
        }

        selectedChanged()
    }

    function intersected(x,y,w,h) {
        var pos = []
        var pos1 = Qt.point(0,0)
        var pos2 = Qt.point(width,height)
        pos.push(pos1)
        pos.push(pos2)
        return pos
    }

    Connections {
        target: thumbnailImage
        onEscKeyPressed: {
            if (haveSelect) {
                selectAll(false)
            }
        }
    }

    //缩略图view的本体
    GridView {
        id: theView
        anchors.fill: parent
        clip: true
        interactive: false //禁用原有的交互逻辑，重新开始定制
        cellWidth: itemWidth
        cellHeight: itemHeight
        model: thumbnailListModel
        cacheBuffer:200*2
        delegate: ThumbnailListDelegate{
            id: thumbnailListDelegate
            m_index: index
            m_url: global.objIsEmpty(thumbnailListModel.get(index)) ? "" : thumbnailListModel.get(index).url
            m_displayFlushHelper: "0"
            width: itemWidth - 4
            height: itemHeight - 4
            remainDays: (global.objIsEmpty(thumbnailListModel.get(index)) || global.objIsEmpty(thumbnailListModel.get(index).remainDays)) ? "" : thumbnailListModel.get(index).remainDays
        }

        currentIndex: -1

        //激活滚动条
        ScrollBar.vertical: ScrollBar {
            id: vbar
            active: false
            onPositionChanged: {
                totalTimeScope()
            }
        }

        //鼠标正在按下状态
        property bool inPress: false

        //负责记录选中的index
        property var ism: new Array

        //橡皮筋显隐状态
        property bool rubberBandDisplayed: false

        //指定缩略图模式
        property double displayFlushHelper: asynImageProvider.getLoadMode()

        // 滚动滑块Delta值
        property int scrollDelta: 60
        //通过坐标获取item
        function getItemIndexFromAxis(x, y)
        {
            var item = itemAt(x, y)
            if(item !== null)
            {
                return item.m_index
            }
            else
            {
                return -1
            }
        }

        onIsmChanged: {
            var tmpPaths = []
            var tmpOriginPaths = []
            for(var i=0 ; i < ism.length ; i++){
                tmpPaths.push(thumbnailListModel.get(ism[i]).url.toString())
                tmpOriginPaths.push(thumbnailListModel.get(ism[i]).filePath.toString())
            }
            if (selectedPaths !== tmpPaths)
                selectedPaths = tmpPaths
            if (selectedOriginPaths != tmpOriginPaths)
                selectedOriginPaths = tmpOriginPaths
        }

        //获取框选区域内所有项的索引值
        //此函数会极大影响框选时的性能表现
        function flushRectSel(startX, startY, lenX, lenY) {
            var tempArray = []

            //统计框入了哪些index
            //1.搜索起始图片
            var searchEndX = startX + lenX
            var searchEndY = startY + lenY
            while(getItemIndexFromAxis(startX, startY) === -1 && startX < searchEndX && startY < searchEndY) {
                startX += 10
                startY += 10
            }

            //2.搜索全域
            for(var i = startX;i < searchEndX;i += itemWidth)
            {
                for(var j = startY;j < searchEndY;j += itemHeight)
                {
                    var index = getItemIndexFromAxis(i, j)
                    if(index !== -1 && tempArray.indexOf(index) == -1)
                    {
                        tempArray.push(index)
                    }
                }
            }

            //3.搜索右侧边框位置
            i = searchEndX
            for(j = startY;j < searchEndY;j += itemHeight)
            {
                index = getItemIndexFromAxis(i, j)
                if(index !== -1 && tempArray.indexOf(index) == -1)
                {
                    tempArray.push(index)
                }
            }

            //4.搜索底部边框位置
            j = searchEndY
            for(i = startX;i < searchEndX;i += itemWidth)
            {
                index = getItemIndexFromAxis(i, j)
                if(index !== -1 && tempArray.indexOf(index) == -1)
                {
                    tempArray.push(index)
                }
            }

            //5.额外搜索右下角位置
            index = getItemIndexFromAxis(searchEndX, searchEndY)
            if(index !== -1 && tempArray.indexOf(index) == -1)
            {
                tempArray.push(index)
            }

            //把起始位置的放进来
            if(tempArray.length == 0)
            {
                index = getItemIndexFromAxis(startX, startY)
                if(index !== -1)
                {
                    tempArray = [index]
                }
            }

            //console.log("flushRectSel:", tempArray)
            return tempArray
        }

        function executeScrollBar(delta) {
            if (enableWheel && visible) {
                if (theView.contentHeight <= theView.height)
                    return

                // 滚动时，激活滚动条显示
                vbar.active = true
                theView.contentY -= delta
                if(theView.contentY < 0) {
                    theView.contentY = 0
                } else if(theView.contentY > theView.contentHeight - theView.height + statusBar.height) {
                    theView.contentY = theView.contentHeight - theView.height + statusBar.height
                }
            } else {
                vbar.active = false
            }
        }

        MouseArea {
            id: theArea

            //按下时的起始坐标
            property int pressedXAxis: -1
            property int pressedYAxis: -1

            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            propagateComposedEvents: true

            onClicked: {
                //解决bug174143
                if(mouse.button == Qt.RightButton) {
                    if(global.currentViewIndex !== GlobalVar.ThumbnailViewType.Device){
                        //设置菜单弹出位置，貌似鼠标位于菜单上，似乎可以强制菜单激活刷新出来
                        thumbnailMenu.popup(mouseX-50, mouseY)
                    }
                }
                //允许鼠标事件传递给子控件处理,否则鼠标点击缩略图收藏图标不能正常工作
                mouse.accepted = false
            }

            onPressed: {
                forceActiveFocus()

                if(mouse.button == Qt.RightButton) {
                    //右键点击区域如果没有选中，则单独选中它
                    var itemIndex = parent.getItemIndexFromAxis(mouseX, mouseY + parent.contentY)
                    if(itemIndex !== -1) {
                        if(theView.ism.indexOf(itemIndex) === -1) {
                            theView.ism = [itemIndex]
                            selectedChanged()
                        }  
                    } else {
                        theView.ism = []
                        selectedChanged()
                        return
                    }

                    //解决bug174143
//                    if(global.currentViewIndex !== GlobalVar.ThumbnailViewType.Device){
//                        thumbnailMenu.popup()
//                        thumbnailMenu.popup(mouseX-50, mouseY)
//                    }
                    return
                }

                parent.inPress = true
                pressedXAxis = mouseX
                pressedYAxis = mouseY
                parent.rubberBandDisplayed = false

                var index = parent.getItemIndexFromAxis(mouseX, mouseY + parent.contentY)
                if(index !== -1)
                {
                    if(parent.ism.indexOf(index) == -1)
                    {
                        // 按住ctrl，连续多选
                        if (Qt.ControlModifier & mouse.modifiers) {
                            //不直接往parent.ism里面push，是为了触发onIsmChanged
                            var tempIsm = parent.ism
                            tempIsm.push(index)
                            parent.ism = tempIsm
                        }
                        else
                            parent.ism = [index]
                        selectedChanged()
                    } else if (parent.ism.indexOf(index) >= 0) {
                        // 按住ctrl，点击在已选项，取消当前已选项
                        if (Qt.ControlModifier & mouse.modifiers) {
                            var removeIsm = parent.ism
                            removeIsm.splice(removeIsm.indexOf(index), 1)
                            parent.ism = removeIsm
                            selectedChanged()
                        }
                    }
                }
            }

            onMouseXChanged: {
                if(mouse.button == Qt.RightButton || pressedXAxis < 0) {
                    return
                }

                rubberBand.m_width = Math.abs(mouseX - pressedXAxis)
                rubberBand.x = Math.min(mouseX, pressedXAxis)
                if(rubberBand.m_width > 0)
                {
                    parent.ism = []
                    parent.ism = parent.flushRectSel(rubberBand.x, rubberBand.y+parent.contentY, Math.max(rubberBand.m_width, 1), Math.max(rubberBand.m_height, 1))
                    parent.rubberBandDisplayed = true
                    selectedChanged()
                }

                //处理结束，记录
                rubberBand.m_lastWidth = rubberBand.m_width
                rubberBand.m_lastX = rubberBand.x
            }

            onMouseYChanged: {
                if(mouse.button == Qt.RightButton || pressedYAxis < 0) {
                    return
                }

                rubberBand.m_height = Math.abs(mouseY - pressedYAxis)
                rubberBand.y = Math.min(mouseY, pressedYAxis)
                if(rubberBand.m_height > 0)
                {
                    parent.ism = []
                    parent.ism = parent.flushRectSel(rubberBand.x, rubberBand.y+parent.contentY, Math.max(rubberBand.m_width, 1), Math.max(rubberBand.m_height, 1))
                    parent.rubberBandDisplayed = true
                    selectedChanged()
                }

                //处理结束，记录
                rubberBand.m_lastHeight = rubberBand.m_height
                rubberBand.m_lastY = rubberBand.y
            }

            onReleased: {
                if(mouse.button == Qt.RightButton) {
                    return
                }

                if(parent.rubberBandDisplayed == false)
                {
                    var index = parent.getItemIndexFromAxis(mouseX, mouseY + parent.contentY)
                    if (index !== -1) {
                        if (!(Qt.ControlModifier & mouse.modifiers))
                            parent.ism = [index]
                    } else {
                        // 应用主窗口被置灰过，第一次点击空白区域，不执行清空选择操作，第二次才清空选择状态
                        if(parent.getItemIndexFromAxis(mouseX, mouseY + parent.contentY) === -1) {
                            if (!global.windowDisActived && !(Qt.ControlModifier & mouse.modifiers)) {
                                parent.ism = []
                            }
                            global.windowDisActived = false
                        }
                    }
                    selectedChanged()
                }

                parent.inPress = false
                pressedXAxis = -1
                pressedYAxis = -1
                rubberBand.m_width = 0
                rubberBand.m_height = 0
            }

            onWheel: {
                var datla = wheel.angleDelta.y / 2
                if (Qt.ControlModifier & wheel.modifiers) {
                    // 按住ctrl，缩放缩略图
                    var curValue = statusBar.sliderValue
                    if (datla > 0)
                        statusBar.setSliderWidgetValue(curValue + 1)
                    else
                        statusBar.setSliderWidgetValue(curValue - 1)
                } else {
                    // 正常滚动显示缩略图内容
                    theView.executeScrollBar(datla)
                }
            }

            onDoubleClicked: {
                if(mouse.button == Qt.LeftButton) {
                    if (theView.ism.length > 0) {
                        //如果是影视，则采用打开视频
                        if (fileControl.isVideo(thumbnailListModel.get(theView.ism[0]).url.toString())){
                            albumControl.openDeepinMovie(thumbnailListModel.get(theView.ism[0]).url.toString())
                        } else {
                            var openPaths = new Array
                            for(var i=0 ; i< thumbnailListModel.count ; i++){
                                openPaths.push(thumbnailListModel.get(i).url.toString())
                            }
                            mainStack.sourcePaths = openPaths
                            mainStack.currentIndex = -1
                            mainStack.currentIndex = theView.ism[0]
                            mainStack.currentWidgetIndex = 1
                            global.stackControlCurrent = 1

                            // 记录当前读取的图片信息，用于监控文件变更
                            fileControl.resetImageFiles(openPaths)
                        }
                        pressedXAxis = -1
                        pressedYAxis = -1
                    }
                }
            }
        }

        Keys.onPressed: {
            switch (event.key) {
            case Qt.Key_Return:
            case Qt.Key_Enter:
                if (menuItemStates.canView)
                    executeViewImage()
                break;
            case Qt.Key_Period:
                if (!menuItemStates.isInTrash) {
                    if (menuItemStates.canFavorite)
                        executeFavorite()
                    else
                        executeUnFavorite()
                }
                break;
            default:
            break;
            }
        }

        Connections {
            target: global
            onSigSelectAll: {
                if (theView.visible)
                    selectAll(bSel)
            }

            onSigPageUp: {
                if (theView.visible) {
                    theView.executeScrollBar(theView.scrollDelta)
                }
            }

            onSigPageDown: {
                if (theView.visible) {
                    theView.executeScrollBar(-theView.scrollDelta)
                }
            }
        }

        //橡皮筋控件
        RubberBand {
            id: rubberBand
            visible: parent.inPress
        }

        //缩略图菜单
        //注意：涉及界面切换的，需要做到从哪里进来，就退出到哪里
        //菜单显隐逻辑有点绕，建议头脑清醒的时候再处理
        Menu {
            id: thumbnailMenu

            //显示大图预览
            RightMenuItem {
                text: qsTr("View")
                visible: menuItemStates.canView
                onTriggered: {
                    executeViewImage()
                }
            }

            //全屏预览
            RightMenuItem {
                text: qsTr("Fullscreen")
                visible:  menuItemStates.canFullScreen
                onTriggered: {
                    executeFullScreen()
                }
                Shortcut {
                    enabled: theView.activeFocus && menuItemStates.canFullScreen
                    autoRepeat: false
                    sequence : "F11"
                    onActivated : {
                        executeFullScreen()
                    }
                }
            }

            //调起打印接口
            RightMenuItem {
                text: qsTr("Print")
                visible: menuItemStates.canPrint

                onTriggered: {
                    fileControl.showPrintDialog(global.selectedPaths)
                }

                Shortcut {
                    enabled: theView.activeFocus && menuItemStates.canPrint
                    autoRepeat: false
                    sequence : "Ctrl+P"
                    onActivated : {
                        fileControl.showPrintDialog(global.selectedPaths)
                    }
                }
            }

            //幻灯片
            RightMenuItem {
                text: qsTr("Slide show")
                visible: menuItemStates.canSlideShow
                onTriggered: {
                    var openPaths = thumnailListView.allUrls()
                    stackControl.startMainSliderShow(openPaths, openPaths.indexOf(selectedPaths[0]))
                }

                Shortcut {
                    enabled: theView.activeFocus && menuItemStates.canSlideShow
                    autoRepeat: false
                    sequence : "F5"
                    onActivated : {
                        var openPaths = thumnailListView.allUrls()
                        stackControl.startMainSliderShow(openPaths, openPaths.indexOf(thumbnailListModel.get(theView.ism[0]).url.toString()))
                    }
                }
            }

            MenuSeparator {
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash
                height: visible ? GlobalVar.rightMenuSeparatorHeight : 0
            }

            //添加到相册子菜单
            //隐藏交给后面的Component.onCompleted解决
            Menu {
                enabled: thumnailListType !== GlobalVar.ThumbnailType.Trash
                id: addToAlbum
                title: qsTr("Add to album")

                RightMenuItem {
                    text: qsTr("New album")
                    onTriggered: {
                        newAlbum.isChangeView = false
                        newAlbum.importSelected = true
                        newAlbum.setNormalEdit()
                        newAlbum.show()
                    }
                }

                MenuSeparator {
                }

                Repeater {
                    id: recentFilesInstantiator
                    property bool bRreshEnableState: false
                    model: albumControl.getAllCustomAlbumId(global.albumChangeList).length
                    delegate: RightMenuItem {
                        text: albumControl.getAllCustomAlbumName(global.albumChangeList)[index]
                        enabled: albumControl.canAddToCustomAlbum(albumControl.getAllCustomAlbumId()[index], global.selectedPaths, recentFilesInstantiator.bRreshEnableState)
                        onTriggered:{
                            // 获取所选自定义相册的Id，根据Id添加到对应自定义相册
                            var customAlbumId = albumControl.getAllCustomAlbumId()[index]
                            albumControl.insertIntoAlbum(customAlbumId , global.selectedPaths)
                            DTK.sendMessage(thumbnailImage, qsTr("Successfully added to “%1”").arg(albumControl.getAllCustomAlbumName(global.albumChangeList)[index]), "notify_checked")
                            recentFilesInstantiator.bRreshEnableState = !recentFilesInstantiator.bRreshEnableState
                        }
                    }
                }
            }

            //导出图片为其它格式
            RightMenuItem {
                text: qsTr("Export")
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash
                         && ((theView.ism.length === 1 && fileControl.pathExists(thumbnailListModel.get(theView.ism[0]).url.toString()) && theArea.haveImage) || !theArea.haveVideo)
                onTriggered: {
                    excuteExport()
                }

                Shortcut {
                    enabled: theView.activeFocus && menuItemStates.canExport
                    autoRepeat: false
                    sequence : "Ctrl+E"
                    onActivated : {
                       excuteExport()
                    }
                }
            }

            //复制图片
            RightMenuItem {
                text: qsTr("Copy")
                visible: menuItemStates.canCopy
                onTriggered: {
                    fileControl.copyImage(global.selectedPaths)
                }
            }

            //删除图片
            RightMenuItem {
                text: qsTr("Delete")
                visible: menuItemStates.canDelete
                onTriggered: {
                    deleteDialog.setDisplay(thumnailListType === GlobalVar.ThumbnailType.Trash ? GlobalVar.FileDeleteType.TrashSel : GlobalVar.FileDeleteType.Normal, selectedOriginPaths.length)
                    deleteDialog.show()
                }
            }

            //从相册移除（只在自定义相册中显示）
            RightMenuItem {
                text: qsTr("Remove from album")
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash
                         && (thumnailListType === GlobalVar.ThumbnailType.CustomAlbum)
                onTriggered: {
                    albumControl.removeFromAlbum(global.currentCustomAlbumUId, selectedPaths)
                    global.sigFlushCustomAlbumView(global.currentCustomAlbumUId)
                }
            }

            MenuSeparator {
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash
                height: visible ? GlobalVar.rightMenuSeparatorHeight : 0
            }

            //添加到我的收藏
            RightMenuItem {
                id: favoriteAction
                text: qsTr("Favorite")
                visible: !menuItemStates.isInTrash && menuItemStates.canFavorite
                onTriggered: {
                    executeFavorite()
                }
            }

            //从我的收藏中移除
            RightMenuItem {
                id: unFavoriteAction
                text: qsTr("Unfavorite")
                visible: !menuItemStates.isInTrash && !menuItemStates.canFavorite
                onTriggered: {
                    executeUnFavorite()
                }
            }

            MenuSeparator {
                visible: menuItemStates.canRotate
                height: visible ? GlobalVar.rightMenuSeparatorHeight : 0
            }

            //顺时针旋转
            RightMenuItem {
                text: qsTr("Rotate clockwise")
                visible: menuItemStates.canRotate
                onTriggered: {
                    fileControl.rotateFile(global.selectedPaths, 90)
                }
            }

            //逆时针旋转
            RightMenuItem {
                text: qsTr("Rotate counterclockwise")
                visible: menuItemStates.canRotate
                onTriggered: {
                    fileControl.rotateFile(global.selectedPaths, -90)
                }
            }

            MenuSeparator {
                visible: !menuItemStates.isInTrash
                         && (setAsWallpaperAction.visible || displayInFileManagerAction.visible || photoInfoAction.visible || videoInfoAction.visible)
                height: visible ? GlobalVar.rightMenuSeparatorHeight : 0
            }

            //设置为壁纸
            RightMenuItem {
                text: qsTr("Set as wallpaper")
                id: setAsWallpaperAction
                visible: menuItemStates.canWallpaper
                onTriggered: {
                    fileControl.setWallpaper(global.selectedPaths[0])
                }

                Shortcut {
                    enabled: theView.activeFocus && menuItemStates.canWallpaper
                    autoRepeat: false
                    sequence : "Ctrl+F9"
                    onActivated : {
                        fileControl.setWallpaper(global.selectedPaths[0])
                    }
                }
            }

            //在文件管理器中显示
            RightMenuItem {
                text: qsTr("Display in file manager")
                id: displayInFileManagerAction
                visible: menuItemStates.canDisplayInFolder
                onTriggered: {
                    fileControl.displayinFileManager(global.selectedPaths[0])
                }

                Shortcut {
                    enabled: theView.activeFocus && menuItemStates.canDisplayInFolder
                    autoRepeat: false
                    sequence : "Alt+D"
                    onActivated : {
                        fileControl.displayinFileManager(global.selectedPaths[0])
                    }
                }
            }

            //恢复
            RightMenuItem {
                text: qsTr("Restore")
                visible: thumnailListType === GlobalVar.ThumbnailType.Trash
                onTriggered: {
                    albumControl.recoveryImgFromTrash(selectedOriginPaths)
                    selectAll(false)
                    global.sigFlushRecentDelView()
                }
            }

            //照片信息
            RightMenuItem {
                text: qsTr("Photo info")
                id: photoInfoAction
                visible: menuItemStates.canViewPhotoInfo
                onTriggered: {
                    executeViewPhotoInfo()
                }

                Shortcut {
                    enabled: theView.activeFocus && menuItemStates.canViewPhotoInfo
                    autoRepeat: false
                    sequence : "Ctrl+I"
                    onActivated : {
                        executeViewPhotoInfo()
                    }
                }
            }

            //视频信息
            RightMenuItem {
                text: qsTr("Video info")
                id: videoInfoAction
                visible: menuItemStates.canViewVideoInfo
                onTriggered: {
                    executeViewVideoInfo()
                }

                Shortcut {
                    enabled: theView.activeFocus && menuItemStates.canViewVideoInfo
                    autoRepeat: false
                    sequence : "Ctrl+I"
                    onActivated : {
                        executeViewVideoInfo()
                    }
                }
            }

            Component.onCompleted: {
                //最近删除界面下隐藏添加到相册的子菜单
                if(thumnailListType === GlobalVar.ThumbnailType.Trash) {
                    var item = itemAt(5)
                    item.visible = false
                    item.height = 0
                }
            }
        }
    }

    Component.onCompleted: {
        global.sigThumbnailStateChange.connect(fouceUpdate)
        deleteDialog.sigDoDeleteImg.connect(runDeleteImg)
    }

    onRealCellWidthChanged: {
        totalTimeScope()
    }

}
