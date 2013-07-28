import QtQuick 2.0
ListModel {
    ListElement {
        text: "Set Up Wizard"
        loaderSource:"SetupWizard.qml"
    }
     ListElement {
         text: "General"
        loaderSource: "GeneralSettings.qml"
     }
     ListElement {
         text:"Theme Chooser"
        loaderSource: "ThemeChooser.qml"
     }
     ListElement {
         text: "Media Settings"
         loaderSource: "Media.qml"
     }
   }
