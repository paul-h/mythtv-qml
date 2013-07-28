import QtQuick 2.0
import MythTv 1.0
MythRemoteTextAll{

    width: parent.width
    height:  parent.height
    backgroundImage: Qt.resolvedUrl("../artwork/background.png")
    buttonButtonColor: "green"
    buttonNameColor: "white"
    buttonRadius:  360
    buttonBorderWidth: 1
    buttonBorderColor: "blue"
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
//                pageLoader.source = ""
//                pageLoader.opacity = 0
//                mediaLibRoot.opacity = 0
            }
        }
    }
}
