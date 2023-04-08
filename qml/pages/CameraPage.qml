import QtQuick 2.5
import Sailfish.Silica 1.0
import QtMultimedia 5.6
import harbour.tripometer 1.0
import "../tripometer-functions.js" as Lib

// T
Page {
  id: idPage
  property int indexM
  property var oModel
  property int nImgNo
  property var oListView
  Component {
    id: idImageFactory
    ScreenCapture {
      id: idScreenCapture

      MouseArea {
        anchors.fill: parent
        onClicked: {
          idScreenCapture.save()
        }
      }
    }
  }

  Camera {
    id: idCamera
    // metaData.orientation: 270
    // imageProcessing.whiteBalanceMode: CameraImageProcessing.WhiteBalanceFlash


    /*
    exposure {
      exposureCompensation: -1.0
      exposureMode: Camera.ExposurePortrait
    }
*/
    flash.mode: Camera.FlashOff
    focus {
      focusMode: Camera.FocusAuto
      focusPointMode: Camera.FocusPointCustom
      customFocusPoint: Qt.point(0.5, 0.5)
    }
  }

  function showOverlays(bVisible) {
    idImageGrid.visible = bVisible
    idShotMark.visible = false
    backNavigation = bVisible
  }

  VideoOutput {
    id: idVideo
    source: idCamera
    anchors.fill: parent
    focus: visible // to receive focus and capture key events when visible
    MouseArea {
      anchors.fill: parent
      onClicked: {
        if (idImageGrid.visible) {
          idImageGrid.visible = false
          idShotMark.visible = true
          return
        }
        var o = idImageFactory.createObject(idImageGrid)
        o.width = parent.width / 4
        o.height = parent.height / 4
        showOverlays(false)
        o.capture()
        showOverlays(true)
        o.setPageAndModel(oListView, oModel, indexM)
      }
    }
  }


  /*
  Timer {
    id: idAddTimer
    interval: 500
    repeat: false
    onTriggered: Lib.addPikeImage(indexM, oModel, sPath)
  }
  */
  Grid {
    id: idImageGrid
    visible: false
    columns: 4
  }

  Rectangle {
    id: idShotMark
    anchors.centerIn: parent
    width: Theme.itemSizeLarge
    height: Theme.itemSizeLarge
    color: "transparent"
    border.color: "orange"
    border.width: 3
    radius: width * 0.5
    MouseArea {
      anchors.fill: parent
      onClicked: {
        idCamera.searchAndLock()
      }
    }
    BusyIndPike {
      anchors.centerIn: parent
      visible: running
      running: idCamera.lockStatus == Camera.Searching
    }
  }

}
