import QtQuick 2.5
import Sailfish.Silica 1.0
import Qt.labs.folderlistmodel 2.1

// ImagePage
Page {
  id: idPage

  property alias model: idView.model

  function setIndex(nI) {
    idView.positionViewAtIndex(nI, ListView.Center)
  }

  SilicaListView {
    id: idView
    snapMode: ListView.SnapToItem

    anchors.fill: parent

    delegate: SilicaFlickable {

      id: imageFlickable
      //enabled: false
      //interactive: false
      height: idPage.height
      width: idPage.width

      contentWidth: idImage.width
      contentHeight: idImage.height

      Image {
        id: idImage
        y: (idPage.height - idImage.height) / 2
        source: filePath
        // autoTransform: true
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        width: idPage.width
        PinchArea {

          // pinch.dragAxis: Pinch.NoDrag
          pinch.dragAxis: Pinch.XAndYAxis
          anchors.fill: parent
          pinch.target: parent
          pinch.minimumScale: 1
          pinch.maximumScale: 10

          TapArea {
            anchors.fill: parent

            onTap: {
              idImage.x = 0
              idImage.y = (idPage.height - idImage.height) / 2
              idImage.scale = 1
            }
          }
        }
      }
    }
  }
}
