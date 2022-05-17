import QtQuick 2.11

Item {

    property var imgPaths
    property var imgCurrentPath
    property int imgCurrentIndex: 0
    property int minHeight: 300
    property int minWidth: 628
    property int albumMinHeight: 300
    property int albumMinWidth: 628

    property int minHideHeight: 428
    property int floatMargin: 60
    property int titleHeight: 50
    property int showBottomY: 80
    property int actionMargin: 9//应用图标距离顶栏

    property int rightMenuItemHeight: 32//右键菜单item的高度
    property bool ctrlPressed: false//记录ctrl键是否按下

    property int currentViewIndex: 0// 0:打开图片界面 1:无图片界面
    property int stackControlCurrent: 0// 0:相册界面 1:看图界面
    property var haveImportedPaths //所有导入的图片

    signal sigWindowStateChange()

    onHaveImportedPathsChanged: {
        currentViewIndex = 2
    }

}
