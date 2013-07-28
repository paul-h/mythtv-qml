import QtQuick 2.0
import QtQuick.XmlListModel 2.0
import  MythTv 1.0
import "../../../plugins/tvdb/content"

Item {
    id: tvdbRoot
    //    property variant video: null
    property string sourceApi: tvdbModel.source
    width: mediascreen.width
    height: mediascreen.height
    
    
    
    Rectangle{
        id: rootListView
        width: mediascreen.width
        height: mediascreen.height
        ListView{
            id: listview
            width: mediascreen.width
            height: mediascreen.height
            model: tvdbModel
            delegate: Item{
                width: mediascreen.width
                height: mediascreen.height
                anchors.centerIn: parent
                Text {
                    id: markerId
                    text: titles.text
                    color: "transparent"
                }
                MythPictureButton{
                    id: tvdbButton
                    width: mediascreen.width /6
                    height: mediascreen.height /6
                    buttonImage: "../../../artwork/tvdb.jpg"
                    anchors.centerIn: parent
                    Rectangle{
                        id: numberCir
                        radius: 180
                        width: 54
                        height: 54
                        color: "grey"
                        anchors{
                            left: tvdbButton.right
                            top: tvdbButton.top
                            
                        }
                        Text {
                            id: numberOfMatch
                            text: tvdbModel.count
                            color: "black"
                            anchors.centerIn: numberCir
                        }
                        Text {
                            id: markerSId
                            text: seriesid
                            color: "transparent"
                        }
                        Text {
                            id: markerOver
                            text: overview
                            color: "transparent"
                        }
                        Text {
                            id: markerName
                            text: name
                            color: "transparent"
                        }
                        Text {
                            id: markerBanner
                            text: banner
                            color: "transparent"
                        }
                    }
                    onButtonClicked: {
                        col.opacity = 1
                        tvdbModel2.source =encodeURI("http://thetvdb.com/api/GetSeries.php?seriesname=" + markerId.text)
                    }
                }
                
                
            }
        }
    }
    
    
    
    
    
    
    
    
    
    
    Rectangle{
        id: col
        opacity: 0
        width: mediascreen.width
        height: mediascreen.height
        color: "blue"
        ListView{
            model: tvdbModel2
            spacing: 28
            orientation: ListView.Vertical
            flickableDirection: Flickable.VerticalFlick
            
            delegate: Item{
                width: mediascreen.width
                height: mediascreen.height /10
                Column{
                    width: mediascreen.width
                    height: mediascreen.height / 10
                    Rectangle{
                        id: mainBanner
                        color: "green"
                        width: mediascreen.width
                        height: mediascreen.height / 10
                        radius: 8
                        Image {
                            id: bannerImg
                            width: mediascreen.width
                            height: mediascreen.height / 10
                            source:  "http://thetvdb.com/banners/"+banner
                            anchors.centerIn:mainBanner
                        }
                        MouseArea{
                            anchors.fill: bannerImg
                            onClicked: {
                                console.log("clicked")
                                //
                                col.opacity = 0
                                singleSerieModel.source ="http://thetvdb.com/api/040BCD04E3D1E109/series/" + marker2.text.toString() + "/all/en.xml"
                                //         seriesDelRec.opacity = 1
                                
                            }
                            
                        }
                    }
                }
            }
        }
    }
    
    
    
    
    
    //Rectangle{
    // id: seriesDelRec
    // width: mediascreen.width
    // height: mediascreen.height
    // color: "#00000000"
    // opacity: 0
    // ListView{
    //     width: mediascreen.width
    //     height: mediascreen.height
    //     model: singleSerieModel
    //  delegate:   Item{
    //      width: mediascreen.width
    //      height: mediascreen.height
    //      Image {
    //          id: bannerImage
    
    //          width: parent.width
    //          height: parent.height / 10
    //          source: "http://thetvdb.com/banners/" + banner
    //          anchors{
    //              top: parent.top
    //              topMargin: 20
    //              horizontalCenter: parent.horizontalCenter
    //          }
    //      }
    //      //Seasons and Cast
    //      Row{
    //          width: parent.width
    //          height: parent.height
    //          spacing: 1
    //          anchors{
    //          top: bannerImage.bottom
    //          }
    //      //Bottom Picture make into Marquee
    //      Rectangle {
    //          id: bottompictures
    //          visible: fan0.status === Image.Error ? false : true
    //          color: "#33CCCCCC"
    //          width: parent.width /  4; height: parent.width /  2
    //          Image {
    //              id: fan0
    //              source: "http://thetvdb.com/banners/posters/"+ seriesid +"-1.jpg"
    //              anchors.fill: bottompictures
    //          }
    //      }
    
    //      Rectangle {
    //          id: bottompictures1
    //          visible: fan1.status === Image.Error ? false : true
    //          width: parent.width /  4; height: parent.width /  2
    //          color: "#33CCCCCC"
    //          Image {
    //              id: fan1
    //              source: "http://thetvdb.com/banners/posters/"+ seriesid +"-2.jpg"
    //              anchors.fill: bottompictures1
    //          }
    //      }
    
    //      Rectangle {
    //          id: bottompictures2
    //          visible: fan2.status === Image.Error ? false : true
    //          color: "#33CCCCCC"
    //          width: parent.width /  4; height: parent.width / 2
    //          Image {
    //              id: fan2
    //              source: "http://thetvdb.com/banners/posters/"+ seriesid +"-3.jpg"
    //              anchors.fill: bottompictures2
    //          }
    //      }
    //      Rectangle {
    //          id: bottompictures3
    //          visible: fan3.status === Image.Error ? false : true
    //          color: "#33CCCCCC"
    //          width: parent.width / 4; height: parent.width / 2
    //          Image {
    //              id: fan3
    //              source: "http://thetvdb.com/banners/posters/"+ seriesid +"-4.jpg"
    //              anchors.fill: bottompictures3
    //          }
    //      }
    
    //  }
    //      Rectangle {
    //          id: seasoncast
    //          visible: true
    //          color: "#88000000"
    //          width: parent.width / 3 ; height: parent.height / 1.2
    //          radius: 4
    //          border.width: 2
    //          border.color: "#33CCCCCC"
    //          anchors{
    //              top:parent.top
    //              topMargin: parent.height / 8.4
    //              right:parent.right
    //              rightMargin: 20
    //          }
    //          Flickable{
    //              interactive: true
    //              height:  seasoncast.height
    //              width: seasoncast.width
    //              contentHeight: seasoncasttxt.height
    //              contentWidth: seasoncasttxt.width
    //              Rectangle{
    //                  id:seasoncasttxt
    //                  visible: false
    //                  anchors.fill:  seasoncast
    //                  width: seasoncast.width; height: seasoncast.height - 30
    //                  color: "#88000000"
    //  }
    //                  Text {
    //              color: "white"
    //              font.pixelSize: 22
    //              height: seasoncasttxt.height
    //              text: serie.overview
    //              wrapMode: Text.Wrap
    //              anchors{
    //                  fill: seasoncasttxt
    //                  margins: 20
    //              }
    //          }
    //      }
    //  }
    //  }
    //}
    //}
    
    XmlListModel {
        id: tvdbModel
        source:  sourceApi
        query: "//Data/Series"
        XmlRole { name: "name"; query: "SeriesName/string()" }
        XmlRole { name: "seriesid"; query: "seriesid/string()" }
        XmlRole { name: "overview"; query: "Overview/string()" }
        XmlRole { name: "banner"; query: "banner/string()" }
        onCountChanged: {
            console.log(count )
        }
        onStatusChanged: {
            if (status == XmlListModel.Loading) console.log(encodeURI(source) );
            if (status == XmlListModel.Error) console.log("Error: " + errorString);
        }
    }
    XmlListModel {
        id: tvdbModel2
        source:  ""
        query: "/Data/Series[*]"
        XmlRole { name: "banner"; query: "banner/string()" }
        XmlRole { name: "seriesid"; query: "seriesid/string()" }
        
        onCountChanged: {
            console.log(count )
        }
        onSourceChanged: {
            reload()
        }
        onStatusChanged: {
            if (status == XmlListModel.Loading) console.log(encodeURI(source) );
            if (status == XmlListModel.Error) console.log("Error: " + errorString);
        }
    }
    XmlListModel {
        id: singleSerieModel
        //    function load(seriesId) {
        //        console.log("Loading " + seriesId)
        //        source = "http://thetvdb.com/api/040BCD04E3D1E109/series/" + seriesId + "/all/en.xml"
        //    }
        //    property string serieModelSource: serieModel.source
        source: ""//serieModelSource
        query: "//Data/Series"
        XmlRole { name: "seriesid"; query: "id/string()" }
        XmlRole { name: "title"; query: "SeriesName/string()" }
        XmlRole { name: "banner"; query: "banner/string()" }
        XmlRole { name: "fanart"; query: "fanart/string()" }
        XmlRole { name: "poster"; query: "poster/string()" }
        XmlRole { name: "thumb"; query: "thumb[1]/string()"}
        XmlRole { name: "status"; query: "Status/string()" }
        XmlRole { name: "runtime"; query: "Runtime/string()" }
        XmlRole { name: "rating"; query: "Rating/string()" }
        XmlRole { name: "network"; query: "Network/string()" }
        XmlRole { name: "firstaired"; query: "FirstAired/string()" }
    }
    Text {
        id: marker2
        text: seriesid
        color: "transparent"
    }
}

