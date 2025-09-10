import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0

// TrippProgBtn
// import QtQuick.Controls 1.0
Rectangle {
  signal doubleClicked
  signal clicked
  radius: 9
  property bool bSelected
  border.width: bSelected ? 10 : 0
  border.color: Theme.highlightColor
  property alias src: idImg.source
  property int progress

  Rectangle {
    opacity: 0.5
    height: parent.height
    color: "orange"
    width: (progress / 100.0) * parent.width
  }

  color: idMouseArea.pressed ? "#ff808080" : "#af808080"
  width: Theme.fontSizeMedium * 3
  height: width
  Image {
    anchors.centerIn: parent
    id: idImg
  }

  MouseArea {
    id: idMouseArea
    anchors.fill: parent

    onClicked: {
      parent.clicked()
    }
    onDoubleClicked: {
      parent.doubleClicked()
    }
  }
}
