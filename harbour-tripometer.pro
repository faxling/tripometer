#TEMPLATE = app

TARGET = harbour-pikefight
DEPENDPATH += .
INCLUDEPATH += .
INCLUDEPATH += $$[QT_HOST_PREFIX]/include/glib-2.0/glib
INCLUDEPATH += $$[QT_HOST_PREFIX]/include/glib-2.0
INCLUDEPATH += $$[QT_HOST_PREFIX]/lib64/glib-2.0/include
INCLUDEPATH += $$[QT_HOST_PREFIX]/include/dconf
INCLUDEPATH += $$[QT_HOST_PREFIX]/include/libxml2
INCLUDEPATH += $$[QT_HOST_PREFIX]/include/cairo
INCLUDEPATH += $$[QT_HOST_PREFIX]/include/libsailfishsilica
CONFIG +=    link_pkgconfig sailfishapp
PKGCONFIG += gobject-2.0 cairo dconf libxml-2.0 libcurl sailfishsilica

QT += qml quick positioning sensors dbus svg websockets
LIBS += -ljpeg
LIBS += -lpng# LIBS += -lsailfishsilica
QMAKE_CXXFLAGS += -std=c++20
QMAKE_CXXFLAGS -= -fvisibility-inlines-hidden
SOURCES += src/harbour-tripometer.cpp \
    infolistmodel.cpp \
    src/osm-gps-map/osm-gps-map-osd-classic.c \
    trackmodel.cpp \
    Utils.cpp


message($$[QT_HOST_PREFIX])


OTHER_FILES += \
    harbour-pikefight.desktop \
    qml/harbour-pikefight.png \
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
    qml/btnSeaMap.png \
    rpm/harbour-pikefight.spec

HEADERS += \
    QExifImageHeader.h \
    src/osm-gps-map/osm-gps-map-osd-classic.h \
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


# eg /usr/share/applications/harbour-pikefight.desktop
DEPLOYMENT_PATH = $${PREFIX}/share/$${TARGET}
DEFINES += DEPLOYMENT_PATH=\"\\\"\"$${DEPLOYMENT_PATH}\"\\\"\"
DEFINES += APP=\"\\\"\"$${TARGET}\"\\\"\"
DEFINES += DATADIR=\"\\\"\"$${DEPLOYMENT_PATH}\"\\\"\"
DEFINES += SAILFISH
DEFINES += VERSION=\"\\\"\"1.0.0\"\\\"\"
DEFINES += GLIB_DISABLE_DEPRECATION_WARNINGS

# Input
HEADERS += src/config.h src/misc.h src/net_io.h src/geonames.h src/search.h src/track.h src/img_loader.h src/converter.h src/osm-gps-map/osm-gps-map.h src/osm-gps-map/osm-gps-map-layer.h src/osm-gps-map/osm-gps-map-qt.h src/osm-gps-map/layer-gps.h
SOURCES += src/misc.c src/net_io.c src/geonames.c src/search.c src/track.c src/img_loader.c src/converter.c src/osm-gps-map/osm-gps-map.c src/osm-gps-map/osm-gps-map-layer.c src/osm-gps-map/osm-gps-map-qt.cpp  src/osm-gps-map/layer-gps.c

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

OTHER_FILES += rpm/harbour-pikefight.spec

# This part is to circumvent harbour limitations.
QMAKE_RPATHDIR = $$DEPLOYMENT_PATH/lib

#QT += qml-private core-private

SAILFISHAPP_ICONS = 86x86 108x108 128x128 172x172

DISTFILES += \
    qml/LargeBtn.qml \
    qml/NameText.qml \
    qml/PikeBtn.qml \
    qml/PikeMapPage.qml \
    qml/PikePanel.qml \
    qml/StepSlider.qml \
    qml/pages/BusyIndPike.qml \
    qml/pages/CameraPage.qml \
    qml/pages/DateTimePage.qml \
    qml/pages/GalleryPage.qml \
    qml/pages/ImageList.qml \
    qml/pages/ImagePage.qml \
    qml/pages/ManPage.qml \
    qml/pages/PikePageImage.qml \
    qml/pages/SettingsPage.qml \
    qml/pages/TapArea.qml \
    qml/pages/TextList.qml \
    qml/TrippBtn.qml \
    qml/tripometer-functions.js \
    qml/pages/PikePage.qml \
    qml/pages/SmallText.qml \
    qml/btnSat.png \
    qml/btnWorld.png \
    qml/pages/SearchPage.qml \
    rpm/harbour-pikefight.changes


RESOURCES += \
    res.qrc
