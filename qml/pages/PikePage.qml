// PikePage.qml
// Maybe PikeList
import QtQuick 2.5
import Sailfish.Silica 1.0
import harbour.tripometer 1.0
import Sailfish.Pickers 1.0
import "../tripometer-functions.js" as Lib
import Sailfish.Share 1.0

SilicaListView {
  id: idListView

  ShareAction {
    id: idShare
    mimeType: "text/plain"
    title: "Share Postion"
  }

  Component {
    id: idImagePickerPage
    ImagePickerPage {
      onSelectedContentPropertiesChanged: {
        Lib.addPikeImageCopy(idListView.currentIndex, idListView.model,
                             selectedContentProperties.filePath)
      }
    }
  }

  Component {
    id: highlightClass
    Rectangle {
      color: Theme.highlightBackgroundColor
      height: 40
    }
  }

  highlightResizeDuration: 0

  highlightFollowsCurrentItem: true
  highlight: highlightClass
  signal pikePressed(int nId)

  function hasImg(sImageFile) {
    if (sImageFile === undefined)
      return -1

    if (sImageFile[0] !== '/')
      return -1

    var nI = folderModel.indexOf("file://" + sImageFile)
    if (nI < 0)
      return -1

    return nI
  }

  function pushImgpage(sImageFile, nIndex) {
    // R val from model
    var nI = hasImg(sImageFile)
    if (nI >= 0) {
      var oImageList = pageStack.push("ImageList.qml", {
                                        "model": folderModel
                                      })

      oImageList.setIndex(nI)
    } else {
      idListView.currentIndex = nIndex
      pageStack.push("CameraPage.qml", {
                       "indexM": nIndex,
                       "oModel": oModel,
                       "oListView": idListView
                     })
    }
  }

  function addImageStart(indexM) {
    idListView.model.get(indexM).tBusy = true
  }

  function addImageStop(indexM) {
    idListView.model.get(indexM).tBusy = false
  }

  function sharePos(fLo, fLa) {
    idShare.resources = Lib.makeSharePosObj(fLo, fLa)
    idShare.trigger()
  }

  function addImageGo(indexM, oModel, urlImg, nOrientaion) {
    Lib.addPikeImage(indexM, oModel, urlImg, nOrientaion)
    idListView.model.get(indexM).tBusy = false
  }

  delegate: ListItem {
    id: idListItem

    function showRemorseItem() {
      idRemorse.execute(idListItem, "Deleting", function () {
        Lib.removePike(nId, idListView.model, idListView.nOwner)
      })
    }
    menu: ContextMenu {

      onActiveChanged: {
        if (idListView.highlightItem != null)
          idListView.highlightItem.visible = !active
      }

      MenuItem {
        id: idMenuItem
        text: "Delete"
        onClicked: {
          idListItem.showRemorseItem()
        }
      }
      MenuItem {
        text: "Center"
        onClicked: {
          idListView.currentIndex = index
          Lib.centerPike(nId, idListView.model)
        }
      }
      MenuItem {
        text: "Add/Replace Image"
        onClicked: {
          idListView.currentIndex = index
          pageStack.push(idImagePickerPage)
        }
      }

      MenuItem {
        text: "Camera"
        onClicked: {
          idListView.currentIndex = index
          pageStack.push("CameraPage.qml", {
                           "indexM": index,
                           "oModel": idListView.model,
                           "oListView": idListView
                         })
        }
      }

      MenuItem {
        text: "Share Pos"
        onClicked: {
          sharePos(fLo, fLa)
        }
      }

      MenuItem {
        text: "Set Date/Time"
        onClicked: {
          idListView.currentIndex = index
          var oDate = Lib.pikeDateFromStr(idListView.model.get(index).sDate)

          var dialog = pageStack.push("DateTimePage.qml", {
                                        "hour": oDate.getHours(),
                                        "minute": oDate.getMinutes(),
                                        "date": oDate
                                      })

          dialog.fnDateTimeChanged = function (date) {
            Lib.updateDateTime(date, index)
          }
        }
      }
    }
    RemorseItem {
      id: idRemorse
    }

    // width: ListView.view.width
    Row {
      id: idRow

      // Left side if panel from left
      PikePageImage {
        visible: nOwner === 1 || nOwner === 3
      }

      TextList {
        width: idListItem.width / 2 + 30
        text: sDate
      }
      TextList {
        visible: idApp.nPikesCounted !== 0
        width: idListItem.width / 4 - 20
        font.strikeout: nLen < idApp.nMinSize
        // font.italic:
        text: sLength
      }
    }

    PikePageImage {
      anchors.right: parent.right
      visible: nOwner === 2
    }

    onClicked: {
      idListView.currentIndex = index
      idListView.pikePressed(nId)
    }
  }
}
