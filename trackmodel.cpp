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

  }
  
  m_oFileWatcher.addPath(sDataFilePath);
  QObject::connect(&m_oFileWatcher, SIGNAL(directoryChanged(QString)), this, SLOT(DirModified(QString)));
  ;
}

TrackModel::~TrackModel()
{

}

void TrackModel::TrackAdded(QString sTrack)
{
  int nRow = oc.size();
  beginInsertRows(QModelIndex(), nRow, nRow);
  oc.append(JustFileNameNoExt(sTrack));
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
  return oc.size();
}

QVariant TrackModel::data(const QModelIndex &index, int ) const
{
  return oc[index.row()];
}

QHash<int, QByteArray> TrackModel::roleNames() const
{
  QHash<int, QByteArray> roleNames;
  roleNames.insert(ValueRole, "aValue");
  return roleNames;
}
