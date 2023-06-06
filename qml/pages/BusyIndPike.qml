import QtQuick 2.2
import Sailfish.Silica 1.0

Image {
  id: idBus
  visible: running
  property alias running: idRotationAnimator.running
  smooth: true
  source: "qrc:/busy.png"
  transformOrigin: Item.Center

  RotationAnimator on rotation {
    id: idRotationAnimator
    from: 0
    to: 360
    duration: 2000
    loops: Animation.Infinite
  }
}
