import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0
import Sailfish.Pickers 1.0
import QtQuick.LocalStorage 2.0 as Sql
import "tripometer-functions.js" as Lib
import "pages"

Item {
  id: idPikePage

  //// property bool bShowBtns: true
  GpsMap {
    id: idMap
    track_capture: !idApp.bIsPause
    function reCalc() {
      Lib.reCalcSizeAndDisplay()
    }
    Component.onDestruction: {

    }

    onTrackChanged: {

    }

    onTrippleDrag: {

      if (idPikePage.state === "") {
        idPikePage.state = "menuInvisible"
        map_controls3.state = ""
      } else {
        idPikePage.state = ""
      }
    }

    Component.onCompleted: {
      mainMap = idMap
      Lib.initDB()
    }

    enable_compass: true
    anchors.fill: parent
    states: [
      State {
        name: "mapRotate"
        PropertyChanges {
          target: idMap
          rotation: 180
        }
      }
    ]

    transitions: Transition {
      NumberAnimation {
        property: "rotation"
        duration: 500
      }
    }
  }
  PikeBtn {

    id: idPikeBtn1
    src: "symPikeL.png"
    nOwner: 1
    anchors.verticalCenter: parent.verticalCenter
    anchors.left: parent.left
    anchors.leftMargin: 20
  }

  PikeBtn {
    id: idPikeBtn2
    src: "symPike2L.png"
    nOwner: 2
    anchors.verticalCenter: parent.verticalCenter
    anchors.right: parent.right
    anchors.rightMargin: 20
  }

  PikeBtn {
    id: idPikeBtn3
    src: "symPikeL.png"
    nOwner: 3
    anchors.top: idPikeBtn1.bottom
    anchors.topMargin: 20
    anchors.left: parent.left
    anchors.leftMargin: 20
  }

  Column {
    id: map_controls3
    spacing: 20
    opacity: 0
    enabled: false
    anchors.top: map_controls2.top
    anchors.left: map_controls2.right
    anchors.leftMargin: 20

    TrippBtn {
      id: idSat
      bSelected: idMap.source === 11
      src: "btnSat.png"
      onClicked: {
        idMap.setSource(11)
      }
    }

    TrippBtn {
      id: idEniro
      bSelected: idMap.source === 17
      src: "btnSeaMap.png"
      onClicked: {
        idMap.setSource(17)
      }
    }

    TrippBtn {
      id: idWorld
      bSelected: idMap.source === 1
      src: "btnMap.png"
      onClicked: {
        idMap.setSource(1)
      }
    }

    TrippBtn {
      id: idCenterde
      bSelected: idMap.auto_center
      src: "btnCenter.png"
      onClicked: {
        idMap.auto_center = !idMap.auto_center
      }
    }

    states: [
      State {
        name: "menuMapVisible"
        PropertyChanges {
          target: map_controls3
          opacity: 1
        }
        PropertyChanges {
          target: map_controls3
          enabled: true
        }
      }
    ]

    transitions: Transition {
      NumberAnimation {
        property: "opacity"
        duration: 300
      }
    }
  }

  Column {
    id: map_controls2
    spacing: 20
    anchors.bottom: map_controls.top
    anchors.bottomMargin: 20
    anchors.left: parent.left
    anchors.leftMargin: 20
    z: idMap.z + 1
    TrippBtn {
      id: idBtnMap
      src: "btnWorld.png"

      onClicked: {
        if (map_controls3.state === "")
          map_controls3.state = "menuMapVisible"
        else
          map_controls3.state = ""
      }
    }

    TrippBtn {
      id: idSearch
      src: "btnSearch.png"
      onClicked: {
        idSearchPageDockedPanel.open = !idSearchPageDockedPanel.open
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

    TrippBtn {
      id: idMarker
      src: "btnMarker.png"
      onClicked: {
        idMap.saveMark(idTrackModel.nextId())
      }
    }
  }

  states: [
    State {
      name: "menuInvisible"
      PropertyChanges {
        target: map_controls2
        opacity: 0.0
      }
      PropertyChanges {
        target: map_controls2
        enabled: false
      }
      PropertyChanges {
        target: map_controls
        opacity: 0.0
      }
      PropertyChanges {
        target: map_controls
        enabled: false
      }
    },
    State {
      name: "menu2Invisible"
      PropertyChanges {
        target: map_controls2
        opacity: 0.0
      }
      PropertyChanges {
        target: map_controls2
        enabled: false
      }
    }
  ]

  transitions: Transition {
    NumberAnimation {
      property: "opacity"
      duration: 300
    }
  }

  LargeBtn {
    id: idCenterBtn
    visible: !idMap.auto_center
    src: "btnLrgCenter.png"
    anchors.right: parent.right
    anchors.rightMargin: 20
    anchors.bottom: idLeftBtn.top
    anchors.bottomMargin: 20
    onClicked: {
      idMap.centerCurrentGps()
    }
  }

  LargeBtn {
    id: idRotateBtn
    src: idMap.state
         === "" ? "image://theme/icon-m-rotate-right" : "image://theme/icon-m-rotate-left"
    anchors.right: parent.right
    anchors.rightMargin: 20
    anchors.bottom: idCenterBtn.top
    anchors.bottomMargin: 20
    onClicked: {
      if (idMap.state === "")
        idMap.state = "mapRotate"
      else
        idMap.state = ""
    }
  }

  LargeBtn {
    id: idLeftBtn
    anchors.right: parent.right
    anchors.rightMargin: 20
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 20
    onClicked: {
      idApp.bFlipped = false
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
      id: idTrack
      enabled: true
      src: idMap.track_capture ? "btnTrackOff.png" : "btnTrack.png"
      onClicked: {
        idMap.track_capture = !idMap.track_capture

        if (idMap.track_capture === true) {
          var o = Qt.createQmlObject("import harbour.tripometer 1.0; Track {}",
                                     idMapPage, "track1")

          idTrackModel.markAllUnload()
          idMap.setTrack(o)

          idListModel.klicked2(6)
        } else {
          idMap.saveTrack(0)
        }
      }
    }

    TrippBtn {
      src: "btnTracks.png"
      onClicked: {
        idTrackPanel.open = !idTrackPanel.open
        if (idTrackPanel.open)
          idPikePage.state = "menu2Invisible"
        else
          idPikePage.state = ""
      }
    }
  }

  PikePanel {
    id: idPikePanel_1
    dock: Dock.Left
    oModel: idPikeModel1
    nOwner: 1
  }

  PikePanel {
    id: idPikePanel_2
    dock: Dock.Right
    oModel: idPikeModel2
    nOwner: 2
  }

  PikePanel {
    id: idPikePanel_3
    dock: Dock.Left
    oModel: idPikeModel3
    nOwner: 3
  }

  DockedPanel {
    id: idSearchPageDockedPanel
    height: idMapPage.width + Theme.itemSizeLarge
    width: idMapPage.width + Theme.itemSizeLarge

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

      TextField {
        id: idSearchText
        color: "black"
        placeholderText: "Enter a place name"
        label: "Place search"
        width: Theme.itemSizeLarge * 4 + Theme.paddingMedium * 2
        height: Theme.itemSizeLarge


        /*
            EnterKey.text: "search"
            EnterKey.onClicked:
            {
              idSearchPage.currentIndex = -1
              idMap.setSearchRequest(idSearchText.text)
            }
            */
      }
      IconButton {

        anchors.verticalCenter: parent.verticalCenter
        width: Theme.itemSizeSmall
        height: Theme.itemSizeSmall
        icon.source: "image://theme/icon-m-search-on-page?"
                     + (down ? Theme.highlightColor : Theme.primaryColor)
        onClicked: {
          idSearchPage.currentIndex = -1
          idMap.setSearchRequest(idSearchText.text)
          nSearchBusy = true
        }

        BusyIndicator {
          running: nSearchBusy
          size: BusyIndicatorSize.Medium
          anchors.centerIn: parent
        }
      }
      IconButton {

        anchors.verticalCenter: parent.verticalCenter
        width: Theme.itemSizeSmall
        height: Theme.itemSizeSmall
        icon.source: "image://theme/icon-m-cancel?"
                     + (down ? Theme.highlightColor : Theme.primaryColor)
        onClicked: {
          idSearchText.text = ""
        }
      }
    }
  }

  Component {
    id: idDownloadPickerPage
    DownloadPickerPage {
      title: "Select gpx download"
      id: idPicker
      Component.onCompleted: {
        _contentModel.filter = ".gpx"
      }

      _contentModel.contentType: 1

      onSelectedContentPropertiesChanged: {
        idTrackModel.trackImport(selectedContentProperties.filePath)
      }
    }
  }

  DockedPanel {
    id: idTrackPanel
    width: parent.width
    height: Theme.itemSizeLarge * 5
    dock: Dock.Bottom

    RemorseItem {
      id: idDeleteRemorse


    }

    SecondPage {
      id: idSecondPage
      anchors.fill: parent
      anchors.topMargin: 100
      anchors.bottomMargin: Theme.itemSizeLarge
      clip: true
    }

    Row {
      x: (Screen.width - width) / 2
      id: idButtonRow
      y: 10
      spacing: 10

      Button {
        width: Theme.itemSizeLarge
        text: "Delete"
        color: "black"
        onClicked: {

          var oM = idTrackModel
          idDeleteRemorse._labels.children[1].font.pixelSize = Theme.fontSizeHuge
          idDeleteRemorse._labels.children[1].palette.primaryColor = Theme.highlightColor
          idDeleteRemorse.execute(idTrackPanel,
                                  "Deleting  " + nSelectCount + " Item(s)",
                                  function () {
                                    oM.unloadSelected()
                                    oM.deleteSelected()
                                  })
        }
      }

      Button {
        color: "black"
        width: Theme.itemSizeLarge
        text: "Load"
        onClicked: idTrackModel.loadSelected()
      }

      Button {
        color: "black"
        width: Theme.itemSizeLarge
        text: "Unload"
        onClicked: idTrackModel.unloadSelected()
      }

      Button {
        color: "black"
        width: Theme.itemSizeLarge
        text: "GPX"
        onClicked: pageStack.push(idDownloadPickerPage)
      }
      Item {
        height: 1
        width: Theme.itemSizeLarge
      }
    }
  }
}
