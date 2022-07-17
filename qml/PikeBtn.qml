import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0

// import QtQuick.Controls 1.0
Rectangle {
  signal doubleClicked
  signal clicked
  signal swiped
  signal swipedLeft
  radius: 9
  property alias src: idImg.source
  color: idMouseArea.pressed ? "#ff808080" : "#af808080"
  width: 200
  height: 200
  Text {
    id: name
    text: ""
  }

  Image {
    anchors.centerIn: parent
    id: idImg
  }

  MouseArea {
    id: idMouseArea
    anchors.fill: parent
    preventStealing: true
    property real velocity: 0.0
    property int xStart: 0
    property int xPrev: 0
    property bool tracing: false
    onPressed: {
      xStart = mouse.x
      xPrev = mouse.x
      velocity = 0
      tracing = true
    }
    onPositionChanged: {
      if (!tracing)
        return
      var currVel = (mouse.x - xPrev)
      velocity = (velocity + currVel) / 2.0
      xPrev = mouse.x
      if (velocity > 15 || velocity < -15) {
        tracing = false
      }
    }
    onReleased: {
      tracing = false
      if (velocity > 15 || velocity < -15) {
        parent.swiped()
      }
    }
    onClicked: {
      parent.clicked()
    }
    onDoubleClicked: {
      parent.doubleClicked()
    }
  }
}