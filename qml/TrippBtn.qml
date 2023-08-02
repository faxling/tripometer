import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0

// import QtQuick.Controls 1.0
Rectangle {
  signal doubleClicked
  signal clicked
  radius: 9
  property bool bSelected
  border.width: bSelected ? 10 : 0
  border.color: Theme.highlightColor
  property alias src: idImg.source
  //  color: idApp.background.color
  //  opacity: idMouseArea.pressed ? 0.9 : 1
  color: idMouseArea.pressed ? "#ff808080" : "#af808080"
  width: 160
  height: 160
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
