#include "Utils.h"

#include <QBasicTimer>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QStandardPaths>

MssTimer::MssTimer()
{
    m_pTimer = new QBasicTimer();
    m_bIsSingleShot = false;
}

MssTimer::MssTimer(std::function<void ()> pfnTimeOut)
{
  m_pTimer = new QBasicTimer;
  m_pfnTimeOut =  pfnTimeOut;
  m_bIsSingleShot = false;
  // connect(m_pTimer,SIGNAL(timeout()),this,SLOT(TimeOut()));
}


MssTimer::~MssTimer()
{
    delete m_pTimer;
}

void MssTimer::Start(int nMilliSec)
{
  m_bIsSingleShot = false;
  m_pTimer->start(nMilliSec,this);
}


void MssTimer::SingleShot(int nMilliSec)
{
  m_bIsSingleShot = true;
  if (m_pTimer->isActive()==true)
    m_pTimer->stop();
  m_pTimer->start(nMilliSec,this);
}


void MssTimer::Stop()
{
  m_pTimer->stop();
}

bool MssTimer::IsActive() {
  return m_pTimer->isActive();
}

void MssTimer::timerEvent(QTimerEvent *)
{
  if (m_pfnTimeOut!= nullptr)
    m_pfnTimeOut();

  if (m_bIsSingleShot == true)
    m_pTimer->stop();
}

QString GpxDatFullName(const QString& sTrackName)
{
  QString sDataFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  QString sGpxFileName;
  sGpxFileName.sprintf("%ls/%ls.dat",(wchar_t*)sDataFilePath.utf16(),(wchar_t*)sTrackName.utf16());

  return sGpxFileName;
}

QString GpxFullName(const QString& sTrackName)
{
  QString sDataFilePath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  QString sGpxFileName;
  sGpxFileName.sprintf("%ls/%ls.gpx",(wchar_t*)sDataFilePath.utf16(),(wchar_t*)sTrackName.utf16());

  return sGpxFileName;
}

QString FormatKm(double f)
{
  char szStr[20];
  sprintf(szStr, "%.3f",f);
  return szStr;
}

QString FormatLatitude(double fLatitude)
{

  int nDegrees = abs(static_cast<int>(fLatitude));
  double fMinutes = 60.0 * (fabs(fLatitude) - nDegrees);

  //Check if we have overflow
  if (fMinutes >= 59.995)
  {
    fMinutes = 0.0;
    nDegrees++;
  }

  char cLat('N');
  if (fLatitude < 0.0)
  {
    cLat = 'S';
  }

  char szStr[20];
  sprintf(szStr, "%c%02d\xb0 %.2f'",cLat,nDegrees,fMinutes);
  return QString::fromLatin1(szStr);

  //   return QString(QLatin1String("%1%2\xB0%3'")).arg(cLat).arg(nDegrees, 2, 10, QChar('0')).arg(fMinutes, 5, 'f', 2, QChar('0'));
}



QString FormatLongitude(double fLongitude)
{

  int nDegrees = abs(static_cast<int>(fLongitude));
  double fMinutes = 60.0 * (fabs(fLongitude) - nDegrees);

  //Check if we have overflow
  if (fMinutes >= 59.995)
  {
    fMinutes = 0.0;
    nDegrees++;
  }

  char cLong('E');
  if (fLongitude < 0.0)
  {
    cLong = 'W';
  }

  char szStr[20];

  sprintf(szStr, "%c%03d\xb0 %.2f'",cLong,nDegrees,fMinutes);
  return QString::fromLatin1(szStr);

  //   return QString(QLatin1String("%1%2\xB0%3'")).arg(cLong).arg(nDegrees, 3, 10, QChar('0')).arg(fMinutes, 5, 'f', 2, QChar('0'));
}


QRegExp& SLASH = *new QRegExp("[\\\\/]");

QString DirName(const QString & sFileName) {
  int n  = sFileName.lastIndexOf(SLASH);
  if (n < 0)
    return sFileName;
  return sFileName.left(n);
}

QString JustFileNameNoExt(const QString & sFileName) {
  return BaseName(JustFileName(sFileName));
}


QString JustFileName(const QString & sFileName) {
  int n  = sFileName.lastIndexOf(SLASH);
  if (n < 0)
    return sFileName;
  return sFileName.right(sFileName.size() - n -1);
}


QString BaseName(const QString & sFileName) {
  return sFileName.left(sFileName.lastIndexOf('.'));
}

QString Ext(const QString & sFileName) {
  return sFileName.right(sFileName.size() - sFileName.lastIndexOf('.') -1 );
}


void ScreenOn(bool b)
{
    QDBusConnection system = QDBusConnection::connectToBus(QDBusConnection::SystemBus,
                                                           "system");
    QDBusInterface interface("com.nokia.mce",
                             "/com/nokia/mce/request",
                             "com.nokia.mce.request",
                             system);


    if (b == true)
    {

      // interface.call( "req_display_state_dim");
        interface.call("req_display_state_on");
        interface.call("req_display_blanking_pause");
    }
    else
        interface.call("req_display_cancel_blanking_pause");

  //   QDBusConnection::disconnectFromBus("system");
}


