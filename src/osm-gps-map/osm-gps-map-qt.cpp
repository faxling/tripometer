/*
 * osm-gps-map-qt.cpp
 * Copyright (C) Damien Caliste 2013-2014 <dcaliste@free.fr>
 *
 * osm-gps-map-qt.cpp is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include "osm-gps-map-qt.h"
#include "../net_io.h"
#include "infolistmodel.h"
#include "osm-gps-map-layer.h"
#include "osm-gps-map.h"
#include "src/misc.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPainterPath>
#include <QStandardPaths>
#include <QtSvg/QSvgRenderer>
#include <Utils.h>
#include <algorithm>
#include <float.h>
#include <glib/gstdio.h>
#include <limits>
#include <math.h>
#include <ranges>
#include <sstream>

#define GCONF_KEY_ZOOM "zoom"
#define GCONF_KEY_SOURCE "source"
#define GCONF_KEY_LATITUDE "latitude"
#define GCONF_KEY_LONGITUDE "longitude"
#define GCONF_KEY_TRACK_PATH "track_path"
#define GCONF_KEY_SCREEN_ROTATE "screen-rotate"
#define GCONF_KEY_GPS_REFRESH_RATE "gps-refresh-rate"

extern QObject* g_pRootObject;
extern QObject* g_pTheTrackModel;
extern int g_nOutstaningCurls;
extern int g_nSkipDraw;

static void osm_gps_map_qt_repaint(Maep::GpsMap* widget, OsmGpsMap* map);
static void osm_gps_map_qt_coordinate(Maep::GpsMap* widget, GParamSpec* pspec, OsmGpsMap* map);
static void osm_gps_map_qt_auto_center(Maep::GpsMap* widget, GParamSpec* pspec, OsmGpsMap* map);
static void osm_gps_map_qt_source(Maep::GpsMap* widget, GParamSpec* pspec, OsmGpsMap* map);
static void osm_gps_map_qt_places(Maep::GpsMap* widget, GSList* places);

static void osm_gps_map_qt_places_failure(Maep::GpsMap* widget, GError* error);

extern "C" void parse_navionics_key(const char* pResponce, int nLen, char* a_pToken, char* c_pToken)
{

  auto oJson = QJsonDocument::fromJson(QByteArray(pResponce, nLen));
  auto oObj = oJson.object();
  // qDebug() << "json" << oJson.toJson();
  QByteArray ocAT = oObj["access_token"].toString().toLatin1();
  QByteArray ocCT = oObj["configuration_token"].toString().toLatin1();

  strncpy(a_pToken, ocAT.constData(), ocAT.length());
  strncpy(c_pToken, ocCT.constData(), ocAT.length());

  // qDebug() << ocAT;
}

IdlePainter::IdlePainter(OsmGpsMap* _map)
{
  m_map = _map;
  m_pIdlePaintTimer = new MssTimer([this]() {
    if (g_nSkipDraw != 0)
      return;

    if (m_bPaused == false && m_bPaint == true)
      osm_gps_map_idle_redraw(m_map);

    m_bPaint = false;
  });
  m_pIdlePaintTimer->Start(100);
};

void IdlePainter::Pause(bool b)
{
  m_bPaused = b;
};

void IdlePainter::RequestPaint()
{
  m_bPaint = true;
};

void Maep::Track::addPoint(QGeoPositionInfo& info)
{
  QGeoCoordinate coord = info.coordinate();
  qreal speed, h_acc;
  speed = NAN;

  if (info.hasAttribute(QGeoPositionInfo::GroundSpeed))
    speed = info.attribute(QGeoPositionInfo::GroundSpeed);
  h_acc = std::numeric_limits<gfloat>::max();
  if (info.hasAttribute(QGeoPositionInfo::HorizontalAccuracy))
    h_acc = info.attribute(QGeoPositionInfo::HorizontalAccuracy);

  maep_geodata_add_trackpoint(track, coord.latitude(), coord.longitude(), h_acc, coord.altitude(),
                              speed, NAN, NAN);

  emit characteristicsChanged((qreal)maep_geodata_track_get_metric_length(track),
                              (unsigned int)maep_geodata_track_get_duration(track));
}
void Maep::Track::finalizeSegment()
{
  if (track)
    maep_geodata_track_finalize_segment(track);
}

void Maep::Track::addWayPoint(const QGeoCoordinate& coord, const QString& name,
                              const QString& comment, const QString& description)
{
  maep_geodata_add_waypoint(track, coord.latitude(), coord.longitude(), name.toLocal8Bit().data(),
                            comment.toLocal8Bit().data(), description.toLocal8Bit().data());
}

void Maep::Track::highlightWayPoint(int iwpt)
{
  maep_geodata_waypoint_set_highlight(track, iwpt);
}

Maep::GpsMap::GpsMap(QQuickItem* parent) : QQuickPaintedItem(parent), compass(parent)
{

  char* path = g_build_filename(g_get_user_cache_dir(), APP, NULL);
  gint source = gconf_get_int(GCONF_KEY_SOURCE, OSM_GPS_MAP_SOURCE_OPENSTREETMAP);
  map = OSM_GPS_MAP(g_object_new(OSM_TYPE_GPS_MAP, "map-source", source, "tile-cache",
                                 OSM_GPS_MAP_CACHE_FRIENDLY, "tile-cache-base", path, "auto-center",
                                 FALSE, "gps-track-point-radius", 10, NULL));

  g_free(path);

  Init();
}

void Maep::GpsMap::Init()
{
  m_pReqCountTimer = new MssTimer([this] {
    if (g_nOutstaningCurls != numberPendingReq_)
    {
      numberPendingReq_ = g_nOutstaningCurls;
      emit numberPendingReqChanged();
      if (g_nOutstaningCurls == 0)
      {
        osm_gps_map_idle_redraw(map);
      }
    }

    if (crossHairEnabled() && weatherEnabled())
      getWeatherCurrentPos();
  });

  m_pIdlePainter = new IdlePainter(map);

  m_pReqCountTimer->Start(200);

  gint zoom = gconf_get_int(GCONF_KEY_ZOOM, 3);

  gfloat lat = gconf_get_float(GCONF_KEY_LATITUDE, 50.0);
  gfloat lon = gconf_get_float(GCONF_KEY_LONGITUDE, 21.0);

  bool orientation = gconf_get_bool(GCONF_KEY_SCREEN_ROTATE, TRUE);

  screenRotation = orientation;

  g_object_set_data(G_OBJECT(map), GCONF_KEY_WEATHER,
                    new int(gconf_get_bool(GCONF_KEY_WEATHER, TRUE)));
  g_object_set_data(G_OBJECT(map), GCONF_KEY_CROSSHAIR,
                    new int(gconf_get_bool(GCONF_KEY_CROSSHAIR, TRUE)));
  g_object_set_data(G_OBJECT(map), GCONF_KEY_COMPASS_ENABLED,
                    new int(gconf_get_bool(GCONF_KEY_COMPASS_ENABLED, FALSE)));

  osm_gps_map_set_mapcenter(map, lat, lon, zoom);
  coordinate = QGeoCoordinate(lat, lon);

  g_signal_connect_swapped(G_OBJECT(map), "dirty", G_CALLBACK(osm_gps_map_qt_repaint), this);
  g_signal_connect_swapped(G_OBJECT(map), "notify::latitude", G_CALLBACK(osm_gps_map_qt_coordinate),
                           this);
  g_signal_connect_swapped(G_OBJECT(map), "notify::auto-center",
                           G_CALLBACK(osm_gps_map_qt_auto_center), this);

  g_signal_connect_swapped(G_OBJECT(map), "notify::map-source", G_CALLBACK(osm_gps_map_qt_source),
                           this);
  net_io_init();

  osd = osm_gps_map_osd_classic_init(map);

  search = maep_search_context_new();
  g_signal_connect_swapped(G_OBJECT(search), "places-available", G_CALLBACK(osm_gps_map_qt_places),
                           this);
  g_signal_connect_swapped(G_OBJECT(search), "download-error",
                           G_CALLBACK(osm_gps_map_qt_places_failure), this);

  screensurf = NULL;
  cr = NULL;

  // pat = NULL;
  white = new QColor(255, 255, 255);
  img = NULL;

  forceActiveFocus();
  setAcceptedMouseButtons(Qt::LeftButton);

  gpsRefreshRate_ = 2000;

  gps = QGeoPositionInfoSource::createDefaultSource(this);

  if (gps)
  {
    connect(gps, SIGNAL(positionUpdated(QGeoPositionInfo)), this,
            SLOT(positionUpdate(QGeoPositionInfo)));
    connect(gps, SIGNAL(updateTimeout()), this, SLOT(positionLost()));

    if (gpsRefreshRate_ > 0)
    {
      gps->setUpdateInterval(gpsRefreshRate_);
      gps->startUpdates();
    }
  }
  else
    g_message("no gps source...");

  /* Timer for dev
    MssTimer* p  = new MssTimer();

    static int nCount = 1;
    p->SetTimeOut([&] {
      QGeoPositionInfo o(QGeoCoordinate(60,16.025 + nCount * 0.0005),QDateTime());

      o.setAttribute(QGeoPositionInfo::HorizontalAccuracy,1000);
      o.setAttribute(QGeoPositionInfo::Direction, nCount % 360);

      positionUpdate(o);
      ++nCount;
    });

    p->Start(2000);

  */

  lastGps = QGeoPositionInfo();
  lgps = maep_layer_gps_new();
  g_signal_connect_swapped(G_OBJECT(lgps), "dirty", G_CALLBACK(osm_gps_map_qt_repaint), this);

  // maep_layer_gps_set_azimuth(lgps, NAN);
  connect(&compass, SIGNAL(readingChanged()), this, SLOT(compassReadingChanged()));
  compass.setDataRate(2);
  initBoatMarkers();
  track_capture = false;
  track_current = NULL;
  enableCompass(compassEnabled());
}

bool Maep::GpsMap::aisEnabled()
{
  return m_bAisEnabled;
}

void Maep::GpsMap::enableAis(bool b)
{
  //  g_signal_emit_by_name(G_OBJECT(map), "dirty");
  m_bAisEnabled = b;

  if (b == true)
  {
    // p->SetAisStyle(0x136fd5, 2);
    m_AisStreamClient.reset(new AisStreamClient(map, m_pIdlePainter));
  }
  else
    m_AisStreamClient.reset(nullptr);


  emit enableAisChanged(b);

  // osm_gps_map_idle_redraw(map);
  // emit enableAisChanged();
}

bool Maep::GpsMap::crossHairEnabled()
{
  bool* pb = (bool*)g_object_get_data(G_OBJECT(map), GCONF_KEY_CROSSHAIR);
  return *pb;
}

void Maep::GpsMap::enableCrossHair(bool b)
{
  bool* pb = (bool*)g_object_get_data(G_OBJECT(map), GCONF_KEY_CROSSHAIR);
  *pb = b;
  m_pIdlePainter->RequestPaint();
  emit enableCrossHairChanged(b);
  // g_signal_emit_by_name(G_OBJECT(map), "dirty");
}

bool Maep::GpsMap::compassEnabled()
{
  bool* pb = (bool*)g_object_get_data(G_OBJECT(map), GCONF_KEY_COMPASS_ENABLED);
  return *pb;
}

void Maep::GpsMap::enableCompass(bool enable)
{
  bool* pb = (bool*)g_object_get_data(G_OBJECT(map), GCONF_KEY_COMPASS_ENABLED);
  *pb = enable;

  if (!enable)
  {
    compass.stop();
  }
  else
  {

    if (compass.isFeatureSupported(QCompass::SkipDuplicates))
    {
      compass.setSkipDuplicates(true);
    }
    compass.start();
  }
  m_pIdlePainter->RequestPaint();

  emit enableCompassChanged(enable);
  // g_signal_emit_by_name(G_OBJECT(map), "dirty");
}

void Maep::GpsMap::enableWeather(bool b)
{
  bool* pb = (bool*)g_object_get_data(G_OBJECT(map), GCONF_KEY_WEATHER);
  *pb = b;
  m_pIdlePainter->RequestPaint();
  emit enableWeatherChanged(b);

  //  g_signal_emit_by_name(G_OBJECT(map), "dirty");
}

bool Maep::GpsMap::weatherEnabled()
{
  bool* pb = (bool*)g_object_get_data(G_OBJECT(map), GCONF_KEY_WEATHER);
  m_pIdlePainter->RequestPaint();
  return *pb;
}

Maep::GpsMap::~GpsMap()
{
  qDebug() << "Destruct Qmap";
  m_pReqCountTimer->Stop();
  delete m_pReqCountTimer;
  gint zoom, source;
  gfloat lat, lon;

  compass.stop();

  g_object_get(map, "zoom", &zoom, "map-source", &source, "latitude", &lat, "longitude", &lon,
               NULL);
  osm_gps_map_osd_classic_free(osd);
  osd = NULL;

  g_object_unref(search);
  net_io_finalize();

  if (screensurf)
    cairo_surface_destroy(screensurf);
  if (cr)
    cairo_destroy(cr);

  delete (white);
  if (gps)
    delete (gps);
  if (track_current && track_current->parent() == this)
    delete (track_current);

  /* ... and store it in gconf */
  g_message("Storing configuration.");
  gconf_set_int(GCONF_KEY_ZOOM, zoom);
  gconf_set_int(GCONF_KEY_SOURCE, source);
  //  gconf_set_int(GCONF_KEY_OVERLAY_SOURCE, overlaySource);
  gconf_set_float(GCONF_KEY_LATITUDE, lat);
  gconf_set_float(GCONF_KEY_LONGITUDE, lon);
  //  gconf_set_bool(GCONF_KEY_DOUBLEPIX, dpix);
  gconf_set_int(GCONF_KEY_GPS_REFRESH_RATE, gpsRefreshRate_);
  gconf_set_bool(GCONF_KEY_COMPASS_ENABLED, compassEnabled());
  gconf_set_bool(GCONF_KEY_WEATHER, weatherEnabled());
  gconf_set_bool(GCONF_KEY_CROSSHAIR, crossHairEnabled());

  g_message("Storing configuration done.");
  g_object_unref(lgps);

  g_object_unref(map);
}

// Called from signal "dirty"
static void osm_gps_map_qt_repaint(Maep::GpsMap* widget, OsmGpsMap* map)
{
  Q_UNUSED(map);
  widget->mapUpdate();
  widget->update();
}

bool Maep::GpsMap::mapSized()
{
  if (width() < 1 || height() < 1)
    return false;

  if (screensurf && (cairo_image_surface_get_width(screensurf) != width() ||
                     cairo_image_surface_get_height(screensurf) < height()))
  {
    cairo_surface_destroy(screensurf);
    cairo_destroy(cr);
    screensurf = NULL;
  }

  if (!screensurf)
  {
    screensurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width(), height());
    cr = cairo_create(screensurf);
    img.reset(new QImage(cairo_image_surface_get_data(screensurf),
                         cairo_image_surface_get_width(screensurf),
                         cairo_image_surface_get_height(screensurf), QImage::Format_ARGB32));

    osm_gps_map_set_viewport(map, width(), height());
    return true;
  }
  return false;
}

void Maep::GpsMap::mapUpdate()
{
  if (g_nSkipDraw != 0)
    return;

  if (!cr)
    return;

  cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(cr);
  cairo_save(cr);
  if (m_bAisEnabled == true || m_AisStreamClient)
  {
    m_AisStreamClient->DrawAllAis();
  }

  int drag_mouse_dx, drag_mouse_dy;
  osm_gps_map_get_offset(map, &drag_mouse_dx, &drag_mouse_dy);
  cairo_translate(cr, drag_mouse_dx, drag_mouse_dy);

  osm_gps_map_blit(map, cr, CAIRO_OPERATOR_SOURCE);

  osm_gps_map_layer_draw(OSM_GPS_MAP_LAYER(lgps), cr, map);

  cairo_restore(cr);

  if (osd)
    osd->draw(osd, cr);

  emit mapChanged();
}

void Maep::GpsMap::paintTo(QPainter* painter, int width, int height)
{
  int w, h;

  //  static QSet<int> ocColors;
  //  static QFile oColorFile;
  static QMap<int, int> ocColorDeepMap{{0x20b0, 5},   {0x38b8, 10},  {0x48c0, 15},  {0x50c0, 20},
                                       {0x58c8, 25},  {0x60c8, 30},  {0x68c8, 35},  {0x70c8, 40},
                                       {0x78d0, 45},  {0x80d0, 50},  {0x88d0, 60},  {0x88d8, 70},
                                       {0x90d8, 80},  {0x98d8, 90},  {0xa0d8, 100}, {0xa8e0, 120},
                                       {0xb0e0, 140}, {0xb8e0, 160}, {0xc0e8, 180}, {0xf8f8, 200}};

  if (!img || !screensurf)
    return;

  w = cairo_image_surface_get_width(screensurf);
  h = cairo_image_surface_get_height(screensurf);

  QRectF target(0, 0, width, height);
  QRectF source((w - width) * 0.5, (h - height) * 0.5, width, height);

  img->pixel(width / 2, height / 2);

  painter->drawImage(target, *img, source);
  QRgb o = img->pixel(img->width() / 2, img->height() / 2);

  if ((0xff & o) == 0xf8)
  {
    for (;;)
    {
      int nCurrentDeep = osm_gps_map_depth(map);
      if (nCurrentDeep < 0)
        break;

      o = o << 8;
      o = o >> 16;

      auto iDeep = ocColorDeepMap.find(o);
      if (iDeep != ocColorDeepMap.end())
      {
        osm_gps_map_set_depth(map, iDeep.value());
      }
      break;
    }

    /* For figuring out the color codes
    if (ocColors.contains(o) == false)
    {
      if (oColorFile.isOpen() == false)
      {
        oColorFile.setFileName(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation)
    +
                               "/pikeFight/colors.txt");
        oColorFile.open(QIODevice::WriteOnly);
      }
      ocColors.insert(o);
      // sColors.append(QString::number(o, 16) + ", ");
      oColorFile.write("{0x");
      oColorFile.write(QString::number(o, 16).toLatin1());
      oColorFile.write(", ");
      oColorFile.write(QString::number(ocColors.size(), 10).toLatin1());
      oColorFile.write("} ,");

    }
    */

    // painter->drawText(100, 100, sColors);
  }
}

void Maep::GpsMap::paint(QPainter* painter)
{

  if (mapSized())
    mapUpdate();

  paintTo(painter, width(), height());
}

void Maep::GpsMap::zoomIn()
{
  osm_gps_map_magnifye(map, 1);
}

void Maep::GpsMap::zoomOut()
{
  osm_gps_map_magnifye(map, -1);
}

#define OSM_GPS_MAP_SCROLL_STEP (10)

void Maep::GpsMap::keyPressEvent(QKeyEvent* event)
{
  int step;

  step = width() / OSM_GPS_MAP_SCROLL_STEP;

  int nKey = event->key();

  switch (nKey)
  {
  case Qt::Key_Up:
    osm_gps_map_uppdate_offset(map, 0, -step);
    osm_gps_map_scroll(map);
    break;
  case Qt::Key_Down:
    osm_gps_map_uppdate_offset(map, 0, -step);
    osm_gps_map_scroll(map);
    break;
  case Qt::Key_Right:
    osm_gps_map_uppdate_offset(map, step, 0);
    osm_gps_map_scroll(map);
    break;
  case Qt::Key_Left:
    osm_gps_map_uppdate_offset(map, -step, 0);
    osm_gps_map_scroll(map);
    break;
  case Qt::Key_ZoomIn:
  case Qt::Key_Plus:
    osm_gps_map_zoom_in(map);
    break;

  case Qt::Key_ZoomOut:
  case Qt::Key_Minus:
    osm_gps_map_zoom_out(map);
    break;
  case Qt::Key_S:
    emit searchRequest();
    break;
  }
}

void Maep::GpsMap::touchEvent(QTouchEvent* touchEvent)
{
  int nTDist = 0;
  static int nTDistLast = 0;
  static int nLastDeltaX = 0;
  static int nLastDeltaY = 0;
  static QTouchEvent::TouchPoint tBeginPoint;
  static QTouchEvent::TouchPoint tEndPoint;

  if (g_nSkipDraw != 0)
    return;

  switch (touchEvent->type())
  {
  case QEvent::TouchBegin:
  {
    QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();

    dragging = touchPoints.count() == 1;

    zooming = touchPoints.count() == 2;
    nLastDeltaX = 0;
    nLastDeltaY = 0;
    if (dragging)
    {
      m_oElapsed.start();
      tBeginPoint = touchPoints.first();
    }
    if (zooming)
    {
      const QTouchEvent::TouchPoint& touchPoint0 = touchPoints.first();
      const QTouchEvent::TouchPoint& touchPoint1 = touchPoints.last();
      nTDistLast = QLineF(touchPoint0.pos(), touchPoint1.pos()).length();
    }
    return;
  }
  case QEvent::TouchUpdate:
  {
    // g_message("touch update %d", haveMouseEvent);
    QList<QTouchEvent::TouchPoint> touchPoints = touchEvent->touchPoints();

    tEndPoint = touchPoints.first();
    if (touchPoints.count() == 2 && dragging)
    {
      dragging = false;
      zooming = true;
      ;
      const QTouchEvent::TouchPoint& touchPoint0 = touchPoints.first();
      const QTouchEvent::TouchPoint& touchPoint1 = touchPoints.last();
      nTDistLast = QLineF(touchPoint0.pos(), touchPoint1.pos()).length();
    }
    if (dragging)
    {
      // Drag case only
      const QTouchEvent::TouchPoint& touchPoint0 = touchPoints.first();
      QPointF delta = touchPoint0.pos() - touchPoint0.startPos();
      osm_gps_map_uppdate_offset(map, delta.x() - nLastDeltaX, delta.y() - nLastDeltaY);
      nLastDeltaX = delta.x();
      nLastDeltaY = delta.y();
      osm_gps_map_scroll(map);
      osm_gps_map_uppdate_offset(map, 0, 0);
      // osm_gps_map_idle_redraw(map);
    }
    else if (zooming)
    {
      // Zoom and drag case
      if (touchPoints.count() < 2)
        return;

      const QTouchEvent::TouchPoint& touchPoint0 = touchPoints.first();
      const QTouchEvent::TouchPoint& touchPoint1 = touchPoints.last();
      int nTDistNew = QLineF(touchPoint0.pos(), touchPoint1.pos()).length();
      if (abs(nTDistNew - nTDistLast) > 50)
      {
        if (nTDistNew >= nTDistLast)
          osm_gps_map_zoom_in(map);
        else
          osm_gps_map_zoom_out(map);
        nTDistLast = nTDistNew;
      }
    }
    return;
  }
  case QEvent::TouchEnd:
    tEndPoint = touchEvent->touchPoints().first();

    nTDist = QLineF(tBeginPoint.pos(), tEndPoint.pos()).length();
    if ((m_oElapsed.elapsed() < 200) && (nTDist < 50))
      emit trippleDrag();
    break;
  default:
    QQuickItem::touchEvent(touchEvent);
    break;
  }
}

static void osm_gps_map_qt_source(Maep::GpsMap* widget, GParamSpec* pspec, OsmGpsMap* map)
{
  Q_UNUSED(pspec);
  Q_UNUSED(map);

  widget->sourceChanged(widget->source());
}

void Maep::GpsMap::setSource(Maep::GpsMap::Source value)
{
  Source orig;
  orig = source();
  if (orig == value)
    return;

  g_object_set(map, "map-source", (OsmGpsMapSource_t)value, NULL);
}
/*
void Maep::GpsMap::clearTrack()
{
  osm_gps_map_clear_tracks(map);
}

*/
void Maep::GpsMap::centerCurrentGps()
{

  if (lastGps.isValid() == true)
  {
    osm_gps_map_set_center(map, lastGps.coordinate().latitude(), lastGps.coordinate().longitude());
  }
}

void curl_wind_cb(net_result_t* result, gpointer data)
{
  if (result->code == 0)
  {
    OsmGpsMap* map = static_cast<OsmGpsMap*>(data);
    QJsonDocument oJD = QJsonDocument::fromJson(QByteArray(result->data.ptr, result->data.len));
    auto oJ = oJD.object()["current"].toObject();
    osm_gps_map_set_windSpeed(map, oJ["wind_speed_10m"].toDouble() / 3.6,
                              oJ["wind_direction_10m"].toDouble(), oJ["temperature_2m"].toDouble());
  }
}

// using namespace std;
class comma_numpunct : public std::numpunct<char>
{
  char do_decimal_point() const override { return '.'; }
};

std::locale comma_locale(std::locale(), new comma_numpunct());

void Maep::GpsMap::getWeatherCurrentPos()
{
  // constexpr char constString[] = "constString";
  constexpr char WAPI[] = "https://api.open-meteo.com/v1/"
                          "forecast?current=temperature_2m,wind_speed_10m,wind_direction_10m";
  coord_t tPos = osm_gps_map_get_center_ordinates(map);
  tPos.rlat = rad2deg(tPos.rlat);
  tPos.rlon = rad2deg(tPos.rlon);
  if (lastLaDeg != tPos.rlat)
  {

    lastLaDeg = tPos.rlat;
    std::stringstream os;
    os.imbue(comma_locale);

    os << WAPI << "&latitude=" << tPos.rlat << "&longitude=" << tPos.rlon << std::ends;
    net_io_download_async(os.str().c_str(), curl_wind_cb, map, 0);
  }
}

void Maep::GpsMap::centerTrack(float lo, float la)
{
  //   MarkData t = GetMarkData(sTrackName);
  osm_gps_map_set_center(map, la, lo);
}

void Maep::GpsMap::renameTrack(const QString& sTrackName, int nId)
{
  auto oI = m_ocMarkers.find(nId);
  if (oI != m_ocMarkers.end())
  {
    osm_gps_map_rename_image(map, *oI, sTrackName.toUtf8().data());
  }
}

void Maep::GpsMap::loadTrack(const QString& sTrackName, int nId)
{
  MarkData t = GetMarkData(sTrackName);

  if (t.nType == 0 || t.nType == 2)
  {
    QString sGpxFileName = GpxFullName(sTrackName);

    GError* error = 0;
    MaepGeodata* track = maep_geodata_new_from_file(sGpxFileName.toUtf8().data(), &error);

    if (track != 0)
      osm_gps_map_add_track(map, track, nId, 2);
  }
  else
  {
    if (m_ocMarkers.contains(nId) == false)
    {
      char* szSymName = find_file("qml/symFia.png");
      cairo_surface_t* pSurface = cairo_image_surface_create_from_png(szSymName);
      m_ocMarkers[nId] = pSurface;
      g_free(szSymName);
      osm_gps_map_add_image_with_alignment(map, t.la, t.lo, pSurface, 0.5, 1.0,
                                           sTrackName.toUtf8().data());
    }
  }
}

void Maep::GpsMap::unloadTrack(int nId)
{
  auto oI = m_ocMarkers.find(nId);
  if (oI != m_ocMarkers.end())
  {
    osm_gps_map_remove_image(map, *oI);

    m_ocMarkers.erase(oI);
    return;
  }

  osm_gps_map_clear_track(map, nId);
}

void Maep::GpsMap::noDbPoint()
{
  osm_gps_map_clear_track(map, -1);
}

void Maep::GpsMap::addDbPoint()
{
  MaepGeodata* pDBTrack = osm_get_track(map, -1);
  if (pDBTrack == 0)
  {
    pDBTrack = maep_geodata_new();
    osm_gps_map_add_track(map, pDBTrack, -1, -1); // -1 The Distance tool
  }
  coord_t tPos = osm_gps_map_get_center_ordinates(map);
  maep_geodata_add_trackpoint(pDBTrack, rad2deg(tPos.rlat), rad2deg(tPos.rlon),
                              std::numeric_limits<gfloat>::max(), 0, 0, NAN, NAN);
}

void Maep::GpsMap::markPikeInMap(int nId)
{
  if (nId < 0)
    osm_gps_map_mark_image(map, 0);
  else
    osm_gps_map_mark_image(map, m_ocPikeMarkers[nId]);
}

void Maep::GpsMap::removePikeInMap(int nId)
{
  osm_gps_map_remove_image(map, m_ocPikeMarkers[nId]);
  m_ocPikeMarkers.remove(nId);
}

void Maep::GpsMap::removePikesInMap()
{
  for (auto oI : m_ocPikeMarkers)
    osm_gps_map_remove_image(map, oI);

  m_ocPikeMarkers.clear();
}

void Maep::GpsMap::loadPikeInMap(int nId, int nType, float fLo, float fLa)
{
  // full_path /home/nemo/.harbour-pikefight/qml/symPike2.png
  // full_path /usr/share/harbour-pikefight/qml/symPike2.png

  if (nId < 0)
    return;
  char* szSymName;
  if (nType == 1)
    szSymName = find_file("qml/symPike.png");
  else if (nType == 2)
    szSymName = find_file("qml/symPike2.png");
  else if (nType == 3)
    szSymName = find_file("qml/symPike3.png");
  else
    return;

  if (szSymName == nullptr)
    return;

  cairo_surface_t* pSurface = cairo_image_surface_create_from_png(szSymName);
  g_free(szSymName);

  osm_gps_map_add_image_with_alignment(map, fLa, fLo, pSurface, 0.5, 1.0, 0);

  m_ocPikeMarkers[nId] = pSurface;
}

QGeoCoordinate Maep::GpsMap::currentPos()
{
  coord_t tPos;

  if (lastGps.isValid() == true)
  {
    tPos.rlat = lastGps.coordinate().latitude();
    tPos.rlon = lastGps.coordinate().longitude();
  }
  else
  {
    tPos = osm_gps_map_get_center_ordinates(map);
    tPos.rlat = rad2deg(tPos.rlat);
    tPos.rlon = rad2deg(tPos.rlon);
  }

  QGeoCoordinate tRet(tPos.rlat, tPos.rlon);

  return tRet;
}

/*
 *
 *  Sorted
oPModel.append({
                 "nId": Number(nId),
                 "sDate": sDate,
                 "sImage": sImage,
                 "sImageThumb": String(oCaptureThumbMaker.name(sImage)),
                 "sLength": sLenText,
                 "nLen": nLen,
                 "fLo": fLo,
                 "fLa": fLa
               })

                 "fLa": 0
                 "fLo": 1
                 "nId":2
                 "nLen": 4
                 "sDate": 5
                 "sImage": 6
                 "sImageThumb": 7,
                 "sLength": 8



*/
struct MssFont : public QFont
{
  MssFont(int nPtSize, QFont::Weight eW) : QFont("Forte")
  {
    setPointSize(nPtSize);
    setWeight(eW);
  }
};

static MssFont NormalFont(22, QFont::Normal);
const static MssFont BigFont1(72, QFont::Normal);
const static MssFont BigFont(42, QFont::Bold);

void Maep::GpsMap::DrawResultForTeam(QVariant pListTeam, QString sTeamNameAndSum, int nMinSize,
                                     QImage& oImg, QPainter* pPainter, double fQuote)
{
  QAbstractListModel* pp = qvariant_cast<QAbstractListModel*>(pListTeam);

  auto oc = pp->roleNames();

  int nLen = std::find(oc.begin(), oc.end(), "sLength").key();
  int nThumb = std::find(oc.begin(), oc.end(), "sImageThumb").key();
  int nDate = std::find(oc.begin(), oc.end(), "sDate").key();

  int nC = pp->rowCount();

  // osm_gps_map_from_co_ordinates(map,

  static const int LINE_SPACING = 30;
  static const int INDENT = 25;
  static const int INDENT1 = 70;
  static const int INDENT2 = 400;
  static const int MAX_THUMBS_COUNT = 32;
  pPainter->setFont(BigFont);
  pPainter->drawText(INDENT1, START_LINE - LINE_SPACING * 2, sTeamNameAndSum);
  pPainter->drawImage(0, START_LINE - LINE_SPACING * 2 - (40), oImg);

  const static int IMG_COLUMNS = 4;
  const static int IMG_COLUMN_WIDTH = 150;
  int nStart = 0;
  if (nC > MAX_THUMBS_COUNT)
  {
    nStart = nC - MAX_THUMBS_COUNT;
  }

  int nCount = 0;
  for (int i = nStart; i < nC; i++)
  {
    QString sDate = pp->data(pp->index(i), nDate).toString();
    QString sLen = pp->data(pp->index(i), nLen).toString();
    QString sNum = QString::number(i + 1);
    QString sThumb = pp->data(pp->index(i), nThumb).toString();

    if (nMinSize > 0)
      NormalFont.setStrikeOut(pp->data(pp->index(i), 3).toInt() < nMinSize);

    pPainter->setFont(NormalFont);
    int nY = START_LINE + nCount * LINE_SPACING;
    int nYImg = START_LINE + (nCount / IMG_COLUMNS) * (LINE_SPACING * IMG_COLUMNS) - LINE_SPACING;
    pPainter->drawText(INDENT1, nY, sDate);
    if (nMinSize > 0)
      pPainter->drawText(INDENT2, nY, sLen);
    pPainter->drawText(INDENT, nY, sNum);
    if (QFile::exists(sThumb))
    {
      QImage oImg = QImage(sThumb).scaledToHeight(LINE_SPACING * IMG_COLUMNS - 2);
      if (oImg.isNull() == false)
        pPainter->drawImage(INDENT2 + IMG_COLUMN_WIDTH * (nCount % IMG_COLUMNS) + 100, nYImg, oImg);
      else
        qDebug() << "null image " << sThumb;
    }
    double fLa = pp->data(pp->index(i), 0).toDouble();
    double fLo = pp->data(pp->index(i), 1).toDouble();
    int x, y;
    osm_gps_map_from_deg(map, fLo, fLa, &x, &y);
    NormalFont.setStrikeOut(false);
    pPainter->setFont(NormalFont);
    pPainter->drawText(x * fQuote - 8, y * fQuote + 23, sNum);
    ++nCount;
  }

  START_LINE += (LINE_SPACING * (nC + 4));
}

static void RenderSvg(QString sImgName, QImage* pImage)
{
  *pImage = QImage(60, 60, QImage::Format_ARGB32);
  pImage->fill(Qt::transparent);
  QPainter painter(pImage);
  QSvgRenderer renderer(sImgName);
  renderer.render(&painter);
}

static void BoatSvg(QSvgRenderer& renderer, QImage* pImage, int nRotation)
{
  *pImage = QImage(BOAT_IMG_SIZE, BOAT_IMG_SIZE, QImage::Format_ARGB32);
  pImage->fill(Qt::transparent);
  QPainter painter(pImage);
  painter.translate(BOAT_IMG_SIZE / 2, BOAT_IMG_SIZE / 2);
  painter.rotate(nRotation);
  painter.translate(-BOAT_IMG_SIZE / 2, -BOAT_IMG_SIZE / 2);
  renderer.render(&painter);
}

QString Maep::GpsMap::saveMap(int w, int h)
{
  QDateTime oNow(QDateTime::currentDateTime());
  QString sTrackName = oNow.toString("yyyy-MM-dd-hh-mm-ss");
  QString sPath = StorageDir() ^ ("map" + sTrackName + ".jpg");

  cairo_surface_t* mapSurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
  cairo_t* handle = cairo_create(mapSurf);

  osm_map_fill_tiles_surface(map, mapSurf, handle);

  std::unique_ptr<QImage> pImg(
      new QImage(cairo_image_surface_get_data(mapSurf), cairo_image_surface_get_width(mapSurf),
                 cairo_image_surface_get_height(mapSurf), QImage::Format_ARGB32));

  pImg->save(sPath);
  cairo_surface_destroy(mapSurf);
  cairo_destroy(handle);
  qDebug() << "save " << sPath;
  return "map" + sTrackName;
}

void Maep::GpsMap::putSkipDraw(bool b)
{
  g_nSkipDraw = b ? 1 : 0;
}

bool Maep::GpsMap::skipDraw()
{
  return g_nSkipDraw != 0;
}

void Maep::GpsMap::initBoatMarkers()
{
  int DEG = 360;
  cairo_surface_t** ocBoatMarker = new cairo_surface_t*[DEG];
  QSvgRenderer renderer(QString(":/boat.svg"));

  for (int nRotDeg = 0; nRotDeg < DEG; ++nRotDeg)
  {
    QImage* oImg = new QImage;
    BoatSvg(renderer, oImg, nRotDeg);
    //    int nStride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, oImg.width());
    //    qDebug() << oImg.format() << " " << nStride;

    ocBoatMarker[nRotDeg] = cairo_image_surface_create_for_data(
        oImg->bits(), CAIRO_FORMAT_ARGB32, oImg->width(), oImg->height(), oImg->width() * 4);
  }

  lgps_init_boat_images(lgps, ocBoatMarker);
}

QString Maep::GpsMap::savePikeReport(QVariant pListTeam1, QString sTeamNameAndSum1,
                                     QVariant pListTeam2, QString sTeamNameAndSum2,
                                     QVariant pListTeam3, QString sTeamNameAndSum3, int nMinSize,
                                     QString sName, int nTeamCount)
{
  QDateTime oNow(QDateTime::currentDateTime());
  QString sTrackName = oNow.toString("yyyy-MM-dd-hh-mm-ss");
  QString sPath = StorageDir() ^ ("report" + sTrackName + ".png");
  cairo_surface_write_to_png(screensurf, sPath.toLatin1().data());
  int nHeight = cairo_image_surface_get_height(screensurf);
  QImage oPaintImage(sPath);
  static const double REPORT_WIDTH = 1200;
  double fQuote = REPORT_WIDTH / oPaintImage.width();
  oPaintImage = oPaintImage.scaledToWidth(REPORT_WIDTH);
  QPainter oImagePainter(&oPaintImage);
  oImagePainter.setRenderHints(QPainter::Antialiasing);
  oImagePainter.setFont(BigFont1);
  oImagePainter.drawText(250, 100, sName);
  static const int START_LINE_BASE = nHeight - 700;
  START_LINE = START_LINE_BASE;
  static QImage oI1;
  static QImage oI2;
  static QImage oI3;
  if (oI1.isNull())
  {
    RenderSvg(":/pike1.svg", &oI1);
    RenderSvg(":/pike2.svg", &oI2);
    RenderSvg(":/pike3.svg", &oI3);
  }

  DrawResultForTeam(pListTeam1, sTeamNameAndSum1, nMinSize, oI1, &oImagePainter, fQuote);
  if (nTeamCount > 1)
    DrawResultForTeam(pListTeam2, sTeamNameAndSum2, nMinSize, oI2, &oImagePainter, fQuote);
  if (nTeamCount > 2)
    DrawResultForTeam(pListTeam3, sTeamNameAndSum3, nMinSize, oI3, &oImagePainter, fQuote);

  oPaintImage.save(sPath);

  return sPath;
}

void Maep::GpsMap::saveSearchMark(int nId, QString sName, float fLo, float fLa)
{
  QString sTrackName = GpxNewName(sName, 1);

  char* szSymName = find_file("qml/symFia.png");
  cairo_surface_t* pSurface = cairo_image_surface_create_from_png(szSymName);

  g_free(szSymName);

  MarkData t;
  t.nSize = 0;
  t.nDuration = 0;
  t.la = fLa;
  t.lo = fLo;
  t.speed = 0;
  t.nType = 1;
  t.nTime = time(0);
  WriteMarkData(sTrackName, t);

  osm_gps_map_add_image_with_alignment(map, t.la, t.lo, pSurface, 0.5, 1.0,
                                       sTrackName.toUtf8().data());

  m_ocMarkers[nId] = pSurface;

  QMetaObject::invokeMethod(g_pTheTrackModel, "trackAdd", Q_ARG(QString, sTrackName));
  QMetaObject::invokeMethod(this, "loadTrack", Q_ARG(QString, sTrackName), Q_ARG(int, nId));
  QMetaObject::invokeMethod(this, "scrollToBottom");
}

void Maep::GpsMap::saveMark(int nId)
{
  coord_t tPos = osm_gps_map_get_center_ordinates(map);

  QDateTime oNow(QDateTime::currentDateTime());
  QString sTrackName = oNow.toString("yyyy-MM-dd-hh-mm-ss");

  char* szSymName = find_file("qml/symFia.png");
  cairo_surface_t* pSurface = cairo_image_surface_create_from_png(szSymName);

  g_free(szSymName);

  MarkData t;
  t.nSize = 0;
  t.nDuration = 0;
  t.la = rad2deg(tPos.rlat);
  t.lo = rad2deg(tPos.rlon);
  t.speed = lastGps.attribute(QGeoPositionInfo::Attribute::GroundSpeed);
  t.nType = 1;
  t.nTime = time(0);
  WriteMarkData(sTrackName, t);

  osm_gps_map_add_image_with_alignment(map, t.la, t.lo, pSurface, 0.5, 1.0,
                                       sTrackName.toUtf8().data());

  m_ocMarkers[nId] = pSurface;

  QMetaObject::invokeMethod(g_pTheTrackModel, "trackAdd", Q_ARG(QString, sTrackName));
  QMetaObject::invokeMethod(this, "scrollToBottom");
}

void Maep::GpsMap::saveCurrentTrack()
{

  if (track_current == 0)
    return;

  double fLen = maep_geodata_track_get_metric_length(track_current->get());

  if (fLen < 10)
    return;

  // 0 Start number (01)
  QString sTrackName = GpxNewName("Track", 0);

  QString sGpxFileName = GpxFullName(sTrackName);

  GError* error;

  maep_geodata_to_file(track_current->get(), sGpxFileName.toLatin1().data(), &error);

  MarkData t;
  t.nDuration = maep_geodata_track_get_duration(track_current->get());
  QFile oF(sGpxFileName);
  oF.open(QIODevice::ReadWrite);
  t.nSize = oF.size();
  oF.close();
  t.nType = 0;
  t.len = fLen;

  t.la = currentPos().latitude();
  t.lo = currentPos().longitude();
  t.speed = InfoListModel::MaxSpeed;
  t.nTime = time(0);

  WriteMarkData(sTrackName, t);

  QMetaObject::invokeMethod(g_pTheTrackModel, "trackAdd", Q_ARG(QString, sTrackName));
  QMetaObject::invokeMethod(this, "scrollToBottom");
}
/*
void Maep::GpsMap::setOverlaySource(Maep::GpsMap::Source value)
{
  Source orig;

  orig = overlaySource();
  if (orig == value)
    return;

  ensureOverlay(value);
 //  g_object_set(overlay, "map-source", (OsmGpsMapSource_t)value, NULL);

  // Overlay becoming NULL will never emit dirty, thus we redraw by hand
  if (value == Maep::GpsMap::SOURCE_NULL)
  {
    mapUpdate();
    update();
  }
}
*/

static void osm_gps_map_qt_auto_center(Maep::GpsMap* widget, GParamSpec* pspec, OsmGpsMap* map)
{
  Q_UNUSED(pspec);
  Q_UNUSED(map);

  widget->autoCenterChanged(widget->autoCenter());
}

void Maep::GpsMap::setAutoCenter(bool status)
{
  bool set;

  set = autoCenter();
  if ((set && status) || (!set && !status))
    return;

  if (status && lastGps.isValid())
    osm_gps_map_set_center(map, lastGps.coordinate().latitude(), lastGps.coordinate().longitude());
  g_object_set(map, "auto-center", status, NULL);
}

void Maep::GpsMap::setScreenRotation(bool status)
{
  if (status == screenRotation)
    return;

  screenRotation = status;
  emit screenRotationChanged(status);
}

void Maep::GpsMap::setSearchResults(GSList* places)
{
  if (places == nullptr)
  {
    g_pRootObject->setProperty("nSearchBusy", false);
    return;
  }

  // 1 is the id no of the result model
  MssListModel* pResultModel = MssListModel::Instance(1);

  int nCount = pResultModel->rowCount(QModelIndex());

  GSList* placeshead = places;
  int iCount = 0;
  for (; places; places = places->next)
    iCount++;

  if (nCount < iCount)
  {
    int n = iCount - nCount;
    // Should match MssListModel("fullName", "lat", "lo", "type","ref");
    for (int i = 0; i < n; ++i)
      pResultModel->AddRow({0, 0, 0, 0, 0});
  }

  if (nCount > iCount)
  {
    int n = nCount - iCount;
    for (int i = 0; i < n; ++i)
      pResultModel->removeRow(--nCount);
  }

  places = placeshead;
  int nRow = 0;
  for (; places; places = places->next)
  {
    const NominatimPlace* p = (const NominatimPlace*)places->data;
    pResultModel->updateItem(nRow, 0, p->name);
    pResultModel->updateItem(nRow, 1, rad2deg(p->pos.rlat));
    pResultModel->updateItem(nRow, 2, rad2deg(p->pos.rlon));
    pResultModel->updateItem(nRow, 3, p->type);
    pResultModel->updateItem(nRow, 4, p->ref);
    ++nRow;
  }

  g_pRootObject->setProperty("nSearchBusy", false);
}

// This is called trough g_obj signaling
static void osm_gps_map_qt_places(Maep::GpsMap* widget, GSList* places)
{
  widget->setSearchResults(places);
}

static void osm_gps_map_qt_places_failure(Maep::GpsMap* widget, GError* error)
{
  g_message("Got download error '%s'.", error->message);
  widget->setSearchResults(NULL);
}

void Maep::GpsMap::setSearchRequest(const QString& request)
{
  // qDeleteAll(searchRes);
  if (request.size() < 3)
    return;

  // searchRes.clear();

  maep_search_context_request(search, request.toLocal8Bit().data());
}

void Maep::GpsMap::setLookAt(float lat, float lon)
{
  osm_gps_map_set_center(map, lat, lon);
}

void Maep::GpsMap::setCoordinate(float lat, float lon)
{
  coordinate = QGeoCoordinate(lat, lon);
  emit coordinateChanged();
}
static void osm_gps_map_qt_coordinate(Maep::GpsMap* widget, GParamSpec* pspec, OsmGpsMap* map)
{
  Q_UNUSED(pspec);

  float lat, lon;

  g_object_get(G_OBJECT(map), "latitude", &lat, "longitude", &lon, NULL);
  widget->setCoordinate(lat, lon);
}

void Maep::GpsMap::positionUpdate(const QGeoPositionInfo& info)
{
  float track, hprec;

  // Redraw GPS position.
  maep_layer_gps_set_active(lgps, TRUE);

  //  osm_gps_map_draw_gps(map,TRUE);

  hprec = 0.;
  if (info.hasAttribute(QGeoPositionInfo::HorizontalAccuracy))
    hprec = info.attribute(QGeoPositionInfo::HorizontalAccuracy);
  track = OSM_GPS_MAP_INVALID;
  if (info.hasAttribute(QGeoPositionInfo::Direction))
    track = info.attribute(QGeoPositionInfo::Direction);
  else if (lastGps.isValid())
    /* Approximate heading with last known position */
    track = lastGps.coordinate().azimuthTo(info.coordinate());
  maep_layer_gps_set_coordinates(lgps, info.coordinate().latitude(), info.coordinate().longitude(),
                                 hprec, track);
  // To generate auto-center if necessary.
  osm_gps_map_auto_center_at(map, info.coordinate().latitude(), info.coordinate().longitude());

  lastGps = info;
  // emit gpsCoordinateChanged();
  // Add track capture, if any.
  if (track_capture)
    gpsToTrack();
}
void Maep::GpsMap::unsetGps()
{
  lastGps = QGeoPositionInfo();
  //  emit gpsCoordinateChanged();

  maep_layer_gps_set_active(lgps, FALSE);
}

void Maep::GpsMap::compassReadingChanged()
{

  //  bool bFlip = property("bFlipped").toBool();

  if (g_nSkipDraw == 1)
    return;

  if (compassEnabled() && compass.isActive())
  {
    QCompassReading* compass_reading = compass.reading();
    double azimuth = compass_reading->azimuth();
    osm_gps_map_set_azimuth(osd, azimuth);
    g_signal_emit_by_name(map, "dirty");
  }
}

void Maep::GpsMap::positionLost()
{
  if (track_capture && track_current)
    track_current->finalizeSegment();
  unsetGps();
}

void Maep::GpsMap::setGpsRefreshRate(unsigned int rate)
{
  bool restart;

  if (gpsRefreshRate_ == rate)
    return;

  restart = (gpsRefreshRate_ == 0);
  gpsRefreshRate_ = rate;
  emit gpsRefreshRateChanged(rate);

  if (rate == 0 && gps)
  {
    gps->stopUpdates();
    unsetGps();
  }
  else if (gps)
  {
    gps->setUpdateInterval(rate);
    if (restart)
      gps->startUpdates();
  }
}

void Maep::GpsMap::setTrackCapture(bool status)
{
  if (status == track_capture)
    return;

  track_capture = status;
  emit trackCaptureChanged(track_capture);
  if (status == true)
    InfoListModel::MaxSpeed = 0;

  if (status && lastGps.isValid())
  {
    //   setTrack(0);
    gpsToTrack();
  }
}

void Maep::GpsMap::setTrack(Maep::Track* track)
{

  // clear current
  osm_gps_map_clear_tracks(map);

  if (track_current && track_current->parent() == this)
    delete (track_current);

  if (track)
  {

    track->setParent(this);

    if (!track->getPath().isEmpty())
      gconf_set_string(GCONF_KEY_TRACK_PATH, track->getPath().toLocal8Bit().data());
  }
  else if (track_capture && lastGps.isValid())
  {
    track = new Maep::Track();
    track->setParent(this);
    track->addPoint(lastGps);
  }
  track_current = track;

  emit trackChanged(track != NULL);

  if (track && track->get())
  {
    coord_t top_left, bottom_right;

    /* Set track for map. */
    osm_gps_map_add_track(map, track->get(), 0, 0);

    /* Adjust map zoom and location according to track bounding box. */
    if (maep_geodata_get_bounding_box(track->get(), &top_left, &bottom_right))
      osm_gps_map_adjust_to(map, &top_left, &bottom_right);
  }
}
void Maep::GpsMap::gpsToTrack()
{
  if (!track_current)
  {
    Maep::Track* track = new Maep::Track();
    track->setParent(this);
    track->addPoint(lastGps);
    setTrack(track);
  }
  else
    track_current->addPoint(lastGps);
}

QMap<int, AisPainter::C> AisPainter::m_ocColorTable;

AisPainter::AisPainter(OsmGpsMap* _map)
{
  map = _map;

  using namespace std;

  if (m_ocColorTable.size() == 0)
  {
    C tC;

    for (int i : views::iota(-1, 99))
    {
      rgbToCario(ColorAisShipType(i), (double*)&tC);
      m_ocColorTable[i] = tC;
    }
  }
  saveDraw(map);
}

AisPainter::~AisPainter()
{
  restoreDraw(map);
}

void AisPainter::DrawAis(Point& tPos, int nType, double fHeading, double fSpeed,
                         const QByteArray& sName)
{
  int xPx, yPx;
  if (loLaToPx(map, tPos.x, tPos.y, &xPx, &yPx) == 0)
    return;

  if (nType == -1)
    setAisStyle2(map, (double*)&m_ocColorTable[nType], 1);
  else
    setAisStyle2(map, (double*)&m_ocColorTable[nType], 2);

  drawAis(map, fHeading, fSpeed, xPx, yPx, sName);
}

void AisPainter::DrawAisMoored(Point& tPos, int nType, const QByteArray& sName)
{
  int xPx, yPx;
  if (loLaToPx(map, tPos.x, tPos.y, &xPx, &yPx) == 0)
    return;

  setAisStyle2(map, (double*)&m_ocColorTable[nType], 1);

  drawAisMoored(map, xPx, yPx, sName);
}

void AisStreamClient::onError(QAbstractSocket::SocketError error)
{
  g_pRootObject->setProperty("bAisError", true);
  if (QAbstractSocket::SocketError::ConnectionRefusedError == error)
    m_webSocket.abort();
  else
    m_webSocket.close();

  g_message("onError");
}

AisStreamClient::~AisStreamClient()
{

  m_pVesselTimeoutTimer->Stop();
  m_pBoundaryTimer->Stop();
  delete m_pBoundaryTimer;
  delete m_pVesselTimeoutTimer;
}

QJsonObject AisStreamClient::GetApiKey()
{
  QJsonObject oBase;

  oBase["Apikey"] = "83dfbaf8d31a0efb443bda51635bddd25a78c9c8";

  return oBase;
}

void AisStreamClient::AddBBToJsonObj(QJsonObject& oBase, QJsonArray ocBox1)
{
  QJsonArray ocBox;
  ocBox.append(ocBox1);
  oBase["BoundingBoxes"] = ocBox;
  m_ocBoxLastSent = ocBox1;
}

void TimedWS::timerEvent(QTimerEvent*)
{

  if (state() == QAbstractSocket::SocketState::UnconnectedState)
  {
    m_nLastPing = 0;
    open(QUrl("wss://stream.aisstream.io/v0/stream"));
    return;
  }
  if (m_nLastPing != 0)
  {
    if ((abs(m_nLastPong - m_nLastPing)) > 5)
    {
      g_pRootObject->setProperty("bAisError", true);
      abort();
    }
    else
      g_pRootObject->setProperty("bAisError", false);
  }

  m_nLastPing = time(0);
  ping();
}

void TimedWS::onPong(quint64, const QByteArray&)
{
  m_nLastPong = time(0);
}

void AisStreamClient::onStateChanged(QAbstractSocket::SocketState state)
{
  // qDebug() << "state changed" << state;
}

AisStreamClient::AisStreamClient(OsmGpsMap* p, IdlePainter* pIdlePainter)
{
  m_map = p;
  m_pIdlePainter = pIdlePainter;
  connect(&m_webSocket, &QWebSocket::connected, this, &AisStreamClient::onConnected);
  connect(&m_webSocket, &QWebSocket::disconnected, this, &AisStreamClient::onDisconnected);
  connect(&m_webSocket, &QWebSocket::sslErrors, this, &AisStreamClient::onSslErrors);
  connect(&m_webSocket, &QWebSocket::pong, &m_webSocket, &TimedWS::onPong);

  connect(&m_webSocket, &QWebSocket::stateChanged, this, &AisStreamClient::onStateChanged);
  connect(&m_webSocket,
          static_cast<void (QWebSocket::*)(QAbstractSocket::SocketError)>(&QWebSocket::error), this,
          &AisStreamClient::onError);

  QSslConfiguration sslConfiguration;
  m_webSocket.setSslConfiguration(sslConfiguration);
  m_webSocket.ignoreSslErrors();
  m_webSocket.open(QUrl("wss://stream.aisstream.io/v0/stream"));

  m_pBoundaryTimer = new MssTimer([this] {
    if (m_webSocket.isValid() == false)
      return;
    auto ocBox1 = GetBoundingBoxJson();
    if (ocBox1 != m_ocBoxLast)
    {
      m_ocBoxLast = ocBox1;
      return;
    }
    if (ocBox1 == m_ocBoxLastSent)
      return;

    QJsonDocument oJD;
    QJsonObject oBase = GetApiKey();
    AddBBToJsonObj(oBase, ocBox1);
    oJD.setObject(oBase);
    qDebug() << "New Bounding box";
    // qDebug() << oJD.toJson();
    m_webSocket.sendBinaryMessage(oJD.toJson());
  });

  m_pVesselTimeoutTimer = new MssTimer([this] {
    int nT = time(0);
    std::erase_if(m_ocAis, [&](auto& o) {
      if (nT - o.second.nTimeStamp > 60)
        o.second.fSpeed = 0;

      return (nT - o.second.nTimeStamp) > 300;
    });
    m_pIdlePainter->RequestPaint();
  });

  m_pVesselTimeoutTimer->Start(10000);
  m_pBoundaryTimer->Start(5000);
  m_webSocket.startTimer(1000 * 10);
}

QJsonArray AisStreamClient::GetBoundingBoxJson()
{
  guint nwidth;
  guint nheight;
  gfloat lo, la;
  gfloat lo2, la2;
  g_object_get(G_OBJECT(m_map), "viewport-width", &nwidth, "viewport-height", &nheight, NULL);
  osm_gps_map_screen_to_geographic(m_map, 0, 0, &la, &lo);
  osm_gps_map_screen_to_geographic(m_map, nwidth, nheight, &la2, &lo2);
  QJsonArray ocBox1;
  QJsonArray ocUL{la, lo};
  QJsonArray ocLR{la2, lo2};
  ocBox1 = {ocUL, ocLR};
  return ocBox1;
}

void AisStreamClient::onDisconnected()
{
  qDebug() << "AisStream Disconnected";
}

void AisStreamClient::onConnected()
{
  g_pRootObject->setProperty("bAisError", false);

  qDebug() << "Aisstream connected";

  connect(&m_webSocket, &QWebSocket::textMessageReceived, this,
          &AisStreamClient::onTextMessageReceived);
  connect(&m_webSocket, &QWebSocket::binaryMessageReceived, this,
          &AisStreamClient::onBinaryMessageReceived);

  QJsonDocument oJD;
  QJsonObject oBase = GetApiKey();
  QJsonArray ocBox1;
  ocBox1 = GetBoundingBoxJson();
  AddBBToJsonObj(oBase, ocBox1);
  QJsonArray ocFilters;
  ocFilters.append("PositionReport");
  ocFilters.append("ShipStaticData");
  oBase["FilterMessageTypes"] = ocFilters;
  oJD.setObject(oBase);
  m_webSocket.sendBinaryMessage(oJD.toJson());
}

void AisStreamClient::onTextMessageReceived(const QString& message)
{
  qDebug() << "text message";
  onBinaryMessageReceived(message.toUtf8());
}

void AisStreamClient::onBinaryMessageReceived(const QByteArray& message)
{
  QJsonDocument oJD = QJsonDocument::fromJson(message);
  QString sType = oJD.object()["MessageType"].toString();
  auto oMeta = oJD.object()["MetaData"].toObject();
  int nMMSI = oMeta["MMSI"].toInt();
  auto oI = m_ocAis.find(nMMSI);
  if (sType == "PositionReport")
  {

    auto oJ = oJD.object()["Message"].toObject()["PositionReport"].toObject();

    if (oI == m_ocAis.end())
    {
      AisData t = {oMeta["ShipName"].toString().toUtf8(),
                   {(float)oMeta["longitude"].toDouble(), (float)oMeta["latitude"].toDouble()},
                   -1,
                   oJ["Sog"].toDouble(),
                   oJ["Cog"].toDouble() + 180,
                   oJ["NavigationalStatus"].toInt()};
      t.nTimeStamp = time(0);
      // qDebug() << "MMSI " << nMMSI;
      m_ocAis[nMMSI] = t;
    }
    else
    {
      AisData& t = oI->second;
      t.sNameUtf8 = oMeta["ShipName"].toString().toUtf8();
      t.tPos = {(float)oMeta["longitude"].toDouble(), (float)oMeta["latitude"].toDouble()};
      t.fCog = oJ["Cog"].toDouble() + 180;

      t.fSpeed = oJ["Sog"].toDouble();
      t.nNavStatus = oJ["NavigationalStatus"].toInt();
      t.nTimeStamp = time(0);
    }
  }
  else if (sType == "ShipStaticData")
  {
    if (oI != m_ocAis.end())
    {
      AisData& t = oI->second;
      t.nTimeStamp = time(0);
      auto oJ = oJD.object()["Message"].toObject()["ShipStaticData"].toObject();
      if (t.nType == -1)
      {
        t.nType = oJ["Type"].toInt();
        // qDebug() << FormatAisShipType(t.nType) << " " << t.nType << " " << t.sNameUtf8;
      }
      else
      {
        t.nType = oJ["Type"].toInt();
      }
    }
  }

  m_pIdlePainter->RequestPaint();
}

void AisStreamClient::SetBoundingBox(Point& ul, Point& lr)
{
  m_ul = ul;
  m_lr = lr;
}

void AisStreamClient::DrawAllAis()
{
  AisPainter oAisPainter(m_map);
  for (auto& [oJ, oI] : m_ocAis)
  {
    if (oI.fSpeed > 0.5)
      oAisPainter.DrawAis(oI.tPos, oI.nType, oI.fCog, oI.fSpeed, oI.sNameUtf8);
    else
      oAisPainter.DrawAisMoored(oI.tPos, oI.nType, oI.sNameUtf8);
  }
}

void AisStreamClient::onSslErrors(const QList<QSslError>& errors)
{
  qWarning() << "SSL errors:" << errors;
  g_pRootObject->setProperty("bAisError", true);
  m_webSocket.close();
}
