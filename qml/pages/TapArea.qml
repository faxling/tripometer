import QtQuick 2.0

MouseArea {
  property double nPressTimeMs
  propagateComposedEvents: true
  signal tap
  onReleased: {
    if ((new Date().getTime() - nPressTimeMs) < 300) {
      tap()
    }
  }
  onPressed: {
    nPressTimeMs = new Date().getTime()
  }
}
