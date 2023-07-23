import QtQuick 2.5
import Sailfish.Silica 1.0
import Qt.labs.folderlistmodel 2.1
import ".."
import Sailfish.Share 1.0

// ImagePage
Page {
  id: idPage

  property alias model: idView.model

  property int nII

  function setIndex(nI) {
    nII = nI
    idView.positionViewAtIndex(nI, ListView.Center)
  }

  SilicaListView {
    id: idView
    snapMode: ListView.SnapToItem

    anchors.fill: parent
    onCountChanged: {
      idView.positionViewAtIndex(nII, ListView.Center)
    }
    RemorsePopup {
      id: idRemorsePop
    }
    delegate: ListItem {
      property alias imgObj: idImage
      height: idPage.height
      width: idPage.width
      SilicaFlickable {

        id: imageFlickable

        //enabled: false
        //interactive: false
        anchors.fill: parent

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
      idRemorsePop.execute("Deleting Image", function () {
        oFileMgr.remove(idView.model.get(oCur.index, "filePath"))
        nII = oCur.index - 1
      })


      /*
      idRemorse.execute(oCur.item.imgObj, "Deleting Image", function () {
        oFileMgr.remove(idView.model.get(oCur.index, "filePath"))
        nII = oCur.index - 1
      })
      */
      // idRemorse.highlighted = true
    }
  }
}
