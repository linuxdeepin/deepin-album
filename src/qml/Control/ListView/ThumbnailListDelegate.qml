import QtQuick 2.11
import QtQuick.Window 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11
import QtQml.Models 2.11
import QtQml 2.11
import QtQuick.Shapes 1.10
import org.deepin.dtk 1.0

import "../"
import "../../"

Rectangle  {
    width: 100
    height: 100

    //注意：在model里面加进去的变量，这边可以直接进行使用，只是部分位置不好拿到，需要使用变量
    property string m_index
    property string m_path

    //缩略图类型，默认为普通模式
    property int type: GlobalVar.ThumbnailType.Trash

    onM_pathChanged: {
    }

    //设置缩略图显示类型
    function setType(newType) {
        type = newType
    }

    //选中后显示的阴影框
    Rectangle {
        id: selectShader
        anchors.fill: parent
        radius: 5 * 2
        color: "#AAAAAA"
        visible: theView.ism.indexOf(parent.m_index) != -1
        opacity: 0.4
    }

    Image{
        source: "image://publisher/" + displayFlushHelper.toString() + theView.displayFlushHelper.toString() + "_" + path
        asynchronous: true
        anchors.centerIn: parent
        sourceSize.width: parent.width - 14
        sourceSize.height: parent.height - 14
        width:parent.width - 14
        height:parent.height - 14
        fillMode:Image.Pad
        clip: true
    }

    property bool haveImage: false
    property bool haveVideo: false
    property bool canDelete: false
    property bool canFavorite: false
    property bool canRotate: true

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.RightButton //仅激活右键，激活左键会和view的冲突
        onClicked: {
            console.log(theView.ism)
            if(theView.ism.indexOf(parent.m_index) === -1) {
                theView.ism = [parent.m_index]
            }

            //已框选的图片状态检查
            haveImage = false
            haveVideo = false
            canDelete = false
            canFavorite = false
            canRotate = true
            for(var i = 0;i != theView.ism.length;++i) {
                var currentPath = thumbnailListModel.get(theView.ism[i]).path
                if(fileControl.isImage(currentPath)) { //图片
                    haveImage = true
                } else if(fileControl.pathExists(currentPath)) { //视频
                    haveVideo = true
                }

                if(fileControl.isCanDelete(currentPath)) {
                    canDelete = true
                }

                if(!albumControl.photoHaveFavorited(currentPath)) {
                    canFavorite = true
                }

                if(!fileControl.isRotatable(currentPath)) {
                    canRotate = false
                }
            }

            thumbnailMenu.popup()
        }
    }

    //缩略图菜单
    //注意：涉及界面切换的，需要做到从哪里进来，就退出到哪里
    Menu {
        id: thumbnailMenu

        //显示大图预览
        RightMenuItem {
            text: qsTr("View")
            visible: type !== GlobalVar.ThumbnailType.Trash && (theView.ism.length === 1)
            onTriggered: {

            }
        }

        //全屏预览
        RightMenuItem {
            text: qsTr("Fullscreen")
            visible:  type !== GlobalVar.ThumbnailType.Trash && (theView.ism.length === 1 && fileControl.pathExists(path))
            onTriggered: {

            }
        }

        //调起打印接口
        RightMenuItem {
            text: qsTr("Print")
            visible: type !== GlobalVar.ThumbnailType.Trash && (theView.ism.length === 1 && fileControl.pathExists(path) && fileControl.isImage(path))
            onTriggered: {

            }
        }

        //幻灯片
        RightMenuItem {
            text: qsTr("Slide show")
            visible: type !== GlobalVar.ThumbnailType.Trash && ((theView.ism.length === 1 && fileControl.pathExists(path)) || haveImage)
            onTriggered: {

            }
        }

        MenuSeparator {
            visible: type !== GlobalVar.ThumbnailType.Trash
            height: visible ? GlobalVar.rightMenuSeparatorHeight : 0
        }

        //添加到相册子菜单
        //QML缺陷：暂时无法隐藏
        Menu {
            title: qsTr("Add to album")

            RightMenuItem {
                text: qsTr("New album")
                onTriggered: {

                }
            }

            MenuSeparator {
            }

            //这个部分需要动态读取相册的数据库情况，需要显示所有的相册，已经在目标相册里的就置灰
            RightMenuItem {
                text: qsTr("TODO: 这个部分需要动态读取相册的数据库情况")
                onTriggered: {

                }
            }
        }

        //导出图片为其它格式
        RightMenuItem {
            text: qsTr("Export")
            visible: type !== GlobalVar.ThumbnailType.Trash && ((theView.ism.length === 1 && fileControl.pathExists(path) && haveImage) || !haveVideo)
            onTriggered: {

            }
        }

        //复制图片
        RightMenuItem {
            text: qsTr("Copy")
            visible: type !== GlobalVar.ThumbnailType.Trash && ((theView.ism.length === 1 && fileControl.pathExists(path)) || theView.ism.length > 1)
            onTriggered: {

            }
        }

        //删除图片
        RightMenuItem {
            text: qsTr("Delete")
            visible: canDelete
            onTriggered: {

            }
        }

        //从相册移除（只在自定义相册中显示）
        RightMenuItem {
            text: qsTr("Remove from album")
            visible: type !== GlobalVar.ThumbnailType.Trash && (type === GlobalVar.ThumbnailType.CustomAlbum)
            onTriggered: {

            }
        }

        MenuSeparator {
            visible: type !== GlobalVar.ThumbnailType.Trash
            height: visible ? GlobalVar.rightMenuSeparatorHeight : 0
        }

        //添加到我的收藏
        RightMenuItem {
            id: favoriteAction
            text: qsTr("Favorite")
            visible: type !== GlobalVar.ThumbnailType.Trash && (canFavorite)
            onTriggered: {

            }
        }

        //从我的收藏中移除
        RightMenuItem {
            id: unFavoriteAction
            text: qsTr("Unfavorite")
            visible: type !== GlobalVar.ThumbnailType.Trash && (!canFavorite)
            onTriggered: {

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
            visible: type !== GlobalVar.ThumbnailType.Trash && (canRotate)
            onTriggered: {

            }
        }

        //逆时针旋转
        RightMenuItem {
            text: qsTr("Rotate counterclockwise")
            visible: rotateClockwiseAction.visible
            onTriggered: {

            }
        }

        MenuSeparator {
            visible: type !== GlobalVar.ThumbnailType.Trash
            height: visible ? GlobalVar.rightMenuSeparatorHeight : 0
        }

        //设置为壁纸
        RightMenuItem {
            text: qsTr("Set as wallpaper")
            visible: type !== GlobalVar.ThumbnailType.Trash && (fileControl.isCanReadable(path) && theView.ism.length === 1 && fileControl.pathExists(path))
            onTriggered: {
                fileControl.setWallpaper(path)
            }
        }

        //在文件管理器中显示
        RightMenuItem {
            text: qsTr("Display in file manager")
            visible: type !== GlobalVar.ThumbnailType.Trash && (theView.ism.length == 1 && fileControl.pathExists(path))
            onTriggered: {
                fileControl.displayinFileManager(path)
            }
        }

        //恢复
        RightMenuItem {
            text: qsTr("Restore")
            visible: type === GlobalVar.ThumbnailType.Trash
            onTriggered: {

            }
        }

        //照片信息
        RightMenuItem {
            text: qsTr("Photo info")
            visible: theView.ism.length == 1 && fileControl.isImage(path)
            onTriggered: {

            }
        }

        //视频信息
        RightMenuItem {
            text: qsTr("Video info")
            visible: theView.ism.length == 1 && !fileControl.isImage(path)
            onTriggered: {

            }
        }
    }
}
