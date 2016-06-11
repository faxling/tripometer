
import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0

import "pages"

ApplicationWindow {

  id:idApp
  property string sDur
  property string sDirname : "pucko"
  property bool bIsPause : false
  property bool bScreenallwaysOn : true
  // 0 = km/h 1 kts
  property int nUnit : 0


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

  Component
  {

    id:idMapComponent
    Page
    {
      id:idMapPage
      backNavigation: false

      Track
      {
        id: idTrack1
       // autosavePeriod: 10
      }

      GpsMap
      {
        Component.onDestruction:
        {
          idTrack1.autosavePeriod = 0

        }

        onTrackChanged:
        {

        }
        track: idTrack1

        id:idMap
        enable_compass : true
        anchors.fill: parent
      }

      Text
      {
        font.pixelSize: 12
        text: sDirname
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 70
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
            if (idMap.track !== null)
              idMap.track.autosavePeriod = 10
            // pageStack.pop()
          }
        }


        IconButton {
          id: idTrack
          icon.source: idMap.track_capture ? "btnTrackOff.png" : "btnTrack.png"
          onClicked: {
            idMap.track_capture = !idMap.track_capture
            //if (idMap.track_capture === true)
            // idMap.setTrack(idTrack1)

          }
        }

        IconButton {
          id: idClearTrack
          icon.source: "btnClearTrack.png"
          onClicked: {
            pageStack.popAttached()()
            //   idMap.setTrack()
          }
        }

        IconButton {
          id: idBack
          icon.source: "btnBack.png"
          onClicked: {
            idMapPage.backNavigation =   !idMapPage.backNavigation
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

      onShowMap: {

      }
    }
  }
  cover: blueCover
}

