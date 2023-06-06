// DateTimePage.qml
import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

Page {

  property var fnDateTimeChanged
  property alias hour: idTimePicker.hour
  property alias minute: idTimePicker.minute
  property alias date: idDatePicker.date
  property alias time: idTimePicker.time

  PageHeader {
    id: idHeader
    title: "Set date/time"
  }

  DatePicker {
    id: idDatePicker
    anchors.top: idHeader.bottom
  }

  TimePicker {
    id: idTimePicker
    anchors.bottom: parent.bottom
    anchors.horizontalCenter: parent.horizontalCenter
    Label {
      anchors.centerIn: parent
      text: idDatePicker.dateText + " " + parent.timeText
    }
  }

  IconButton {
    anchors.bottom: parent.bottom
    anchors.right: parent.right
    width: Theme.itemSizeMedium
    height: Theme.itemSizeMedium
    anchors.rightMargin: Theme.paddingLarge
    anchors.bottomMargin: Theme.paddingLarge
    icon.source: "image://theme/icon-m-accept?" + (down ? Theme.highlightColor : Theme.primaryColor)
    onClicked: {
      var o = new Date(date)
      o.setMinutes(minute)
      o.setHours(hour)
      fnDateTimeChanged(o)
    }
  }
}
