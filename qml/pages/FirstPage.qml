import QtQuick 2.0
import Sailfish.Silica 1.0
import Sailfish.Share 1.0

//import harbour.maep.qt 1.0
Item {
  id: page
  // SilicaFlickable to get the dropdown menue
  ShareAction {
    id: idShare
    mimeType: "image/jpg"
    title: "Share Pike Report"
  }
  SilicaFlickable {
    // anchors.fill: parent
    height: 120
    width: page.width
    PullDownMenu {

      MenuItem {
        text: bScreenallwaysOn ? "Turn On Powersaver" : "Turn Off Powersaver"
        onClicked: {
          bScreenallwaysOn = !bScreenallwaysOn
        }
      }

      MenuItem {
        text: "Reset Max Speed"
        onClicked: {
          idListModel.klicked2(2)
        }
      }

      MenuItem {
        text: "Help"
        onClicked: {
          pageStack.push("ManPage.qml")
        }
      }

      MenuItem {
        text: "Settings"
        onClicked: {
          pageStack.push("SettingsPage.qml")
        }
      }

      MenuItem {
        text: "Pike Gallery"
        onClicked: {
          pageStack.push("GalleryPage.qml")
        }
      }
      MenuItem {
        text: "Screenshot Gallery"
        onClicked: {
          pageStack.push("GalleryPage.qml", {
                           "folderPath": StandardPaths.pictures + "/Screenshots/",
                           "nameFilters": ["*.png"]
                         })

          console.log(StandardPaths.pictures + "/Screenshots/")
        }
      }

      MenuItem {
        text: "Share Report"
        onClicked: {
          idShare.resources = [idApp.sReportPath]
          idShare.trigger()
        }
      }

      MenuItem {
        text: "Make Report"
        onClicked: {
          mainMap.markPikeInMap(-1)

          if (idApp.nPikesCounted !== 0) {
            idApp.sReportPath
                = mainMap.savePikeReport(idPikeModel1, idApp.ocTeamName[1] + " "
                                         + idApp.ocSumSize[0], idPikeModel2,
                                         idApp.ocTeamName[2] + " "
                                         + idApp.ocSumSize[1], idPikeModel3,
                                         idApp.ocTeamName[3] + " "
                                         + idApp.ocSumSize[2], idApp.nMinSize,
                                         idApp.ocTeamName[0], idApp.nNrTeams)
          } else {
            idApp.sReportPath = mainMap.savePikeReport(idPikeModel1,
                                                       idApp.ocTeamName[1],
                                                       idPikeModel2,
                                                       idApp.ocTeamName[2],
                                                       idPikeModel3,
                                                       idApp.ocTeamName[3], 0,
                                                       idApp.ocTeamName[0],
                                                       idApp.nNrTeams)
          }

          // idApp.ocTeamName[0] The name of the day
          // idApp.sImage = idApp.sReportPath
          // idApp.sImage
          pageStack.push("ImagePage.qml", {
                           "oImgSrc": idApp.sReportPath
                         })
        }
      }
    }
    Row {
      anchors.horizontalCenter: parent.horizontalCenter
      Text {
        font.family: Theme.fontFamilyHeading
        font.bold: true
        color: Theme.highlightColor
        font.pixelSize: Theme.fontSizeHuge
        text: "PikeFight"
      }
    }
  }

  ListView {
    clip: true
    id: listView
    model: idListModel
    anchors.fill: parent
    anchors.topMargin: 120
    delegate: ListItem {
      id: delegate
      height: Theme.itemSizeMedium
      Text {
        font.family: Theme.fontFamilyHeading
        text: aLabel

        //  font.pixelSize:  Theme.fontSizeHuge
        color: Theme.primaryColor
      }
      Text {
        id: idText
        y: 30
        font.family: Theme.fontFamilyHeading
        text: aValue
        font.pixelSize: Theme.fontSizeHuge
        color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
      }

      Text {
        y: 40
        x: idText.width
        font.family: Theme.fontFamily
        text: aUnit
        font.pixelSize: Theme.fontSizeTiny
        color: Theme.primaryColor
      }
      onPressed: {

      }

      onPressAndHold: {

      }
    }
  }
}
