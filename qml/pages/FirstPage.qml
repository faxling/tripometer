import QtQuick 2.0
import Sailfish.Silica 1.0
import harbour.tripometer 1.0


Page {
    id: page
    InfoListModel {
        id: idListModel
    }

    SilicaFlickable {
        // anchors.fill: parent
        height: 120
        width: page.width
        PullDownMenu {
            MenuItem {
                text: "Exit"
                onClicked: {
                    Qt.quit()
                }
            }
            MenuItem {
                text: "Reset Max Speed"
                onClicked: {
                    idListModel.klicked2(2)
                }
            }
            MenuItem {
                text: "Screen On"
                onClicked: {
                    idListModel.klicked2(4)
                }
            }
            MenuItem {
                text: "Reset"
                onClicked: {
                    idListModel.klicked2(3)
                }
            }

            MenuItem {
                text: bIsPause ? "Resume" : "Pause"
                onClicked: {
                    idListModel.klicked2(1)
                }
            }
        }
        Text {
            x: 50
            font.family: Theme.fontFamilyHeading
            font.bold: true
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeHuge

            width: listView.width
            text: "Tripometer"
        }
    }
    ListView {
        clip: true
        id: listView

        Component.onCompleted: {

        }
        model: idListModel
        anchors.fill: parent
        anchors.topMargin: 120

        delegate: ListItem {
            id: delegate
            height: 100
            Text {
                font.family: Theme.fontFamilyHeading
                text: aLabel

                //  font.pixelSize:  Theme.fontSizeHuge
                color: Theme.primaryColor
            }
            Text {
                id: idText
                y: 30
                font.family: Theme.fontFamilyHeading
                text: aValue
                font.pixelSize: Theme.fontSizeHuge
                color: delegate.highlighted ? Theme.highlightColor : Theme.primaryColor
            }

            Text {
                y: 40
                x: idText.width
                font.family: Theme.fontFamily
                text: aUnit
                font.pixelSize: Theme.fontSizeTiny
                color: Theme.primaryColor
            }
            onPressed: {

            }

            onPressAndHold: {

                //   idListModel.get(index).fV = 10 * index
                //  console.log("Clicked " + index)
                // idListModel.klicked2(index)
            }
        }
        //  VerticalScrollDecorator{
        //  }
    }
}
