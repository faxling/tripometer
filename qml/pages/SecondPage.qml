import QtQuick 2.0
// import QtQuick.Controls 1.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0

//import QtQml 2.2
SilicaListView {

  //   transitions.running
  id: idObjectList
  Component.onCompleted: {
    console.log("maximumFlickVelocity " + maximumFlickVelocity)
  }

  // maximumFlickVelocity: 2000
  // cacheBuffer: 2000
  Component {
    id: idDetailsFactory
    Rectangle {
      y: 40
      x: 20
      property alias sName: idNametext1.text
      property alias sDuration: idNametext2.text
      property alias sLength: idNametext3.text
      property alias sDateTime: idNametext4.text
      property alias sMaxSpeed: idNametext5.text
      property alias sType: idNametext6.text
      property alias sDiskSize: idNametext7.text

      id: idDetails
      radius: 5
      width: idObjectList.width - 40
      height: 215
      color: "steelblue"

      Column {
        y: 10
        x: 20
        Row {
          SmallText {
            text: "Name:"
          }
          SmallText {
            id: idNametext1
          }
        }

        Row {
          SmallText {
            text: "Duration:"
          }
          SmallText {
            id: idNametext2
          }
        }
        Row {
          SmallText {
            text: "Distance:"
          }
          SmallText {
            id: idNametext3
          }
        }
        Row {
          SmallText {
            text: "Date:"
          }
          SmallText {
            id: idNametext4
          }
        }
        Row {
          SmallText {
            text: "Max Speed:"
          }
          SmallText {
            id: idNametext5
          }
        }
        Row {
          SmallText {
            text: "Size:"
          }
          SmallText {
            id: idNametext7
          }
        }
        Row {
          SmallText {
            text: "Type:"
          }
          SmallText {
            id: idNametext6
          }
        }
      }

      Button {
        y: 80
        x: parent.width - 25 - width
        //anchors.right: parent.right
        // anchors.rightMargin: 20
        color: "black"
        width: 110
        text: "OK"
        onClicked: {
          idDetails.destroy()
        }
      }
    }
  }

  // anchors.fill: parent


  /*
    x:100
    */
  width: parent.width
  height: 800

  model: idTrackModelFiltered

  delegate: ListItem {

    contentHeight: Theme.iconSizeSmall + Theme.paddingMedium
    width: ListView.view.width
    menu: contextMenu

    // if the menu opens we like to reset the selection marker
    RemorseItem {
      id: idRemorse
    }
    Row {
      id: idRow

      Rectangle {
        color: bSelected ? "chartreuse" : "transparent"
        // x:2
        y: Theme.paddingMedium
        width: Theme.iconSizeSmall
        height: Theme.iconSizeSmall
        radius: 3
      }

      Text {
        id: idReadText
        font.pixelSize: Theme.fontSizeMedium
        font.bold: bLoaded
        font.italic: nId === nSavedTrackId
        text: aValue
        width: Theme.itemSizeLarge * 3

        // font.italic: bSelected
        MouseArea {
          anchors.fill: parent
          onPressed: idTrackModel.trackToggleSelect(nId)
        }
      }

      TextField {
        id: idEditText
        textLeftMargin: 0
        // textLeftPadding: 0
        width: Theme.itemSizeLarge * 3
        color: "black"
        readOnly: true

        onReadOnlyChanged: {
          visible = !readOnly
          idReadText.visible = readOnly
        }

        text: aValue

        Keys.onPressed: {
          if (event.key === Qt.Key_Return) {
            idTrackModel.trackRename(text, nId)
            mainMap.renameTrack(text, nId)
            idEditText.readOnly = true
          }
        }

        //font.italic: bSelected
      }
      Text {
        id: idText
        color: "black"
        font.pixelSize: Theme.fontSizeMedium
        text: sLength
        MouseArea {
          anchors.fill: parent
          onPressed: {
            mainMap.skipDraw = false
            idTrackModel.trackCenter(nId, mainMap)
          }
        }
      }
      Rectangle {
        // filler space
        color: "transparent"
        height: 10
        width: 10
      }
      Text {
        id: idDur
        color: "black"
        font.pixelSize: Theme.fontSizeMedium
        // @disable-check M325
        text: sDuration === "x" ? "" : sDuration
      }
    }

    Component {
      id: contextMenu
      ContextMenu {
        MenuItem {
          text: "Center and Load"
          height: Theme.itemSizeExtraSmall
          onClicked: {
            mainMap.skipDraw = false
            idTrackModel.trackCenterAndLoad(nId, mainMap)
          }
        }

        MenuItem {
          text: "Rename"
          height: Theme.itemSizeExtraSmall
          onClicked: {
            idEditText.readOnly = false
            idEditText.selectAll()
            idEditText.forceActiveFocus()
          }
        }


        /*
        MenuItem {
          height: Theme.itemSizeExtraSmall
          text: "Delete"
          onClicked: {
            var idx = nId
            var oT = idTrackModel
            var oM = mainMap
            idRemorse.execute(idRow, "Deleting", function () {
              oM.skipDraw = false
              oT.trackDelete(idx)
              oM.unloadTrack(idx)
            })
          }
        }
*/
        MenuItem {
          id: idMenue4
          height: Theme.itemSizeExtraSmall
          text: "Details"
          onClicked: {
            var o = idDetailsFactory.createObject(idObjectList)
            o.sName = aValue
            o.sLength = sLength
            o.sDuration = sDuration
            o.sDateTime = sDateTime
            o.sMaxSpeed = sMaxSpeed
            o.sDiskSize = sDiskSize
            o.sType = sType
          }
        }
      }
    }
  }

  VerticalScrollDecorator {}
}
