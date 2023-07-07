// PikePageImage.qml
// Maybe PikeList
import QtQuick 2.5
import Sailfish.Silica 1.0
import harbour.tripometer 1.0

Image {
  width: 180
  source: sImageThumb
  asynchronous: true
  // no autoTransform here
  Timer {
    id: idPageTimer2
    interval: 2000
    repeat: false
    onTriggered: idListView.model.get(index).tBusy = false
  }
  Timer {
    id: idPageTimer1
    interval: 100
    repeat: false
    onTriggered: pushImgpage(idListView.model.get(index).sImage, index)
  }
  MouseArea {
    anchors.fill: parent
    onClicked: {
      idListView.model.get(index).tBusy = true
      idPageTimer1.start()
      idPageTimer2.start()
    }
  }

  BusyIndPike {
    running: tBusy
    anchors.centerIn: parent
  }
}
