import QtQuick 2.0
import Sailfish.Silica 1.0

TextField {
  property int nOwner: 1
  //  property alias touched: _editor.touched
  visible: idApp.nNrTeams >= nOwner
  label: {
    if (nOwner === 0)
      return "Report Header"

    return "Team " + nOwner + " name"
  }
  text: idApp.ocTeamName[nOwner]
}
