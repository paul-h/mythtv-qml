import QtQuick 2.0
import  QtQuick.XmlListModel 2.0
Item {
    id: videoMeta
    property string sourceMeta: metaData.source
    property variant db: null
    ListView{
        width: parent.width
        height: parent.height
        model: metaData
        delegate:    Item{
            width: parent.width
            height: parent.height
            Image {
                width: parent.width
                height: parent.height
                source: Banner
            }
        }
    }
    XmlListModel{
        id: metaData
        source: sourceMeta      //DataBase.ipAddress()+":"+DataBase.port()+"/Video/LookupVideo?Title="
        query: "/VideoLookupList/VideoLookups/VideoLookup"
        XmlRole{name: "Title";query: "Title/string()"}
        XmlRole{name: "SubTitle";query: "SubTitle/string()"}
        XmlRole{name: "Season";query: "Season/string()"}
        XmlRole{name: "Episode";query: "Episode/string()"}
        XmlRole{name: "Description";query: "Description/string()"}
        XmlRole{name: "Year";query: "Year/string()"}
        XmlRole{name: "Inetref";query: "Inetref/string()"}
        XmlRole{name: "ReleaseDate";query: "ReleaseDate/string()"}
        XmlRole{name: "UserRating";query: "UserRating/string()"}
        XmlRole{name: "Length";query: "Length/string()"}
        XmlRole{name: "Language";query: "Language/string()"}
        XmlRole{name: "Popularity";query: "Popularity/string()"}
        XmlRole{name: "Budget";query: "Budget/string()"}
        XmlRole{name: "Revenue";query: "Revenue/string()"}
        XmlRole{name: "IMDB";query: "IMDB/string()"}
        XmlRole{name: "CoverArt" ; query: "Artwork/ArtworkItem[Type='coverart'][1]/Url/string()"}
        XmlRole{name: "CoverArtThumb";query: "Artwork/ArtworkItem[Type='coverart'][1]/Thumbnail/string()"}
        
        XmlRole{name: "Fanart";query: "Artwork/ArtworkItem[Type='fanart'][1]/Url/string()"}
        XmlRole{name: "FanartThumb";query: "Artwork/ArtworkItem[Type='fanart'][1]/Thumbnail/string()"}
        
        XmlRole{name: "Banner";query: "Artwork/ArtworkItem[Type='banner'][1]/Url/string()"}
        XmlRole{name: "BannerThumb";query: "Artwork/ArtworkItem[Type='banner'][1]/Thumbnail/string()"}
        //    }
        
        
        
        //    XmlListModel{
        //        id: metadata
        //        source: sourceMeta      //DataBase.ipAddress()+":"+DataBase.port()+"/Video/LookupVideo?Title="
        //        query: "/VideoLookupList/VideoLookups/VideoLookup"
        //        //               <xs:element minOccurs="0" type="xs:string" name="Title" nillable="1"/>
        //        //               <xs:element minOccurs="0" type="xs:string" name="Subtitle" nillable="1"/>
        //        //               <xs:element minOccurs="0" type="xs:string" name="Inetref" nillable="1"/>
        //        //               <xs:element minOccurs="0" type="xs:int" name="Season" nillable="1"/>
        //        //               <xs:element minOccurs="0" type="xs:int" name="Episode" nillable="1"/>
        //        //               <xs:element minOccurs="0" type="xs:string" name="GrabberType" nillable="1"/>
        //        //               <xs:element minOccurs="0" type="xs:boolean" name="AllowGeneric" nillable="1"/>
        //        XmlRole{name: "MetaTitle";query: "Title/string()"}
        //        XmlRole{name: "SubTitle";query: "SubTitle/string()"}
        //        XmlRole{name: "Season";query: "Season/string()"}
        //        XmlRole{name: "Episode";query: "Episode/string()"}
        //        XmlRole{name: "Description";query: "Description/string()"}
        //        XmlRole{name: "Year";query: "Year/string()"}
        //        XmlRole{name: "Inetref";query: "Inetref/string()"}
        //        XmlRole{name: "ReleaseDate";query: "ReleaseDate/string()"}
        //        XmlRole{name: "UserRating";query: "UserRating/string()"}
        //        XmlRole{name: "Length";query: "Length/string()"}
        //        XmlRole{name: "Language";query: "Language/string()"}
        //        XmlRole{name: "Popularity";query: "Popularity/string()"}
        //        XmlRole{name: "Budget";query: "Budget/string()"}
        //        XmlRole{name: "Revenue";query: "Revenue/string()"}
        //        XmlRole{name: "IMDB";query: "IMDB/string()"}
        //        XmlRole{name: "CoverArt";query: "Artwork/ArtworkItem/type[@coverart]/Url/string()"}
        //        XmlRole{name: "CoverArtThumb";query: "Artwork/ArtworkItem/type[@coverart]/Thumbnail/string()"}
        //        XmlRole{name: "Fanart";query: "Artwork/ArtworkItem/type[@fanart]/Url/string()"}
        //        XmlRole{name: "FanartThumb";query: "Artwork/ArtworkItem/type[@fanart]/Thumbnail/string()"}
        //        XmlRole{name: "Banner";query: "Artwork/ArtworkItem/type[@banner]/Url/string()"}
        //        XmlRole{name: "BannerThumb";query: "Artwork/ArtworkItem/type[@banner]/Thumbnail/string()"}
        //        onStatusChanged: if (status == XmlListModel.Ready) {
        //                             if (count > 0){
        //                                 videoMeta.db = metadata.get(0)
        //                         }
        ///                             if (status == XmlListModel.Error){
        //                                     console.log("Error: " + errorString)
        //}                             if (status == XmlListModel.Loading){
        //                                 console.log("Loading Banner Image:")
        //}
        
        //    }
    }
}
