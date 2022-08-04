import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0
import "../tripometer-functions.js" as Lib

SilicaListView {
  id: idListView
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
    }
    RemorseItem {
      id: idRemorse
    }
    function showRemorseItem() {
      var idx = index
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
        text: sLength
      }
    }

    onClicked: {
      idListView.currentIndex = index
      idListView.pikePressed(nId)
    }
  }
}
