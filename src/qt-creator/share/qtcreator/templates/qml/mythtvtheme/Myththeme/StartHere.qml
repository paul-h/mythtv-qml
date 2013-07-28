import QtQuick 2.0
import QtQuick.Window 2.0
import MythTv 1.0

// We Set are main image up here aka the background
Image {
    id: root
    width: Screen.width
    height: Screen.height
    
    source: "artwork/background.png"
    
    //Here we call to are main menu Model Via ListView
    ListView{
        id: menuView
        width: root.width
        height: root.height
        cacheBuffer: 1000
        spacing: 15
        x: parent.width /  5
        y: parent.width / 5
        model: MainMenuModel{}
        delegate: Item {
            // we could make are own main menu item in a different file.
            id: mainMenuModel;
            height: parent.height /10
            width: parent.width / 1.5
            
            
            MythButton{
                id: mediaLib
                enabled: true
                radius: 80
                height: parent.height//focus === true ? parent.height / 7.7 : parent.height / 8
                width: parent.width //focus === true ? parent.width : parent.width / 1.7
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
                onStateChanged: {
                    if (state === "hovered"){
                        buttonColor = Qt.darker(buttonColor,1.2)
                        scale = 1.2
                        
                        //Say We wanted are theme to have a WaterMark
                        //                     watermark.source = waterMark
                        
                    }
                    if (state === "exited"){
                        buttonColor = "#44FFFFFF"
                        scale = 1
                        
                        //Say We wanted are theme to have a WaterMark
                        //                     watermark.source = waterMark
                        
                    }
                }
            }//MythButton
            
        }//Item
    } // end of the List View
    
    
    // Now We add are Time
    //FIXME this should be on a TImmer
    
    Text {
        id: time
        color: "grey"
        text: Qt.formatDateTime(new Date(),"ddd MMMM d yyyy , hh:mm ap")
        font.pixelSize:  parent.width / 30
        anchors{
            bottom:parent.bottom
            bottomMargin: root.height / 20
            right: parent.right
            rightMargin: 12
        }
    }
    
    // Here is are Loader that will Load the Page from the ListView
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
    
} // Background

