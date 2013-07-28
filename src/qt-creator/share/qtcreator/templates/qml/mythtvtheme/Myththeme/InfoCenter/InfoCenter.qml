import QtQuick 2.0
import QtQuick.XmlListModel 2.0
import QtQuick.LocalStorage 2.0
import MythTv 1.0
Image{
    id: root
    width: parent.width
    height: parent.height
    source: "../artwork/background.png"
            ListView{
                id: versionInfo
                model: versions
                delegate: Item{
                    width: root.width
                    height: root.height /2
                    Column{
                        width:  root.width
                        height: root.height /2
                        spacing: 4
                    Rectangle{
                        id: branchRec
                        width:  root.width
                        height: root.height / 10
                        color: "#64C7C7C7"
                    Text {
                        id: branchTxt
                        font.pixelSize: 42
                        color: "lightblue"
                        text:"Myth Branch Version: \t "+ Branch
                    anchors.centerIn: branchRec
                    }
                }

                    Rectangle{
                        id: versionRec
                        width:  root.width
                        height: root.height / 10
                        color: "#64C7C7C7"
                    Text {
                        id: versionTxt
                        font.pixelSize: 42
                        color: "lightblue"
                        text:"Myth Backend Version: \t "+ Version
                    anchors.centerIn: versionRec
                    }
                }
                    Rectangle{
                        id: protocolRec
                        width:  root.width
                        height: root.height / 10
                        color: "#64C7C7C7"
                    Text {
                        id: protocolTxt
                        font.pixelSize: 42
                        color: "lightblue"
                        text:"Myth Protocol Version: \t "+ Protocol
                    anchors.centerIn: protocolRec
                    }
                }


                    Rectangle{
                        id: schemaRec
                        width:  root.width
                        height: root.height / 10
                        color: "#64C7C7C7"
                    Text {
                        id: schemaTxt
                        font.pixelSize: 42
                        color: "lightblue"
                        text:"Myth Schema Version: \t "+ Schema
                    anchors.centerIn: schemaRec
                    }
                }
                    MythDBInfo{
                        width:  parent.width
                        height:     parent.height
                    }

}

                }//Item
}
//FIXME ADD A BACK BUTTON
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
                        pageLoader.source = ""
                        pageLoader.opacity = 0
                        mediaLibRoot.opacity = 0
                    }
                }
            }
    XmlListModel{
        id: versions
        source: DataBase.ipAddress() + ":"+ DataBase.port() +"/Myth/GetConnectionInfo"
        query: "//ConnectionInfo/Version"
        XmlRole{name: "Version"; query: "Version/string()"  }
        XmlRole{name: "Branch"; query: "Branch/string()"  }
        XmlRole{name: "Protocol"; query: "Protocol/string()"  }
        XmlRole{name: "Binary"; query: "Binary/string()"  }
        XmlRole{name: "Schema"; query: "Schema/string()"  }

    }

}
