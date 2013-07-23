import QtQuick 2.0
import QtQuick.LocalStorage 2.0
import QtQuick.Window 2.0
import  MythTv 1.0
import "common/qtlook.js" as Theming
import "common"
FocusScope{
    width: parent.width
    height: parent.height
    focus: true
    Rectangle{
        id: starthere
        width: parent.width
        height: parent.height
        color: "#00000000"
        BorderImage {
            id: bkgRoot
            source: "background.png"
            width: parent.width
            height: parent.height
            BorderImage {
                id: watermark
                source:"watermark/dvd.png"
                state: "base"
                anchors.fill: bkgRoot
                width: parent.width
                height: parent.height
          onSourceChanged{
               SequentialAnimation{
               NumberAnimation{target: watermark;property: "opacity" ; from: 0;to:0; duration: 200  }
               NumberAnimation{target: watermark; property: "opacity" ; from: .55;to:1; duration: 400; easing.type: Easing.OutQuint }
           }
           }
        }

            PathView {
                anchors.fill: bkgRoot
                width: parent.width
                height: parent.height /*/1.2*/
                focus: true
                Keys.onUpPressed: incrementCurrentIndex()
                Keys.onDownPressed: decrementCurrentIndex()
                model: HomeMenuModel{}
                delegate: menuDel
                path: Path {
                    startX:  bkgRoot.width /  2.8; startY: bkgRoot.height / 2.6
                    PathAttribute { name: "iconOpacity"; value: .1 }
                    PathAttribute { name: "iconScale"; value: .3 }
                    PathQuad { x: bkgRoot.width / 1.8; y: bkgRoot.height /1.02; controlX: parent.width / 2.2; controlY: bkgRoot.height / 2.6}

                    PathAttribute { name: "iconScale"; value: 1}
                    PathAttribute { name: "iconOpacity"; value: 1 }

                    PathQuad { x:bkgRoot.width /  2 ; y: bkgRoot.height  / 1.08; controlX: bkgRoot.width / 2.2; controlY:bkgRoot.height / 1.1}

                    PathAttribute { name: "iconScale"; value: .3 }
                    PathAttribute { name: "iconOpacity"; value: 1 }
                    PathQuad { x:bkgRoot.width / 2.8 ;  y:bkgRoot.height /1.08; controlX: bkgRoot.width / 2.2; controlY:bkgRoot.height /1.1 }

                    PathAttribute { name: "iconScale"; value: .5 }
                    PathAttribute { name: "iconOpacity"; value: .3 }
                    PathQuad { x:bkgRoot.width / 2.6 ;  y:bkgRoot.height /1.06; controlX: bkgRoot.width / 2.2; controlY:bkgRoot.height  /1.07}
                }

                Component{
                    id: menuDel
                    Item{
                        scale: PathView.iconScale
                        width: parent.width
                        height: parent.height /1.2
                        Column{
                            width: parent.width
                            height: parent.height /1.2
                            MythButton{
                                id: mediaLib
                                enabled: true
                                onStateChanged: {
                                    if (state === "hovered"){
                                        buttonColor = Qt.darker(buttonColor,1.2)
                                        scale = 1.2
                                        watermark.source = waterMark
                                    }
                                    if (state === "exited"){
                                     buttonColor = "#44FFFFFF"
                                     scale = 1
                                     watermark.source = waterMark
                                    }
                                }
                                radius: 80
                                height: focus === true ? parent.height / 7.7 : parent.height / 8
                                width: focus === true ? parent.width : parent.width / 1.7
                                name: text
                                buttonColor: "#44FFFFFF"
                                nameColor: "white"
                                nameItalic: false
                                nameBold: false
                                nameEffect: Text.Raised
                                nameEffectColor: "grey"
                                namepxSize:  Math.round(height / 1.7)
                                onButtonClick: {
                                    pageLoader.source =  loaderSource
                                    pageLoader.opacity = 1
                                }

                            }
                        }
                    }
                }
            }
            Text {
                id: time
                color: "grey"
                text: Qt.formatDateTime(new Date(),"ddd MMMM d yyyy , hh:mm ap")
                font.pixelSize:  parent.width / 30
                anchors{
                    bottom:parent.bottom
                    bottomMargin: bkgRoot.height / 20
                    right: parent.right
                    rightMargin: 12
                }
            }
        }
        Loader{
            id: pageLoader
            anchors.fill: parent
            source: ""
            opacity:  0
            Behavior on opacity {
                NumberAnimation {
                    target: pageLoader
                    property: "scale"
                    from:0
                    to:1
                    duration: 1000
                    easing.type: Easing.OutExpo
                }
            }
        }
    }
}
