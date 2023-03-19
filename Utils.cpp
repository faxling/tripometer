#include <math.h>

#include "Utils.h"
#include <QBasicTimer>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QPixmap>
#include <QQuickItemGrabResult>
#include <QQuickView>
#include <QStandardPaths>

QRegExp& SLASH = *new QRegExp("[\\\\/]");

QString operator^(const QString& sIn, const QString& s2In)
{
  QString s(sIn), s2(s2In);
  int nLen1 = s.length() - 1;
  int nLen2 = s2.length() - 1;
  bool bIsBack = true;

  // use  the last dir separator if we need to append
  int nSP = sIn.indexOf(SLASH);
  if (nSP >= 0)
    if (sIn[nSP] == '/')
      bIsBack = false;

  if (nLen1 == -1 && nLen1 == -2)
  {
    return "";
  }

  if (nLen2 == -1)
  {
    if (s[nLen1] == '\\' || s[nLen1] == '/')
      s.remove(nLen1, 1);

    return s;
  }

  if (nLen1 == -1)
    return s2;

  if (s[nLen1] == '\\' || s[nLen1] == '/')
    s.remove(nLen1, 1);

  if (s2[0] == '\\' || s[nLen1] == '/')
    s2.remove(0, 1);

  if (bIsBack == true)
    return s + "\\" + s2;
  else
    return s + "/" + s2;
}

void mssutils::MkCache()
{
  QString sCasheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  QDir o(sCasheDir);
  if (o.exists() == false)
  {
    o.mkpath(sCasheDir);
  }
}

QString mssutils::Hash(const QString& s)
{
  unsigned long long h = 0, g;
  for (auto ik : s)
  {
    h = (h << 4) + ik.unicode(); // shift h 4 bits left, add in ki
    g = h & 0xf000000000000000;  // get the top 4 bits of h
    if (g != 0)
    {
      // if the top 4 bits aren't zero,
      h = h ^ (g >> 56); //   move them to the low end of h
      h = h ^ g;
    }
  }

  QString sRet;

  sRet.sprintf("%016llx", h);
  QString sCasheDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
  return (sCasheDir ^ sRet) + Ext(s);
}

QQuickView* currentView_ = nullptr;
ScreenCapture* screenCapture_ = nullptr;

void ScreenCapture::SetView(QQuickView* parent)
{
  currentView_ = parent;
}

ScreenCapture::ScreenCapture()
{
}

ScreenCapture::~ScreenCapture()
{
  qDebug() << "~S";
}

void ScreenCapture::save()
{
  if (this->IsSelected)
  {
    this->IsSelected = false;
    update();
    return;
  }
  if (screenCapture_ != nullptr)
  {
    screenCapture_->IsSelected = false;
    screenCapture_->update();
  }
  screenCapture_ = this;
  screenCapture_->IsSelected = true;
  screenCapture_->update();
}

void ScreenCapture::saveImgIfSelected()
{
  if (IsSelected)
  {
    QDateTime oNow(QDateTime::currentDateTime());
    QString sImgkName = oNow.toString("yyyy-MM-dd-hh-mm-ss");
    QString sPath = StorageDir() ^ "img" + sImgkName + ".png";
    m_oImage.save(sPath);
    emit addImage(sPath);
    screenCapture_ = nullptr;
  }
}

void ScreenCapture::capture(QQuickItem* p)
{
  qDebug() << "capture";
  QEventLoop oLoop;
  oLoop.processEvents();
  m_oImage = currentView_->grabWindow();
  m_oImagePreview = m_oImage.scaledToHeight(height());
  update();
}

void ScreenCapture::paint(QPainter* p)
{
  p->drawImage(0, 0, m_oImagePreview);
  int nPenW = 2;
  auto nColor = Qt::white;
  if (IsSelected)
  {
    nColor = Qt::green;
    nPenW = 6;
  }

  QPen oPen(nColor, nPenW);
  p->setPen(oPen);
  QRect o(nPenW / 2, nPenW / 2, width() - nPenW * 2, height() - nPenW * 2);
  p->drawRect(o);
}

ImageThumb::ImageThumb(QObject* parent) : QObject(parent)
{
}

void ImageThumb::save(QString s)
{
  if (s.isEmpty())
    return;

  qDebug() << s;

  QImageReader oImageReader(s);

  oImageReader.setAutoTransform(true);
  QImage oO = oImageReader.read();

  QRect oT;
  if (oO.height() > oO.width())
  {
    oT.setX(0);
    oT.setY((oO.height() - oO.width()) / 2);
    oT.setWidth(oO.width());
    oT.setHeight(oO.width());
  }
  else
  {
    oT.setY(0);
    oT.setX((oO.width() - oO.height()) / 2);
    oT.setWidth(oO.height());
    oT.setHeight(oO.height());
  }

  auto oOriginalPixmap = oO.copy(oT);

  oOriginalPixmap.scaledToWidth(180).save(mssutils::Hash(s));
}

QUrl ImageThumb::name(QString s)
{
  QString sRet;
  if (s.startsWith("/") == false)
    return QUrl("image://theme/icon-m-file-image");
  sRet = mssutils::Hash(s);
  return sRet;
}

StopWatch::StopWatch(const QString& sMsg)
{
  m_oTimer = new QElapsedTimer;
  m_sMsg = sMsg;
  m_oTimer->start();
  return;
}

StopWatch::StopWatch()
{
  m_oTimer = new QElapsedTimer;
  m_oTimer->start();
  m_bMsgPrinted = true;
  return;
}

StopWatch::~StopWatch()
{
  // Only print in destructtor if Stop not called
  if (m_bMsgPrinted == false)
  {
    qint64 nanoSec = m_oTimer->nsecsElapsed();
    double fTime = (nanoSec) / 1000000.0; // milliseconds

    QString sMsg(m_sMsg.arg(fTime));
    qDebug() << sMsg;
  }
  delete m_oTimer;
}

double StopWatch::StopTimeSec()
{
  qint64 nanoSec = m_oTimer->nsecsElapsed();
  return (nanoSec) / 1000000000.0;
}

void StopWatch::Stop()
{
  qint64 nanoSec = m_oTimer->nsecsElapsed();
  double fTime = (nanoSec) / 1000000.0; // milliseconds

  QString sMsg(m_sMsg.arg(fTime));
  m_bMsgPrinted = true;
  qDebug() << sMsg;
}

MssTimer::MssTimer()
{
  m_pTimer = new QBasicTimer();
  m_bIsSingleShot = false;
}

MssTimer::MssTimer(std::function<void()> pfnTimeOut)
{
  m_pTimer = new QBasicTimer;
  m_pfnTimeOut = pfnTimeOut;
  m_bIsSingleShot = false;
}

MssTimer::~MssTimer()
{
  delete m_pTimer;
}

void MssTimer::Start(int nMilliSec)
{
  m_bIsSingleShot = false;
  m_pTimer->start(nMilliSec, this);
}

void MssTimer::SingleShot(int nMilliSec)
{
  m_bIsSingleShot = true;
  if (m_pTimer->isActive() == true)
    m_pTimer->stop();
  m_pTimer->start(nMilliSec, this);
}

void MssTimer::Stop()
{
  m_pTimer->stop();
}

bool MssTimer::IsActive()
{
  return m_pTimer->isActive();
}

void MssTimer::timerEvent(QTimerEvent*)
{
  if (m_pfnTimeOut != nullptr)
    m_pfnTimeOut();

  if (m_bIsSingleShot == true)
    m_pTimer->stop();
}

QString StorageDir()
{
  static bool bIsChecked = false;
  QString sRet = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/pikeFight";
  if (bIsChecked == false)
  {
    QDir o(sRet);
    if (o.exists() == false)
      o.mkpath(sRet);

    bIsChecked = true;
  }

  return sRet;
}

QString GpxNewName(const QString& _sTrackName)
{
  int nCount = 0;
  QString sTrackName = _sTrackName;
  while (QFile::exists(GpxDatFullName(sTrackName)) == true)
  {
    sTrackName.sprintf("%ls(%d)", (wchar_t*)_sTrackName.utf16(), ++nCount);
  }
  return sTrackName;
}

QString GpxDatFullName(const QString& sTrackName)
{
  QString sDataFilePath = StorageDir();
  QString sGpxFileName;
  sGpxFileName.sprintf("%ls/%ls.dat", (wchar_t*)sDataFilePath.utf16(),
                       (wchar_t*)sTrackName.utf16());

  return sGpxFileName;
}

QString GpxFullName(const QString& sTrackName)
{
  QString sDataFilePath = StorageDir();
  QString sGpxFileName;
  sGpxFileName.sprintf("%ls/%ls.gpx", (wchar_t*)sDataFilePath.utf16(),
                       (wchar_t*)sTrackName.utf16());
  return sGpxFileName;
}

QString FormatKm(double f)
{
  char szStr[21];
  snprintf(szStr, 20, "%.3f", f);
  return szStr;
}

QString FormatLatitude(double fLatitude)
{
  int nDegrees = abs(static_cast<int>(fLatitude));
  double fMinutes = 60.0 * (fabs(fLatitude) - nDegrees);

  // Check if we have overflow
  if (fMinutes >= 59.995)
  {
    fMinutes = 0.0;
    nDegrees++;
  }

  char cLat('N');
  if (fLatitude < 0.0)
    cLat = 'S';

  char szStr[40];
  sprintf(szStr, "%c%02d\xb0 %.2f'", cLat, nDegrees, fMinutes);
  return QString::fromLatin1(szStr);
}

QString FormatDuration(unsigned int nTime)
{
  wchar_t szStr[20];
  time_t now = nTime / 10;

  tm* tmNow = gmtime(&now);

  if (nTime <= 0)
    return "x";

  if (tmNow->tm_hour > 0)
    wcsftime(szStr, 20, L"%H:%M:%S", tmNow);
  else
    wcsftime(szStr, 20, L"%M:%S", tmNow);

  wchar_t szStr2[20];

  swprintf(szStr2, 20, L"%ls.%d", szStr, nTime % 10);

  return QString::fromWCharArray(szStr2);
}

QString FormatDateTime(unsigned int nTime)
{
  if (nTime == 0)
    return "-";

  wchar_t szStr[20];
  time_t now = nTime;

  wcsftime(szStr, 20, L"%Y-%m-%d %H:%M:%S", localtime(&now));

  QString sRet = QString::fromWCharArray(szStr);

  return sRet;
}

QString FormatKmH(double f)
{
  char szStr[60];
  sprintf(szStr, "%.1f", f);
  return szStr;
}

QString FormatLongitude(double fLongitude)
{
  int nDegrees = abs(static_cast<int>(fLongitude));
  double fMinutes = 60.0 * (fabs(fLongitude) - nDegrees);

  // Check if we have overflow
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

  sprintf(szStr, "%c%03d\xb0 %.2f'", cLong, nDegrees, fMinutes);
  return QString::fromLatin1(szStr);

  //   return QString(QLatin1String("%1%2\xB0%3'")).arg(cLong).arg(nDegrees, 3,
  //   10, QChar('0')).arg(fMinutes, 5, 'f', 2, QChar('0'));
}

QString DirName(const QString& sFileName)
{
  int n = sFileName.lastIndexOf(SLASH);
  if (n < 0)
    return sFileName;
  return sFileName.left(n);
}

QString JustFileNameNoExt(const QString& sFileName)
{
  return BaseName(JustFileName(sFileName));
}

QString JustFileName(const QString& sFileName)
{
  int n = sFileName.lastIndexOf(SLASH);
  if (n < 0)
    return sFileName;
  return sFileName.right(sFileName.size() - n - 1);
}

QString BaseName(const QString& sFileName)
{
  return sFileName.left(sFileName.lastIndexOf('.'));
}

QString Ext(const QString& sFileName)
{
  return sFileName.right(sFileName.size() - sFileName.lastIndexOf('.'));
}

const int kb = 1024;
const int mb = 1024 * kb;
const int gb = 1024 * mb;

QString FormatNrBytes(int nBytes)
{
  double fBytes = nBytes;
  wchar_t szStr[30];

  if (nBytes == 0)
    return "-";

  if (nBytes >= gb)
    swprintf(szStr, 30, L"%.2f GB", fBytes / gb);
  else if (nBytes >= mb)
    swprintf(szStr, 30, L"%.1f MB", fBytes / mb);
  else if (nBytes >= kb)
    swprintf(szStr, 30, L"%d KB", nBytes / kb);
  else
    swprintf(szStr, 30, L"%d byte(s)", nBytes);

  return QString::fromWCharArray(szStr);
}

void WriteMarkData(const QString& sTrackName, MarkData& t)
{
  QString sGpxDatFileName = GpxDatFullName(sTrackName);
  QFile oDat(sGpxDatFileName);
  oDat.open(QIODevice::WriteOnly);
  oDat.write((char*)&t, sizeof t);
  oDat.close();
  return;
}

MarkData GetMarkData(const QString& sTrackName)
{
  QString sGpxDatFileName = GpxDatFullName(sTrackName);
  QFile oDat;
  oDat.setFileName(sGpxDatFileName);
  bool bRet = oDat.open(QIODevice::ReadOnly);
  MarkData tData;
  memset((void*)&tData, 0, sizeof tData);
  tData.nType = -1;
  if (bRet == false)
    return tData;
  oDat.read((char*)&tData, sizeof tData);
  oDat.close();
  return tData;
}

void ScreenOn(bool b)
{
  QDBusConnection system = QDBusConnection::connectToBus(QDBusConnection::SystemBus, "system");
  QDBusInterface interface("com.nokia.mce", "/com/nokia/mce/request", "com.nokia.mce.request",
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

namespace msslistmodel
{
  QHash<int, MssListModel*> g_ocInstance;
}

void MssListModel::Init(int nInstanceId)
{
  msslistmodel::g_ocInstance[nInstanceId] = this;
}

MssListModel* MssListModel::Instance(int nInstanceId)
{
  if (msslistmodel::g_ocInstance.contains(nInstanceId) == false)
    return 0;
  return msslistmodel::g_ocInstance[nInstanceId];
}
void MssListModel::Reset(InitFunc pfInit)
{
  if (m_ocRows.size() > 0)
  {
    beginRemoveRows(QModelIndex(), 0, m_ocRows.size() - 1);
    m_ocRows.erase(m_ocRows.begin(), m_ocRows.end());
    endRemoveRows();
  }

  pfInit(&m_ocRows);
  if (m_ocRows.size() < 1)
    return;

  beginInsertRows(QModelIndex(), 0, m_ocRows.size() - 1);
  endInsertRows();
}

int MssListModel::FindRow(const QVariant& oValToFind, int nCol)
{
  for (auto& oI : m_ocRows)
  {
    if (oI[nCol] == oValToFind)
      return mssutils::IndexOf(oI, m_ocRows);
  }
  return -1;
}

QHash<int, QByteArray> MssListModel::roleNames() const
{
  return m_ocRole;
}

int MssListModel::columnCount(const QModelIndex&) const
{
  return 1;
}

int MssListModel::rowCount(const QModelIndex&) const
{
  return m_ocRows.size();
}

QModelIndex MssListModel::index(int row, int column, const QModelIndex&) const
{
  QModelIndex o = createIndex(row, column);
  return o;
}

QModelIndex MssListModel::parent(const QModelIndex&) const
{
  return QModelIndex();
}

QVariant MssListModel::data(const QModelIndex& oIndex, int role) const
{
  int roleIdx = role - Qt::UserRole;
  int nRow = oIndex.row();
  if (m_ocRows.size() <= nRow || nRow < 0)
    return QVariant();

  return m_ocRows[nRow][roleIdx];
}

void MssListModel::updateItem(int nRow, int nCol, const QVariant& value)
{
  if (nRow < 0)
    return;
  if (m_ocRows[nRow][nCol] == value)
    return;

  QVector<int> oc;
  oc.push_back(Qt::UserRole + nCol);
  m_ocRows[nRow][nCol] = value;
  emit dataChanged(index(nRow, 0), index(nRow, 0), oc);
}

void MssListModel::clearAll()
{
  if (m_ocRows.isEmpty() == true)
    return;

  beginRemoveRows(QModelIndex(), 0, m_ocRows.size() - 1);
  m_ocRows.erase(m_ocRows.begin(), m_ocRows.end());
  endRemoveRows();
}

void MssListModel::removeRow(int nRow)
{
  if (m_ocRows.size() <= nRow)
    return;

  if (nRow < 0)
    return;
  beginRemoveRows(QModelIndex(), nRow, nRow);
  m_ocRows.remove(nRow, 1);
  endRemoveRows();
}

int MssListModel::AddRow(const QVector<QVariant>& ocRow)
{
  int nIndex = m_ocRows.size();
  beginInsertRows(QModelIndex(), nIndex, nIndex);
  m_ocRows.append(ocRow);
  endInsertRows();
  return nIndex;
}
