/*
 * Copyright (C) 2010 Till Harbaum <till@harbaum.org>.
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

#include "config.h"

#include "misc.h"
#include "net_io.h"

#include <curl/curl.h>
/*#include <curl/types.h>*/ /* new for v7 */
#include <curl/easy.h>      /* new for v7 */
#include <string.h>
#include <unistd.h>

static GQuark error_quark = 0;
GQuark net_io_get_quark()
{
  if (!error_quark)
    error_quark = g_quark_from_static_string("MAEP_NET_IO");
  return error_quark;
}

static struct http_message_s
{
  int id;
  char* msg;
} http_messages[] = {{0, "Curl internal failure"},
                     {200, "Ok"},
                     {301, "Moved permanently"},
                     {302, "Found"},
                     {303, "See Other"},
                     {400, "Bad Request (area too big?)"},
                     {401, "Unauthorized (wrong user/password?)"},
                     {403, "Forbidden"},
                     {404, "Not Found"},
                     {405, "Method Not Allowed"},
                     {410, "Gone"},
                     {412, "Precondition Failed"},
                     {417, "Expectation failed (expect rejected)"},
                     {500, "Internal Server Error"},
                     {503, "Service Unavailable"},
                     {0, NULL}};

/* structure shared between worker and master thread */
typedef struct
{
  //  gint refcount; /* reference counter for master and worker thread */
  struct curl_slist* chunk;
  struct proxy_config* proxy;
  char* url;
  char* user;
  // gboolean cancel;
  // float progress;

  /* curl/http related stuff: */
  CURLcode res;
  long response;
  char buffer[CURL_ERROR_SIZE];

  net_io_cb cb;
  gpointer data;

  net_result_t result;

} net_io_request_t;

static char* http_message(int id)
{
  struct http_message_s* msg = http_messages;

  while (msg->msg)
  {
    if (msg->id == id)
      return _(msg->msg);
    msg++;
  }

  return NULL;
}

void net_io_init()
{
  curl_global_init(CURL_GLOBAL_NOTHING);
}
void net_io_finalize()
{
  curl_global_cleanup();
}

static void request_free(net_io_request_t* request)
{
  if (request->proxy)
    proxy_config_free(request->proxy);
  if (request->url)
    g_free(request->url);
  if (request->user)
    g_free(request->user);

  if (request->result.data.ptr)
  {
    g_free(request->result.data.ptr);
    request->result.data.ptr = 0;
    request->result.data.len = 0;
  }

  g_free(request);
}

static size_t mem_write(void* ptr, size_t size, size_t nmemb, void* stream)
{
  net_mem_t* p = (net_mem_t*)stream;

  p->ptr = g_realloc(p->ptr, p->len + size * nmemb + 1);
  if (p->ptr)
  {
    memcpy(p->ptr + p->len, ptr, size * nmemb);
    p->len += size * nmemb;
    p->ptr[p->len] = 0;
  }
  return nmemb;
}

static void set_proxy(CURL* curl, const struct proxy_config* config)
{
  if (config->host)
  {
    curl_easy_setopt(curl, CURLOPT_PROXY, config->host);
    curl_easy_setopt(curl, CURLOPT_PROXYPORT, config->port);

    if (config->username)
    {
      char* cred = g_strdup_printf("%s:%s", config->username, config->password);
      curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, cred);
      g_free(cred);
    }
  }
}

// In Main thread
static gboolean net_io_idle_cb(gpointer data)
{
  net_io_request_t* request = (net_io_request_t*)data;
  request->result.respCode = request->response;
  /* the http connection itself may have failed */
  if (request->res != 0)
  {
    request->result.code = 2;
    //  printf("Download failed with message: %s\n", request->buffer);
  }
  else if (request->response != 200)
  {
    /* a valid http connection may have returned an error */
    request->result.code = 3;
    printf("Download failed with code %ld: %s\n", request->response,
           http_message(request->response));
  }

  /* call application callback */
  if (request->cb)
    request->cb(&request->result, request->data);

  request_free(request);

  return FALSE;
}

int g_nOutstaningCurls = 0;

static void* worker_thread(void* ptr)
{
  net_io_request_t* request = (net_io_request_t*)ptr;

  CURL* curl = curl_easy_init();
  if (curl == NULL)
  {
    g_thread_unref(g_thread_self());
    return NULL;
  }
  static GMutex mutex;
  g_mutex_lock (&mutex);
  ++g_nOutstaningCurls;
  g_mutex_unlock (&mutex);
  request->result.data.ptr = NULL;
  request->result.data.len = 0;
  request->result.respCode = -1;

  /* In case Curl is causing stack fault on long jumps. */
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

  curl_easy_setopt(curl, CURLOPT_URL, request->url);

  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &request->result.data);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mem_write);

  /* g_message("thread: set proxy"); */
  set_proxy(curl, request->proxy);

  /* set user name and password for the authentication */
  // g_message("thread: set username if any");
  if (request->user)
    curl_easy_setopt(curl, CURLOPT_USERPWD, request->user);

  /* play nice and report some user agent */

  if (request->chunk)
  {
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, request->chunk);
  }
  else
  {
    curl_easy_setopt(curl, CURLOPT_USERAGENT, PACKAGE "-libcurl/" VERSION);
  }

  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, request->buffer);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1l);

  //  g_message("thread: perform request");
  request->res = curl_easy_perform(curl);
  // g_message("thread: curl perform returned with %d\n", request->res);

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &request->response);

  /* always cleanup */
  curl_easy_cleanup(curl);

  if (request->chunk)
    curl_slist_free_all(request->chunk);
  request->chunk = 0;
  g_mutex_lock (&mutex);
  --g_nOutstaningCurls;
  g_mutex_unlock (&mutex);
  if (request->cb)
    g_idle_add(net_io_idle_cb, request);

  // request_free(request);

  // g_message("end curl req");
  g_thread_unref(g_thread_self());



  return NULL;
}

/* --------- start of async io ------------ */

static gboolean net_io_do_async(net_io_request_t* request)
{
  GError* error = 0;
  if (g_nOutstaningCurls > 300)
  {
    g_warning("to many");
    request_free(request);
    return FALSE;
  }

  GThread* p = g_thread_try_new("worker_thread", &worker_thread, request, &error);

  if (error != 0)
  {
    g_warning("failed to create the worker thread");
    request_free(request);
    g_error_free(error);
    return FALSE;
  }

  return TRUE;
}

void net_io_append_header(struct curl_slist** chunk, const char* szVal)
{
  *chunk = curl_slist_append(*chunk, szVal);
}

net_result_t net_io_download_sync(char* url, struct curl_slist* chunk)
{
  net_result_t result;
  result.data.ptr = NULL;
  result.data.len = 0;
  result.respCode = -1;

  CURL* curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.data);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, mem_write);

  if (chunk)
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);
  else
    curl_easy_setopt(curl, CURLOPT_USERAGENT, PACKAGE "-libcurl/" VERSION);

  result.code = curl_easy_perform(curl);

  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &result.respCode);

  curl_easy_cleanup(curl);
  curl_slist_free_all(chunk);

  return result;
}

net_io_t net_io_download_async(char* url, net_io_cb cb, gpointer data, struct curl_slist* chunk)
{
  net_io_request_t* request = g_new0(net_io_request_t, 1);
  request->proxy = proxy_config_get();
  request->url = g_strdup(url);
  request->cb = cb;
  request->data = data;
  request->chunk = chunk;

  if (!net_io_do_async(request))
  {
    // request->result.code = 1; // failure
    //cb(&request->result, data);
    return NULL;
  }

  return (net_io_t)request;
}
