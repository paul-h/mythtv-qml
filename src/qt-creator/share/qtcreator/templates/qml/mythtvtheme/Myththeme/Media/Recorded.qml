import QtQuick 2.0
import QtQuick.XmlListModel 2.0
import QtQuick.LocalStorage 2.0
import MythSystemTools 1.0
import MythTv 1.0
Image{
    id: videosRoot
    width: parent.width
    height: parent.height
    source: "../artwork/background.png"
    
    //\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
    //#FIXME  Add previews
    //\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
    
    Image{
        id: gridRec
        width:  videosRoot.width
        height: videosRoot.height
        source: "../background.png"
        Behavior on x {NumberAnimation{duration: 900; easing.type: Easing.OutBack}}
        Behavior on opacity {NumberAnimation{  duration: 900 }  }
        GridView{
            id:gridView
            focus: true
            contentHeight: videosRoot.height
            contentWidth: videosRoot.width
            width: videosRoot.width
            height: videosRoot.height
            cellHeight: videosRoot.height / 3
            cellWidth: videosRoot.width / 4
            
            clip: true
            anchors{
                left: parent.left
                top: parent.top
            }
            model: videoNfo
            delegate: Item {
                id:stateOneItem
                focus: true
                width: videosRoot.width / 4.5
                height: videosRoot.height / 3.5
                //background to the Image
                Rectangle{
                    id:backey
                    width: parent.width / 1.3
                    height:    parent.height
                    color: "#88000000"
                    Image {
                        id:mainImage
                        smooth: false
                        sourceSize.height: backey.height
                        sourceSize.width: backey.width
                        anchors.centerIn: backey
                        fillMode: Image.PreserveAspectFit
                        source: {
                            var u = DataBase.ipAddress()+":"+DataBase.port()+aURL1
                            return   u
                        }
                    }
                }
                MouseArea{
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                    }
                    onDoubleClicked: {
                        launcher.launchScript("mythavtest  " + DataBase.ipAddress()+":"+DataBase.port()+"/Content/GetRecording?ChanId="+chanID+"&StartTime="+rStartTs)
                    }
                    onClicked: {
                        gridRec.source = DataBase.ipAddress()+":"+DataBase.port()+aURL2
                    }
                }
                Text {
                    id: textID
                    text: title
                    color: "white"
                    width: backey.width
                    wrapMode: Text.WordWrap
                    anchors{
                        top: backey.bottom
                        topMargin: 1
                    }
                }
            }
        }//GridView
    }
    XmlListModel{
        id: videoNfo
        source: DataBase.ipAddress()+":"+DataBase.port()+"/Dvr/GetRecordedList?StartIndex=0&Descending=true&Details=true"
        query: "/ProgramList/Programs/Program"
        XmlRole{name: "startTime" ; query: "StartTime/string()"}
        XmlRole{name: "endTime" ; query: "EndTime/string()"}
        XmlRole{name: "title" ; query: "Title/string()"}
        XmlRole{name: "subs" ; query: "SubTitle/string()"}
        XmlRole{name: "category" ; query: "Category/string()"}
        XmlRole{name: "catType" ; query: "CatType/string()"}
        XmlRole{name: "repeat" ; query: "Repeat/string()"}
        XmlRole{name: "vidProps" ; query: "VideosProps/string()"}
        XmlRole{name: "fileName" ; query: "FileName/string()"}
        XmlRole{name: "Description" ; query: "Description/string()"}
        XmlRole{name: "Inetref" ; query: "Inetref/string()"}
        XmlRole{name: "Season" ; query: "Season/string()"}
        XmlRole{name: "Episode" ; query: "Episode/string()"}
        XmlRole{name: "chanID" ; query: "Channel/ChanId/string()"}
        XmlRole{name: "CallSign" ; query: "Channel/CallSign/string()"}
        XmlRole{name: "rStatus" ; query: "Recording/Status/string()"}
        XmlRole{name: "rPriority" ; query: "Recording/Priority/string()"}
        XmlRole{name: "rStartTs" ; query: "Recording/StartTs/string()"}
        XmlRole{name: "rEndTs" ; query: "Recording/EndTs/string()"}
        XmlRole{name: "rRecordId " ; query: "Recording/RecordId /string()"}
        XmlRole{name: "rRecGroup" ; query: "Recording/RecGroup/string()"}
        XmlRole{name: "rPlayGroup" ; query: "Recording/PlayGroup/string()"}
        XmlRole{name: "rStorageGroup" ; query: "Recording/StorageGroup/string()"}
        XmlRole{name: " rRecType" ; query: "Recording/RecType/string()"}
        XmlRole{name: "rProfile" ; query: "Recording/Profile/string()"}
        //coverart
        XmlRole{name: "aURL1" ; query: "Artwork/ArtworkInfos/ArtworkInfo[1]/URL/string()"}
        XmlRole{name: "aFileName1" ; query: "Artwork/ArtworkInfos/ArtworkInfo[1]/FileName/string()"}
        XmlRole{name: "aStorageGroup1" ; query: "Artwork/ArtworkInfos/ArtworkInfo[1]/StorageGroup/string()"}
        XmlRole{name: "aType1" ; query: "Artwork/ArtworkInfos/ArtworkInfo[1]/Type/string()"}
        //fanart
        XmlRole{name: "aURL2" ; query: "Artwork/ArtworkInfos/ArtworkInfo[2]/URL/string()"}
        XmlRole{name: "aFileName2" ; query: "Artwork/ArtworkInfos/ArtworkInfo[2]/FileName/string()"}
        XmlRole{name: "aStorageGroup2" ; query: "Artwork/ArtworkInfos/ArtworkInfo[2]/StorageGroup/string()"}
        XmlRole{name: "aType2" ; query: "Artwork/ArtworkInfos/ArtworkInfo[2]/Type/string()"}
        onStatusChanged: {
            //debug
            //            if (status == XmlListModel.Ready){console.log(videoNfo.source.toString())}
            //            if (status === XmlListModel.Loading) console.log("Loading");
            if (status === XmlListModel.Error) console.log("Error: " + errorString + "\n \n \n " + videoNfo.source.toString());
        }
    }
    
    XmlListModel{
        id: videoSingleNfo
        source: "" // DataBase.ipAddress()+":"+DataBase.port()+"/Dvr/GetRecordedList?StartIndex=0&Descending=true&Details=true"
        query: "/ProgramList/Programs/Program"
        XmlRole{name: "startTime" ; query: "StartTime/string()"}
        XmlRole{name: "endTime" ; query: "EndTime/string()"}
        XmlRole{name: "title" ; query: "Title/string()"}
        XmlRole{name: "subs" ; query: "SubTitle/string()"}
        XmlRole{name: "category" ; query: "Category/string()"}
        XmlRole{name: "catType" ; query: "CatType/string()"}
        XmlRole{name: "repeat" ; query: "Repeat/string()"}
        XmlRole{name: "vidProps" ; query: "VideosProps/string()"}
        XmlRole{name: "fileName" ; query: "FileName/string()"}
        XmlRole{name: "Description" ; query: "Description/string()"}
        XmlRole{name: "Inetref" ; query: "Inetref/string()"}
        XmlRole{name: "Season" ; query: "Season/string()"}
        XmlRole{name: "Episode" ; query: "Episode/string()"}
        XmlRole{name: "chanID" ; query: "Channel/ChanId/string()"}
        XmlRole{name: "CallSign" ; query: "Channel/CallSign/string()"}
        XmlRole{name: "rStatus" ; query: "Recording/Status/string()"}
        XmlRole{name: "rPriority" ; query: "Recording/Priority/string()"}
        XmlRole{name: "rStartTs" ; query: "Recording/StartTs/string()"}
        XmlRole{name: "rEndTs" ; query: "Recording/EndTs/string()"}
        XmlRole{name: "rRecordId " ; query: "Recording/RecordId /string()"}
        XmlRole{name: "rRecGroup" ; query: "Recording/RecGroup/string()"}
        XmlRole{name: "rPlayGroup" ; query: "Recording/PlayGroup/string()"}
        XmlRole{name: "rStorageGroup" ; query: "Recording/StorageGroup/string()"}
        XmlRole{name: " rRecType" ; query: "Recording/RecType/string()"}
        XmlRole{name: "rProfile" ; query: "Recording/Profile/string()"}
        XmlRole{name: "aURL" ; query: "Artwork/ArtworkInfos/ArtworkInfo/URL/string()"}
        XmlRole{name: "aFileName" ; query: "Artwork/ArtworkInfos/ArtworkInfo/FileName/string()"}
        XmlRole{name: "aStorageGroup" ; query: "Artwork/ArtworkInfos/ArtworkInfo/StorageGroup/string()"}
        XmlRole{name: "aType" ; query: "Artwork/ArtworkInfos/ArtworkInfo/Type/string()"}
        onStatusChanged: {
            if (status === XmlListModel.Error) console.log("Error: " + errorString + "\n \n \n " + videoNfo.source.toString());
        }
    }
    Player{
        y: videosRoot.height * 2
        id: internalPlayer
        mainBackgroundColor: "#88000000"
        width:  videosRoot.width
        height: videosRoot.height
        opacity: 0
        videoAutoLoad: "false"
        videoAutoPlay: "true"
        videoSource: ""
        onVideoStatusChanged: {
            if (videoStatus === "7"){
                gridRec.opacity = 1
                y = videosRoot.height * 2
                videoAutoPlay = "false"
                opacity = 0
                videoSource: ""
                
            }else{
                //                console.log(videoStatus +"\n" + state)
            }
        }
    }
    ScriptLauncher{
        id:launcher
    }
}//videosRoot
