import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Window 2.10
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11

import org.deepin.dtk 1.0 as D
import org.deepin.dtk 1.0

DialogWindow {

    id: exportdialog
    modality: Qt.WindowModal
    flags: Qt.Window | Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint
    title: " "
    visible: false

    minimumWidth: 380
    maximumWidth: 380
    minimumHeight: 280
    maximumHeight: 280

    width: 380
    height: 280

    property string filePath: ""
    property string saveName: nameedit.text
    property int saveIndex: saveCombox.currentIndex
    property string savefileFormat: formatCombox.displayText
    property int pictureQuality: piczSlider.value
    property string saveFolder: ""
    property var messageToId: thumbnailImage

    icon : "deepin-album"

    Label{
        id:nameLabel
        width:42
        height: 35
        font.pixelSize: 14
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: nameedit.top
        anchors.topMargin:0
        text:qsTr("Name:")
    }
    LineEdit {
        id: nameedit
        anchors.top: parent.top
        anchors.topMargin: 16
        anchors.right: parent.right
        anchors.rightMargin: 10
        width: 260
        height: 35
        font: DTK.fontManager.t5
        focus: true
        maximumLength: 255
        validator: RegExpValidator {regExp: /^[^\\.\\\\/\':\\*\\?\"<>|%&][^\\\\/\':\\*\\?\"<>|%&]*/ }
        text: ""
        selectByMouse: true
//        alertText: qsTr("The file already exists, please use another name")
//        showAlert: fileControl.isShowToolTip(source,nameedit.text)
    }

    Label{
        id:saveLabel
        width:42
        height: 35
        font.pixelSize: 14
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: nameLabel.bottom
        anchors.topMargin:10
        text:qsTr("Save to:")
    }

    ComboBox{
        id :saveCombox
        width:260
        height: 35
        font.pixelSize: 14
        anchors.top: nameedit.bottom
        anchors.topMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10

        textRole: "key"
        model: ListModel {
            ListElement { key: qsTr("Pictures"); value: 0 }
            ListElement { key: qsTr("Documents"); value: 1 }
            ListElement { key: qsTr("Downloads"); value: 2 }
            ListElement { key: qsTr("Desktop"); value: 3 }
            ListElement { key: qsTr("Videos"); value: 4 }
            ListElement { key: qsTr("Music"); value: 5 }
            ListElement { key: qsTr("Select other directories"); value: 6 }
        }
        delegate:    ItemDelegate {
            text: key
            width: parent.width-10
            height : 35
            checked: index == 0
            backgroundVisible: false
            onClicked: {
                saveIndex = value
                if(value == 6){

                    exportdialog.flags = Qt.Window
                    var tmpFolder = albumControl.getFolder();
                    if(tmpFolder.length >0){
                        key = tmpFolder
                    }else{
                        key = qsTr("Select other directories")
                    }
                    exportdialog.flags = Qt.Window | Qt.WindowCloseButtonHint | Qt.WindowStaysOnTopHint

                    saveFolder = key
                }
            }
        }

    }



    Label{
        id:fileFormatLabel
        width:42
        height: 35
        font.pixelSize: 14
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: saveLabel.bottom
        anchors.topMargin:10
        text:qsTr("Format:")
    }

    ComboBox{
        id :formatCombox
        width:260
        height: 35
        font.pixelSize: 14
        anchors.top: saveCombox.bottom
        anchors.topMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
        model :albumControl.imageCanExportFormat(filePath).length
        displayText: albumControl.imageCanExportFormat(filePath)[currentIndex]
        delegate:    ItemDelegate {
            text: albumControl.imageCanExportFormat(filePath)[index]
            width: parent.width-10
            height : 35
            checked: index == 0
            backgroundVisible: false
            onClicked: {

            }
        }
    }

    Label{
        id:piczLabel
        width:42
        height: 35
        font.pixelSize: 14
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: fileFormatLabel.bottom
        anchors.topMargin:10
        text:qsTr("Quality:")
    }

    Slider{
        id :piczSlider
        width:200
        height: 30
        font.pixelSize: 14
        anchors.top: formatCombox.bottom
        anchors.topMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 70
        from: 1
        value: 100
        to: 100
        stepSize:1
    }

    Label{
        id:bfLbale
        width:40
        height: 30
        font.pixelSize: 14
        anchors.top: formatCombox.bottom
        anchors.topMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
        text: piczSlider.value +"%"
    }


    Button {
        id: cancelbtn
        anchors.top: bfLbale.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: 0
        text: qsTr("Cancel")
        width: 170
        height: 32
        font.pixelSize: 16
        onClicked: {
            exportdialog.visible=false
        }
    }

    Button {
        id: enterbtn
        anchors.top: bfLbale.bottom
        anchors.topMargin: 10
        anchors.left: cancelbtn.right
        anchors.leftMargin: 10
        text: qsTr("Confirm")
        enabled: true
        width: 170
        height: 32

        onClicked: {
            if (saveName === "") {
                exportdialog.visible = false
                emptyWarningDig.show()
                return
            }

            var bRet = albumControl.saveAsImage(filePath , saveName , saveIndex , savefileFormat ,pictureQuality ,saveFolder)
            exportdialog.visible=false
            if (bRet)
                DTK.sendMessage(messageToId, qsTr("Export successful"), "checked")
            else
                DTK.sendMessage(messageToId, qsTr("Export failed"), "warning")

        }
    }

    onVisibleChanged: {
        // 窗口显示时，重置显示内容
        if (visible) {
            nameedit.text = fileControl.slotGetFileName(filePath)
            piczSlider.value = 100
        }

        setX(root.x  + root.width / 2 - width / 2)
        setY(root.y  + root.height / 2 - height / 2)
    }

    function setParameter(path, toId) {
        filePath = path
        messageToId = toId
    }
}
