import QtQuick 2.0
import Sailfish.Silica 1.0

Row {
  id: idSliderRow
  property alias valueText: idSlider.valueText
  property alias value: idSlider.value
  property alias stepSize: idSlider.stepSize

  IconButton {
    id: idLeftBtn
    property int nCount

    icon.source: "image://theme/icon-m-left?"
                 + (pressed ? Theme.highlightColor : Theme.primaryColor)

    function decrement() {
      var fNyVal = idSlider.value - idSlider.stepSize
      if (fNyVal < 0)
        fNyVal = 0

      nCount++

      if (nCount === 20)
        idDecrementTimer.interval = 10

      idSlider.value = fNyVal
    }
    onClicked: {
      decrement()
    }
    Timer {
      id: idDecrementTimer
      interval: 100
      repeat: true
      onTriggered: idLeftBtn.decrement()
    }

    onDownChanged: {
      if (!down) {
        idDecrementTimer.interval = 100
        idDecrementTimer.stop()
      }
    }

    onPressAndHold: {
      nCount = 0
      idDecrementTimer.start()
    }
  }

  Slider {
    id: idSlider
    leftMargin: 0
    rightMargin: 0
    width: parent.width - idLeftBtn.width * 2
  }

  IconButton {
    id: idRightBtn
    property int nCount

    icon.source: "image://theme/icon-m-right?"
                 + (pressed ? Theme.highlightColor : Theme.primaryColor)

    Timer {
      id: idIncrementTimer
      interval: 100
      repeat: true
      onTriggered: idRightBtn.increment()
    }

    onPressAndHold: {
      nCount = 0
      idIncrementTimer.start()
    }

    onDownChanged: {
      if (!down) {
        idIncrementTimer.interval = 100
        idIncrementTimer.stop()
      }
    }

    function increment() {
      var fNyVal = idSlider.value + idSlider.stepSize
      if (fNyVal > 1)
        fNyVal = 1

      nCount++

      if (nCount === 20)
        idIncrementTimer.interval = 10

      idSlider.value = fNyVal
    }

    onClicked: {
      increment()
    }
  }
}
