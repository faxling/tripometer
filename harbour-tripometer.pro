#TEMPLATE = app

TARGET = harbour-tripometer
DEPENDPATH += .
INCLUDEPATH += .
INCLUDEPATH += $$(MER_SSH_SHARED_TARGET)/$$(MER_SSH_TARGET_NAME)\usr\include\dconf
INCLUDEPATH += $$(MER_SSH_SHARED_TARGET)/$$(MER_SSH_TARGET_NAME)\usr\include\libxml2
INCLUDEPATH += $$(MER_SSH_SHARED_TARGET)/$$(MER_SSH_TARGET_NAME)\usr\include\glib-2.0
INCLUDEPATH += $$(MER_SSH_SHARED_TARGET)/$$(MER_SSH_TARGET_NAME)\usr\include\cairo
INCLUDEPATH += $$(MER_SSH_SHARED_TARGET)/$$(MER_SSH_TARGET_NAME)/usr/include/c++\4.8.3
INCLUDEPATH += $$(MER_SSH_SHARED_TARGET)/$$(MER_SSH_TARGET_NAME)\usr\include\libsoup-2.4

CONFIG +=  link_pkgconfig hide_symbols sailfishapp
PKGCONFIG += gobject-2.0 cairo libsoup-2.4 dconf libxml-2.0 libcurl
QT += qml quick positioning sensors dbus
LIBS += -ljpeg

QMAKE_CXXFLAGS += -std=c++0x

SOURCES += src/harbour-tripometer.cpp \
    infolistmodel.cpp \
    trackmodel.cpp \
    Utils.cpp

OTHER_FILES += \
    rpm/harbour-tripometer.spec \
    qml/harbour-tripometer.qml \
    qml/btnPlus.png \
    qml/btnMinus.png \
    qml/btnTrack.png \
    qml/btnTrackOff.png \
    qml/btnCenter.png \
    qml/btnBack.png \
    qml/btnBackDis.png \
    qml/btnClearTrack.png \
    qml/pages/FirstPage.qml \
    qml/pages/SecondPage.qml \
    qml/symFia.png \
    qml/btnTracks.png \
    harbour-tripometer.desktop

HEADERS += \
    trackmodel.h \
    infolistmodel.h \
    Utils.h


packagesExist(qdeclarative5-boostable) {
  DEFINES += HAS_BOOSTER
  PKGCONFIG += qdeclarative5-boostable
} else {
  warning("qdeclarative-boostable not available; startup times will be slower")
}

isEmpty(PREFIX)
{
  PREFIX = /usr
}


# eg /usr/share/applications/harbour-tripometer.desktop
DEPLOYMENT_PATH = $$PREFIX/share/$$TARGET
DEFINES += DEPLOYMENT_PATH=\"\\\"\"$${DEPLOYMENT_PATH}\"\\\"\"
DEFINES += APP=\"\\\"\"$${TARGET}\"\\\"\"
DEFINES += DATADIR=\"\\\"\"$${DEPLOYMENT_PATH}\"\\\"\"
DEFINES += SAILFISH
DEFINES += VERSION=\"\\\"\"2.2.0\"\\\"\"

# Input
HEADERS += src/config.h src/misc.h src/net_io.h src/geonames.h src/search.h src/track.h src/img_loader.h src/icon.h src/converter.h src/osm-gps-map/osm-gps-map.h src/osm-gps-map/osm-gps-map-layer.h src/osm-gps-map/osm-gps-map-qt.h src/osm-gps-map/osm-gps-map-osd-classic.h src/osm-gps-map/layer-wiki.h src/osm-gps-map/layer-gps.h
SOURCES += src/misc.c src/net_io.c src/geonames.c src/search.c src/track.c src/img_loader.c src/icon.c src/converter.c src/osm-gps-map/osm-gps-map.c src/osm-gps-map/osm-gps-map-layer.c src/osm-gps-map/osm-gps-map-qt.cpp src/osm-gps-map/osm-gps-map-osd-classic.c src/osm-gps-map/layer-wiki.c src/osm-gps-map/layer-gps.c

# Installation
#target.path = $$PREFIX/bin

#desktop.path = $$PREFIX/share/applications
#desktop.files = harbour-tripometer.desktop

#icon.path = $$PREFIX/share/icons/hicolor/86x86/apps
#icon.files = harbour-tripometer.png

#qml.path = $$DEPLOYMENT_PATH
#qml.files = qml/harbour-tripometer.qml qml/pages/FirstPage.qml

#src/main.qml src/Header.qml src/PlaceHeader.qml src/TrackHeader.qml src/TrackView.qml src/About.qml src/Settings.qml src/FileChooser.qml

#resources.path = $$DEPLOYMENT_PATH
#resources.files = data/wikipedia_w.48.png data/icon-camera-zoom-wide.png data/icon-camera-zoom-tele.png data/icon-cover-remove.png data/AUTHORS data/COPYING

#INSTALLS += target desktop icon resources

OTHER_FILES += rpm/harbour-tripometer.spec

# This part is to circumvent harbour limitations.
QMAKE_RPATHDIR = $$DEPLOYMENT_PATH/lib

#QT += qml-private core-private

DISTFILES += \
    qml/pages/SmallText.qml \
    rpm/harbour-tripometer.changes \
    qml/btnSat.png \
    qml/btnWorld.png \
    qml/pages/SearchPage.qml
