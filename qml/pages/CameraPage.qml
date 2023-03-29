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
    id: camera
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
      focusMode: Camera.FocusMacro + Camera.FocusContinuous
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
    source: camera
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
        //  idBusy.running = true
        // camera.imageCapture.capture()
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

  BusyIndicator {
    id: idBusy
    size: BusyIndicatorSize.Large
    anchors.centerIn: parent
  }

  IconButton {
    id: idShotMark
    anchors.centerIn: parent
    width: Theme.itemSizeSmall
    height: Theme.itemSizeSmall
    icon.source: "image://theme/icon-m-dot?" + (down ? Theme.highlightColor : Theme.primaryColor)
  }
}
