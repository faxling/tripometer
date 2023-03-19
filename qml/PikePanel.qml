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

  Text {
    anchors.horizontalCenter: parent.horizontalCenter
    font.family: Theme.fontFamilyHeading
    font.bold: true
    color: "black"
    font.pixelSize: Theme.fontSizeHuge
    text: idApp.ocTeamName[nOwner] + " " + ocSumSize[nOwner - 1]
    MouseArea {
      // strange, this is needed for the docked panel to be enabled:
      // to respond on this area
      propagateComposedEvents: true
      anchors.fill: parent
    }
  }

  PikePage {
    id: idPikePage
    property alias nOwner: idDockedPanel.nOwner
    y: Theme.itemSizeLarge
    // anchors.bottomMargin: idSlider1.height
    width: idDockedPanel.width
    height: idDockedPanel.height - Theme.itemSizeLarge - idSlider1.height
    model: oModel
    onCurrentIndexChanged: {
      if (idApp.bAppStarted === false)
        return
      if (currentIndex < 0) {
        return
      }
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
      if (idApp.bAppStarted)
        Lib.calcSizeAndDisplay(nOwner, Math.round(value * 200))
      else
        Lib.calcSizeAndDisplay(nOwner)
    }
    value: 0.25
    valueText: "Length:" + Math.round(value * 200)
  }
}
