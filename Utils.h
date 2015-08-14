#ifndef UTILS_H
#define UTILS_H

#include <functional>

#include <QObject>


class QBasicTimer;
class MssTimer : public QObject
{
public:
  MssTimer(std::function<void()> pfnTimeOut );
  MssTimer();
  ~MssTimer();
  void SetTimeOut(std::function<void ()> pfnTimeOut) { m_pfnTimeOut = pfnTimeOut;};
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
