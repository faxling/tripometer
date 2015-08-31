# NOTICE:
#
# Application name defined in TARGET has a corresponding QML filename.
# If name defined in TARGET is changed, the following needs to be done
# to match new name:
#   - corresponding QML filename must be changed
#   - desktop icon filename must be changed
#   - desktop filename must be changed
#   - icon definition filename in desktop file must be changed
#   - translation filenames have to be changed

# The name of your application
TARGET = harbour-tripometer

QT += quick positioning dbus

CONFIG += sailfishapp
#CONFIG += c++11
QMAKE_CXXFLAGS += -std=c++0x

SOURCES += src/harbour-tripometer.cpp \
    infolistmodel.cpp \
    Utils.cpp

OTHER_FILES += qml/harbour-tripometer.qml \
    qml/pages/FirstPage.qml \
    qml/pages/SecondPage.qml \
    rpm/harbour-tripometer.changes.in \
    rpm/harbour-tripometer.spec \
    rpm/harbour-tripometer.yaml \
    harbour-tripometer.desktop

# to disable building translations every time, comment out the
# following CONFIG line
# CONFIG += sailfishapp_i18n

# German translation is enabled as an example. If you aren't
# planning to localize your app, remember to comment out the
# following TRANSLATIONS line. And also do not forget to
# modify the localized app name in the the .desktop file.
#TRANSLATIONS += translations/harbour-tripometer-de.ts

HEADERS += \
    infolistmodel.h \
    Utils.h

