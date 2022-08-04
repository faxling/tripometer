import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0
import QtQuick.LocalStorage 2.0 as Sql
import "tripometer-functions.js" as Lib

import "pages"

DockedPanel {
  id: idDockedPanel
  height: idApp.width + Theme.itemSizeLarge
  width: idApp.width
  property alias currentIndex: idPikePage.currentIndex
  property int nOwner
  property var oModel
  Rectangle {
    color: Theme.primaryColor
    // opacity: Theme.highlightBackgroundOpacity
    height: idSlider1.height
    width: parent.width
    Text {
      anchors.horizontalCenter: parent.horizontalCenter
      font.family: Theme.fontFamilyHeading
      font.bold: true
      color: Theme.highlightColor
      font.pixelSize: Theme.fontSizeHuge
      text: "Angler " + nOwner
    }
  }

  PikePage {
    id: idPikePage
    property alias nOwner: idDockedPanel.nOwner
    anchors.topMargin: idSlider1.height
    anchors.bottomMargin: idSlider1.height
    anchors.fill: parent
    model: oModel
    onCurrentIndexChanged: {
      if (currentIndex < 0)
        return
      idSlider1.value = oModel.get(currentIndex).nLen
    }
  }

  Slider {
    id: idSlider1
    anchors.bottom: parent.bottom
    width: parent.width
    minimumValue: 40
    maximumValue: 200
    value: 50
    onValueChanged: {
      Lib.calcSizeAndDisplay(nOwner, value)
    }
  }
}
