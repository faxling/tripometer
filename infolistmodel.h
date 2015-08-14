#ifndef INFOLISTMODEL_H
#define INFOLISTMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include <QGeoLocation>
#include <QGeoCoordinate>
#include <QGeoPositionInfo>
#include <QList>
#include <stdio.h>

class MssTimer;




class InfoListModel : public QAbstractListModel
{
    Q_OBJECT

public:
    Q_INVOKABLE void klicked2(int row);
    Q_INVOKABLE void klicked1(int row);
    InfoListModel(QObject *parent = 0);
    ~InfoListModel();
    static QObject* m_pRoot;
    enum {
        ValueRole = Qt::UserRole +1,
        LabelRole = Qt::UserRole +2,
        UnitRole = Qt::UserRole +3
    };
private slots:
    void PositionUpdated(const QGeoPositionInfo& o);


private:

    void UpdateOnTimer();
    struct Data
    {
        Data() {}
        QString f;
        QString sU;
        QString sL;
    };

    struct SpeedStruct
    {
        SpeedStruct() {fTimeSec=0;fDistM=0;}
        SpeedStruct(double _fTimeSec,double _fDistM ) {fTimeSec =_fTimeSec;fDistM =_fDistM;}
        double fTimeSec;
        double fDistM;
    };


    struct SaveStruct
    {
        double m_fMaxSpeed;
        double m_fDurationSec;
        double m_fDist;
        double m_oStartPosLat;
        double m_oStartPosLong;
    };


    //From QAbstractListModel
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    QHash<int, QByteArray> roleNames() const;
    QVector<Data> m_nData;
    FILE* m_pDataFile;



    MssTimer* m_pTimer;
    void ResetData();
    QGeoCoordinate m_oLastPos;
    QList<SpeedStruct> m_ocSpeedVal;
    SaveStruct p;
 //   double m_fGpsSpeedMs;
    double m_fLastTimeSec;

};

#endif // INFOLISTMODEL_H
