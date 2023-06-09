import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

Page {

  Component.onDestruction: {
    var t = idApp.ocTeamName
    t[0] = idReportLabel.text
    t[1] = idnOwner1.text
    t[2] = idnOwner2.text
    t[3] = idnOwner3.text
    idApp.ocTeamName = t
  }

  Column {
    id: column
    width: parent.width
    spacing: 20

    PageHeader {
      title: "Settings"
    }

    ComboBox {
      width: parent.width
      label: "Number of Teams"
      currentIndex: nNrTeams - 1
      onCurrentIndexChanged: {
        idApp.nNrTeams = currentIndex + 1
      }
      menu: ContextMenu {
        MenuItem {
          text: "1"
        }
        MenuItem {
          text: "2"
        }
        MenuItem {
          text: "3"
        }
      }
    }

    NameText {
      id: idnOwner1
      nOwner: 1
    }

    NameText {
      id: idnOwner2
      nOwner: 2
    }
    NameText {
      id: idnOwner3
      nOwner: 3
    }
    NameText {
      id: idReportLabel
      nOwner: 0
    }
    ComboBox {
      width: parent.width
      label: "Units"
      currentIndex: idApp.nUnit
      onCurrentIndexChanged: {
        idApp.nUnit = currentIndex
        idListModel.klicked2(5)
      }

      menu: ContextMenu {
        MenuItem {
          text: "Metric"
        }
        MenuItem {
          text: "Nautical"
        }
      }
    }

    ComboBox {
      width: parent.width
      label: "Pikes Counted"
      currentIndex: nPikesCounted
      onCurrentIndexChanged: {
        idApp.nPikesCounted = currentIndex
        if (idApp.nPikesCounted === 7)
          idApp.nPikesCounted = null
      }
      menu: ContextMenu {
        MenuItem {
          text: "Don't count"
        }
        MenuItem {
          text: "1"
        }
        MenuItem {
          text: "2"
        }
        MenuItem {
          text: "3"
        }
        MenuItem {
          text: "4"
        }
        MenuItem {
          text: "5"
        }
        MenuItem {
          text: "6"
        }
        MenuItem {
          text: "All"
        }
      }
    }

    StepSlider {
      id: idSliderInterval
      stepSize: 0.005
      width: parent.width
      value: idApp.nMinSize / 200.0
      onValueChanged: {
        idApp.nMinSize = Math.round(value * 200)
      }
      valueText: "Min Size:" + Math.round(value * 200) + " cm"
    }
  }
}
