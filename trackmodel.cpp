#include "trackmodel.h"

#include "utils.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <time.h>

enum TRACK_ROLES_t
{
  NAME_t = Qt::UserRole,
  ISLOADED_t,
  ID_t ,
  SELECTED_t,
  LENGTH_t,
  DATETIME_t,
  DURATION_t,
  MAX_SPEED_t,
  DISKSIZE_t
};

extern QObject* g_pTheMap;


TrackModel::ModelDataNode TrackModel::GetNodeFromTrack(const QString& sTrackName, bool bIsLoaded)
{
  ModelDataNode tNode;
  MarkData t = GetMarkData(sTrackName);
  tNode.sName = sTrackName;
  tNode.nId = m_nLastId;
  tNode.la = t.la;
  tNode.lo = t.lo;
  tNode.nType = t.nType;
  tNode.nTime = t.nTime;
  tNode.bIsLoaded = bIsLoaded;
  tNode.sDateTime = FormatDateTime(t.nTime);
  tNode.sMaxSpeed = FormatKmH(t.speed*3.6) + " km/h";
  tNode.sDuration = FormatDuration(t.nDuration*10);
  tNode.sLength = FormatKm(t.len / 1000.0) + " km";
  tNode.sDiskSize = FormatNrBytes(t.nSize );

  if (t.nType == 0)
  {
    if (t.nDuration == 0)
    {
      QString sGpxFileName = GpxFullName(sTrackName);
      QFile oF(sGpxFileName);
      oF.open(QIODevice::ReadWrite);
      t.nSize = oF.size();
      oF.close();
      tNode.sDiskSize = FormatNrBytes(t.nSize );
      MaepGeodata *track = maep_geodata_new_from_file(sGpxFileName.toUtf8().data(), 0);
      t.nDuration = maep_geodata_track_get_duration(track);
      tNode.sDuration = FormatDuration(t.nDuration* 10);
      t.len = maep_geodata_track_get_metric_length(track);

      WriteMarkData(sGpxFileName,  t );
      g_object_unref(G_OBJECT(track));
    }
  }
  else
  {
    tNode.sLength = "x";
    tNode.sDuration = "x";
  }

  return tNode;
}

TrackModel::TrackModel(QObject *)
{
  QString sDataFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  m_nLastId = 0;
  QDir oDir(sDataFilePath);
  oDir.setNameFilters(QStringList() << "*.dat");
  oDir.setFilter(QDir::Files);

  for (auto &oI : oDir.entryList() )
  {
    ++m_nLastId;
    ModelDataNode tNode =  GetNodeFromTrack(JustFileNameNoExt(oI),false);
    m_oc.append(tNode);
  }

}

TrackModel::~TrackModel()
{

}


void TrackModel::trackCenter(int nId)
{
  std::find_if(m_oc.begin(),m_oc.end(), [&] (const ModelDataNode &t)
  {
    if (t.nId == nId)
    {
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
      nRow = IndexOf(oJ,m_oc);
      oJ.bIsLoaded = false;
      break;
    }
  }
  if (nRow<0)
    return;

  QModelIndex oMI = index(nRow, 1, QModelIndex());
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
      nRow = IndexOf(oJ,m_oc);
      oJ.bIsLoaded = true;
      break;
    }
  }
  if (nRow<0)
    return;

  QModelIndex oMI = index(nRow, 1, QModelIndex());
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
      QMetaObject::invokeMethod(g_pTheMap, "loadTrack", Q_ARG(QString, oJ.sName),  Q_ARG(int, oJ.nId));
      QModelIndex oMI = index(IndexOf(oJ,m_oc), 1, QModelIndex());
      emit dataChanged(oMI, oMI, oc) ;
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
      QMetaObject::invokeMethod(g_pTheMap, "unloadTrack",  Q_ARG(int, oJ.nId));
      QModelIndex oMI = index(IndexOf(oJ,m_oc), 1, QModelIndex());
      emit dataChanged(oMI, oMI, oc) ;
    }
  }
}

void TrackModel::deleteSelected()
{
  for (;;)
  {
    auto oI = std::find_if(m_oc.begin(),m_oc.end(), [&] (const ModelDataNode &t)
    {
      if (t.bSelected == true)
      {
        int nRow = IndexOf(t,m_oc);
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
}

QModelIndex TrackModel::IndexFromId(int nId) const
{
  for (auto& oJ : m_oc)
  {
    if (nId == oJ.nId)
    {
      return index(IndexOf(oJ,m_oc),0, QModelIndex());
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
      nRow = IndexOf(oJ,m_oc);
      QFile::remove(GpxFullName(oJ.sName));
      QFile::remove(GpxDatFullName(oJ.sName));
      break;
    }
  }
  if (nRow<0)
    return;

  beginRemoveRows(QModelIndex(), nRow, nRow);
  m_oc.remove(nRow, 1);
  endRemoveRows();
}

void TrackModel::trackRename(QString _sTrackName, int nId)
{
  QString sTrackName = _sTrackName;
  for (auto& oJ : m_oc)
  {
    if (nId == oJ.nId)
    {
      if (oJ.sName == sTrackName)
        return;
      int nCount=0;

      bool bNameChanged = false;

      while(QFile::exists(GpxDatFullName(sTrackName))== true)
      {
        bNameChanged = true;
        sTrackName.sprintf("%ls(%d)",(wchar_t*)_sTrackName.utf16(),++nCount);
      }

      QFile::rename(GpxFullName(oJ.sName),GpxFullName(sTrackName));
      QFile::rename(GpxDatFullName(oJ.sName),GpxDatFullName(sTrackName));
      oJ.sName = sTrackName;
      if ( bNameChanged==true )
      {
        QVector<int> oc;
        oc.push_back(NAME_t);
        QModelIndex oMI = IndexFromId(nId);
        emit dataChanged(oMI, oMI, oc);
      }

      break;
    }
  }

}

int TrackModel::nextId()
{
  return m_nLastId + 1;
}


void TrackModel::trackAdd(const QString& sTrackName)
{
  ++m_nLastId;
  int nRow = m_oc.size();
  beginInsertRows(QModelIndex(), nRow, nRow);
  ModelDataNode tNode = GetNodeFromTrack(sTrackName, true);
  m_oc.append(tNode);
  endInsertRows();
}

QModelIndex TrackModel::index(int row , int column , const QModelIndex&) const
{
  return createIndex(row, column);
}

int TrackModel::columnCount(const QModelIndex &) const
{
  return 1;
}

QModelIndex TrackModel::parent(const QModelIndex&) const
{
  return QModelIndex();
}

int TrackModel::rowCount(const QModelIndex &) const
{
  return m_oc.size();
}



bool TrackModel::setData(const QModelIndex &index, const QVariant &value, int nRole)
{
  if (nRole == SELECTED_t)
  {
    m_oc[index.row()].bSelected = value.toBool();
    QVector<int> oc;
    oc.push_back(SELECTED_t);
    emit dataChanged(index, index, oc);
  }
  return true;

}

QVariant TrackModel::data(const QModelIndex &index, int nRole) const
{
  switch (nRole)
  {
  case NAME_t:
    return m_oc[index.row()].sName;
  case SELECTED_t:
    return m_oc[index.row()].bSelected;
  case ISLOADED_t:
    return  m_oc[index.row()].bIsLoaded;
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
  return roleNames;
}
