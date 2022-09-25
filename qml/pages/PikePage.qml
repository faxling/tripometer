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


      /*
      y: idListView.currentItem.y
      Behavior on y {
        SpringAnimation {
          spring: 3
          damping: 0.2
        }
      }
      */
    }
  }

  highlightFollowsCurrentItem: true
  highlight: highlight
  signal pikePressed(int nId)

  delegate: ListItem {
    id: idListItem
    menu: ContextMenu {
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

      TextList {
        width: idListItem.width / 2
        text: sDate
      }
      TextList {
        width: idListItem.width / 4
        font.italic: nLen < idApp.nMinSize
        text: sLength
      }
      Image {
        source: sImage
        autoTransform: true
        height: idListItem.height
        width: idListItem.height
        MouseArea {
          anchors.fill: parent
          onClicked: {
            idApp.sImage = sImage
            pageStack.push("ImagePage.qml")
          }
        }
      }
    }

    onClicked: {
      idListView.currentIndex = index
      idListView.pikePressed(nId)
    }
  }
}
