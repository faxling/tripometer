/*
 * Copyright (C) 2009 Till Harbaum <till@harbaum.org>.
 *
 * Contributor: Damien Caliste 2013-2014 <dcaliste@free.fr>
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

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <locale.h>
#include <string.h>

#include "config.h"
#include "geonames.h"
#include "misc.h"
#include "net_io.h"

#ifndef LIBXML_TREE_ENABLED
#error "Tree not enabled in libxml"
#endif

/* -------------- begin of xml parser ---------------- */

static gboolean string_get(xmlNode* node, char* name, char** dst)
{
  if (*dst)
    return FALSE; /* don't overwrite anything */

  if (strcasecmp((char*)node->name, name) != 0)
    return FALSE;

  char* str = (char*)xmlNodeGetContent(node);
  *dst = g_strdup(str);
  xmlFree(str);

  return TRUE;
}

static MaepGeonamesPlace* nominatim_parse_place(xmlDocPtr doc, xmlNode* a_node)
{
  xmlAttr* attr;
  xmlChar* value;
  MaepGeonamesPlace* geoname = g_new0(MaepGeonamesPlace, 1);

  geoname->pos.rlat = geoname->pos.rlon = OSM_GPS_MAP_INVALID;

  for (attr = a_node->properties; attr; attr = attr->next)
    if (attr->name && !strcmp((char*)attr->name, "lat") && attr->children)
    {
      value = xmlNodeListGetString(doc, attr->children, 1);
      geoname->pos.rlat = deg2rad(g_ascii_strtod((gchar*)value, NULL));
      xmlFree(value);
    }
    else if (attr->name && !strcmp((char*)attr->name, "lon") && attr->children)
    {
      value = xmlNodeListGetString(doc, attr->children, 1);
      geoname->pos.rlon = deg2rad(g_ascii_strtod((gchar*)value, NULL));
      xmlFree(value);
    }
    else if (attr->name && !strcmp((char*)attr->name, "display_name") && attr->children)
    {
      value = xmlNodeListGetString(doc, attr->children, 1);
      geoname->name = g_strdup((gchar*)value);
      xmlFree(value);
    }
    else if (attr->name && !strcmp((char*)attr->name, "type") && attr->children)
    {
      value = xmlNodeListGetString(doc, attr->children, 1);
      geoname->country = g_strdup((gchar*)value);
      xmlFree(value);
    }
  return geoname;
}

static MaepGeonamesPlace* geonames_parse_geoname(G_GNUC_UNUSED xmlDocPtr doc, xmlNode* a_node)
{
  xmlNode* cur_node = NULL;
  MaepGeonamesPlace* geoname = g_new0(MaepGeonamesPlace, 1);
  geoname->pos.rlat = geoname->pos.rlon = OSM_GPS_MAP_INVALID;

  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next)
  {
    if (cur_node->type == XML_ELEMENT_NODE)
    {

      string_get(cur_node, "name", &geoname->name);
      string_get(cur_node, "countryName", &geoname->country);

      if (strcasecmp((char*)cur_node->name, "lat") == 0)
      {
        char* str = (char*)xmlNodeGetContent(cur_node);
        geoname->pos.rlat = deg2rad(g_ascii_strtod(str, NULL));
        xmlFree(str);
      }
      else if (strcasecmp((char*)cur_node->name, "lng") == 0)
      {
        char* str = (char*)xmlNodeGetContent(cur_node);
        geoname->pos.rlon = deg2rad(g_ascii_strtod(str, NULL));
        xmlFree(str);
      }
    }
  }

  return geoname;
}

static MaepGeonamesEntry* geonames_parse_entry(G_GNUC_UNUSED xmlDocPtr doc, xmlNode* a_node)
{
  xmlNode* cur_node = NULL;
  MaepGeonamesEntry* entry = g_new0(MaepGeonamesEntry, 1);
  entry->pos.rlat = entry->pos.rlon = OSM_GPS_MAP_INVALID;

  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next)
  {
    if (cur_node->type == XML_ELEMENT_NODE)
    {

      string_get(cur_node, "title", &entry->title);
      string_get(cur_node, "summary", &entry->summary);
      string_get(cur_node, "thumbnailImg", &entry->thumbnail_url);
      string_get(cur_node, "wikipediaUrl", &entry->url);

      if (strcasecmp((char*)cur_node->name, "lat") == 0)
      {
        char* str = (char*)xmlNodeGetContent(cur_node);
        entry->pos.rlat = deg2rad(g_ascii_strtod(str, NULL));
        xmlFree(str);
      }
      else if (strcasecmp((char*)cur_node->name, "lng") == 0)
      {
        char* str = (char*)xmlNodeGetContent(cur_node);
        entry->pos.rlon = deg2rad(g_ascii_strtod(str, NULL));
        xmlFree(str);
      }
    }
  }

  if (entry->thumbnail_url && strlen(entry->thumbnail_url) == 0)
  {
    g_free(entry->thumbnail_url);
    entry->thumbnail_url = NULL;
  }

  return entry;
}

static GSList* geonames_parse_geonames(xmlDocPtr doc, xmlNode* a_node)
{
  GSList* list = NULL;
  xmlNode* cur_node = NULL;

  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next)
  {
    if (cur_node->type == XML_ELEMENT_NODE)
    {
      if (strcasecmp((char*)cur_node->name, "geoname") == 0)
      {
        list = g_slist_append(list, geonames_parse_geoname(doc, cur_node));
      }
      else if (strcasecmp((char*)cur_node->name, "entry") == 0)
      {
        list = g_slist_append(list, geonames_parse_entry(doc, cur_node));
      }
      else if (strcasecmp((char*)cur_node->name, "place") == 0)
      {
        list = g_slist_append(list, nominatim_parse_place(doc, cur_node));
      }
    }
  }
  return list;
}

/* parse root element and search for "track" */
static GSList* geonames_parse_root(xmlDocPtr doc, xmlNode* a_node)
{
  GSList* list = NULL;
  xmlNode* cur_node = NULL;

  for (cur_node = a_node; cur_node; cur_node = cur_node->next)
  {
    if (cur_node->type == XML_ELEMENT_NODE)
    {
      if (!list && (strcasecmp((char*)cur_node->name, "geonames") == 0 ||
                    strcasecmp((char*)cur_node->name, "searchresults") == 0))
        list = geonames_parse_geonames(doc, cur_node);
    }
  }
  return list;
}

static GSList* geonames_parse_doc(xmlDocPtr doc)
{
  /* Get the root element node */
  xmlNode* root_element = xmlDocGetRootElement(doc);

  GSList* list = geonames_parse_root(doc, root_element);

  xmlFreeDoc(doc);

  /* xmlCleanupParser(); */

  return list;
}

/* -------------- end of xml parser ---------------- */

/* ------------- begin of freeing ------------------ */

void maep_geonames_place_free(MaepGeonamesPlace* geoname)
{
  if (geoname->name)
    g_free(geoname->name);
  if (geoname->country)
    g_free(geoname->country);
  g_free(geoname);
}

void maep_geonames_place_list_free(GSList* list)
{
  g_slist_foreach(list, (GFunc)maep_geonames_place_free, NULL);
  g_slist_free(list);
}

MaepGeonamesEntry* maep_geonames_entry_copy(MaepGeonamesEntry* src)
{
  MaepGeonamesEntry* entry = g_memdup2(src, sizeof(MaepGeonamesEntry));

  entry->title = g_strdup(src->title);
  entry->summary = g_strdup(src->summary);
  entry->thumbnail_url = g_strdup(src->thumbnail_url);
  entry->url = g_strdup(src->url);
  return entry;
}

void maep_geonames_entry_free( gpointer       data,
                               gpointer       user_data)
{

  UNUSED(user_data);
  MaepGeonamesEntry* entry = data;
  if (entry->title)
    g_free(entry->title);
  if (entry->summary)
    g_free(entry->summary);
  if (entry->thumbnail_url)
    g_free(entry->thumbnail_url);
  if (entry->url)
    g_free(entry->url);
  g_free(entry);
}

void maep_geonames_entry_list_free(GSList* list)
{
  g_slist_foreach(list, (GFunc)maep_geonames_entry_free, NULL);
  g_slist_free(list);
}

/* ------------- end of freeing ------------------ */

#define MAX_RESULT 30
#define GEONAMES "http://api.geonames.org/"
#define GEONAMES_SEARCH "geonames_search"

typedef struct
{
  MaepGeonamesRequestCallback cb;
  gpointer obj;
} request_cb_t;

static void geonames_request_cb(net_result_t* result, gpointer data)
{
  GError* err;
  request_cb_t* context = (request_cb_t*)data;

  g_message("asynchronous request callback.");
  g_return_if_fail(context && context->cb);

  if (!result->code)
  {
    /* feed this into the xml parser */
    xmlDoc* doc = NULL;

    LIBXML_TEST_VERSION;


    /* parse the file and get the DOM */
    if ((doc = xmlReadMemory(result->data.ptr, result->data.len, NULL, NULL, 0)) == NULL)
    {
      const xmlError* errP = xmlGetLastError();
      err = g_error_new(MAEP_NET_IO_ERROR, 0, "While parsing:\n%s", errP->message);
      context->cb(context->obj, NULL, err);
      g_error_free(err);
    }
    else
    {
      g_message("XML parsed without error");
      GSList* list = geonames_parse_doc(doc);

      context->cb(context->obj, list, (GError*)0);
    }
  }
  else
  {
    err = g_error_new(MAEP_NET_IO_ERROR, 0, "Geonames download failed!");
    context->cb(context->obj, NULL, err);
    g_error_free(err);
  }
  g_free(context);
}


void maep_geonames_place_request(const gchar* request, MaepGeonamesRequestCallback cb, gpointer obj)
{
  request_cb_t* context;

  /* gconf_set_string("search_text", phrase); */

  gchar *locale, lang[3] = {0, 0, 0};
  locale = setlocale(LC_MESSAGES, NULL);
  g_utf8_strncpy(lang, locale, 2);

  /* build search request */
  char* encoded_phrase = url_encode(request);
  char* url = g_strdup_printf(GEONAMES "search?q=%s&maxRows=%u&lang=%s"
                                       "&isNameRequired=1&featureClass=P&username=" PACKAGE,

                              encoded_phrase, MAX_RESULT, lang);
  g_free(encoded_phrase);

  /* request search results asynchronously */
  g_message("start asynchronous place download (%s).", url);
  context = g_malloc0(sizeof(request_cb_t));
  context->cb = cb;
  context->obj = obj;
  net_io_download_async(url, geonames_request_cb, context, 0);

  g_free(url);
}

#define NOMINATIM "https://nominatim.openstreetmap.org/"
void maep_nominatim_address_request(const gchar* request, MaepGeonamesRequestCallback cb,
                                    gpointer obj)
{

  request_cb_t* context;

  /* gconf_set_string("search_text", phrase); */

  gchar *locale, lang[3] = {0, 0, 0};
  locale = setlocale(LC_MESSAGES, NULL);
  g_utf8_strncpy(lang, locale, 2);

  /* build search request */
  char* encoded_phrase = url_encode(request);
  char* url =
      g_strdup_printf(NOMINATIM "search.php?q=%s&format=xml&limit=%u", encoded_phrase, MAX_RESULT);
  g_free(encoded_phrase);

  /* request search results asynchronously */
  g_message("start asynchronous nominatim download (%s).", url);
  context = g_malloc0(sizeof(request_cb_t));
  context->cb = cb;
  context->obj = obj;
  net_io_download_async(url, geonames_request_cb, context, 0);

  g_free(url);
}
