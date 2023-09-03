#include "trackmodel.h"
#include "Utils.h"
#include "infolistmodel.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <time.h>

enum TRACK_ROLES_t
{
  NAME_t = Qt::UserRole,
  ISLOADED_t,
  ID_t,
  SELECTED_t,
  LENGTH_t,
  DATETIME_t,
  DURATION_t,
  MAX_SPEED_t,
  DISKSIZE_t,
  TYPE_t
};

extern QObject* g_pTheMap;

TrackModel::ModelDataNode TrackModel::GetNodeFromTrack(const QString& sTrackName, bool bIsLoaded)
{
  ModelDataNode tNode;
  MarkData t = GetMarkData(sTrackName);
  tNode.sName = sTrackName;
  tNode.nId = m_nLastId;
  tNode.nType = t.nType;
  // -1
  if (tNode.nType >= 0)
  {
    tNode.la = t.la;
    tNode.lo = t.lo;
    tNode.nType = t.nType;
    tNode.nTime = t.nTime;
    tNode.bIsLoaded = bIsLoaded;
    tNode.sDateTime = FormatDateTime(t.nTime);
    tNode.sMaxSpeed = FormatKmH(t.speed * 3.6) + " km/h";
    tNode.sDuration = FormatDuration(t.nDuration );
    if (tNode.nType == 1)
      tNode.sLength = QString::asprintf("%.4f %.4f",t.lo, t.la);
    else
      tNode.sLength = FormatKm(t.len / 1000.0) + " km";

    tNode.sDiskSize = FormatNrBytes(t.nSize);
  }

  // nType = 1 point



  if (t.nType <= 0)
  {
    if (t.nSize == 0)
    {
      if (t.nType == -1)
        t.nType = 2;
      qDebug() << sTrackName << " Len " << t.len;
      tNode.nType = t.nType;
      QString sGpxFileName = GpxFullName(sTrackName);
      QFileInfo oFI(sGpxFileName);
      t.nSize = oFI.size();
      tNode.sDiskSize = FormatNrBytes(t.nSize);
      MaepGeodata* track = maep_geodata_new_from_file(sGpxFileName.toUtf8().data(), 0);
      MaepGeodataTrackIter iter;
      maep_geodata_track_iter_new(&iter, track);
      int status = 0;

      maep_geodata_track_iter_next(&iter, &status);
      t.la = rad2deg(iter.cur->coord.rlat);
      t.lo = rad2deg(iter.cur->coord.rlon);
      tNode.la = t.la;
      tNode.lo = t.lo;
      t.nTime = oFI.lastModified().toTime_t();
      tNode.bIsLoaded = false;
      tNode.sDateTime = FormatDateTime(t.nTime);
      t.nDuration = maep_geodata_track_get_duration(track);
      tNode.sDuration = FormatDuration(t.nDuration);
      t.len = maep_geodata_track_get_metric_length(track);
      tNode.sLength = FormatKm(t.len / 1000.0) + " km";
      tNode.bSelected = true;
      WriteMarkData(sTrackName, t);
      g_object_unref(G_OBJECT(track));
    }
  }

  return tNode;
}

TrackModel::TrackModel(QObject*)
{
  QString sDataFilePath = StorageDir();
  QDir oDir(sDataFilePath);
  oDir.setNameFilters(QStringList() << "*.dat");
  oDir.setFilter(QDir::Files);
  QStringList oc = oDir.entryList();

  for (auto& oI : oc)
  {
    m_nLastId = m_oc.size();
    ModelDataNode tNode = GetNodeFromTrack(JustFileNameNoExt(oI), false);
    m_oc.append(tNode);
  }
}

TrackModel::~TrackModel()
{
}

void TrackModel::trackCenter(int nId)
{
  std::find_if(m_oc.begin(), m_oc.end(), [&](const ModelDataNode& t) {
    if (t.nId == nId)
    {
      trackLoaded(nId);
      QMetaObject::invokeMethod(g_pTheMap, "centerTrack", Q_ARG(QString, t.sName));
      return true;
    }
    else
      return false;
  });
}

void TrackModel::trackUnloaded(int nId)
{
  int nRow = -1;
  for (auto& oJ : m_oc)
  {
    if (nId == oJ.nId)
    {
      nRow = IndexOf(oJ, m_oc);
      oJ.bIsLoaded = false;
      break;
    }
  }
  if (nRow < 0)
    return;

  QModelIndex oMI = index(nRow, 0, QModelIndex());
  QVector<int> oc;
  oc.push_back(ISLOADED_t);
  emit dataChanged(oMI, oMI, oc);
}

void TrackModel::trackLoaded(int nId)
{
  int nRow = -1;
  for (auto& oJ : m_oc)
  {
    if (nId == oJ.nId)
    {
      nRow = IndexOf(oJ, m_oc);
      oJ.bIsLoaded = true;
      break;
    }
  }
  if (nRow < 0)
    return;

  QModelIndex oMI = index(nRow, 0, QModelIndex());
  QVector<int> oc;
  oc.push_back(ISLOADED_t);
  emit dataChanged(oMI, oMI, oc);
}

extern QObject* g_pTheMap;

void TrackModel::loadSelected()
{
  QVector<int> oc;
  oc.push_back(ISLOADED_t);
  for (auto& oJ : m_oc)
  {
    if (oJ.bSelected == true)
    {
      oJ.bIsLoaded = true;
      QMetaObject::invokeMethod(g_pTheMap, "loadTrack", Q_ARG(QString, oJ.sName), Q_ARG(int, oJ.nId));
      QModelIndex oMI = index(IndexOf(oJ, m_oc), 0, QModelIndex());
      emit dataChanged(oMI, oMI, oc);
    }
  }
}

void TrackModel::markAllUnload()
{
  QVector<int> oc;
  oc.push_back(ISLOADED_t);
  for (auto& oJ : m_oc)
  {
    if (oJ.bSelected == true)
    {
      oJ.bIsLoaded = false;
      QModelIndex oMI = index(IndexOf(oJ, m_oc), 1, QModelIndex());
      emit dataChanged(oMI, oMI, oc);
    }
  }
}

void TrackModel::unloadSelected()
{

  QVector<int> oc;
  oc.push_back(ISLOADED_t);
  for (auto& oJ : m_oc)
  {
    if (oJ.bSelected == true)
    {
      oJ.bIsLoaded = false;
      QMetaObject::invokeMethod(g_pTheMap, "unloadTrack", Q_ARG(int, oJ.nId));
      QModelIndex oMI = index(IndexOf(oJ, m_oc), 0, QModelIndex());
      emit dataChanged(oMI, oMI, oc);
    }
  }
}

void TrackModel::deleteSelected()
{
  for (;;)
  {
    auto oI = std::find_if(m_oc.begin(), m_oc.end(), [&](const ModelDataNode& t) {
      if (t.bSelected == true)
      {
        int nRow = IndexOf(t, m_oc);
        if (t.nType == 0)
          QFile::remove(GpxFullName(t.sName));
        QFile::remove(GpxDatFullName(t.sName));
        beginRemoveRows(QModelIndex(), nRow, nRow);
        m_oc.remove(nRow, 1);
        endRemoveRows();
        return true;
      }
      return false;
    });

    if (oI == m_oc.end())
      return;
  }

  UpdateSelected();
}
QModelIndex TrackModel::IndexFromId(int nId) const
{
  for (auto& oJ : m_oc)
  {
    if (nId == oJ.nId)
    {
      return index(IndexOf(oJ, m_oc), 0, QModelIndex());
      break;
    }
  }
  return QModelIndex();
}

void TrackModel::trackDelete(int nId)
{
  int nRow = -1;

  for (auto& oJ : m_oc)
  {
    if (nId == oJ.nId)
    {
      nRow = IndexOf(oJ, m_oc);
      QFile::remove(GpxFullName(oJ.sName));
      QFile::remove(GpxDatFullName(oJ.sName));
      break;
    }
  }
  if (nRow < 0)
    return;

  beginRemoveRows(QModelIndex(), nRow, nRow);
  m_oc.remove(nRow, 1);
  endRemoveRows();
  UpdateSelected();
}

void TrackModel::trackRename(QString _sTrackName, int nId)
{
  for (auto& oJ : m_oc)
  {
    if (nId == oJ.nId)
    {
      if (oJ.sName == _sTrackName)
        return;

      QString sTrackName = GpxNewName(_sTrackName);

      QFile::rename(GpxFullName(oJ.sName), GpxFullName(sTrackName));
      QFile::rename(GpxDatFullName(oJ.sName), GpxDatFullName(sTrackName));
      oJ.sName = sTrackName;

      QVector<int> oc;
      oc.push_back(NAME_t);
      QModelIndex oMI = IndexFromId(nId);
      emit dataChanged(oMI, oMI, oc);
      break;
    }
  }
}

int TrackModel::nextId()
{
  return m_nLastId + 1;
}

void TrackModel::trackImport(const QString& sPath)
{
  QString sTrackName = GpxNewName(JustFileNameNoExt(sPath));
  QString s = GpxFullName(sTrackName);
  QFile::copy(sPath, s);
  trackAdd(sTrackName);
}

void TrackModel::trackAdd(const QString& sTrackName)
{
  ++m_nLastId;

  ModelDataNode tNode = GetNodeFromTrack(sTrackName, true);
  tNode.bSelected = false;
  m_oc.push_back(tNode);
  beginInsertRows(QModelIndex(), m_oc.size() - 1, m_oc.size() - 1);
  endInsertRows();
  UpdateSelected();
}

QModelIndex TrackModel::index(int row, int column, const QModelIndex&) const
{
  return createIndex(row, column);
}

int TrackModel::columnCount(const QModelIndex&) const
{
  return 1;
}

QModelIndex TrackModel::parent(const QModelIndex&) const
{
  return QModelIndex();
}

int TrackModel::rowCount(const QModelIndex&) const
{
  return m_oc.size();
}

void TrackModel::UpdateSelected()
{
  int nCount = 0;
  for (auto& oJ : m_oc)
    nCount += oJ.bSelected ? 1 : 0;

  InfoListModel::m_pRoot->setProperty("nSelectCount", nCount);
}

bool TrackModel::setData(const QModelIndex& index, const QVariant& value, int nRole)
{
  if (nRole == SELECTED_t)
  {
    m_oc[index.row()].bSelected = value.toBool();
    QVector<int> oc;
    oc.push_back(SELECTED_t);
    emit dataChanged(index, index, oc);
    UpdateSelected();
  }
  return true;
}

QString FormatType(int nType)
{
  switch (nType)
  {
  case 0:
    return "track";
  case 1:
    return "point";
  case 2:
    return "gpxTrack";
  }
  return "-";
}

QVariant TrackModel::data(const QModelIndex& index, int nRole) const
{
  switch (nRole)
  {
  case NAME_t:
    return m_oc[index.row()].sName;
  case SELECTED_t:
    return m_oc[index.row()].bSelected;
  case ISLOADED_t:
    return m_oc[index.row()].bIsLoaded;
  case ID_t:
    return m_oc[index.row()].nId;
  case LENGTH_t:
    return m_oc[index.row()].sLength;
  case DATETIME_t:
    return m_oc[index.row()].sDateTime;
  case DURATION_t:
    return m_oc[index.row()].sDuration;
  case MAX_SPEED_t:
    return m_oc[index.row()].sMaxSpeed;
  case DISKSIZE_t:
    return m_oc[index.row()].sDiskSize;
  case TYPE_t:
    return FormatType(m_oc[index.row()].nType);
  }

  return QVariant();
}

QHash<int, QByteArray> TrackModel::roleNames() const
{
  QHash<int, QByteArray> roleNames;
  roleNames.insert(NAME_t, "aValue");
  roleNames.insert(ISLOADED_t, "bLoaded");
  roleNames.insert(ID_t, "nId");
  roleNames.insert(SELECTED_t, "bSelected");
  roleNames.insert(LENGTH_t, "sLength");
  roleNames.insert(DURATION_t, "sDuration");
  roleNames.insert(DATETIME_t, "sDateTime");
  roleNames.insert(MAX_SPEED_t, "sMaxSpeed");
  roleNames.insert(DISKSIZE_t, "sDiskSize");
  roleNames.insert(TYPE_t, "sType");
  return roleNames;
}
