#ifndef UTILS_H
#define UTILS_H

#include "src/track.h"
#include <QAbstractItemModel>
#include <QDateTime>
#include <QDir>
#include <QFileSystemWatcher>
#include <QImage>
#include <QMutex>
#include <QObject>
#include <QOrientationReading>
#include <QOrientationSensor>
#include <QQuickImageProvider>
#include <QQuickPaintedItem>
#include <QThread>
#include <QUrl>
#include <QVector>
#include <QWaitCondition>

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
  Q_INVOKABLE QString newImgName(QString s);
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

class QQmlContext;
class QModelIndex;

class QQuickFolderListModelPrivate;

class QQuickFolderListModel : public QAbstractListModel, public QQmlParserStatus
{
  Q_OBJECT
  Q_INTERFACES(QQmlParserStatus)

  Q_PROPERTY(QUrl folder READ folder WRITE setFolder NOTIFY folderChanged)
  Q_PROPERTY(QUrl rootFolder READ rootFolder WRITE setRootFolder)
  Q_PROPERTY(QUrl parentFolder READ parentFolder NOTIFY folderChanged)
  Q_PROPERTY(QStringList nameFilters READ nameFilters WRITE setNameFilters)
  Q_PROPERTY(SortField sortField READ sortField WRITE setSortField)
  Q_PROPERTY(bool sortReversed READ sortReversed WRITE setSortReversed)
  Q_PROPERTY(bool showFiles READ showFiles WRITE setShowFiles REVISION 1)
  Q_PROPERTY(bool showDirs READ showDirs WRITE setShowDirs)
  Q_PROPERTY(bool showDirsFirst READ showDirsFirst WRITE setShowDirsFirst)
  Q_PROPERTY(bool showDotAndDotDot READ showDotAndDotDot WRITE setShowDotAndDotDot)
  Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden REVISION 1)
  Q_PROPERTY(bool showOnlyReadable READ showOnlyReadable WRITE setShowOnlyReadable)
  Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
  QQuickFolderListModel(QObject* parent = 0);
  ~QQuickFolderListModel();

  enum Roles
  {
    FileNameRole = Qt::UserRole + 1,
    FilePathRole = Qt::UserRole + 2,
    FileBaseNameRole = Qt::UserRole + 3,
    FileSuffixRole = Qt::UserRole + 4,
    FileSizeRole = Qt::UserRole + 5,
    FileLastModifiedRole = Qt::UserRole + 6,
    FileLastReadRole = Qt::UserRole + 7,
    FileIsDirRole = Qt::UserRole + 8,
    FileUrlRole = Qt::UserRole + 9
  };

  virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
  virtual QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const;
  virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
  virtual QHash<int, QByteArray> roleNames() const;

  int count() const { return rowCount(QModelIndex()); }

  QUrl folder() const;
  void setFolder(const QUrl& folder);
  QUrl rootFolder() const;
  void setRootFolder(const QUrl& path);

  QUrl parentFolder() const;

  QStringList nameFilters() const;
  void setNameFilters(const QStringList& filters);

  enum SortField
  {
    Unsorted,
    Name,
    Time,
    Size,
    Type
  };
  SortField sortField() const;
  void setSortField(SortField field);
  Q_ENUMS(SortField)

  bool sortReversed() const;
  void setSortReversed(bool rev);

  bool showFiles() const;
  void setShowFiles(bool showFiles);
  bool showDirs() const;
  void setShowDirs(bool showDirs);
  bool showDirsFirst() const;
  void setShowDirsFirst(bool showDirsFirst);
  bool showDotAndDotDot() const;
  void setShowDotAndDotDot(bool on);
  bool showHidden() const;
  void setShowHidden(bool on);
  bool showOnlyReadable() const;
  void setShowOnlyReadable(bool on);

  Q_INVOKABLE bool isFolder(int index) const;
  Q_INVOKABLE QVariant get(int idx, const QString& property) const;
  Q_INVOKABLE int indexOf(const QUrl& file) const;

  virtual void classBegin();
  virtual void componentComplete();

  int roleFromString(const QString& roleName) const;

Q_SIGNALS:
  void folderChanged();
  void rowCountChanged() const;
  Q_REVISION(1) void countChanged() const;

private:
  Q_DISABLE_COPY(QQuickFolderListModel)
  Q_DECLARE_PRIVATE(QQuickFolderListModel)
  QScopedPointer<QQuickFolderListModelPrivate> d_ptr;

  Q_PRIVATE_SLOT(d_func(), void _q_directoryChanged(const QString& directory,
                                                    const QList<FileProperty>& list))
  Q_PRIVATE_SLOT(d_func(),
                 void _q_directoryUpdated(const QString& directory, const QList<FileProperty>& list,
                                          int fromIndex, int toIndex))
  Q_PRIVATE_SLOT(d_func(), void _q_sortFinished(const QList<FileProperty>& list))
};

class FileProperty
{
public:
  FileProperty(const QFileInfo& info)
  {
    mFileName = info.fileName();
    mFilePath = info.filePath();
    mBaseName = info.baseName();
    mSize = info.size();
    mSuffix = info.completeSuffix();
    mIsDir = info.isDir();
    mIsFile = info.isFile();
    mLastModified = info.lastModified();
    mLastRead = info.lastRead();
  }
  ~FileProperty() {}

  inline QString fileName() const { return mFileName; }
  inline QString filePath() const { return mFilePath; }
  inline QString baseName() const { return mBaseName; }
  inline qint64 size() const { return mSize; }
  inline QString suffix() const { return mSuffix; }
  inline bool isDir() const { return mIsDir; }
  inline bool isFile() const { return mIsFile; }
  inline QDateTime lastModified() const { return mLastModified; }
  inline QDateTime lastRead() const { return mLastRead; }

  inline bool operator!=(const FileProperty& fileInfo) const { return !operator==(fileInfo); }
  bool operator==(const FileProperty& property) const
  {
    return ((mFileName == property.mFileName) && (isDir() == property.isDir()));
  }

private:
  QString mFileName;
  QString mFilePath;
  QString mBaseName;
  QString mSuffix;
  qint64 mSize;
  bool mIsDir;
  bool mIsFile;
  QDateTime mLastModified;
  QDateTime mLastRead;
};

class FileInfoThread : public QThread
{
  Q_OBJECT

Q_SIGNALS:
  void directoryChanged(const QString& directory, const QList<FileProperty>& list) const;
  void directoryUpdated(const QString& directory, const QList<FileProperty>& list, int fromIndex,
                        int toIndex) const;
  void sortFinished(const QList<FileProperty>& list) const;

public:
  FileInfoThread(QObject* parent = 0);
  ~FileInfoThread();

  void clear();
  void removePath(const QString& path);
  void setPath(const QString& path);
  void setRootPath(const QString& path);
  void setSortFlags(QDir::SortFlags flags);
  void setNameFilters(const QStringList& nameFilters);
  void setShowFiles(bool show);
  void setShowDirs(bool showFolders);
  void setShowDirsFirst(bool show);
  void setShowDotAndDotDot(bool on);
  void setShowHidden(bool on);
  void setShowOnlyReadable(bool on);

public Q_SLOTS:
  void dirChanged(const QString& directoryPath);
  void updateFile(const QString& path);

protected:
  void run();
  void getFileInfos(const QString& path);
  void findChangeRange(const QList<FileProperty>& list, int& fromIndex, int& toIndex);

private:
  QMutex mutex;
  QWaitCondition condition;
  volatile bool abort;

  QFileSystemWatcher* watcher;

  QList<FileProperty> currentFileList;
  QDir::SortFlags sortFlags;
  QString currentPath;
  QString rootPath;
  QStringList nameFilters;
  bool needUpdate;
  bool folderUpdate;
  bool sortUpdate;
  bool showFiles;
  bool showDirs;
  bool showDirsFirst;
  bool showDotAndDotDot;
  bool showHidden;
  bool showOnlyReadable;
};

class QQuickFolderListModelPrivate
{
  Q_DECLARE_PUBLIC(QQuickFolderListModel)

public:
  QQuickFolderListModelPrivate(QQuickFolderListModel* q)
      : q_ptr(q), sortField(QQuickFolderListModel::Name), sortReversed(false), showFiles(true),
        showDirs(true), showDirsFirst(false), showDotAndDotDot(false), showOnlyReadable(false),
        showHidden(false)
  {
    nameFilters << QLatin1String("*");
  }

  QQuickFolderListModel* q_ptr;
  QUrl currentDir;
  QUrl rootDir;
  FileInfoThread fileInfoThread;
  QList<FileProperty> data;
  QHash<int, QByteArray> roleNames;
  QQuickFolderListModel::SortField sortField;
  QStringList nameFilters;
  bool sortReversed;
  bool showFiles;
  bool showDirs;
  bool showDirsFirst;
  bool showDotAndDotDot;
  bool showOnlyReadable;
  bool showHidden;

  ~QQuickFolderListModelPrivate() {}
  void init();
  void updateSorting();

  // private slots
  void _q_directoryChanged(const QString& directory, const QList<FileProperty>& list);
  void _q_directoryUpdated(const QString& directory, const QList<FileProperty>& list, int fromIndex,
                           int toIndex);
  void _q_sortFinished(const QList<FileProperty>& list);

  static QString resolvePath(const QUrl& path);
};
#endif // UTILS_H
