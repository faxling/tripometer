import QtQuick 2.0
import Sailfish.Silica 1.0

TextField {
  property int nOwner: 1
  property alias touched: _editor.touched
  visible: idApp.nNrTeams >= nOwner
  text: idApp.ocTeamName[nOwner - 1]
}
