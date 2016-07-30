import QtQuick 2.0
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
Page {
  id: idPage2


  SilicaListView {

    PullDownMenu {
      MenuItem {
        text: "Delete Selected"
        onClicked: idTrackModel.deleteSelected()
      }
      MenuItem {
        text: "Load Selected"
        onClicked: idTrackModel.loadSelected()
      }
      MenuItem {
        text: "Unload Selected"
        onClicked: idTrackModel.unloadSelected()
      }
    }

    anchors.fill: parent
    /*
    x:100
    width: 480; height: 800
    */
    model: idTrackModel

    delegate: ListItem {
      contentHeight:50
      width: ListView.view.width
      menu: contextMenu
      Row
      {
        TextField   {
          id:idEditText
          width:400
          color:   bSelected ? Theme.highlightColor : Theme.primaryColor
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
          }

          readOnly: true
          font.italic: bSelected

          font.bold: bLoaded

          text: aValue
        }

        Label
        {
          // color: Theme.primaryColor
          y:10
          text: sLength + " km"
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

            text: "Rename"
            onClicked: {
              idEditText.readOnly = false

            }
          }

        }
      }

    }

    VerticalScrollDecorator {}
  }

}


