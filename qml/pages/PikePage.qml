import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0

SilicaListView {
  id: idListView
  Component {
    id: highlight
    Rectangle {
      color: Theme.highlightBackgroundColor
      y: idListView.currentItem.y
      Behavior on y {
        SpringAnimation {
          spring: 3
          damping: 0.2
        }
      }
    }
  }

  highlight: highlight
  signal pikePressed(int nId)

  delegate: ListItem {
    id: idListItem

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
