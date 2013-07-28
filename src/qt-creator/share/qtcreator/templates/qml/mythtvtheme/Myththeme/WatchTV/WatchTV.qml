import QtQuick 2.0
import MythSystemTools 1.0
import QtQuick.Window 2.0
Image {
    width: Screen.width
    height: Screen.height
    source: "../artwork/background.png"
    Text {
        id: fds
        color: "white"
        anchors.centerIn: parent
        text: qsTr("Opening TV Please Wait")
        font.pixelSize: Math.round(parent.height / 20 )
    }
    Component.onCompleted: {
        la.launchScript("mythavtest")
    }
    ScriptLauncher{
        id: la
    }
    //#FIXME add a backbutton
}
