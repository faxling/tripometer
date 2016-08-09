import QtQuick 2.0
//import QtQuick.Controls 1.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0

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


SilicaListView {

  // anchors.fill: parent
  /*
    x:100
    */
  width: parent.width;
  height: 800

  model: idTrackModel

  delegate: ListItem {
    contentHeight:50
    width: ListView.view.width
    menu: contextMenu
    Rectangle
    {
      color: "chartreuse"
      x:2
      y:21
      width:25
      height:25
      radius:3
      visible: bSelected
    }
    Row
    {

      // x:20

      TextField   {
        id:idEditText
        width:400

        color:  "black"
        onClicked: {
          bSelected = !bSelected
        }
        RemorseItem { id: remorse }

        onTextChanged:{
          if (readOnly===true)
            return
          if (text == aValue)
            return

          idTrackModel.trackRename(text,nId)
          mainMap.renameTrack(text,nId);
        }

        readOnly: true
        //font.italic: bSelected

        font.bold: bLoaded

        text: aValue
      }

      Label
      {
        color: "black"
        y:10
        text: sLength
      }

    }
    Component {
      id: contextMenu
      ContextMenu {
        MenuItem {
          height: 50
          text: "Delete"
          onClicked: {
            var idx = nId
            var oT = idTrackModel
            var oM = mainMap
            remorse.execute(idEditText, "Deleting", function()
            {
              oT.trackDelete(idx)
              oM.unloadTrack(idx)
            }  )

          }
        }
        MenuItem {
          height: 50
          text: "Load/Unload"
          onClicked: {
            if (bLoaded===false)
            {
              idTrackModel.trackLoaded(nId)

              // invocable function
              mainMap.loadTrack(aValue, nId)
            }
            else
            {
              idTrackModel.trackUnloaded(nId)

              // invocable function
              mainMap.unloadTrack(nId)
            }

          }
        }
        MenuItem {
          height: 50
          text: "Rename"
          onClicked: {
            idEditText.readOnly = !idEditText.readOnly
          }
        }
        MenuItem {
          height: 50
          text: "Center"
          onClicked: {
            idTrackModel.trackCenter(nId)
          }
        }
      }
    }
  }

  VerticalScrollDecorator {}
}



