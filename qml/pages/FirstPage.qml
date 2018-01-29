import QtQuick 2.0
import Sailfish.Silica 1.0
//import harbour.maep.qt 1.0

Page {
  id: page

  SilicaFlickable {
    // anchors.fill: parent
    height: 120
    width: page.width
    PullDownMenu {

      MenuItem {
        text: "Nautical Units"
        font.bold: nUnit === 1
        onClicked: {
          nUnit = 1;
          idListModel.klicked2(5)
        }
      }
      MenuItem {
        font.bold: nUnit === 0
        text: "Metric Units"
        onClicked: {
          nUnit = 0;
          idListModel.klicked2(5)
        }
      }


      MenuItem {
        text: bScreenallwaysOn ? "Turn On Powersaver" : "Turn Off Powersaver"
        onClicked: {
          bScreenallwaysOn = !bScreenallwaysOn

        }
      }
      MenuItem {
        text: bIsPause ? "Resume" : "Pause"
        onClicked: {
          idListModel.klicked2(1)
        }
      }
      MenuItem {
        text: "Reset Max Speed"
        onClicked: {
          idListModel.klicked2(2)
        }
      }
      MenuItem {
        text: "Reset"
        onClicked: {
          idListModel.klicked2(3)
        }
      }
    }
    Text {
      font.family: Theme.fontFamilyHeading
      font.bold: true
      color: Theme.highlightColor
      font.pixelSize: Theme.fontSizeHuge
      anchors.horizontalCenter: parent.horizontalCenter
      text: "Tripometer"
    }

  }


  ListView {
    clip: true
    id: listView
    model: idListModel
    anchors.fill: parent
    anchors.topMargin: 120

    delegate: ListItem {
      id: delegate
      height: Theme.itemSizeMedium
      Text {
        font.family: Theme.fontFamilyHeading
        text: aLabel

        //  font.pixelSize:  Theme.fontSizeHuge
        color: Theme.primaryColor
      }
      Text {
        id: idText
        y: 30
        font.family: Theme.fontFamilyHeading
        text: aValue
        font.pixelSize: Theme.fontSizeHuge
        color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
      }

      Text {
        y: 40
        x: idText.width
        font.family: Theme.fontFamily
        text: aUnit
        font.pixelSize: Theme.fontSizeTiny
        color: Theme.primaryColor
      }
      onPressed: {

      }

      onPressAndHold: {

      }
    }
  }

}

