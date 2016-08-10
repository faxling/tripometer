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
  property bool bScreenallwaysOn : true
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

        id:idMap
        enable_compass : true
        anchors.fill: parent
      }

      Text
      {
        font.pixelSize: 15
        text: sDirname
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 90
        anchors.leftMargin: 10
      }

      Row {
        id: map_controls
        anchors.bottom: parent.bottom
        width: parent.width
        height: Theme.itemSizeMedium
        z: idMap.z + 1
        anchors.bottomMargin: -Theme.paddingMedium
        //  visible: !Qt.inputMethod.visible
        IconButton {
          id: zoomout
          icon.source: "btnMinus.png"
          onClicked: { idMap.zoomOut() }
        }
        IconButton {
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

              idMap.setTrack(o)
            }

          }
        }

        IconButton {
          id: idClearTrack
          icon.source: "btnTracks.png"
          onClicked: {
            idTrackPanel.open =  !idTrackPanel.open
          }
        }

        IconButton {
          id: idBack
          icon.source: idMapPage.backNavigation ? "btnBackDis.png": "btnBack.png"
          onClicked: {
            idMapPage.backNavigation =   !idMapPage.backNavigation
          }
        }
      }

      DockedPanel {
        id: idTrackPanel
        width: parent.width
        height: 700
        dock: Dock.Bottom

        SecondPage
        {
          anchors.fill: parent
          anchors.topMargin: 100
          anchors.bottomMargin: 100
          clip: true
        }
        Row
        {
          id:idButtonRow
          x:30
          y:30
          spacing: 10
          Button {
            width: 110
            text: "Delete"
            color: "black"
            onClicked:
            {
              idTrackModel.unloadSelected()
              idTrackModel.deleteSelected()
            }
          }
          Button {
            color: "black"
            width: 110
            text: "Load"
            onClicked: idTrackModel.loadSelected()
          }
          Button {
            color: "black"
            width: 110
            text: "Unload"
            onClicked: idTrackModel.unloadSelected()
          }
          Button {
            color: "black"
            width: 110
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



    }
  }
  cover: blueCover
}
