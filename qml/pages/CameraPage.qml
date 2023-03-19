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

  // width: 640
  // height: 360
  Component {
    id: idImageFactory
    ScreenCapture {
      id: idScreenCapture
      Component.onDestruction: {
        saveImgIfSelected()
      }
      onAddImage: {
        console.log("addimage " + urlImg)
        Lib.addPikeImage(indexM, oModel, urlImg)
      }
      MouseArea {
        anchors.fill: parent
        onClicked: {
          backNavigation = false
          idScreenCapture.save()
          backNavigation = true
        }
      }
      Rectangle {
        width: 10
        height: 10
        color: "green"
      }
    }
  }

  Camera {
    id: camera
    metaData.orientation: 270
    // imageProcessing.whiteBalanceMode: CameraImageProcessing.WhiteBalanceFlash


    /*
    exposure {
      exposureCompensation: -1.0
      exposureMode: Camera.ExposurePortrait
    }
*/
    flash.mode: Camera.FlashOff

    imageCapture {
      onImageSaved: {
        // photoPreview.source = path
        console.log("save " + path)
        sPath = path
        idBusy.running = false
        var o = idImageFactory.createObject(idImageGrid)
        o.width = parent.width / 4
        o.height = parent.height / 4
        o.source = path
      }
      onImageCaptured: {
        // does not work: missing SailfishOS feature
        console.log("preview " + preview)
        photoPreview.source = preview // Show the preview in an Image
      }
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
        o.capture(idVideo)
        showOverlays(true)
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

  Text {
    id: idSavedText
    opacity: 0
    anchors.centerIn: parent
    font.family: Theme.fontFamilyHeading
    font.bold: true
    color: Theme.primaryColor
    font.pixelSize: Theme.fontSizeHuge
    text: "Image Updated"
    Behavior on opacity {
      NumberAnimation {
        duration: 500
        easing.type: Easing.InOutQuad
      }
    }
  }
  BusyIndicator {
    id: idBusy
    size: BusyIndicatorSize.Large
    anchors.centerIn: parent
  }

  Image {
    id: idShotMark
    anchors.centerIn: parent
    source: "image://theme/icon-m-dot"
  }


  /*
  ScreenCapture {
    id: photoPreview
    width: parent.width / 4
    height: parent.height / 4
  }
  */
}
