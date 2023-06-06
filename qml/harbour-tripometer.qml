import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0
import Sailfish.Pickers 1.0
import QtQuick.LocalStorage 2.0 as Sql
import "tripometer-functions.js" as Lib
// import QtQuick.Controls 1.0
import "pages"

// import QtDocGallery 1.0
ApplicationWindow {
  id: idApp
  property var db
  property bool bFlipped: false
  property bool bAppStarted: false
  Component.onDestruction: {
    mainMap.saveTrack(0)
  }

  property var ocSumSize: ["", "", ""]
  property int nPikesCounted: 6
  property int nNrTeams: 2
  property int nMinSize: 60
  property string sDur
  property bool bIsPause: true
  property int nSelectCount
  property bool nSearchBusy
  property bool bScreenallwaysOn: false
  // 0 = km/h 1 kts
  property int nUnit: 1
  property GpsMap mainMap
  // property string sImage
  // property string sImageThumb
  property string sReportPath
  // Total , team 1 , team 2 ..
  property var ocPikeCount: [0, 0, 0, 0]
  property var ocTeamName: []
  CoverBackground {
    id: blueCover

    //CoverBackground  {
    //  anchors.fill: parent
    // color: Qt.rgba(0, 0, 1, 0.4)
    Label {
      anchors.topMargin: 30
      anchors.horizontalCenter: parent.horizontalCenter
      text: "PikeFight"
    }
    Image {
      id: idIcon
      anchors.centerIn: parent
      source: "harbour-pikefight.png"
    }
    Label {
      anchors.top: idIcon.bottom
      anchors.horizontalCenter: parent.horizontalCenter
      text: sDur
    }
  }

  Component.onCompleted: {

    // pageStack.pushAttached(idMapPage)
    // console.log("int db")
    // Lib.initDB()
  }
  FontLoader {
    id: webFont
    source: "qrc:/FORTE.TTF"
  }
  ListModel {
    id: idPikeModel1
  }
  ListModel {
    id: idPikeModel2
  }
  ListModel {
    id: idPikeModel3
  }
  initialPage: Component {
    Page {
      Flipable {
        id: flipable
        anchors.fill: parent

        front: FirstPage {
          height: idApp.height
          width: idApp.width

          LargeBtn {
            id: idFlipBtn
            opacity: 0.5
            visible: idApp.bAppStarted
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20

            onClicked: {
              idApp.bFlipped = true
            }
          }

          Rectangle {
            visible: !idApp.bAppStarted
            id: idStartMsgBox
            width: parent.width - 40
            height: 300
            radius: 5
            anchors.centerIn: parent

            color: Theme.highlightBackgroundColor
            opacity: 0.9

            Button {
              y: 100
              anchors.left: parent.left
              anchors.leftMargin: 20

              width: 200
              text: "New"
              onClicked: {
                Lib.newSession()
              }
            }
            Button {
              y: 100
              anchors.horizontalCenter: parent.horizontalCenter
              width: 200
              text: "View"
              onClicked: {
                onClicked: {
                  Lib.viewSession()
                }
              }
            }

            Button {
              y: 100
              anchors.right: parent.right
              anchors.rightMargin: 20
              width: 200
              text: "Resume"
              onClicked: {
                onClicked: {
                  Lib.resumeSession()
                }
              }
            }
          }
        }
        back: PikeMapPage {
          anchors.fill: parent
          id: idMapPage
        }

        transform: Rotation {
          id: rotation
          origin.x: flipable.width / 2
          origin.y: flipable.height / 2
          axis.x: 0
          axis.y: 1
          axis.z: 0 // set axis.y to 1 to rotate around y-axis
          angle: 0 // the default angle
        }

        states: State {
          name: "back"
          PropertyChanges {
            target: rotation
            angle: 180
          }
          when: idApp.bFlipped
        }

        transitions: Transition {
          NumberAnimation {
            target: rotation
            property: "angle"
            duration: 1000
          }
        }
      }
    }
  }
  cover: blueCover
}
