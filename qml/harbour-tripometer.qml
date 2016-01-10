
import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.maep.qt 1.0


import "pages"

ApplicationWindow {

  property string sDur
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
      GpsMap
      {
        id:idMap

        anchors.fill: parent
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
          icon.source: "icon-camera-zoom-wide.png"
          onClicked: { idMap.zoomOut() }
        }
        IconButton {
          id: zoomin
          icon.source: "icon-camera-zoom-tele.png"
          onClicked: { idMap.zoomIn() }
        }
        IconButton {
          id: idTrack
          icon.source: idMap.track_capture ? "btnTrackOff.png" : "btnTrack.png"
          onClicked: { idMap.track_capture = !idMap.track_capture }
        }

      }
    }
  }

  Component.onCompleted:
  {
    pageStack.pushAttached(idMapComponent)
  }

  initialPage: Component {
    FirstPage {

    }
  }
  cover: blueCover
}

