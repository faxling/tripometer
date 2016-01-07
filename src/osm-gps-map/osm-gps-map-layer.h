/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/* vim:set et sw=4 ts=4 cino=t0,(0: */
/*
 * osm-gps-map-layer.h
 * Copyright (C) Marcus Bauer 2008 <marcus.bauer@gmail.com>
 * Copyright (C) John Stowers 2009 <john.stowers@gmail.com>
 * Copyright (C) Till Harbaum 2009 <till@harbaum.org>
 *
 * Contributions by
 * Everaldo Canuto 2009 <everaldo.canuto@gmail.com>
 *
 * This is free software: you can redistribute it and/or modify it
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

#ifndef _OSM_GPS_MAP_LAYER_H_
#define _OSM_GPS_MAP_LAYER_H_

#include <glib.h>
#include <glib-object.h>
#include <cairo.h>

G_BEGIN_DECLS

#define OSM_TYPE_GPS_MAP_LAYER                  (osm_gps_map_layer_get_type ())
#define OSM_GPS_MAP_LAYER(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), OSM_TYPE_GPS_MAP_LAYER, OsmGpsMapLayer))
#define OSM_GPS_MAP_IS_LAYER(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), OSM_TYPE_GPS_MAP_LAYER))
#define OSM_GPS_MAP_LAYER_GET_INTERFACE(inst)   (G_TYPE_INSTANCE_GET_INTERFACE ((inst), OSM_TYPE_GPS_MAP_LAYER, OsmGpsMapLayerIface))

typedef struct _OsmGpsMapLayer          OsmGpsMapLayer;             /* dummy object */
typedef struct _OsmGpsMapLayerIface     OsmGpsMapLayerIface;

#include "osm-gps-map.h"

struct _OsmGpsMapLayerIface {
    GTypeInterface parent;

    void (*render) (OsmGpsMapLayer *self, OsmGpsMap *map);
    void (*draw) (OsmGpsMapLayer *self, cairo_t *cr, OsmGpsMap *map);
    gboolean (*busy) (OsmGpsMapLayer *self);
    gboolean (*button) (OsmGpsMapLayer *self, int x, int y, gboolean press);
};

GType osm_gps_map_layer_get_type (void);

void        osm_gps_map_layer_render (OsmGpsMapLayer *self, OsmGpsMap *map);
void        osm_gps_map_layer_draw   (OsmGpsMapLayer *self, cairo_t *cr,
                                      OsmGpsMap *map);
gboolean    osm_gps_map_layer_busy   (OsmGpsMapLayer *self);
gboolean    osm_gps_map_layer_button (OsmGpsMapLayer *self, int x, int y,
                                      gboolean press);

G_END_DECLS

#endif /* _OSM_GPS_MAP_LAYER_H_ */
