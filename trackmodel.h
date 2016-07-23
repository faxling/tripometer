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


  Q_INVOKABLE void trackLoaded(int nId);
  Q_INVOKABLE void trackUnloaded(int nId);
  TrackModel(QObject *parent = 0);
  ~TrackModel();
  static QObject* m_pRoot;

  enum {
    ValueRole = Qt::UserRole +1,
    
  };
  QModelIndex index(int, int, const QModelIndex&) const;
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QModelIndex parent(const QModelIndex&) const;
  QHash<int, QByteArray> roleNames() const;
  QVariant data(const QModelIndex&, int) const;
private slots:
  void TrackAdded(QString);

private:
  struct ModelDataNode
  {
    QString sName;
    bool bIsLoaded;
    int nId;
  };

  QVector<ModelDataNode> m_oc;
  int m_nLastId;

};

#endif // INFOLISTMODEL_H
