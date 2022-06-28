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
        if(doSelect) {
            var len = thumbnailListModel.count
            var tempArray = theView.ism
            for(var i = 0;i != len;++i) {
                tempArray.push(i.toString()) //数据类型必须和代理里面的m_index对齐，否则会判断失败
            }
            theView.ism = tempArray
        } else {
            theView.ism = []
        }
        selectedChanged()
    }

    //强制重新刷新整个缩略图界面
    function fouceUpdate() {
        //QML的图片强制重刷机制：改变图片路径
        theView.displayFlushHelper = Math.random()
    }

    //设置缩略图显示类型
    function setType(newType) {
        thumnailListType = newType
    }

    // 获取列表中项的个数
    function count()
    {
        return global.objIsEmpty(thumbnailListModel) ? 0 : thumbnailListModel.count
    }

    // 获取列表中所有原始路径
    function allOriginPaths() {
        var originPaths = []
        for(var i=0 ; i < count(); i++) {
            originPaths.push(thumbnailListModel.get(i).filePath.toString())
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
        if(thumnailListType === GlobalVar.ThumbnailType.AllCollection && global.currentViewIndex === GlobalVar.ThumbnailViewType.Collecttion && collecttionView.currentViewIndex === 3) { //仅在合集模式的时候激活计算，以此节省性能
            var visilbeIndexs = theView.flushRectSel(0, 0, theView.width, theView.height)
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
        if ( thumnailListType !== GlobalVar.ThumbnailType.Trash ){
            albumControl.insertTrash(global.selectedPaths)
        } else {
            albumControl.deleteImgFromTrash(selectedOriginPaths)
            selectAll(false)
            global.sigFlushRecentDelView()
        }
    }

    function imageFullScreen() {
        if (root.visibility !== Window.FullScreen && selectedPaths.length !== 0) {
            showFullScreen()
            var openPaths = []
            for(var i=0 ; i< thumbnailListModel.count ; i++){
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

    Connections {
        target: thumbnailImage
        onEscKeyPressed: {
            if (haveSelect) {
                selectAll(false)
            }
        }
    }

    //view依赖的model管理器
    property ListModel thumbnailListModel: ListModel { }

    // 缩略图Item尺寸
    property real itemWidth: realCellWidth
    property real itemHeight: realCellWidth
    // 是否开启滚轮
    property bool enableWheel: true
    //缩略图类型，默认为普通模式
    property int thumnailListType: global.currentViewIndex === GlobalVar.ThumbnailViewType.RecentlyDeleted ? GlobalVar.ThumbnailType.Trash : GlobalVar.ThumbnailType.Normal
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
    //缩略图动态变化（-10是右侧的边距）
    property real cellBaseWidth: global.thumbnailSizeLevel >= 0 && global.thumbnailSizeLevel <= 9 ? 80 + global.thumbnailSizeLevel * 10 : 80
    property int  rowSizeHint: (width - 10) / cellBaseWidth
    property real realCellWidth: (width - 10) / rowSizeHint

    //缩略图view的本体
    GridView {
        id: theView
        anchors.fill: parent
        clip: true
        interactive: false //禁用原有的交互逻辑，重新开始定制
        cellWidth: itemWidth
        cellHeight: itemHeight
        model: thumbnailListModel
        delegate: ThumbnailListDelegate{
            id: thumbnailListDelegate
            m_index: index
            m_url: global.objIsEmpty(thumbnailListModel.get(index)) ? "" : thumbnailListModel.get(index).url
            m_displayFlushHelper: "0"
            width: itemWidth - 4
            height: itemHeight - 4
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

        //辅助强制刷新变量
        property double displayFlushHelper: 0

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
             //console.log("onIsmChanged: ", selectedPaths)
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

            console.log("flushRectSel:", tempArray)
            return tempArray
        }

        MouseArea {
            id: theArea

            //按下时的起始坐标
            property int pressedXAxis: -1
            property int pressedYAxis: -1

            //框选状态检查
            property bool haveImage: false
            property bool haveVideo: false
            property bool canDelete: false
            property bool canFavorite: false
            property bool canRotate: true
            property bool canPrint: true

            anchors.fill: parent
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            onPressed: {
                forceActiveFocus()

                if(mouse.button == Qt.RightButton) {
                    //右键点击区域如果没有选中，则单独选中它
                    var itemIndex = parent.getItemIndexFromAxis(mouseX, mouseY)
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

                    //已框选的图片状态检查
                    haveImage = fileControl.haveImage(selectedPaths)
                    haveVideo = fileControl.haveVideo(selectedPaths)
                    canDelete = fileControl.isCanDelete(selectedPaths)
                    canFavorite = albumControl.canFavorite(selectedPaths)
                    canRotate = fileControl.isRotatable(selectedPaths)
                    canPrint = fileControl.isCanPrint(selectedPaths)

                    if(global.currentViewIndex !== GlobalVar.ThumbnailViewType.Device){
                        thumbnailMenu.popup()
                    }
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
                        parent.ism = [index]
                        selectedChanged()
                    } else if (parent.ism.indexOf(index) >= 0)
                    {
                        mouse.accepted = false
                    }
                }
            }

            onMouseXChanged: {
                if(mouse.button == Qt.RightButton) {
                    return
                }

                rubberBand.m_width = Math.abs(mouseX - pressedXAxis)
                rubberBand.x = Math.min(mouseX, pressedXAxis)
                if(rubberBand.m_width > 0)
                {
                    parent.ism = []
                    parent.rubberBandDisplayed = true
                    parent.ism = parent.flushRectSel(rubberBand.x, rubberBand.y+parent.contentY, Math.max(rubberBand.m_width, 1), Math.max(rubberBand.m_height, 1))
                    selectedChanged()
                }

                //处理结束，记录
                rubberBand.m_lastWidth = rubberBand.m_width
                rubberBand.m_lastX = rubberBand.x
            }

            onMouseYChanged: {
                if(mouse.button == Qt.RightButton) {
                    return
                }

                rubberBand.m_height = Math.abs(mouseY - pressedYAxis)
                rubberBand.y = Math.min(mouseY, pressedYAxis)
                if(rubberBand.m_height > 0)
                {
                    parent.ism = []
                    parent.rubberBandDisplayed = true
                    parent.ism = parent.flushRectSel(rubberBand.x, rubberBand.y+parent.contentY, Math.max(rubberBand.m_width, 1), Math.max(rubberBand.m_height, 1))
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
                    if(index !== -1) {
                        parent.ism = [index]
                    } else {
                        // 应用主窗口被置灰过，第一次点击空白区域，不执行清空选择操作，第二次才清空选择状态
                        if (!global.windowDisActived) {
                            parent.ism = []
                        }
                        global.windowDisActived = false
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
                if (enableWheel) {
                    if (parent.contentHeight <= parent.height)
                        return

                    // 滚动时，激活滚动条显示
                    vbar.active = true
                    var datla = wheel.angleDelta.y / 2
                    parent.contentY -= datla
                    if(parent.contentY < 0) {
                        parent.contentY = 0
                    } else if(parent.contentY > parent.contentHeight - parent.height + statusBar.height) {
                        parent.contentY = parent.contentHeight - parent.height + statusBar.height
                    }
                } else {
                    vbar.active = false
                }
            }

            onDoubleClicked: {
                if(mouse.button == Qt.LeftButton) {
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
                    }
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
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash
                         && (theView.ism.length === 1)
                onTriggered: {
                    //如果是影视，则采用打开视频
                    if (fileControl.isVideo(thumbnailListModel.get(theView.ism[0]).url.toString())){
                        albumControl.openDeepinMovie(thumbnailListModel.get(theView.ism[0]).url.toString())
                    } else {
                        var openPaths = new Array
                        for(var i=0 ; i< thumbnailListModel.count ; i++){
                            openPaths.push(thumbnailListModel.get(i).url.toString())
                        }
                        mainStack.sourcePaths = openPaths
                        mainStack.currentIndex = theView.ism[0]
                        mainStack.currentWidgetIndex = 1
                        global.stackControlCurrent = 1
                    }
                }
            }

            //全屏预览
            RightMenuItem {
                text: qsTr("Fullscreen")
                visible:  thumnailListType !== GlobalVar.ThumbnailType.Trash
                          && (theView.ism.length === 1 && fileControl.pathExists(thumbnailListModel.get(theView.ism[0]).url.toString()))
                          && fileControl.isImage(thumbnailListModel.get(theView.ism[0]).url.toString())
                onTriggered: {
                    imageFullScreen()
                }
                Shortcut {
                    enabled: theView.activeFocus
                    autoRepeat: false
                    sequence : "F11"
                    onActivated : {
                        console.log("thumbnailListView F11..")
                        imageFullScreen()
                    }
                }
            }

            //调起打印接口
            RightMenuItem {
                text: qsTr("Print")
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash && theArea.canPrint

                onTriggered: {
                    fileControl.showPrintDialog(global.selectedPaths)
                }
            }

            //幻灯片
            RightMenuItem {
                text: qsTr("Slide show")
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash
                         && ((theView.ism.length === 1 && fileControl.pathExists(thumbnailListModel.get(theView.ism[0]).url.toString())) || theArea.haveImage)
                         && fileControl.isImage(thumbnailListModel.get(theView.ism[0]).url.toString())
                onTriggered: {

                    var openPaths = new Array
                    for(var i=0 ; i< thumbnailListModel.count ; i++){
                        openPaths.push(thumbnailListModel.get(i).url.toString())
                    }

                    stackControl.startMainSliderShow(openPaths, openPaths.indexOf(thumbnailListModel.get(theView.ism[0]).url.toString()))

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
                        var x = parent.mapToGlobal(0, 0).x + parent.width / 2 - 190
                        var y = parent.mapToGlobal(0, 0).y + parent.height / 2 - 89
                        newAlbum.setX(x)
                        newAlbum.setY(y)
                        newAlbum.isChangeView = true
                        newAlbum.setNormalEdit()
                        newAlbum.show()
                    }
                }

                MenuSeparator {
                }

                Repeater {
                    id: recentFilesInstantiator
                    model: albumControl.getAllCustomAlbumId(global.albumChangeList).length
                    delegate: RightMenuItem {
                        text: albumControl.getAllCustomAlbumName(global.albumChangeList)[index]

                        onTriggered:{
                            //未知原因导致global.siderGroup.buttons内部index混乱，先这样规避
                            var realIndex = index
                            if(global.haveCreateAlbum) {
                                realIndex += 4
                            }

                            albumControl.insertIntoAlbum(albumControl.getAllCustomAlbumId(global.albumChangeList)[realIndex] , global.selectedPaths)
                            global.currentCustomAlbumUId = albumControl.getAllCustomAlbumId(global.albumChangeList)[realIndex]
                            global.siderGroup.buttons[realIndex].checked = true
                            global.currentViewIndex = GlobalVar.ThumbnailViewType.CustomAlbum
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
                    if (global.selectedPaths.length > 1){
                        albumControl.getFolders(global.selectedPaths);
                    } else{
                        exportdig.filePath = global.objIsEmpty(thumbnailListModel) ? "" : (global.objIsEmpty(thumbnailListModel.get(theView.ism[0])) ? "" : thumbnailListModel.get(theView.ism[0]).url.toString())
                        var x = parent.mapToGlobal(0, 0).x + parent.width / 2 - 190
                        var y = parent.mapToGlobal(0, 0).y + parent.height / 2 - 89
                        exportdig.setX(x)
                        exportdig.setY(y)
                        exportdig.show()
                    }
                }
            }

            //复制图片
            RightMenuItem {
                text: qsTr("Copy")
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash
                         && ((theView.ism.length === 1 && fileControl.pathExists(thumbnailListModel.get(theView.ism[0]).url.toString())) || theView.ism.length > 1)
                 && fileControl.isImage(thumbnailListModel.get(theView.ism[0]).url.toString())
                onTriggered: {
                    fileControl.copyImage(global.selectedPaths)
                }
            }

            //删除图片
            RightMenuItem {
                text: qsTr("Delete")
                visible: theArea.canDelete
                onTriggered: {
                    if(thumnailListType === GlobalVar.ThumbnailType.Trash) {
                        deleteDialog.setDisplay(thumnailListType === GlobalVar.ThumbnailType.Trash, selectedOriginPaths.length)
                    } else {
                        deleteDialog.setDisplay(thumnailListType === GlobalVar.ThumbnailType.Trash, global.selectedPaths.length)
                    }

                    deleteDialog.show()
                }
            }

            //从相册移除（只在自定义相册中显示）
            RightMenuItem {
                text: qsTr("Remove from album")
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash
                         && (thumnailListType === GlobalVar.ThumbnailType.CustomAlbum)
                onTriggered: {

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
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash && (theArea.canFavorite)
                onTriggered: {
                    albumControl.insertIntoAlbum(0, selectedPaths)
                    global.bRefreshFavoriteIconFlag = !global.bRefreshFavoriteIconFlag

                    // 若当前视图为我的收藏，需要实时刷新我的收藏列表内容
                    if (global.currentViewIndex === GlobalVar.ThumbnailViewType.Favorite && global.currentCustomAlbumUId === 0) {
                        global.sigFlushCustomAlbumView(global.currentCustomAlbumUId)
                    }
                }
            }

            //从我的收藏中移除
            RightMenuItem {
                id: unFavoriteAction
                text: qsTr("Unfavorite")
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash && (!theArea.canFavorite)
                onTriggered: {
                    albumControl.removeFromAlbum(0, selectedPaths)
                    global.bRefreshFavoriteIconFlag = !global.bRefreshFavoriteIconFlag
                    // 若当前视图为我的收藏，需要实时刷新我的收藏列表内容
                    if (global.currentViewIndex === GlobalVar.ThumbnailViewType.Favorite && global.currentCustomAlbumUId === 0) {
                        global.sigFlushCustomAlbumView(global.currentCustomAlbumUId)
                    }
                }
            }

            MenuSeparator {
                visible: rotateClockwiseAction.visible
                height: visible ? GlobalVar.rightMenuSeparatorHeight : 0
            }

            //顺时针旋转
            RightMenuItem {
                id: rotateClockwiseAction
                text: qsTr("Rotate clockwise")
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash && (theArea.canRotate)
                onTriggered: {
                    fileControl.rotateFile(global.selectedPaths, 90)
                }
            }

            //逆时针旋转
            RightMenuItem {
                text: qsTr("Rotate counterclockwise")
                visible: rotateClockwiseAction.visible
                onTriggered: {
                    fileControl.rotateFile(global.selectedPaths, -90)
                }
            }

            MenuSeparator {
                visible: (thumnailListType !== GlobalVar.ThumbnailType.Trash)
                         && (setAsWallpaperAction.visible || displayInFileManagerAction.visible || photoInfoAction.visible || videoInfoAction.visible)
                height: visible ? GlobalVar.rightMenuSeparatorHeight : 0
            }

            //设置为壁纸
            RightMenuItem {
                text: qsTr("Set as wallpaper")
                id: setAsWallpaperAction
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash
                         && (theView.ism.length === 1 && fileControl.isSupportSetWallpaper(thumbnailListModel.get(theView.ism[0]).url.toString()))
                onTriggered: {
                    fileControl.setWallpaper(thumbnailListModel.get(theView.ism[0]).url.toString())
                }
            }

            //在文件管理器中显示
            RightMenuItem {
                text: qsTr("Display in file manager")
                id: displayInFileManagerAction
                visible: thumnailListType !== GlobalVar.ThumbnailType.Trash
                         && (theView.ism.length == 1 && fileControl.pathExists(thumbnailListModel.get(theView.ism[0]).url.toString()))
                onTriggered: {
                    fileControl.displayinFileManager(thumbnailListModel.get(theView.ism[0]).url.toString())
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
                visible: theView.ism.length == 1 && fileControl.isImage(thumbnailListModel.get(theView.ism[0]).url.toString())
                onTriggered: {
                    albumInfomationDig.filePath = thumbnailListModel.get(theView.ism[0]).url.toString()
                    albumInfomationDig.show()
                }
            }

            //视频信息
            RightMenuItem {
                text: qsTr("Video info")
                id: videoInfoAction
                visible: theView.ism.length == 1 && !fileControl.isImage(thumbnailListModel.get(theView.ism[0]).url.toString())
                onTriggered: {
                    videoInfomationDig.filePath = thumbnailListModel.get(theView.ism[0]).url.toString()
                    videoInfomationDig.show()
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
