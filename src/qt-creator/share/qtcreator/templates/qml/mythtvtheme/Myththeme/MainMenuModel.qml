import QtQuick 2.0
ListModel {
    ListElement {
        text: "Media Libray"
        loaderSource:"Media/MediaLibs.qml"
        //
        //say that we wanted to add WaterMarks to are theme
        //        wartemark: "artwork/watermarks/medialib.png"
    }
    ListElement {
        text: "Manage Recordings"
        loaderSource: "Recordings/MainRecodings.qml"
    }
    ListElement {
        text:"Information Center"
        loaderSource: "InfoCenter/InfoCenter.qml"
    }
    ListElement {
        text: "Optical Disks"
        loaderSource: "Dvd/Dvd.qml"
    }
    ListElement {
        text:"Watch TV"
        loaderSource: "WatchTV/WatchTV.qml"
    }
    ListElement {
        text:"Setup"
        loaderSource:   "SetUp/SetUp.qml"
        
    }
    
    
    //
    // DO NOT alter these next couple of LINES when you or someone else Installs this Theme into there themes it
    // will work just fine. The Path is set to match the same as
    //
    //FIXME have a way of themeing the Weather.  IE make more Plugins.
    //
    ListElement {
        text:"Weather"
        loaderSource: "../../plugins/Weather/Main.qml"
    }
    //end weather rant
    
    
    ListElement {
        text:"Remote"
        loaderSource: "Remote/Remote.qml"
    }
}
