import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0
// import QtQuick.Controls 1.0
Rectangle
{
  signal doubleClicked
  signal clicked
  radius: 9
  property alias src : idImg.source
  color:idMouseArea.pressed ? "#8800ffc0" : "#00ffc0"
  width: 100
  height: 100
  Image

  {
    anchors.centerIn: parent
    id:idImg
  }

  MouseArea
  {
    id:idMouseArea
    anchors.fill: parent

    onClicked:
    {
      parent.clicked()
    }
    onDoubleClicked:
    {
       parent.doubleClicked()
    }

  }

}

