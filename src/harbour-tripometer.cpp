
#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <sailfishapp.h>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QSettings>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtPositioning/QtPositioning>

#include "osm-gps-map/osm-gps-map-qt.h"

// #include <QtQml/qqml>

//

#include "Utils.h"
#include "infolistmodel.h"
#include "trackmodel.h"

QObject* g_pTheTrackModel;
QObject* g_pTheMap;

int main(int argc, char* argv[]) {

  // /home/nemo/Documents/pikefight
  //  cashe /home/nemo/.cache/harbour-pikefight
  // settings file  "/home/nemo/.config/harbour-pikefight/PikeFight.conf"
  // local storage "/home/nemo/.local/share/harbour-pikefight/harbour-pikefight"
  // To display the view, call "show()" (will show fullscreen on device).

  // QStringList oc = QGeoPositionInfoSource::availableSources();
  // QtPositioning::

  // qmlRegisterType<InfoListModel>("harbour.tripometer", 1, 0,
  // "InfoListModel");
  StopWatch oSW("Start %1");
  QGuiApplication* app = SailfishApp::application(argc, argv);
  QQuickView* pU = SailfishApp::createView();
  QQmlContext* pContext = pU->rootContext();
  InfoListModel* pInfoListModel = new InfoListModel;
  pContext->setContextProperty("idListModel", pInfoListModel);

  g_pTheTrackModel = new TrackModel;
  pContext->setContextProperty("idTrackModel", g_pTheTrackModel);
  MssListModel* pSearchResultModel =
      new MssListModel("name", "lat", "lo", "type");
  oSW.Stop();
  pSearchResultModel->Init(1);
  pContext->setContextProperty("oImageThumb", new ImageThumb(app));
  pContext->setContextProperty("oFileMgr", new FileMgr());
  pContext->setContextProperty("idSearchResultModel", pSearchResultModel);
  qRegisterMetaType<QGeoCoordinate>("QGeoCoordinate");
  ScreenCapture::SetView(pU);
  qmlRegisterType<Maep::GpsMap>("harbour.tripometer", 1, 0, "GpsMap");
  qmlRegisterType<ScreenCapture>("harbour.tripometer", 1, 0, "ScreenCapture");

  qmlRegisterType<Maep::Track>("harbour.tripometer", 1, 0, "Track");
  qmlRegisterType<Maep::GeonamesPlace>("harbour.tripometer", 1, 0,
                                       "GeonamesPlace");
  // QObject::connect(pU->engine(),&QQmlEngine::quit, app ,
  // &QGuiApplication::quit,Qt::DirectConnection);
  pU->setSource(SailfishApp::pathTo("qml/harbour-tripometer.qml"));
  pU->showFullScreen();
  oSW.Stop();
  InfoListModel::m_pRoot = pU->rootObject();

  QSettings oSettings("harbour-pikefight", "PikeFight");
  qDebug() << "settings file " << oSettings.fileName();

  QString filePath = QStandardPaths::writableLocation(
      QStandardPaths::StandardLocation::AppLocalDataLocation);

  qDebug() << "local storage " << filePath;
  pU->rootObject()->setProperty("nUnit", oSettings.value("nUnit", 1));
  pInfoListModel->klicked2(5);
  oSW.Stop();

  pU->rootObject()->setProperty("nMinSize", oSettings.value("nMinSize", 60));
  pU->rootObject()->setProperty("nNrTeams", oSettings.value("nNrTeams", 2));
  pU->rootObject()->setProperty("nPikesCounted",
                                oSettings.value("nPikesCounted", 6));


  pU->rootObject()->setProperty(
      "ocTeamName",
      oSettings.value("ocTeamName",
                      QStringList({"Pike Report", "Team 1", "Team 2", "Team 3"})));
  MssTimer oTimer([] {
    if (InfoListModel::m_pRoot == 0)
      return;
    if (InfoListModel::m_pRoot->property("bScreenallwaysOn").toBool() == true)
      ScreenOn(true);
  });
  oTimer.Start(1000 * 30);
  oSW.Stop();
  pInfoListModel->klicked2(5);
  oSW.Stop();
  mssutils::MkCache();

  app->exec();
  oSettings.setValue("ocTeamName", pU->rootObject()->property("ocTeamName"));

  oSettings.setValue("nPikesCounted",
                     pU->rootObject()->property("nPikesCounted"));
  oSettings.setValue("nUnit", pU->rootObject()->property("nUnit"));
  oSettings.setValue("nMinSize", pU->rootObject()->property("nMinSize"));
  oSettings.setValue("nNrTeams", pU->rootObject()->property("nNrTeams"));

  oSettings.sync();

  ScreenOn(false);
  oTimer.Stop();
  delete pU;
  qDebug() << "Exit";
}
