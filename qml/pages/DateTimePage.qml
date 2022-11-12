// DateTimePage.qml
import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

Page {

  property alias hour: idTimePicker.hour
  property alias minute: idTimePicker.minute
  property alias date: idDatePicker.date
  property alias time: idTimePicker.time
  Component.onDestruction: {

  }
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
}
