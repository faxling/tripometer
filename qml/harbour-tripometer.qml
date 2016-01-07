
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
      id:idMap
      Page
      {
          GpsMap
          {
            anchors.fill: parent
          }
      }
  }


  Component.onCompleted:
  {
    pageStack.pushAttached(idMap)
  }

  initialPage: Component {
    FirstPage {

    }
  }
  cover: blueCover
}
