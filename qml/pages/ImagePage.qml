import QtQuick 2.5
import Sailfish.Silica 1.0

Page {
  id: idPage
  SilicaFlickable {
    anchors.fill: parent
    id: imageFlickable
    contentWidth: idImage.width
    contentHeight: idImage.height

    Image {
      id: idImage
      autoTransform: true
      asynchronous: true
      source: idApp.sImage
      fillMode: Image.PreserveAspectFit
      y: (idPage.height - height) / 2
      width: idPage.width
      PinchArea {
        pinch.dragAxis: Pinch.XAndYAxis
        anchors.fill: parent
        pinch.target: parent
        pinch.minimumScale: 1
        pinch.maximumScale: 8
      }
    }
  }

  Image {
    width: idPage.width
    anchors.fill: parent
    opacity: 0.5
    autoTransform: true
    visible: idPageBusyIndicator.running
    fillMode: Image.PreserveAspectFit
    source: idApp.sImageThumb
  }

  PageBusyIndicator {
    id: idPageBusyIndicator
    anchors.verticalCenter: parent.verticalCenter
    running: idImage.progress != Image.Ready
  }
}
