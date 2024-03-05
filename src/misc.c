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

#include <glib.h>
// #include <glib/gstdio.h>
#include <stdlib.h>

/* #include <gconf/gconf.h> */
/* #include <gconf/gconf-client.h> */

#include <dconf.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "misc.h"

#ifdef MAEMO5
#include <hildon/hildon-button.h>
#include <hildon/hildon-note.h>
#include <hildon/hildon-entry.h>
#include <hildon/hildon-pannable-area.h>
#include <mce/dbus-names.h>
#include <mce/mode-names.h>
#endif

#define GCONF_PATH         "/apps/" APP "/%s"
#define OLD_PATH         "/apps/maep/%s"

static DConfClient* dconfClient = NULL;
static DConfClient* dconf_client_get_default()
{
  if (!dconfClient)
    dconfClient = dconf_client_new();
  return dconfClient;
}

void gconf_set_string(const char *m_key, const char *str) {
  /* GConfClient *client = gconf_client_get_default(); */
  DConfClient *client = dconf_client_get_default();
  char *key = g_strdup_printf(GCONF_PATH, m_key);
  GError *error = NULL;
  GVariant *var;
  if(!str || !strlen(str))
    var = g_variant_ref_sink(g_variant_new_string(""));
  else
    var = g_variant_ref_sink(g_variant_new_string(str));
  dconf_client_write_sync(client, key, var, NULL, NULL, &error);
  /* gconf_client_set_float(client, key, value, NULL); */
  g_free(key);
  if (error) {
    g_warning("%s", error->message);
    g_error_free(error);
  }
  g_variant_unref(var);
}

char *gconf_get_string(const char *m_key) {
  /* GConfClient *client = gconf_client_get_default(); */
  DConfClient *client = dconf_client_get_default();

  char *key = g_strdup_printf(GCONF_PATH, m_key);
  /* GConfValue *value = gconf_client_get(client, key, NULL); */
  GVariant *value = dconf_client_read(client, key);
  if(!value) {
    g_free(key);
    key = g_strdup_printf(OLD_PATH, m_key);
    value = dconf_client_read(client, key);
    if (!value) {
      g_free(key);
      return NULL;
    }
  }  

  /* char *ret = gconf_client_get_string(client, key, NULL); */
  gsize len;
  char *ret = g_variant_dup_string(value, &len);
  g_free(key);
  g_variant_unref(value);
  return ret;
}

void gconf_set_bool(const char *m_key, gboolean value) {
  /* GConfClient *client = gconf_client_get_default(); */
  DConfClient *client = dconf_client_get_default();
  char *key = g_strdup_printf(GCONF_PATH, m_key);
  GError *error = NULL;
  GVariant *var = g_variant_ref_sink(g_variant_new_boolean(value));
  dconf_client_write_sync(client, key, var, NULL, NULL, &error);
  /* gconf_client_set_float(client, key, value, NULL); */
  g_free(key);
  if (error) {
    g_warning("%s", error->message);
    g_error_free(error);
  }
  g_variant_unref(var);
}

gboolean gconf_get_bool(const char *m_key, gboolean default_value) {
  /* GConfClient *client = gconf_client_get_default(); */
  DConfClient *client = dconf_client_get_default();

  char *key = g_strdup_printf(GCONF_PATH, m_key);
  /* GConfValue *value = gconf_client_get(client, key, NULL); */
  GVariant *value = dconf_client_read(client, key);
  if(!value) {
    key = g_strdup_printf(OLD_PATH, m_key);
    value = dconf_client_read(client, key);
    if (!value) {
      g_free(key);
      return default_value;
    }
  }

  if(!g_variant_is_of_type(value, G_VARIANT_TYPE_BOOLEAN)) {
    g_message("wrong type returning %d", default_value);
    g_free(key);
    g_variant_unref(value);
    return default_value;
  }
  /* gboolean ret = gconf_client_get_bool(client, key, NULL); */
  gboolean ret = g_variant_get_boolean(value);
  g_free(key);
  g_variant_unref(value);
  return ret;
}

void gconf_set_int(const char *m_key, gint value) {
  /* GConfClient *client = gconf_client_get_default(); */
  DConfClient *client = dconf_client_get_default();
  char *key = g_strdup_printf(GCONF_PATH, m_key);
  GError *error = NULL;
  GVariant *var = g_variant_ref_sink(g_variant_new_int32(value));
  dconf_client_write_sync(client, key, var, NULL, NULL, &error);
  /* gconf_client_set_float(client, key, value, NULL); */
  g_free(key);
  if (error) {
    g_warning("%s", error->message);
    g_error_free(error);
  }
  g_variant_unref(var);
}

gint gconf_get_int(const char *m_key, gint def_value) {
  /* GConfClient *client = gconf_client_get_default(); */
  DConfClient *client = dconf_client_get_default();

  char *key = g_strdup_printf(GCONF_PATH, m_key);
  /* GConfValue *value = gconf_client_get(client, key, NULL); */
  GVariant *value = dconf_client_read(client, key);
  if(!value) {
    key = g_strdup_printf(OLD_PATH, m_key);
    value = dconf_client_read(client, key);
    if (!value) {
      g_free(key);
      return def_value;
    }
  }

  /* gint ret = gconf_client_get_int(client, key, NULL); */
  if(!g_variant_is_of_type(value, G_VARIANT_TYPE_INT32)) {
    g_message("wrong type returning %d", def_value);
    g_free(key);
    g_variant_unref(value);
    return def_value;
  }
  gint ret = g_variant_get_int32(value);
  g_free(key);
  g_variant_unref(value);
  return ret;
}

void gconf_set_float(const char *m_key, gfloat value) {
  /* GConfClient *client = gconf_client_get_default(); */
  DConfClient *client = dconf_client_get_default();
  char *key = g_strdup_printf(GCONF_PATH, m_key);
  GError *error = NULL;
  GVariant *var = g_variant_ref_sink(g_variant_new_double(value));
  dconf_client_write_sync(client, key, var, NULL, NULL, &error);
  /* gconf_client_set_float(client, key, value, NULL); */
  g_free(key);
  if (error) {
    g_warning("%s", error->message);
    g_error_free(error);
  }
  g_variant_unref(var);
}

gfloat gconf_get_float(const char *m_key, gfloat def_value) {
  /* GConfClient *client = gconf_client_get_default(); */
  DConfClient *client = dconf_client_get_default();

  char *key = g_strdup_printf(GCONF_PATH, m_key);
  /* GConfValue *value = gconf_client_get(client, key, NULL); */
  GVariant *value = dconf_client_read(client, key);
  if(!value) {
    key = g_strdup_printf(OLD_PATH, m_key);
    value = dconf_client_read(client, key);
    if (!value) {
      g_free(key);
      return def_value;
    }
  }

  if(!g_variant_is_of_type(value, G_VARIANT_TYPE_DOUBLE)) {
    g_message("wrong type returning %g", def_value);
    g_free(key);
    g_variant_unref(value);
    return def_value;
  }
  /* gfloat ret = gconf_client_get_float(client, key, NULL); */
  gfloat ret = g_variant_get_double(value);
  g_free(key);
  g_variant_unref(value);
  return ret;
}


struct proxy_config *proxy_config_get()
{
    struct proxy_config *config = g_new0(struct proxy_config, 1);

    // TODO: As fallback, get proxy from environment ("http_proxy")
    // TODO: On Sailfish, get proxy settings from Qt or ConnMan(?)

#define PROXY_KEY  "/system/http_proxy/"
    if (gconf_get_bool(PROXY_KEY "use_http_proxy", FALSE)) {
        g_message("thread: using proxy.");

        /* basic settings */
        config->host = gconf_get_string(PROXY_KEY "host");
        config->port = gconf_get_int(PROXY_KEY "port", 0);

        /* authentication settings */
        if(gconf_get_bool(PROXY_KEY "use_authentication", FALSE)) {
            config->username = gconf_get_string(PROXY_KEY "authentication_user");
            config->password = gconf_get_string(PROXY_KEY "authentication_password");
        }
    }
#undef PROXY_KEY

    return config;
}

void proxy_config_free(struct proxy_config *config)
{
    if (config) {
        g_free(config->host);
        g_free(config->username);
        g_free(config->password);
        g_free(config);
    }
}

//  "~/" APP,                 // in home directory
static const char *data_paths[] = {
  DATADIR ,                  // final installation path (e.g. /usr/share/maep)
#ifdef USE_MAEMO
  "/media/mmc1/" APP,        // path to external memory card
  "/media/mmc2/" APP,        // path to internal memory card
#endif
  "./data", "../data",       // local paths for testing
  NULL
};

char *find_file(const char *name) {
  const char **path = data_paths;
  char *p = getenv("HOME");

  while(*path) {
    char *full_path = NULL;

    if(*path[0] == '~')
      full_path = g_strdup_printf("%s/%s/%s", p, *path+2, name);
    else
      full_path = g_strdup_printf("%s/%s", *path, name);


 //    g_message(full_path);
    if(g_file_test(full_path, G_FILE_TEST_IS_REGULAR))
      return full_path;

    g_free(full_path);
    path++;
  }

  return NULL;
}

/* Converts an integer value to its hex character*/
static char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(const char *str) {
  const char *pstr = str;
  char *buf = g_malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || 
	*pstr == '.' || *pstr == '~') 
      *pbuf++ = *pstr;
    else if (*pstr == ' ') 
      *pbuf++ = '+';
    else 
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), 
	*pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}

