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


    // imageProcessing.whiteBalanceMode: CameraImageProcessing.WhiteBalanceFlash


    /*
    exposure {
      exposureCompensation: -1.0
      exposureMode: Camera.ExposurePortrait
    }

    viewfinder.resolution.width: 1920
    viewfinder.resolution.height: 1080
*/
    viewfinder.resolution.width: oImageThumb.HEIGHT
    viewfinder.resolution.height: oImageThumb.WIDTH
    flash.mode: Camera.FlashOff

    focus {
      focusMode: Camera.FocusAuto
      focusPointMode: Camera.FocusPointCustom
      customFocusPoint: Qt.point(0.5, 0.5)
    }
  }

  function showOverlays(bVisible) {


    /*
    var oc = idCamera.supportedViewfinderResolutions()
        176x144
        240x320
        320x240
        352x288
        640x360
        640x400
        640x480
        720x480
        800x480
        864x480
        800x600
        1024x738
        1024x768
        720x1280
        1280x720
        1280x768
        1080x1080
        1280x960
        1440x1080
        1600x1200
        1920x1080
        1920x1440




        screen 1080x2520
    for (var nI = 0; nI < oc.length; nI++) {
      console.log(oc[nI].width + "x" + oc[nI].height)
    }
        */
    //console.log(oc.length)
    idImageGrid.visible = bVisible
    idShotMark.visible = false
    backNavigation = bVisible
    idExposure.visible = false
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
          idExposure.visible = true
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
        idCamera.unlock()
        idCamera.searchAndLock()
      }
    }
    BusyIndPike {
      anchors.centerIn: parent
      visible: running
      running: idCamera.lockStatus == Camera.Searching
    }
  }

  Slider {
    id: idExposure
    onValueChanged: idCamera.exposure.exposureCompensation = value / 10
    width: parent.width
    minimumValue: -50
    maximumValue: 50
    value: 0
    stepSize: 1
    valueText: (value / 10)
  }


  /*
  ComboBox {

    visible: false
    width: 480
    label: "Exposure Compensation"
    onCurrentIndexChanged: {
      switch (currentIndex) {
      case 0:
        idCamera.exposure.exposureCompensation = 0
        break
      case 1:
        idCamera.exposure.exposureCompensation = 1
        break
      case 2:
        idCamera.exposure.exposureCompensation = 2
        break
      case 3:
        idCamera.exposure.exposureCompensation = 2
        break
      case 4:
        idCamera.exposure.exposureCompensation = -1
        break
      case 5:
        idCamera.exposure.exposureCompensation = -2
        break
      case 6:
        idCamera.exposure.exposureCompensation = -3
        break
      }
    }

    menu: ContextMenu {
      MenuItem {
        text: "Default"
      }
      MenuItem {
        text: "Some 1"
      }
      MenuItem {
        text: "More 2"
      }
      MenuItem {
        text: "Max 3"
      }
      MenuItem {
        text: "Little -1"
      }
      MenuItem {
        text: "Less -2"
      }
      MenuItem {
        text: "Min -3"
      }
    }
  }


  */
}
