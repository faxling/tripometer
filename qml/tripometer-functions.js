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
  idApp.ocSumSize = ["", "", ""]
  idListModel.klicked2(3)
  idApp.bFlipped = true
  idApp.bAppStarted = true
}

function resumeSession() {
  idApp.bFlipped = true
  idApp.bAppStarted = true
  idApp.bIsPause = false
}

function viewSession() {
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
  idApp.ocPikeCount = [0, 0, 0, 0]
}

function calcSizeAndDisplay(nOwner, value) {
  var oPModel
  var nCurrentIndex
  var oSizeText

  if (idPikePanel_1 == null)
    return
  if (idPikePanel_2 == null)
    return
  if (idPikePanel_3 == null)
    return
  if (nOwner === 1) {
    nCurrentIndex = idPikePanel_1.currentIndex
    oPModel = idPikeModel1
  } else if (nOwner === 2) {
    nCurrentIndex = idPikePanel_2.currentIndex
    oPModel = idPikeModel2
  } else {
    nCurrentIndex = idPikePanel_3.currentIndex
    oPModel = idPikeModel3
  }

  if (value !== undefined) {
    var nLen = Math.round(value)
    var nId = oPModel.get(nCurrentIndex).nId
    oPModel.setProperty(nCurrentIndex, "nLen", nLen)
    oPModel.setProperty(nCurrentIndex, "sLength", nLen + " cm")
    db.transaction(function (tx) {
      tx.executeSql('UPDATE Catch_V2 SET nLen=? WHERE pike_id=?', [nLen, nId])
    })
  }

  var nC = oPModel.count

  var ocLength = []
  for (var i = 0; i < nC; ++i) {
    nLen = oPModel.get(i).nLen

    if (nLen < idApp.nMinSize)
      continue
    ocLength.push(nLen)
  }

  ocLength.sort()
  nC = ocLength.length

  if (nC > idApp.nPikesCounted) {
    nC = idApp.nPikesCounted
  }

  var nSum = 0

  for (i = 0; i < nC; ++i) {
    nSum = nSum + ocLength[i]
  }

  oSizeText = nSum + " cm"
  var t = idApp.ocSumSize
  t[nOwner - 1] = oSizeText
  idApp.ocSumSize = t

  var ocNewPikeCount = idApp.ocPikeCount
  ocNewPikeCount[nOwner] = nC
  idApp.ocPikeCount = ocNewPikeCount
}

function showPike(nOwner) {
  if (nOwner === 1) {
    idPikePanel_1.show()
    idPikePanel_1.currentIndex = -1
  } else if (nOwner === 2) {
    idPikePanel_2.show()
    idPikePanel_2.currentIndex = -1
  } else {
    idPikePanel_3.show()
    idPikePanel_3.currentIndex = -1
  }
}

function addPikeImage(i, oPModel, sImage) {
  var nId = oPModel.get(i).nId
  oPModel.get(i).sImage = sImage
  oImageThumb.save(sImage)
  oPModel.get(i).sImageThumb = String(oImageThumb.name(sImage))
  db.transaction(function (tx) {
    tx.executeSql('UPDATE Catch_V2 SET sImage=? WHERE pike_id=?', [sImage, nId])
  })
}

function addPikeEx(nId, nOwner, sDate, sImage, nLen, fLo, fLa, bShow) {
  var oPModel
  var nCurrentIndex
  var oPanel
  if (nOwner === 1) {
    oPModel = idPikeModel1
    oPanel = idPikePanel_1
    if (bShow)
      idPikePanel_1.show()
  } else if (nOwner === 2) {
    oPModel = idPikeModel2
    oPanel = idPikePanel_2
    if (bShow)
      idPikePanel_2.show()
  } else {
    oPModel = idPikeModel3
    oPanel = idPikePanel_3
    if (bShow)
      idPikePanel_3.show()
  }

  var sLenText = nLen + " cm"

  oPModel.append({
                   "nId": Number(nId),
                   "sDate": sDate,
                   "sImage": sImage,
                   "sImageThumb": String(oImageThumb.name(sImage)),
                   "sLength": sLenText,
                   "nLen": nLen,
                   "fLo": fLo,
                   "fLa": fLa
                 })

  var nC = oPModel.count

  calcSizeAndDisplay(nOwner)
  mainMap.loadPikeInMap(nId, nOwner, fLo, fLa)
  // oPanel.currentIndex = nC - 1
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

function pikeDateFromStr(sStr) {
  console.log(sStr)
  var o = new Date()
  var oc = sStr.split("-")
  o.setFullYear(oc[0])
  o.setMonth(oc[1])
  var ocTimeEx = String(oc[2]).split(" ")
  o.setDate(ocTimeEx[0])
  var ocTime = ocTimeEx[1].split(":")
  o.setHours(ocTime[0])
  o.setMinutes(ocTime[1])
  console.log(o)
  return o
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
          [undefined, 1, sDate, null, nLen, nOwner, tPos.longitude, tPos.latitude])

    addPikeEx(rs.insertId, nOwner, sDate, "image://theme/icon-m-file-image",
              nLen, tPos.longitude, tPos.latitude, true)
  })
}
