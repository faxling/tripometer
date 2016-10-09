#include <QGeoLocation>
#include <QGeoCoordinate>
#include <QCompass>
#include <QFile>
#include <QStandardPaths>
#include "infolistmodel.h"
#include "QtSensors/QAccelerometer"
#include <QtGlobal>
#include <QGeoPositionInfoSource>
#include <stdio.h>
#include <QDebug>
#include <stdio.h>
#include "Utils.h"
#include "time.h"

QString FormatBearing(double direction )
{
  const char* dirStr;
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
  sprintf(szStr, "%s %.1f",dirStr,direction);
  return szStr;

}

QString FormatBearing(double direction, double fLevel )
{
  char szStr[20];
  sprintf(szStr, "%s : %.1f",FormatBearing(direction).toLatin1().data(),fLevel);
  return szStr;
}

QString FormatCurrentTime()
{
  char szStr[20];
  time_t now = 0 ;
  time(&now);
  // time(&now);
  strftime(szStr, 20, "%H:%M:%S",localtime(&now) );
  return szStr;
}

QString FormatDurationSec(unsigned int nTime)
{
  char szStr[20];
  time_t now = nTime ;

  strftime(szStr, 20, "%H:%M:%S", gmtime(&now));
  return szStr;
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



QString FormatM(double f)
{
  if (f != f)
    return "-";

  char szStr[20];
  sprintf(szStr, "%.1f",f);
  return szStr;
}


QObject* InfoListModel::m_pRoot;


enum TRIP_FIELDS
{
  GPS_SPEED,
  DISTANS,
  DURATION,
  CURRENTTIME,
  TIMEKM,
  ELEVATION,
  MAXSPEED,
  AVERAGE_SPEED,
  DISTANCE_TO_HOME,
  COMPASS,
  BEARING,
  LAT,
  LONG,
  PREC,
  LAST_VAL
};


void InfoListModel::UpdateOnTimer()
{
  double fCurTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
  if (m_pRoot==0)
    return;

  int nUnit = m_pRoot->property("nUnit").toInt();
  if (m_fLastTimeSec != 0.0)
    p.m_fDurationSec += ( fCurTime - m_fLastTimeSec) ;

  m_fLastTimeSec = fCurTime;
  QVector<int> oc;
  oc.push_back(ValueRole);
  // varje sec
  if ((((int)(p.m_fDurationSec * 10)) % 10) == 0)
  {
    m_nData[nUnit][CURRENTTIME].f = FormatCurrentTime();
    emit dataChanged(index(CURRENTTIME), index(CURRENTTIME),oc);

  }
  // var tioned


  if ((((int)(p.m_fDurationSec )) % 10) == 0)
  {
    m_pDataFile.seek(0);
    m_pDataFile.write((char*)&p,sizeof p);
  }

  m_nData[nUnit][DURATION].f = FormatDuration(p.m_fDurationSec*10);
  m_pRoot->setProperty("sDur",FormatDurationSec(p.m_fDurationSec));

  emit dataChanged(index(DURATION), index(DURATION),oc);
}


InfoListModel::~InfoListModel()
{
  delete m_pTimer;
}





InfoListModel::InfoListModel(QObject *parent) :
  QAbstractListModel(parent)
{
  QString sDataFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  QString sDataFileName;
  sDataFileName.sprintf("%ls/%ls",(wchar_t*)sDataFilePath.utf16(),L"fraxtrip.dat");
  m_pDataFile.setFileName(sDataFileName);
  m_pDataFile.open(QIODevice::ReadOnly);

  size_t nSize = m_pDataFile.read((char*)&p,sizeof p);
  m_pDataFile.close();

  if (nSize != sizeof p)
  {
    p.m_fDist =0;
    p.m_fDurationSec = 0;
    p.m_fMaxSpeed = 0;
    p.m_oStartPosLat = 0;
    p.m_oStartPosLong = 0;
  }

  m_pDataFile.open(QIODevice::ReadWrite);

  QGeoPositionInfoSource *source = QGeoPositionInfoSource::createDefaultSource(this);
  connect(&m_oCompass, SIGNAL(readingChanged()), this, SLOT(CompassReadingChanged()));
  m_oCompass.start();
  if (source != nullptr)
  {
    connect(source, SIGNAL(positionUpdated(const QGeoPositionInfo&)),
            this, SLOT(PositionUpdated(const QGeoPositionInfo&)));
    source->startUpdates();
  }
  m_pTimer = new MssTimer();
  m_pTimer->SetTimeOut([this]{UpdateOnTimer();});

  m_pTimer->Start(100);
  m_nData.resize(2);
  m_nData[0].resize(LAST_VAL);
  m_nData[1].resize(LAST_VAL);

  m_nData[0][DISTANS] = Data("km","Distans");
  m_nData[0][TIMEKM] = Data("time","Time/km");
  m_nData[0][DURATION] = Data("time","Duration");
  m_nData[0][BEARING] = Data("deg","Bearing");
  m_nData[0][COMPASS] = Data("deg","Compass");
  m_nData[0][ELEVATION] = Data("m","Elevation");
  m_nData[0][MAXSPEED] = Data("km/h","Max Speed");
  m_nData[0][AVERAGE_SPEED] = Data("km/h","Average Speed");
  m_nData[0][DISTANCE_TO_HOME] = Data("km","Distance To Home");
  m_nData[0][GPS_SPEED] = Data("km/h","Speed");
  m_nData[0][LAT] = Data("","Lat");
  m_nData[0][LONG] = Data("","Long");
  m_nData[0][CURRENTTIME] = Data("time","Current");
  m_nData[0][PREC] = Data("m","Precision");

  m_nData[1][DISTANS] = Data("NM","Distans");
  m_nData[1][TIMEKM] = Data("time","Time/NM");
  m_nData[1][DURATION] = Data("time","Duration");
  m_nData[1][BEARING] = Data("deg","Bearing");
  m_nData[1][COMPASS] = Data("deg","Compass");
  m_nData[1][ELEVATION] = Data("m","Elevation");
  m_nData[1][MAXSPEED] = Data("kts","Max Speed");
  m_nData[1][AVERAGE_SPEED] = Data("kts","Average Speed");
  m_nData[1][DISTANCE_TO_HOME] = Data("NM","Distance To Home");
  m_nData[1][GPS_SPEED] = Data("kts","Speed");
  m_nData[1][LAT] = Data("","Lat");
  m_nData[1][LONG] = Data("","Long");
  m_nData[1][CURRENTTIME] = Data("time","Current");
  m_nData[1][PREC] = Data("m","Precision");

  ResetData();
}

void InfoListModel::ResetData()
{
  m_oLastPos = QGeoCoordinate();
  m_fLastTimeSec = 0;
  for (auto & oJ : m_nData)
    for(auto &oI : oJ)
      oI.f = "-";
}


void InfoListModel::CompassReadingChanged()
{
  double fAz = m_oCompass.reading()->azimuth();
  double fLevel = m_oCompass.reading()->calibrationLevel();
  for (int i = 0; i < 2;++i)
    m_nData[i][COMPASS].f = FormatBearing(fAz,fLevel);

  QVector<int> oc;
  oc.push_back(ValueRole);

  emit dataChanged(index(COMPASS), index(COMPASS),oc);
}

// Share for maep
double g_fMaxSpeed = 0;
void InfoListModel::PositionUpdated(const QGeoPositionInfo& o)
{
  QVector<int> oc;
  oc.push_back(ValueRole);

  bool bHasSpeed =  o.hasAttribute(QGeoPositionInfo::Attribute::GroundSpeed);

  double fPrec = 10000;

  if (o.hasAttribute(QGeoPositionInfo::Attribute::HorizontalAccuracy)==true)
    fPrec = o.attribute(QGeoPositionInfo::Attribute::HorizontalAccuracy);

  for (int i = 0; i < 2;++i)
    m_nData[i][PREC].f = FormatKmH(fPrec);

  emit dataChanged(index(PREC), index(PREC),oc);

  if (m_oLastPos.isValid()==false)
  {

    for (int i = 0; i < 2;++i)
    {
      m_nData[i][LAT].f = FormatLatitude(o.coordinate().latitude());
      m_nData[i][LONG].f = FormatLongitude(o.coordinate().longitude());
    }

    if (bHasSpeed == true)
    {
        m_oLastPos = o.coordinate();
    }

    if (p.m_oStartPosLat == 0 && bHasSpeed == true)
    {

      p.m_oStartPosLat = m_oLastPos.latitude();
      p.m_oStartPosLong = m_oLastPos.longitude();

    }

    emit dataChanged(index(LAT), index(LONG),oc);

    //  double fTimestamp = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    // m_ocSpeedVal.push_back(SpeedStruct(fTimestamp,0));
    return;
  }


  if (fPrec > 20)
    return;

  double fSpeed =  0;


  if ( bHasSpeed == true )
  {
    fSpeed = o.attribute(QGeoPositionInfo::Attribute::GroundSpeed);

    if (g_fMaxSpeed < fSpeed)
    {
        g_fMaxSpeed = fSpeed;
    }

    if (p.m_fMaxSpeed < fSpeed)
    {

      p.m_fMaxSpeed = fSpeed;
    }
  }
  double fStep = m_oLastPos.distanceTo(o.coordinate());

  p.m_fDist+=fStep;


  for (int i = 0; i < 2;++i)
  {
    m_nData[i][LAT].f = FormatLatitude(o.coordinate().latitude());
    m_nData[i][LONG].f = FormatLongitude(o.coordinate().longitude());


    double fFactor = 0;
    double fFactorDist = 0;
    switch (i)
    {
    case 0:
      fFactor = 3.6;
      fFactorDist = 1;

      break;
    case 1:
      fFactor = 1.9438;
      fFactorDist = 1.852;

    }


    if (bHasSpeed == true)
    {
      m_nData[i][GPS_SPEED].f = FormatKmH(fSpeed*fFactor);
      m_nData[i][MAXSPEED].f = FormatKmH(p.m_fMaxSpeed*fFactor);
      if (fabs(fSpeed) > 0.5)
        m_nData[i][TIMEKM].f = FormatDurationSec(1000 * fFactorDist / fSpeed   );
    }


    // if (fStep < 0.5 )
    // {
    //m_nData[0].f = "0";
    //emit dataChanged(index(0), index(0),oc);
    // m_ocSpeedVal.clear();
    //  return;
    // }
    // double fTimestamp = QDateTime::currentMSecsSinceEpoch() / 1000.0;
    //m_ocSpeedVal.push_back(SpeedStruct(fTimestamp, fStep));
    m_nData[i][DISTANCE_TO_HOME].f = FormatKm( QGeoCoordinate(p.m_oStartPosLat,p.m_oStartPosLong).distanceTo(o.coordinate()) / fFactorDist/ 1000.0);

    m_nData[i][DISTANS].f = FormatKm(p.m_fDist / fFactorDist/1000.0);

    //if (m_ocSpeedVal.size() > 10)
    //     m_ocSpeedVal.removeFirst();

    // double fAvgSpeed  = 0;
    //double fTimeSegment = 0;
    // double fDistSegment = 0;

    // for ( auto oI = m_ocSpeedVal.begin()+1; oI != m_ocSpeedVal.end() ; ++oI)
    //{
    //    fTimeSegment+= (oI->fTimeSec - (oI - 1)->fTimeSec);
    //    fDistSegment+= oI->fDistM;
    //}

    //if (fabs(fTimeSegment) > 0.1)
    //   fAvgSpeed = fDistSegment / fTimeSegment ;


    if (o.hasAttribute(QGeoPositionInfo::Attribute::Direction)==true)
      m_nData[i][BEARING].f =  FormatBearing(o.attribute(QGeoPositionInfo::Attribute::Direction));
    else
      m_nData[i][BEARING].f = "-";

    //m_nData[SPEED].f = FormatKmH(fAvgSpeed*3.6);
    if (p.m_fDurationSec != 0)
      m_nData[i][AVERAGE_SPEED].f = FormatKmH(( p.m_fDist /  p.m_fDurationSec) *fFactor);

    m_nData[i][ELEVATION].f = FormatM(o.coordinate().altitude());
  }
  m_oLastPos = o.coordinate();
  //qDebug() << m_fLastSpeed;
  // qDebug(QString("Speed %1").arg(fSpeed));
  emit dataChanged(index(0), index(LAST_VAL-1),oc);

}


int InfoListModel::rowCount(const QModelIndex &) const
{
  return m_nData[0].size();
}

int InfoListModel::columnCount(const QModelIndex &) const
{
  return 1;
}

QVariant InfoListModel::data(const QModelIndex &index, int role) const
{
  /*
    if (ValueRole != role && UnitRole != role)
        return QVariant();
*/



  int nR = index.row();
  int nUnit = 0;
  if (m_pRoot==0)
    return  QVariant("-");

  nUnit = m_pRoot->property("nUnit").toInt();

  if (nR < 0 || nR >= m_nData[0].size())
    return QVariant("-");

  switch (role)
  {
  case ValueRole:
    return m_nData[nUnit][nR].f;
  case UnitRole:
    return m_nData[nUnit][nR].sU;
  case LabelRole:
    return m_nData[nUnit][nR].sL;
  }

  return QVariant("");


}

void InfoListModel::klicked1(int)
{

}

void InfoListModel::klicked2(int nCmd)
{

  // Pause
  if (nCmd == 1)
  {
    if (m_pTimer->IsActive()  == true)
    {
      m_pRoot->setProperty("bIsPause",true);
      m_pTimer->Stop();
    }
    else
    {
      double fCurTime = QDateTime::currentMSecsSinceEpoch() / 1000.0;
      m_fLastTimeSec = fCurTime;
      m_pRoot->setProperty("bIsPause",false);
      m_pTimer->Start(100);
    }
    return;
  }
  QVector<int> oc;
  oc.push_back(ValueRole);
  if (nCmd == 2)
  {
    int nUnit = m_pRoot->property("nUnit").toInt();
    m_nData[nUnit][MAXSPEED].f = "-";
    emit dataChanged(index(MAXSPEED), index(MAXSPEED),oc);
    p.m_fMaxSpeed = 0;
    return;
  }
  if (nCmd == 3)
  {
    p.m_oStartPosLat = 0;
    p.m_oStartPosLong = 0;
    p.m_fDist = 0;
    p.m_fDurationSec = 0;
    p.m_fMaxSpeed = 0;

    ResetData();

    oc.push_back(UnitRole);
    oc.push_back(LabelRole);
    emit dataChanged(index(0), index(LAST_VAL-1),oc);
  }

  if (nCmd == 4)
  {
    ScreenOn(true);
  }

  if (nCmd == 5)
  {
    oc.push_back(UnitRole);
    oc.push_back(LabelRole);
    emit dataChanged(index(0), index(LAST_VAL-1),oc);
  }
}

QHash<int, QByteArray> InfoListModel::roleNames() const
{
  QHash<int, QByteArray> roleNames;
  roleNames.insert(ValueRole, "aValue");
  roleNames.insert(LabelRole, "aLabel");
  roleNames.insert(UnitRole, "aUnit");
  return roleNames;
}
