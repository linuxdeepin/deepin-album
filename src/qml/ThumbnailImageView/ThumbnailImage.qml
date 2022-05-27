import QtQuick 2.0
import org.deepin.dtk 1.0
import QtQuick.Controls 2.4
import QtQml 2.11
import "./CollecttionView"
import "./HaveImportedView"
import "./MyFavoriteView"
import "./RecentlyDeletedView"
import "./CustomAlbum"
import "./"
//本文件用于替代stackwidget的作用，通过改变global的0-n来切换窗口
Rectangle{
    property int m_topMargin: 0
    property int m_leftMargin: 20

    ImportView{
        visible: global.currentViewIndex == 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
    NoPictureView{
        visible: global.currentViewIndex == 1
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
    CollecttionView{
        visible: global.currentViewIndex == 2
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    HaveImportedView{
        visible: global.currentViewIndex == 3
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    MyFavoriteView{
        visible: global.currentViewIndex == 4
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    RecentlyDeletedView{
        visible: global.currentViewIndex == 5
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
    CustomAlbum{
        visible: global.currentViewIndex == 6
        anchors.topMargin: m_topMargin
        anchors.leftMargin: m_leftMargin
        anchors.fill: parent
    }
}
