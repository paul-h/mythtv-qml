import QtQuick 2.0
import QtQuick.Window 2.0
import MythTv 1.0
Image {
    id:root
    source: "../artwork/background.png"
    width: Screen.width
    height: Screen.height

    Loader{
        id: settingsPageLoader
        anchors.fill: parent
        source: ""
        opacity:  0
        Behavior on opacity {
            NumberAnimation {
                target: settingsPageLoader
                property: "scale"
                from:0
                to:1
                duration: 1800
                easing.type: Easing.OutQuart
            }
        }
      onStatusChanged: {
          if (status === Loader.Ready)
              backaroo.opacity = 0
      }
    }
    BackButton{
        id: backaroo
        backButtonWidth: parent.width / 6
        backButtionHeight:  parent.width / 6
        iconSource: "../artwork/icon.png"
        backButtonsmooth: false
        MouseArea{
            anchors.fill: backaroo
            onClicked:{
                settingsPageLoader.source = "../StartHere.qml"
                settingsPageLoader.opacity = 1
                root.opacity = 0
            }
        }
    }

    Text {
        id: fd
        text: qsTr("Coming Soon")
        color: "white"
        font.pixelSize: Math.round(parent.height / 20 )
        anchors.centerIn: parent

    }
}
