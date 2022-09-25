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


  /*
  Rectangle {
    color: idApp.background.color
    // opacity: Theme.highlightBackgroundOpacity
    height: Theme.itemSizeLarge
    width: parent.width
    Text {
      anchors.horizontalCenter: parent.horizontalCenter
      font.family: Theme.fontFamilyHeading
      font.bold: true
      color: Theme.highlightColor
      font.pixelSize: Theme.fontSizeHuge
      text: idApp.ocTeamName[nOwner - 1]
    }
  }

  */
  PikePage {
    id: idPikePage
    property alias nOwner: idDockedPanel.nOwner
    anchors.topMargin: Theme.itemSizeLarge
    anchors.bottomMargin: idSlider1.height
    anchors.fill: parent
    model: oModel
    onCurrentIndexChanged: {
      if (currentIndex < 0)
        return
      idMap.loadPikeInMap(-1, null, null, null)
      idSlider1.value = oModel.get(currentIndex).nLen / 200.0
      idMap.markPikeInMap(oModel.get(currentIndex).nId)
    }
  }

  StepSlider {
    id: idSlider1
    visible: idPikePage.currentIndex >= 0
    anchors.bottom: parent.bottom
    stepSize: 0.005
    width: parent.width
    onValueChanged: {
      idMap.loadPikeInMap(-1, null, null, null)
      if (idApp.bAppStarted)
        Lib.calcSizeAndDisplay(nOwner, Math.round(value * 200))
      else
        Lib.calcSizeAndDisplay(nOwner)
    }
    value: 0.25
    valueText: "Length:" + Math.round(value * 200)
  }

  Rectangle {
    color: idApp.background.color
    // opacity: Theme.highlightBackgroundOpacity
    height: Theme.itemSizeLarge
    width: parent.width
    Text {
      anchors.horizontalCenter: parent.horizontalCenter
      font.family: Theme.fontFamilyHeading
      font.bold: true
      color: Theme.highlightColor
      font.pixelSize: Theme.fontSizeHuge
      text: idApp.ocTeamName[nOwner - 1]
    }
  }
}
