/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
 *
 * GData Client is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * GData Client is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GData Client.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * SECTION:gdata-download-stream
 * @short_description: GData download stream object
 * @stability: Unstable
 * @include: gdata/gdata-download-stream.h
 *
 * #GDataDownloadStream is a #GInputStream subclass to allow downloading of files from GData services with authentication from a #GDataService.
 *
 * Once a #GDataDownloadStream is instantiated with gdata_download_stream_new(), the standard #GInputStream API can be used on the stream to download
 * the file. Network communication may not actually begin until the first call to g_input_stream_read(), so having a #GDataDownloadStream around is no
 * guarantee that the file is being downloaded.
 *
 * The content type and length of the file being downloaded are made available through #GDataDownloadStream:content-type and
 * #GDataDownloadStream:content-length as soon as the appropriate data is received from the server. Connect to the
 * #GObject::notify <code type="literal">content-type</code> and <code type="literal">content-length</code> details to be notified as
 * soon as the data is available.
 *
 * Since: 0.5.0
 **/

#include <config.h>
#include <glib.h>

#include "gdata-download-stream.h"
#include "gdata-buffer.h"
#include "gdata-private.h"

static void gdata_download_stream_seekable_iface_init (GSeekableIface *seekable_iface);
static GObject *gdata_download_stream_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params);
static void gdata_download_stream_dispose (GObject *object);
static void gdata_download_stream_finalize (GObject *object);
static void gdata_download_stream_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_download_stream_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static gssize gdata_download_stream_read (GInputStream *stream, void *buffer, gsize count, GCancellable *cancellable, GError **error);
static gboolean gdata_download_stream_close (GInputStream *stream, GCancellable *cancellable, GError **error);

static goffset gdata_download_stream_tell (GSeekable *seekable);
static gboolean gdata_download_stream_can_seek (GSeekable *seekable);
static gboolean gdata_download_stream_seek (GSeekable *seekable, goffset offset, GSeekType type, GCancellable *cancellable, GError **error);
static gboolean gdata_download_stream_can_truncate (GSeekable *seekable);
static gboolean gdata_download_stream_truncate (GSeekable *seekable, goffset offset, GCancellable *cancellable, GError **error);

static void create_network_thread (GDataDownloadStream *self, GError **error);

struct _GDataDownloadStreamPrivate {
	gchar *download_uri;
	GDataService *service;
	SoupSession *session;
	SoupMessage *message;
	GDataBuffer *buffer;
	goffset offset; /* seek offset */
	GThread *network_thread;

	gboolean finished;
	GCond *finished_cond;
	GStaticMutex finished_mutex; /* mutex for ->finished, protected by ->finished_cond */

	/* Cached data from the SoupMessage */
	gchar *content_type;
	gssize content_length;
	GStaticMutex content_mutex; /* mutex to protect them */
};

enum {
	PROP_SERVICE = 1,
	PROP_DOWNLOAD_URI,
	PROP_CONTENT_TYPE,
	PROP_CONTENT_LENGTH
};

G_DEFINE_TYPE_WITH_CODE (GDataDownloadStream, gdata_download_stream, G_TYPE_INPUT_STREAM,
                         G_IMPLEMENT_INTERFACE (G_TYPE_SEEKABLE, gdata_download_stream_seekable_iface_init))

static void
gdata_download_stream_class_init (GDataDownloadStreamClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GInputStreamClass *stream_class = G_INPUT_STREAM_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataDownloadStreamPrivate));

	gobject_class->constructor = gdata_download_stream_constructor;
	gobject_class->dispose = gdata_download_stream_dispose;
	gobject_class->finalize = gdata_download_stream_finalize;
	gobject_class->get_property = gdata_download_stream_get_property;
	gobject_class->set_property = gdata_download_stream_set_property;

	/* We use the default implementations of the async functions, which just run
	 * our implementation of the sync function in a thread. */
	stream_class->read_fn = gdata_download_stream_read;
	stream_class->close_fn = gdata_download_stream_close;

	/**
	 * GDataDownloadStream:service:
	 *
	 * The service which is used to authenticate the download, and to which the download relates.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_SERVICE,
	                                 g_param_spec_object ("service",
	                                                      "Service", "The service which is used to authenticate the download.",
	                                                      GDATA_TYPE_SERVICE,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDownloadStream:download-uri:
	 *
	 * The URI of the file to download.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_DOWNLOAD_URI,
	                                 g_param_spec_string ("download-uri",
	                                                      "Download URI", "The URI of the file to download.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDownloadStream:content-type:
	 *
	 * The content type of the file being downloaded.
	 *
	 * Note that change notifications for this property (#GObject::notify emissions) may be emitted in threads other than the one which created
	 * the #GDataDownloadStream. It is the client's responsibility to ensure that any notification signal handlers are either multi-thread safe
	 * or marshal the notification to the thread which owns the #GDataDownloadStream as appropriate.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_CONTENT_TYPE,
	                                 g_param_spec_string ("content-type",
	                                                      "Content type", "The content type of the file being downloaded.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDownloadStream:content-length:
	 *
	 * The length (in bytes) of the file being downloaded.
	 *
	 * Note that change notifications for this property (#GObject::notify emissions) may be emitted in threads other than the one which created
	 * the #GDataDownloadStream. It is the client's responsibility to ensure that any notification signal handlers are either multi-thread safe
	 * or marshal the notification to the thread which owns the #GDataDownloadStream as appropriate.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_CONTENT_LENGTH,
	                                 g_param_spec_long ("content-length",
	                                                    "Content length", "The length (in bytes) of the file being downloaded.",
	                                                    -1, G_MAXSSIZE, -1,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_download_stream_seekable_iface_init (GSeekableIface *seekable_iface)
{
	seekable_iface->tell = gdata_download_stream_tell;
	seekable_iface->can_seek = gdata_download_stream_can_seek;
	seekable_iface->seek = gdata_download_stream_seek;
	seekable_iface->can_truncate = gdata_download_stream_can_truncate;
	seekable_iface->truncate_fn = gdata_download_stream_truncate;
}

static void
gdata_download_stream_init (GDataDownloadStream *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_DOWNLOAD_STREAM, GDataDownloadStreamPrivate);
	self->priv->content_length = -1;
	self->priv->buffer = gdata_buffer_new ();

	self->priv->finished = FALSE;
	self->priv->finished_cond = g_cond_new ();
	g_static_mutex_init (&(self->priv->finished_mutex));

	g_static_mutex_init (&(self->priv->content_mutex));
}

static GObject *
gdata_download_stream_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
	GDataDownloadStreamPrivate *priv;
	GDataServiceClass *klass;
	GObject *object;

	/* Chain up to the parent class */
	object = G_OBJECT_CLASS (gdata_download_stream_parent_class)->constructor (type, n_construct_params, construct_params);
	priv = GDATA_DOWNLOAD_STREAM (object)->priv;

	/* Build the message */
	priv->message = soup_message_new (SOUP_METHOD_GET, priv->download_uri);

	/* Make sure the headers are set */
	klass = GDATA_SERVICE_GET_CLASS (priv->service);
	if (klass->append_query_headers != NULL)
		klass->append_query_headers (priv->service, priv->message);

	/* We don't want to accumulate chunks */
	soup_message_body_set_accumulate (priv->message->request_body, FALSE);

	/* Downloading doesn't actually start until the first call to read() */

	return object;
}

static void
gdata_download_stream_dispose (GObject *object)
{
	GDataDownloadStreamPrivate *priv = GDATA_DOWNLOAD_STREAM (object)->priv;

	if (priv->service != NULL)
		g_object_unref (priv->service);
	priv->service = NULL;

	if (priv->message != NULL)
		g_object_unref (priv->message);
	priv->message = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_download_stream_parent_class)->dispose (object);
}

static void
gdata_download_stream_finalize (GObject *object)
{
	GDataDownloadStreamPrivate *priv = GDATA_DOWNLOAD_STREAM (object)->priv;

	g_thread_join (priv->network_thread);

	g_cond_free (priv->finished_cond);
	g_static_mutex_free (&(priv->finished_mutex));

	g_static_mutex_free (&(priv->content_mutex));
	gdata_buffer_free (priv->buffer);
	g_free (priv->download_uri);
	g_free (priv->content_type);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_download_stream_parent_class)->finalize (object);
}

static void
gdata_download_stream_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataDownloadStreamPrivate *priv = GDATA_DOWNLOAD_STREAM (object)->priv;

	switch (property_id) {
		case PROP_SERVICE:
			g_value_set_object (value, priv->service);
			break;
		case PROP_DOWNLOAD_URI:
			g_value_set_string (value, priv->download_uri);
			break;
		case PROP_CONTENT_TYPE:
			g_static_mutex_lock (&(priv->content_mutex));
			g_value_set_string (value, priv->content_type);
			g_static_mutex_unlock (&(priv->content_mutex));
			break;
		case PROP_CONTENT_LENGTH:
			g_static_mutex_lock (&(priv->content_mutex));
			g_value_set_long (value, priv->content_length);
			g_static_mutex_unlock (&(priv->content_mutex));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_download_stream_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataDownloadStreamPrivate *priv = GDATA_DOWNLOAD_STREAM (object)->priv;

	switch (property_id) {
		case PROP_SERVICE:
			priv->service = g_value_dup_object (value);
			priv->session = _gdata_service_get_session (priv->service);
			break;
		case PROP_DOWNLOAD_URI:
			priv->download_uri = g_value_dup_string (value);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gssize
gdata_download_stream_read (GInputStream *stream, void *buffer, gsize count, GCancellable *cancellable, GError **error)
{
	GDataDownloadStreamPrivate *priv = GDATA_DOWNLOAD_STREAM (stream)->priv;
	gssize length_read;
	GError *child_error = NULL;

	/* We're lazy about starting the network operation so we don't end up with a massive buffer */
	if (priv->network_thread == NULL) {
		create_network_thread (GDATA_DOWNLOAD_STREAM (stream), error);
		if (priv->network_thread == NULL)
			return -1;
	}

	/* Read the data off the buffer. If the operation is cancelled, it'll probably still return a positive number of bytes read — if it does, we
	 * can return without error. Iff it returns a non-positive number of bytes should we return an error. */
	length_read = (gssize) gdata_buffer_pop_data (priv->buffer, buffer, count, NULL, cancellable);

	if (g_cancellable_set_error_if_cancelled (cancellable, &child_error) == TRUE && length_read < 1) {
		/* Handle cancellation */
		g_propagate_error (error, child_error);

		return -1;
	} else if (SOUP_STATUS_IS_SUCCESSFUL (priv->message->status_code) == FALSE) {
		GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (priv->service);

		/* Set an appropriate error */
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (priv->service, GDATA_OPERATION_DOWNLOAD, priv->message->status_code, priv->message->reason_phrase,
		                             NULL, 0, error);

		return -1;
	}

	g_clear_error (&child_error);

	return length_read;
}

static gboolean
gdata_download_stream_close (GInputStream *stream, GCancellable *cancellable, GError **error)
{
	GDataDownloadStreamPrivate *priv = GDATA_DOWNLOAD_STREAM (stream)->priv;

	g_static_mutex_lock (&(priv->finished_mutex));

	/* If the operation has started but hasn't already finished, cancel the SoupMessage and wait for it to finish before returning */
	if (priv->network_thread != NULL && priv->finished == FALSE) {
		soup_session_cancel_message (priv->session, priv->message, SOUP_STATUS_CANCELLED);
		g_cond_wait (priv->finished_cond, g_static_mutex_get_mutex (&(priv->finished_mutex)));
	}

	g_static_mutex_unlock (&(priv->finished_mutex));

	return TRUE;
}

static goffset
gdata_download_stream_tell (GSeekable *seekable)
{
	return GDATA_DOWNLOAD_STREAM (seekable)->priv->offset;
}

static gboolean
gdata_download_stream_can_seek (GSeekable *seekable)
{
	return TRUE;
}

extern void soup_message_io_cleanup (SoupMessage *msg);

/* Copied from SoupInputStream */
static gboolean
gdata_download_stream_seek (GSeekable *seekable, goffset offset, GSeekType type, GCancellable *cancellable, GError **error)
{
	GDataDownloadStreamPrivate *priv = GDATA_DOWNLOAD_STREAM (seekable)->priv;
	gchar *range = NULL;

	if (type == G_SEEK_END) {
		/* FIXME: we could send "bytes=-offset", but unless we know the
		 * Content-Length, we wouldn't be able to answer a tell() properly.
		 * We could find the Content-Length by doing a HEAD...
		*/
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "G_SEEK_END not currently supported");
		return FALSE;
	}

	if (g_input_stream_set_pending (G_INPUT_STREAM (seekable), error) == FALSE)
		return FALSE;

	soup_session_cancel_message (priv->session, priv->message, SOUP_STATUS_CANCELLED);
	soup_message_io_cleanup (priv->message);

	switch (type) {
		case G_SEEK_CUR:
			offset += priv->offset;
			/* fall through */
		case G_SEEK_SET:
			range = g_strdup_printf ("bytes=%" G_GUINT64_FORMAT "-", (guint64) offset);
			priv->offset = offset;
			break;
		case G_SEEK_END:
		default:
			g_assert_not_reached ();
	}

	/* Change the Range header and re-send the message */
	soup_message_headers_remove (priv->message->request_headers, "Range");
	soup_message_headers_append (priv->message->request_headers, "Range", range);
	g_free (range);

	/* Wait for the thread to quit, then launch another one with the modified message */
	g_thread_join (priv->network_thread);
	create_network_thread (GDATA_DOWNLOAD_STREAM (seekable), error);
	if (priv->network_thread == NULL)
		return FALSE;

	g_input_stream_clear_pending (G_INPUT_STREAM (seekable));

	return TRUE;
}

static gboolean
gdata_download_stream_can_truncate (GSeekable *seekable)
{
	return FALSE;
}

static gboolean
gdata_download_stream_truncate (GSeekable *seekable, goffset offset, GCancellable *cancellable, GError **error)
{
	g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "Truncate not allowed on input stream");
	return FALSE;
}

static void
got_headers_cb (SoupMessage *message, GDataDownloadStream *self)
{
	/* Don't get the client's hopes up by setting the Content-Type or -Length if the response
	 * is actually unsuccessful. */
	if (SOUP_STATUS_IS_SUCCESSFUL (message->status_code) == FALSE)
		return;

	g_static_mutex_lock (&(self->priv->content_mutex));
	self->priv->content_type = g_strdup (soup_message_headers_get_content_type (message->response_headers, NULL));
	self->priv->content_length = soup_message_headers_get_content_length (message->response_headers);
	g_static_mutex_unlock (&(self->priv->content_mutex));

	/* Emit the notifications for the Content-Length and -Type properties */
	g_object_freeze_notify (G_OBJECT (self));
	g_object_notify (G_OBJECT (self), "content-length");
	g_object_notify (G_OBJECT (self), "content-type");
	g_object_thaw_notify (G_OBJECT (self));
}

static void
got_chunk_cb (SoupMessage *message, SoupBuffer *buffer, GDataDownloadStream *self)
{
	/* Ignore the chunk if the response is unsuccessful or it has zero length */
	if (SOUP_STATUS_IS_SUCCESSFUL (message->status_code) == FALSE || buffer->length == 0)
		return;

	/* Push the data onto the buffer immediately */
	gdata_buffer_push_data (self->priv->buffer, (const guint8*) buffer->data, buffer->length);
}

static void
finished_cb (SoupMessage *message, GDataDownloadStream *self)
{
	GDataDownloadStreamPrivate *priv = self->priv;

	/* Mark the buffer as having reached EOF */
	gdata_buffer_push_data (priv->buffer, NULL, 0);

	/* Mark the download as finished */
	g_static_mutex_lock (&(priv->finished_mutex));
	priv->finished = TRUE;
	g_cond_signal (priv->finished_cond);
	g_static_mutex_unlock (&(priv->finished_mutex));
}

static gpointer
download_thread (GDataDownloadStream *self)
{
	/* Connect to the got-headers signal so we can notify clients of the values of content-type and content-length */
	g_signal_connect (self->priv->message, "got-headers", (GCallback) got_headers_cb, self);
	g_signal_connect (self->priv->message, "got-chunk", (GCallback) got_chunk_cb, self);
	g_signal_connect (self->priv->message, "finished", (GCallback) finished_cb, self);

	soup_session_send_message (self->priv->session, self->priv->message);

	return NULL;
}

static void
create_network_thread (GDataDownloadStream *self, GError **error)
{
	self->priv->network_thread = g_thread_create ((GThreadFunc) download_thread, self, TRUE, error);
}

/**
 * gdata_download_stream_new:
 * @service: a #GDataService
 * @download_uri: the URI to download
 *
 * Creates a new #GDataDownloadStream, allowing a file to be downloaded from a GData service using standard #GInputStream API.
 *
 * As well as the standard GIO errors, calls to the #GInputStream API on a #GDataDownloadStream can also return any relevant specific error from
 * #GDataServiceError, or %GDATA_SERVICE_ERROR_PROTOCOL_ERROR in the general case.
 *
 * Return value: a new #GInputStream, or %NULL; unref with g_object_unref()
 *
 * Since: 0.5.0
 **/
GInputStream *
gdata_download_stream_new (GDataService *service, const gchar *download_uri)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (download_uri != NULL, NULL);

	return G_INPUT_STREAM (g_object_new (GDATA_TYPE_DOWNLOAD_STREAM, "download-uri", download_uri, "service", service, NULL));
}

/**
 * gdata_download_stream_get_service:
 * @self: a #GDataDownloadStream
 *
 * Gets the service used to authenticate the download, as passed to gdata_download_stream_new().
 *
 * Return value: (transfer none): the #GDataService used to authenticate the download
 *
 * Since: 0.5.0
 **/
GDataService *
gdata_download_stream_get_service (GDataDownloadStream *self)
{
	g_return_val_if_fail (GDATA_IS_DOWNLOAD_STREAM (self), NULL);
	return self->priv->service;
}

/**
 * gdata_download_stream_get_download_uri:
 * @self: a #GDataDownloadStream
 *
 * Gets the URI of the file being downloaded, as passed to gdata_download_stream_new().
 *
 * Return value: the URI of the file being downloaded
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_download_stream_get_download_uri (GDataDownloadStream *self)
{
	g_return_val_if_fail (GDATA_IS_DOWNLOAD_STREAM (self), NULL);
	return self->priv->download_uri;
}

/**
 * gdata_download_stream_get_content_type:
 * @self: a #GDataDownloadStream
 *
 * Gets the content type of the file being downloaded. If the <literal>Content-Type</literal> header has not yet
 * been received, %NULL will be returned.
 *
 * Return value: the content type of the file being downloaded, or %NULL
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_download_stream_get_content_type (GDataDownloadStream *self)
{
	g_return_val_if_fail (GDATA_IS_DOWNLOAD_STREAM (self), NULL);
	return self->priv->content_type;
}

/**
 * gdata_download_stream_get_content_length:
 * @self: a #GDataDownloadStream
 *
 * Gets the length (in bytes) of the file being downloaded. If the <literal>Content-Length</literal> header has not yet
 * been received from the server, <code class="literal">-1</code> will be returned.
 *
 * Return value: the content length of the file being downloaded, or <code class="literal">-1</code>
 *
 * Since: 0.5.0
 **/
gssize
gdata_download_stream_get_content_length (GDataDownloadStream *self)
{
	g_return_val_if_fail (GDATA_IS_DOWNLOAD_STREAM (self), -1);
	return self->priv->content_length;
}
