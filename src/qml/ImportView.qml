import QtQuick 2.0
import org.deepin.dtk 1.0


Rectangle{


    Rectangle{
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        ActionButton {
            id: openViewImageIcon
            anchors.top: parent.top
            anchors.topMargin: -70
            anchors.left : parent.left
            anchors.leftMargin: -width/2

            icon {
                name:"nopicture2"
                width: 140
                height: 140
            }
        }
        RecommandButton{
            id: openPictureBtn
            font.capitalization: Font.MixedCase
            text: qsTr("导入图片和视频")
            onClicked:{
            }
            width: 302
            height: 36
            anchors.top:openViewImageIcon.bottom
            anchors.topMargin:10

            anchors.left : parent.left
            anchors.leftMargin: -width/2
        }
        Label{
            anchors.top:openPictureBtn.bottom
            anchors.topMargin: 20
            anchors.left : parent.left
            anchors.leftMargin: -width/2
            text:qsTr("您也可以拖拽图片和视频到此")
        }
    }
}
