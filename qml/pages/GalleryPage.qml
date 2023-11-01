import QtQuick 2.0
import Sailfish.Silica 1.0
import Nemo.Thumbnailer 1.0
import Sailfish.Share 1.0
import harbour.tripometer 1.0

Page {
  id: idPage

  property alias folderPath: folderModel.folder
  property alias nameFilters: folderModel.nameFilters
  ShareAction {
    id: idShare
    mimeType: "image/*"
    title: "Share Image"
  }

  SilicaGridView {
    id: idGrid
    cellHeight: idPage.width / 3
    cellWidth: idPage.width / 3

    //Math.floor(width / Theme.itemSizeHuge)
    anchors.fill: parent
    FolderListModel {
      id: folderModel
      sortField: FolderListModel.Time
      folder: pikeFightDocFolder
      nameFilters: ["img*.png", "img*.jpg", "map*.jpg", "map*.jpg", "report*.png"]
    }

    delegate: GridItem {
      id: idGridItem

      onReleased: {
        if (_menuItem === null) {
          pageStack.push("ImagePage.qml", {
                           "oImgSrc": filePath
                         })
          idGrid.currentIndex = index
        }
      }

      function remove() {
        remorseDelete(function () {
          oFileMgr.remove(folderModel.get(index, "filePath"))
        })
      }
      function share(filePathIn, indexIn) {
        var oFP = oFileMgr.renameToAscii(filePathIn)
        folderModel.setData(indexIn, "filePath", oFP)
        idShare.resources = [oFP]
        idShare.trigger()
      }
      menu: Component {
        ContextMenu {

          MenuItem {
            text: "Share"
            onClicked: share(filePath, index)
          }
          MenuItem {
            text: "View"

            onClicked: {

              pageStack.push("ImagePage.qml", {
                               "oImgSrc": filePath
                             })
              idGrid.currentIndex = index
            }
          }
          MenuItem {
            text: "Remove"
            onClicked: remove()
          }
        }
      }

      Thumbnail {
        width: idGrid.cellHeight
        height: idGrid.cellHeight
        source: filePath
        sourceSize.width: width
        sourceSize.height: height
      }

      Rectangle {
        visible: index === idGrid.currentIndex
        anchors.fill: parent
        color: "blue"
        opacity: 0.3
      }
    }

    model: folderModel
  }
}
