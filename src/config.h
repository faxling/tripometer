/*
 * Copyright (C) 2008-2009 Till Harbaum <till@harbaum.org>.
 *
 * This file is part of Maep.
 *
 * Maep is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Maep is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Maep.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef CONFIG_H
#define CONFIG_H
#ifndef SAILFISH
#define SAILFISH
#endif

/*#include <gtk/gtk.h>*/

#include <locale.h>
#include <libintl.h>

#define LOCALEDIR PREFIX "/share/locale"
#define PACKAGE   "maep"

#define _(String) gettext(String)
#define N_(String) (String)

/* map configuration: */

#define OSD_SCALE

#define OSD_COORDINATES
#undef OSD_NAV

#define MAP_DRAG_LIMIT      (10)
#define MAP_KEY_FULLSCREEN  GDK_F11
#define MAP_KEY_ZOOMIN      '+'
#define MAP_KEY_ZOOMOUT     '-'

#define MAP_KEY_UP          GDK_Up
#define MAP_KEY_DOWN        GDK_Down
#define MAP_KEY_LEFT        GDK_Left
#define MAP_KEY_RIGHT       GDK_Right


#ifdef SAILFISH
#define THUMB_OSD
#define HIGH_DPI
#endif

/* specify OSD colors explicitely. Otherwise gtk default */
/* colors are used. fremantle always uses gtk defaults */

#define OSD_COLOR            1, 1, 1         // white
#define OSD_COLOR_BG         0, 0, 0, 0.5    // transparent dark background
#define OSD_COLOR_DISABLED   0.5, 0.5, 0.5   // grey disabled controls
/* fremantle has controls at botton (for fringer friendlyness) */
// #define OSD_Y  -10
#define OSD_HR_Y 60    // HR is always at screens top

#ifdef HIGH_DPI
#define OSD_FONT_SIZE             (28.0)
#define OSD_DIAMETER              (60)
#define OSD_SCALE_FONT_SIZE       (20.0)
#define OSD_COORDINATES_FONT_SIZE (30.0)
#endif

#define WITH_QT

/* #define OSD_DOUBLE_BUFFER */    // render osd/map together
                                   // offscreen
/* #define OSD_CONTROLS */      // display a zooming plus other button
                                // control panel on the bottom left.
// #define OSD_GPS_BUTTON       // display a GPS button
// #define OSD_NO_DPAD          // no direction arrows (map is panned)
/* #define OSD_SOURCE_SEL */       // display source selection tab
// #define OSD_BALLOON
/* #define OSD_DOUBLEPIXEL */      // allow pixel doubling from OSD
// #define OSD_HEARTRATE
#define GCONF_KEY_WEATHER "weather"
#define GCONF_KEY_CROSSHAIR "cross-hair"
#define GCONF_KEY_COMPASS_ENABLED "compass-enabled"

#endif // CONFIG_H
