/*
 * Copyright (C) 2009 Till Harbaum <till@harbaum.org>.
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

#ifndef MISC_H
#define MISC_H



#include <glib.h>

#if __has_include("../lib/glib-2.0/include/glibconfig.h")
#include <../lib/glib-2.0/include/glibconfig.h>
#endif

G_BEGIN_DECLS

char *find_file(const char *name);

void gconf_set_string(const char *key, const char *str);
char *gconf_get_string(const char *key);
void gconf_set_bool(const char *key, gboolean value);
gboolean gconf_get_bool(const char *key, gboolean default_value);
void gconf_set_int(const char *key, gint value);
gint gconf_get_int(const char *key, gint default_value);
void gconf_set_float(const char *key, gfloat value);
gfloat gconf_get_float(const char *key, gfloat default_value);

char *url_encode(const char *str);

struct proxy_config {
    // Host is NULL if no proxy should be used
    gchar *host;
    int port;

    // Username is NULL if no auth should be used
    gchar *username;
    gchar *password;
};

struct proxy_config *proxy_config_get();
void proxy_config_free(struct proxy_config *config);



G_END_DECLS

#endif // MISC_H
