import QtQuick 2.0
import "ubuntu/components"
import QtQuick.XmlListModel 2.0
import QtQuick.Particles 2.0
import QtQuick.LocalStorage 2.0
import  MythTv 1.0

MainView {
    objectName: "mainView"
//    applicationName: "mythbuntu-phablet"
    width: parent.width//units.gu(100)
    height: parent.height//units.gu(75)
    //start the theming engine here have sqllight DB that controls what themes are selected.
    // figure out a way to implat downloading and installing new themes and some sorta package management system.
    // Try to keep this all QML

    Tabs {
        id: tabs
        anchors.fill: parent
        // First tab begins here
//        Tab {
//            objectName: "Home"
//            title: qsTr("Home")
//            page:
//                Loader {
//                id: homeLoader
//                anchors{
//                    top: parent.top
//                    topMargin: units.gu(9)
//                    bottom: parent.bottom
//                    left: parent.left
//                    right: parent.right
//                }
//                source:    Qt.resolvedUrl("UbuntuSDK/StartHere.qml")

//}

//        }
        Tab{
            objectName: "Videos"
            title:qsTr("Videos")

            page: Videos{
                anchors{
                    top: parent.top
                    topMargin: 9
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                }
            }
        }

//        Tab{
//            objectName: "Remote"
//            title:qsTr("Remote")
//            page: Remote{
//                anchors{
//                    top: parent.top
//                    topMargin: units.gu(9)
//                    bottom: parent.bottom
//                    left: parent.left
//                    right: parent.right
//                }
//            }
//        }
        Tab{
            objectName: "Schedule"
            title:qsTr("Schedule")
            page: Schedule{
                anchors{
                    top: parent.top
                    topMargin: 9
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                }
            }
        }
        Tab{
            objectName: "CaptureCards"
            title:qsTr("CaptureCards")
            page: CaptureCards{
                anchors{
                    top: parent.top
                    topMargin: 9
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                }
            }
        }
        Tab{
            objectName: "SetUp"
            title:qsTr("SetUp")
            page: StartHere{
                anchors{
                    top: parent.top
                    topMargin: 9
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                }
            }
        }
        Tab{
            objectName: "Theme"
            title:qsTr("Theme")
            page: ThemingSettings{
                anchors{
                    top: parent.top
                    topMargin: 9
                    bottom: parent.bottom
                    left: parent.left
                    right: parent.right
                }
            }
        }
    }
}
