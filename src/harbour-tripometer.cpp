
#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include "libsailfishsilica/silicatheme.h"
#include "osm-gps-map/osm-gps-map-qt.h"
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QSettings>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtPositioning/QtPositioning>
#include <sailfishapp/sailfishapp.h>

#include "Utils.h"
#include "infolistmodel.h"
#include "trackmodel.h"

QObject* g_pTheTrackModel = nullptr;
QObject* g_pRootObject = nullptr;
// To be shared with the c implementation
int g_nFontSizePx = 0;
int g_nSkipDraw = 0;

void MssMessageOutput(QtMsgType, const QMessageLogContext&, const QString& msg)
{
  g_message("%s", msg.toLatin1().constData());
}

int main(int argc, char* argv[])
{

  // Making gpx
  // https://www.komoot.com/tour/2288497802


  // https://studio.app-mockup.com

  // https://www.appstorescreenshot.com

  // Xperia 10 III /home/defaultuser
  //
  // /home/nemo/Documents/pikefight
  //  cashe /home/nemo/.cache/harbour-pikefight
  // settings file  "/home/nemo/.config/harbour-pikefight/PikeFight.conf"
  // local storage "/home/nemo/.local/share/harbour-pikefight/harbour-pikefight"
  qInstallMessageHandler(MssMessageOutput);
  StopWatch oSW("Start pike application %1");
  QGuiApplication* app = SailfishApp::application(argc, argv);
  oSW.Stop();
  QGuiApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
  QQuickView* pU = SailfishApp::createView();
  oSW.Stop();
  QQmlContext* pContext = pU->rootContext();
  InfoListModel* pInfoListModel = new InfoListModel;
  pContext->setContextProperty("idListModel", pInfoListModel);
  oSW.Stop();
  auto pTM = new TrackModel;
  g_pTheTrackModel = pTM;
  auto pTMF = new TrackModelFiltered;
  pTMF->setSourceModel(pTM);
  oSW.Stop();
  pContext->setContextProperty("idTrackModel", pTM);
  pContext->setContextProperty("idTrackModelFiltered", pTMF);
  MssListModel* pSearchResultModel = new MssListModel("fullName", "lat", "lo", "type", "ref");
  oSW.Stop();
  pSearchResultModel->Init(1);
  pContext->setContextProperty("pikeFightDocFolder", StorageDir());
  pContext->setContextProperty("oCaptureThumbMaker", new CaptureThumbMaker(app));
  pContext->setContextProperty("oFileMgr", new FileMgr());
  pContext->setContextProperty("idSearchResultModel", pSearchResultModel);
  qRegisterMetaType<QGeoCoordinate>("QGeoCoordinate");
  ScreenCapture::SetView(pU);
  qmlRegisterType<Maep::GpsMap>("harbour.tripometer", 1, 0, "GpsMap");
  qmlRegisterType<ScreenCapture>("harbour.tripometer", 1, 0, "ScreenCapture");

  qmlRegisterType<QQuickFolderListModel>("harbour.tripometer", 1, 0, "FolderListModel");
  qmlRegisterType<Maep::Track>("harbour.tripometer", 1, 0, "Track");

  pU->engine()->addImageProvider("capturedImage", new ScreenCapturedImg());
  QObject::connect(pU->engine(), &QQmlEngine::quit, app, &QGuiApplication::quit);

  auto pTheme = Silica::Theme::instance();
  g_nFontSizePx = pTheme->fontSizeTiny();

  qDebug() << "Tiny fontsize px =" << g_nFontSizePx;

  pU->setSource(SailfishApp::pathTo("qml/harbour-tripometer.qml"));
  pU->showFullScreen();
  oSW.Stop();
  g_pRootObject = pU->rootObject();

  QSettings oSettings("harbour-pikefight", "PikeFight");
  qDebug() << "settings file " << oSettings.fileName();

  QString filePath =
      QStandardPaths::writableLocation(QStandardPaths::StandardLocation::AppLocalDataLocation);

  qDebug() << "local storage " << filePath;
  pU->rootObject()->setProperty("nUnit", oSettings.value("nUnit", 1));
  pInfoListModel->klicked2(5);
  oSW.Stop();

  pU->rootObject()->setProperty("bEnableAis", oSettings.value("bEnableAis", false));
  pU->rootObject()->setProperty("nMinSize", oSettings.value("nMinSize", 60));
  pU->rootObject()->setProperty("nNrTeams", oSettings.value("nNrTeams", 2));
  pU->rootObject()->setProperty("nPikesCounted", oSettings.value("nPikesCounted", 6));
  pU->rootObject()->setProperty("nExportMapW", oSettings.value("nExportMapW", 2480));
  pU->rootObject()->setProperty("nExportMapH", oSettings.value("nExportMapH", 3508));

  pU->rootObject()->setProperty(
      "ocTeamName",
      oSettings.value("ocTeamName", QStringList({"Pike Report", "Team 1", "Team 2", "Team 3"})));

  MssTimer oTimer([] {
    if (g_pRootObject == nullptr)
      return;
    if (g_pRootObject->property("bScreenallwaysOn").toBool() == true)
      ScreenOn(true);
  });

  oTimer.Start(1000 * 30);

  oSW.Stop();
  pInfoListModel->klicked2(5);
  oSW.Stop();
  mssutils::MkCache();

  app->exec();

  oSettings.setValue("ocTeamName", pU->rootObject()->property("ocTeamName"));

  oSettings.setValue("nPikesCounted", pU->rootObject()->property("nPikesCounted"));
  oSettings.setValue("nUnit", pU->rootObject()->property("nUnit"));
  oSettings.setValue("nMinSize", pU->rootObject()->property("nMinSize"));
  oSettings.setValue("nNrTeams", pU->rootObject()->property("nNrTeams"));
  oSettings.setValue("nExportMapW", pU->rootObject()->property("nExportMapW"));
  oSettings.setValue("nExportMapH", pU->rootObject()->property("nExportMapH"));
  oSettings.setValue("bEnableAis", pU->rootObject()->property("bEnableAis"));

  oSettings.sync();

  ScreenOn(false);
  oTimer.Stop();
  delete pU;
  qDebug() << "Exit";
}
