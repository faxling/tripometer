#ifndef UTILS_H
#define UTILS_H

#include <functional>

#include <QObject>


void ScreenOn(bool b);

QString FormatLatitude(double fLatitude);
QString FormatLongitude(double fLongitude);

// C:/user/foo.txt
// C:/user
QString DirName(const QString & sFileName);
// foo.txt
QString JustFileName(const QString & sFileName);
// foo
QString JustFileNameNoExt(const QString & sFileName);
// C:/user/foo
QString BaseName(const QString & sFileName);
// txt
QString Ext(const QString & sFileName);

class QBasicTimer;
class MssTimer : public QObject
{
public:
  MssTimer(std::function<void()> pfnTimeOut );
  MssTimer();
  ~MssTimer();
  void SetTimeOut(std::function<void ()> pfnTimeOut) { m_pfnTimeOut = pfnTimeOut;}
  void Start(int nMilliSec);
  void SingleShot(int nMilliSec);
  void Stop();
  bool IsActive();




private:

  void timerEvent(QTimerEvent *pEvent);
  QBasicTimer* m_pTimer;
  std::function<void ()> m_pfnTimeOut;

  bool m_bIsSingleShot;
};

#endif // UTILS_H
