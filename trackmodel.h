#ifndef TRACKMODEL_H
#define TRACKMODEL_H

#include <QAbstractListModel>
#include <QCompass>
#include <QGeoCoordinate>
#include <QGeoLocation>
#include <QGeoPositionInfo>
#include <QList>
#include <QObject>
#include <QSortFilterProxyModel>
#include <stdio.h>

class MssTimer;


class TrackModelFiltered : public QSortFilterProxyModel
{
  Q_OBJECT
public:
  Q_INVOKABLE void setFilter(QString sFilter);
  bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
  QString m_sFilterStr;
};

class TrackModel : public QAbstractItemModel
{
  Q_OBJECT

public:
  Q_INVOKABLE int nextId();
  Q_INVOKABLE void trackAdd(const QString& sName);
  Q_INVOKABLE void trackImport(const QString& sPath);
  Q_INVOKABLE void trackCenter(int nId);
  Q_INVOKABLE void trackLoaded(int nId);
  Q_INVOKABLE void trackUnloaded(int nId);
  Q_INVOKABLE void deleteSelected();
  Q_INVOKABLE void loadSelected();
  Q_INVOKABLE void markAllUnload();
  Q_INVOKABLE void unloadSelected();
  Q_INVOKABLE void trackDelete(int nId);
  Q_INVOKABLE void trackRename(QString sName, int nId);

  TrackModel(QObject* parent = 0);
  ~TrackModel();
  static QObject* m_pRoot;

  enum
  {
    ValueRole = Qt::UserRole + 1,

  };
  QModelIndex index(int, int, const QModelIndex&) const override;
  QModelIndex IndexFromId(int nId) const;
  int rowCount(const QModelIndex& parent) const override;
  int columnCount(const QModelIndex& parent) const override;
  QModelIndex parent(const QModelIndex&) const override;
  QHash<int, QByteArray> roleNames() const override;
  QVariant data(const QModelIndex&, int) const override;
  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

private:
  struct ModelDataNode
  {
    ModelDataNode()
    {
      bSelected = false;
      nId = -1;
      bIsLoaded = false;
    }
    QString sName;
    QString sLength;
    QString sDateTime;
    QString sDuration;
    QString sMaxSpeed;
    QString sDiskSize;
    bool bIsLoaded;
    int nId;
    bool bSelected;
    double lo;
    double la;
    int nType;
    int nTime;
  };

  ModelDataNode GetNodeFromTrack(const QString& sTrackName, bool bIsLoaded);

  void UpdateSelected();
  QVector<ModelDataNode> m_oc;
  int m_nLastId = -1;
};

#endif // INFOLISTMODEL_H
