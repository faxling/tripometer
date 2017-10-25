#ifndef TRACKMODEL_H
#define TRACKMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include <QGeoLocation>
#include <QGeoCoordinate>
#include <QGeoPositionInfo>
#include <QCompass>
#include <QList>
#include <stdio.h>

class MssTimer;


class TrackModel : public QAbstractItemModel
{
  Q_OBJECT



public:

  Q_INVOKABLE int  nextId();
  Q_INVOKABLE void trackAdd(const QString& sName);
  Q_INVOKABLE void trackCenter(int nId);
  Q_INVOKABLE void trackLoaded(int nId);
  Q_INVOKABLE void trackUnloaded(int nId);
  Q_INVOKABLE void deleteSelected();
  Q_INVOKABLE void loadSelected();
  Q_INVOKABLE void markAllUnload();
  Q_INVOKABLE void unloadSelected();
  Q_INVOKABLE void trackDelete(int nId);
  Q_INVOKABLE void trackRename(QString sName, int nId);
  TrackModel(QObject *parent = 0);
  ~TrackModel();
  static QObject* m_pRoot;

  enum {
    ValueRole = Qt::UserRole +1,
    
  };
  QModelIndex index(int, int, const QModelIndex&) const;
  QModelIndex IndexFromId(int nId) const;
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QModelIndex parent(const QModelIndex&) const;
  QHash<int, QByteArray> roleNames() const;
  QVariant data(const QModelIndex&, int) const;

  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);


private:
  struct ModelDataNode
  {
    ModelDataNode() {bSelected=false; nId = -1; bIsLoaded = false;}
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

  QVector<ModelDataNode> m_oc;
  int m_nLastId;

};

#endif // INFOLISTMODEL_H
