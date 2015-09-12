#include "Utils.h"

#include <QBasicTimer>
#include <QDBusConnection>
#include <QDBusInterface>


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


void ScreenOn(bool b)
{
    QDBusConnection system = QDBusConnection::connectToBus(QDBusConnection::SystemBus,
                                                           "system");
    QDBusInterface interface("com.nokia.mce",
                             "/com/nokia/mce/request",
                             "com.nokia.mce.request",
                             system);


    if (b == true)
        interface.call("req_display_blanking_pause");
    else
        interface.call("req_display_cancel_blanking_pause");

  //   QDBusConnection::disconnectFromBus("system");
}


