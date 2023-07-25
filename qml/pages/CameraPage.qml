import QtQuick 2.5
import Sailfish.Silica 1.0
import QtMultimedia 5.6
import harbour.tripometer 1.0
import ".."
import "../tripometer-functions.js" as Lib

// T
Page {
  id: idPage
  property int indexM
  property var oModel
  property int nImgNo
  property var oListView
  forwardNavigation: oCaptureThumbMaker.HasSelectedCapture
  Component {
    id: idImageFactory
    ScreenCapture {
      id: idScreenCapture

      MouseArea {
        anchors.fill: parent
        onClicked: {
          idScreenCapture.save()

          if (oCaptureThumbMaker.HasSelectedCapture)

            pageStack.pushAttached("ImagePage.qml", {
                                     "oImgSrc": "image://capturedImage/selected",
                                     "cache": false
                                   })
        }
      }
    }
  }

  Component.onCompleted: {

    var nC = QtMultimedia.availableCameras.length
    for (var i = 0; i < nC; ++i) {
      console.log(QtMultimedia.availableCameras[i].deviceId + " "
                  + QtMultimedia.availableCameras[i].displayName)
    }
  }
  Camera {
    id: idCamera
    viewfinder.resolution.width: oCaptureThumbMaker.HEIGHT
    viewfinder.resolution.height: oCaptureThumbMaker.WIDTH
    flash.mode: Camera.FlashOff
    position: Camera.BackFace
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
    idBackFronBtn.visible = false
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
          idBackFronBtn.visible = true
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
  LargeBtn {
    id: idBackFronBtn
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 20
    anchors.right: parent.right
    anchors.rightMargin: 20
    src: "image://theme/icon-camera-switch"
    onClicked: {
      if (idCamera.position != Camera.FrontFace)
        idCamera.position = Camera.FrontFace
      else
        idCamera.position = Camera.BackFace
    }
  }
  ScreenCapture {
    anchors.centerIn: parent
    id: idScreenCaptureBig

    MouseArea {
      anchors.fill: parent
      onClicked: {
        idScreenCaptureBig.visible = false
      }
    }
  }
}
