import QtQuick 2.5
import Sailfish.Silica 1.0

// ImagePage
Page {
  id: idPage
  property alias oImgSrc: idImage.source
  property alias oImgThumbSrc: idThumbImg.source
  property int nSquare: Math.min(width, height) - 10

  SilicaFlickable {
    anchors.fill: parent
    id: imageFlickable
    contentWidth: idImage.width
    contentHeight: idImage.height

    Image {
      id: idImage
      y: (idPage.height - height) / 2
      x: (idPage.width - width) / 2
      autoTransform: true
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
            idImage.y = 0
            idImage.scale = 1
          }
        }
      }
    }
  }

  Image {
    id: idThumbImg
    width: idPage.width
    anchors.fill: parent
    opacity: 0.5
    autoTransform: true
    visible: idPageBusyIndicator.running
    fillMode: Image.PreserveAspectFit
  }

  PageBusyIndicator {
    id: idPageBusyIndicator
    anchors.verticalCenter: parent.verticalCenter
    running: idImage.progress != Image.Ready
  }
}
