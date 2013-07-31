/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2013 <philip@tecnocode.co.uk>
 * 
 * GData Client is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GData Client is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GData Client.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GDATA_MOCK_SERVER_H
#define GDATA_MOCK_SERVER_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "mock-resolver.h"

G_BEGIN_DECLS

#define GDATA_TYPE_MOCK_SERVER		(gdata_mock_server_get_type ())
#define GDATA_MOCK_SERVER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_MOCK_SERVER, GDataMockServer))
#define GDATA_MOCK_SERVER_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_MOCK_SERVER, GDataMockServerClass))
#define GDATA_IS_MOCK_SERVER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_MOCK_SERVER))
#define GDATA_IS_MOCK_SERVER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_MOCK_SERVER))
#define GDATA_MOCK_SERVER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_MOCK_SERVER, GDataMockServerClass))

typedef struct _GDataMockServerPrivate	GDataMockServerPrivate;

typedef struct {
	GObject parent;
	GDataMockServerPrivate *priv;
} GDataMockServer;

typedef struct {
	GObjectClass parent;

	/**
	 * handle_message:
	 *
	 * Class handler for the #GDataMockServer::handle-message signal. Subclasses may implement this to override the
	 * default handler for the signal. The default handler should always return %TRUE to indicate that it has handled
	 * the @message from @client by setting an appropriate response on the #SoupMessage.
	 */
	gboolean (*handle_message) (GDataMockServer *self, SoupMessage *message, SoupClientContext *client);
} GDataMockServerClass;

GType gdata_mock_server_get_type (void) G_GNUC_CONST;

GDataMockServer *gdata_mock_server_new (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

void gdata_mock_server_load_trace (GDataMockServer *self, GFile *trace_file, GCancellable *cancellable, GError **error);
void gdata_mock_server_load_trace_async (GDataMockServer *self, GFile *trace_file, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data);
void gdata_mock_server_load_trace_finish (GDataMockServer *self, GAsyncResult *result, GError **error);
void gdata_mock_server_unload_trace (GDataMockServer *self);

void gdata_mock_server_run (GDataMockServer *self);
void gdata_mock_server_stop (GDataMockServer *self);

GFile *gdata_mock_server_get_trace_directory (GDataMockServer *self) G_GNUC_WARN_UNUSED_RESULT;
void gdata_mock_server_set_trace_directory (GDataMockServer *self, GFile *trace_directory);

void gdata_mock_server_start_trace (GDataMockServer *self, const gchar *trace_name);
void gdata_mock_server_start_trace_full (GDataMockServer *self, GFile *trace_file);
void gdata_mock_server_end_trace (GDataMockServer *self);

gboolean gdata_mock_server_get_enable_online (GDataMockServer *self) G_GNUC_WARN_UNUSED_RESULT;
void gdata_mock_server_set_enable_online (GDataMockServer *self, gboolean enable_online);

gboolean gdata_mock_server_get_enable_logging (GDataMockServer *self) G_GNUC_WARN_UNUSED_RESULT;
void gdata_mock_server_set_enable_logging (GDataMockServer *self, gboolean enable_logging);

void gdata_mock_server_received_message_chunk (GDataMockServer *self, const gchar *message_chunk, goffset message_chunk_length);

SoupAddress *gdata_mock_server_get_address (GDataMockServer *self);
guint gdata_mock_server_get_port (GDataMockServer *self);

GDataMockResolver *gdata_mock_server_get_resolver (GDataMockServer *self) G_GNUC_WARN_UNUSED_RESULT;

G_END_DECLS

#endif /* !GDATA_MOCK_SERVER_H */
