// PikeBtn
import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0

import "tripometer-functions.js" as Lib

// import QtQuick.Controls 1.0
Rectangle {
  signal clicked
  radius: 9
  property alias src: idImg.source
  color: idMouseArea.pressed ? "#ff808080" : "#af808080"
  width: Theme.fontSizeMedium * 3.5
  height: width

  MouseArea {
    id: idMouseArea
    anchors.fill: parent

    Image {
      id: idImg
      source: "image://theme/icon-m-flip"
      anchors.centerIn: parent
    }

    onClicked: {
      parent.clicked()
    }
  }
}
