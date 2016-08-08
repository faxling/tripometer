#ifndef UTILS_H
#define UTILS_H

#include <functional>

#include <QObject>
#include <QVector>

void ScreenOn(bool b);

QString FormatLatitude(double fLatitude);
QString FormatLongitude(double fLongitude);
QString FormatKm(double f);
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
QString PointFullName(const QString& sTrackName);
QString GpxDatFullName(const QString& sTrackName);
QString GpxFullName(const QString& sTrackName);

template <class T>
int IndexOf(const T& o, const QVector<T>& oc)
{
  if (oc.size() <= 0)
    return 0;
  return ((&o - &oc[0]));
}

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
