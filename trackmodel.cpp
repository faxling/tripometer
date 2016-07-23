#include "trackmodel.h"
#include "src/track.h"
#include "utils.h"
#include <QStandardPaths>
#include <QDir>


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
    tNode.bIsLoaded = false;
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
  oc.push_back(Qt::UserRole+1);
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
  oc.push_back(Qt::UserRole+1);
  emit dataChanged(oMI, oMI, oc);

}
void TrackModel::TrackAdded(QString sTrack)
{
  ++m_nLastId;
  int nRow = m_oc.size();
  beginInsertRows(QModelIndex(), nRow, nRow);
  ModelDataNode tNode;
  tNode.sName = JustFileNameNoExt(sTrack);
  tNode.bIsLoaded = true;
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
  return 2;
}

QModelIndex TrackModel::parent(const QModelIndex&) const
{
  return QModelIndex();
}

int TrackModel::rowCount(const QModelIndex &) const
{
  return m_oc.size();
}

QVariant TrackModel::data(const QModelIndex &index, int nRole) const
{
  if (nRole == Qt::UserRole)
    return m_oc[index.row()].sName;

  if (nRole == (Qt::UserRole + 2))
    return m_oc[index.row()].nId;

  return m_oc[index.row()].bIsLoaded;
}

QHash<int, QByteArray> TrackModel::roleNames() const
{
  QHash<int, QByteArray> roleNames;
  roleNames.insert(Qt::UserRole, "aValue");
  roleNames.insert(Qt::UserRole +1, "bLoaded");
  roleNames.insert(Qt::UserRole +2, "nId");

  return roleNames;
}
