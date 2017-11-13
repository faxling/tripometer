import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0
// import QtQuick.Controls 1.0

import "pages"


ApplicationWindow {

  id:idApp
  property string sDur
  property string sDirname : "track name"
  property bool bIsPause : false
  property bool bScreenallwaysOn : false
  // 0 = km/h 1 kts
  property int nUnit : 0
  property GpsMap mainMap

  CoverBackground {
    id: blueCover


    //CoverBackground  {
    //  anchors.fill: parent
    // color: Qt.rgba(0, 0, 1, 0.4)
    Label {
      anchors.topMargin: 30
      anchors.horizontalCenter: parent.horizontalCenter
      text: "Tripometer"
    }
    Image
    {
      id: idIcon
      anchors.centerIn: parent
      source: "harbour-tripometer.png"
    }


    Label {
      anchors.top: idIcon.bottom
      anchors.horizontalCenter: parent.horizontalCenter
      text: sDur
    }

  }

  Component  {

    id:idMapComponent
    Page
    {
      id:idMapPage
      backNavigation: false


      GpsMap
      {
        id:idMap

        Component.onDestruction:
        {

        }

        onTrackChanged:
        {

        }
        Component.onCompleted:
        {
          mainMap = idMap
        }


        enable_compass : true
        anchors.fill: parent
      }

      Column
      {
        id: map_controls2
        anchors.bottom:  map_controls.top
        anchors.right: parent.right
        z: idMap.z + 1

        IconButton {
          id: idSearch
          icon.source: "btnSearch.png"
          onClicked: {
            idSearchPageDockedPanel.open =  !idSearchPageDockedPanel.open
          }
        }

        IconButton {
          id: idWorld
          icon.source: "btnWorld.png"
          onClicked: {
            idMap.setSource(10)
          }
          onDoubleClicked: {
            idMap.setSource(1)
          }
        }

        IconButton {
          id: idSat
          icon.source: "btnSat.png"
          onClicked: {
            idMap.setSource(11)
          }
          onDoubleClicked: {
            idMap.setSource(1)}
        }

        IconButton {
          id: idAddDbPoint
          icon.source: "btnDB.png"
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
        anchors.bottom: parent.bottom
        width: parent.width
        height: Theme.itemSizeMedium
        z: idMap.z + 1
        anchors.bottomMargin: - Theme.paddingMedium
        //  visible: !Qt.inputMethod.visible
        IconButton {
          id: zoomout
          icon.source: "btnMinus.png"
          onClicked: { idMap.zoomOut() }
        }
        IconButton  {
          id: zoomin
          icon.source: "btnPlus.png"
          onClicked: { idMap.zoomIn() }

        }
        IconButton {
          id: idCenter
          icon.source: "btnCenter.png"
          onClicked: { idMap.auto_center = !idMap.auto_center }
        }
        IconButton {
          id: idMarker
          icon.source: "btnMarker.png"
          onClicked: {
            idMap.saveMark(idTrackModel.nextId())
          }
        }
        IconButton {
          id: idTrack
          icon.source: idMap.track_capture ? "btnTrackOff.png" : "btnTrack.png"
          onClicked: {
            idMap.track_capture = !idMap.track_capture

            if (idMap.track_capture === true)
            {
              var o = Qt.createQmlObject("import harbour.tripometer 1.0; Track {}",
                                         idMapPage, "track1");

              idTrackModel.markAllUnload()
              idMap.setTrack(o)

            }

          }
        }
        IconButton {
          id: idClearTrack
          icon.source: "btnTracks.png"
          onClicked: {
            /*
              if (idTrackPanel.open === false)
              idTrackPanel.show();
              else
                  idTrackPanel.hide();*/

            // idTrackPanel.
            idTrackPanel.open =  !idTrackPanel.open
          }
        }
        IconButton {
          id: idBack
          icon.source: idMapPage.backNavigation ? "btnBackDis.png": "btnBack.png"
          onClicked: {
            idMapPage.backNavigation = !idMapPage.backNavigation
          }
        }
      }

      DockedPanel
      {
        height :idMapPage.width + Theme.itemSizeLarge
        width:idMapPage.width + Theme.itemSizeLarge
        id: idSearchPageDockedPanel
        dock: Dock.Right

        SearchPage
        {
          model: idSearchResultModel
          id:idSearchPage
          anchors.fill: parent
          anchors.leftMargin:Theme.itemSizeLarge+Theme.paddingMedium
          anchors.topMargin:Theme.itemSizeLarge
          onSelection:{
            idMap.setLookAt(lat, lon)
          }
        }
        Rectangle
        {
          id:idSearchBgRect
          width:parent.width
          height:Theme.itemSizeLarge
          color:Theme.highlightBackgroundColor
        }
        Row
        {

          id:idSearchRow
          x: Theme.itemSizeLarge
          TextField
          {
            id: idSearchText
            color:"black"
            placeholderText: "Enter a place name"
            label: "Place search"
            width: Theme.itemSizeLarge*4+Theme.paddingMedium*3
            height: Theme.itemSizeLarge

            EnterKey.text: "search"
            EnterKey.onClicked:
            {
              idSearchPage.currentIndex = -1
              idMap.setSearchRequest(idSearchText.text)
            }
          }
          Button {
            color: "black"
            anchors.verticalCenter: parent.verticalCenter
            width: Theme.itemSizeLarge
            text: "Clear"
            onClicked:
            {
              idSearchText.text = ""
            }
          }
        }
      }

      DockedPanel {
        id: idTrackPanel
        width: parent.width
        height: Theme.itemSizeLarge * 5
        dock: Dock.Bottom
        RemorseItem { id: remorse }

        SecondPage
        {
          anchors.fill: parent
          anchors.topMargin: 100
          anchors.bottomMargin: Theme.itemSizeLarge
          clip: true
        }
        Row
        {
          id:idButtonRow
          x:30
          y:10
          spacing: 10
          Button {
            width: Theme.itemSizeLarge
            text: "Delete"
            color: "black"
            onClicked:
            {
              var oM = idTrackModel
              remorse.execute(idTrackPanel, "Deleting Selected", function()
              {
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
            text: "Save"
            onClicked:  idMap.saveTrack(idTrackModel.nextId())
          }
        }
      }
    }
  }

  Component.onCompleted:
  {
    pageStack.pushAttached(idMapComponent)
  }


  initialPage: Component {

    FirstPage  {
      Rectangle
      {
        id:idStartMsgBox
        width: parent.width - 40
        height: 300
        radius: 5
        anchors.centerIn:  parent

        color: Theme.highlightBackgroundColor
        opacity:0.9
        Button {
          y:100
          anchors.left: parent.left
          anchors.leftMargin: 20

          width: 200
          text: "New"
          onClicked:
          {
            idListModel.klicked2(3)
            idStartMsgBox.visible = false
          }
        }
        Button {
          y:100
          anchors.right: parent.right
          anchors.rightMargin: 20
          width: 200
          text: "Resume"
          onClicked:
          {
            idStartMsgBox.visible = false
          }
        }
      }

    }
  }
  cover: blueCover
}

