import QtQuick 2.0
// import QtQuick.Controls 1.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0
//import QtQml 2.2

SilicaListView {

  id: idObjectList

  Component
  {
    id: idDetailsFactory
    Rectangle
    {
      y:40
      x:20
      property alias sName: idNametext1.text
      property alias sDuration: idNametext2.text
      property alias sLength: idNametext3.text
      property alias sDateTime: idNametext4.text
      property alias sMaxSpeed: idNametext5.text
      property alias sType: idNametext6.text
      property alias sDiskSize: idNametext7.text

      id: idDetails
      radius: 5
      width: idObjectList.width -40
      height:215
      color:"steelblue"

      Column
      {
        y : 10
        x : 20
        Row
        {
          SmallText
          {
            text:"Name:"
          }
          SmallText
          {
            id:idNametext1
          }
        }

        Row
        {
          SmallText
          {
            text: "Duration:"
          }
          SmallText
          {
            id : idNametext2
          }
        }
        Row
        {
          SmallText
          {
            text: "Distance:"
          }
          SmallText
          {
            id : idNametext3
          }
        }
        Row
        {
          SmallText
          {
            text: "Date:"
          }
          SmallText
          {
            id : idNametext4
          }
        }
        Row
        {
          SmallText
          {
            text: "Max Speed:"
          }
          SmallText
          {
            id : idNametext5
          }
        }
        Row
        {
          SmallText
          {
            text: "Size:"
          }
          SmallText
          {
            id : idNametext7
          }
        }
        Row
        {
          SmallText
          {
            text: "Type:"
          }
          SmallText
          {
            id : idNametext6
          }
        }
      }
      Button {
        y:100
        anchors.right: parent.right
        anchors.rightMargin: 20
        color: "black"
        width: 110
        text: "OK"
        onClicked:
        {
          idDetails.destroy()
        }
      }
    }

  }
  // anchors.fill: parent
  /*
    x:100
    */

  width: parent.width;
  height: 800

  model: idTrackModel

  delegate: ListItem {

    contentHeight: Theme.iconSizeSmall + Theme.paddingMedium
    width: ListView.view.width
    menu: contextMenu
    Rectangle
    {

      color: "chartreuse"
     // x:2
      y:Theme.paddingMedium
      width:Theme.iconSizeSmall
      height:Theme.iconSizeSmall
      radius:3
      visible: bSelected
    }
    Row
    {
      id:idRow

      // x:20

      TextField   {
        id:idEditText
        width:Theme.itemSizeLarge*3

        color:  "black"
        onClicked: {
          bSelected = !bSelected
        }
        RemorseItem { id: remorse }

        onTextChanged:{
          if (readOnly===true)
            return
          if (text === aValue)
            return

          idTrackModel.trackRename(text,nId)
          mainMap.renameTrack(text,nId);
        }

        Keys.onPressed: {
          if (event.key === Qt.Key_Return) {
            idEditText.readOnly = true
          }
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
            idEditText.readOnly = !idEditText.readOnly
            idEditText.selectAll()
            idEditText.forceActiveFocus()
          }
        }
        MenuItem {
          text: "Center"
          onClicked: {
            mainMap.loadTrack(aValue, nId)
            idTrackModel.trackCenter(nId)
          }
        }
        MenuItem {
          id:idMenue4
          text: "Details"
          onClicked: {
            var o = idDetailsFactory.createObject(idObjectList)
            o.sName = aValue
            o.sLength = sLength
            o.sDuration = sDuration
            o.sDateTime = sDateTime
            o.sMaxSpeed = sMaxSpeed
            o.sDiskSize = sDiskSize
            o.sType = sLength === "x" ? "point" : "track"
          }
        }
      }
    }
  }

  VerticalScrollDecorator {}
}



