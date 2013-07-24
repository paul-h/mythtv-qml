import QtQuick 1.1

Rectangle {
    id: rootTangle
    width: parent.width
    height: parent.height
    color: "green"

    Text {
        id: text1
        width: 291
        height: 48
        anchors.centerIn: parent
        text: qsTr("MythTV QML ")
        font.pointSize: 22
        font.pixelSize: 12
        Behavior on rotation {NumberAnimation{from: 0 ;to: 360 ;duration:  3200 ; easing.type: Easing.OutCirc}}
        Behavior on color {NumberAnimation { duration: 1200; easing.type: Easing.InOutQuad }}
    }
    MouseArea{
        anchors.fill: text1
        onClicked: {
            text1.rotation = 45
            text1.color = "green"
        }
    }
}
