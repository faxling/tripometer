import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
  Column {
    id: column
    width: parent.width
    spacing: 20

    PageHeader {
      title: "Settings"
    }
    ComboBox {
      width: parent.width
      label: "Units"
      currentIndex: idApp.nUnit
      onCurrentIndexChanged: {
        console.log("index " + currentIndex)
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
      currentIndex: nPikesCounted - 1
      onCurrentIndexChanged: {
        console.log("nPikesCounted " + currentIndex)
        idApp.nPikesCounted = currentIndex + 1
        if (idApp.nPikesCounted === 7)
          idApp.nPikesCounted = null
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
  }
}
