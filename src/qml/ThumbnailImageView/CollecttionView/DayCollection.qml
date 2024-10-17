// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Dialogs
import org.deepin.dtk 1.0

import org.deepin.album 1.0 as Album

import "../../Control"
import "../../Control/ListView"
import "../../Control/Animation"
import "../../"

SwitchViewAnimation {
    id: dayView

    signal sigListViewPressed(int x, int y)
    signal sigListViewReleased(int x, int y)
    property int scrollDelta: 60
    property int timeLineLblHeight: 36
    property int timeLineLblMargin: 10
    property int selAllCheckBoxHeight: 22
    // property int rowSizeHint: (width - GStatus.thumbnailListRightMargin) / GStatus.cellBaseWidth
    // property real realCellWidth : (width - GStatus.thumbnailListRightMargin) / rowSizeHint
    // property var dayHeights: []

    property var selectedPaths: GStatus.selectedPaths
    property string numLabelText: "" //总数标签显示内容
    property string selectedText: getSelectedText(selectedPaths)

    // property alias count: theModel.count

    Rectangle {
        anchors.fill : parent
        color: DTK.themeType === ApplicationHelper.LightType ? "#f8f8f8"
                                                              : "#202020"
        visible: numLabelText !== ""
        Album.QmlWidget{
            id: timeline
            anchors.fill: parent
            anchors.margins: 0
            focus: false
        }
    }


    Connections {
        target: GStatus
        function onSigDoubleClickedFromQWidget(url) {
            console.log("double clicked", url)
            if (url !== undefined) {
                var allUrls = timeline.allUrls()
                menuItemStates.executeViewImageCutSwitch(url, allUrls)
            }
        }

        function onSigMenuItemClickedFromQWidget(id, uid) {
            var sels = GStatus.selectedPaths
            var url = ""
            if (sels.length > 0)
                url = sels[0]
            var allUrls = timeline.allUrls()
            if (id === Album.Types.IdView) {
                menuItemStates.executeViewImageCutSwitch(url, allUrls)
            } else if (id === Album.Types.IdMoveToTrash) {
                deleteDialog.setDisplay(Album.Types.TrashNormal, GStatus.selectedPaths.length)
                deleteDialog.show()
            } else if (id === Album.Types.IdFullScreen) {
                menuItemStates.executeFullScreen(url, allUrls)
            } else if (id === Album.Types.IdPrint) {
                menuItemStates.executePrint()
            } else if (id === Album.Types.IdStartSlideShow) {
                menuItemStates.excuteSlideShow(allUrls)
            } else if (id === Album.Types.IdExport) {
                menuItemStates.excuteExport()
            } else if (id === Album.Types.IdCopyToClipboard) {
                menuItemStates.executeCopy()
            } else if (id === Album.Types.IdAddToFavorites) {
                menuItemStates.executeFavorite()
            } else if (id === Album.Types.IdRemoveFromFavorites) {
                menuItemStates.executeUnFavorite()
            } else if (id === Album.Types.IdRotateClockwise) {
                menuItemStates.executeRotate(90)
            } else if (id === Album.Types.IdRotateCounterclockwise) {
                menuItemStates.executeRotate(-90)
            } else if (id === Album.Types.IdSetAsWallpaper) {
                menuItemStates.executeSetWallpaper()
            } else if (id === Album.Types.IdDisplayInFileManager) {
                menuItemStates.executeDisplayInFileManager()
            } else if (id === Album.Types.IdImageInfo) {
                menuItemStates.executeViewPhotoInfo()
            } else if (id === Album.Types.IdVideoInfo) {
                menuItemStates.executeViewVideoInfo()
            } else if (id === Album.Types.IdNewAlbum) {
                newAlbum.isChangeView = false
                newAlbum.importSelected = true
                newAlbum.setNormalEdit()
                newAlbum.show()
            } else if (id === Album.Types.IdAddToAlbum) {
                // 获取所选自定义相册的Id，根据Id添加到对应自定义相册
                albumControl.insertIntoAlbum(uid , GStatus.selectedPaths)
                DTK.sendMessage(thumbnailImage, qsTr("Successfully added to “%1”").arg(albumControl.getCustomAlbumByUid(uid)), "notify_checked")
            }
        }

        function onSigZoomInOutFromQWidget(delta) {
            var curValue = statusBar.sliderValue
            if (delta > 0)
                statusBar.setSliderWidgetValue(curValue + 1)
            else
                statusBar.setSliderWidgetValue(curValue - 1)
        }
    }

    onVisibleChanged: {
        // 窗口显示时，重置显示内容
        if (visible && !GStatus.backingToMainAlbumView) {
            flushView()
        }
    }

    function flushView() {
        timeline.refresh()
        getNumLabelText()
    }

    function unSelectAll() {
        timeline.unSelectAll()
    }

    function runDeleteImg() {
        menuItemStates.executeDelete()
        getNumLabelText()
    }

    Connections {
        target: collecttionView
        function onFlushDayViewStatusText() {
            if (visible) {
                if (selectedPaths.length > 0)
                    getSelectedText(selectedPaths)
                else
                    getNumLabelText()
            }
        }
    }

    Connections {
        target: albumControl
        function onSigRepeatUrls(urls) {
            if (visible && collecttionView.currentViewIndex === 2) {
                ///theView.sigUnSelectAll()
                selectedPaths = urls
                if (selectedPaths.length > 0)
                    getSelectedText(selectedPaths)
                else
                    getNumLabelText()
                GStatus.selectedPaths = selectedPaths
            }
        }

        //收到导入完成消息
        // function onSigImportFinished() {
        //     if (visible) {
        //         //刷新数量显示
        //         getNumLabelText()
        //     }
        // }
    }

    // 刷新总数标签
    function getNumLabelText() {
        //QML的翻译不支持%n的特性，只能拆成这种代码

        var photoCountText = ""
        var photoCount = albumControl.getAllInfoConut(1)
        if(photoCount === 0) {
            photoCountText = ""
        } else if(photoCount === 1) {
            photoCountText = qsTr("1 photo")
        } else {
            photoCountText = qsTr("%1 photos").arg(photoCount)
        }

        var videoCountText = ""
        var videoCount = albumControl.getAllInfoConut(2)
        if(videoCount === 0) {
            videoCountText = ""
        } else if(videoCount === 1) {
            videoCountText = qsTr("1 video")
        } else {
            videoCountText = qsTr("%1 videos").arg(videoCount)
        }

        numLabelText = photoCountText + (videoCountText !== "" ? ((photoCountText !== "" ? " " : "") + videoCountText) : "")

        if (visible) {
            GStatus.statusBarNumText = numLabelText
        }
    }

    // 刷新选中项目标签内容
    function getSelectedText(paths) {
        var selectedNumText = GStatus.getSelectedNumText(paths, numLabelText)
        if (visible)
            GStatus.statusBarNumText = selectedNumText
        return selectedNumText
    }

    //月视图切日视图
    function scrollToMonth(year, month) {
        timeline.navigateToMonth(year+"/"+month)
    }

    // function unSelectAll() {
    //     theView.sigUnSelectAll()
    //     selectedPaths = []
    //     GStatus.selectedPaths = []
    // }

    // function flushView() {
    //     //0.清理
    //     theModel.clear()
    //     theModel.selectedPathObjs = []
    //     selectedPaths = []
    //     dayHeights = []
    //     //1.获取日期
    //     var days = []
    //     if (Number(FileControl.getConfigValue("", "loadDayView", 1)))
    //         days = albumControl.getDays()

    //     //2.构建model
    //     var dayHeight = 0
    //     var listHeight = 0
    //     theView.listContentHeight = 0
    //     var dayPaths
    //     for (var i = 0;i !== days.length;++i) {
    //         theModel.append({dayToken: days[i]})

    //         // 当前日期列表选中数据初始化
    //         dayPaths = []
    //         theModel.selectedPathObj = {"id": i, "paths": dayPaths}
    //         theModel.selectedPathObjs.push(theModel.selectedPathObj)

    //         // 计算每个日期列表高度
    //         dayPaths = albumControl.getDayPaths(days[i])
    //         listHeight = Math.abs(Math.ceil(dayPaths.length / Math.floor(width / realCellWidth)) * realCellWidth)
    //         dayHeight = timeLineLblHeight + timeLineLblMargin + selAllCheckBoxHeight + listHeight
    //         dayHeights.push(dayHeight)
    //         theView.listContentHeight += dayHeight
    //     }
    // }

    // function executeScrollBar(delta) {
    //     if (theView.contentHeight <= theView.height)
    //         return

    //     vbar.active = true
    //     theView.contentY -= delta

    //     if(vbar.position < 0) {
    //         vbar.position = 0
    //     } else if(vbar.position > 1 - theView.height / theView.contentHeight) {
    //         vbar.position = 1 - theView.height / theView.contentHeight
    //     }
    // }

    // // 日视图的标题栏
    // Item {
    //     id: allCollectionTitleRect
    //     width: parent.width - GStatus.verticalScrollBarWidth
    //     height: timeLineLblMargin + timeLineLblHeight + selAllCheckBoxHeight
    //     z: 2
    //     Rectangle {
    //         color: DTK.themeType === ApplicationHelper.LightType ? "#f8f8f8"
    //                                                               : "#202020"
    //         anchors.fill : parent
    //         opacity: 0.95
    //     }

    //     // 标题栏时间标签
    //     Label {
    //         id: titleTimeLineLabel
    //         z:3
    //         anchors {
    //             top: parent.top
    //             topMargin: timeLineLblMargin
    //             leftMargin: timeLineLblMargin
    //             left: parent.left
    //         }
    //         font: DTK.fontManager.t3
    //         visible: text !== ""
    //         text : ""
    //     }

    //     //标题栏照片数label
    //     Label {
    //         id: titlePhotoNumberLabel
    //         z:3
    //         height: selAllCheckBoxHeight
    //         anchors {
    //             top: titleTimeLineLabel.bottom
    //             leftMargin: timeLineLblMargin
    //             left: parent.left
    //         }
    //         topPadding: -1
    //         text : ""
    //         visible: text !== ""
    //     }
    // }

    // ListView {
    //     id: theView
    //     clip: true
    //     model: theModel
    //     width: parent.width
    //     height: parent.height
    //     delegate: theDelegate
    //     interactive: false

    //     signal sigUnSelectAll()
    //     signal dbClicked(string url)

    //     MouseArea {
    //         id: theMouseArea

    //         onDoubleClicked: (mouse)=> {
    //             if (GStatus.selectedPaths.length > 0)
    //                 theView.dbClicked(GStatus.selectedPaths[0])

    //             parent.inPress = false
    //             rubberBand.clearRect()

    //             mouse.accepted = true
    //         }

    //         onWheel: (wheel)=> {
    //             var datla = wheel.angleDelta.y / 2
    //             if (Qt.ControlModifier & wheel.modifiers) {
    //                 // 按住ctrl，缩放缩略图
    //                 var curValue = statusBar.sliderValue
    //                 if (datla > 0)
    //                     statusBar.setSliderWidgetValue(curValue + 1)
    //                 else
    //                     statusBar.setSliderWidgetValue(curValue - 1)
    //             } else {
    //                 // 正常滚动显示缩略图内容
    //                 executeScrollBar(datla)
    //             }
    //         }
    //     }
    // }
    Connections {
        target: GStatus

        function onSigSelectAll(sel) {
            if (visible) {
                if (sel)
                    GStatus.sigKeyPressFromQml("Ctrl+A")
                else
                    GStatus.sigKeyPressFromQml("Esc")
            }
        }

        function onSigPageUp() {
            if (visible) {
                GStatus.sigKeyPressFromQml("Page Up")
            }
        }

        function onSigPageDown() {
            if (visible) {
                GStatus.sigKeyPressFromQml("Page Down")
            }
        }
    }

    // Component {
    //     id: theDelegate

    //     Item {
    //         id: delegateRect
    //         width: theView.width
    //         height: timeLineLblHeight + timeLineLblMargin + selAllCheckBoxHeight + theSubView.height

    //         property string m_dayToken: dayToken

    //         Label {
    //             id: timeLineLabel
    //             font: DTK.fontManager.t3
    //             height: timeLineLblHeight
    //             anchors {
    //                 top: parent.top
    //                 topMargin: timeLineLblMargin
    //                 left: parent.left
    //                 leftMargin: timeLineLblMargin
    //             }
    //         }

    //         CheckBox {
    //             id: selectAllBox
    //             height: selAllCheckBoxHeight
    //             anchors {
    //                 top: timeLineLabel.bottom
    //                 left: timeLineLabel.left
    //             }
    //             checked: theSubView.haveSelectAll
    //             visible: selectedPaths.length > 0
    //             onClicked: {
    //                 if(checked) {
    //                     theSubView.selectAll(true)
    //                 } else {
    //                     theSubView.selectAll(false)
    //                 }
    //             }
    //         }

    //         Connections {
    //             target: dayView
    //             function onSigListViewPressed(x, y) {
    //                 var object = selectAllBox.mapFromGlobal(x,y)
    //                 if (selectAllBox.contains(object)) {
    //                     checkBoxClicked = true
    //                     if (selectAllBox.checkState === Qt.Checked) {
    //                         selectAllBox.checkState = Qt.Unchecked
    //                         theSubView.selectAll(false)
    //                     } else {
    //                         selectAllBox.checkState = Qt.Checked
    //                         theSubView.selectAll(true)
    //                     }
    //                 }
    //             }

    //             function onSigListViewReleased(x, y) {
    //                 checkBoxClicked = false
    //             }
    //         }

    //         Label {
    //             id: numLabelTitle
    //             height: selAllCheckBoxHeight
    //             anchors {
    //                 top: timeLineLabel.bottom
    //                 left: selectAllBox.visible ? selectAllBox.right : timeLineLabel.left
    //             }
    //             topPadding: -1
    //         }

    //         ThumbnailListViewAlbum {
    //             id: theSubView
    //             // 监听缩略图子控件选中状态，一旦改变，更新日视图所有选中路径
    //             Connections {
    //                 target: theSubView
    //                 function onSelectedChanged() {
    //                     if (index > -1) {
    //                         theModel.selectedPathObjs[index].paths = theSubView.selectedUrls
    //                     }
    //                     updateSelectedPaths()
    //                 }
    //             }

    //             Connections {
    //                 target: theView
    //                 function onSigUnSelectAll() {
    //                     theSubView.selectAll(false)
    //                 }
    //             }

    //             Connections {
    //                 target: theView
    //                 function onDbClicked(url) {
    //                     var openPaths = theSubView.allUrls()
    //                     if (openPaths.indexOf(url) !== -1) {
    //                         var pos = theMouseArea.mapToItem(theSubView, rubberBand._left(), rubberBand._top())
    //                         theSubView.viewImageFromOuterDbClick(pos.x, pos.y)
    //                     }
    //                 }
    //             }
    //         }

    //         function flushView() {
    //             var picTotal = albumControl.getDayInfoCount(m_dayToken, 3)
    //             var videoTotal = albumControl.getDayInfoCount(m_dayToken, 4)
    //             //1.刷新图片显示
    //             dataModel.dayToken = m_dayToken
    //             theSubView.proxyModel.refresh()

    //             //2.刷新checkbox
    //             var str = generatePhotoVideoString(picTotal, videoTotal);
    //             numLabelTitle.text = str

    //             //3.刷新标题
    //             var dates = m_dayToken.split("-")
    //             timeLineLabel.text = qsTr("%1/%2/%3").arg(dates[0]).arg(Number(dates[1])).arg(Number(dates[2]))

    //             if (isFirstLoad) {
    //                 isFirstLoad = false
    //                 timelineLabelTextUpdate(timeLineLabel.text)
    //                 numLabelTextUpdate(numLabelTitle.text)
    //             }
    //         }

    //         Component.onCompleted: {
    //             flushView()
    //         }
    //     }
    // }

    Component.onCompleted: {
        GStatus.sigFlushAllCollectionView.connect(flushView)
        deleteDialog.sigDoDeleteImg.connect(runDeleteImg)
    }
}
