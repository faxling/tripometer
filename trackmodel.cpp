#include "trackmodel.h"
#include "src/track.h"
#include "utils.h"
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

enum TRACK_ROLES_t
{
  NAME_t = Qt::UserRole,
  ISLOADED_t,
  ID_t ,
  SELECTED_t
};


TrackModel::TrackModel(QObject *)
{
  QString sDataFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  m_nLastId = 0;
  QDir oDir(sDataFilePath);
  oDir.setNameFilters(QStringList() << "*.gpx");
  oDir.setFilter(QDir::Files);

  for (auto &oI : oDir.entryList() )
  {
    ++m_nLastId;
    ModelDataNode tNode;
    tNode.sName = JustFileNameNoExt(oI);
    tNode.nId = m_nLastId;
    m_oc.append(tNode);
  }
  ;
}

TrackModel::~TrackModel()
{

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
    int nRow = -1;
    for (auto& oJ : m_oc)
    {
      if (oJ.bSelected == true)
      {
        nRow = IndexOf(oJ,m_oc);
        QFile::remove(GpxFullName(oJ.sName));
        break;
      }
    }
    if (nRow<0)
      return;

    beginRemoveRows(QModelIndex(), nRow, nRow);
    m_oc.remove(nRow, 1);
    endRemoveRows();
  }
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
      break;
    }
  }
  if (nRow<0)
    return;

  beginRemoveRows(QModelIndex(), nRow, nRow);
  m_oc.remove(nRow, 1);
  endRemoveRows();
}

void TrackModel::trackRename(QString sTrackName, int nId)
{
  for (auto& oJ : m_oc)
  {
    if (nId == oJ.nId)
    {
      if (oJ.sName == sTrackName)
        return;
      QFile::rename(GpxFullName(oJ.sName),GpxFullName(sTrackName));
      oJ.sName = sTrackName;
      break;
    }
  }

}


void TrackModel::trackAdd(QString sTrack)
{
  ++m_nLastId;
  int nRow = m_oc.size();
  beginInsertRows(QModelIndex(), nRow, nRow);
  ModelDataNode tNode;
  tNode.sName = sTrack;
  tNode.nId = m_nLastId;
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
    int nRow = index.row();
    qDebug() << nRow;
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
  return roleNames;
}
