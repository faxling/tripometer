function initDB() {

  if (db === undefined)
    db = Sql.LocalStorage.openDatabaseSync("PikeDB", "1.0",
                                           "Pike Database!", 1000000)

  db.transaction(function (tx) {

    tx.executeSql(
          'CREATE TABLE IF NOT EXISTS Catch_V2(pike_id INTEGER PRIMARY KEY, dbnumber INT , sDate TEXT,sImage TEXT, nLen INT, nOwner INT, fLo DOUBLE, fLa DOUBLE)')

    var rs = tx.executeSql("SELECT * FROM Catch_V2")
    var nRowLen = rs.rows.length
    for (var j = 0; j < nRowLen; j++) {
      var o = rs.rows.item(j)
      addPikeEx(Number(o.pike_id), o.nOwner, o.sDate, o.sImage, o.nLen, o.fLo,
                o.fLa, false)
    }
  })
}

function newSession() {
  deleteDB()
  mainMap.removePikesInMap()
  idApp.sSumSize2 = ""
  idApp.sSumSize1 = ""
  idListModel.klicked2(3)
  idApp.bFlipped = true
  idApp.bAppStarted = true
}

function resumeSession() {
  idApp.bFlipped = true
  idApp.bAppStarted = true
}

function deleteDB() {

  db.transaction(function (tx) {
    tx.executeSql("DROP TABLE Catch_V2")
  })
  initDB()

  idPikeModel1.clear()
  idPikeModel2.clear()
  nPikeCount = [0, 0, 0]
}

function calcSizeAndDisplay(nOwner, value) {
  var oPModel
  var currentIndex
  var oSizeText

  if (nOwner === 1) {
    currentIndex = idPikePanel_1.currentIndex
    oPModel = idPikeModel1
  } else {
    currentIndex = idPikePanel_2.currentIndex
    oPModel = idPikeModel2
  }

  if (value !== undefined) {
    var nLen = Math.round(value)
    var nId = oPModel.get(currentIndex).nId
    oPModel.setProperty(currentIndex, "nLen", nLen)
    oPModel.setProperty(currentIndex, "sLength", nLen + " cm")
    db.transaction(function (tx) {
      tx.executeSql('UPDATE Catch_V2 SET nLen=? WHERE pike_id=?', [nLen, nId])
    })
  }

  var nC = oPModel.count

  var ocLength = []
  for (var i = 0; i < nC; ++i) {
    ocLength.push(oPModel.get(i).nLen)
  }

  ocLength.sort()
  // @disable-check M325
  if (nPikesCounted != null) {
    if (nC > nPikesCounted) {
      nC = nPikesCounted
    }
  }
  var nSum = 0
  for (i = 0; i < nC; ++i) {
    nSum = nSum + ocLength[i]
  }

  oSizeText = nSum + " cm"
  if (nOwner === 1) {
    idApp.sSumSize1 = oSizeText
  } else {
    idApp.sSumSize2 = oSizeText
  }
}

function showPike(nOwner) {
  if (nOwner === 1) {
    idPikePanel_1.show()
    idPikePanel_1.currentIndex = -1
  } else {
    idPikePanel_2.show()
    idPikePanel_2.currentIndex = -1
  }
}

function addPikeImage(i, oPModel, sImage) {
  var nId = oPModel.get(i).nId
  oPModel.get(i).sImage = sImage
  db.transaction(function (tx) {
    tx.executeSql('UPDATE Catch_V2 SET sImage=? WHERE pike_id=?', [sImage, nId])
  })
}

function addPikeEx(nId, nOwner, sDate, sImage, nLen, fLo, fLa, bShow) {
  var oPModel
  var currentIndex
  var oPanel
  if (nOwner === 1) {
    oPModel = idPikeModel1
    currentIndex = idPikePanel_1.currentIndex
    oPanel = idPikePanel_1
    if (bShow)
      idPikePanel_1.show()
  } else {
    oPModel = idPikeModel2
    oPanel = idPikePanel_2
    currentIndex = idPikePanel_2.currentIndex
    if (bShow)
      idPikePanel_2.show()
  }

  var sLenText = nLen + " cm"
  var nNewPikeCount = nPikeCount
  nNewPikeCount[nOwner] = nPikeCount[nOwner] + 1
  nNewPikeCount[0] = nPikeCount[0] + 1
  nPikeCount = nNewPikeCount
  oPModel.append({
                   "nId": Number(nId),
                   "sDate": sDate,
                   "sImage": sImage,
                   "sLength": sLenText,
                   "nLen": nLen,
                   "fLo": fLo,
                   "fLa": fLa
                 })
  var nC = oPModel.count
  oPanel.currentIndex = nC - 1
  calcSizeAndDisplay(nOwner)
  mainMap.loadPikeInMap(nId, nOwner, fLo, fLa)
}

function centerPike(nId, oPModel) {
  var nC = oPModel.count
  for (var i = 0; i < nC; ++i) {
    if (oPModel.get(i).nId === nId) {
      mainMap.setLookAt(oPModel.get(i).fLa, oPModel.get(i).fLo)
      break
    }
  }
}
function removePike(nId, oPModel, nOwnerIn) {
  var nC = oPModel.count
  for (var i = 0; i < nC; ++i) {
    if (oPModel.get(i).nId === nId) {
      oPModel.remove(i)
      break
    }
  }

  db.transaction(function (tx) {
    tx.executeSql('DELETE FROM Catch_V2 WHERE pike_id=?', [nId])
  })

  mainMap.removePikeInMap(nId)
  calcSizeAndDisplay(nOwner)
  var nNewPikeCount = idApp.nPikeCount
  nNewPikeCount[nOwner] = nPikeCount[nOwner] - 1
  nNewPikeCount[0] = nPikeCount[0] - 1
  idApp.nPikeCount = nNewPikeCount
}

function zN(nValue) {
  if (nValue < 10)
    return ('0' + nValue.toString())
  return nValue
}

function pikeDateTimeStr() {
  var o = new Date()
  return o.getFullYear(
        ) + "-" + (o.getMonth() + 1) + "-" + o.getDate() + " " + zN(
        o.getHours()) + ":" + zN(o.getMinutes()) + ":" + zN(o.getSeconds())
}

function addPike(nOwner) {
  var tPos = mainMap.currentPos()

  if (tPos.longitude === undefined)
    return
  var sDate = pikeDateTimeStr()
  var nLen = 50

  db.transaction(function (tx) {

    var rs = tx.executeSql(
          'INSERT INTO Catch_V2 VALUES(?,?,?,?,?,?,?,?)',
          [undefined, 1, sDate, "image://theme/icon-m-file-image", nLen, nOwner, tPos.longitude, tPos.latitude])

    addPikeEx(rs.insertId, nOwner, sDate, "image://theme/icon-m-file-image",
              nLen, tPos.longitude, tPos.latitude, true)
  })
}
