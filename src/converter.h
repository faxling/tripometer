/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/* vim:set et sw=4 ts=4 cino=t0,(0: */
/*
 * converter.h
 * Copyright (C) Marcus Bauer 2008 <marcus.bauer@gmail.com>
 * Copyright (C) John Stowers 2009 <john.stowers@gmail.com>
 *
 * Contributions by
 * Everaldo Canuto 2009 <everaldo.canuto@gmail.com>
 *
 * osm-gps-map.c is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * osm-gps-map.c is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONVERTER_H
#define CONVERTER_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
    float rlat;
    float rlon;
} coord_t;

#define OSM_GPS_MAP_INVALID         (0.0/0.0)

float
deg2rad(float deg);

float
rad2deg(float rad);

int
lat2pixel(  int zoom,
            float lat);

int
lon2pixel(  int zoom,
            float lon);

float
pixel2lon(  int zoom,
            int pixel_x);

float
pixel2lat(  int zoom,
            int pixel_y);

G_END_DECLS

#endif
