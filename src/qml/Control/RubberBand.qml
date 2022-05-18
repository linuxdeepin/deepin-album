import QtQuick 2.9
import QtQuick.Window 2.2
import QtQuick.Controls 2.1

//橡皮筋控件
//此处仅预留控制接口，具体控制逻辑由调用方编写

Item {
//    id: root

    //选择框当前的大小
    property int m_width: 0
    property int m_height: 0

    //上一次的坐标，外部调用者根据需要自行设置
    property int m_lastX: 0
    property int m_lastY: 0
    property int m_lastWidth: 0
    property int m_lastHeight: 0

    visible: false

    Rectangle {
        width: m_width
        height: m_height

        //TODO：颜色参数和透明度需要和UI确认好，目前DTK没有提供相关的控件，也无法获取系统设置的参数
        color: "#1E90FF"
        opacity: 0.4
    }
}
