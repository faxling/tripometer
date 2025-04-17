import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0

SilicaListView {
  id: idSearchResulView

  signal selection(real lat, real lon)
  signal createMarker(string name, real lat, real lon)

  Component {
    id: contextMenu
    ContextMenu {

      MenuItem {
        height: Theme.itemSizeExtraSmall
        text: "Save As Marker"
        onClicked: {
          if (currentItem === undefined)
            return

          createMarker(currentItem.myData.ref, currentItem.myData.lat,
                       currentItem.myData.lo)
        }
      }
    }
  }

  delegate: ListItem {
    property variant myData: model
    width: ListView.view.width

    menu: contextMenu

    Label {
      maximumLineCount: 2
      wrapMode: Text.WrapAnywhere
      anchors.fill: parent
      font.pixelSize: Theme.fontSizeExtraSmall
      font.bold: idSearchResulView.currentIndex === index
      color: "black"
      text: "[" + model.type + "]" + " (" + model.ref + ")" + model.fullName
    }

    onPressed: {
      idSearchResulView.currentIndex = index

      selection(model.lat, model.lo)
    }
  }
}
