import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0
import Sailfish.Pickers 1.0
import QtQuick.LocalStorage 2.0 as Sql
import "tripometer-functions.js" as Lib
import "pages"
import ".."

Item {
  id: idPikePage
  property alias bTransitionRunning: idMap.skipDraw
  property int nSavedTrackId: -1
  GpsMap {
    id: idMap
    property bool bIsRotated: idApp.bIsRotated
    property int downloadProgress
    enable_ais: idApp.bEnableAis
    track_capture: !idApp.bIsPause
    function reCalc() {
      Lib.reCalcSizeAndDisplay()
    }

    function scrollToBottom() {
      idSecondPage.positionViewAtEnd()
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
      idApp.mainMap = idMap
      Lib.initDB()
    }

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
    anchors.verticalCenterOffset: -50
    anchors.left: parent.left
    anchors.leftMargin: 20
  }

  PikeBtn {
    id: idPikeBtn2
    src: "symPike2L.png"
    nOwner: 2
    anchors.verticalCenter: parent.verticalCenter
    anchors.verticalCenterOffset: -50
    anchors.right: parent.right
    anchors.rightMargin: 20
  }

  PikeBtn {
    id: idPikeBtn3
    src: "symPikeL.png"
    nOwner: 3
    anchors.bottom: idPikeBtn1.top
    anchors.bottomMargin: 20
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
      bSelected: idMap.source === 10
      src: "btnSat.png"
      onClicked: {
        idMap.setSource(10)
      }
    }

    TrippBtn {
      id: idEniro
      bSelected: idMap.source === 16
      src: "btnSeaMap.png"
      onClicked: {
        idMap.setSource(16)
      }
    }

    TrippBtn {
      id: idWorld
      bSelected: idMap.source === 1
      src: "btnMap.png"
      onClicked: {
        idMap.setSource(1)
      }
      Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        text: "OSM"
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
          enabled: true
        }
        PropertyChanges {
          target: map_controls4
          opacity: 1
          enabled: true
        }
        PropertyChanges {
          target: map_controls5
          opacity: 1
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
    id: map_controls4
    spacing: 20
    opacity: 0
    enabled: false
    anchors.top: map_controls3.top
    anchors.left: map_controls3.right
    anchors.leftMargin: 20

    TrippBtn {
      id: idNavionics1
      bSelected: idMap.source === 18
      src: "btnSeaMap.png"
      onClicked: {
        idMap.setSource(18)
      }
    }
    TrippBtn {
      id: idNavionics2
      bSelected: idMap.source === 19
      src: "btnSeaMap.png"
      onClicked: {
        idMap.setSource(19)
      }
    }

    TrippBtn {
      id: idGoogle
      bSelected: idMap.source === 6
      src: "btnMap.png"
      onClicked: {
        idMap.setSource(6)
      }
      Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        text: "Google"
      }
    }
  }

  ////
  Column {
    id: map_controls5
    enabled :false
    opacity: 0
    spacing: 20
    anchors.bottomMargin: 20
    anchors.leftMargin: 20
    anchors.top: map_controls4.top
    anchors.left: map_controls4.right

    TrippBtn {
      id: idBottom
      src: "btnCompositionOverlay.png"
      onClicked: {
        bSelected = !bSelected
        idMap.enableComposition(bSelected)
      }
    }
    TrippBtn {
      id: idSat2
      bSelected: idMap.source === 21
      src: "btnComposition.png"
      onClicked: {
        idMap.setSource(21)
      }
      Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        text: "Hard"
      }
      Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        text: "Soft"
      }
    }
  }

  ///
  Column {
    id: map_controls2
    spacing: 20
    anchors.bottom: map_controls1.top
    anchors.bottomMargin: 20
    anchors.left: parent.left
    anchors.leftMargin: 20

    //z: idMap.z + 1
    TrippProgBtn {
      id: idBtnMap
      src: "btnWorld.png"
      progress: idMap.numberPendingReq

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
        enabled: false
      }
      PropertyChanges {
        target: map_controls1
        opacity: 0.0
        enabled: false
      }

      PropertyChanges {
        target: idPikeBtn1
        opacity: 0.0
        enabled: false
      }
      PropertyChanges {
        target: idPikeBtn2
        opacity: 0.0
        enabled: false
      }
      PropertyChanges {
        target: idPikeBtn3
        opacity: 0.0
        enabled: false
      }
    },
    State {
      name: "menu2Invisible"
      PropertyChanges {
        target: map_controls2
        opacity: 0.0
        enabled: false
      }
      PropertyChanges {
        target: map_controls3
        state: ""
      }
    }
  ]

  transitions: Transition {
    NumberAnimation {
      property: "opacity"
      duration: 300
    }
  }

  LargeProgBtn {
    id: idDownloadBtn
    visible: idApp.mainMap.enableDownload
    src: "image://theme/icon-m-cloud-download?"
         + (idMap.downloadProgress > 0 ? Theme.highlightColor : Theme.primaryColor)
    anchors.right: parent.right
    anchors.rightMargin: 20
    anchors.bottom: idRotateBtn.top
    anchors.bottomMargin: 20
    progress: idMap.downloadProgress
    onClicked: {
      idMap.downloadMapSquare()
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

    Rectangle {
      radius: 10
      visible: idApp.bAisError
      anchors.centerIn: parent
      width: 20
      height: 20
      color: "red"
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
    id: map_controls1
    spacing: 20
    x: 20
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 20
    width: parent.width
    z: idMap.z + 1

    //  visible: !Qt.inputMethod.visible

    TrippBtn {
      id: zoomin
      src: "btnZoom.png"
      onClicked: {
        bSelected = !bSelected
        idMap.zoomIn(bSelected)
      }
    }

    TrippBtn {
      id: idTrack
      enabled: true

      src: idMap.track_capture ? "btnTrackOff.png" : "btnTrack.png"
      onClicked: {
        idMap.track_capture = !idMap.track_capture

        if (idMap.track_capture === true) {
          idMap.skipDraw = false
          var o = Qt.createQmlObject("import harbour.tripometer 1.0; Track {}",
                                     idMapPage, "track1")

          idMap.unloadTrack(nSavedTrackId)
          idMap.setTrack(o)

          idTrackModel.trackUnloaded(nSavedTrackId)

          // Saves current time
          idListModel.klicked2(6)
        } else {
          nSavedTrackId = idTrackModel.nextId()
          // Uses trackAdd that sets default to loaded in list ()
          // stays in map until new track_capture
          idMap.saveCurrentTrack()
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
      onCreateMarker: {
        idMap.saveSearchMark(idTrackModel.nextId(), name, lon, lat)
      }
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

    onMovingChanged: {
      if (moving)
        idMap.skipDraw = true
      else {
        if (open)
          idMap.skipDraw = true
        else {
          idTrackModel.trackUnselectAll()
          idMap.skipDraw = false
        }
      }
    }
    id: idTrackPanel
    width: parent.width
    height: Theme.itemSizeLarge * 6
    dock: Dock.Bottom

    RemorseItem {
      id: idDeleteRemorse
    }

    SecondPage {
      id: idSecondPage
      anchors.fill: parent
      anchors.topMargin: idBtnDelete.height + Theme.itemSizeLarge
      anchors.bottomMargin: Theme.itemSizeLarge
      clip: true
    }

    Text {
      visible: nSelectCount > 0
      anchors.verticalCenter: idButtonRow.verticalCenter
      anchors.right: idButtonRow.left
      anchors.rightMargin: 10
      font.bold: true
      id: idSelectCount
      text: nSelectCount
    }

    Row {

      id: idButtonRow
      x: (Screen.width - width) / 2

      y: 10
      spacing: 10

      Button {
        id: idBtnDelete
        width: Theme.itemSizeLarge
        text: "Delete"
        color: "black"
        onClicked: {

          var oM = idTrackModel
          var oMap = idMap
          idDeleteRemorse._labels.children[1].font.pixelSize = Theme.fontSizeHuge
          idDeleteRemorse._labels.children[1].palette.primaryColor = Theme.highlightColor
          idDeleteRemorse.execute(idTrackPanel,
                                  "Deleting  " + nSelectCount + " Item(s)",
                                  function () {
                                    oMap.skipDraw = false
                                    oM.unloadSelected(mainMap)
                                    oM.deleteSelected()
                                  })
        }
      }

      Button {
        color: "black"
        width: Theme.itemSizeLarge
        text: "Load"

        onClicked: {
          idMap.skipDraw = false
          idTrackModel.loadSelected(mainMap)
        }
      }

      Button {
        color: "black"
        width: Theme.itemSizeLarge
        text: "Unload"
        onClicked: {
          idMap.skipDraw = false
          idTrackModel.unloadSelected(mainMap)
        }
      }

      Button {
        color: "black"
        width: Theme.itemSizeLarge
        text: "GPX"
        onClicked: pageStack.push(idDownloadPickerPage)
      }


    }

    Row {

      id: idSearchMarkerRow
      //x: Theme.itemSizeLarge
      anchors.top: idButtonRow.bottom

      TextField {
        id: idSearchMarkerText
        color: "black"
        placeholderText: "Marker filter"
        label: "Filter"
        width: Theme.itemSizeLarge * 4 + Theme.paddingMedium * 2
        height: Theme.itemSizeLarge

        onTextChanged: {
          idTrackModelFiltered.setFilter(text)
        }
      }

      IconButton {

        anchors.verticalCenter: parent.verticalCenter
        width: Theme.itemSizeSmall
        height: Theme.itemSizeSmall
        icon.source: "image://theme/icon-m-cancel?"
                     + (down ? Theme.highlightColor : Theme.primaryColor)
        onClicked: {
          idSearchMarkerText.text = ""
        }
      }
    }

    // Search
  }
}
