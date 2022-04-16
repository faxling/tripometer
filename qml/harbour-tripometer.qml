import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0
import Sailfish.Pickers 1.0
// import QtQuick.Controls 1.0
import "pages"
import QtDocGallery 5.0

ApplicationWindow {

  id: idApp
  property string sDur
  property string sDirname: "track name"
  property bool bIsPause: false
  property bool bScreenallwaysOn: false
  // 0 = km/h 1 kts
  property int nUnit: 0
  property GpsMap mainMap

  CoverBackground {
    id: blueCover

    //CoverBackground  {
    //  anchors.fill: parent
    // color: Qt.rgba(0, 0, 1, 0.4)
    // @disable-check M301
    Label {
      anchors.topMargin: 30
      anchors.horizontalCenter: parent.horizontalCenter
      text: "Tripometer"
    }
    Image {
      id: idIcon
      anchors.centerIn: parent
      source: "harbour-tripometer.png"
    }
    // @disable-check M301
    Label {
      anchors.top: idIcon.bottom
      anchors.horizontalCenter: parent.horizontalCenter
      text: sDur
    }
  }

  Component {

    id: idMapComponent
    // @disable-check M301
    Page {
      id: idMapPage
      backNavigation: false

      GpsMap {
        id: idMap

        Component.onDestruction: {

        }

        onTrackChanged: {

        }
        Component.onCompleted: {
          mainMap = idMap
        }

        enable_compass: true
        anchors.fill: parent
      }

      Column {
        id: map_controls2
        spacing: 20
        anchors.bottom: map_controls.top
        anchors.right: parent.right
        anchors.rightMargin: 20
        z: idMap.z + 1

        TrippBtn {
          id: idSearch
          src: "btnSearch.png"
          onClicked: {
            idSearchPageDockedPanel.open = !idSearchPageDockedPanel.open
          }
        }

        TrippBtn {
          id: idWorld
          src: "btnWorld.png"
          onClicked: {
            idMap.setSource(10)
          }
          onDoubleClicked: {
            idMap.setSource(1)
          }
        }

        TrippBtn {
          id: idSat
          src: "btnSat.png"
          onClicked: {
            idMap.setSource(11)
          }
          onDoubleClicked: {
            idMap.setSource(1)
          }
        }

        TrippBtn {
          id: idAddDbPoint
          src: "btnDB.png"
          onClicked: {
            idMap.addDbPoint()
          }
          onDoubleClicked: {
            idMap.noDbPoint()
          }
        }
      }

      Row {
        id: map_controls
        spacing: 20
        x: 20
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        width: parent.width
        z: idMap.z + 1

        //  visible: !Qt.inputMethod.visible
        TrippBtn {
          id: zoomout
          src: "btnMinus.png"
          onClicked: {
            idMap.zoomOut()
          }
        }
        TrippBtn {
          id: zoomin
          src: "btnPlus.png"
          onClicked: {
            idMap.zoomIn()
          }
        }
        TrippBtn {
          id: idCenter
          src: "btnCenter.png"
          onClicked: {
            idMap.auto_center = !idMap.auto_center
          }
        }
        TrippBtn {
          id: idMarker
          src: "btnMarker.png"
          onClicked: {
            idMap.saveMark(idTrackModel.nextId())
          }
        }
        TrippBtn {
          id: idTrack
          enabled: true
          src: idMap.track_capture ? "btnTrackOff.png" : "btnTrack.png"
          onClicked: {
            idMap.track_capture = !idMap.track_capture

            if (idMap.track_capture === true) {
              var o = Qt.createQmlObject(
                    "import harbour.tripometer 1.0; Track {}",
                    idMapPage, "track1")

              idTrackModel.markAllUnload()
              idMap.setTrack(o)
            }
          }
        }
        TrippBtn {
          id: idClearTrack
          src: "btnTracks.png"
          onClicked: {


            /*
              if (idTrackPanel.open === false)
              idTrackPanel.show();
              else
                  idTrackPanel.hide();*/

            // idTrackPanel.
            idTrackPanel.open = !idTrackPanel.open
          }
        }
        TrippBtn {
          id: idBack
          src: idMapPage.backNavigation ? "btnBackDis.png" : "btnBack.png"
          onClicked: {
            idMapPage.backNavigation = !idMapPage.backNavigation
          }
        }
      }

      DockedPanel {
        height: idMapPage.width + Theme.itemSizeLarge
        width: idMapPage.width + Theme.itemSizeLarge
        id: idSearchPageDockedPanel
        dock: Dock.Right

        SearchPage {
          model: idSearchResultModel
          id: idSearchPage
          anchors.fill: parent
          anchors.leftMargin: Theme.itemSizeLarge + Theme.paddingMedium
          anchors.topMargin: Theme.itemSizeLarge
          onSelection: {
            idMap.setLookAt(lat, lon)
          }
        }
        Rectangle {
          id: idSearchBgRect
          width: parent.width
          height: Theme.itemSizeLarge
          color: Theme.highlightBackgroundColor
        }
        Row {

          id: idSearchRow
          x: Theme.itemSizeLarge
          // @disable-check M301
          TextField {
            id: idSearchText
            color: "black"
            placeholderText: "Enter a place name"
            label: "Place search"
            width: Theme.itemSizeLarge * 4 + Theme.paddingMedium * 3
            height: Theme.itemSizeLarge
            onTextChanged: {
              idSearchPage.currentIndex = -1
              idMap.setSearchRequest(idSearchText.text)
            }


            /*
            EnterKey.text: "search"
            EnterKey.onClicked:
            {
              idSearchPage.currentIndex = -1
              idMap.setSearchRequest(idSearchText.text)
            }
            */
          }
          // @disable-check M301
          Button {
            color: "black"
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.itemSizeLarge
            text: "Clear"
            onClicked: {
              idSearchText.text = ""
            }
          }
        }
      }

      Component {
        id: idDownloadPickerPage
        // @disable-check M301
        DownloadPickerPage {
          title: "Select gpx download"

          _contentModel.contentType: 0
          onSelectedContentPropertiesChanged: {
            idTrackModel.trackImport(selectedContentProperties.filePath)
          }

          _contentModel.contentFilter: GalleryFilterIntersection {
            filters: [
              GalleryStartsWithFilter {
                property: "filePath"
                value: StandardPaths.download
              },
              GalleryWildcardFilter {
                property: "fileName"
                value: "*.gpx"
              }
            ]
          }
        }
      }

      DockedPanel {
        id: idTrackPanel
        width: parent.width
        height: Theme.itemSizeLarge * 5
        dock: Dock.Bottom
        // @disable-check M301
        RemorseItem {
          id: remorse
        }

        SecondPage {
          anchors.fill: parent
          anchors.topMargin: 100
          anchors.bottomMargin: Theme.itemSizeLarge
          clip: true
        }

        Row {
          id: idButtonRow
          x: 30
          y: 10
          spacing: 10
          // @disable-check M301
          Button {
            width: Theme.itemSizeLarge
            text: "Delete"
            color: "black"
            onClicked: {
              var oM = idTrackModel
              remorse.execute(idTrackPanel, "Deleting Selected", function () {
                oM.unloadSelected()
                oM.deleteSelected()
              })
            }
          }
          // @disable-check M301
          Button {
            color: "black"
            width: Theme.itemSizeLarge
            text: "Load"
            onClicked: idTrackModel.loadSelected()
          }
          // @disable-check M301
          Button {
            color: "black"
            width: Theme.itemSizeLarge
            text: "Unload"
            onClicked: idTrackModel.unloadSelected()
          }
          // @disable-check M301
          Button {
            color: "black"
            width: Theme.itemSizeLarge
            text: "Save"
            onClicked: idMap.saveTrack(idTrackModel.nextId())
          }
          // @disable-check M301
          Button {
            color: "black"
            width: Theme.itemSizeLarge
            text: "Gpx"
            onClicked: pageStack.push(idDownloadPickerPage)
          }
        }
      }
    }
  }

  Component.onCompleted: {
    pageStack.pushAttached(idMapComponent)
  }

  initialPage: Component {
    // @disable-check M301
    FirstPage {
      Rectangle {
        id: idStartMsgBox
        width: parent.width - 40
        height: 300
        radius: 5
        anchors.centerIn: parent

        color: Theme.highlightBackgroundColor
        opacity: 0.9
        // @disable-check M301
        Button {
          y: 100
          anchors.left: parent.left
          anchors.leftMargin: 20

          width: 200
          text: "New"
          onClicked: {
            idListModel.klicked2(3)
            idStartMsgBox.visible = false
          }
        }
        // @disable-check M301
        Button {
          y: 100
          anchors.right: parent.right
          anchors.rightMargin: 20
          width: 200
          text: "Resume"
          onClicked: {
            idStartMsgBox.visible = false
          }
        }
      }
    }
  }
  cover: blueCover
}
