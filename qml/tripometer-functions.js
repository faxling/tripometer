function initDB() {

  if (db === undefined)
    db = Sql.LocalStorage.openDatabaseSync("PikeDB", "1.0",
                                           "Pike Database!", 1000000)

  db.transaction(function (tx) {

    tx.executeSql(
          'CREATE TABLE IF NOT EXISTS Catch_V2(pike_id INTEGER PRIMARY KEY, dbnumber INT , sDate TEXT, nLen INT, nOwner INT, fLO DOUBLE, fLA DOUBLE)')

    var rs = tx.executeSql("SELECT * FROM Catch_V2")
    var nRowLen = rs.rows.length
    for (var j = 0; j < nRowLen; j++) {
      var o = rs.rows.item(j)
      addPikeEx(Number(o.pike_id), o.nOwner, o.sDate, o.nLen, o.fLo,
                o.fLa, false)
    }
  })
}

function newSession() {
  deleteDB()
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

function showPike(nOwner) {
  if (nOwner === 1) {
    idPike1DockedPanel.show()
  } else {
    idPike2DockedPanel.show()
  }
}

function addPikeEx(nId, nOwner, sDate, nLen, fLo, fLa, bShow) {
  console.log("add pike " + nId)
  var oPModel
  var oSizeText
  var oPage
  if (nOwner === 1) {
    oSizeText = idSize1
    oPModel = idPikeModel1
    oPage = idPikePage1
    if (bShow)
      idPike1DockedPanel.show()
  } else {
    oPModel = idPikeModel2
    oSizeText = idSize2
    oPage = idPikePage2
    if (bShow)
      idPike2DockedPanel.show()
  }

  var sLenText = nLen + " cm"
  var nNewPikeCount = nPikeCount
  nNewPikeCount[nOwner] = nPikeCount[nOwner] + 1
  nNewPikeCount[0] = nPikeCount[0] + 1
  nPikeCount = nNewPikeCount
  oPModel.append({
                   "nId": Number(nId),
                   "sDate": sDate,
                   "sLength": sLenText,
                   "nLen": nLen,
                   "fLO": fLo,
                   "fLA": fLa
                 })

  var nC = oPModel.count
  oPage.currentIndex = nC - 1
  var nSum = 0
  for (var i = 0; i < nC; ++i) {
    nSum = nSum + oPModel.get(i).nLen
  }
  oSizeText.text = nSum + " cm"
  mainMap.loadPikeInMap(nId, nOwner, fLo, fLa)
}

function removePike(nId) {
  console.log("removePike " + nId)
  db.transaction(function (tx) {
    tx.executeSql('DELETE FROM Catch_V2 WHERE=pike_id=?', [nId])
  })
}

function addPike(nOwner) {
  var tPos = mainMap.currentPos()
  var sDate = new Date().toLocaleTimeString('en-GB')
  var nLen = 50

  db.transaction(function (tx) {

    var rs = tx.executeSql(
          'INSERT INTO Catch_V2 VALUES(?,?,?,?,?,?,?)',
          [undefined, 1, sDate, nLen, nOwner, tPos.longitude, tPos.latitude])

    addPikeEx(rs.insertId, nOwner, sDate, nLen, tPos.longitude,
              tPos.latitude, true)
  })
}
