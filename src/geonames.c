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

static NominatimPlace* nominatim_parse_place(xmlDocPtr doc, xmlNode* a_node)
{
  xmlAttr* attr;
  xmlChar* value;
  NominatimPlace* geoname = g_new0(NominatimPlace, 1);

  geoname->pos.rlat = geoname->pos.rlon = OSM_GPS_MAP_INVALID;

  for (attr = a_node->properties; attr; attr = attr->next)
  {
    if (attr->name == 0 || attr->children == 0)
      continue;
    if (!strcmp((char*)attr->name, "lat"))
    {
      value = xmlNodeListGetString(doc, attr->children, 1);
      geoname->pos.rlat = deg2rad(g_ascii_strtod((gchar*)value, NULL));
      xmlFree(value);
    }
    else if (!strcmp((char*)attr->name, "lon"))
    {
      value = xmlNodeListGetString(doc, attr->children, 1);
      geoname->pos.rlon = deg2rad(g_ascii_strtod((gchar*)value, NULL));
      xmlFree(value);
    }
    else if ( !strcmp((char*)attr->name, "display_name"))
    {
      value = xmlNodeListGetString(doc, attr->children, 1);
      geoname->name = g_strdup((gchar*)value);
      xmlFree(value);
    }
    else if (!strcmp((char*)attr->name, "type") )
    {
      value = xmlNodeListGetString(doc, attr->children, 1);
      geoname->type = g_strdup((gchar*)value);
      xmlFree(value);
    }
    else if (!strcmp((char*)attr->name, "ref"))
    {
      value = xmlNodeListGetString(doc, attr->children, 1);
      geoname->ref = g_strdup((gchar*)value);
      xmlFree(value);
    }
  }
  return geoname;
}


static GSList* geonames_parse_nominatim(xmlDocPtr doc, xmlNode* a_node)
{
  GSList* list = NULL;
  xmlNode* cur_node = NULL;

  for (cur_node = a_node->children; cur_node; cur_node = cur_node->next)
  {
    if (cur_node->type == XML_ELEMENT_NODE)
    {
      if (strcasecmp((char*)cur_node->name, "place") == 0)
      {
        list = g_slist_append(list, nominatim_parse_place(doc, cur_node));
      }
    }
  }

  return list;
}


static GSList* geonames_parse_root(xmlDocPtr doc, xmlNode* a_node)
{
  GSList* list = NULL;
  xmlNode* cur_node = NULL;

  for (cur_node = a_node; cur_node; cur_node = cur_node->next)
  {
    if (cur_node->type == XML_ELEMENT_NODE)
    {
      if (strcasecmp((char*)cur_node->name, "searchresults") == 0)
        list = geonames_parse_nominatim(doc, cur_node);
    }
  }
  return list;
}

static GSList* geonames_parse_doc(xmlDocPtr doc)
{

  xmlNode* root_element = xmlDocGetRootElement(doc);
  GSList* list = geonames_parse_root(doc, root_element);

  xmlFreeDoc(doc);

  return list;
}

void nominatim_place_free(NominatimPlace* geoname, gpointer* p)
{
  UNUSED(p);
  if (geoname->name)
    g_free(geoname->name);
  if (geoname->type)
    g_free(geoname->type);
  if (geoname->ref)
    g_free(geoname->ref);
  g_free(geoname);
}

void nominatim_place_list_free(GSList* list)
{
  g_slist_foreach(list, (GFunc)nominatim_place_free, NULL);
  g_slist_free(list);
}


#define MAX_RESULT 30
#define NOMINATIM "https://nominatim.openstreetmap.org/"
typedef struct
{
  NominatimRequestCallback cb;
  gpointer obj;
} request_cb_t;

static void nominatim_request_cb(net_result_t* result, gpointer data)
{
  GError* err;
  request_cb_t* context = (request_cb_t*)data;

  g_return_if_fail(context && context->cb);

  if (!result->code)
  {
    xmlDoc* doc = NULL;

    LIBXML_TEST_VERSION;

    if ((doc = xmlReadMemory(result->data.ptr, result->data.len, NULL, NULL, 0)) == NULL)
    {
      const xmlError* errP = xmlGetLastError();
      err = g_error_new(MAEP_NET_IO_ERROR, 0, "While parsing:\n%s", errP->message);
      context->cb(context->obj, NULL, err);
      g_error_free(err);
    }
    else
    {
      // Here is the main thing with the result
      // geonames_parse_doc -> eonames_parse_root -> geonames_parse_nominatim -> nominatim_parse_place
      GSList* list = geonames_parse_doc(doc);
      // the callback just sends a signal
      context->cb(context->obj, list, (GError*)0);
    }
  }
  else
  {
    err = g_error_new(MAEP_NET_IO_ERROR, 0, "Nominatim download failed!");
    context->cb(context->obj, NULL, err);
    g_error_free(err);
  }
  g_free(context);
}

void nominatim_address_request(const gchar* request, NominatimRequestCallback cb, gpointer obj)
{

  request_cb_t* context;

  /* gconf_set_string("search_text", phrase); */

  gchar *locale, lang[3] = {0, 0, 0};
  locale = setlocale(LC_MESSAGES, NULL);
  g_utf8_strncpy(lang, locale, 2);

  /* build search request */
  char* encoded_phrase = url_encode(request);
  char* url =
      g_strdup_printf(NOMINATIM "search?q=%s&format=xml&limit=%u", encoded_phrase, MAX_RESULT);
  g_free(encoded_phrase);

  /* request search results asynchronously */
  g_message("start asynchronous nominatim download (%s).", url);
  context = g_malloc0(sizeof(request_cb_t));
  context->cb = cb;
  context->obj = obj;
  net_io_download_async(url, nominatim_request_cb, context, 0);

  g_free(url);
}
