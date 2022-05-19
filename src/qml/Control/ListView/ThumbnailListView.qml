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
Item {
    id : root

    //当前区域时间区间改变信号，字符串可以直接刷新在界面上
    signal timeChanged(string str)

    //整个缩略图的图片/视频统计信息改变，字符串可以直接刷新在界面上
    signal countChanged(string str)

    //设置图片缩放等级，传入的参数为外部滑动条的值
    function setPicZoomLevel(level)
    {
        ;
    }

    //重设整个model，以此来刷新整个缩略图界面
    function setPaths(paths)
    {
        theModel.clear()
        for(var i = 0;i < paths.length;++i)
        {
            //model似乎满足JSON格式即可
            theModel.append({path : paths[i], displayFlushHelper : 0})
        }

        //统计并对外同步当前显示的视频、图片的数量
        countChanged(tools.getFileCountStr(paths))
    }

    //强制重新刷新整个缩略图界面
    function fouceUpdate()
    {
        //QML的图片强制重刷机制：改变图片路径
        theView.displayFlushHelper = Math.random()
    }

    //view依赖的model管理器
    property ListModel thumbnailListModel: ThumbnailListModel {}

    //缩略图view的本体
    GridView {
        id: theView
        anchors.fill: parent
        anchors.margins: 10
        clip: true
        interactive: false //禁用原有的交互逻辑，重新开始定制
        cellWidth: 110
        cellHeight: 110
        model: thumbnailListModel
        delegate: ThumbnailListDelegate{
            id: thumbnailListDelegate
            m_index: index
            m_path: thumbnailListModel.get(index).path
        }

        currentIndex: -1

        //激活滚动条
        ScrollBar.vertical: ScrollBar {
            id: vbar
            active: true
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

        //刷新选中的元素
        //此函数会极大影响框选时的性能表现
        function flushIsm()
        {
            var startX = rubberBand.x
            var startY = rubberBand.y + contentY //rubberBand.y是相对于parent窗口的坐标，itemAt的时候是用的view的内部坐标，因此要加上滚动偏移
            var lenX = Math.max(rubberBand.m_width, 1)
            var lenY = Math.max(rubberBand.m_height, 1)

            var tempArray = ism

            //统计框入了哪些index
            //TODO: 后续考虑使用框选矩形区域判断所有item中心点是否被包含其中，来判定框入了哪些index
            for(var i = startX;i < startX + lenX;i += 10)
            {
                for(var j = startY;j < startY + lenY;j += 10)
                {
                    var index = getItemIndexFromAxis(i, j)
                    if(index !== -1 && tempArray.indexOf(index) == -1)
                    {
                        tempArray.push(index)
                    }
                }
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

            //刷新整个view的选择效果
            ism = tempArray
        }

        MouseArea {
            //按下时的起始坐标
            property int pressedXAxis: -1
            property int pressedYAxis: -1

            anchors.fill: parent
            acceptedButtons: Qt.LeftButton //仅激活左键

            onPressed: {
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
                    }
                }
            }

            onMouseXChanged: {
                rubberBand.m_width = Math.abs(mouseX - pressedXAxis)
                rubberBand.x = Math.min(mouseX, pressedXAxis)
                if(rubberBand.m_width > 0)
                {
                    parent.ism = []
                    parent.rubberBandDisplayed = true
                    parent.flushIsm()
                }

                //处理结束，记录
                rubberBand.m_lastWidth = rubberBand.m_width
                rubberBand.m_lastX = rubberBand.x
            }

            onMouseYChanged: {
                rubberBand.m_height = Math.abs(mouseY - pressedYAxis)
                rubberBand.y = Math.min(mouseY, pressedYAxis)
                if(rubberBand.m_height > 0)
                {
                    parent.ism = []
                    parent.rubberBandDisplayed = true
                    parent.flushIsm()
                }

                //处理结束，记录
                rubberBand.m_lastHeight = rubberBand.m_height
                rubberBand.m_lastY = rubberBand.y
            }

            onReleased: {
                if(parent.rubberBandDisplayed == false)
                {
                    var index = parent.getItemIndexFromAxis(mouseX, mouseY + parent.contentY)
                    if(index !== -1)
                    {
                        parent.ism = [index]
                    }
                }

                parent.inPress = false
                pressedXAxis = -1
                pressedYAxis = -1
                rubberBand.m_width = 0
                rubberBand.m_height = 0
            }

            onWheel: {
                // 滚动时，激活滚动条显示
                vbar.active = true
                //滚动事件
                var datla = wheel.angleDelta.y / 2
                parent.contentY -= datla
                if(parent.contentY < 0)
                {
                    parent.contentY = 0
                }
                else if(parent.contentY > parent.contentHeight - parent.height)
                {
                    parent.contentY = parent.contentHeight - parent.height
                }

//                //统计当前页面的缩略图时间范围
//                var item1 = theView.itemAt(100, 100);
//                var item2 = theView.itemAt(theView.width - 200, theView.height - 200)
//                var str = tools.getFileTime(item1.m_path, item2.m_path)
//                timeChanged(str)
            }
        }

        //橡皮筋控件
        RubberBand {
            id: rubberBand
            visible: parent.inPress
        }
    }
}
