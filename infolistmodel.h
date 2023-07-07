#ifndef INFOLISTMODEL_H
#define INFOLISTMODEL_H

#include <QAbstractListModel>
#include <QCompass>
#include <QFile>
#include <QGeoCoordinate>
#include <QGeoLocation>
#include <QGeoPositionInfo>
#include <QList>
#include <QObject>

class MssTimer;

class InfoListModel : public QAbstractListModel {
  Q_OBJECT

 public:
  Q_INVOKABLE void klicked2(int row);
  Q_INVOKABLE void klicked1(int row);
  InfoListModel(QObject* parent = 0);
  ~InfoListModel();
  static QObject* m_pRoot;
  enum {
    ValueRole = Qt::UserRole + 1,
    LabelRole = Qt::UserRole + 2,
    UnitRole = Qt::UserRole + 3
  };
 private slots:
  void PositionUpdated(const QGeoPositionInfo& o);
  void CompassReadingChanged();

 private:
  void UpdateOnTimer();
  struct Data {
    Data() {}
    Data(const QString& _sU, const QString& _sL) {
      sU = _sU;
      sL = _sL;
    }
    QString f;
    QString sU;
    QString sL;
  };
  /*
    struct SpeedStruct
    {
        SpeedStruct() {fTimeSec=0;fDistM=0;}
        SpeedStruct(double _fTimeSec,double _fDistM ) {fTimeSec
    =_fTimeSec;fDistM =_fDistM;} double fTimeSec; double fDistM;
    };
*/

  struct SaveStruct {
    double m_fMaxSpeed;
    double m_fDurationSec;
    double m_fDist;
    double m_oStartPosLat;
    double m_oStartPosLong;
  };

  // From QAbstractListModel
  int rowCount(const QModelIndex& parent) const;
  int columnCount(const QModelIndex& parent) const;
  QVariant data(const QModelIndex& index, int role) const;
  QHash<int, QByteArray> roleNames() const;
  QVector<QVector<Data>> m_nData;
  QFile m_pDataFile;

  QCompass m_oCompass;
  MssTimer* m_pTimer;
  void ResetData();
  QGeoCoordinate m_oLastPos;
  //   QList<SpeedStruct> m_ocSpeedVal;
  SaveStruct p;
  //   double m_fGpsSpeedMs;
  double m_fLastTimeSec;
  double m_fLastMidTimeSec;
  double m_fMidDist = 0;
};

#endif  // INFOLISTMODEL_H
