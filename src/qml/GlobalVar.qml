import QtQuick 2.11
import QtQuick.Controls 2.4
import org.deepin.dtk 1.0

Item {

    property var imgPaths
    property var imgCurrentPath
    property int imgCurrentIndex: 0
    property int minHeight: 520
    property int minWidth: 724
    property int albumMinHeight: 300
    property int albumMinWidth: 628

    property int needHideSideBarWidth: 783 //需要隐藏侧边栏的时，主界面宽度

    property int minHideHeight: 425 //调整窗口高度小于425px时，隐藏工具栏和标题栏
    property int floatMargin: 60
    property int titleHeight: 50
    property int showBottomY: 80
    property int actionMargin: 9    //应用图标距离顶栏

    property int rightMenuItemHeight: 32//右键菜单item的高度
    property int rightMenuSeparatorHeight: 12//右键菜单分割层的高度

    property int statusBarHeight: 30 //状态栏高度

    property int thumbnailViewTitleHieght: 85 // 缩略图视图区域标题显示区域高度
    property int verticalScrollBarWidth: 15 // 垂直滚动条宽度

    property int rectSelScrollStep: 30 // 框选滚动步进
    property var selectedPaths: [] // 已选路径
    property bool bRefreshFavoriteIconFlag: false //刷新收藏图标标记，翻转一次，图标就刷新一次
    property bool bRefreshRangeBtnState: false //刷新显示比例图标激活状态标记，翻转一次，图标就刷新一次
    property int currentViewIndex: 2// 0:导入图片视图 1:无图片视图 2:合集视图 3:已导入视图 4:我的收藏视图 5:最近删除视图 6:系统/自定义相册视图 7:搜索结果视图 8:设备视图
    property int currentCustomAlbumUId: -1// 当前自定义相册所在UId，-1：照片库(非我的收藏) 0:我的收藏 1:截图录屏 2:相机 3:画板 其他:自定义相册
    property int stackControlCurrent: 0// 0:相册界面 1:看图界面 2:幻灯片
    property int stackControlLastCurrent: -1 //记录上一次显示的主界面索引 0:相册界面 1:看图界面 2:幻灯片

    property int thumbnailSizeLevel: 0 //缩略图缩放等级
    property real cellBaseWidth: thumbnailSizeLevel >= 0 && thumbnailSizeLevel <= 9 ? 80 + thumbnailSizeLevel * 10 : 80
    property int thumbnailListRightMargin: 10
    property string statusBarNumText: "" //状态栏显示的总数文本内容
    property string searchEditText: ""

    property bool albumImportChangeList: false //自动导入相册改变
    property bool albumChangeList: false //自定义相册改变
    property ButtonGroup siderGroup: ButtonGroup {} //控制导航栏的group

    property bool deviceChangeList: false //设备相册改变
    property int deviceCurrentIndex: 0 //设备index
    property string deviceCurrentName: albumControl.getDeviceName(deviceCurrentPath) //设备当前名称
    property string deviceCurrentPath: "" //设备当前P
    property bool windowDisActived: false

    function objIsEmpty(obj) {
        var ret = (String(obj) === "undefined" || String(obj) === "null")
        //console.log("obj is", ret ? "empty." : "not empty.", "objStr:", String(obj))
        return ret
    }

    signal sigWindowStateChange()
    signal sigThumbnailStateChange()
    signal sigRunSearch(int UID, string keywords) //执行搜索
    signal sigFlushAllCollectionView()   // 刷新合集所有项目视图内容
    signal sigFlushHaveImportedView()   // 刷新已导入视图内容
    signal sigFlushRecentDelView()      // 刷新最近删除视图内容
    signal sigFlushCustomAlbumView(int customAlbumUId)    //刷新我的收藏/自定义相册视图内容 customAlbumUId: >= 0 刷新指定视图，-1: 默认刷新所有视图
    signal sigCollectionViewIndexChanged(int index) //合集页面发生改变
    signal sigFlushSearchView() // 刷新搜索结果视图内容
    signal sigThumbnailSizeLevelChanged()
    signal sigCustomAlbumNameChaged(int UID, string name) // 刷新自定义相册视图的相册名称
    signal sigSelectAll(bool bSel)
    signal sigPageUp()
    signal sigPageDown()

    Component.onCompleted: {
        currentViewIndex = GlobalVar.ThumbnailViewType.Collecttion
    }

    enum ThumbnailViewType {
        Import = 0,            // 导入图片视图
        NoPicture,             // 无图片视图
        Collecttion,           // 合集视图
        HaveImported,          // 已导入视图
        Favorite,              // 我的收藏视图
        RecentlyDeleted,       // 最近删除视图
        CustomAlbum,           // 系统/自定义相册视图
        SearchResult,          // 搜索结果视图
        Device                 // 设备视图
    }

    //缩略图类型枚举
    enum ThumbnailType {
        Normal,       //普通模式
        Trash,        //最近删除
        CustomAlbum,  //自定义相册
        AutoImport,   //自动导入路径
        AllCollection //合集模式
    }

    // 框选超出边界朝向类型
    enum RectScrollDirType {
        NoType,   // 框选没有朝向
        ToTop,    // 框选超出边界朝上
        ToBottom  // 框选超出边界朝下
    }

    // 文件删除类型
    enum FileDeleteType {
        Normal,   // 普通删除
        TrashSel, // 最近删除-选中删除
        TrashAll  // 最近删除-全部删除
    }

    //刷新自定义相册
    Connections {
        target: albumControl
        onSigRefreshCustomAlbum: {
           sigFlushCustomAlbumView(UID)
        }
    }

    //数据库监听-刷新合集所有项目
    Connections {
        target: albumControl
        onSigRefreshAllCollection: {
            sigFlushAllCollectionView()
        }
    }

    //数据库监听-刷新已导入
    Connections {
        target: albumControl
        onSigRefreshImportAlbum: {
            sigFlushHaveImportedView()
            bRefreshRangeBtnState = !bRefreshRangeBtnState
        }
    }

    //数据库监听-刷新搜索结果
    Connections {
        target: albumControl
        onSigRefreshSearchView: {
            if (global.currentViewIndex === GlobalVar.ThumbnailViewType.SearchResult)
                sigFlushSearchView()
        }
    }

    // 自动导入相册有新增相册，刷新自动导入相册列表
    Connections {
        target: albumControl
        onSigRefreshSlider: {
            albumImportChangeList = !albumImportChangeList
        }
    }

    // 导入重复文件提示
    Connections {
        target: albumControl
        onSigRepeatUrls: {
            DTK.sendMessage(stackControl, qsTr("The photo/video already exists"), "warning")
        }
    }

    onCurrentViewIndexChanged: {
        console.log("currentViewIndex   :", currentViewIndex)
        if(albumControl.getAllCount() <= 0) {
            switch (currentViewIndex) {
                case GlobalVar.ThumbnailViewType.Import:
                case GlobalVar.ThumbnailViewType.NoPicture:
                case GlobalVar.ThumbnailViewType.Collecttion:
                    break
                case GlobalVar.ThumbnailViewType.SearchResult:
                    currentViewIndex = GlobalVar.ThumbnailViewType.NoPicture
                    break
            }
        }
    }

    onThumbnailSizeLevelChanged: {
        sigThumbnailSizeLevelChanged()
    }

    onSelectedPathsChanged: {
        menuItemStates.updateMenuItemStates()
    }

    function getSelectedNumText(paths, text) {
        var ret = albumControl.getPicVideoCountFromPaths(paths)

        //QML的翻译不支持%n的特性，只能拆成这种代码
        var photoCount = ret[0]
        var videoCount = ret[1]
        var selectedNumText
        if(paths.length === 0) {
            selectedNumText = text
        } else if(paths.length === 1 && photoCount === 1) {
            selectedNumText = qsTr("1 item selected (1 photo)")
        } else if(paths.length === 1 && videoCount === 1) {
            selectedNumText = qsTr("1 item selected (1 video)")
        } else if(photoCount > 1 && videoCount === 0) {
            selectedNumText = qsTr("%1 items selected (%1 photos)").arg(photoCount)
        } else if(videoCount > 1 && photoCount === 0) {
            selectedNumText = qsTr("%1 items selected (%1 videos)").arg(videoCount)
        } else if (photoCount === 1 && videoCount > 1) {
            selectedNumText = qsTr("%1 items selected (1 photo, %2 videos)").arg(photoCount + videoCount).arg(videoCount)
        } else if (videoCount === 1 && photoCount > 1) {
            selectedNumText = qsTr("%1 items selected (%2 photos, 1 video)").arg(photoCount + videoCount).arg(photoCount)
        } else if (photoCount > 1 && videoCount > 1){
            selectedNumText = qsTr("%1 items selected (%2 photos, %3 videos)").arg(photoCount + videoCount).arg(photoCount).arg(videoCount)
        }

        return selectedNumText
    }
}
