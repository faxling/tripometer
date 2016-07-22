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


  Q_INVOKABLE void trackLoaded(QString sTrack);
  Q_INVOKABLE void tracksUnloaded(QString sTrack);
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
   
  QStringList oc; 
  QVector<bool> ocBool;
};

#endif // INFOLISTMODEL_H
