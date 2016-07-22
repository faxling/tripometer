#include "trackmodel.h"
#include "src/track.h"
#include "utils.h"
#include <QStandardPaths>
#include <QDir>


TrackModel::TrackModel(QObject *)
{
  QString sDataFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);

  QDir oDir(sDataFilePath);
  oDir.setNameFilters(QStringList() << "*.gpx");
  oDir.setFilter(QDir::Files);

  for (auto &oI : oDir.entryList() )
  {
    oc.append(JustFileNameNoExt(oI));
    ocBool.append(false);

  }
  

  ;
}

TrackModel::~TrackModel()
{

}

void TrackModel::tracksUnloaded(QString sTrack)
{



}
void TrackModel::trackLoaded(QString sTrack)
{

  int nRow = 0;
  int nRowF = -1;
  for (auto& oJ : oc)
  {
    if (sTrack == oJ)
    {
      nRowF = nRow;
      break;
    }
    nRow++;
  }
  if (nRowF<0)
    return;
  ocBool[nRowF] = true;
  QModelIndex oMI = index(nRowF, 1, QModelIndex());
  QVector<int> oc;
  oc.push_back(Qt::UserRole+1);
  emit dataChanged(oMI, oMI, oc);

}
void TrackModel::TrackAdded(QString sTrack)
{
  int nRow = oc.size();
  beginInsertRows(QModelIndex(), nRow, nRow);
  oc.append(JustFileNameNoExt(sTrack));
  ocBool.append(true);
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
  return oc.size();
}

QVariant TrackModel::data(const QModelIndex &index, int nRole) const
{
  if (nRole == Qt::UserRole)
    return oc[index.row()];
  return ocBool[index.row()];
}

QHash<int, QByteArray> TrackModel::roleNames() const
{
  QHash<int, QByteArray> roleNames;
  roleNames.insert(Qt::UserRole, "aValue");
  roleNames.insert(Qt::UserRole +1, "bLoaded");
  return roleNames;
}
