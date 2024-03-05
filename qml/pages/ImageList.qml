import QtQuick 2.5
import Sailfish.Silica 1.0
import ".."
import Sailfish.Share 1.0
import "../tripometer-functions.js" as Lib

// ImagePage
Page {
  id: idPage

  property alias model: idView.model

  property int nIndexDeleted: -1
  function setIndex(nI) {
    idView.positionViewAtIndex(nI, ListView.Center)
    console.log("positionViewAtIndex ")
  }

  SilicaListView {
    id: idView
    snapMode: ListView.SnapToItem

    anchors.fill: parent

    onCountChanged: {
      if (nIndexDeleted < 0)
        return

      idView.positionViewAtIndex(nIndexDeleted, ListView.Center)
    }

    RemorsePopup {
      id: idRemorsePop
    }

    delegate: Item {
      height: idPage.height
      width: idPage.width
      SilicaFlickable {
        anchors.fill: parent

        id: imageFlickable
        contentWidth: idImage.width
        contentHeight: idImage.height

        Image {

          id: idImage
          y: (idPage.height - idImage.height) / 2
          source: filePath
          // autoTransform: true
          asynchronous: true
          fillMode: Image.PreserveAspectFit
          width: idPage.width

          PinchArea {

            // pinch.dragAxis: Pinch.NoDrag
            pinch.dragAxis: Pinch.XAndYAxis
            anchors.fill: parent
            pinch.target: parent
            pinch.minimumScale: 1
            pinch.maximumScale: 10

            TapArea {
              anchors.fill: parent
              onTap: {
                idImage.x = 0
                idImage.y = (idPage.height - idImage.height) / 2
                idImage.scale = 1
              }
            }
          }
        }
      }
      Text {
        id: idText
        anchors.bottom: parent.bottom
        anchors.bottomMargin: Theme.fontSizeMedium
        anchors.leftMargin: Theme.fontSizeMedium
        anchors.left: parent.left
        font.family: Theme.fontFamilyHeading
        text: Lib.pikeDateTimeStr(fileModified)
        font.pixelSize: Theme.fontSizeLarge
        color: Theme.primaryColor
      }
    }
  }
  ShareAction {
    id: idShare
    mimeType: "image/jpg"
    title: "Share Pike Report"
  }

  function getCurrent() {
    var nX = idPage.width / 2
    var nY = idView.contentY + idPage.height / 2
    return {
      "index": idView.indexAt(nX, nY),
      "item": idView.itemAt(nX, nY)
    }
  }

  LargeBtn {
    id: idFlipBtn
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 20
    anchors.right: parent.right
    anchors.rightMargin: 20
    src: "image://theme/icon-m-share"
    onClicked: {
      idShare.resources = [idView.model.get(getCurrent().index, "filePath")]
      idShare.trigger()
    }
  }

  LargeBtn {
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 20
    anchors.right: idFlipBtn.left
    anchors.rightMargin: 20
    src: "image://theme/icon-m-delete"
    onClicked: {
      var oCur = getCurrent()
      nIndexDeleted = oCur.index - 1
      idRemorsePop.execute("Deleting Image", function () {
        oFileMgr.remove(idView.model.get(oCur.index, "filePath"))
      })
    }
  }
}
