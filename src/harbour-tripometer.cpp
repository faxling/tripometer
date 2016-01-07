#3/*
  Copyright (C) 2013 Jolla Ltd.
  Contact: Thomas Perl <thomas.perl@jollamobile.com>
  All rights reserved.

  You may use this file under the terms of BSD license as follows:

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  * Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.
  * Neither the name of the Jolla Ltd nor the
  names of its contributors may be used to endorse or promote products
  derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR
  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  */

#ifdef QT_QML_DEBUG
#include <QtQuick>
#endif

#include <QGuiApplication>
#include <QQuickView>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQmlContext>
#include <sailfishapp.h>
#include <QtPositioning/QtPositioning>
#include <QtPositioning/QGeoPositionInfoSource>

#include "osm-gps-map/osm-gps-map-qt.h"

// #include <QtQml/qqml>

//


#include "infolistmodel.h"
#include "utils.h"

int main(int argc, char *argv[])
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
  //QtPositioning::




  // qmlRegisterType<InfoListModel>("harbour.tripometer", 1, 0, "InfoListModel");
  QGuiApplication *app = SailfishApp::application(argc, argv);
  QQuickView * pU = SailfishApp::createView();
  QQmlContext *pContext = pU->rootContext();
  InfoListModel* pInfoListModel =  new InfoListModel;
  pContext->setContextProperty("idListModel", pInfoListModel);

  qmlRegisterType<Maep::GpsMap>("harbour.maep.qt", 1, 0, "GpsMap");

  // QObject::connect(pU->engine(),&QQmlEngine::quit, app , &QGuiApplication::quit,Qt::DirectConnection);
  pU->setSource(SailfishApp::pathTo("qml/harbour-tripometer.qml"));
  pU->showFullScreen();
  InfoListModel::m_pRoot = pU->rootObject();

  MssTimer oTimer([] {
    if (InfoListModel::m_pRoot->property("bScreenallwaysOn").toBool()==true)
      ScreenOn(true);
  });
  oTimer.Start(1000*30);
  pInfoListModel->klicked2(5);
  app->exec();
  ScreenOn(false);
  qDebug() << "Exit";

}

