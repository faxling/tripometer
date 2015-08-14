#include <QGeoLocation>
#include <QGeoCoordinate>
#include "infolistmodel.h"
#include "QtSensors/QAccelerometer"
#include <QtGlobal>
#include <QGeoPositionInfoSource>
#include <stdio.h>
#include <QDebug>
#include <QStandardPaths>
#include <stdio.h>
#include "Utils.h"

QString FormatBearing(double direction)
{
    QString dirStr;
    if (direction < 11.25) {
        dirStr = "N";
    } else if (direction < 33.75) {
        dirStr = "NNE";
    } else if (direction < 56.25) {
        dirStr = "NE";
    } else if (direction < 78.75) {
        dirStr = "ENE";
    } else if (direction < 101.25) {
        dirStr = "E";
    } else if (direction < 123.75) {
        dirStr = "ESE";
    } else if (direction < 146.25) {
        dirStr = "SE";
    } else if (direction < 168.75) {
        dirStr = "SSE";
    } else if (direction < 191.25) {
        dirStr = "S";
    } else if (direction < 213.75) {
        dirStr = "SSW";
    } else if (direction < 236.25) {
        dirStr = "SW";
    } else if (direction < 258.75) {
        dirStr = "WSW";
    } else if (direction < 281.25) {
        dirStr = "W";
    } else if (direction < 303.75) {
        dirStr = "WNW";
    } else if (direction < 326.25) {
        dirStr = "NW";
    } else if (direction < 348.75) {
        dirStr = "NNW";
    } else if (direction < 360) {
        dirStr = "N";
    } else {
        dirStr = "?";
    }

    char szStr[20];
    sprintf(szStr, "%s %.1f",dirStr.toLatin1().data(),direction);
    return szStr;

}
QString FormatDurationSec(unsigned int nTime)
{
    char szStr[20];
    time_t now = nTime ;

    strftime(szStr, 20, "%H:%M:%S", gmtime(&now));



    return szStr;
}

QString FormatDuration(unsigned int nTime)
{
    char szStr[20];
    time_t now = nTime / 10;

    strftime(szStr, 20, "%H:%M:%S", gmtime(&now));
    char szStr2[20];
    sprintf(szStr2,"%s.%d",szStr,nTime%10);


    return szStr2;
}

QString FormatAcc(double f)
{
    char szStr[20];
    sprintf(szStr, "%04d",(int)(f*100));
    return szStr;

}

QString FormatPos(double f)
{
    char szStr[20];
    sprintf(szStr, "%.5f",f);
    return szStr;
}

QString FormatKm(double f)
{
    char szStr[20];
    sprintf(szStr, "%.3f",f);
    return szStr;
}

QString FormatKmH(double f)
{
    char szStr[20];
    sprintf(szStr, "%.1f",f);
    return szStr;
}
QString FormatM(double f)
{
    if (f != f)
        return "-";

    char szStr[20];
    sprintf(szStr, "%.1f",f);
    return szStr;
}


QObject* InfoListModel::m_pRoot;

void InfoListModel::UpdateOnTimer()
{
    double fCurTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;

    if (m_fLastTimeSec != 0.0)
        p.m_fDurationSec += ( fCurTime - m_fLastTimeSec) ;

    m_fLastTimeSec = fCurTime;

    if ((((int)(p.m_fDurationSec * 10)) % 10) == 0)
    {
        fseek(m_pDataFile,0,SEEK_SET );
         fwrite((void*)&p,1,sizeof p,m_pDataFile);
        //  qDebug() << "fwrite " << n;
    }
    m_nData[3].f = FormatDuration(p.m_fDurationSec*10);
    m_pRoot->setProperty("sDur",FormatDurationSec(p.m_fDurationSec));
    QVector<int> oc;
    oc.push_back(ValueRole);
    emit dataChanged(index(3), index(3),oc);
}


InfoListModel::~InfoListModel()
{



    delete m_pTimer;
}


InfoListModel::InfoListModel(QObject *parent) :
    QAbstractListModel(parent)
{
    QString sDataFileName = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    sDataFileName+="/fraxtrip.dat";
    qDebug() << sDataFileName;
    m_pDataFile = fopen(sDataFileName.toLatin1(),"r");
    size_t nSize = fread(&p,1,sizeof p,m_pDataFile);
    if ( m_pDataFile != 0 )
        fclose(m_pDataFile);
    if (nSize != sizeof p)
    {
        p.m_fDist =0;
        p.m_fDurationSec = 0;
        p.m_fMaxSpeed = 0;
        p.m_oStartPosLat = 0;
        p.m_oStartPosLong = 0;
    }

    m_pDataFile = fopen(sDataFileName.toLatin1(),"w+");

    QGeoPositionInfoSource *source = QGeoPositionInfoSource::createDefaultSource(this);

    if (source != nullptr)
    {
        connect(source, SIGNAL(positionUpdated(const QGeoPositionInfo&)),
                this, SLOT(PositionUpdated(const QGeoPositionInfo&)));
        source->startUpdates();
    }
    m_pTimer = new MssTimer();
    m_pTimer->SetTimeOut([this]{UpdateOnTimer();});

    m_pTimer->Start(100);

    m_nData.resize(12);


    m_nData[0].sU = "km/h";
    m_nData[1].sU = "km";
    m_nData[2].sU = "time";
    m_nData[3].sU = "time";
    m_nData[4].sU = "deg";
    m_nData[5].sU = "m";
    m_nData[6].sU = "deg";
    m_nData[7].sU = "deg";
    m_nData[8].sU = "km/h";
    m_nData[9].sU = "km/h";
    m_nData[10].sU = "km";
    m_nData[11].sU = "km/h";

    m_nData[0].sL = "Speed";
    m_nData[1].sL = "Distans";
    m_nData[2].sL = "Time/km";
    m_nData[3].sL = "Duration";
    m_nData[4].sL = "Bearing";
    m_nData[5].sL = "Elevation";
    m_nData[6].sL = "Lat";
    m_nData[7].sL = "Long";
    m_nData[8].sL = "Max Speed";
    m_nData[9].sL = "Average Speed";
    m_nData[10].sL = "Distance To Home";
    m_nData[11].sL = "Gps speed";
    ResetData();
}

void InfoListModel::ResetData()
{
    m_ocSpeedVal.clear();
    m_oLastPos = QGeoCoordinate();
    m_fLastTimeSec = 0;
    for(auto &oI : m_nData)
        oI.f = "-";
}

void InfoListModel::PositionUpdated(const QGeoPositionInfo& o)
{
    QVector<int> oc;
    oc.push_back(ValueRole);
    m_nData[6].f = FormatPos(o.coordinate().latitude());
    m_nData[7].f = FormatPos(o.coordinate().longitude());
    if (m_oLastPos.isValid()==false)
    {
        m_oLastPos = o.coordinate();
        if (p.m_oStartPosLat == 0)
        {
            p.m_oStartPosLat = m_oLastPos.latitude();
            p.m_oStartPosLong = m_oLastPos.longitude();
        }

        emit dataChanged(index(6), index(7),oc);
        double fTimestamp = QDateTime::currentMSecsSinceEpoch() / 1000.0;
        m_ocSpeedVal.push_back(SpeedStruct(fTimestamp,0));
        return;
    }

    if (o.hasAttribute(QGeoPositionInfo::Attribute::GroundSpeed) == true)
        m_nData[11].f = FormatKmH(o.attribute(QGeoPositionInfo::Attribute::GroundSpeed));

    double fStep = m_oLastPos.distanceTo(o.coordinate());
    // if (fStep < 0.5 )
    // {
    //m_nData[0].f = "0";
    //emit dataChanged(index(0), index(0),oc);
    // m_ocSpeedVal.clear();
    //  return;
    // }
    double fTimestamp = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    m_ocSpeedVal.push_back(SpeedStruct(fTimestamp, fStep));
    m_nData[10].f = FormatKm( QGeoCoordinate(p.m_oStartPosLat,p.m_oStartPosLong).distanceTo(o.coordinate()) / 1000.0);


    p.m_fDist+=fStep;
    m_nData[1].f = FormatKm(p.m_fDist/1000.0);

    if (m_ocSpeedVal.size() > 10)
        m_ocSpeedVal.removeFirst();

    double fAvgSpeed  = 0;
    double fTimeSegment = 0;
    double fDistSegment = 0;

    for ( auto oI = m_ocSpeedVal.begin()+1; oI != m_ocSpeedVal.end() ; ++oI)
    {
        fTimeSegment+= (oI->fTimeSec - (oI - 1)->fTimeSec);
        fDistSegment+= oI->fDistM;
    }

    if (fabs(fTimeSegment) > 0.1)
        fAvgSpeed = fDistSegment / fTimeSegment ;



    if (p.m_fMaxSpeed < fAvgSpeed)
        p.m_fMaxSpeed = fAvgSpeed;

    m_nData[8].f = FormatKmH(p.m_fMaxSpeed*3.6);

    if (fAvgSpeed > 0.5)
    {
        m_nData[4].f = FormatBearing(m_oLastPos.azimuthTo(o.coordinate()));
        m_nData[2].f = FormatDuration(10000 / fAvgSpeed);
    }
    else
    {
        m_nData[2].f = "-";
        m_nData[4].f = "-";
    }



    m_nData[0].f = FormatKmH(fAvgSpeed*3.6);
    if (p.m_fDurationSec != 0)
        m_nData[9].f = FormatKmH(( p.m_fDist /  p.m_fDurationSec) * 3.6);
    m_nData[5].f = FormatM(o.coordinate().altitude());

    m_oLastPos = o.coordinate();
    //qDebug() << m_fLastSpeed;
    // qDebug(QString("Speed %1").arg(fSpeed));
    emit dataChanged(index(0), index(11),oc);

}


int InfoListModel::rowCount(const QModelIndex &) const
{
    return m_nData.size();
}

QVariant InfoListModel::data(const QModelIndex &index, int role) const
{
    /*
    if (ValueRole != role && UnitRole != role)
        return QVariant();
*/

    int nR = index.row();
    if (nR < 0 || nR >= m_nData.size())
        return QVariant();

    switch (role)
    {
    case ValueRole:
        return m_nData[nR].f;
    case UnitRole:
        return m_nData[nR].sU;
    case LabelRole:
        return m_nData[nR].sL;
    }

    return QVariant();


}

void InfoListModel::klicked1(int)
{

}

void InfoListModel::klicked2(int row)
{
    if (row == 3)
    {
        if (m_pTimer->IsActive()  == true)
        {
            m_pRoot->setProperty("bIsPause",true);
            m_pTimer->Stop();
        }
        else
        {
            m_pRoot->setProperty("bIsPause",false);
            m_pTimer->Start(100);
        }
        return;
    }
    QVector<int> oc;
    oc.push_back(ValueRole);
    if (row == 8)
    {
        m_nData[8].f = "-";
        emit dataChanged(index(8), index(8),oc);
        p.m_fMaxSpeed = 0;
        return;
    }
    if (row != 1)
        return;
    p.m_oStartPosLat = 0;
    p.m_oStartPosLong = 0;
    p.m_fDist = 0;
    p.m_fDurationSec = 0;
    p.m_fMaxSpeed = 0;

    ResetData();


    emit dataChanged(index(0), index(11),oc);

}

QHash<int, QByteArray> InfoListModel::roleNames() const
{
    QHash<int, QByteArray> roleNames;
    roleNames.insert(ValueRole, "aValue");
    roleNames.insert(LabelRole, "aLabel");
    roleNames.insert(UnitRole, "aUnit");
    return roleNames;
}
