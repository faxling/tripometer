/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/* vim:set et sw=4 ts=4 cino=t0,(0: */
/*
 * osm-gps-map.c
 * Copyright (C) Marcus Bauer 2008 <marcus.bauer@gmail.com>
 * Copyright (C) John Stowers 2009 <john.stowers@gmail.com>
 * Copyright (C) Till Harbaum 2009 <till@harbaum.org>
 *
 * Contributions by
 * Everaldo Canuto 2009 <everaldo.canuto@gmail.com>
 *
 * osm-gps-map.c is free software: you can redistribute it and/or modify it
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

#include "../config.h"
#include "../converter.h"
#include "../img_loader.h"
#include "../net_io.h"

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "glibconfig.h"
#include <cairo.h>
#include <glib/gfileutils.h>
#include <glib/gtypes.h>
// Include generated file to avoid ide warnings
#include "src/misc.h"
// #include <../lib/glib-2.0/include/tglibconfig.h>

#include "osm-gps-map-types.h"
#include "osm-gps-map.h"

#define ENABLE_DEBUG (0)

extern int g_nSkipDraw;

struct _OsmGpsMapPrivate
{
  int drag_mouse_dx;
  int drag_mouse_dy;

  GHashTable* tile_queue;
  GHashTable* missing_tiles;
  GHashTable* tile_cache;

  guint viewport_width;
  guint viewport_height;

  /* Dirty region. */
  cairo_region_t* dirty;

  gfloat map_factor;
  int map_depth;

  gfloat windDirectionRad;
  gfloat windSpeedMs;
  gfloat tempDeg;
  int map_zoom;
  int max_zoom;
  int min_zoom;
  gboolean map_auto_center;
  //gboolean map_auto_download;
  int map_x;
  int map_y;

  /* Latitude and longitude of the center of the map, in radians */
  gfloat center_rlat;
  gfloat center_rlon;

  guint max_tile_cache_size;
  /* Incremented at each redraw */
  guint redraw_cycle;
  /* ID of the idle redraw operation */
  gulong idle_map_redraw;
  char* proxy_uri;

  // where downloaded tiles are cached
  char* tile_dir;
  char* tile_base_dir;
  char* cache_dir;

  // contains flags indicating the various special characters
  // the uri string contains, that will be replaced when calculating
  // the uri to download.
  OsmGpsMapSource_t map_source;
  const char* repo_const_uri;
  char* image_format;
  int uri_format;
  gboolean the_navionics;

  // gps tracking state
  //gboolean record_trip_history;
  //gboolean show_trip_history;
  //MaepGeodata* trip_history;
  coord_t* osm_gps;
  float osm_gps_heading;
  gboolean osm_gps_valid;
  cairo_surface_t* cr_markedImage;
  // additional images or tracks added to the map
  GSList* tracks;
  GSList* images;

  // Used for storing the joined tiles
  cairo_surface_t* map_surf;
  cairo_t* cr;

  // The tile painted when one cannot be found
  cairo_surface_t* null_tile;

  // A list of OsmGpsMapLayer* layers, such as the OSD
  //  GSList* layers;

  // for customizing the redering of the gps track
  int ui_gps_track_width;
  int ui_gps_point_inner_radius;
  int ui_gps_point_outer_radius;

  guint fullscreen : 1;
  guint is_disposed : 1;
  //   guint double_pixel : 1;
};

#define OSM_GPS_MAP_PRIVATE(o) (OSM_GPS_MAP(o)->priv)

typedef struct
{
  cairo_surface_t* cr_surf;
  /* We keep track of the number of the redraw cycle this tile was last used,
   * so that osm_gps_map_purge_cache() can remove the older ones */
  guint redraw_cycle;
} OsmCachedTile;

typedef struct
{
  MaepGeodata* track;
  gulong dirty_sig, nwp_prop, iwp_prop;
  int m_nId;

  // -1 Distance Tool, 0 current recorded tarck, 1 trip history , 2 Loaded track
  int m_nTrackType;
} OsmTrackRef;

enum
{
  PROP_0,
  PROP_AUTO_CENTER,
  //PROP_RECORD_TRIP_HISTORY,
  //PROP_SHOW_TRIP_HISTORY,
  //PROP_AUTO_DOWNLOAD,
  // PROP_REPO_URI,
  // PROP_PROXY_URI,
  PROP_TILE_CACHE_DIR,
  PROP_TILE_CACHE_BASE_DIR,
  PROP_TILE_CACHE_DIR_IS_FULL_PATH,
  PROP_ZOOM,
 // PROP_MAX_ZOOM,
 //  PROP_MIN_ZOOM,
  PROP_FACTOR,
  PROP_LATITUDE,
  PROP_LONGITUDE,
  PROP_MAP_X,
  PROP_MAP_Y,
  PROP_TILES_QUEUED,
  PROP_GPS_TRACK_WIDTH,
  PROP_GPS_POINT_R1,
  PROP_GPS_POINT_R2,
  PROP_MAP_SOURCE,
  PROP_IMAGE_FORMAT,
  PROP_VIEWPORT_WIDTH,
  PROP_VIEWPORT_HEIGHT,

  PROP_LAST
};

static GParamSpec* properties[PROP_LAST];

G_DEFINE_TYPE(OsmGpsMap, osm_gps_map, G_TYPE_OBJECT)

static gchar* replace_string(const gchar* src, const gchar* from, const gchar* to);

static void inspect_map_uri(OsmGpsMapPrivate* priv);

static void osm_gps_map_print_images(OsmGpsMap* map);

static void osm_gps_map_download_tile2(OsmGpsMap* map, int zoom, int x, int y, gboolean redraw);

static void osm_gps_map_load_tile(OsmGpsMap* map, int zoom, int x, int y, int offset_x,
                                  int offset_y, cairo_t*);
static void osm_gps_map_fill_tiles_pixel(OsmGpsMap* map);

static void cached_tile_free(OsmCachedTile* tile)
{
  /* g_message("destroying cached tile."); */
  cairo_surface_destroy(tile->cr_surf);
  g_slice_free(OsmCachedTile, tile);
}

static void track_ref_free(OsmTrackRef* st)
{
  g_signal_handler_disconnect(st->track, st->dirty_sig);
  g_signal_handler_disconnect(st->track, st->nwp_prop);
  g_signal_handler_disconnect(st->track, st->iwp_prop);
  g_object_unref(st->track);
  g_slice_free(OsmTrackRef, st);
}

/*
 * Description:
 *   Find and replace text within a string.
 *
 * Parameters:
 *   src  (in) - pointer to source string
 *   from (in) - pointer to search text
 *   to   (in) - pointer to replacement text
 *
 * Returns:
 *   Returns a pointer to dynamically-allocated memory containing string
 *   with occurences of the text pointed to by 'from' replaced by with the
 *   text pointed to by 'to'.
 */
static gchar* replace_string(const gchar* src, const gchar* from, const gchar* to)
{
  size_t size = strlen(src) + 1;
  size_t fromlen = strlen(from);
  size_t tolen = strlen(to);

  /* Allocate the first chunk with enough for the original string. */
  gchar* value = g_malloc(size);

  /* We need to return 'value', so let's make a copy to mess around with. */
  gchar* dst = value;

  if (value != NULL)
  {
    for (;;)
    {
      /* Try to find the search text. */
      const gchar* match = g_strstr_len(src, size, from);
      if (match != NULL)
      {
        gchar* temp;
        size_t count = match - src;

        size += tolen - fromlen;

        temp = g_realloc(value, size);
        if (temp == NULL)
        {
          g_free(value);
          return NULL;
        }

        dst = temp + (dst - value);
        value = temp;

        memmove(dst, src, count);
        src += count;
        dst += count;

        memmove(dst, to, tolen);
        src += fromlen;
        dst += tolen;
      }
      else
      {

        strcpy(dst, src);
        break;
      }
    }
  }
  return value;
}

static void map_convert_coords_to_quadtree_string(gint x, gint y, gint zoomlevel, gchar* buffer,
                                                  const gchar initial, const gchar* const quadrant)
{
  gchar* ptr = buffer;
  gint n;

  if (initial)
    *ptr++ = initial;

  for (n = zoomlevel - 1; n >= 0; n--)
  {
    gint xbit = (x >> n) & 1;
    gint ybit = (y >> n) & 1;
    *ptr++ = quadrant[xbit + 2 * ybit];
  }

  *ptr++ = '\0';
}

void inspect_map_uri(OsmGpsMapPrivate* priv)
{
  int uri_format;

  if (priv->map_source == OSM_GPS_MAP_SOURCE_NAVIONICS_2 ||
      priv->map_source == OSM_GPS_MAP_SOURCE_NAVIONICS)
  {
    priv->the_navionics = 1;
    priv->uri_format = URI_HAS_X | URI_HAS_Y | URI_HAS_Z;
    return;
  }
  priv->the_navionics = 0;
  const gchar* repo_uri = priv->repo_const_uri;
  uri_format = 0;

  if (g_strrstr(repo_uri, URI_MARKER_X))
    uri_format |= URI_HAS_X;

  if (g_strrstr(repo_uri, URI_MARKER_Y))
    uri_format |= URI_HAS_Y;

  if (g_strrstr(repo_uri, URI_MARKER_Z))
    uri_format |= URI_HAS_Z;

  if (g_strrstr(repo_uri, URI_MARKER_S))
    uri_format |= URI_HAS_S;

  if (g_strrstr(repo_uri, URI_MARKER_Q))
    uri_format |= URI_HAS_Q;

  if (g_strrstr(repo_uri, URI_MARKER_Q0))
    uri_format |= URI_HAS_Q0;

  if (g_strrstr(repo_uri, URI_MARKER_YS))
    uri_format |= URI_HAS_YS;

  if (g_strrstr(repo_uri, URI_MARKER_R))
    uri_format |= URI_HAS_R;

  priv->uri_format = uri_format;
}

static gchar* get_tile_uri(const gchar* uri, int uri_format, int max_zoom, int zoom, int x, int y)
{
  char* url;
  if (uri == 0)
    return 0;

  if (uri[0] == 0)
    return 0;

  unsigned int i;
  char location[22];

  i = 1;
  url = g_strdup(uri);
  while (i < URI_FLAG_END)
  {
    char* s = NULL;
    char* old;

    old = url;
    switch (i & uri_format)
    {
    case URI_HAS_X:
      s = g_strdup_printf("%d", x);
      url = replace_string(url, URI_MARKER_X, s);
      break;
    case URI_HAS_Y:
      s = g_strdup_printf("%d", y);
      url = replace_string(url, URI_MARKER_Y, s);
      break;
    case URI_HAS_Z:
      s = g_strdup_printf("%d", zoom);
      url = replace_string(url, URI_MARKER_Z, s);
      break;
    case URI_HAS_S:
      s = g_strdup_printf("%d", max_zoom - zoom);
      url = replace_string(url, URI_MARKER_S, s);
      break;
    case URI_HAS_Q:
      map_convert_coords_to_quadtree_string(x, y, zoom, location, 't', "qrts");
      s = g_strdup_printf("%s", location);
      url = replace_string(url, URI_MARKER_Q, s);
      break;
    case URI_HAS_Q0:
      map_convert_coords_to_quadtree_string(x, y, zoom, location, '\0', "0123");
      s = g_strdup_printf("%s", location);
      url = replace_string(url, URI_MARKER_Q0, s);
      break;
    case URI_HAS_YS:
      s = g_strdup_printf("%d", (1 << (zoom)) - y - 1);
      url = replace_string(url, URI_MARKER_YS, s);
      break;
    case URI_HAS_R:
      s = g_strdup_printf("%d", g_random_int_range(0, 4));
      url = replace_string(url, URI_MARKER_R, s);
      // g_debug("FOUND " URI_MARKER_R);
      break;
    default:
      s = NULL;
      break;
    }

    if (s)
    {
      g_free(s);
      g_free(old);
    }

    i = (i << 1);
  }

  return url;
}

static void my_log_handler(const gchar* log_domain, GLogLevelFlags log_level, const gchar* message,
                           gpointer user_data)
{
  if (!(log_level & G_LOG_LEVEL_DEBUG) || ENABLE_DEBUG)
    g_log_default_handler(log_domain, log_level, message, user_data);
}

static float osm_gps_map_get_scale_at_lat(int zoom, gfloat factor, float rlat)
{
  /* world at zoom 1 == 512 pixels */
  return cos(rlat) * M_PI * OSM_EQ_RADIUS / (1 << (7 + zoom)) / factor;
}



static float get_distance(float lat1, float lon1, float lat2, float lon2)
{
  float aob =
      acos(CLAMP(cos(lat1) * cos(lat2) * cos(lon2 - lon1) + sin(lat1) * sin(lat2), -1.f, +1.f));

  return (aob * 6371000.0); /* great circle radius in meters */
}

float osm_db_last_dist(OsmGpsMap* map, float la, float lo)
{
  OsmGpsMapPrivate* priv = map->priv;
  if (priv->tracks)
  {
    GSList* tmp = priv->tracks;
    while (tmp != NULL)
    {
      OsmTrackRef* pNode = (OsmTrackRef*)tmp->data;
      if (pNode->m_nId == -1)
      {
        coord_t tLast = maep_geodata_track_get_lastpoint(pNode->track);
        if (tLast.rlat == 0)
          return 0;
        float dist = get_distance(deg2rad(la), deg2rad(lo), tLast.rlat, tLast.rlon);

        dist += maep_geodata_track_get_metric_length(pNode->track);

        return dist;
      }
      tmp = g_slist_next(tmp);
    }
  }
  return 0;
}

/* clears the tracks and all resources */
static void osm_gps_map_free_tracks(OsmGpsMap* map)
{

  OsmGpsMapPrivate* priv = map->priv;
  if (priv->tracks)
  {
    GSList* tmp = priv->tracks;
    while (tmp != NULL)
    {
      OsmTrackRef* pNode = (OsmTrackRef*)tmp->data;
      if (pNode->m_nTrackType == 0)
      {
        priv->tracks = g_slist_remove_link(priv->tracks, tmp);
        track_ref_free((OsmTrackRef*)tmp->data);
        return;
      }
      tmp = g_slist_next(tmp);
    }
  }
}

void osm_gps_map_free_track(OsmGpsMap* map, int nId)
{
  OsmGpsMapPrivate* priv = map->priv;
  if (priv->tracks)
  {
    GSList* tmp = priv->tracks;
    while (tmp != NULL)
    {
      OsmTrackRef* pNode = (OsmTrackRef*)tmp->data;
      if (pNode->m_nId == nId)
      {
        priv->tracks = g_slist_remove_link(priv->tracks, tmp);
        track_ref_free((OsmTrackRef*)tmp->data);
        return;
      }
      tmp = g_slist_next(tmp);
    }
  }
}

/* free the poi image lists */
static void osm_gps_map_free_images(OsmGpsMap* map)
{
  OsmGpsMapPrivate* priv = map->priv;
  if (priv->images)
  {
    GSList* list;
    for (list = priv->images; list != NULL; list = list->next)
    {
      image_t* im = list->data;
      cairo_surface_destroy(im->image);
      g_free(im->sz);
      g_free(im);
    }
    g_slist_free(priv->images);
    priv->images = NULL;
  }
}
/*
static void osm_gps_map_free_layers(OsmGpsMap* map)
{
  OsmGpsMapPrivate* priv = map->priv;
   g_message("g_object_list unref enter");
  if (priv->layers)
  {
    //   g_slist_foreach(priv->layers, (GFunc)g_object_unref, NULL);
    GSList* list;
    for (list = priv->layers; list; list = list->next)
    {
      g_message("g_object_unref");
      g_object_unref(list);
    }
 g_message("g_object_list unref");
    g_slist_free(priv->layers);
    priv->layers = NULL;
  }
}
*/
/*
void osm_gps_map_add_layer(OsmGpsMap* map, OsmGpsMapLayer* layer)
{
  OsmGpsMapPrivate* priv;
  GSList* list;

  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;

  for (list = priv->layers; list; list = list->next)
    if (list->data == layer)
      return;
  g_object_ref(layer);
  priv->layers = g_slist_prepend(priv->layers, layer);
}

void osm_gps_map_layer_changed(OsmGpsMap* map, G_GNUC_UNUSED OsmGpsMapLayer* layer)
{
  OsmGpsMapPrivate* priv;

  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;
  if (!priv->idle_map_redraw)
    priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
}
void osm_gps_map_remove_layer(OsmGpsMap* map, OsmGpsMapLayer* layer)
{
  OsmGpsMapPrivate* priv;
  GSList* list;

  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;

  for (list = priv->layers; list; list = list->next)
    if (list->data == layer)
      break;
  if (!list)
    return;

  priv->layers = g_slist_remove(priv->layers, layer);
  g_object_unref(layer);

  if (!priv->idle_map_redraw)
    priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
}
*/

void move_to(cairo_t* cr, cairo_matrix_t* m, double x, double y)
{
  double x_new = m->xx * x + m->xy * y + m->x0;
  double y_new = m->yx * x + m->yy * y + m->y0;
  cairo_move_to(cr, x_new, y_new);
}

void line_to(cairo_t* cr, cairo_matrix_t* m, double x, double y)
{
  double x_new = m->xx * x + m->xy * y + m->x0;
  double y_new = m->yx * x + m->yy * y + m->y0;
  cairo_line_to(cr, x_new, y_new);
}

void setAisStyle(OsmGpsMap* map, unsigned int nRGB, double fWidth)
{
  cairo_t* cr = map->priv->cr;
  if (cr == 0)
    return;
  cairo_set_line_width(cr, fWidth);
  static double rgb[3];
  rgbToCario(nRGB, rgb);
  cairo_set_source_rgb(cr, rgb[0], rgb[1], rgb[2]);
}

void setAisStyle2(OsmGpsMap* map, double* rgb, double fWidth)
{
  cairo_t* cr = map->priv->cr;
  if (cr == 0)
    return;
  cairo_set_line_width(cr, fWidth);
  cairo_set_source_rgb(cr, rgb[0], rgb[1], rgb[2]);
}

void saveDraw(OsmGpsMap* map)
{
  cairo_t* cr = map->priv->cr;
  cairo_save(cr);
  cairo_set_dash(cr, 0, 0, 0);
}

void restoreDraw(OsmGpsMap* map)
{
  cairo_t* cr = map->priv->cr;
  cairo_restore(cr);
}

void drawAisMoored(OsmGpsMap* map, double x0, double y0, const char* szName)
{
  cairo_t* cr = map->priv->cr;
  cairo_matrix_t mat;
  cairo_matrix_init_translate(&mat, x0, y0);
  move_to(cr, &mat, 0, 7);
  line_to(cr, &mat, 7, 0);
  line_to(cr, &mat, 0, -7);
  line_to(cr, &mat, -7, 0);
  line_to(cr, &mat, 0, 7);
  cairo_stroke(cr);
  cairo_move_to(cr, x0, y0);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_show_text(cr, szName);
}

void drawAis(OsmGpsMap* map, float v, double speed, double x0, double y0, const char* szName)
{
  //  cairo_save(cr);
  cairo_t* cr = map->priv->cr;
  cairo_matrix_t mat;
  cairo_matrix_init_translate(&mat, x0, y0);
  cairo_matrix_rotate(&mat, deg2rad(v));

  move_to(cr, &mat, 0, 10);
  line_to(cr, &mat, 0, 10 + speed);
  move_to(cr, &mat, 0, 10);
  line_to(cr, &mat, 5, 0);
  line_to(cr, &mat, 5, -20);
  line_to(cr, &mat, 0, -10);
  line_to(cr, &mat, -5, -20);
  line_to(cr, &mat, -5, 0);
  line_to(cr, &mat, 0, 10);

  cairo_stroke(cr);

  cairo_move_to(cr, x0, y0);

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_show_text(cr, szName);


}

int loLaToPx(OsmGpsMap* map, float lo, float la, int* x, int* y)
{

  OsmGpsMapPrivate* priv = map->priv;
  // int x, y;

  int pixel_x, pixel_y;
  pixel_x = lon2pixel(priv->map_zoom, deg2rad(lo));
  pixel_y = lat2pixel(priv->map_zoom, deg2rad(la));
  int map_x0, map_y0;
  map_x0 = priv->map_x - 0.25 * priv->viewport_width - EXTRA_BORDER;
  map_y0 = priv->map_y - 0.25 * priv->viewport_height - EXTRA_BORDER;
  *x = pixel_x - map_x0;
  *y = pixel_y - map_y0;
  if (*x < 0 || (*x > (priv->viewport_width * 1.5)))
    return 0;

  if (*y < 0 || (*y > (priv->viewport_height * 1.5)))
    return 0;

  return 1;
}


static void osm_gps_map_print_images(OsmGpsMap* map)
{
  GSList* list;
  int x, y, pixel_x, pixel_y;
  int min_x = 0, min_y = 0, max_x = 0, max_y = 0;
  int map_x0, map_y0;
  cairo_rectangle_int_t rect;
  OsmGpsMapPrivate* priv = map->priv;

  map_x0 = priv->map_x - 0.25 * priv->viewport_width - EXTRA_BORDER;
  map_y0 = priv->map_y - 0.25 * priv->viewport_height - EXTRA_BORDER;

  cairo_set_source_rgb(priv->cr, 0.9, 0.2, 0.2);
  cairo_select_font_face(priv->cr, "Purisa", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);

  cairo_set_font_size(priv->cr, 10);

  for (list = priv->images; list != NULL; list = list->next)
  {
    image_t* im = list->data;

    // pixel_x,y, offsets
    pixel_x = lon2pixel(priv->map_zoom, im->pt.rlon);
    pixel_y = lat2pixel(priv->map_zoom, im->pt.rlat);

    x = pixel_x - map_x0;
    y = pixel_y - map_y0;

    if (im->sz != 0)
    {
      cairo_move_to(priv->cr, x + 10, y);
      cairo_show_text(priv->cr, im->sz);
    }
  }

  for (list = priv->images; list != NULL; list = list->next)
  {
    image_t* im = list->data;

    // pixel_x,y, offsets
    pixel_x = lon2pixel(priv->map_zoom, im->pt.rlon);
    pixel_y = lat2pixel(priv->map_zoom, im->pt.rlat);

    x = pixel_x - map_x0;
    y = pixel_y - map_y0;

    cairo_set_source_surface(priv->cr, im->image, x - im->xoffset, y - im->yoffset);
    cairo_paint(priv->cr);

    max_x = MAX(x + im->w, max_x);
    min_x = MIN(x - im->w, min_x);
    max_y = MAX(y + im->h, max_y);
    min_y = MIN(y - im->h, min_y);

    // The pike with square as marker
    if (im->image == priv->cr_markedImage)
    {
      int nM = 3;
      x = x - im->xoffset - nM;
      y = y - im->yoffset - nM;
      double fStr[1] = {4};
      cairo_set_dash(priv->cr, fStr, 1, 0);
      cairo_move_to(priv->cr, x, y);
      static double rgb[3];
      if (rgb[0] == 0)
        rgbToCario(0xcf683f, rgb);
      cairo_set_source_rgb(priv->cr, rgb[0], rgb[1], rgb[2]);
      cairo_set_line_width(priv->cr, 3);
      cairo_line_to(priv->cr, x + im->w + nM, y);
      cairo_line_to(priv->cr, x + im->w + nM, y + im->h + nM);
      cairo_line_to(priv->cr, x, y + im->h + nM);
      cairo_close_path(priv->cr);
      cairo_stroke(priv->cr);
    }
  }
  rect.x = min_x + EXTRA_BORDER;
  rect.y = min_y + EXTRA_BORDER;
  rect.width = max_x - min_x;
  rect.height = max_y - min_y;

  cairo_region_union_rectangle(priv->dirty, &rect);
}

static void osm_gps_map_blit_surface(cairo_t* cr, cairo_surface_t* cr_surf, int offset_x,
                                     int offset_y, int modulo, int area_x, int area_y)
{
  cairo_rectangle(cr, offset_x, offset_y, TILESIZE, TILESIZE);
  cairo_save(cr);
  cairo_translate(cr, offset_x - area_x * modulo, offset_y - area_y * modulo);
  cairo_scale(cr, modulo, modulo);
  cairo_set_source_surface(cr, cr_surf, 0, 0);
  cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);
  cairo_fill(cr);
  cairo_restore(cr);
}

static cairo_surface_t* osm_gps_map_from_file(const char* filename, const char* ext)
{
  cairo_surface_t* surf;
  GError* error;

  if (!strcmp(ext, "png"))
  {
    surf = cairo_image_surface_create_from_png(filename);
  }
  else
  {
    error = NULL;
    surf = maep_loader_jpeg_from_file(filename, &error);
    if (error)
    {
      g_warning("%s", error->message);
      g_error_free(error);
    }
  }

  return surf;
}

#define UNUSED(x) (void)(x)
#define MAXTOKEN 1024

char g_szNAVTOKEN_A[MAXTOKEN] = {0};
char g_szNAVTOKEN_C[MAXTOKEN] = {0};
char g_szNAVURL1[MAXTOKEN * 2] = {0};
char g_szNAVURL2[MAXTOKEN * 2] = {0};
char g_szAUTH[MAXTOKEN * 2] = {0};

/*
 *
#define NAVURL1                                                                                    \
  "https://tile3.navionics.com/tile/#Z/#X/"                                                        \
  "#Y?LAYERS=config_1_20.00_0&TRANSPARENT=FALSE&UGC=TRUE&theme=0&navtoken=%s"

#define NAVURL2                                                                                    \
  "https://tile3.navionics.com/tile/#Z/#X/"                                                        \
  "#Y?LAYERS=config_1_20.00_1&TRANSPARENT=FALSE&UGC=TRUE&theme=0&navtoken=%s"


*/

#define NAVURL1 "https://tile1.navionics.com/viewer/api/v1/tile/#Z/#X/#Y?config=%s&transparent=false&ugc=false&layer=0&du=1&sd=2&sa=false"

#define NAVURL2 "https://tile1.navionics.com/viewer/api/v1/tile/#Z/#X/#Y?config=%s&transparent=false&ugc=false&layer=1&du=1&sd=20&sa=false"

extern void parse_navionics_key(const char* pResponce, int nLen, char* a_pToken, char* c_pToken);

static void navionics_request_cb(net_result_t* result, gpointer p)
{
  UNUSED(p);

  g_message("navionics_request_cb %d", result->code);

  if (result->code != 0)
    return;

  if (result->data.len > MAXTOKEN)
    return;


  parse_navionics_key(result->data.ptr, result->data.len, g_szNAVTOKEN_A, g_szNAVTOKEN_C);

  sprintf(g_szAUTH, "authorization: Bearer %s", g_szNAVTOKEN_A);

  sprintf(g_szNAVURL1, NAVURL1, g_szNAVTOKEN_C);
  sprintf(g_szNAVURL2, NAVURL2, g_szNAVTOKEN_C);

}

void get_navionics_key2()
{
  if (g_szNAVURL1[0] != 0)
    return;

  //   strcpy(g_szNAVTOKEN,
  //   "eyJrZXkiOiJOYXZpb25pY3Nfd2ViYXBpXzA0MDQxIiwia2V5RG9tYWluIjoibWFwcy5nYXJtaW4uY29tIiwicmVmZXJlciI6Im1hcHMuZ2FybWluLmNvbSIsInJhbmRvbSI6MTcyMzAwOTE4Mzg4N30");

  struct curl_slist* chunk = 0;

  net_io_append_header(&chunk, "referer: https://maps.garmin.com/");

  net_io_append_header(&chunk,
                       "user-agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
                       "(KHTML, like Gecko) Chrome/136.0.0.0 Safari/537.36 Edg/136.0.0.0");



  net_io_download_async("https://maps.garmin.com/marine/api/getNavionicsTokens",
                        navionics_request_cb, 0, chunk);

  /*
  net_io_download_async("https://tile3.navionics.com/tile/get_key/Navionics_webapi_04041/"
                        "maps.garmin.com?_=1690792122212",
                        navionics_request_cb, 0, chunk);
*/
}

struct curl_slist* chunk = 0;

// Run in main thread
void curl_cb(net_result_t* result, gpointer data)
{
  tile_download_t* dl = (tile_download_t*)data;

  FILE* file;
  OsmGpsMap* map = OSM_GPS_MAP(dl->map);
  OsmGpsMapPrivate* priv = map->priv;
  cairo_surface_t* cr_surf;

  if (result->code == 0)
  {

    /* save tile into cachedir if one has been specified */
    if (priv->cache_dir)
    {
      if (g_mkdir_with_parents(dl->folder, 0700) == 0)
      {
        file = fopen(dl->filename, "wb");
        if (file != NULL)
        {
          fwrite(result->data.ptr, 1, result->data.len, file);
          fclose(file);
        }
      }
    }

    if (dl->redraw)
    {
      cr_surf = osm_gps_map_from_file(dl->filename, priv->image_format);
      if (cr_surf && cairo_surface_status(cr_surf) == CAIRO_STATUS_SUCCESS)
      {
        OsmCachedTile* tile = g_slice_new(OsmCachedTile);
        tile->cr_surf = cr_surf;
        tile->redraw_cycle = priv->redraw_cycle;
        g_hash_table_insert(priv->tile_cache, dl->filename, tile);
        dl->filename = NULL;
      }
      if (!priv->idle_map_redraw)
        priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
    }
  }
  else
  {
    if (result->respCode == 404)
    {
      g_hash_table_insert(priv->missing_tiles, dl->uri, NULL);
    }
  }
  g_hash_table_remove(priv->tile_queue, dl->uri);
  g_free(dl->uri);
  g_free(dl->folder);
  g_free(dl->filename);
  g_free(dl);
}

static void osm_gps_map_download_tile2(OsmGpsMap* map, int zoom, int x, int y, gboolean redraw)
{
  OsmGpsMapPrivate* priv = map->priv;

  gchar* szUrl = get_tile_uri(priv->repo_const_uri, priv->uri_format, priv->max_zoom, zoom, x, y);

  if (szUrl == 0)
    return;

  tile_download_t* dl = g_new0(tile_download_t, 1);

  dl->uri = szUrl;

  if (g_hash_table_lookup_extended(priv->tile_queue, dl->uri, NULL, NULL) ||
      g_hash_table_lookup_extended(priv->missing_tiles, dl->uri, NULL, NULL))
  {
    // g_message("Tile already downloading (or missing)");
    g_free(dl->uri);
    g_free(dl);
    return;
  }
  else
  {

    dl->folder = g_strdup_printf("%s%c%d%c%d%c", priv->cache_dir, G_DIR_SEPARATOR, zoom,
                                 G_DIR_SEPARATOR, x, G_DIR_SEPARATOR);
    dl->filename = g_strdup_printf("%s%c%d%c%d%c%d.%s", priv->cache_dir, G_DIR_SEPARATOR, zoom,
                                   G_DIR_SEPARATOR, x, G_DIR_SEPARATOR, y, priv->image_format);
    dl->map = map;
    dl->redraw = redraw;
    g_hash_table_insert(priv->tile_queue, dl->uri, NULL);
  }

  struct curl_slist* chunk = 0;
  if (priv->the_navionics)
  {
    net_io_append_header(&chunk, "referer: https://maps.garmin.com/");
    net_io_append_header(&chunk, g_szAUTH);
    net_io_append_header(
        &chunk, "authority: accept: image/webp,image/apng,image/svg+xml,image/*,*/*;q=0.8");
    net_io_append_header(&chunk, "accept-language: en-GB,en;q=0.9,en-US;q=0.8,sv;q=0.7");
    net_io_append_header(&chunk, "origin: https://maps.garmin.com");
    net_io_append_header(&chunk, "sec-ch-ua: \"Not/A)Brand\";v=\"99\", \"Microsoft "
                                 "Edge\";v=\"115\", \"Chromium\";v=\"115\"");

    net_io_append_header(&chunk, "ec-ch-ua-mobile: ?0");

    net_io_append_header(&chunk, "sec-ch-ua-platform: \"Windows\"");
    net_io_append_header(&chunk, "sec-fetch-dest: image");
    net_io_append_header(&chunk, "sec-fetch-mode: cors");
    net_io_append_header(&chunk, "sec-fetch-site: same-site");
  }
  net_io_download_async(dl->uri, curl_cb, dl, chunk);
}

gchar* get_cached_file(const gchar* cache_dir, const gchar* format, int zoom, int x, int y)
{
  gchar* filename;

  filename = g_strdup_printf("%s%c%d%c%d%c%d.%s", cache_dir, G_DIR_SEPARATOR, zoom, G_DIR_SEPARATOR,
                             x, G_DIR_SEPARATOR, y, format);
  if (g_file_test(filename, G_FILE_TEST_EXISTS))
    return filename;
  else
  {
    g_free(filename);
    return NULL;
  }
}

gchar* osm_gps_map_source_get_cached_file(OsmGpsMapSource_t source, const gchar* cache_dir,
                                          int zoom, int x, int y)
{
  return get_cached_file(cache_dir, osm_gps_map_source_get_image_format(source), zoom, x, y);
}

static OsmCachedTile* osm_gps_map_load_cached_tile(OsmGpsMap* map, const gchar* filename)
{
  OsmGpsMapPrivate* priv = map->priv;
  OsmCachedTile* tile;
  cairo_surface_t* cr_surf;
  gchar* key;

  tile = g_hash_table_lookup(priv->tile_cache, filename);
  if (!tile)
  {
    cr_surf = osm_gps_map_from_file(filename, priv->image_format);
    if (cr_surf && cairo_surface_status(cr_surf) == CAIRO_STATUS_SUCCESS)
    {
      tile = g_slice_new(OsmCachedTile);
      tile->cr_surf = cr_surf;
      key = g_strdup(filename);
      g_hash_table_insert(priv->tile_cache, key, tile);
      /* g_message("caching %s %p.", filename, (gpointer)cr_surf); */
    }
  }

  /* set/update the redraw_cycle timestamp on the tile */
  if (tile)
  {
    tile->redraw_cycle = priv->redraw_cycle;
  }

  return tile;
}

static OsmCachedTile* osm_gps_map_find_bigger_tile(OsmGpsMap* map, int zoom, int x, int y,
                                                   int* zoom_found)
{
  OsmCachedTile* tile;
  gchar* filename;
  int next_zoom, next_x, next_y;

  if (zoom == 0)
    return NULL;
  next_zoom = zoom - 1;
  next_x = x / 2;
  next_y = y / 2;

  filename =
      get_cached_file(map->priv->cache_dir, map->priv->image_format, next_zoom, next_x, next_y);
  if (!filename)
    return osm_gps_map_find_bigger_tile(map, next_zoom, next_x, next_y, zoom_found);

  tile = osm_gps_map_load_cached_tile(map, filename);
  g_free(filename);
  if (tile)
    *zoom_found = next_zoom;
  else
    tile = osm_gps_map_find_bigger_tile(map, next_zoom, next_x, next_y, zoom_found);
  return tile;
}

static OsmCachedTile* osm_gps_map_render_missing_tile_upscaled(OsmGpsMap* map, int zoom, int x,
                                                               int y, int* modulo, int* area_x,
                                                               int* area_y)
{
  OsmCachedTile* big;
  int zoom_big, zoom_diff, area_size;

  big = osm_gps_map_find_bigger_tile(map, zoom, x, y, &zoom_big);
  if (!big)
    return NULL;

  g_debug("Found bigger tile (zoom = %d, wanted = %d)", zoom_big, zoom);

  /* get a Pixbuf for the area to magnify */
  zoom_diff = zoom - zoom_big;
  area_size = TILESIZE >> zoom_diff;
  *modulo = 1 << zoom_diff;
  *area_x = (x % (*modulo)) * area_size;
  *area_y = (y % (*modulo)) * area_size;

  return big;
}

static OsmCachedTile* osm_gps_map_render_missing_tile(OsmGpsMap* map, int zoom, int x, int y,
                                                      int* modulo, int* area_x, int* area_y)
{
  /* maybe TODO: render from downscaled tiles, if the following fails */
  /* g_message("look for upscaled at %dx%d.", x, y); */
  return osm_gps_map_render_missing_tile_upscaled(map, zoom, x, y, modulo, area_x, area_y);
}

static void osm_gps_map_load_tile(OsmGpsMap* map, int zoom, int x, int y, int offset_x,
                                  int offset_y, cairo_t* cairohandle)
{
  OsmGpsMapPrivate* priv = map->priv;
  gchar* filename;
  OsmCachedTile* tile = NULL;
  int modulo, area_x, area_y;
  if (priv->map_source == OSM_GPS_MAP_SOURCE_NULL)
  {
    osm_gps_map_blit_surface(cairohandle, priv->null_tile, offset_x, offset_y, 1, 0, 0);
    return;
  }

  filename = get_cached_file(priv->cache_dir, priv->image_format, zoom, x, y);
  if (filename)
  {
    tile = osm_gps_map_load_cached_tile(map, filename);
    g_free(filename);
  }

  if (tile)
    osm_gps_map_blit_surface(cairohandle, tile->cr_surf, offset_x, offset_y, 1, 0, 0);

  if (!tile)
  {
    osm_gps_map_download_tile2(map, zoom, x, y, TRUE);
    /* try to render the tile by scaling cached tiles from other zoom
     * levels */

    tile = osm_gps_map_render_missing_tile(map, zoom, x, y, &modulo, &area_x, &area_y);
    if (tile)
    {

      osm_gps_map_blit_surface(cairohandle, tile->cr_surf, offset_x, offset_y, modulo, area_x,
                               area_y);
    }
  }
}

void osm_map_fill_tiles_surface(OsmGpsMap* map, cairo_surface_t* surf, cairo_t* cairoHandle)
{
  OsmGpsMapPrivate* priv = map->priv;
  int i, j, tile_x0, tile_y0, tiles_nx, tiles_ny, fmap_x, fmap_y;
  int offset_xn = 0;
  int offset_yn = 0;
  int offset_x;
  int offset_y;
  int tilesize, zoom;
  tilesize = TILESIZE;
  int h = cairo_image_surface_get_height(surf);
  int w = cairo_image_surface_get_width(surf);
  int wv = priv->viewport_width;
  int hv = priv->viewport_height;
  int dx = w / 2 - wv / 2;
  int dy = h / 2 - hv / 2;

  fmap_x = priv->map_x - dx;
  fmap_y = priv->map_y - dy;

  zoom = priv->map_zoom;

  offset_x = -fmap_x % tilesize;
  offset_y = -fmap_y % tilesize;
  if (offset_x > 0)
    offset_x -= tilesize;
  if (offset_y > 0)
    offset_y -= tilesize;

  offset_xn = offset_x;
  offset_yn = offset_y;

  tiles_nx = (w / priv->map_factor - offset_x) / tilesize + 1;
  tiles_ny = (h / priv->map_factor - offset_y) / tilesize + 1;

  tile_x0 = floor((float)fmap_x / (float)tilesize);
  tile_y0 = floor((float)fmap_y / (float)tilesize);

  int nRows = (tile_y0 + tiles_ny);

  int offset_yn_top = offset_yn;
  offset_yn = offset_yn_top;
  for (i = tile_x0; i < (tile_x0 + tiles_nx); i++)
  {
    for (j = tile_y0; j < nRows; j++)
    {
      osm_gps_map_load_tile(map, zoom, i, j, offset_xn, offset_yn, cairoHandle);
      offset_yn += tilesize;
    }
    offset_xn += tilesize;
    offset_yn = offset_yn_top;
  }
}

static void osm_gps_map_fill_tiles_pixel(OsmGpsMap* map)
{
  OsmGpsMapPrivate* priv = map->priv;
  int i, j, tile_x0, tile_y0, tiles_nx, tiles_ny, fmap_x, fmap_y;
  int offset_xn = 0;
  int offset_yn = 0;
  int offset_x;
  int offset_y;
  int tilesize, zoom;

  tilesize = TILESIZE;
  zoom = priv->map_zoom;
  fmap_x = priv->map_x + 0.5 * priv->viewport_width * (1. - 1. / priv->map_factor);
  fmap_y = priv->map_y + 0.5 * priv->viewport_height * (1. - 1. / priv->map_factor);

  offset_x = -fmap_x % tilesize;
  offset_y = -fmap_y % tilesize;
  if (offset_x > 0)
    offset_x -= tilesize;
  if (offset_y > 0)
    offset_y -= tilesize;

  offset_xn = offset_x + 0.5 * priv->viewport_width * (1.5 - 1. / priv->map_factor);
  offset_yn = offset_y + 0.5 * priv->viewport_height * (1.5 - 1. / priv->map_factor);

  tiles_nx = (priv->viewport_width / priv->map_factor - offset_x) / tilesize + 1;
  tiles_ny = (priv->viewport_height / priv->map_factor - offset_y) / tilesize + 1;

  tile_x0 = floor((float)fmap_x / (float)tilesize);
  tile_y0 = floor((float)fmap_y / (float)tilesize);
  // TODO: implement wrap around
  int nRows = (tile_y0 + tiles_ny);

  //   int offset_yn_top = tilesize * (nRows-1) + offset_yn;
  int offset_yn_top = offset_yn;
  offset_yn = offset_yn_top;
  for (i = tile_x0; i < (tile_x0 + tiles_nx); i++)
  {
    for (j = tile_y0; j < nRows; j++)
    {
      if (j < 0 || i < 0 || i >= exp(priv->map_zoom * M_LN2) || j >= exp(priv->map_zoom * M_LN2))
      {
        cairo_rectangle(priv->cr, offset_xn, offset_yn, tilesize, tilesize);
        cairo_set_source_rgb(priv->cr, 1., 1., 1.);
        cairo_fill(priv->cr);
      }
      else
        osm_gps_map_load_tile(map, zoom, i, j, offset_xn, offset_yn, priv->cr);
      offset_yn += tilesize;
    }
    offset_xn += tilesize;
    offset_yn = offset_yn_top;
  }
}

void osm_gps_map_get_tile_xy_at(OsmGpsMap* map, float lat, float lon, int* zoom, int* x, int* y)
{
  int tilesize;

  g_return_if_fail(OSM_IS_GPS_MAP(map));

  tilesize = TILESIZE;
  *zoom = map->priv->map_zoom;
  *x = (int)floor(lon2pixel(map->priv->map_zoom, deg2rad(lon)) / (float)tilesize);
  *y = (int)floor(lat2pixel(map->priv->map_zoom, deg2rad(lat)) / (float)tilesize);
}

static void draw_startpoint(OsmGpsMapPrivate* priv, MaepGeodataTrackIter iter)
{
  double s = 10.;
  int map_x0 = priv->map_x - 0.25 * priv->viewport_width - EXTRA_BORDER;
  int map_y0 = priv->map_y - 0.25 * priv->viewport_height - EXTRA_BORDER;
  int x = lon2pixel(priv->map_zoom, iter.cur->coord.rlon) - map_x0;
  int y = lat2pixel(priv->map_zoom, iter.cur->coord.rlat) - map_y0;

  cairo_move_to(priv->cr, x, y);
  cairo_arc(priv->cr, x, y - 1.5 * s, s, 2. * M_PI / 3., M_PI / 3.);
  cairo_line_to(priv->cr, x, y);
  cairo_new_sub_path(priv->cr);
  cairo_arc(priv->cr, x, y - 1.5 * s, s * 3. / 8., 0, 2 * M_PI);
  // 0xFF/255. 0x14 0x93 DeepPink
  cairo_set_source_rgba(priv->cr, 0xFF / 255.0, 0x14 / 255.0, 0x93 / 255.0, 0.6);
  cairo_fill_preserve(priv->cr);
  cairo_set_source_rgba(priv->cr, 0.0, 0.0, 0.0, 0.6);
  cairo_stroke(priv->cr);
}

static void osm_gps_map_print_track(OsmGpsMapPrivate* priv, MaepGeodata* track, int lw, int* max_x,
                                    int* min_x, int* max_y, int* min_y, int nType)
{
  MaepGeodataTrackIter iter;
  MaepGeodataTrackIter iterFirst = {};
  const way_point_t* wpt;
  int x, y, map_x0, map_y0, st;
  guint i;
  double s;
  gint iwpt;

  map_x0 = priv->map_x - 0.25 * priv->viewport_width - EXTRA_BORDER;
  map_y0 = priv->map_y - 0.25 * priv->viewport_height - EXTRA_BORDER;

  /* Draw all segments. */

  // g_message("track type %d", nType);

  // -1 Distance tool
  if (nType == -1)
    cairo_set_source_rgba(priv->cr, 60000.0 / 65535.0, 0.0, 0.0, 0.6);

  // nType 0 Current track
  else if (nType == 0)
    cairo_set_source_rgba(priv->cr, 60000.0 / 65535.0, 0.0, 0.5, 0.6);
  // 1 Tripp history
  else if (nType == 1)
    cairo_set_source_rgba(priv->cr, 0, 0.9, 0, 0.6);
  // Loaded track
  else if (nType == 2)
    cairo_set_source_rgba(priv->cr, 1, 0.5, 0.01, 0.6);

  cairo_set_line_cap(priv->cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(priv->cr, CAIRO_LINE_JOIN_ROUND);
  maep_geodata_track_iter_new(&iter, track);

  double fStr[2] = {10, 20};
  cairo_set_dash(priv->cr, fStr, 2, 0);
  cairo_set_line_width(priv->cr, lw * 3);

  while (maep_geodata_track_iter_next(&iter, &st))
  {
    x = lon2pixel(priv->map_zoom, iter.cur->coord.rlon) - map_x0;
    y = lat2pixel(priv->map_zoom, iter.cur->coord.rlat) - map_y0;

    if (st & TRACK_POINT_START)
    {
      if (nType == -1)
      {
        iterFirst = iter;
      }
      else
      {
        cairo_move_to(priv->cr, x, y);
        cairo_line_to(priv->cr, x, y);
        cairo_stroke(priv->cr);
        cairo_move_to(priv->cr, x, y);
      }
    }
    cairo_line_to(priv->cr, x, y);
    if (st & TRACK_POINT_STOP)
    {
      cairo_set_line_width(priv->cr, lw);
      cairo_stroke(priv->cr);
      cairo_move_to(priv->cr, x, y);
      cairo_line_to(priv->cr, x, y);
      cairo_set_line_width(priv->cr, lw * 3);
      cairo_stroke(priv->cr);
    }

    *max_x = MAX(x, *max_x);
    *min_x = MIN(x, *min_x);
    *max_y = MAX(y, *max_y);
    *min_y = MIN(y, *min_y);
  }

  /* Draw all way points. */
  iwpt = maep_geodata_waypoint_get_highlight(track);
  cairo_set_line_width(priv->cr, 1);
  cairo_set_fill_rule(priv->cr, CAIRO_FILL_RULE_EVEN_ODD);
  for (i = 0, wpt = maep_geodata_waypoint_get(track, i); wpt;
       wpt = maep_geodata_waypoint_get(track, ++i))
  {
    s = ((gint)i == iwpt) ? 16.66667 : 10.;

    x = lon2pixel(priv->map_zoom, wpt->pt.coord.rlon) - map_x0;
    y = lat2pixel(priv->map_zoom, wpt->pt.coord.rlat) - map_y0;

    cairo_move_to(priv->cr, x, y);
    cairo_arc(priv->cr, x, y - 1.5 * s, s, 2. * M_PI / 3., M_PI / 3.);
    cairo_line_to(priv->cr, x, y);
    cairo_new_sub_path(priv->cr);
    cairo_arc(priv->cr, x, y - 1.5 * s, s * 3. / 8., 0, 2 * M_PI);
    cairo_set_source_rgba(priv->cr, 60000.0 / 65535.0, 0.0, 0.0, 0.6);
    cairo_fill_preserve(priv->cr);
    cairo_set_source_rgba(priv->cr, 0.0, 0.0, 0.0, 0.6);
    cairo_stroke(priv->cr);
  }

  // -1 Distance tool
  if (nType == -1)
    draw_startpoint(priv, iterFirst);
}

/* Prints the gps trip history, and any other tracks */
static void osm_gps_map_print_tracks(OsmGpsMap* map)
{

  OsmGpsMapPrivate* priv = map->priv;
  int lw = priv->ui_gps_track_width;
  int min_x = G_MAXINT, min_y = G_MAXINT, max_x = 0, max_y = 0;
  cairo_rectangle_int_t rect;

  if (priv->tracks)
  {
    /* g_message("Print a track list!"); */

   // if (priv->show_trip_history)
   //   osm_gps_map_print_track(priv, priv->trip_history, lw, &max_x, &min_x, &max_y, &min_y, 1);
    GSList* tmp = priv->tracks;
    while (tmp != NULL)
    {
      int nT = ((OsmTrackRef*)(tmp->data))->m_nTrackType;
      osm_gps_map_print_track(priv, ((OsmTrackRef*)tmp->data)->track, lw, &max_x, &min_x, &max_y,
                              &min_y, nT);
      tmp = g_slist_next(tmp);
    }

    if (max_x > 0 && max_y > 0)
    {
      rect.x = min_x - lw;
      rect.y = min_y - lw;
      rect.width = max_x - min_x + 2 * lw;
      rect.height = max_y - min_y + 2 * lw;
      cairo_region_union_rectangle(priv->dirty, &rect);
    }
  }
}

static gboolean osm_gps_map_purge_cache_check(G_GNUC_UNUSED gpointer key, gpointer value,
                                              gpointer user)
{
  return (((OsmCachedTile*)value)->redraw_cycle != ((OsmGpsMapPrivate*)user)->redraw_cycle);
}

static void osm_gps_map_purge_cache(OsmGpsMap* map)
{
  OsmGpsMapPrivate* priv = map->priv;

  if (g_hash_table_size(priv->tile_cache) < priv->max_tile_cache_size)
    return;

  /* run through the cache, and remove the tiles which have not been used
   * during the last redraw operation */
  g_hash_table_foreach_remove(priv->tile_cache, osm_gps_map_purge_cache_check, priv);
}

void osm_gps_map_blit(OsmGpsMap* map, cairo_t* cr, cairo_operator_t op)
{
  OsmGpsMapPrivate* priv;

  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;

  cairo_surface_flush(priv->map_surf);

  cairo_save(cr);
  cairo_translate(cr, -(1.5 * priv->map_factor - 1.) * priv->viewport_width * 0.5f,
                  -(1.5 * priv->map_factor - 1.) * priv->viewport_height * 0.5f);
  cairo_scale(cr, priv->map_factor, priv->map_factor);

  cairo_set_source_surface(cr, priv->map_surf, 0., 0.);
  cairo_set_operator(cr, op);
  cairo_paint(cr);

  cairo_restore(cr);
}

static gboolean osm_gps_map_redraw(OsmGpsMap* map)
{
  if (g_nSkipDraw != 0)
  {
    return FALSE;
  }

  OsmGpsMapPrivate* priv = map->priv;
  // GSList* list;

  /* on diablo the map comes up at 1x1 pixel size and */
  /* isn't really usable. we'll just ignore this ... */

  if ((priv->viewport_width < 2) || (priv->viewport_height < 2))
  {
    g_message("not a useful sized map yet for source %d ...", priv->map_source);
    return FALSE;
  }

  /* Don't draw anything for a NULL source.
       Caller is responsible for buffer filling. */
  if (priv->map_source == OSM_GPS_MAP_SOURCE_NULL)
    return FALSE;


  priv->redraw_cycle++;

  /* draw transparent background to initialise pixmap */
  cairo_save(priv->cr);
  cairo_set_operator(priv->cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint(priv->cr);
  cairo_restore(priv->cr);

  osm_gps_map_fill_tiles_pixel(map);
  osm_gps_map_print_tracks(map);

  osm_gps_map_print_images(map);

  osm_gps_map_purge_cache(map);

  g_signal_emit_by_name(G_OBJECT(map), "dirty");
  cairo_region_destroy(priv->dirty);
  priv->dirty = cairo_region_create();

  return TRUE;
}

gboolean osm_gps_map_idle_redraw(OsmGpsMap* map)
{
  OsmGpsMapPrivate* priv = map->priv;
  priv->idle_map_redraw = 0;
  osm_gps_map_redraw(map);
  return FALSE;
}

static void center_coord_update(OsmGpsMap* map)
{
  OsmGpsMapPrivate* priv = OSM_GPS_MAP_PRIVATE(map);

  // pixel_x,y, offsets
  gint pixel_x = priv->map_x + priv->viewport_width / 2;
  gint pixel_y = priv->map_y + priv->viewport_height / 2;

  priv->center_rlon = pixel2lon(priv->map_zoom, pixel_x);
  priv->center_rlat = pixel2lat(priv->map_zoom, pixel_y);

  g_object_notify_by_pspec(G_OBJECT(map), properties[PROP_LATITUDE]);
  g_signal_emit_by_name(map, "changed");
}

static void osm_gps_map_init(OsmGpsMap* object)
{
  OsmGpsMapPrivate* priv;

  priv = G_TYPE_INSTANCE_GET_PRIVATE(object, OSM_TYPE_GPS_MAP, OsmGpsMapPrivate);
  object->priv = priv;

  priv->map_surf = NULL;
  priv->cr = NULL;
  priv->map_factor = 1.;
  //priv->trip_history = NULL;
  priv->osm_gps = g_new0(coord_t, 1);
  priv->osm_gps_valid = FALSE;
  priv->cr_markedImage = NULL;
  priv->osm_gps_heading = OSM_GPS_MAP_INVALID;
  priv->tracks = NULL;
  priv->images = NULL;
  // priv->layers = NULL;
  priv->viewport_width = 0;
  priv->viewport_height = 0;
  priv->dirty = cairo_region_create();
  priv->uri_format = 0;
  priv->map_source = -1;
  priv->idle_map_redraw = 0;
  priv->tile_queue = g_hash_table_new(g_str_hash, g_str_equal);

  // Some mapping providers (Google) have varying degrees of tiles at multiple
  // zoom levels
  priv->missing_tiles = g_hash_table_new(g_str_hash, g_str_equal);

  /* memory cache for most recently used tiles */
  priv->tile_cache =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, (GDestroyNotify)cached_tile_free);
  priv->max_tile_cache_size = 80;

  g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_MASK, my_log_handler, NULL);
}

static char* osm_gps_map_get_cache_dir(OsmGpsMapPrivate* priv)
{
  if (priv->tile_base_dir)
    return g_strdup(priv->tile_base_dir);
  return osm_gps_map_get_default_cache_directory();
}

gchar* osm_gps_map_source_get_cache_dir(OsmGpsMapSource_t source, const gchar* tile_dir,
                                        const gchar* base)
{
  gchar* cache_dir;

  if (g_strcmp0(tile_dir, OSM_GPS_MAP_CACHE_AUTO) == 0)
  {
    char* md5 =
        g_compute_checksum_for_string(G_CHECKSUM_MD5, osm_gps_map_source_get_repo_uri(source), -1);

    cache_dir = g_strdup_printf("%s%c%s", base, G_DIR_SEPARATOR, md5);
    g_free(md5);
  }
  else if (g_strcmp0(tile_dir, OSM_GPS_MAP_CACHE_FRIENDLY) == 0)
  {
    cache_dir = g_strdup_printf("%s%c%s", base, G_DIR_SEPARATOR,
                                osm_gps_map_source_get_friendly_name(source));
  }
  else
  {
    cache_dir = g_strdup(tile_dir);
  }

  return cache_dir;
}

static void osm_gps_map_setup(OsmGpsMapPrivate* priv)
{
  const char* uri;
  gchar* base;
  cairo_t* cr;
  priv->map_depth = 0;
  // user can specify a map source ID, or a repo URI as the map source
  if (priv->map_source == OSM_GPS_MAP_SOURCE_NAVIONICS_2 ||
      priv->map_source == OSM_GPS_MAP_SOURCE_NAVIONICS)
  {
    priv->uri_format = URI_HAS_X | URI_HAS_Y | URI_HAS_Z;
    priv->the_navionics = 1;
    get_navionics_key2();
  }
  else
    priv->the_navionics = 0;

  if (priv->map_source == OSM_GPS_MAP_SOURCE_NULL)
  {
    priv->map_source = OSM_GPS_MAP_SOURCE_NULL;

    priv->null_tile = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, TILESIZE, TILESIZE);
    cr = cairo_create(priv->null_tile);
    cairo_set_source_rgb(cr, 1., 1., 1.);
    cairo_paint(cr);
    cairo_destroy(cr);
  }
  else
  {
    // check if the source given is valid

    uri = osm_gps_map_source_get_repo_uri(priv->map_source);

    if (uri)
    {

      // 77g_free((char*)priv->repo_uri);
      // inspect_map_uri(priv->repo_const_uri);
      priv->repo_const_uri = uri;
      priv->image_format = g_strdup(osm_gps_map_source_get_image_format(priv->map_source));
      priv->max_zoom = osm_gps_map_source_get_max_zoom(priv->map_source);
      priv->min_zoom = osm_gps_map_source_get_min_zoom(priv->map_source);
    }
  }

  base = osm_gps_map_get_cache_dir(priv);
  priv->cache_dir = osm_gps_map_source_get_cache_dir(priv->map_source, priv->tile_dir, base);

  g_free(base);
}

static GObject* osm_gps_map_constructor(GType gtype, guint n_properties,
                                        GObjectConstructParam* properties)
{
  OsmGpsMapPrivate* priv;

  // Always chain up to the parent constructor
  GObject* object =
      G_OBJECT_CLASS(osm_gps_map_parent_class)->constructor(gtype, n_properties, properties);

  priv = OSM_GPS_MAP_PRIVATE(object);
  osm_gps_map_setup(priv);
  inspect_map_uri(priv);

  return object;
}

static void osm_gps_map_dispose(GObject* object)
{
  OsmGpsMap* map = OSM_GPS_MAP(object);
  OsmGpsMapPrivate* priv = map->priv;

  if (priv->is_disposed)
    return;

  g_message("disposing map.");
  priv->is_disposed = TRUE;

  g_hash_table_destroy(priv->tile_queue);
  g_hash_table_destroy(priv->missing_tiles);
  g_hash_table_destroy(priv->tile_cache);

  /* images and layers contain GObjects which need unreffing, so free here */
  osm_gps_map_free_images(map);
  // osm_gps_map_free_layers(map);

  cairo_region_destroy(priv->dirty);

  if (priv->cr)
    cairo_destroy(priv->cr);
  if (priv->map_surf)
    cairo_surface_destroy(priv->map_surf);

  if (priv->null_tile)
    cairo_surface_destroy(priv->null_tile);

  if (priv->idle_map_redraw != 0)
    g_source_remove(priv->idle_map_redraw);

  g_free(priv->osm_gps);

  G_OBJECT_CLASS(osm_gps_map_parent_class)->dispose(object);
}

static void osm_gps_map_finalize(GObject* object)
{
  OsmGpsMap* map = OSM_GPS_MAP(object);
  OsmGpsMapPrivate* priv = map->priv;

  if (priv->tile_dir)
    g_free(priv->tile_dir);

  if (priv->cache_dir)
    g_free(priv->cache_dir);

  // g_free((char*)priv->repo_uri);
  g_free(priv->image_format);

  /* trip and tracks contain simple non GObject types, so free them here */
  //osm_gps_map_free_trip(map);
  osm_gps_map_free_tracks(map);

  G_OBJECT_CLASS(osm_gps_map_parent_class)->finalize(object);
}

static void osm_gps_map_set_property(GObject* object, guint prop_id, const GValue* value,
                                     GParamSpec* pspec)
{
  g_return_if_fail(OSM_IS_GPS_MAP(object));
  OsmGpsMap* map = OSM_GPS_MAP(object);
  OsmGpsMapPrivate* priv = map->priv;

  switch (prop_id)
  {
  case PROP_AUTO_CENTER:
    priv->map_auto_center = g_value_get_boolean(value);
    break;
/*
  case PROP_PROXY_URI:
    if (g_value_get_string(value))
    {
      priv->proxy_uri = g_value_dup_string(value);
      g_debug("Setting proxy server: %s", priv->proxy_uri);
    }
    else
      priv->proxy_uri = NULL;

    break;

    */
  case PROP_TILE_CACHE_DIR:
    priv->tile_dir = g_value_dup_string(value);
    break;
  case PROP_TILE_CACHE_BASE_DIR:
    priv->tile_base_dir = g_value_dup_string(value);
    break;
  case PROP_TILE_CACHE_DIR_IS_FULL_PATH:
    g_warning("GObject property tile-cache-is-full-path depreciated");
    break;
  case PROP_ZOOM:
    osm_gps_map_set_zoom(map, g_value_get_int(value));
    break;
    /*
  case PROP_MAX_ZOOM:
    priv->max_zoom = g_value_get_int(value);
    break;
  case PROP_MIN_ZOOM:
    priv->min_zoom = g_value_get_int(value);
    break;

    */
  case PROP_FACTOR:
    osm_gps_map_set_factor(map, g_value_get_float(value));
    break;
  case PROP_MAP_X:
    priv->map_x = g_value_get_int(value);
    g_message("set map_x at %d.", priv->map_x);
    center_coord_update(map);
    break;
  case PROP_MAP_Y:
    priv->map_y = g_value_get_int(value);
    g_message("set map_y at %d.", priv->map_y);
    center_coord_update(map);
    break;
  case PROP_GPS_TRACK_WIDTH:
    priv->ui_gps_track_width = g_value_get_int(value);
    break;
  case PROP_GPS_POINT_R1:
    priv->ui_gps_point_inner_radius = g_value_get_int(value);
    break;
  case PROP_GPS_POINT_R2:
    // The value is given in meters.
    priv->ui_gps_point_outer_radius = g_value_get_int(value);
    break;
  case PROP_MAP_SOURCE:
  {
    OsmGpsMapSource_t old = priv->map_source;
    priv->map_source = g_value_get_uint(value);
    if (priv->map_source != old && priv->map_source < OSM_GPS_MAP_SOURCE_LAST &&
        priv->repo_const_uri)
    {
      g_message("Change map source to %d.", priv->map_source);

      g_hash_table_remove_all(priv->tile_cache);

      osm_gps_map_setup(priv);

      inspect_map_uri(priv);

      if (!priv->idle_map_redraw)
        priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);

      if (priv->map_zoom > priv->max_zoom)
        osm_gps_map_set_zoom(map, priv->max_zoom);

      if (priv->map_zoom < priv->min_zoom)
        osm_gps_map_set_zoom(map, priv->min_zoom);
    }
  }
  break;
  case PROP_IMAGE_FORMAT:
    priv->image_format = g_value_dup_string(value);
    break;
  case PROP_VIEWPORT_WIDTH:
    osm_gps_map_set_viewport(map, g_value_get_uint(value), priv->viewport_height);
    break;
  case PROP_VIEWPORT_HEIGHT:
    osm_gps_map_set_viewport(map, priv->viewport_width, g_value_get_uint(value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

static void osm_gps_map_get_property(GObject* object, guint prop_id, GValue* value,
                                     GParamSpec* pspec)
{
  g_return_if_fail(OSM_IS_GPS_MAP(object));
  OsmGpsMap* map = OSM_GPS_MAP(object);
  OsmGpsMapPrivate* priv = map->priv;

  switch (prop_id)
  {
  /*
  case PROP_DOUBLE_PIXEL:
    g_value_set_boolean(value, priv->double_pixel);
    break;

    */
  case PROP_AUTO_CENTER:
    g_value_set_boolean(value, priv->map_auto_center);
    break;

    /*
  case PROP_RECORD_TRIP_HISTORY:
    g_value_set_boolean(value, priv->record_trip_history);
    break;
  case PROP_SHOW_TRIP_HISTORY:
    g_value_set_boolean(value, priv->show_trip_history);
    break;
  case PROP_AUTO_DOWNLOAD:
    g_value_set_boolean(value, priv->map_auto_download);
    break;
    */
  // case PROP_REPO_URI:
  //  g_value_set_string(value, priv->repo_uri);
  //  break;
 // case PROP_PROXY_URI:
 //   g_value_set_string(value, priv->proxy_uri);
 //    break;
  case PROP_TILE_CACHE_DIR:
    g_value_set_string(value, priv->cache_dir);
    break;
  case PROP_TILE_CACHE_BASE_DIR:
    g_value_set_string(value, priv->tile_base_dir);
    break;
  case PROP_TILE_CACHE_DIR_IS_FULL_PATH:
    g_value_set_boolean(value, FALSE);
    break;
  case PROP_ZOOM:
    g_value_set_int(value, priv->map_zoom);
    break;
    /*
  case PROP_MAX_ZOOM:
    g_value_set_int(value, priv->max_zoom);
    break;
  case PROP_MIN_ZOOM:
    g_value_set_int(value, priv->min_zoom);
    break;

    */
  case PROP_FACTOR:
    g_value_set_float(value, priv->map_factor);
    break;
  case PROP_LATITUDE:
    g_value_set_float(value, rad2deg(priv->center_rlat));
    break;
  case PROP_LONGITUDE:
    g_value_set_float(value, rad2deg(priv->center_rlon));
    break;
  case PROP_MAP_X:
    g_value_set_int(value, priv->map_x);
    break;
  case PROP_MAP_Y:
    g_value_set_int(value, priv->map_y);
    break;
  case PROP_TILES_QUEUED:
    g_value_set_int(value, g_hash_table_size(priv->tile_queue));
    break;
  case PROP_GPS_TRACK_WIDTH:
    g_value_set_int(value, priv->ui_gps_track_width);
    break;
  case PROP_GPS_POINT_R1:
    g_value_set_int(value, priv->ui_gps_point_inner_radius);
    break;
  case PROP_GPS_POINT_R2:
    g_value_set_int(value, priv->ui_gps_point_outer_radius);
    break;
  case PROP_MAP_SOURCE:
    g_value_set_uint(value, priv->map_source);
    break;
  case PROP_IMAGE_FORMAT:
    g_value_set_string(value, priv->image_format);
    break;
  case PROP_VIEWPORT_WIDTH:
    g_value_set_uint(value, priv->viewport_width);
    /*g_message("get width %d.", priv->viewport_width);*/
    break;
  case PROP_VIEWPORT_HEIGHT:
    g_value_set_uint(value, priv->viewport_height);
    /*g_message("get height %d.", priv->viewport_height);*/
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
  }
}

void osm_gps_map_set_viewport(OsmGpsMap* map, guint width, guint height)
{
  OsmGpsMapPrivate* priv;

  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;

  g_message("Set view port to %dx%d for source %d.", width, height, priv->map_source);
  if (priv->viewport_width == width && priv->viewport_height == height)
    return;

  /* Set viewport. */
  priv->viewport_width = width;
  priv->viewport_height = height;

  if (priv->map_surf)
    cairo_surface_destroy(priv->map_surf);
  priv->map_surf =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1.5 * priv->viewport_width + EXTRA_BORDER * 2,
                                 1.5 * priv->viewport_height + EXTRA_BORDER * 2);
  if (priv->cr)
    cairo_destroy(priv->cr);
  priv->cr = cairo_create(priv->map_surf);

  // pixel_x,y, offsets
  gint pixel_x = lon2pixel(priv->map_zoom, priv->center_rlon);
  gint pixel_y = lat2pixel(priv->map_zoom, priv->center_rlat);

  priv->map_x = pixel_x - priv->viewport_width / 2;
  priv->map_y = pixel_y - priv->viewport_height / 2;

  g_object_notify_by_pspec(G_OBJECT(map), properties[PROP_VIEWPORT_WIDTH]);
  g_object_notify_by_pspec(G_OBJECT(map), properties[PROP_VIEWPORT_HEIGHT]);

  osm_gps_map_redraw(map);

  g_signal_emit_by_name(map, "changed");
}

// This is called through some macro magic on g_object_new
static void osm_gps_map_class_init(OsmGpsMapClass* klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS(klass);

  g_type_class_add_private(klass, sizeof(OsmGpsMapPrivate));

  object_class->dispose = osm_gps_map_dispose;
  object_class->finalize = osm_gps_map_finalize;
  object_class->constructor = osm_gps_map_constructor;
  object_class->set_property = osm_gps_map_set_property;
  object_class->get_property = osm_gps_map_get_property;

  properties[PROP_AUTO_CENTER] =
      g_param_spec_boolean("auto-center", "auto center", "map auto center", TRUE,
                           G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT);

  g_object_class_install_property(object_class, PROP_AUTO_CENTER, properties[PROP_AUTO_CENTER]);

  g_object_class_install_property(
      object_class, PROP_TILE_CACHE_DIR,
      g_param_spec_string("tile-cache", "tile cache", "osm local tile cache dir",
                          OSM_GPS_MAP_CACHE_AUTO,
                          G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property(
      object_class, PROP_TILE_CACHE_BASE_DIR,
      g_param_spec_string("tile-cache-base", "tile cache-base",
                          "base directory to which friendly and auto paths are appended", NULL,
                          G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property(object_class, PROP_TILE_CACHE_DIR_IS_FULL_PATH,
                                  g_param_spec_boolean("tile-cache-is-full-path",
                                                       "tile cache is full path", "DEPRECIATED",
                                                       FALSE, G_PARAM_READABLE | G_PARAM_WRITABLE));

  properties[PROP_ZOOM] = g_param_spec_int("zoom", "zoom", "initial zoom level", MIN_ZOOM, MAX_ZOOM,
                                           3, G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(object_class, PROP_ZOOM, properties[PROP_ZOOM]);

  properties[PROP_FACTOR] = g_param_spec_float("factor", "factor", "zooming adjustment factor", 0.4,
                                               2.8, 1., G_PARAM_READWRITE);
  g_object_class_install_property(object_class, PROP_FACTOR, properties[PROP_FACTOR]);


  /*
  g_object_class_install_property(
      object_class, PROP_MAX_ZOOM,
      g_param_spec_int("max-zoom", "max zoom", "maximum zoom level", MIN_ZOOM, MAX_ZOOM,
                       OSM_MAX_ZOOM, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  g_object_class_install_property(
      object_class, PROP_MIN_ZOOM,
      g_param_spec_int("min-zoom", "min zoom", "minimum zoom level", MIN_ZOOM, MAX_ZOOM,
                       OSM_MIN_ZOOM, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  */

  properties[PROP_LATITUDE] = g_param_spec_float("latitude", "latitude", "latitude in degrees",
                                                 -90.0, 90.0, 0, G_PARAM_READABLE);
  g_object_class_install_property(object_class, PROP_LATITUDE, properties[PROP_LATITUDE]);

  properties[PROP_LONGITUDE] = g_param_spec_float("longitude", "longitude", "longitude in degrees",
                                                  -180.0, 180.0, 0, G_PARAM_READABLE);
  g_object_class_install_property(object_class, PROP_LONGITUDE, properties[PROP_LONGITUDE]);

  properties[PROP_MAP_X] = g_param_spec_int("map-x", "map-x", "initial map x location", G_MININT,
                                            G_MAXINT, 890, G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(object_class, PROP_MAP_X, properties[PROP_MAP_X]);

  properties[PROP_MAP_Y] = g_param_spec_int("map-y", "map-y", "initial map y location", G_MININT,
                                            G_MAXINT, 515, G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(object_class, PROP_MAP_Y, properties[PROP_MAP_Y]);

  g_object_class_install_property(object_class, PROP_TILES_QUEUED,
                                  g_param_spec_int("tiles-queued", "tiles-queued",
                                                   "number of tiles currently waiting to download",
                                                   G_MININT, G_MAXINT, 0, G_PARAM_READABLE));

  g_object_class_install_property(
      object_class, PROP_GPS_TRACK_WIDTH,
      g_param_spec_int("gps-track-width", "gps-track-width",
                       "width of the lines drawn for the gps track", 1, G_MAXINT, 4,
                       G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

  g_object_class_install_property(
      object_class, PROP_GPS_POINT_R1,
      g_param_spec_int("gps-track-point-radius", "gps-track-point-radius",
                       "radius of the gps point inner circle", 0, G_MAXINT, 5,
                       G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

  g_object_class_install_property(
      object_class, PROP_GPS_POINT_R2,
      g_param_spec_int("gps-track-highlight-radius", "gps-track-highlight-radius",
                       "radius of the gps point highlight circle", 0, G_MAXINT, 20,
                       G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT));

  properties[PROP_MAP_SOURCE] = g_param_spec_uint(
      "map-source", "map source", "map source ID", OSM_GPS_MAP_SOURCE_NULL, OSM_GPS_MAP_SOURCE_LAST,
      OSM_GPS_MAP_SOURCE_LAST, G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT);

  g_object_class_install_property(object_class, PROP_MAP_SOURCE, properties[PROP_MAP_SOURCE]);

  g_object_class_install_property(
      object_class, PROP_IMAGE_FORMAT,
      g_param_spec_string("image-format", "image format",
                          "map source tile repository image format (jpg, png)", OSM_IMAGE_FORMAT,
                          G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

  properties[PROP_VIEWPORT_WIDTH] =
      g_param_spec_uint("viewport-width", "Viewport width", "width of the viewable area", 1, 2048,
                        1, G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(object_class, PROP_VIEWPORT_WIDTH,
                                  properties[PROP_VIEWPORT_WIDTH]);

  properties[PROP_VIEWPORT_HEIGHT] =
      g_param_spec_uint("viewport-height", "Viewport height", "height of the viewable area", 1,
                        2048, 1, G_PARAM_READABLE | G_PARAM_WRITABLE);
  g_object_class_install_property(object_class, PROP_VIEWPORT_HEIGHT,
                                  properties[PROP_VIEWPORT_HEIGHT]);

  g_signal_new("changed", OSM_TYPE_GPS_MAP, G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
               g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  g_signal_new("dirty", OSM_TYPE_GPS_MAP, G_SIGNAL_RUN_FIRST, 0, NULL, NULL,
               g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

const char* osm_gps_map_source_get_friendly_name(OsmGpsMapSource_t source)
{
  switch (source)
  {
  case OSM_GPS_MAP_SOURCE_NULL:
    return "None";
  case OSM_GPS_MAP_SOURCE_NAVIONICS:
    return "NAVIONICS Sonar";
  case OSM_GPS_MAP_SOURCE_NAVIONICS_2:
    return "NAVIONICS Sea Chart";
  case OSM_GPS_MAP_SOURCE_OPENSTREETMAP:
    return "OpenStreetMap I";
  case OSM_GPS_MAP_SOURCE_MML_PERUSKARTTA:
    return "Peruskartta";
  case OSM_GPS_MAP_SOURCE_MML_ORTOKUVA:
    return "Ortoilmakuva";
  case OSM_GPS_MAP_SOURCE_MML_TAUSTAKARTTA:
    return "Taustakartta";
  case OSM_GPS_MAP_SOURCE_OPENSTREETMAP_RENDERER:
    return "OpenStreetMap II";
  case OSM_GPS_MAP_SOURCE_OPENAERIALMAP:
    return "OpenAerialMap";
  case OSM_GPS_MAP_SOURCE_OPENSEAMAP:
    return "OpenSeaMap";
  case OSM_GPS_MAP_SOURCE_OPENCYCLEMAP:
    return "OpenCycleMap";
  case OSM_GPS_MAP_SOURCE_OSM_PUBLIC_TRANSPORT:
    return "Public Transport";
  case OSM_GPS_MAP_SOURCE_OSMC_TRAILS:
    return "OSMC Trails";
  case OSM_GPS_MAP_SOURCE_MAPS_FOR_FREE:
    return "Maps-For-Free";
  case OSM_GPS_MAP_SOURCE_GOOGLE_STREET:
    return "Google Maps";
  case OSM_GPS_MAP_SOURCE_GOOGLE_SATELLITE:
    return "Google Satellite";
  case OSM_GPS_MAP_SOURCE_GOOGLE_HYBRID:
    return "Google Hybrid";
  case OSM_GPS_MAP_SOURCE_GOOGLE_TRAFFIC:
    return "Google traffic";
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_STREET:
    return "Virtual Earth";
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_SATELLITE:
    return "Virtual Earth Satellite";
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_HYBRID:
    return "Virtual Earth Hybrid";
  case OSM_GPS_MAP_SOURCE_YAHOO_STREET:
    return "Yahoo Maps";
  case OSM_GPS_MAP_SOURCE_YAHOO_SATELLITE:
    return "Yahoo Satellite";
  case OSM_GPS_MAP_SOURCE_YAHOO_HYBRID:
    return "Yahoo Hybrid";
  case OSM_GPS_MAP_SOURCE_LAST:
  default:
    return NULL;
  }
  return NULL;
}

// http://www.internettablettalk.com/forums/showthread.php?t=5209
// https://garage.maemo.org/plugins/scmsvn/viewcvs.php/trunk/src/maps.c?root=maemo-mapper&view=markup
// http://www.ponies.me.uk/maps/GoogleTileUtils.java
// http://www.mgmaps.com/cache/MapTileCacher.perl

const char* osm_gps_map_source_get_repo_uri(OsmGpsMapSource_t source)
{
  switch (source)
  {
  case OSM_GPS_MAP_SOURCE_NULL:
    return "none://";
  case OSM_GPS_MAP_SOURCE_OPENSTREETMAP:
    return OSM_REPO_URI;
  case OSM_GPS_MAP_SOURCE_MML_PERUSKARTTA:
    return "http://tiles.kartat.kapsi.fi/peruskartta/#Z/#X/#Y.png";
  case OSM_GPS_MAP_SOURCE_MML_ORTOKUVA:
    return "http://tiles.kartat.kapsi.fi/ortokuva/#Z/#X/#Y.png";
  case OSM_GPS_MAP_SOURCE_MML_TAUSTAKARTTA:
    return "http://tiles.kartat.kapsi.fi/taustakartta/#Z/#X/#Y.png";
  case OSM_GPS_MAP_SOURCE_OPENAERIALMAP:
    /* OpenAerialMap is down, offline till furthur notice
               http://openaerialmap.org/pipermail/talk_openaerialmap.org/2008-December/000055.html
     */
    return "";

    // curl
    // 'https://backend.navionics.com/tile/get_key/NAVIONICS_WEBAPP_P01/webapp.navionics.com?_=1690792123234'
    // -H 'Referer: https://webapp.navionics.com/'

  case OSM_GPS_MAP_SOURCE_NAVIONICS:
    return g_szNAVURL1;
  case OSM_GPS_MAP_SOURCE_NAVIONICS_2:
    return g_szNAVURL2;

  case OSM_GPS_MAP_SOURCE_OPENSEAMAP:
    return "https://map04.eniro.com/geowebcache/service/tms1.0.0/nautical/#Z/#X/#U.png?";
    // return "http://t1.openseamap.org/seamark/#Z/#X/#Y.png";
  case OSM_GPS_MAP_SOURCE_OPENSTREETMAP_RENDERER:
    return "http://otile1.mqcdn.com/tiles/1.0.0/osm/#Z/#X/#Y.png";
    /* return "http://tah.openstreetmap.org/Tiles/tile/#Z/#X/#Y.png"; */
  case OSM_GPS_MAP_SOURCE_OPENCYCLEMAP:
    return "http://c.tile.opencyclemap.org/cycle/#Z/#X/#Y.png";
  case OSM_GPS_MAP_SOURCE_OSM_PUBLIC_TRANSPORT:
    return "http://tile.xn--pnvkarte-m4a.de/tilegen/#Z/#X/#Y.png";
  case OSM_GPS_MAP_SOURCE_OSMC_TRAILS:
    return "http://topo.geofabrik.de/trails/#Z/#X/#Y.png";
  case OSM_GPS_MAP_SOURCE_MAPS_FOR_FREE:
    return "http://maps-for-free.com/layer/relief/z#Z/row#Y/#Z_#X-#Y.jpg";
  case OSM_GPS_MAP_SOURCE_GOOGLE_STREET:
    return "https://mt0.google.com/vt/lyrs=m&x=#X&y=#Y&z=#Z";
    // return "http://mt#R.google.com/vt/v=w2.97&x=#X&y=#Y&z=#Z";
    /* http://mt0.google.com/mapstt?zoom=13&x=1406&y=3272 */
  case OSM_GPS_MAP_SOURCE_GOOGLE_HYBRID:
    /* No longer working
               "http://mt#R.google.com/mt?n=404&v=w2t.99&x=#X&y=#Y&zoom=#S" */
    return "";
  case OSM_GPS_MAP_SOURCE_GOOGLE_SATELLITE:
    return "http://khm#R.google.com/kh/v=51&x=#X&y=#Y&z=#Z";
  case OSM_GPS_MAP_SOURCE_GOOGLE_TRAFFIC:
    return "http://mt#R.google.com/mapstt?zoom=#Z&x=#X&y=#Y";
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_STREET:
    return "http://a#R.ortho.tiles.virtualearth.net/tiles/r#W.jpeg?g=50";
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_SATELLITE:
    return "http://a#R.ortho.tiles.virtualearth.net/tiles/a#W.png?g=50";
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_HYBRID:
    return "http://a#R.ortho.tiles.virtualearth.net/tiles/h#W.jpeg?g=50";
  case OSM_GPS_MAP_SOURCE_YAHOO_STREET:
  case OSM_GPS_MAP_SOURCE_YAHOO_SATELLITE:
  case OSM_GPS_MAP_SOURCE_YAHOO_HYBRID:
    /* TODO: Implement signed Y, aka U
     * http://us.maps3.yimg.com/aerial.maps.yimg.com/ximg?v=1.7&t=a&s=256&x=%d&y=%-d&z=%d
     *  x = tilex,
     *  y = (1 << (MAX_ZOOM - zoom)) - tiley - 1,
     *  z = zoom - (MAX_ZOOM - 17));
     */
    return "";
  case OSM_GPS_MAP_SOURCE_LAST:
  default:
    return "";
  }
  return "";
}

void osm_gps_map_source_get_repo_copyright(OsmGpsMapSource_t source, const gchar** notice,
                                           const gchar** url)
{
  g_return_if_fail(notice && url);

  *notice = NULL;
  *url = NULL;
  switch (source)
  {
  case OSM_GPS_MAP_SOURCE_NULL:
    return;
  case OSM_GPS_MAP_SOURCE_OPENSTREETMAP:
    *notice = "© OpenStreetMap contributors";
    *url = "http://www.openstreetmap.org/copyright";
    return;
  case OSM_GPS_MAP_SOURCE_MML_PERUSKARTTA:
  case OSM_GPS_MAP_SOURCE_MML_ORTOKUVA:
  case OSM_GPS_MAP_SOURCE_MML_TAUSTAKARTTA:
    *notice = "CC 4.0 licence (© Maanmittauslaitos)";
    *url = "http://www.maanmittauslaitos.fi/"
           "avoimen-tietoaineiston-cc-40-lisenssi";
    return;
  case OSM_GPS_MAP_SOURCE_OPENAERIALMAP:
    return;
  case OSM_GPS_MAP_SOURCE_NAVIONICS_2:
  case OSM_GPS_MAP_SOURCE_NAVIONICS:
    *notice = "© navionics contributors";
    *url = "https://navionics.com/";
    return;
  case OSM_GPS_MAP_SOURCE_OPENSEAMAP:
    *notice = "© OpenStreetMap contributors";
    *url = "http://openseamap.org/";
    return;
  case OSM_GPS_MAP_SOURCE_OPENSTREETMAP_RENDERER:
    *notice = "Tiles Courtesy of MapQuest";
    *url = "http://www.mapquest.com/";
    return;
  case OSM_GPS_MAP_SOURCE_OPENCYCLEMAP:
    *notice = "© OpenCycleMap";
    *url = "http://www.opencyclemap.org/";
    return;
  case OSM_GPS_MAP_SOURCE_OSM_PUBLIC_TRANSPORT:
    *notice = "CC-BY-SA license (© by MeMomaps)";
    *url = "http://memomaps.de";
    return;
  case OSM_GPS_MAP_SOURCE_OSMC_TRAILS:
  case OSM_GPS_MAP_SOURCE_MAPS_FOR_FREE:
    return;
  case OSM_GPS_MAP_SOURCE_GOOGLE_STREET:
  case OSM_GPS_MAP_SOURCE_GOOGLE_TRAFFIC:
    *notice = "©2014 Google";
    *url = "http://www.google.com/intl/fr_fr/help/legalnotices_maps.html";
    return;
  case OSM_GPS_MAP_SOURCE_GOOGLE_HYBRID:
  case OSM_GPS_MAP_SOURCE_GOOGLE_SATELLITE:
    return;
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_STREET:
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_SATELLITE:
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_HYBRID:
    *notice = "©2014 Microsoft Corporation";
    *url = "http://windows.microsoft.com:80/en-gb/windows-live/"
           "microsoft-services-agreement";
    return;
  case OSM_GPS_MAP_SOURCE_YAHOO_STREET:
  case OSM_GPS_MAP_SOURCE_YAHOO_SATELLITE:
  case OSM_GPS_MAP_SOURCE_YAHOO_HYBRID:
    return;
  case OSM_GPS_MAP_SOURCE_LAST:
  default:
    return;
  }
}

const char* osm_gps_map_source_get_image_format(OsmGpsMapSource_t source)
{
  switch (source)
  {
  case OSM_GPS_MAP_SOURCE_NULL:
  case OSM_GPS_MAP_SOURCE_OPENSTREETMAP:
  case OSM_GPS_MAP_SOURCE_NAVIONICS_2:
  case OSM_GPS_MAP_SOURCE_NAVIONICS:
  case OSM_GPS_MAP_SOURCE_MML_PERUSKARTTA:
  case OSM_GPS_MAP_SOURCE_MML_ORTOKUVA:
  case OSM_GPS_MAP_SOURCE_MML_TAUSTAKARTTA:
  case OSM_GPS_MAP_SOURCE_OPENCYCLEMAP:
  case OSM_GPS_MAP_SOURCE_OPENSEAMAP:
  case OSM_GPS_MAP_SOURCE_OSM_PUBLIC_TRANSPORT:
  case OSM_GPS_MAP_SOURCE_OSMC_TRAILS:
  case OSM_GPS_MAP_SOURCE_GOOGLE_STREET:
  case OSM_GPS_MAP_SOURCE_GOOGLE_TRAFFIC:
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_STREET:
    return "png";
  case OSM_GPS_MAP_SOURCE_OPENSTREETMAP_RENDERER:
  case OSM_GPS_MAP_SOURCE_OPENAERIALMAP:
  case OSM_GPS_MAP_SOURCE_GOOGLE_HYBRID:
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_SATELLITE:
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_HYBRID:
  case OSM_GPS_MAP_SOURCE_YAHOO_STREET:
  case OSM_GPS_MAP_SOURCE_YAHOO_SATELLITE:
  case OSM_GPS_MAP_SOURCE_YAHOO_HYBRID:
  case OSM_GPS_MAP_SOURCE_MAPS_FOR_FREE:
  case OSM_GPS_MAP_SOURCE_GOOGLE_SATELLITE:
    return "jpg";
  case OSM_GPS_MAP_SOURCE_LAST:
  default:
    return "bin";
  }
  return "bin";
}

int osm_gps_map_source_get_min_zoom(G_GNUC_UNUSED OsmGpsMapSource_t source)
{
  return 1;
}

int osm_gps_map_source_get_max_zoom(OsmGpsMapSource_t source)
{
  switch (source)
  {
  case OSM_GPS_MAP_SOURCE_NAVIONICS:
  case OSM_GPS_MAP_SOURCE_NAVIONICS_2:
  case OSM_GPS_MAP_SOURCE_NULL:
  case OSM_GPS_MAP_SOURCE_GOOGLE_TRAFFIC:
    return 18;
  case OSM_GPS_MAP_SOURCE_OPENSTREETMAP:
  case OSM_GPS_MAP_SOURCE_OPENCYCLEMAP:
  case OSM_GPS_MAP_SOURCE_OSM_PUBLIC_TRANSPORT:
  case OSM_GPS_MAP_SOURCE_OPENSEAMAP:
    return OSM_MAX_ZOOM;
  case OSM_GPS_MAP_SOURCE_OPENSTREETMAP_RENDERER:
  case OSM_GPS_MAP_SOURCE_OPENAERIALMAP:

  case OSM_GPS_MAP_SOURCE_GOOGLE_HYBRID:
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_STREET:
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_SATELLITE:
  case OSM_GPS_MAP_SOURCE_VIRTUAL_EARTH_HYBRID:
  case OSM_GPS_MAP_SOURCE_YAHOO_STREET:
  case OSM_GPS_MAP_SOURCE_YAHOO_SATELLITE:
  case OSM_GPS_MAP_SOURCE_YAHOO_HYBRID:
    return 17;
  case OSM_GPS_MAP_SOURCE_OSMC_TRAILS:
    return 15;
  case OSM_GPS_MAP_SOURCE_MAPS_FOR_FREE:
    return 11;
  case OSM_GPS_MAP_SOURCE_GOOGLE_SATELLITE:
    return 18;
  case OSM_GPS_MAP_SOURCE_MML_PERUSKARTTA:
  case OSM_GPS_MAP_SOURCE_MML_ORTOKUVA:
  case OSM_GPS_MAP_SOURCE_MML_TAUSTAKARTTA:
    return 20;
  case OSM_GPS_MAP_SOURCE_GOOGLE_STREET:
    return 21;
  case OSM_GPS_MAP_SOURCE_LAST:
  default:
    return 17;
  }
  return 17;
}

void osm_gps_map_download_maps(OsmGpsMap* map, coord_t* pt1, coord_t* pt2, int zoom_start,
                               int zoom_end)
{
  int i, j, zoom, num_tiles;
  OsmGpsMapPrivate* priv = map->priv;

  if (pt1 && pt2)
  {
    gchar* filename;
    num_tiles = 0;
    zoom_end = CLAMP(zoom_end, priv->min_zoom, priv->max_zoom);

    for (zoom = zoom_start; zoom <= zoom_end; zoom++)
    {
      int x1, y1, x2, y2;

      x1 = (int)floor((float)lon2pixel(zoom, pt1->rlon) / (float)TILESIZE);
      y1 = (int)floor((float)lat2pixel(zoom, pt1->rlat) / (float)TILESIZE);

      x2 = (int)floor((float)lon2pixel(zoom, pt2->rlon) / (float)TILESIZE);
      y2 = (int)floor((float)lat2pixel(zoom, pt2->rlat) / (float)TILESIZE);

      // loop x1-x2
      for (i = x1; i <= x2; i++)
      {
        // loop y1 - y2
        for (j = y1; j <= y2; j++)
        {
          // x = i, y = j
          filename = g_strdup_printf("%s%c%d%c%d%c%d.%s", priv->cache_dir, G_DIR_SEPARATOR, zoom,
                                     G_DIR_SEPARATOR, i, G_DIR_SEPARATOR, j, priv->image_format);

          if ((!g_file_test(filename, G_FILE_TEST_EXISTS)))
          {
            osm_gps_map_download_tile2(map, zoom, i, j, FALSE);
            num_tiles++;
          }

          g_free(filename);
        }
      }
    }
  }
}

void osm_gps_map_get_bbox(OsmGpsMap* map, coord_t* pt1, coord_t* pt2)
{
  OsmGpsMapPrivate* priv = map->priv;

  if (pt1 && pt2)
  {
    pt1->rlat = pixel2lat(priv->map_zoom, priv->map_y);
    pt1->rlon = pixel2lon(priv->map_zoom, priv->map_x);
    pt2->rlat = pixel2lat(priv->map_zoom, priv->map_y + priv->viewport_height);
    pt2->rlon = pixel2lon(priv->map_zoom, priv->map_x + priv->viewport_width);

    g_debug("BBOX: %f %f %f %f", pt1->rlat, pt1->rlon, pt2->rlat, pt2->rlon);
  }
}

static void _update_screen_pos(OsmGpsMap* map)
{
  OsmGpsMapPrivate* priv;

  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;

  priv->map_x = lon2pixel(priv->map_zoom, priv->center_rlon) - priv->viewport_width / 2;
  priv->map_y = lat2pixel(priv->map_zoom, priv->center_rlat) - priv->viewport_height / 2;

  if (!priv->idle_map_redraw)
    priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);

  g_object_notify_by_pspec(G_OBJECT(map), properties[PROP_MAP_Y]);
  g_signal_emit_by_name(map, "changed");
}

static gboolean _set_center(OsmGpsMap* map, float rlat, float rlon)
{
  g_return_val_if_fail(OSM_IS_GPS_MAP(map), FALSE);

  if (rlat == map->priv->center_rlat && rlon == map->priv->center_rlon)
    return FALSE;

  map->priv->center_rlat = rlat;
  map->priv->center_rlon = rlon;
  g_object_notify_by_pspec(G_OBJECT(map), properties[PROP_LATITUDE]);

  return TRUE;
}

static gboolean _set_zoom(OsmGpsMap* map, int zoom)
{
  OsmGpsMapPrivate* priv;

  g_return_val_if_fail(OSM_IS_GPS_MAP(map), FALSE);
  priv = map->priv;

  // constrain zoom min_zoom -> max_zoom
  zoom = CLAMP(zoom, priv->min_zoom, priv->max_zoom);
  if (zoom == priv->map_zoom)
    return FALSE;

  priv->map_zoom = zoom;
  g_object_notify_by_pspec(G_OBJECT(map), properties[PROP_ZOOM]);

  return TRUE;
}

void osm_gps_map_set_center(OsmGpsMap* map, float latitude, float longitude)
{
  g_object_set(G_OBJECT(map), "auto-center", FALSE, NULL);
  if (_set_center(map, deg2rad(latitude), deg2rad(longitude)))
    _update_screen_pos(map);
}

int osm_gps_map_set_zoom(OsmGpsMap* map, int zoom)
{
  g_return_val_if_fail(OSM_IS_GPS_MAP(map), 0);
  if (_set_zoom(map, zoom))
    _update_screen_pos(map);

  return map->priv->map_zoom;
}

int osm_gps_map_get_zoom(OsmGpsMap* map)
{
  g_return_val_if_fail(OSM_IS_GPS_MAP(map), 0);

  return map->priv->map_zoom;
}

void osm_gps_map_magnifye(OsmGpsMap* map, int nOrder)
{
  if (nOrder > 0)
    osm_gps_map_set_factor(map, map->priv->map_factor + 0.5);
  else
    osm_gps_map_set_factor(map, map->priv->map_factor - 0.5);
}

int osm_gps_map_depth(OsmGpsMap* map)
{
  if (map->priv->the_navionics == 0)
    return -1;
  return map->priv->map_depth;
}

void osm_gps_map_set_depth(OsmGpsMap* map, int depthDm)
{
  map->priv->map_depth = depthDm;
}

void osm_gps_map_set_windSpeed(OsmGpsMap* map, double speedMs, double directionDeg, double tempDeg)
{
  map->priv->tempDeg = tempDeg;
  map->priv->windSpeedMs = speedMs;
  map->priv->windDirectionRad = deg2rad(directionDeg);
}

double windSpeedMs(OsmGpsMap* map)
{
  return map->priv->windSpeedMs;
}

double windDirRad(OsmGpsMap* map)
{
  return map->priv->windDirectionRad;
}

double tempDeg(OsmGpsMap* map)
{
  return map->priv->tempDeg;
}

int osm_gps_map_zoom_in(OsmGpsMap* map)
{
  g_return_val_if_fail(OSM_IS_GPS_MAP(map), 0);

  //  int nLZoom = map->priv->map_zoom;
  int nRZoom = osm_gps_map_set_zoom(map, map->priv->map_zoom + 1);

  return nRZoom;
}

int osm_gps_map_zoom_out(OsmGpsMap* map)
{
  g_return_val_if_fail(OSM_IS_GPS_MAP(map), 0);

  return osm_gps_map_set_zoom(map, map->priv->map_zoom - 1);
}

void osm_gps_map_set_factor(OsmGpsMap* map, gfloat factor)
{
  g_return_if_fail(OSM_IS_GPS_MAP(map));

  factor = CLAMP(factor, 1, 5);
  if (factor == map->priv->map_factor)
    return;
  map->priv->map_factor = factor;

  if (!map->priv->idle_map_redraw)
    map->priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);

  g_object_notify_by_pspec(G_OBJECT(map), properties[PROP_FACTOR]);
  g_signal_emit_by_name(map, "changed");
}
gfloat osm_gps_map_get_factor(OsmGpsMap* map)
{
  g_return_val_if_fail(OSM_IS_GPS_MAP(map), 1.f);

  return map->priv->map_factor;
}

void osm_gps_map_set_mapcenter(OsmGpsMap* map, float latitude, float longitude, int zoom)
{
  gboolean update;

  g_object_set(G_OBJECT(map), "auto-center", FALSE, NULL);
  update = _set_center(map, deg2rad(latitude), deg2rad(longitude));
  update = _set_zoom(map, zoom) || update;
  if (update)
    _update_screen_pos(map);
}

void osm_gps_map_adjust_to(OsmGpsMap* map, coord_t* top_left, coord_t* bottom_right)
{
  gboolean update;
  float dlon, lon0, lat0;
  int zoom_lon, zoom_lat, size;

  g_return_if_fail(OSM_IS_GPS_MAP(map) && top_left && bottom_right);

  dlon = bottom_right->rlon - top_left->rlon;
  lon0 = (top_left->rlon + bottom_right->rlon) * 0.5f;
  if (bottom_right->rlon < top_left->rlon)
  {
    dlon += 2 * M_PI;
    lon0 += (lon0 > 0) ? -M_PI : M_PI;
  }
  lat0 = (top_left->rlat + bottom_right->rlat) * 0.5f;
  g_message("Variations are dlat = %g x dlon = %g", top_left->rlat - bottom_right->rlat, dlon);
  g_message("Center is      lat0 = %g x lon0 = %g", lat0, lon0);

  size = (int)(2. * M_PI * map->priv->viewport_width / TILESIZE / dlon);
  g_message("Fit lon zoom to %d", size);
  for (zoom_lon = 0; size > 1; zoom_lon++)
    size = (size >> 1);

  size = (int)(2. * M_PI * map->priv->viewport_height / TILESIZE /
               (atanh(sin(bottom_right->rlat)) - atanh(sin(top_left->rlat))));
  g_message("Fit lat zoom to %d", size);
  for (zoom_lat = 0; size > 1; zoom_lat++)
    size = (size >> 1);

  g_object_set(G_OBJECT(map), "auto-center", FALSE, NULL);

  g_message("Set fitting zoom from %d x %d", zoom_lat, zoom_lon);
  update = _set_center(map, lat0, lon0);
  update = _set_zoom(map, MIN(zoom_lat, zoom_lon)) || update;
  if (update)
    _update_screen_pos(map);
}

void osm_gps_map_auto_center_at(OsmGpsMap* map, float latitude, float longitude)
{
  OsmGpsMapPrivate* priv;

  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;

  // Automatically center the map if the track approaches the edge
  if (priv->map_auto_center)
  {
    // pixel_x,y, offsets
    int x, y;
    int width = priv->viewport_width;
    int height = priv->viewport_height;
    coord_t pos;

    pos.rlat = deg2rad(latitude);
    pos.rlon = deg2rad(longitude);
    osm_gps_map_from_co_ordinates(map, &pos, &x, &y);
    if (x < (width / 2 - width / 8) || x > (width / 2 + width / 8) ||
        y < (height / 2 - height / 8) || y > (height / 2 + height / 8))
    {
      if (_set_center(map, pos.rlat, pos.rlon))
        _update_screen_pos(map);
    }
  }
}

static void _on_track_changed(G_GNUC_UNUSED MaepGeodata* track_state,
                              G_GNUC_UNUSED GParamSpec* pspec, OsmGpsMap* map)
{
  OsmGpsMapPrivate* priv;

  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;

  if (!priv->idle_map_redraw)
    priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
}
static void _on_track_dirty(G_GNUC_UNUSED MaepGeodata* track_state, OsmGpsMap* map)
{
  OsmGpsMapPrivate* priv;

  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;

  if (!priv->idle_map_redraw)
    priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
}

MaepGeodata* osm_get_track(OsmGpsMap* map, int nId)
{
  OsmGpsMapPrivate* priv = map->priv;
  if (priv->tracks)
  {
    GSList* tmp = priv->tracks;
    while (tmp != NULL)
    {
      OsmTrackRef* pNode = (OsmTrackRef*)tmp->data;
      if (pNode->m_nId == nId)
      {
        return pNode->track;
      }
      tmp = g_slist_next(tmp);
    }
  }
  return 0;
}

void osm_gps_map_add_track(OsmGpsMap* map, MaepGeodata* track, int nId, int nTrackType)
{
  OsmGpsMapPrivate* priv;
  OsmTrackRef* st;

  g_return_if_fail(OSM_IS_GPS_MAP(map) && track);
  priv = map->priv;

  g_object_ref(G_OBJECT(track));
  st = g_slice_new(OsmTrackRef);
  st->track = track;
  st->m_nId = nId;
  st->m_nTrackType = nTrackType;
  st->nwp_prop = g_signal_connect(G_OBJECT(track), "notify::n-waypoints",
                                  G_CALLBACK(_on_track_changed), (gpointer)map);
  st->iwp_prop = g_signal_connect(G_OBJECT(track), "notify::waypoint-highlight-index",
                                  G_CALLBACK(_on_track_changed), (gpointer)map);
  st->dirty_sig =
      g_signal_connect(G_OBJECT(track), "dirty", G_CALLBACK(_on_track_dirty), (gpointer)map);
  priv->tracks = g_slist_append(priv->tracks, st);
  if (!priv->idle_map_redraw)
    priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
}

void osm_gps_map_clear_tracks(OsmGpsMap* map)
{
  g_return_if_fail(OSM_IS_GPS_MAP(map));

  osm_gps_map_free_tracks(map);
  if (!map->priv->idle_map_redraw)
    map->priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
}

void osm_gps_map_clear_track(OsmGpsMap* map, int nId)
{
  g_return_if_fail(OSM_IS_GPS_MAP(map));

  osm_gps_map_free_track(map, nId);
  if (!map->priv->idle_map_redraw)
    map->priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
}

void osm_gps_map_add_image_with_alignment(OsmGpsMap* map, float latitude, float longitude,
                                          cairo_surface_t* image, float xalign, float yalign,
                                          const char* szName)
{
  g_return_if_fail(OSM_IS_GPS_MAP(map));

  if (image)
  {
    OsmGpsMapPrivate* priv = map->priv;
    image_t* im;

    // cache w/h for speed, and add image to list
    im = g_new0(image_t, 1);
    if (szName != 0)
      im->sz = g_strdup(szName);
    else
      im->sz = 0;

    im->w = cairo_image_surface_get_width(image);
    im->h = cairo_image_surface_get_height(image);
    im->pt.rlat = deg2rad(latitude);
    im->pt.rlon = deg2rad(longitude);

    // handle alignment
    im->xoffset = xalign * im->w;
    im->yoffset = yalign * im->h;

    cairo_surface_reference(image);
    im->image = image;

    priv->images = g_slist_append(priv->images, im);

    if (!priv->idle_map_redraw)
      priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
  }
}

void osm_gps_map_mark_image(OsmGpsMap* map, cairo_surface_t* image)
{
  OsmGpsMapPrivate* priv = map->priv;
  priv->cr_markedImage = image;
  if (image == 0)
  {
    osm_gps_map_idle_redraw(map);
    return;
  }

  if (!priv->idle_map_redraw)
    priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
}

void osm_gps_map_add_image(OsmGpsMap* map, float latitude, float longitude, cairo_surface_t* image)
{
  osm_gps_map_add_image_with_alignment(map, latitude, longitude, image, 0.5, 0.5, 0);
}

gboolean osm_gps_map_remove_image(OsmGpsMap* map, cairo_surface_t* image)
{
  OsmGpsMapPrivate* priv = map->priv;
  if (priv->images)
  {
    GSList* list;
    for (list = priv->images; list != NULL; list = list->next)
    {
      image_t* im = list->data;
      if (im->image == image)
      {
        priv->images = g_slist_remove_link(priv->images, list);
        cairo_surface_destroy(im->image);
        g_free(im->sz);
        g_free(im);
        if (!priv->idle_map_redraw)
          priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
        return TRUE;
      }
    }
  }
  return FALSE;
}

void osm_gps_map_rename_image(OsmGpsMap* map, cairo_surface_t* image, const char* sz)
{
  OsmGpsMapPrivate* priv = map->priv;
  if (priv->images)
  {
    GSList* list;
    for (list = priv->images; list != NULL; list = list->next)
    {
      image_t* im = list->data;
      if (im->image == image)
      {
        g_free(im->sz);
        im->sz = g_strdup(sz);
        if (!priv->idle_map_redraw)
          priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
        return;
      }
    }
  }
}

void osm_gps_map_clear_images(OsmGpsMap* map)
{
  g_return_if_fail(OSM_IS_GPS_MAP(map));

  osm_gps_map_free_images(map);
  if (!map->priv->idle_map_redraw)
    map->priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
}

void osm_gps_map_set_gps(OsmGpsMap* map, float latitude, float longitude, float heading)
{
  OsmGpsMapPrivate* priv;

  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;

  priv->osm_gps->rlat = deg2rad(latitude);
  priv->osm_gps->rlon = deg2rad(longitude);
  priv->osm_gps_heading = deg2rad(heading);

  // If trip marker add to list of gps points.
  /*
  if (priv->record_trip_history)
  {
    if (!priv->trip_history)
      priv->trip_history = maep_geodata_new();
    maep_geodata_add_trackpoint(priv->trip_history, latitude, longitude, FLT_MAX, NAN, NAN, NAN,
                                NAN);
  }

  */

  // dont draw anything if we are dragging
  /* g_error("implement here."); */
  /* if (priv->dragging) { */
  /*     g_debug("Dragging"); */
  /*     return; */
  /* } */

  // Automatically center the map if the track approaches the edge
  if (priv->map_auto_center)
  {
    // pixel_x,y, offsets
    int x, y;
    int width = priv->viewport_width;
    int height = priv->viewport_height;

    osm_gps_map_from_co_ordinates(map, priv->osm_gps, &x, &y);
    if (x < (width / 2 - width / 8) || x > (width / 2 + width / 8) ||
        y < (height / 2 - height / 8) || y > (height / 2 + height / 8))
    {
      if (_set_center(map, priv->osm_gps->rlat, priv->osm_gps->rlon))
        _update_screen_pos(map);
    }
  }

  // this redraws the map (including the gps track, and adjusts the
  // map center if it was changed
  if (!priv->idle_map_redraw && priv->osm_gps_valid)
    priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
}

void osm_gps_map_draw_gps(OsmGpsMap* map, gboolean status)
{
  OsmGpsMapPrivate* priv;

  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;

  priv->osm_gps_valid = status;

  if (!map->priv->idle_map_redraw)
    map->priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);
}

coord_t osm_gps_map_get_center_ordinates(OsmGpsMap* map)
{
  coord_t coord;
  OsmGpsMapPrivate* priv = map->priv;
  coord.rlon = priv->center_rlon;
  coord.rlat = priv->center_rlat;
  return coord;
}

coord_t osm_gps_map_get_co_ordinates(OsmGpsMap* map, int pixel_x, int pixel_y)
{
  coord_t coord;
  OsmGpsMapPrivate* priv = map->priv;

  coord.rlat =
      pixel2lat(priv->map_zoom,
                priv->map_y + (pixel_y + (priv->map_factor - 1.f) * priv->viewport_height * 0.5f) /
                                  priv->map_factor);
  coord.rlon = pixel2lon(priv->map_zoom, priv->map_x + (pixel_x + (priv->map_factor - 1.f) *
                                                                      priv->viewport_width * 0.5f) /
                                                           priv->map_factor);
  return coord;
}

void osm_gps_map_from_deg(OsmGpsMap* map, double logDeg, double latDeg, int* px, int* py)
{
  coord_t pos;
  pos.rlat = deg2rad(latDeg);
  pos.rlon = deg2rad(logDeg);

  osm_gps_map_from_co_ordinates(map, &pos, px, py);
}

void osm_gps_map_from_co_ordinates(OsmGpsMap* map, coord_t* coord, int* pixel_x, int* pixel_y)
{
  OsmGpsMapPrivate* priv;

  g_return_if_fail(OSM_IS_GPS_MAP(map) && coord);
  priv = map->priv;

  if (pixel_x)
    *pixel_x = priv->map_factor * (lon2pixel(priv->map_zoom, coord->rlon) - priv->map_x) -
               (priv->map_factor - 1.f) * priv->viewport_width * 0.5f;
  if (pixel_y)
    *pixel_y = priv->map_factor * (lat2pixel(priv->map_zoom, coord->rlat) - priv->map_y) -
               (priv->map_factor - 1.f) * priv->viewport_height * 0.5f;
}

OsmGpsMap* osm_gps_map_new(void)
{
  return g_object_new(OSM_TYPE_GPS_MAP, NULL);
}

void osm_gps_map_screen_to_geographic(OsmGpsMap* map, gint pixel_x, gint pixel_y, gfloat* latitude,
                                      gfloat* longitude)
{
  OsmGpsMapPrivate* priv;

  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;

  if (latitude)
    *latitude = rad2deg(pixel2lat(priv->map_zoom, priv->map_y + pixel_y - priv->drag_mouse_dy));
  if (longitude)
    *longitude = rad2deg(pixel2lon(priv->map_zoom, priv->map_x + pixel_x - priv->drag_mouse_dx));
}

void osm_gps_map_geographic_to_screen(OsmGpsMap* map, gfloat latitude, gfloat longitude,
                                      gint* pixel_x, gint* pixel_y)
{
  OsmGpsMapPrivate* priv;

  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;

  if (pixel_x)
    *pixel_x = lon2pixel(priv->map_zoom, deg2rad(longitude)) - priv->map_x;
  if (pixel_y)
    *pixel_y = lat2pixel(priv->map_zoom, deg2rad(latitude)) - priv->map_y;
}

void osm_gps_map_uppdate_offset(OsmGpsMap* map, int pixel_x, int pixel_y)
{
  OsmGpsMapPrivate* priv = priv = map->priv;
  priv->drag_mouse_dx = pixel_x;
  priv->drag_mouse_dy = pixel_y;
}

void osm_gps_map_get_offset(OsmGpsMap* map, int* pixel_x, int* pixel_y)
{
  OsmGpsMapPrivate* priv = priv = map->priv;
  *pixel_x = priv->drag_mouse_dx;
  *pixel_y = priv->drag_mouse_dy;
}

void osm_gps_map_scroll(OsmGpsMap* map)
{
  OsmGpsMapPrivate* priv;
  g_return_if_fail(OSM_IS_GPS_MAP(map));
  priv = map->priv;
  gint dx = -priv->drag_mouse_dx;
  gint dy = -priv->drag_mouse_dy;

  if (dx == 0 && dy == 0)
    return;

  /* g_message("scroll of %dx%d %g.", dx, dy, priv->map_factor); */
  priv->map_x += dx / priv->map_factor;
  priv->map_y += dy / priv->map_factor;
  center_coord_update(map);

  if (!priv->idle_map_redraw)
    priv->idle_map_redraw = g_idle_add((GSourceFunc)osm_gps_map_idle_redraw, map);

  g_object_notify_by_pspec(G_OBJECT(map), properties[PROP_MAP_X]);
  g_object_notify_by_pspec(G_OBJECT(map), properties[PROP_MAP_Y]);
}

float osm_gps_map_get_scale(OsmGpsMap* map)
{
  OsmGpsMapPrivate* priv;

  g_return_val_if_fail(OSM_IS_GPS_MAP(map), OSM_GPS_MAP_INVALID);
  priv = map->priv;

  return osm_gps_map_get_scale_at_lat(priv->map_zoom, priv->map_factor, priv->center_rlat);
}

cairo_surface_t* osm_gps_map_get_surface(OsmGpsMap* map)
{
  OsmGpsMapPrivate* priv;

  g_return_val_if_fail(OSM_IS_GPS_MAP(map), (cairo_surface_t*)0);
  priv = map->priv;

  cairo_surface_reference(priv->map_surf);
  cairo_surface_flush(priv->map_surf);
  return priv->map_surf;
}

char* osm_gps_map_get_default_cache_directory(void)
{
  return g_build_filename(g_get_user_cache_dir(), "osmgpsmap", NULL);
}

coord_t* osm_gps_map_get_gps(OsmGpsMap* map)
{
  g_return_val_if_fail(OSM_IS_GPS_MAP(map), NULL);

  if (!map->priv->osm_gps_valid)
    return NULL;

  return map->priv->osm_gps;
}
struct _OsmGpsMapSource
{
  guint id;
  gchar* name;
  gchar* repo_uri;
  gchar* image_format;
  gchar* copyright_notice;
  gchar* copyright_url;
  guint min_zoom;
  guint max_zoom;
};

#define UNUSED(x) (void)(x)
const OsmGpsMapSource* osm_gps_map_source_new(const gchar* name, const gchar repo_uri,
                                              const gchar* image_format,
                                              const gchar* copyright_notice,
                                              const gchar* copyright_url, guint min_zoom,
                                              guint max_zoom)
{
  UNUSED(name);
  UNUSED(repo_uri);
  UNUSED(image_format);
  UNUSED(copyright_notice);
  UNUSED(copyright_url);
  UNUSED(min_zoom);
  UNUSED(max_zoom);
  return 0;
}
