#ifndef UTILS_H
#define UTILS_H

#include "src/track.h"
#include <QAbstractItemModel>
#include <QImage>
#include <QObject>
#include <QOrientationReading>
#include <QOrientationSensor>
#include <QQuickImageProvider>
#include <QQuickPaintedItem>
#include <QUrl>
#include <QVector>
#include <functional>
void ScreenOn(bool b);
QString FormatKmH(double f);
QString FormatDuration(unsigned int nTime);
QString FormatDateTime(unsigned int nTime);
QString FormatLatitude(double fLatitude);
QString FormatLongitude(double fLongitude);
QString FormatKm(double f);
QString FormatNrBytes(int nBytes);
// C:/user/foo.txt
// C:/user
QString DirName(const QString& sFileName);
// foo.txt
QString JustFileName(const QString& sFileName);
// foo
QString JustFileNameNoExt(const QString& sFileName);
// C:/user/foo
QString BaseName(const QString& sFileName);
// .txt
QString Ext(const QString& sFileName);

QString operator^(const QString& s, const QString& s2);

QString CacheDir();
QString StorageDir();
QString GpxNewName(const QString& sTrackName, int nCount = -1);
QString GpxDatFullName(const QString& sTrackName);
QString GpxFullName(const QString& sTrackName);

MarkData GetMarkData(const QString& sTrackName);
void WriteMarkData(const QString& sTrackName, MarkData& t);

template <class T> int IndexOf(const T& o, const QVector<T>& oc)
{
  if (oc.size() <= 0)
    return 0;
  return ((&o - &oc[0]));
}

class QBasicTimer;

class QElapsedTimer;
class StopWatch
{
public:
  // Use %1 for time
  StopWatch(const QString& sMsg);
  StopWatch();
  ~StopWatch();
  void Pause();
  void Continue();
  void Stop();
  double StopTimeSec();

private:
  bool m_bMsgPrinted = false;
  QString m_sMsg;
  QElapsedTimer* m_oTimer;
};

class MssTimer : public QObject
{
public:
  MssTimer(std::function<void()> pfnTimeOut);
  MssTimer();
  ~MssTimer();
  void SetTimeOut(std::function<void()> pfnTimeOut) { m_pfnTimeOut = pfnTimeOut; }
  void Start(int nMilliSec);
  void SingleShot(int nMilliSec);
  void Stop();
  bool IsActive();

private:
  void timerEvent(QTimerEvent* pEvent);
  QBasicTimer* m_pTimer;
  std::function<void()> m_pfnTimeOut;
  bool m_bIsSingleShot;
};

namespace mssutils
{
  template <class T> int IndexOf(const T& o, const QVector<T>& oc)
  {
    if (oc.size() <= 0)
      return 0;
    return ((&o - &oc[0]));
  }

  template <class T> int IndexOf(const T& o, const std::vector<T>& oc)
  {
    if (oc.size() <= 0)
      return 0;
    return ((&o - &oc[0]));
  }
  void MkCache();
  QString Hash(const QString& s);

} // namespace mssutils

// ImageThumb maker
class CaptureThumbMaker : public QObject
{
  Q_OBJECT
public:
  static int HEIGHT();
  static int WIDTH();
  bool HasSelectedCapture();
  CaptureThumbMaker(QObject* parent = 0);

  // The captured image height/width
  Q_PROPERTY(int HEIGHT READ HEIGHT CONSTANT)
  Q_PROPERTY(int WIDTH READ WIDTH CONSTANT)
  Q_PROPERTY(bool HasSelectedCapture READ HasSelectedCapture NOTIFY hasSelectedCaptureChanged)
  Q_INVOKABLE void save(QString s, int nOrientation);
  Q_INVOKABLE QUrl name(QString s);
signals:
  void hasSelectedCaptureChanged();
};

class FileMgr : public QObject
{
  Q_OBJECT
public:
  FileMgr();
  Q_INVOKABLE void remove(QString s);
  Q_INVOKABLE QString renameToAscii(QString s);
  Q_INVOKABLE void clearCache();
  Q_INVOKABLE QString getUsedCache();
};

class QQuickView;

class ScreenCapturedImg : public QQuickImageProvider
{
public:
  ScreenCapturedImg();
  QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override;
};

class ScreenCapture : public QQuickPaintedItem
{
  Q_OBJECT
public:
  static void SetView(QQuickView* parent);
  Q_INVOKABLE void setPageAndModel(QObject* pPage, QObject* pModel, int nIndexM);
  bool IsSelected = false;
  ScreenCapture();
  ~ScreenCapture();

  // saves orientation
  Q_INVOKABLE void capture();
  Q_INVOKABLE void save();

  void paint(QPainter* ppainter) override;
  friend class ScreenCapturedImg;

private:
  void StartBusyInd();
  void StopBusyInd();

  QObject* m_pPage = nullptr;
  QObject* m_pModel = nullptr;
  int m_nIndex = -1;
  QImage m_oImagePreview;
  QImage m_oImage;
  int m_nOrientation = 0;
};

class MssListModel : public QAbstractItemModel
{
  Q_OBJECT
public:
  using ModelDataType = QVector<QVector<QVariant>>;
  using InitFunc = std::function<void(ModelDataType*)>;

  void Init(int nInstanceNr);
  void Init(InitFunc pfInit, int nInstanceNr);
  void Reset(InitFunc pfInit);

  static MssListModel* Instance(int nInstanceNr);
  template <typename... Args> MssListModel(const Args&... args)
  {
    QVector<const char*> vec = {args...};
    for (auto& oI : vec)
    {
      m_ocRole.insert(Qt::UserRole + mssutils::IndexOf(oI, vec), oI);
    }
  }

  Q_INVOKABLE void updateItem(int nRow, int nCol, const QVariant& value);
  Q_INVOKABLE void clearAll();
  Q_INVOKABLE void removeRow(int nRow);
  int AddRow(const QVector<QVariant>& ocRow);
  QVariant virtual data(const QModelIndex& index, int role) const override;
  int FindRow(const QVariant&, int nCol);
  QVariant Data(int nRow, int nUserRole) const
  {
    return (data(index(nRow, 0), Qt::UserRole + nUserRole));
  }
  QHash<int, QByteArray> roleNames() const override;
  int rowCount(const QModelIndex&) const override;
  int columnCount(const QModelIndex&) const override;
  QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex&) const override;

protected:
  //   Rows values indexed by roles - Qt::UserRole::
  ModelDataType m_ocRows;
  QHash<int, QByteArray> m_ocRole;
};
#endif // UTILS_H
