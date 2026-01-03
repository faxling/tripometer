/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/* vim:set et sw=4 ts=4 cino=t0,(0: */
/*
 * Copyright (C) Till Harbaum 2009 <till@harbaum.org>
 *
 * osm-gps-map is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * osm-gps-map is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../config.h"
#include "../converter.h"
#include <math.h> // M_PI/cos()
#include <stdio.h>
#include <stdlib.h> // abs
#include <string.h>

/* parameters that can be overwritten from the config file: */
/* OSD_DIAMETER */
/* OSD_X, OSD_Y */

#include <cairo.h>

#include "osm-gps-map-osd-classic.h"
#include "osm-gps-map.h"

// #define OSD_DPIX_EXTRA 0
//#define OSD_DPIX_SKIP 0

// the osd controls
typedef struct
{
  struct
  {
    cairo_surface_t* surface;
    int zoom;
    gfloat factor;
    float compass_azimuth;
  } scale;

  struct
  {
    cairo_surface_t* surface;
    gboolean rendered;
  } crosshair;

  struct
  {
    cairo_surface_t* surface;
    float lat, lon;
  } coordinates;

  // Turned on of by gui
  int* pbWeather;
  int* pbCrossHair;
  int* pbCompass;

} osd_priv_t;

/* position and extent of bounding box */
#define OSD_X (10)

#define OSD_Y (-10)

/* parameters of the "zoom" pad */
#define Z_STEP (D_RAD / 4) // distance between dpad and zoom
#define Z_RAD (D_RAD / 2) // radius of "caps" of zoom bar

#define OSD_COORDINATES_OFFSET (OSD_COORDINATES_FONT_SIZE / 4)

#define OSD_COORDINATES_W (8 * OSD_COORDINATES_FONT_SIZE + 2 * OSD_COORDINATES_OFFSET)
#define OSD_COORDINATES_H                                                                          \
  (4 * OSD_COORDINATES_FONT_SIZE + 2 * OSD_COORDINATES_OFFSET + OSD_COORDINATES_FONT_SIZE / 4)

#define OSD_COORDINATES_CHR_N "N"

#define OSD_COORDINATES_CHR_S "S"

#define OSD_COORDINATES_CHR_E "E"

#define OSD_COORDINATES_CHR_W "W"

/* this is the classic geocaching notation */
static char* osd_latitude_str(float latitude)
{
  char* c = OSD_COORDINATES_CHR_N;
  float integral, fractional;

  if (isnan(latitude))
    return NULL;

  if (latitude < 0)
  {
    latitude = fabs(latitude);
    c = OSD_COORDINATES_CHR_S;
  }

  fractional = modff(latitude, &integral);

  return g_strdup_printf("%s %02d° %06.3f'", c, (int)integral, fractional * 60.0);
}

static char* osd_longitude_str(float longitude)
{
  char* c = OSD_COORDINATES_CHR_E;
  float integral, fractional;

  if (isnan(longitude))
    return NULL;

  if (longitude < 0)
  {
    longitude = fabs(longitude);
    c = OSD_COORDINATES_CHR_W;
  }

  fractional = modff(longitude, &integral);

  return g_strdup_printf("%s %03d° %06.3f'", c, (int)integral, fractional * 60.0);
}

/* render a string at the given screen position */
static int osd_render_text_w_halo(cairo_t* cr, int x, int y, int nFontSize, char* text )
{
  if (!text)
    return y;

  cairo_set_font_size(cr, nFontSize);
  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_set_line_width(cr, nFontSize / 6);
  cairo_move_to(cr, x, y );
  cairo_text_path(cr, text);
  cairo_stroke(cr);

  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  cairo_move_to(cr, x, y );
  cairo_show_text(cr, text);
  return y + nFontSize  ;
}

static void osd_render_coordinates(osm_gps_map_osd_t* osd)
{
  osd_priv_t* priv = (osd_priv_t*)osd->priv;

  if (!priv->coordinates.surface)
    return;

  /* get current map position */
  gfloat lat, lon;
  g_object_get(osd->map, "latitude", &lat, "longitude", &lon, NULL);

  /* check if position has changed enough to require redraw */
  if (!isnan(priv->coordinates.lat) && !isnan(priv->coordinates.lon))
    /* 1/60000 == 1/1000 minute */
    if ((fabsf(lat - priv->coordinates.lat) < 1 / 60000) &&
        (fabsf(lon - priv->coordinates.lon) < 1 / 60000))
      return;

  priv->crosshair.rendered = FALSE;
  priv->coordinates.lat = lat;
  priv->coordinates.lon = lon;

  /* first fill with transparency */

  g_assert(priv->coordinates.surface);
  cairo_t* cr = cairo_create(priv->coordinates.surface);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  //    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 0.5);
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.0);
  cairo_paint(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);


  char* latitude = osd_latitude_str(lat);
  char* longitude = osd_longitude_str(lon);

  int y = OSD_COORDINATES_OFFSET*4;
  y = osd_render_text_w_halo(cr,0,y,OSD_COORDINATES_FONT_SIZE, latitude);
  y = osd_render_text_w_halo(cr,0, y, OSD_COORDINATES_FONT_SIZE, longitude);

  // Render distance tool

  guint nwidth;
  guint nheight;

  g_object_get(G_OBJECT(osd->map), "viewport-width", &nwidth, "viewport-height", &nheight, NULL);
  float fx, fy;

  osm_gps_map_screen_to_geographic(osd->map, nwidth / 2, nheight / 2, &fy, &fx);
  float fDist = osm_db_last_dist(osd->map, fy, fx);

  int nD = osm_gps_map_depth(osd->map);
  if (fDist > 0)
  {
    gchar* dist_str = g_strdup_printf("Dist %.3f km", fDist / 1000);
    y = osd_render_text_w_halo(cr,0,y, OSD_COORDINATES_FONT_SIZE, dist_str);
    g_free(dist_str);
  }

  if (nD > 0)
  {
    if (nD < 200)
    {
      gchar* deep_str = g_strdup_printf("Depth %.1f m", nD / 10.0);
      osd_render_text_w_halo(cr,0,y, OSD_COORDINATES_FONT_SIZE, deep_str);
      g_free(deep_str);
    }
    else
      osd_render_text_w_halo(cr,0,y,OSD_COORDINATES_FONT_SIZE, ">= 20 m");
  }

  g_free(latitude);
  g_free(longitude);

  cairo_destroy(cr);
}
static void onLatLon(G_GNUC_UNUSED GObject* obj, G_GNUC_UNUSED GParamSpec* pspec,
                     gpointer user_data)
{
  osm_gps_map_osd_t* osd = (osm_gps_map_osd_t*)user_data;

  osd_render_coordinates(osd);
}
// Assigned in osm_gps_map_osd_classic_init
int OSD_CROSSHAIR_WH = -1;
int OSD_CROSSHAIR_RADIUS = -1;
int OSD_CROSSHAIR_BORDER = -1;
int MAXARR = -1;

void moveTo(cairo_t* cr, double v, int nR)
{
  double y = -cos(v);
  double x = sin(v);
  cairo_move_to(cr, x * nR + OSD_CROSSHAIR_WH / 2, y * nR + OSD_CROSSHAIR_WH / 2);
}

void drawLineTo(cairo_t* cr, double v, int nR)
{
  double y = -cos(v);
  double x = sin(v);
  cairo_line_to(cr, x * nR + OSD_CROSSHAIR_WH / 2, y * nR + OSD_CROSSHAIR_WH / 2);
}
static void drawLineFromTo(cairo_t* cr, double nx, double ny, double v, int nR)
{
  double y = -cos(v);
  double x = sin(v);
  cairo_line_to(cr, x * nR + nx, y * nR + ny);
}

// #define MAXARR (OSD_CROSSHAIR_H / 2 - 5)
static void drawArrowTo(cairo_t* cr, double v, int nR)
{

  if (nR > MAXARR)
    nR = MAXARR;
  double y = -cos(v) * nR + OSD_CROSSHAIR_WH / 2;
  double x = sin(v) * nR + OSD_CROSSHAIR_WH / 2;
  cairo_line_to(cr, x, y);
  cairo_stroke(cr);
  cairo_move_to(cr, x, y);
  drawLineFromTo(cr, x, y, v + M_PI + 0.5, 10);
  cairo_move_to(cr, x, y);
  drawLineFromTo(cr, x, y, v + M_PI - 0.5, 10);
  cairo_stroke(cr);
}

static void drawCrosshair(cairo_t* cr)
{
  int i;
  moveTo(cr, M_PI_2, OSD_CROSSHAIR_RADIUS);
  // Cirkel??
  cairo_arc(cr, OSD_CROSSHAIR_WH / 2, OSD_CROSSHAIR_WH / 2, OSD_CROSSHAIR_RADIUS, 0, 2 * M_PI);
  for (i = 0; i < 4; ++i)
  {
    moveTo(cr, M_PI_2 * i, OSD_CROSSHAIR_RADIUS);
    drawLineTo(cr, M_PI_2 * i, OSD_CROSSHAIR_RADIUS * 2);
  }

  cairo_stroke(cr);
}

extern int g_nFontSizePx;

void setStyle(cairo_t* cr, unsigned int nRGB, double fWidth)
{
  if (cr == 0)
    return;
  cairo_set_line_width(cr, fWidth);
  static double rgb[3];
  rgbToCario(nRGB, rgb);
  cairo_set_source_rgb(cr, rgb[0], rgb[1], rgb[2]);
}

void setColor(cairo_t* cr, unsigned int nRGB)
{
  static double rgb[3];
  rgbToCario(nRGB, rgb);
  cairo_set_source_rgb(cr, rgb[0], rgb[1], rgb[2]);
}

static void osd_render_crosshair(osm_gps_map_osd_t* osd)
{
  osd_priv_t* priv = (osd_priv_t*)osd->priv;

  if (!priv->crosshair.surface || priv->crosshair.rendered)
    return;

  priv->crosshair.rendered = TRUE;

  /* first fill with transparency */
  g_assert(priv->crosshair.surface);
  cairo_t* cr = cairo_create(priv->crosshair.surface);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba(cr, 0.0, 0, 0.0, 0);
  cairo_paint(cr);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);




  setStyle(cr, 0x00f4fb, 4);

  drawCrosshair(cr);
  setStyle(cr, 0, 1);
  drawCrosshair(cr);
  if (*priv->pbWeather == 0)
  {
    cairo_destroy(cr);
    return;
  }


  // Draw weather

  double w = windDirRad(osd->map) + M_PI;
  int nTemp = lround(tempDeg(osd->map));
  int nMs = lround(windSpeedMs(osd->map));
  double fUvIndex = uvIndex(osd->map);

  setStyle(cr, 0xffdb00,7);
  cairo_arc(cr, OSD_CROSSHAIR_WH / 2, OSD_CROSSHAIR_WH / 2, OSD_CROSSHAIR_RADIUS / 1.5, M_PI , M_PI + (M_PI * (fUvIndex / 6)));
  cairo_stroke(cr);

  int nAL = OSD_CROSSHAIR_RADIUS * (nMs / 5.0) + OSD_CROSSHAIR_RADIUS;
  moveTo(cr, w, 5);
  setStyle(cr, 0x00f4fb, 3);
  drawArrowTo(cr, w, nAL);

  moveTo(cr, w, 5);
  setStyle(cr, 0x3300ff, 1);
  drawArrowTo(cr, w, nAL);
  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_source_rgb(cr, 0, 0, 0.0);
  cairo_set_font_size(cr, g_nFontSizePx);
  int nMargin = g_nFontSizePx / 2;
  moveTo(cr, -M_PI_2, nMargin);
  char wind_str[20];
  char temp_str[20];
  sprintf(wind_str, "%d", nMs);
  cairo_show_text(cr, wind_str);
  sprintf(temp_str, "%d", nTemp);

  if (nTemp < 0)
    nMargin *= 2;

  cairo_move_to(cr, OSD_CROSSHAIR_WH / 2 - nMargin,
                OSD_CROSSHAIR_WH / 2 + g_nFontSizePx - nMargin / 3);

  cairo_show_text(cr, temp_str);
  int nFS = g_nFontSizePx/ 1.5;
  int nMidle = OSD_CROSSHAIR_WH / 2 - 3;
  // cairo_set_font_size(cr, nFS);
  char sun_str[20];
  time_t nT = sunrise(osd->map);
  struct tm* t = localtime(&nT);
  strftime(sun_str, 20, "%H:%M", t);
  osd_render_text_w_halo(cr,0,nMidle,nFS,sun_str);

  nT = sunset(osd->map);
  t = localtime(&nT);
  strftime(sun_str, 20, "%H:%M", t);

  osd_render_text_w_halo(cr,OSD_CROSSHAIR_WH- nFS*3 ,nMidle , nFS,sun_str);

  cairo_destroy(cr);
}

#define OSD_SCALE_W (10 * OSD_SCALE_FONT_SIZE)
#define OSD_SCALE_H (5 * OSD_SCALE_FONT_SIZE / 2)

/* various parameters used to create the scale */
#define OSD_SCALE_H2 (OSD_SCALE_H / 2)
#define OSD_SCALE_TICK (2 * OSD_SCALE_FONT_SIZE / 3)
#define OSD_SCALE_M (OSD_SCALE_H2 - OSD_SCALE_TICK)
#define OSD_SCALE_I (OSD_SCALE_H2 + OSD_SCALE_TICK)
#define OSD_SCALE_FD (OSD_SCALE_FONT_SIZE / 4)
#define OSD_SCALE_FD_X 20
#define MAX_SCALE_TEXT_LEN 20


void osd_render_scale_and_compass(osm_gps_map_osd_t* osd)
{
  osd_priv_t* priv = (osd_priv_t*)osd->priv;

  if (!priv->scale.surface)
    return;

  float m_per_pix = osm_gps_map_get_scale(OSM_GPS_MAP(osd->map));

  /* first fill with transparency */
  g_assert(priv->scale.surface);
  cairo_t* cr = cairo_create(priv->scale.surface);
  cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
  cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.0);
  // pink for testing:    cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.2);
  cairo_paint(cr);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  // Compass

  if (*priv->pbCompass != 0)
  {
    float r = 10;
    int nPos = 150;

    //  g_message("az %f",priv->scale.compass_azimuth);

    cairo_move_to(cr, nPos + -r * cos(priv->scale.compass_azimuth),
                  nPos + r * sin(priv->scale.compass_azimuth));
    cairo_line_to(cr, nPos + -14.0 * r * sin(priv->scale.compass_azimuth),
                  nPos + -14.0 * r * cos(priv->scale.compass_azimuth));
    cairo_line_to(cr, nPos + r * cos(priv->scale.compass_azimuth),
                  nPos + -r * sin(priv->scale.compass_azimuth));
    cairo_close_path(cr);

    cairo_set_source_rgba(cr, 1.0, 0.3, 0.3, 0.5);

    cairo_fill_preserve(cr);
    cairo_set_source_rgba(cr, 0.0, 0.5, 0.0, 0.5);
    cairo_set_line_width(cr, 1.0);

    cairo_stroke(cr);

    cairo_move_to(cr, nPos + r * cos(priv->scale.compass_azimuth),
                  nPos + -r * sin(priv->scale.compass_azimuth));
    cairo_line_to(cr, nPos + 14.0 * r * sin(priv->scale.compass_azimuth),
                  nPos + 14.0 * r * cos(priv->scale.compass_azimuth));
    cairo_line_to(cr, nPos + -r * cos(priv->scale.compass_azimuth),
                  nPos + r * sin(priv->scale.compass_azimuth));
    cairo_close_path(cr);
  }

  cairo_set_source_rgba(cr, 0.3, 1.0, 0.3, 0.5);

  cairo_fill_preserve(cr);

  cairo_set_line_width(cr, 1.0);
  cairo_set_source_rgba(cr, 0.5, 0.0, 0.0, 0.5);
  cairo_stroke(cr);

  // width used both for meter and yards
  /* determine the size of the scale width in meters */
  float width = (OSD_SCALE_W - OSD_SCALE_FONT_SIZE / 6) * m_per_pix;

  int exp = logf(width) * M_LOG10E;
  int mant = width / pow(10, exp);
  int width_metric = mant * pow(10, exp);

  char dist_str_metric[MAX_SCALE_TEXT_LEN];
  char dist_str_imperial[MAX_SCALE_TEXT_LEN];
  if (width_metric < 1000)
    sprintf(dist_str_metric, "%u m",width_metric);
  else
    sprintf(dist_str_metric, "%u km", width_metric / 1000);

  width_metric /= m_per_pix;

  /* and now the hard part: scale for useful imperial values :-( */
  /* try to convert to feet, 1ft == 0.3048 m */
  width /= 0.3048;
  float imp_scale = 0.3048;
  char* dist_imp_unit = "ft";

  if (width >= 100)
  {
    /* 1yd == 3 feet */
    width /= 3.0;
    imp_scale *= 3.0;
    dist_imp_unit = "yd";

    if (width >= 1760.0)
    {
      /* 1mi == 1760 yd */
      width /= 1760.0;
      imp_scale *= 1760.0;
      dist_imp_unit = "mi";
    }
  }

  /* also convert this to full tens/hundreds */
  exp = logf(width) * M_LOG10E;
  mant = width / pow(10, exp);
  int width_imperial = mant * pow(10, exp);
  sprintf(dist_str_imperial , "%u %s",width_imperial ,dist_imp_unit);

  /* convert back to pixels */
  width_imperial *= imp_scale;
  width_imperial /= m_per_pix;

  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);

  int y = osd_render_text_w_halo(cr, 2 * OSD_SCALE_FD_X, OSD_SCALE_H2 - OSD_SCALE_FD,OSD_SCALE_FONT_SIZE,dist_str_metric);
  osd_render_text_w_halo(cr, 2 * OSD_SCALE_FD_X, y + OSD_SCALE_FONT_SIZE / 6,OSD_SCALE_FONT_SIZE,dist_str_imperial);


  int LEFT_X = OSD_SCALE_FONT_SIZE / 3;
  /* draw white line */
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
  cairo_set_line_width(cr, OSD_SCALE_FONT_SIZE / 3);
  cairo_move_to(cr, LEFT_X, OSD_SCALE_M);
  cairo_rel_line_to(cr, 0, OSD_SCALE_TICK);
  cairo_rel_line_to(cr, width_metric, 0);
  cairo_rel_line_to(cr, 0, -OSD_SCALE_TICK);
  cairo_stroke(cr);
  cairo_move_to(cr, LEFT_X, OSD_SCALE_I);
  cairo_rel_line_to(cr, 0, -OSD_SCALE_TICK);
  cairo_rel_line_to(cr, width_imperial, 0);
  cairo_rel_line_to(cr, 0, +OSD_SCALE_TICK);
  cairo_stroke(cr);

  /* draw black line */
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
  cairo_set_line_width(cr, OSD_SCALE_FONT_SIZE / 6);
  cairo_move_to(cr, LEFT_X, OSD_SCALE_M);
  cairo_rel_line_to(cr, 0, OSD_SCALE_TICK);
  cairo_rel_line_to(cr, width_metric, 0);
  cairo_rel_line_to(cr, 0, -OSD_SCALE_TICK);
  cairo_stroke(cr);
  cairo_move_to(cr, LEFT_X, OSD_SCALE_I);
  cairo_rel_line_to(cr, 0, -OSD_SCALE_TICK);
  cairo_rel_line_to(cr, width_imperial, 0);
  cairo_rel_line_to(cr, 0, +OSD_SCALE_TICK);
  cairo_stroke(cr);

  cairo_destroy(cr);
}

static void onZoom(G_GNUC_UNUSED GObject* gobject, G_GNUC_UNUSED GParamSpec* pspec,
                   gpointer user_data)
{
  osm_gps_map_osd_t* osd = (osm_gps_map_osd_t*)user_data;
  osd_render_scale_and_compass(osd);
}

static void osd_draw(osm_gps_map_osd_t* osd, cairo_t* cr)
{

  osd_priv_t* priv = (osd_priv_t*)osd->priv;

  if (!priv->scale.surface)
  {
    priv->scale.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 300, 300);
    priv->scale.zoom = -1;
    priv->scale.factor = 0.f;
  }

  osd_render_scale_and_compass(osd);

  if (!priv->crosshair.surface)
  {
  g_message("OSD_CROSSHAIR_WH %d", OSD_CROSSHAIR_WH);
    priv->crosshair.surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, OSD_CROSSHAIR_WH, OSD_CROSSHAIR_WH);
  }

  if (*(priv->pbCrossHair) != 0)
    osd_render_crosshair(osd);

  if (!priv->coordinates.surface)
  {
    priv->coordinates.surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, OSD_COORDINATES_W, OSD_COORDINATES_H);

    priv->coordinates.lat = priv->coordinates.lon = OSM_GPS_MAP_INVALID;
  }
  osd_render_coordinates(osd);

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  // now draw this onto the original context
  gint x, y;
  guint width, height;

  g_object_get(G_OBJECT(osd->map), "viewport-width", &width, "viewport-height", &height, NULL);

  x = OSD_X*2;
  y = -OSD_Y;
  if (x < 0)
    x += width - OSD_SCALE_W;
  if (y < 0)
    y += height - OSD_SCALE_H;

  cairo_set_source_surface(cr, priv->scale.surface, x, y);
  cairo_paint(cr);

  if (*(priv->pbCrossHair) != 0)
  {
    x = (width - OSD_CROSSHAIR_WH) / 2;
    y = (height - OSD_CROSSHAIR_WH) / 2;

    cairo_set_source_surface(cr, priv->crosshair.surface, x, y);
    cairo_paint(cr);
  }
  x = -OSD_X;
  y = -OSD_Y;
  if (x < 0)
    x += width - OSD_COORDINATES_W;
  if (y < 0)
    y += height - OSD_COORDINATES_H;

  cairo_set_source_surface(cr, priv->coordinates.surface, x, y);
  cairo_paint(cr);
}

static void osd_free(osm_gps_map_osd_t* osd)
{
  osd_priv_t* priv = (osd_priv_t*)(osd->priv);

  if (priv->scale.surface)
    cairo_surface_destroy(priv->scale.surface);

  if (priv->crosshair.surface)
    cairo_surface_destroy(priv->crosshair.surface);

  if (priv->coordinates.surface)
    cairo_surface_destroy(priv->coordinates.surface);
  g_free(priv);
  osd->priv = NULL;
}

osm_gps_map_osd_t* osm_gps_map_osd_classic_init(OsmGpsMap* map)
{
  osm_gps_map_osd_t* osd_classic = g_new0(osm_gps_map_osd_t, 1);
  osd_priv_t* priv = g_new0(osd_priv_t, 1);
  osd_classic->map = NULL;
  osd_classic->priv = priv;
  priv->scale.compass_azimuth = 0;
  osd_classic->draw = osd_draw;
  osd_classic->map = map;
  g_object_ref(map);

  g_signal_connect(G_OBJECT(map), "notify::zoom", G_CALLBACK(onZoom), osd_classic);
  g_signal_connect(G_OBJECT(map), "notify::factor", G_CALLBACK(onZoom), osd_classic);
  g_signal_connect(G_OBJECT(map), "notify::latitude", G_CALLBACK(onLatLon), osd_classic);

  priv->pbWeather = (int*)g_object_get_data(G_OBJECT(map), GCONF_KEY_WEATHER);
  priv->pbCrossHair = (int*)g_object_get_data(G_OBJECT(map), GCONF_KEY_CROSSHAIR);
  priv->pbCompass = (int*)g_object_get_data(G_OBJECT(map), GCONF_KEY_COMPASS_ENABLED);
  int nQ = g_nFontSizePx * 1.5;
  OSD_CROSSHAIR_BORDER = nQ;
  OSD_CROSSHAIR_RADIUS = (nQ * 2) / 3;
  OSD_CROSSHAIR_WH = (OSD_CROSSHAIR_BORDER + OSD_CROSSHAIR_RADIUS) * 2 + nQ;
  MAXARR = OSD_CROSSHAIR_WH / 2;
  return osd_classic;
}

void osm_gps_map_osd_classic_free(osm_gps_map_osd_t* osd)
{
  osd_free(osd);
  g_object_unref(osd->map);
  g_free(osd);
}

void osm_gps_map_set_azimuth(osm_gps_map_osd_t* osd, double azimuth)
{
  osd_priv_t* priv = (osd_priv_t*)osd->priv;
  priv->scale.compass_azimuth = deg2rad((float)azimuth);
}
