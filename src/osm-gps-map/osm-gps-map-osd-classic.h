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

#ifndef _OSM_GPS_MAP_OSD_CLASSIC_H_
#define _OSM_GPS_MAP_OSD_CLASSIC_H_

#include "osm-gps-map.h"

G_BEGIN_DECLS

typedef enum {
    OSD_NONE = 0,
    OSD_BG,
    OSD_UP,
    OSD_DOWN,
    OSD_LEFT,
    OSD_RIGHT,
    OSD_IN,
    OSD_OUT,
    OSD_CUSTOM   // first custom buttom
} osd_button_t;


/* the osd structure mainly contains various callbacks */
typedef struct osm_gps_map_osd_s {
    OsmGpsMap *map;
    void(*draw)(struct osm_gps_map_osd_s *, cairo_t *);
    gpointer priv;
} osm_gps_map_osd_t;

osm_gps_map_osd_t* osm_gps_map_osd_classic_init(OsmGpsMap *map);

void osm_gps_map_osd_classic_free(osm_gps_map_osd_t *osd);
void osm_gps_map_set_azimuth(osm_gps_map_osd_t *osd, double azimuth);
void osd_render_scale_and_compass(osm_gps_map_osd_t *osd) ;



G_END_DECLS

#endif
