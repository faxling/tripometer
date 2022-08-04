import QtQuick 2.0
import Sailfish.Silica 1.0

Text {
  id: idText
  signal click
  signal pressAndHold
  color: "black"
  font.pixelSize: Theme.fontSizeLarge
  MouseArea {
    anchors.fill: parent
    onClicked: idText.click()
    onPressAndHold: idText.pressAndHold()
  }
}
