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

#ifndef NET_IO_H
#define NET_IO_H

#include <glib.h>

G_BEGIN_DECLS

typedef struct {
  char *ptr;
  int len;
} net_mem_t;

typedef struct {
  int code;          /* 0 == success */
  net_mem_t data;
} net_result_t;

typedef void (*net_io_cb)(net_result_t *result, gpointer data);
#define	NET_IO_CB(f) ((net_io_cb) (f))

#define MAEP_NET_IO_ERROR net_io_get_quark()
GQuark net_io_get_quark();

typedef void* net_io_t;

#ifdef WITH_GTK
#include <gtk/gtk.h>
gboolean net_io_download(GtkWidget *parent, char *url, char **mem);
#endif

void net_io_init();
void net_io_finalize();
net_io_t net_io_download_async(char *url, net_io_cb, gpointer data);
void net_io_cancel_async(net_io_t io);

G_END_DECLS

#endif // NET_IO_H
