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
    id: highlight
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
  highlight: highlight
  signal pikePressed(int nId)

  delegate: ListItem {
    id: idListItem
    menu: ContextMenu {

      onActiveChanged: {
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
      TextList {
        width: idListItem.width / 2
        text: sDate
      }
      TextList {
        width: idListItem.width / 4
        font.strikeout: nLen < idApp.nMinSize
        // font.italic:
        text: sLength
      }

      Image {
        source: sImageThumb
        asynchronous: true
        // no autoTransform here
        MouseArea {
          anchors.fill: parent
          onClicked: {
            idApp.sImage = sImage
            idApp.sImageThumb = sImageThumb
            pageStack.push("ImagePage.qml")
          }
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
