
#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QSettings>
#include <QtPositioning/QGeoPositionInfoSource>
#include <QtPositioning/QtPositioning>
#include <sailfishapp.h>

#include "osm-gps-map/osm-gps-map-qt.h"

// #include <QtQml/qqml>

//

#include "Utils.h"
#include "infolistmodel.h"
#include "trackmodel.h"

QObject* g_pTheTrackModel;
QObject* g_pTheMap;

int main(int argc, char* argv[])
{
    // SailfishApp::main() will display "qml/template.qml", if you need more
    // control over initialization, you can use:
    //
    //   - SailfishApp::application(int, char *[]) to get the QGuiApplication *
    //   - SailfishApp::createView() to get a new QQuickView * instance
    //   - SailfishApp::pathTo(QString) to get a QUrl to a resource file
    //
    // To display the view, call "show()" (will show fullscreen on device).

    // QStringList oc = QGeoPositionInfoSource::availableSources();
    // QtPositioning::

    // qmlRegisterType<InfoListModel>("harbour.tripometer", 1, 0,
    // "InfoListModel");
    QGuiApplication* app = SailfishApp::application(argc, argv);
    QQuickView* pU = SailfishApp::createView();
    QQmlContext* pContext = pU->rootContext();
    InfoListModel* pInfoListModel = new InfoListModel;
    pContext->setContextProperty("idListModel", pInfoListModel);

    g_pTheTrackModel = new TrackModel;
    pContext->setContextProperty("idTrackModel", g_pTheTrackModel);
    MssListModel* pSearchResultModel = new MssListModel("name", "lat", "lo", "type");
    pSearchResultModel->Init(1);
    pContext->setContextProperty("idSearchResultModel", pSearchResultModel);

    qRegisterMetaType<QGeoCoordinate>("QGeoCoordinate");
    qmlRegisterType<Maep::GpsMap>("harbour.tripometer", 1, 0, "GpsMap");
    qmlRegisterType<Maep::Track>("harbour.tripometer", 1, 0, "Track");
    qmlRegisterType<Maep::GeonamesPlace>(
        "harbour.tripometer", 1, 0, "GeonamesPlace");
    // QObject::connect(pU->engine(),&QQmlEngine::quit, app ,
    // &QGuiApplication::quit,Qt::DirectConnection);
    pU->setSource(SailfishApp::pathTo("qml/harbour-tripometer.qml"));
    pU->showFullScreen();
    InfoListModel::m_pRoot = pU->rootObject();

    QSettings oSettings("harbour-pikefight", "PikeFight");
    qDebug() << "settings file " << oSettings.fileName();
    pU->rootObject()->setProperty("nUnit", oSettings.value("nUnit", 1).toInt());
    int nPikesCounted = oSettings.value("nPikesCounted", 6).toInt();
    pInfoListModel->klicked2(5);
    pU->rootObject()->setProperty("nPikesCounted", nPikesCounted);

    MssTimer oTimer([] {
        if (InfoListModel::m_pRoot == 0)
            return;
        if (InfoListModel::m_pRoot->property("bScreenallwaysOn").toBool() == true)
            ScreenOn(true);
    });
    oTimer.Start(1000 * 30);
    pInfoListModel->klicked2(5);
    app->exec();
    oSettings.setValue("nPikesCounted",
        pU->rootObject()->property("nPikesCounted"));
    oSettings.setValue("nUnit", pU->rootObject()->property("nUnit"));
    oSettings.sync();

    ScreenOn(false);
    oTimer.Stop();
    delete pU;
    qDebug() << "Exit";
}
