import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0



SilicaListView
{
  id:idSearchResulView

  signal selection(string place, real lat, real lon)

  delegate: ListItem  {

    width: ListView.view.width

    Label {
      maximumLineCount :2
      wrapMode : Text.WrapAnywhere
      anchors.fill : parent
      font.bold: idSearchResulView.currentIndex === index
      color:  "black"
      text: "[" + model.type + "]" + model.name
    }

    onClicked: {
      idSearchResulView.currentIndex = index
      selection(model.name, model.lat, model.lo)
    }
  }
}

