// PikePage.qml
// Maybe PikeList
import QtQuick 2.5
import Sailfish.Silica 1.0
import harbour.tripometer 1.0
import Sailfish.Pickers 1.0
import "../tripometer-functions.js" as Lib

SilicaListView {
  id: idListView

  Component {
    id: idImagePickerPage
    ImagePickerPage {
      onSelectedContentPropertiesChanged: {
        Lib.addPikeImage(idListView.currentIndex, idListView.model,
                         selectedContentProperties.filePath)
      }
    }
  }

  Component {
    id: highlightClass
    Rectangle {
      color: Theme.highlightBackgroundColor
      height: 40
      //visible: !idListItem.menu.active
      // y: idListView.currentItem.y


      /*
      Behavior on y {
        SpringAnimation {
          spring: 3
          damping: 0.2
        }
      }
      */
    }
  }

  highlightResizeDuration: 0

  highlightFollowsCurrentItem: true
  highlight: highlightClass
  signal pikePressed(int nId)
  function pushImgpage(sImagePar1, sImagePar2, nIndex) {
    // R val from model
    if (sImagePar2[0] === '/') {

      // Image has been selected
      // idApp.sImage = sImagePar1
      // idApp.sImageThumb = sImagePar2
      pageStack.push("ImagePage.qml", {
                       "oImgSrc": sImagePar1,
                       "oImgThumbSrc": sImagePar2
                     })
    } else {
      idListView.currentIndex = nIndex
      pageStack.push("CameraPage.qml", {
                       "indexM": nIndex,
                       "oModel": idListView.model
                     })

      //      pageStack.push(idImagePickerPage)
    }
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
        console.log("highligt " + idListView.highlightItem)
        if (idListView.highlightItem != null)
          //   if (idListView.highlightItem instanceof highlightClass)
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
                           "oModel": idListView.model
                         })
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

          // idListView.model.get(index).sDate = Lib.pikeDateTimeStr(o)
        }
      }
    }
    RemorseItem {
      id: idRemorse
    }

    function showRemorseItem() {
      idRemorse.execute(idListItem, "Deleting", function () {
        Lib.removePike(nId, idListView.model, idListView.nOwner)
      })
    }

    // width: ListView.view.width
    Row {
      id: idRow

      // Left side if panel from left
      Image {
        visible: nOwner === 1 || nOwner === 3
        // from model
        source: sImageThumb
        asynchronous: true
        // no autoTransform here
        MouseArea {
          anchors.fill: parent
          onClicked: {
            pushImgpage(sImage, sImageThumb, index)
          }
        }
      }
      TextList {
        width: idListItem.width / 2 + 30
        text: sDate
      }
      TextList {
        width: idListItem.width / 4 - 20
        font.strikeout: nLen < idApp.nMinSize
        // font.italic:
        text: sLength
      }
    }

    Image {
      anchors.right: parent.right
      visible: nOwner === 2
      source: sImageThumb
      asynchronous: true
      // no autoTransform here
      MouseArea {
        anchors.fill: parent
        onClicked: {
          pushImgpage(sImage, sImageThumb, index)
        }
      }
    }


    /*
    Rectangle {
      anchors.fill: idRow
      visible: index === currentIndex
      color: Theme.highlightBackgroundColor
    }

    */
    onClicked: {
      idListView.currentIndex = index
      idListView.pikePressed(nId)
    }
  }
}
