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

#define OSD_DPIX_EXTRA 0
#define OSD_DPIX_SKIP 0

// the osd controls
typedef struct
{
  /* the offscreen representation of the OSD */
  struct
  {
    cairo_surface_t* surface;
    gboolean rendered;
#ifdef OSD_GPS_BUTTON
    gboolean gps_enabled;
#endif
  } controls;

#ifdef OSD_SCALE
  struct
  {
    cairo_surface_t* surface;
    int zoom;
    gfloat factor;
    float compass_azimuth;
  } scale;
#endif

#ifdef OSD_CROSSHAIR
  struct
  {
    cairo_surface_t* surface;
    gboolean rendered;
  } crosshair;
#endif

#ifdef OSD_COORDINATES
  struct
  {
    cairo_surface_t* surface;
    float lat, lon;
  } coordinates;
#endif

} osd_priv_t;

#define OSD_STATE_DOWN 0
#define OSD_STATE_UP 1
#define OSD_STATE_CHECK 2

/* position and extent of bounding box */
#ifndef OSD_X
#define OSD_X (10)
#endif

#ifndef OSD_Y
#define OSD_Y (10)
#endif

/* parameters of the direction shape */
#ifndef OSD_DIAMETER
#define D_RAD (30) // diameter of dpad
#else
#define D_RAD (OSD_DIAMETER)
#endif
#define D_TIP (4 * D_RAD / 5) // distance of arrow tip from dpad center
#define D_LEN (D_RAD / 4) // length of arrow
#define D_WID (D_LEN) // width of arrow

/* parameters of the "zoom" pad */
#define Z_STEP (D_RAD / 4) // distance between dpad and zoom
#define Z_RAD (D_RAD / 2) // radius of "caps" of zoom bar

#ifdef OSD_SHADOW_ENABLE
/* shadow also depends on control size */
#define OSD_SHADOW (D_RAD / 6)
#else
#define OSD_SHADOW (0)
#endif

/* normally the GPS button is in the center of the dpad. if there's */
/* no dpad it will go into the zoom area */
#if defined(OSD_GPS_BUTTON) && defined(OSD_NO_DPAD)
#define Z_GPS 1
#else
#define Z_GPS 0
#endif

/* total width and height of controls incl. shadow */
#define OSD_W (2 * D_RAD + OSD_SHADOW + Z_GPS * 2 * Z_RAD)
#if !Z_GPS
#define OSD_H (2 * D_RAD + Z_STEP + 2 * Z_RAD + OSD_SHADOW)
#else
#define OSD_H (2 * Z_RAD + OSD_SHADOW)
#endif

#ifdef OSD_SHADOW_ENABLE
#define OSD_LBL_SHADOW (OSD_SHADOW / 2)
#endif

#define Z_TOP ((1 - Z_GPS) * (2 * D_RAD + Z_STEP))

#define Z_MID (Z_TOP + Z_RAD)
#define Z_BOT (Z_MID + Z_RAD)
#define Z_LEFT (Z_RAD)
#define Z_RIGHT (2 * D_RAD - Z_RAD + Z_GPS * 2 * Z_RAD)
#define Z_CENTER ((Z_RIGHT + Z_LEFT) / 2)

/*
static gboolean
osm_gps_map_in_circle(gint x, gint y, gint cx, gint cy, gint rad)
{
  return( pow(cx - x, 2) + pow(cy - y, 2) < rad * rad);
}
*/

#define Z_LEN (2 * Z_RAD / 3)

#ifdef OSD_COORDINATES

#ifndef OSD_COORDINATES_FONT_SIZE
#define OSD_COORDINATES_FONT_SIZE (12.0)
#endif

#define OSD_COORDINATES_OFFSET (OSD_COORDINATES_FONT_SIZE / 6)

#define OSD_COORDINATES_W (8 * OSD_COORDINATES_FONT_SIZE + 2 * OSD_COORDINATES_OFFSET)
#define OSD_COORDINATES_H                                                                          \
  (4 * OSD_COORDINATES_FONT_SIZE + 2 * OSD_COORDINATES_OFFSET + OSD_COORDINATES_FONT_SIZE / 4)

/* these can be overwritten with versions that support */
/* localization */
#ifndef OSD_COORDINATES_CHR_N
#define OSD_COORDINATES_CHR_N "N"
#endif
#ifndef OSD_COORDINATES_CHR_S
#define OSD_COORDINATES_CHR_S "S"
#endif
#ifndef OSD_COORDINATES_CHR_E
#define OSD_COORDINATES_CHR_E "E"
#endif
#ifndef OSD_COORDINATES_CHR_W
#define OSD_COORDINATES_CHR_W "W"
#endif

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
static int osd_render_centered_text(cairo_t* cr, int y, int width, char* text)
{
  if (!text)
    return y;

  char* p = g_malloc(strlen(text) + 4); // space for "...\n"
  strcpy(p, text);

  cairo_text_extents_t extents;
  memset(&extents, 0, sizeof(cairo_text_extents_t));
  cairo_text_extents(cr, p, &extents);
  g_assert(extents.width != 0.0);

  /* check if text needs to be truncated */
  int trunc_at = strlen(text);
  while (extents.width > width)
  {

    /* cut off all utf8 multibyte remains so the actual */
    /* truncation only deals with one byte */
    while ((p[trunc_at - 1] & 0xc0) == 0x80)
    {
      trunc_at--;
      g_assert(trunc_at > 0);
    }

    trunc_at--;
    g_assert(trunc_at > 0);

    strcpy(p + trunc_at, "...");
    cairo_text_extents(cr, p, &extents);
  }

  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_set_line_width(cr, OSD_COORDINATES_FONT_SIZE / 6);
  cairo_move_to(cr, 0, y - extents.y_bearing);
  cairo_text_path(cr, p);
  cairo_stroke(cr);

  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  cairo_move_to(cr, 0, y - extents.y_bearing);
  cairo_show_text(cr, p);

  g_free(p);

  /* skip + 1/5 line */
  return y + 6 * OSD_COORDINATES_FONT_SIZE / 5;
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
  cairo_set_font_size(cr, OSD_COORDINATES_FONT_SIZE);

  char* latitude = osd_latitude_str(lat);
  char* longitude = osd_longitude_str(lon);

  int y = OSD_COORDINATES_OFFSET;
  y = osd_render_centered_text(cr, y, OSD_COORDINATES_W, latitude);
  y = osd_render_centered_text(cr, y, OSD_COORDINATES_W, longitude);

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
    y = osd_render_centered_text(cr, y, OSD_COORDINATES_W, dist_str);
    g_free(dist_str);
  }

  if (nD > 0)
  {
    if (nD < 200)
    {
      gchar* deep_str = g_strdup_printf("Depth %.1f m", nD / 10.0);
      y = osd_render_centered_text(cr, y, OSD_COORDINATES_W, deep_str);
      g_free(deep_str);
    }
    else
      y = osd_render_centered_text(cr, y, OSD_COORDINATES_W, ">= 20 m");
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
#endif // OSD_COORDINATES

static osd_button_t osd_check_int(osm_gps_map_osd_t* osd, gboolean click, gint state, gint x,
                                  gint y)
{
  osd_button_t but = OSD_NONE;

  return but;
}

#ifdef OSD_CROSSHAIR

#ifndef OSD_CROSSHAIR_RADIUS
#define OSD_CROSSHAIR_RADIUS 20
#endif

#define OSD_CROSSHAIR_TICK (OSD_CROSSHAIR_RADIUS)
#define OSD_CROSSHAIR_BORDER (OSD_CROSSHAIR_TICK + OSD_CROSSHAIR_RADIUS / 4)
#define OSD_CROSSHAIR_W ((OSD_CROSSHAIR_RADIUS + OSD_CROSSHAIR_BORDER) * 2)
#define OSD_CROSSHAIR_H ((OSD_CROSSHAIR_RADIUS + OSD_CROSSHAIR_BORDER) * 2)

void moveTo(cairo_t* cr, double v, int nR)
{
  double y = -cos(v);
  double x = sin(v);
  cairo_move_to(cr, x * nR + OSD_CROSSHAIR_W / 2, y * nR + OSD_CROSSHAIR_H / 2);
}

void drawLineTo(cairo_t* cr, double v, int nR)
{
  double y = -cos(v);
  double x = sin(v);
  cairo_line_to(cr, x * nR + OSD_CROSSHAIR_W / 2, y * nR + OSD_CROSSHAIR_H / 2);
}

static void osd_render_crosshair_shape(cairo_t* cr)
{
  int i;
  moveTo(cr, M_PI_2, OSD_CROSSHAIR_RADIUS);
  // Cirkel??
  cairo_arc(cr, OSD_CROSSHAIR_W / 2, OSD_CROSSHAIR_H / 2, OSD_CROSSHAIR_RADIUS, 0, 2 * M_PI);
  for (i = 0; i < 4; ++i)
  {
    moveTo(cr, M_PI_2 * i, OSD_CROSSHAIR_RADIUS);
    drawLineTo(cr, M_PI_2 * i, OSD_CROSSHAIR_RADIUS * 2);
  }

  /*
  cairo_line_to(cr, -OSD_CROSSHAIR_TICK, 0);
  cairo_move_to(cr, OSD_CROSSHAIR_W / 2 + OSD_CROSSHAIR_RADIUS, OSD_CROSSHAIR_H / 2);
  cairo_line_to(cr, OSD_CROSSHAIR_TICK, 0);

  cairo_move_to(cr, OSD_CROSSHAIR_W / 2, OSD_CROSSHAIR_H / 2 - OSD_CROSSHAIR_RADIUS);
  cairo_line_to(cr, 0, -OSD_CROSSHAIR_TICK);
  cairo_move_to(cr, OSD_CROSSHAIR_W / 2, OSD_CROSSHAIR_H / 2 + OSD_CROSSHAIR_RADIUS);
  cairo_line_to(cr, 0, OSD_CROSSHAIR_TICK);
  */
  cairo_stroke(cr);
}

static const double A = 0.6;

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

  //  cairo_set_line_width(cr, 1.0);

  //  cairo_stroke(cr);

  // moveTo(cr,0, OSD_CROSSHAIR_W / 2);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  // cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

  cairo_set_source_rgb(cr, 0, 0xf4 / 255.0, 0xfb / 255.0);
  cairo_set_line_width(cr, 6);
  osd_render_crosshair_shape(cr);
  cairo_set_line_width(cr, 1);
  cairo_set_source_rgb(cr, 0, 0, 0);
  osd_render_crosshair_shape(cr);
  double v = windDirRad(osd->map);

  cairo_set_line_width(cr, OSD_CROSSHAIR_RADIUS / 10);
  cairo_set_source_rgb(cr, 0x33 / 255.0, 0, 255);

  moveTo(cr, -v, 5);
  double dTemp = tempDeg(osd->map);
  double dMs = windSpeedMs(osd->map);
  drawLineTo(cr, -v, OSD_CROSSHAIR_RADIUS * (dMs / 5) + OSD_CROSSHAIR_RADIUS);
  drawLineTo(cr, -v, OSD_CROSSHAIR_RADIUS * (dMs / 5) + OSD_CROSSHAIR_RADIUS);
  /*
  drawLineTo(cr, v + A, OSD_CROSSHAIR_RADIUS / 3);
  drawLineTo(cr, v , OSD_CROSSHAIR_RADIUS / 3 + 5);
  drawLineTo(cr, v - A, OSD_CROSSHAIR_RADIUS / 3 );
  */
  cairo_stroke(cr);
  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  cairo_set_source_rgb(cr, 0, 0, 0.0);
  cairo_set_font_size(cr, 20);
  moveTo(cr, -M_PI_2, 10);
  char dist_str[20];
  char temp_str[20];
  sprintf(dist_str, "%d", (int)dMs);
  cairo_show_text(cr, dist_str);
  sprintf(temp_str, "%d", (int)dTemp);

  cairo_move_to(cr, OSD_CROSSHAIR_W / 2 - 10, OSD_CROSSHAIR_H / 2 + 15);
  cairo_show_text(cr, temp_str);

  cairo_destroy(cr);
}

#endif

#ifdef OSD_SCALE

#ifndef OSD_SCALE_FONT_SIZE
#define OSD_SCALE_FONT_SIZE (12.0)
#endif
#define OSD_SCALE_W (10 * OSD_SCALE_FONT_SIZE)
#define OSD_SCALE_H (5 * OSD_SCALE_FONT_SIZE / 2)

/* various parameters used to create the scale */
#define OSD_SCALE_H2 (OSD_SCALE_H / 2)
#define OSD_SCALE_TICK (2 * OSD_SCALE_FONT_SIZE / 3)
#define OSD_SCALE_M (OSD_SCALE_H2 - OSD_SCALE_TICK)
#define OSD_SCALE_I (OSD_SCALE_H2 + OSD_SCALE_TICK)
#define OSD_SCALE_FD (OSD_SCALE_FONT_SIZE / 4)

void osd_render_scale(osm_gps_map_osd_t* osd)
{
  osd_priv_t* priv = (osd_priv_t*)osd->priv;

  if (!priv->scale.surface)
    return;

  /* this only needs to be rendered if the zoom has changed */
  /* gint zoom; */
  /* gfloat factor; */
  /* g_object_get(OSM_GPS_MAP(osd->map), "zoom", &zoom, "factor", &factor, NULL); */
  /* if(zoom == priv->scale.zoom && factor == priv->scale.factor) */
  /*     return; */

  /* priv->scale.zoom = zoom; */
  /* priv->scale.factor = factor; */

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
  if (!isnan(priv->scale.compass_azimuth))
  {
    float r = 10;
    int nPos = 150;

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

  /* determine the size of the scale width in meters */
  float width = (OSD_SCALE_W - OSD_SCALE_FONT_SIZE / 6) * m_per_pix;

  /* scale this to useful values */
  int exp = logf(width) * M_LOG10E;
  int mant = width / pow(10, exp);
  int width_metric = mant * pow(10, exp);
  char* dist_str = NULL;
  if (width_metric < 1000)
    dist_str = g_strdup_printf("%u m", width_metric);
  else
    dist_str = g_strdup_printf("%u km", width_metric / 1000);
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
  int width_imp = mant * pow(10, exp);
  char* dist_str_imp = g_strdup_printf("%u %s", width_imp, dist_imp_unit);

  /* convert back to pixels */
  width_imp *= imp_scale;
  width_imp /= m_per_pix;

  cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
  cairo_set_font_size(cr, OSD_SCALE_FONT_SIZE);
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);

  cairo_text_extents_t extents;
  cairo_text_extents(cr, dist_str, &extents);

  cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
  cairo_set_line_width(cr, OSD_SCALE_FONT_SIZE / 6);
  cairo_move_to(cr, 2 * OSD_SCALE_FD, OSD_SCALE_H2 - OSD_SCALE_FD);
  cairo_text_path(cr, dist_str);
  cairo_stroke(cr);
  cairo_move_to(cr, 2 * OSD_SCALE_FD, OSD_SCALE_H2 + OSD_SCALE_FD + extents.height);
  cairo_text_path(cr, dist_str_imp);
  cairo_stroke(cr);

  cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
  cairo_move_to(cr, 2 * OSD_SCALE_FD, OSD_SCALE_H2 - OSD_SCALE_FD);
  cairo_show_text(cr, dist_str);
  cairo_move_to(cr, 2 * OSD_SCALE_FD, OSD_SCALE_H2 + OSD_SCALE_FD + extents.height);
  cairo_show_text(cr, dist_str_imp);

  g_free(dist_str);
  g_free(dist_str_imp);

  /* draw white line */
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
  cairo_set_line_width(cr, OSD_SCALE_FONT_SIZE / 3);
  cairo_move_to(cr, OSD_SCALE_FONT_SIZE / 6, OSD_SCALE_M);
  cairo_rel_line_to(cr, 0, OSD_SCALE_TICK);
  cairo_rel_line_to(cr, width_metric, 0);
  cairo_rel_line_to(cr, 0, -OSD_SCALE_TICK);
  cairo_stroke(cr);
  cairo_move_to(cr, OSD_SCALE_FONT_SIZE / 6, OSD_SCALE_I);
  cairo_rel_line_to(cr, 0, -OSD_SCALE_TICK);
  cairo_rel_line_to(cr, width_imp, 0);
  cairo_rel_line_to(cr, 0, +OSD_SCALE_TICK);
  cairo_stroke(cr);

  /* draw black line */
  cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
  cairo_set_line_width(cr, OSD_SCALE_FONT_SIZE / 6);
  cairo_move_to(cr, OSD_SCALE_FONT_SIZE / 6, OSD_SCALE_M);
  cairo_rel_line_to(cr, 0, OSD_SCALE_TICK);
  cairo_rel_line_to(cr, width_metric, 0);
  cairo_rel_line_to(cr, 0, -OSD_SCALE_TICK);
  cairo_stroke(cr);
  cairo_move_to(cr, OSD_SCALE_FONT_SIZE / 6, OSD_SCALE_I);
  cairo_rel_line_to(cr, 0, -OSD_SCALE_TICK);
  cairo_rel_line_to(cr, width_imp, 0);
  cairo_rel_line_to(cr, 0, +OSD_SCALE_TICK);
  cairo_stroke(cr);

  cairo_destroy(cr);
}

static void onZoom(G_GNUC_UNUSED GObject* gobject, G_GNUC_UNUSED GParamSpec* pspec,
                   gpointer user_data)
{
  osm_gps_map_osd_t* osd = (osm_gps_map_osd_t*)user_data;

  osd_render_scale(osd);
}
#endif

static void osd_render(osm_gps_map_osd_t* osd)
{
  /* this function is actually called pretty often since the */
  /* OSD contents may have changed (due to a coordinate/zoom change). */
  /* The different OSD parts have to make sure that they don't */
  /* render unneccessarily often and thus waste CPU power */

#ifdef OSD_SCALE
  osd_render_scale(osd);
#endif

#ifdef OSD_CROSSHAIR
  osd_render_crosshair(osd);
#endif

#ifdef OSD_COORDINATES
  osd_render_coordinates(osd);
#endif
}

static void osd_draw(osm_gps_map_osd_t* osd, cairo_t* cr)
{
  osd_priv_t* priv = (osd_priv_t*)osd->priv;

#ifdef OSD_SCALE
  if (!priv->scale.surface)
  {
    priv->scale.surface =
        // cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
        //                            OSD_SCALE_W, OSD_SCALE_H);

        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 300, 300);
    priv->scale.zoom = -1;
    priv->scale.factor = 0.f;
    osd_render_scale(osd);
  }

#endif

#ifdef OSD_CROSSHAIR
  if (!priv->crosshair.surface)
  {
    priv->crosshair.surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, OSD_CROSSHAIR_W, OSD_CROSSHAIR_H);
  }

  osd_render_crosshair(osd);
#endif

#ifdef OSD_COORDINATES
  if (!priv->coordinates.surface)
  {
    priv->coordinates.surface =
        cairo_image_surface_create(CAIRO_FORMAT_ARGB32, OSD_COORDINATES_W, OSD_COORDINATES_H);

    priv->coordinates.lat = priv->coordinates.lon = OSM_GPS_MAP_INVALID;
  }
  osd_render_coordinates(osd);

#endif

  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

  // now draw this onto the original context
  gint x, y;
  guint width, height;

  g_object_get(G_OBJECT(osd->map), "viewport-width", &width, "viewport-height", &height, NULL);
  /* g_message("%dx%d--------------------", width, height); */
#ifdef OSD_SCALE
  x = OSD_X;
  y = -OSD_Y;
  if (x < 0)
    x += width - OSD_SCALE_W;
  if (y < 0)
    y += height - OSD_SCALE_H;

  cairo_set_source_surface(cr, priv->scale.surface, x, y);
  cairo_paint(cr);
#endif

#ifdef OSD_CROSSHAIR
  x = (width - OSD_CROSSHAIR_W) / 2;
  y = (height - OSD_CROSSHAIR_H) / 2;

  cairo_set_source_surface(cr, priv->crosshair.surface, x, y);
  cairo_paint(cr);
#endif

#ifdef OSD_COORDINATES
  x = -OSD_X;
  y = -OSD_Y;
  if (x < 0)
    x += width - OSD_COORDINATES_W;
  if (y < 0)
    y += height - OSD_COORDINATES_H;

  cairo_set_source_surface(cr, priv->coordinates.surface, x, y);
  cairo_paint(cr);
#endif
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

static gboolean osd_busy(G_GNUC_UNUSED osm_gps_map_osd_t* osd)
{

  return FALSE;
}

static osd_button_t osd_check(osm_gps_map_osd_t* osd, gboolean down, gint x, gint y)
{
  return osd_check_int(osd, TRUE, down ? OSD_STATE_DOWN : OSD_STATE_UP, x, y);
}

/* this is the only function that's externally visible */
osm_gps_map_osd_t* osm_gps_map_osd_classic_init(OsmGpsMap* map)
{
  osm_gps_map_osd_t* osd_classic = g_new0(osm_gps_map_osd_t, 1);
  osd_priv_t* priv = g_new0(osd_priv_t, 1);

  /* reset entries to default value */
  osd_classic->map = NULL;
  // osd_classic->gps_enabled = FALSE;
  osd_classic->priv = priv;
  priv->scale.compass_azimuth = NAN;
  osd_classic->draw = osd_draw, osd_classic->check = osd_check, osd_classic->render = osd_render,
  osd_classic->free = osd_free, osd_classic->busy = osd_busy,

  osd_classic->map = map;
  g_object_ref(map);
#ifdef OSD_SCALE
  g_signal_connect(G_OBJECT(map), "notify::zoom", G_CALLBACK(onZoom), osd_classic);
  g_signal_connect(G_OBJECT(map), "notify::factor", G_CALLBACK(onZoom), osd_classic);
#endif
#ifdef OSD_COORDINATES
  g_signal_connect(G_OBJECT(map), "notify::latitude", G_CALLBACK(onLatLon), osd_classic);
#endif
  return osd_classic;
}

void osm_gps_map_osd_classic_free(osm_gps_map_osd_t* osd)
{
  osd->free(osd);
  g_object_unref(osd->map);
  g_free(osd);
}

void osm_gps_map_set_azimuth(osm_gps_map_osd_t* osd, double azimuth)
{
  osd_priv_t* priv = (osd_priv_t*)osd->priv;
  if (isnan(azimuth))
  {
    priv->scale.compass_azimuth = NAN;
    return;
  }
  priv->scale.compass_azimuth = deg2rad((float)azimuth);
}

osd_button_t osm_gps_map_osd_check(osm_gps_map_osd_t* osd, gint x, gint y)
{
  g_return_val_if_fail(osd, OSD_NONE);

  return osd_check_int(osd, FALSE, OSD_STATE_CHECK, x, y);
}
