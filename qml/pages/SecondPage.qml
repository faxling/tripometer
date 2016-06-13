import QtQuick 2.0
import Sailfish.Silica 1.0
//import harbour.maep.qt 1.0

/*

  idTrackModel
ListModel {
          ListElement { fruit: "jackfruit" }
          ListElement { fruit: "orange" }
          ListElement { fruit: "lemon" }
          ListElement { fruit: "lychee" }
          ListElement { fruit: "apricots" }
      }
*/
Page {
  id: idPage2

  SilicaListView {
    x:100
      width: 480; height: 800
      model: idTrackModel

      delegate: Item {
          width: ListView.view.width
          height: 50

          Label { text: aValue}
      }
  }

}
