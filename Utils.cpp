#include <math.h>

#include "QExifImageHeader.h"
#include "Utils.h"
#include <QBasicTimer>
#include <QBuffer>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QImage>
#include <QImageReader>
#include <QOrientationReading>
#include <QPainter>
#include <QPixmap>
#include <QQuickItemGrabResult>
#include <QQuickView>
#include <QStandardPaths>
#include <QTextCodec>

#include <QtMultimedia/QMediaObject>
#include <QtMultimedia/QtMultimedia>
#include <thread>

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

static QQuickView* g_currentView = nullptr;
static ScreenCapture* g_selectedScreenCapture = nullptr;
static QOrientationSensor* g_oOrientationSensor = nullptr;
static CaptureThumbMaker* g_oCaptureThumbMaker = nullptr;

bool CaptureThumbMaker::HasSelectedCapture()
{
  if (g_selectedScreenCapture == nullptr)
    return false;

  return g_selectedScreenCapture->IsSelected;
}

int CaptureThumbMaker::HEIGHT()
{
  return 1920;
}

int CaptureThumbMaker::WIDTH()
{
  return 1080;
}

ScreenCapturedImg::ScreenCapturedImg() : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage ScreenCapturedImg::requestImage(const QString&, QSize* size, const QSize&)
{
  if (g_selectedScreenCapture == nullptr)
    return QImage();

  *size = g_selectedScreenCapture->m_oImage.size();

  return g_selectedScreenCapture->m_oImage;
}

void ScreenCapture::StartBusyInd()
{
  QMetaObject::invokeMethod(m_pPage, "addImageStart", Q_ARG(QVariant, m_nIndex));
}

void ScreenCapture::StopBusyInd()
{
  QMetaObject::invokeMethod(m_pPage, "addImageStop", Q_ARG(QVariant, m_nIndex));
}

void ScreenCapture::SetView(QQuickView* parent)
{
  g_currentView = parent;
}

ScreenCapture::ScreenCapture()
{
}

ScreenCapture::~ScreenCapture()
{
  g_selectedScreenCapture = nullptr;
  if (IsSelected)
  {
    auto oW = std::thread(
        [](QObject* p, QObject* pModel, QImage oImg, int nIndex, int nO) {
          QDateTime oNow(QDateTime::currentDateTime());
          QString sImgName = oNow.toString("yyyy-MM-dd-hh-mm-ss");
          QString sPath = StorageDir() ^ ("img" + sImgName + ".jpg");

          int nW = CaptureThumbMaker::WIDTH();
          int nH = CaptureThumbMaker::HEIGHT();
          int nY = (oImg.height() - nH) / 2;
          QRect tRect(0, nY, nW, nH);
          QImage oImageFromRect = oImg.copy(tRect); // .save(sPath);
          QByteArray ocImgBuffer;
          QBuffer oAnnotatedImageBuffer(&ocImgBuffer);
          oAnnotatedImageBuffer.open(QIODevice::ReadWrite);
          oImageFromRect.save(&oAnnotatedImageBuffer, "jpg");
          QExifImageHeader oExif;
          oExif.setValue(QExifImageHeader::Make, "Sony");
          oExif.setValue(QExifImageHeader::Software, "PikeFight 1.1");
          oExif.setValue(QExifImageHeader::DateTimeOriginal, QExifValue(oNow));
          QVector<quint8> ver{2, 3, 0, 0};
          oExif.setValue(QExifImageHeader::ExifVersion, QExifValue(ver));

          /*
          enum Orientation {
              Undefined = 0,
              TopUp, 1
              TopDown, 2
              LeftUp, 3
              RightUp, 4
              FaceUp,
              FaceDown
          };


    1: Normal (0° rotation)
    3: Upside-down (180° rotation)
    6: Rotated 90° counterclockwise (270° clockwise)
    8: Rotated 90° clockwise (270° counterclockwise)
              */

          if (nO == 4)
            oExif.setValue(QExifImageHeader::Orientation, 8);
          else if (nO == 3)
            oExif.setValue(QExifImageHeader::Orientation, 6);
          else if (nO == 2)
            oExif.setValue(QExifImageHeader::Orientation, 3);
          else if (nO == 1)
            oExif.setValue(QExifImageHeader::Orientation, 1);

          oExif.saveToJpeg(&oAnnotatedImageBuffer);
          oAnnotatedImageBuffer.close();
          QFile oFile(sPath);
          oFile.open(QIODevice::WriteOnly);
          oFile.write(ocImgBuffer);
          oFile.close();

          emit g_oCaptureThumbMaker->hasSelectedCaptureChanged();

          QMetaObject::invokeMethod(p, "addImageGo", Q_ARG(QVariant, nIndex),
                                    Q_ARG(QVariant, QVariant::fromValue(pModel)),
                                    Q_ARG(QVariant, sPath), Q_ARG(QVariant, nO));
        },
        m_pPage, m_pModel, m_oImage, m_nIndex, m_nOrientation);

    oW.detach();
  }
}

void ScreenCapture::save()
{
  if (this->IsSelected)
  {
    this->IsSelected = false;
    StopBusyInd();
    update();
    emit g_oCaptureThumbMaker->hasSelectedCaptureChanged();
    return;
  }
  if (g_selectedScreenCapture != nullptr)
  {
    g_selectedScreenCapture->IsSelected = false;
    g_selectedScreenCapture->update();
  }

  StartBusyInd();

  g_selectedScreenCapture = this;
  g_selectedScreenCapture->IsSelected = true;
  g_selectedScreenCapture->update();
  emit g_oCaptureThumbMaker->hasSelectedCaptureChanged();
}

void ScreenCapture::setPageAndModel(QObject* pPage, QObject* pModel, int nIndex)
{
  m_nIndex = nIndex;
  m_pModel = pModel;
  m_pPage = pPage;
}

void ScreenCapture::capture()
{
  QEventLoop oLoop;
  oLoop.processEvents();
  m_oImage = g_currentView->grabWindow();
  m_oImagePreview = m_oImage.scaledToHeight(height());
  if (g_oOrientationSensor == nullptr)
    g_oOrientationSensor = new QOrientationSensor;
  if (!g_oOrientationSensor->isActive())
  {
    g_oOrientationSensor->start();
  }
  m_nOrientation = g_oOrientationSensor->reading()->orientation();

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

FileMgr::FileMgr()
{
}

void FileMgr::remove(QString s)
{
  QFile::remove(s);
}

QString FileMgr::renameToAscii(QString s)
{
  QString sRet;
  bool bRename = false;
  for (auto o : s)
    if (o < 0x7f)
      sRet.push_back(o);
    else
      bRename = true;

  if (bRename)
    QFile::rename(s, sRet);

  return sRet;
}

CaptureThumbMaker::CaptureThumbMaker(QObject* parent) : QObject(parent)
{
  g_oCaptureThumbMaker = this;
}

void CaptureThumbMaker::save(QString s, int nOrientation)
{
  if (s.isEmpty())
    return;

  QImageReader oImageReader(s);
  if (nOrientation == 0)
    oImageReader.setAutoTransform(false);

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

  oOriginalPixmap = oOriginalPixmap.scaledToWidth(180);
  if (nOrientation == 4)
    oOriginalPixmap = oOriginalPixmap.transformed(QMatrix().rotate(270.0));
  if (nOrientation == 3)
    oOriginalPixmap = oOriginalPixmap.transformed(QMatrix().rotate(90.0));
  oOriginalPixmap.save(mssutils::Hash(s));
}

QUrl CaptureThumbMaker::name(QString s)
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

QString GpxNewName(const QString& _sTrackName, int nCount)
{
  QString sTrackName;
  for (;;)
  {
    if (nCount != -1)
      sTrackName.sprintf("%ls(%02d)", (wchar_t*)_sTrackName.utf16(), ++nCount);
    else
    {
      sTrackName = _sTrackName;
      nCount = 0;
    }

    if (QFile::exists(GpxDatFullName(sTrackName)) == false)
      break;
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


struct ExifIfdHeader
{
  quint16 tag;
  quint16 type;
  quint32 count;
  union
  {
    quint32 offset;
    quint8 offsetBytes[4];
    char offsetAscii[4];
    quint16 offsetShorts[2];
  };
};

QDataStream& operator>>(QDataStream& stream, ExifIfdHeader& header)
{
  stream >> header.tag;
  stream >> header.type;
  stream >> header.count;

  if (header.type == QExifValue::Byte && header.count <= 4)
  {
    stream.readRawData(header.offsetAscii, 4);
  }
  else if (header.type == QExifValue::Ascii && header.count <= 4)
  {
    stream.readRawData(header.offsetAscii, 4);
  }
  else if (header.type == QExifValue::Short && header.count <= 2)
  {
    stream >> header.offsetShorts[0];
    stream >> header.offsetShorts[1];
  }
  else
  {
    stream >> header.offset;
  }

  return stream;
}

class QExifValuePrivate : public QSharedData
{
public:
  QExifValuePrivate(quint16 t, int c) : type(t), count(c) {}
  virtual ~QExifValuePrivate() {}

  quint16 type;
  int count;
};

class QExifByteValuePrivate : public QExifValuePrivate
{
public:
  QExifByteValuePrivate() : QExifValuePrivate(QExifValue::Byte, 0) { ref.ref(); }
  QExifByteValuePrivate(const QVector<quint8>& v)
      : QExifValuePrivate(QExifValue::Byte, v.size()), value(v)
  {
  }

  QVector<quint8> value;
};

class QExifUndefinedValuePrivate : public QExifValuePrivate
{
public:
  QExifUndefinedValuePrivate(const QByteArray& v)
      : QExifValuePrivate(QExifValue::Undefined, v.size()) /*, value( v )*/
  {
    value = v;
    // Bugfix: by Sig sig@sigvdr.de
    // a real copy of the value is needed.
    value.detach();
    //        value = "";
    //        value.append(v);
  }
  QByteArray value;
};

class QExifAsciiValuePrivate : public QExifValuePrivate
{
public:
  QExifAsciiValuePrivate(const QString& v)
      : QExifValuePrivate(QExifValue::Ascii, v.size() + 1), value(v)
  {
  }
  QExifAsciiValuePrivate(const char* v)
      : QExifValuePrivate(QExifValue::Ascii, (int)strlen(v) + 1), value(v)
  {
  }

  QString value;
};

class QExifShortValuePrivate : public QExifValuePrivate
{
public:
  QExifShortValuePrivate(const QVector<quint16>& v)
      : QExifValuePrivate(QExifValue::Short, v.size()), value(v)
  {
  }

  QVector<quint16> value;
};

class QExifLongValuePrivate : public QExifValuePrivate
{
public:
  QExifLongValuePrivate(const QVector<quint32>& v)
      : QExifValuePrivate(QExifValue::Long, v.size()), value(v)
  {
  }

  QVector<quint32> value;
};

class QExifSignedLongValuePrivate : public QExifValuePrivate
{
public:
  QExifSignedLongValuePrivate(const QVector<qint32>& v)
      : QExifValuePrivate(QExifValue::SignedLong, v.size()), value(v)
  {
  }

  QVector<qint32> value;
};

class QExifRationalValuePrivate : public QExifValuePrivate
{
public:
  QExifRationalValuePrivate(const QVector<QExifURational>& v)
      : QExifValuePrivate(QExifValue::Rational, v.size()), value(v)
  {
  }

  QVector<QExifURational> value;
};

class QExifSignedRationalValuePrivate : public QExifValuePrivate
{
public:
  QExifSignedRationalValuePrivate(const QVector<QExifSRational>& v)
      : QExifValuePrivate(QExifValue::SignedRational, v.size()), value(v)
  {
  }

  QVector<QExifSRational> value;
};

Q_GLOBAL_STATIC(QExifByteValuePrivate, qExifValuePrivateSharedNull)

QExifValue::QExifValue() : d(qExifValuePrivateSharedNull())
{
}

QExifValue::QExifValue(quint8 value) : d(new QExifByteValuePrivate(QVector<quint8>(1, value)))
{
}
QExifValue::QExifValue(const QVector<quint8>& values) : d(new QExifByteValuePrivate(values))
{
}
/*!
Constructs a QExifValue with a \a value of type Ascii or Undefined.
If the \a encoding is NoEncoding the value will be of type Ascii, otherwise it will be Undefined and
the string encoded using the given \a encoding.
*/
QExifValue::QExifValue(const QString& value, TextEncoding encoding)
    : d(qExifValuePrivateSharedNull())
{
  switch (encoding)
  {
  case AsciiEncoding:
    d = new QExifUndefinedValuePrivate(QByteArray::fromRawData("ASCII\0\0\0", 8) +
                                       value.toLatin1());
    break;
  case JisEncoding:
  {
    QTextCodec* codec = QTextCodec::codecForName("JIS X 0208");
    if (codec)
      d = new QExifUndefinedValuePrivate(QByteArray::fromRawData("JIS\0\0\0\0\0", 8) +
                                         codec->fromUnicode(value));
  }
  break;
  case UnicodeEncoding:
  {
    QTextCodec* codec = QTextCodec::codecForName("UTF-16");
    if (codec)
      d = new QExifUndefinedValuePrivate(QByteArray::fromRawData("UNICODE\0", 8) +
                                         codec->fromUnicode(value));
  }
  break;
  case UndefinedEncoding:
    d = new QExifUndefinedValuePrivate(QByteArray::fromRawData("\0\0\0\0\0\0\0\\0", 8) +
                                       value.toLocal8Bit());
    break;
  default:
    d = new QExifAsciiValuePrivate(value);
  }
}

QExifValue::QExifValue(const char* value) : d(qExifValuePrivateSharedNull())
{
  d = new QExifAsciiValuePrivate(value);
}

QExifValue::QExifValue(const QByteArray& value) : d(new QExifUndefinedValuePrivate(value))
{
}

QExifValue::QExifValue(quint16 value) : d(new QExifShortValuePrivate(QVector<quint16>(1, value)))
{
}

QExifValue::QExifValue(const QVector<quint16>& values) : d(new QExifShortValuePrivate(values))
{
}

QExifValue::QExifValue(quint32 value) : d(new QExifLongValuePrivate(QVector<quint32>(1, value)))
{
}

QExifValue::QExifValue(const QVector<quint32>& values) : d(new QExifLongValuePrivate(values))
{
}

QExifValue::QExifValue(const QExifURational& value)
    : d(new QExifRationalValuePrivate(QVector<QExifURational>(1, value)))
{
}

QExifValue::QExifValue(const QVector<QExifURational>& values)
    : d(new QExifRationalValuePrivate(values))
{
}

QExifValue::QExifValue(qint32 value) : d(new QExifSignedLongValuePrivate(QVector<qint32>(1, value)))
{
}

QExifValue::QExifValue(const QVector<qint32>& values) : d(new QExifSignedLongValuePrivate(values))
{
}

QExifValue::QExifValue(const QExifSRational& value)
    : d(new QExifSignedRationalValuePrivate(QVector<QExifSRational>(1, value)))
{
}

QExifValue::QExifValue(const QVector<QExifSRational>& values)
    : d(new QExifSignedRationalValuePrivate(values))
{
}

/*!
    Constructs a QExifValue of type Ascii with an ascii string formatted from a date-time \a value.

    Date-times are stored as strings in the format \c {yyyy:MM:dd HH:mm:ss}.
    */
QExifValue::QExifValue(const QDateTime& value)
    : d(new QExifAsciiValuePrivate(value.toString(QLatin1String("yyyy:MM:dd HH:mm:ss"))))
{
}

QExifValue::QExifValue(const QExifValue& other) : d(other.d)
{
}

QExifValue& QExifValue::operator=(const QExifValue& other)
{
  d = other.d;

  return *this;
}

QExifValue::~QExifValue()
{
}

bool QExifValue::operator==(const QExifValue& other) const
{
  return d == other.d;
}

bool QExifValue::isNull() const
{
  return d == qExifValuePrivateSharedNull();
}

int QExifValue::type() const
{
  return d->type;
}

/*!
    Returns the number of elements in a QExifValue.  For ascii strings this is the length of the
   string including the terminating null.
    */
int QExifValue::count() const
{
  return d->count;
}

/*!
      Returns the encoding of strings stored in Undefined values.
      */
QExifValue::TextEncoding QExifValue::encoding() const
{
  if (d->type == Undefined && d->count > 8)
  {
    QByteArray value = static_cast<const QExifUndefinedValuePrivate*>(d.constData())->value;

    if (value.startsWith(QByteArray::fromRawData("ASCII\0\0\0", 8)))
      return AsciiEncoding;
    else if (value.startsWith(QByteArray::fromRawData("JIS\0\0\0\0\0", 8)))
      return JisEncoding;
    else if (value.startsWith(QByteArray::fromRawData("UNICODE\0", 8)))
      return UnicodeEncoding;
    else if (value.startsWith(QByteArray::fromRawData("\0\0\0\0\0\0\0\0", 8)))
      return UndefinedEncoding;
  }
  return NoEncoding;
}

quint8 QExifValue::toByte() const
{
  return d->type == Byte && d->count == 1
             ? static_cast<const QExifByteValuePrivate*>(d.constData())->value.at(0)
             : 0;
}

QVector<quint8> QExifValue::toByteVector() const
{
  return d->type == Byte ? static_cast<const QExifByteValuePrivate*>(d.constData())->value
                         : QVector<quint8>();
}

QString QExifValue::toString() const
{
  switch (d->type)
  {
  case Ascii:
    return static_cast<const QExifAsciiValuePrivate*>(d.constData())->value;

  case Rational:
    return "Rational";

  case Undefined:
  {
    QByteArray string = static_cast<const QExifUndefinedValuePrivate*>(d.constData())->value.mid(8);

    switch (encoding())
    {
    case AsciiEncoding:
      return QString::fromLatin1(string.constData(), string.length());
    case JisEncoding:
    {
      QTextCodec* codec = QTextCodec::codecForName("JIS X 0208");
      if (codec)
        return codec->toUnicode(string);
    }
    break;
    case UnicodeEncoding:
    {
      QTextCodec* codec = QTextCodec::codecForName("UTF-16");
      if (codec)
        return codec->toUnicode(string);
    }
    case UndefinedEncoding:
      return QString::fromLocal8Bit(string.constData(), string.length());
    default:
      break;
    }
  }
  default:
    return QString();
  }
}

quint16 QExifValue::toShort() const
{
  if (d->count == 1)
  {
    switch (d->type)
    {
    case Byte:
      return static_cast<const QExifByteValuePrivate*>(d.constData())->value.at(0);
    case Short:
      return static_cast<const QExifShortValuePrivate*>(d.constData())->value.at(0);
    }
  }
  return 0;
}

QVector<quint16> QExifValue::toShortVector() const
{
  return d->type == Short ? static_cast<const QExifShortValuePrivate*>(d.constData())->value
                          : QVector<quint16>();
}

quint32 QExifValue::toLong() const
{
  if (d->count == 1)
  {
    switch (d->type)
    {
    case Byte:
      return static_cast<const QExifByteValuePrivate*>(d.constData())->value.at(0);
    case Short:
      return static_cast<const QExifShortValuePrivate*>(d.constData())->value.at(0);
    case Long:
      return static_cast<const QExifLongValuePrivate*>(d.constData())->value.at(0);
    case SignedLong:
      return static_cast<const QExifSignedLongValuePrivate*>(d.constData())->value.at(0);
    }
  }
  return 0;
}

QVector<quint32> QExifValue::toLongVector() const
{
  return d->type == Long ? static_cast<const QExifLongValuePrivate*>(d.constData())->value
                         : QVector<quint32>();
}

QExifURational QExifValue::toRational() const
{
  return d->type == Rational && d->count == 1
             ? static_cast<const QExifRationalValuePrivate*>(d.constData())->value.at(0)
             : QExifURational();
}

QVector<QExifURational> QExifValue::toRationalVector() const
{
  return d->type == Rational ? static_cast<const QExifRationalValuePrivate*>(d.constData())->value
                             : QVector<QExifURational>();
}

QByteArray QExifValue::toByteArray() const
{
  switch (d->type)
  {
  case Ascii:
    return static_cast<const QExifAsciiValuePrivate*>(d.constData())->value.toLatin1();
  case Undefined:
    return static_cast<const QExifUndefinedValuePrivate*>(d.constData())->value;
  default:
    return QByteArray();
  }
}

qint32 QExifValue::toSignedLong() const
{
  if (d->count == 1)
  {
    switch (d->type)
    {
    case Byte:
      return static_cast<const QExifByteValuePrivate*>(d.constData())->value.at(0);
    case Short:
      return static_cast<const QExifShortValuePrivate*>(d.constData())->value.at(0);
    case Long:
      return static_cast<const QExifLongValuePrivate*>(d.constData())->value.at(0);
    case SignedLong:
      return static_cast<const QExifSignedLongValuePrivate*>(d.constData())->value.at(0);
    }
  }
  return 0;
}

QVector<qint32> QExifValue::toSignedLongVector() const
{
  return d->type == SignedLong
             ? static_cast<const QExifSignedLongValuePrivate*>(d.constData())->value
             : QVector<qint32>();
}

QExifSRational QExifValue::toSignedRational() const
{
  return d->type == SignedRational && d->count == 1
             ? static_cast<const QExifSignedRationalValuePrivate*>(d.constData())->value.at(0)
             : QExifSRational();
}

QVector<QExifSRational> QExifValue::toSignedRationalVector() const
{
  return d->type == SignedRational
             ? static_cast<const QExifSignedRationalValuePrivate*>(d.constData())->value
             : QVector<QExifSRational>();
}

/*!
    Returns the value of QExifValue storing a date-time.
    Date-times are stored as ascii strings in the format \c {yyyy:MM:dd HH:mm:ss}.
    */
QDateTime QExifValue::toDateTime() const
{
  return d->type == Ascii && d->count == 20
             ? QDateTime::fromString(
                   static_cast<const QExifAsciiValuePrivate*>(d.constData())->value,
                   QLatin1String("yyyy:MM:dd HH:mm:ss"))
             : QDateTime();
}

QDate QExifValue::toDate() const
{
  return QDate::fromString(static_cast<const QExifAsciiValuePrivate*>(d.constData())->value,
                           QLatin1String("yyyy:MM:dd"));
}

QTime QExifValue::toTime() const
{
  if (d->count == 3)
  {
    auto& tVec = static_cast<const QExifRationalValuePrivate*>(d.constData())->value;
    return QTime(tVec[0].first / tVec[0].second, tVec[1].first / tVec[1].second,
                 tVec[2].first / tVec[2].second);
  }
  return QTime();
}

class QExifImageHeaderPrivate
{
public:
  QSysInfo::Endian byteOrder;
  mutable qint64 size;
  QMap<QExifImageHeader::ImageTag, QExifValue> imageIfdValues;
  QMap<QExifImageHeader::ExifExtendedTag, QExifValue> exifIfdValues;
  QMap<QExifImageHeader::GpsTag, QExifValue> gpsIfdValues;

  QSize thumbnailSize;
  QByteArray thumbnailData;
  QExifValue thumbnailXResolution;
  QExifValue thumbnailYResolution;
  QExifValue thumbnailResolutionUnit;
  QExifValue thumbnailOrientation;
};

QExifImageHeader::QExifImageHeader() : d(new QExifImageHeaderPrivate)
{
  d->byteOrder = QSysInfo::ByteOrder;
  d->size = -1;
}

QExifImageHeader::QExifImageHeader(const QString& fileName) : d(new QExifImageHeaderPrivate)
{
  d->byteOrder = QSysInfo::ByteOrder;
  d->size = -1;
  exifHeaderId = 0xE1;
  loadFromJpeg(fileName);
}

QExifImageHeader::~QExifImageHeader()
{
  clear();

  delete d;
}

bool QExifImageHeader::loadFromJpeg(const QString& fileName)
{
  QFile file(fileName);

  if (file.open(QIODevice::ReadOnly))
    return loadFromJpeg(&file);
  else
    return false;
}

bool QExifImageHeader::loadFromJpeg(QIODevice* device)
{
  clear();

  QByteArray exifData = extractExif(device);
  QByteArray nextData = device->read(20);
  if (nextData.isEmpty())
    return false;
  if (!exifData.isEmpty())
  {
    QBuffer buffer(&exifData);

    return buffer.open(QIODevice::ReadOnly) && read(&buffer);
  }

  return false;
}

bool QExifImageHeader::saveToJpeg(const QString& fileName) const
{
  QFile file(fileName);

  if (file.open(QIODevice::ReadWrite))
    return saveToJpeg(&file);
  else
    return false;
}

bool QExifImageHeader::saveToJpeg(QIODevice* device) const
{
  if (device->isSequential())
    return false;

  QByteArray exif;

  {
    QBuffer buffer(&exif);
    if (!buffer.open(QIODevice::WriteOnly))
      return false;

    write(&buffer);
    buffer.close();
    exif = QByteArray::fromRawData("Exif\0\0", 6) + exif;
  }

  QDataStream stream(device);
  // Should be default
  stream.setByteOrder(QDataStream::BigEndian);
  device->seek(0);
  QByteArray oc = device->read(2);
  if (oc != "\xFF\xD8")
  {
    return false;
  }

  quint16 segmentId;
  quint16 segmentLength;

  stream >> segmentId;
  stream >> segmentLength;

  if (segmentId == 0xFFE0)
  {
    QByteArray jfif = device->read(segmentLength - 2);

    if (!jfif.startsWith("JFIF"))
      return false;

    stream >> segmentId;
    stream >> segmentLength;

    if (segmentId == 0xFFE1)
    {
      QByteArray oldExif = device->read(segmentLength - 2);

      if (!oldExif.startsWith("Exif"))
        return false;

      int dSize = oldExif.size() - exif.size();

      if (dSize > 0)
        exif += QByteArray(dSize, '\0');

      QByteArray remainder = device->readAll();

      device->seek(0);
      stream << quint16(0xFFD8); // SOI
      stream << quint16(0xFFE0); // APP0
      stream << quint16(jfif.size() + 2);
      device->write(jfif);
      stream << quint16(0xFFE1); // APP1
      stream << quint16(exif.size() + 2);
      device->write(exif);
      device->write(remainder);
    }
    else
    {
      QByteArray remainder = device->readAll();
      device->seek(0);
      stream << quint16(0xFFD8); // SOI
      stream << quint16(0xFFE0); // APP0
      stream << quint16(jfif.size() + 2);
      device->write(jfif);
      stream << quint16(0xFFE1); // APP1
      stream << quint16(exif.size() + 2);
      device->write(exif);
      /// stream << quint16(0xFFE0); // APP0 BUGG
      stream << segmentId;
      stream << segmentLength;
      device->write(remainder);
    }
  }
  else if (segmentId == 0xFFE1)
  {
    QByteArray oldExif = device->read(segmentLength - 2);

    if (!oldExif.startsWith("Exif"))
      return false;

    int dSize = oldExif.size() - exif.size();

    if (dSize > 0)
      exif += QByteArray(dSize, '\0');

    QByteArray remainder = device->readAll();

    device->seek(0);
    stream << quint16(0xFFD8); // SOI
    stream << quint16(0xFFE1); // APP1
    stream << quint16(exif.size() + 2);
    device->write(exif);
    device->write(remainder);
  }
  else
  {
    QByteArray remainder = device->readAll();

    device->seek(0);

    stream << quint16(0xFFD8); // SOI
    stream << quint16(0xFFE1); // APP1
    stream << quint16(exif.size() + 2);
    device->write(exif);
    stream << segmentId;
    stream << segmentLength;
    device->write(remainder);
  }

  return true;
}

QSysInfo::Endian QExifImageHeader::byteOrder() const
{
  return d->byteOrder;
}

quint32 QExifImageHeader::sizeOf(const QExifValue& value) const
{
  switch (value.type())
  {
  case QExifValue::Byte:
  case QExifValue::Undefined:
    return value.count() > 4 ? 12 + value.count() : 12;
  case QExifValue::Ascii:
    return value.count() > 4 ? 12 + value.count() : 12;
  case QExifValue::Short:
    return value.count() > 2 ? 12 + value.count() * sizeof(quint16) : 12;
  case QExifValue::Long:
  case QExifValue::SignedLong:
    return value.count() > 1 ? 12 + value.count() * sizeof(quint32) : 12;
  case QExifValue::Rational:
  case QExifValue::SignedRational:
    return value.count() > 0 ? 12 + value.count() * sizeof(quint32) * 2 : 12;
  default:
    return 0;
  }
}

template <typename T>
quint32 QExifImageHeader::calculateSize(const QMap<T, QExifValue>& values) const
{
  quint32 size = sizeof(quint16);

  for (const auto& value : values)
    size += sizeOf(value);

  return size;
}

qint64 QExifImageHeader::size() const
{
  if (d->size == -1)
  {
    d->size = 2                                  // Byte Order
              + 2                                // Marker
              + 4                                // Image Ifd offset
              + 12                               // ExifIfdPointer Ifd
              + 4                                // Thumbnail Ifd offset
              + calculateSize(d->imageIfdValues) // Image headers and values.
              + calculateSize(d->exifIfdValues); // Exif headers and values.

    if (!d->gpsIfdValues.isEmpty())
    {
      d->size += 12                                // GpsInfoIfdPointer Ifd
                 + calculateSize(d->gpsIfdValues); // Gps headers and values.
    }

    if (!d->thumbnailData.isEmpty())
    {
      d->size += 2                          // Thumbnail Ifd count
                 + 12                       // Compression Ifd
                 + 20                       // XResolution Ifd
                 + 20                       // YResolution Ifd
                 + 12                       // ResolutionUnit Ifd
                 + 12                       // JpegInterchangeFormat Ifd
                 + 12                       // JpegInterchangeFormatLength Ifd
                 + d->thumbnailData.size(); // Thumbnail data size.
    }
  }

  return d->size;
}

void QExifImageHeader::clear()
{
  d->imageIfdValues.clear();
  d->exifIfdValues.clear();
  d->gpsIfdValues.clear();
  d->thumbnailData.clear();
  d->size = -1;
}

QList<QExifImageHeader::ImageTag> QExifImageHeader::imageTags() const
{
  return d->imageIfdValues.keys();
}

QList<QExifImageHeader::ExifExtendedTag> QExifImageHeader::extendedTags() const
{
  return d->exifIfdValues.keys();
}

QList<QExifImageHeader::GpsTag> QExifImageHeader::gpsTags() const
{
  return d->gpsIfdValues.keys();
}

bool QExifImageHeader::contains(ImageTag tag) const
{
  return d->imageIfdValues.contains(tag);
}

/*!
    Returns true if a header contains a a value for an extended EXIF \a tag and false otherwise.
    */
bool QExifImageHeader::contains(ExifExtendedTag tag) const
{
  return d->exifIfdValues.contains(tag);
}

bool QExifImageHeader::contains(GpsTag tag) const
{
  return d->gpsIfdValues.contains(tag);
}

void QExifImageHeader::remove(ImageTag tag)
{
  d->imageIfdValues.remove(tag);
  d->size = -1;
}

void QExifImageHeader::remove(ExifExtendedTag tag)
{
  d->exifIfdValues.remove(tag);
  d->size = -1;
}

void QExifImageHeader::remove(GpsTag tag)
{
  d->gpsIfdValues.remove(tag);
  d->size = -1;
}

QExifValue QExifImageHeader::operator[](ImageTag tag) const
{
  return d->imageIfdValues.value(tag);
}

QExifValue QExifImageHeader::operator[](ExifExtendedTag tag) const
{
  return d->exifIfdValues.value(tag);
}

QExifValue QExifImageHeader::operator[](GpsTag tag) const
{
  return d->gpsIfdValues.value(tag);
}

void QExifImageHeader::setValue(ImageTag tag, const QExifValue& value)
{
  d->imageIfdValues[tag] = value;
  d->size = -1;
}

void QExifImageHeader::setValue(ExifExtendedTag tag, const QExifValue& value)
{
  d->exifIfdValues[tag] = value;
  d->size = -1;
}

void QExifImageHeader::setValue(GpsTag tag, const QExifValue& value)
{
  d->gpsIfdValues[tag] = value;
  d->size = -1;
}

void QExifImageHeader::setLatWithRef(GpsTag tag, double valueDeg, GpsTag refTag)
{
  PosValDeg v(valueDeg);

  if (valueDeg > 0)
    setValue(refTag, "N");
  else
    setValue(refTag, "S");
  setValue(tag, v);
}

void QExifImageHeader::setLngWithRef(GpsTag tag, double valueDeg, GpsTag refTag)
{
  PosValDeg v(valueDeg);
  if (valueDeg > 0)
    setValue(refTag, "E");
  else
    setValue(refTag, "W");

  setValue(tag, v);
}

double QExifValue::toLat(const QExifValue& tRef) const
{
  if (tRef.toString() == "S")
    return -toDouble();

  return toDouble();
}

double QExifValue::toLng(const QExifValue& tRef) const
{
  if (tRef.toString() == "W")
    return -toDouble();

  return toDouble();
}

double QExifValue::toDouble() const
{
  if (d->count == 1)
  {
    switch (d->type)
    {
    case Byte:
      return static_cast<const QExifByteValuePrivate*>(d.constData())->value.at(0);
    case Short:
      return static_cast<const QExifShortValuePrivate*>(d.constData())->value.at(0);
    case Long:
      return static_cast<const QExifLongValuePrivate*>(d.constData())->value.at(0);
    case SignedLong:
      return static_cast<const QExifSignedLongValuePrivate*>(d.constData())->value.at(0);
    case Rational:
    {
      auto r = static_cast<const QExifRationalValuePrivate*>(d.constData())->value.at(0);
      return r.first / (double)r.second;
    }
    case SignedRational:
    {
      auto r = static_cast<const QExifSignedRationalValuePrivate*>(d.constData())->value.at(0);
      return r.first / (double)r.second;
    }
    case Undefined:
      return -1;
    }

    return 0;
  }

  if (d->count == 3)
  {
    auto& tVec = static_cast<const QExifRationalValuePrivate*>(d.constData())->value;
    double fDegrees = double(tVec[0].first) / double(tVec[0].second);
    double fMinutes = double(tVec[1].first) / double(tVec[1].second);
    double fSeconds = double(tVec[2].first) / double(tVec[2].second);
    return fDegrees + fMinutes / 60.0 + fSeconds / 3600;
  }
  return 0;
}

bool QExifImageHeader::HasThumb() const
{
  return (d->thumbnailData.size() > 100);
}

QImage QExifImageHeader::thumbnail() const
{
  QImage image;
  image.loadFromData(d->thumbnailData, "JPG");

  if (!d->thumbnailOrientation.isNull())
  {
    switch (d->thumbnailOrientation.toShort())
    {
    case 1:
      return image;
    case 2:
      return image.transformed(QTransform().rotate(180, Qt::YAxis));
    case 3:
      return image.transformed(QTransform().rotate(180, Qt::ZAxis));
    case 4:
      return image.transformed(QTransform().rotate(180, Qt::XAxis));
    case 5:
      return image.transformed(QTransform().rotate(180, Qt::YAxis).rotate(90, Qt::ZAxis));
    case 6:
      return image.transformed(QTransform().rotate(90, Qt::ZAxis));
    case 7:
      return image.transformed(QTransform().rotate(180, Qt::XAxis).rotate(90, Qt::ZAxis));
    case 8:
      return image.transformed(QTransform().rotate(270, Qt::ZAxis));
    }
  }

  return image;
}

void QExifImageHeader::setThumbnail(const QImage& thumbnail)
{
  if (!thumbnail.isNull())
  {
    QBuffer buffer;

    if (buffer.open(QIODevice::WriteOnly) && thumbnail.save(&buffer, "JPG"))
    {
      buffer.close();
      d->thumbnailSize = thumbnail.size();
      d->thumbnailData = buffer.data();
      d->thumbnailOrientation = QExifValue();
    }
  }
  else
  {
    d->thumbnailSize = QSize();
    d->thumbnailData = QByteArray();
  }
  d->size = -1;
}

QByteArray QExifImageHeader::extractExif(QIODevice* device) const
{
  QDataStream stream(device);
  stream.setByteOrder(QDataStream::BigEndian);

  if (device->read(2) != "\xFF\xD8")
    return QByteArray();

  while (device->read(2) != "\xFF\xE1")
  {
    if (device->atEnd())
      return QByteArray();

    quint16 length;
    stream >> length;
    device->seek(device->pos() + length - 2);
  }

  quint16 length;

  stream >> length;

  if (device->read(4) != "Exif")
    return QByteArray();

  device->read(2);

  return device->read(length - 8);
}

QList<ExifIfdHeader> QExifImageHeader::readIfdHeaders(QDataStream& stream) const
{
  QList<ExifIfdHeader> headers;
  quint16 count;
  stream >> count;

  for (quint16 i = 0; i < count; i++)
  {
    ExifIfdHeader header;
    stream >> header;
    headers.append(header);
  }

  return headers;
}

QExifValue QExifImageHeader::readIfdValue(QDataStream& stream, int startPos,
                                          const ExifIfdHeader& header) const
{
  switch (header.type)
  {
  case QExifValue::Byte:
  {
    QVector<quint8> value(header.count);

    if (header.count > 4)
    {
      stream.device()->seek(startPos + header.offset);

      for (quint32 i = 0; i < header.count; i++)
        stream >> value[i];
    }
    else
    {
      for (quint32 i = 0; i < header.count; i++)
        value[i] = header.offsetBytes[i];
    }
    return QExifValue(value);
  }
  case QExifValue::Undefined:
    if (header.count > 4)
    {
      stream.device()->seek(startPos + header.offset);
      return QExifValue(stream.device()->read(header.count));
    }
    else
    {
      return QExifValue(QByteArray::fromRawData(header.offsetAscii, header.count));
    }
  case QExifValue::Ascii:
    if (header.count > 4)
    {
      stream.device()->seek(startPos + header.offset);
      QByteArray ascii = stream.device()->read(header.count);
      return QExifValue(QString::fromLatin1(ascii.constData(), ascii.size() - 1));
    }
    else
    {
      return QExifValue(QString::fromLatin1(header.offsetAscii, header.count - 1));
    }
  case QExifValue::Short:
  {
    QVector<quint16> value(header.count);
    if (header.count > 2)
    {
      stream.device()->seek(startPos + header.offset);
      for (quint32 i = 0; i < header.count; i++)
        stream >> value[i];
    }
    else
    {
      for (quint32 i = 0; i < header.count; i++)
        value[i] = header.offsetShorts[i];
    }
    return QExifValue(value);
  }
  case QExifValue::Long:
  {
    QVector<quint32> value(header.count);
    if (header.count > 1)
    {
      stream.device()->seek(startPos + header.offset);
      for (quint32 i = 0; i < header.count; i++)
        stream >> value[i];
    }
    else if (header.count == 1)
    {
      value[0] = header.offset;
    }
    return QExifValue(value);
  }
  case QExifValue::SignedLong:
  {
    QVector<qint32> value(header.count);
    if (header.count > 1)
    {
      stream.device()->seek(startPos + header.offset);
      for (quint32 i = 0; i < header.count; i++)
        stream >> value[i];
    }
    else if (header.count == 1)
    {
      value[0] = header.offset;
    }
    return QExifValue(value);
  }
  break;
  case QExifValue::Rational:
  {
    QVector<QExifURational> value(header.count);
    stream.device()->seek(startPos + header.offset);
    for (quint32 i = 0; i < header.count; i++)
      stream >> value[i];

    return QExifValue(value);
  }
  case QExifValue::SignedRational:
  {
    QVector<QExifSRational> value(header.count);
    stream.device()->seek(startPos + header.offset);
    for (quint32 i = 0; i < header.count; i++)
      stream >> value[i];

    return QExifValue(value);
  }
  default:
    qWarning() << "Invalid Ifd Type" << header.type;
    return QExifValue();
  }
}

template <typename T>
QMap<T, QExifValue> QExifImageHeader::readIfdValues(QDataStream& stream, int startPos,
                                                    const QList<ExifIfdHeader>& headers) const
{
  QMap<T, QExifValue> values;

  // This needs to be non-const so it works with gcc3
  QList<ExifIfdHeader> headers_ = headers;
  foreach (const ExifIfdHeader& header, headers_)
    values[T(header.tag)] = readIfdValue(stream, startPos, header);

  return values;
}

template <typename T>
QMap<T, QExifValue> QExifImageHeader::readIfdValues(QDataStream& stream, int startPos,
                                                    const QExifValue& pointer) const
{
  if (pointer.type() == QExifValue::Long && pointer.count() == 1)
  {
    stream.device()->seek(startPos + pointer.toLong());
    QList<ExifIfdHeader> headers = readIfdHeaders(stream);
    return readIfdValues<T>(stream, startPos, headers);
  }
  else
  {
    return QMap<T, QExifValue>();
  }
}

/*!
    Reads the contents of an EXIF header from an I/O \a device.
    Returns true if the header was read and false otherwise.
    \sa loadFromJpeg(), write()
    */
bool QExifImageHeader::read(QIODevice* device)
{
  clear();
  int startPos = device->pos();
  QDataStream stream(device);
  QByteArray byteOrder = device->read(2);

  if (byteOrder == "II")
  {
    d->byteOrder = QSysInfo::LittleEndian;
    stream.setByteOrder(QDataStream::LittleEndian);
  }
  else if (byteOrder == "MM")
  {
    d->byteOrder = QSysInfo::BigEndian;
    stream.setByteOrder(QDataStream::BigEndian);
  }
  else
  {
    return false;
  }

  quint16 id;
  quint32 offset;

  stream >> id;
  stream >> offset;

  if (id != 0x002A)
    return false;

  device->seek(startPos + offset);

  QList<ExifIfdHeader> headers = readIfdHeaders(stream);

  stream >> offset;
  d->imageIfdValues = readIfdValues<ImageTag>(stream, startPos, headers);

  QExifValue exifIfdPointer = d->imageIfdValues.take(ImageTag(ExifIfdPointer));
  QExifValue gpsIfdPointer = d->imageIfdValues.take(ImageTag(GpsInfoIfdPointer));

  d->exifIfdValues = readIfdValues<ExifExtendedTag>(stream, startPos, exifIfdPointer);
  d->gpsIfdValues = readIfdValues<GpsTag>(stream, startPos, gpsIfdPointer);

  d->exifIfdValues.remove(ExifExtendedTag(InteroperabilityIfdPointer));

  if (offset)
  {
    device->seek(startPos + offset);

    QMap<quint16, QExifValue> thumbnailIfdValues =
        readIfdValues<quint16>(stream, startPos, readIfdHeaders(stream));

    QExifValue jpegOffset = thumbnailIfdValues.value(JpegInterchangeFormat);
    QExifValue jpegLength = thumbnailIfdValues.value(JpegInterchangeFormatLength);

    if (jpegOffset.type() == QExifValue::Long && jpegOffset.count() == 1 &&
        jpegLength.type() == QExifValue::Long && jpegLength.count() == 1)
    {
      device->seek(startPos + jpegOffset.toLong());

      d->thumbnailData = device->read(jpegLength.toLong());
      d->thumbnailXResolution = thumbnailIfdValues.value(XResolution);
      d->thumbnailYResolution = thumbnailIfdValues.value(YResolution);
      d->thumbnailResolutionUnit = thumbnailIfdValues.value(ResolutionUnit);
      d->thumbnailOrientation = thumbnailIfdValues.value(Orientation);
    }
  }
  return true;
}

quint32 QExifImageHeader::writeExifHeader(QDataStream& stream, quint16 tag, const QExifValue& value,
                                          quint32 offset) const
{
  stream << tag;
  stream << quint16(value.type());
  stream << quint32(value.count());

  switch (value.type())
  {
  case QExifValue::Byte:
    if (value.count() <= 4)
    {
      foreach (quint8 byte, value.toByteVector())
        stream << byte;
      for (int j = value.count(); j < 4; j++)
        stream << quint8(0);
    }
    else
    {
      stream << offset;
      offset += value.count();
    }
    break;
  case QExifValue::Undefined:
    if (value.count() <= 4)
    {
      stream.device()->write(value.toByteArray());

      if (value.count() < 4)
        stream.writeRawData("\0\0\0\0", 4 - value.count());
    }
    else
    {
      stream << offset;
      offset += value.count();
    }
    break;
  case QExifValue::Ascii:
    if (value.count() <= 4)
    {
      QByteArray bytes = value.toByteArray();
      stream.writeRawData(bytes.constData(), value.count());
      if (value.count() < 4)
        stream.writeRawData("\0\0\0\0", 4 - value.count());
    }
    else
    {
      stream << offset;
      offset += value.count();
    }
    break;
  case QExifValue::Short:
    if (value.count() <= 2)
    {
      foreach (quint16 shrt, value.toShortVector())
        stream << shrt;
      for (int j = value.count(); j < 2; j++)
        stream << quint16(0);
    }
    else
    {
      stream << offset;
      offset += value.count() * sizeof(quint16);
    }
    break;
  case QExifValue::Long:
    if (value.count() == 0)
    {
      stream << quint32(0);
    }
    else if (value.count() == 1)
    {
      stream << value.toLong();
    }
    else
    {
      stream << offset;

      offset += value.count() * sizeof(quint32);
    }
    break;
  case QExifValue::SignedLong:
    if (value.count() == 0)
    {
      stream << quint32(0);
    }
    else if (value.count() == 1)
    {
      stream << value.toSignedLong();
    }
    else
    {
      stream << offset;
      offset += value.count() * sizeof(qint32);
    }
    break;
  case QExifValue::Rational:
    if (value.count() == 0)
    {
      stream << quint32(0);
    }
    else
    {
      stream << offset;
      offset += value.count() * sizeof(quint32) * 2;
    }
    break;
  case QExifValue::SignedRational:
    if (value.count() == 0)
    {
      stream << quint32(0);
    }
    else
    {
      stream << offset;
      offset += value.count() * sizeof(qint32) * 2;
    }
    break;
  default:
    qWarning() << "Invalid Ifd Type" << value.type();
    stream << quint32(0);
  }
  return offset;
}

void QExifImageHeader::writeExifValue(QDataStream& stream, const QExifValue& value) const
{
  switch (value.type())
  {
  case QExifValue::Byte:
    if (value.count() > 4)
      foreach (quint8 byte, value.toByteVector())
        stream << byte;
    break;
  case QExifValue::Undefined:
    if (value.count() > 4)
      stream.device()->write(value.toByteArray());
    break;
  case QExifValue::Ascii:
    if (value.count() > 4)
    {
      QByteArray bytes = value.toByteArray();
      stream.writeRawData(bytes.constData(), bytes.size() + 1);
    }
    break;
  case QExifValue::Short:
    if (value.count() > 2)
      foreach (quint16 shrt, value.toShortVector())
        stream << shrt;
    break;
  case QExifValue::Long:
    if (value.count() > 1)
      foreach (quint32 lng, value.toLongVector())
        stream << lng;
    break;
  case QExifValue::SignedLong:
    if (value.count() > 1)
      foreach (qint32 lng, value.toSignedLongVector())
        stream << lng;
    break;
  case QExifValue::Rational:
    if (value.count() > 0)
      foreach (QExifURational rational, value.toRationalVector())
        stream << rational;
    break;
  case QExifValue::SignedRational:
    if (value.count() > 0)
      foreach (QExifSRational rational, value.toSignedRationalVector())
        stream << rational;
    break;
  default:
    qWarning() << "Invalid Ifd Type" << value.type();
    break;
  }
}

template <typename T>
quint32 QExifImageHeader::writeExifHeaders(QDataStream& stream, const QMap<T, QExifValue>& values,
                                           quint32 offset) const
{
  offset += values.count() * 12;

  for (typename QMap<T, QExifValue>::const_iterator i = values.constBegin(); i != values.constEnd();
       i++)
    offset = writeExifHeader(stream, i.key(), i.value(), offset);

  return offset;
}

template <typename T>
void QExifImageHeader::writeExifValues(QDataStream& stream, const QMap<T, QExifValue>& values) const
{
  for (typename QMap<T, QExifValue>::const_iterator i = values.constBegin(); i != values.constEnd();
       i++)
    writeExifValue(stream, i.value());
}

qint64 QExifImageHeader::write(QIODevice* device) const
{
  QDataStream stream(device);

  if (d->byteOrder == QSysInfo::LittleEndian)
  {
    stream.setByteOrder(QDataStream::LittleEndian);
    device->write("II", 2);
    device->write("\x2A\x00", 2);
    device->write("\x08\x00\x00\x00", 4);
  }
  else if (d->byteOrder == QSysInfo::BigEndian)
  {
    stream.setByteOrder(QDataStream::BigEndian);
    device->write("MM", 2);
    device->write("\x00\x2A", 2);
    device->write("\x00\x00\x00\x08", 4);
  }

  quint16 count = d->imageIfdValues.count() + 1;
  quint32 offset = 26;

  if (!d->gpsIfdValues.isEmpty())
  {
    count++;
    offset += 12;
  }

  stream << count;

  offset = writeExifHeaders(stream, d->imageIfdValues, offset);

  quint32 exifIfdOffset = offset;

  stream << quint16(ExifIfdPointer);
  stream << quint16(QExifValue::Long);
  stream << quint32(1);
  stream << exifIfdOffset;
  offset += calculateSize(d->exifIfdValues);

  quint32 gpsIfdOffset = offset;

  if (!d->gpsIfdValues.isEmpty())
  {
    stream << quint16(GpsInfoIfdPointer);
    stream << quint16(QExifValue::Long);
    stream << quint32(1);
    stream << gpsIfdOffset;
    d->imageIfdValues.insert(ImageTag(GpsInfoIfdPointer), QExifValue(offset));
    offset += calculateSize(d->gpsIfdValues);
  }

  if (!d->thumbnailData.isEmpty())
    stream << offset; // Write offset to thumbnail Ifd.
  else
    stream << quint32(0);

  writeExifValues(stream, d->imageIfdValues);
  stream << quint16(d->exifIfdValues.count());

  exifIfdOffset += 2; // BUGG ???

  writeExifHeaders(stream, d->exifIfdValues, exifIfdOffset);
  writeExifValues(stream, d->exifIfdValues);
  if (!d->gpsIfdValues.isEmpty())
  {
    gpsIfdOffset += 2; // BUGG ???
    stream << quint16(d->gpsIfdValues.count());

    writeExifHeaders(stream, d->gpsIfdValues, gpsIfdOffset);
    writeExifValues(stream, d->gpsIfdValues);
  }

  if (!d->thumbnailData.isEmpty())
  {
    offset += 86;

    stream << quint16(7);

    QExifValue xResolution = d->thumbnailXResolution.isNull() ? QExifValue(QExifURational(72, 1))
                                                              : d->thumbnailXResolution;

    QExifValue yResolution = d->thumbnailYResolution.isNull() ? QExifValue(QExifURational(72, 1))
                                                              : d->thumbnailYResolution;

    QExifValue resolutionUnit =
        d->thumbnailResolutionUnit.isNull() ? QExifValue(quint16(2)) : d->thumbnailResolutionUnit;

    QExifValue orientation =
        d->thumbnailOrientation.isNull() ? QExifValue(quint16(0)) : d->thumbnailOrientation;

    writeExifHeader(stream, Compression, QExifValue(quint16(6)), offset);

    offset = writeExifHeader(stream, XResolution, xResolution, offset);
    offset = writeExifHeader(stream, YResolution, yResolution, offset);

    writeExifHeader(stream, ResolutionUnit, resolutionUnit, offset);
    writeExifHeader(stream, Orientation, orientation, offset);
    writeExifHeader(stream, JpegInterchangeFormat, QExifValue(offset), offset);
    writeExifHeader(stream, JpegInterchangeFormatLength,
                    QExifValue(quint32(d->thumbnailData.size())), offset);

    writeExifValue(stream, xResolution);
    writeExifValue(stream, yResolution);
    device->write(d->thumbnailData);
    offset += d->thumbnailData.size();
  }
  d->size = offset;

  return offset;
}
