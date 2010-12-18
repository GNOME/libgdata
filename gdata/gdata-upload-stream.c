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
 * SECTION:gdata-upload-stream
 * @short_description: GData upload stream object
 * @stability: Unstable
 * @include: gdata/gdata-upload-stream.h
 *
 * #GDataUploadStream is a #GOutputStream subclass to allow uploading of files from GData services with authentication from a #GDataService.
 *
 * Once a #GDataUploadStream is instantiated with gdata_upload_stream_new(), the standard #GOutputStream API can be used on the stream to upload
 * the file. Network communication may not actually begin until the first call to g_output_stream_write(), so having a #GDataUploadStream around is no
 * guarantee that the file is being uploaded.
 *
 * Uploads of a file, or a file with associated metadata (a #GDataEntry) may take place, but if you want to simply upload a single #GDataEntry,
 * use gdata_service_insert_entry() instead. #GDataUploadStream is for large streaming uploads.
 *
 * Once an upload is complete, the server's response can be retrieved from the #GDataUploadStream using gdata_upload_stream_get_response(). In order
 * for network communication to be guaranteed to have stopped (and thus the response definitely available), g_output_stream_close() must be called
 * on the #GDataUploadStream first. Otherwise, gdata_upload_stream_get_response() may return saying that the operation is still in progress.
 *
 * Since: 0.5.0
 **/

/*
 * We have a network thread which does all the uploading work. We send the message encoded as chunks, but cannot use the SoupMessageBody as a
 * data buffer, since it can only ever be touched by the network thread. Instead, we pass data to the network thread through a GDataBuffer, with
 * the main thread pushing it on as and when write() is called. The network thread cannot block on popping data off the buffer, as it requests fixed-
 * size chunks, and there's no way to notify it that we've reached EOF; so when it gets to popping the last chunk off the buffer, which may well be
 * smaller than its chunk size, it would block for more data and therefore hang. Consequently, the network thread instead pops as much data as it can
 * off the buffer, up to its chunk size, which is a non-blocking operation.
 *
 * The write() and close() operations on the output stream are synchronised with the network thread, so that the write() call only returns once the
 * network thread has written a chunk (although we don't guarantee that it's the same chunk which was passed to the write() function), and the
 * close() call only returns once all network activity has finished (including receiving the response from the server). Async versions of these calls
 * are provided by GOutputStream.
 *
 * Mutex locking order:
 *  1. response_mutex
 *  2. write_mutex
 */

#include <config.h>
#include <glib.h>
#include <string.h>

#include "gdata-upload-stream.h"
#include "gdata-buffer.h"
#include "gdata-private.h"

#define BOUNDARY_STRING "0003Z5W789deadbeefRTE456KlemsnoZV"

static GObject *gdata_upload_stream_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params);
static void gdata_upload_stream_dispose (GObject *object);
static void gdata_upload_stream_finalize (GObject *object);
static void gdata_upload_stream_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_upload_stream_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static gssize gdata_upload_stream_write (GOutputStream *stream, const void *buffer, gsize count, GCancellable *cancellable, GError **error);
static gboolean gdata_upload_stream_flush (GOutputStream *stream, GCancellable *cancellable, GError **error);
static gboolean gdata_upload_stream_close (GOutputStream *stream, GCancellable *cancellable, GError **error);

static void create_network_thread (GDataUploadStream *self, GError **error);

struct _GDataUploadStreamPrivate {
	gchar *method;
	gchar *upload_uri;
	GDataService *service;
	GDataEntry *entry;
	gchar *slug;
	gchar *content_type;
	SoupSession *session;
	SoupMessage *message;
	GDataBuffer *buffer;

	GCancellable *cancellable;
	GThread *network_thread;

	GStaticMutex write_mutex; /* mutex for write operations (specifically, write_finished) */
	gsize message_bytes_outstanding; /* the number of bytes which have been written to the buffer but not libsoup (signalled by write_cond) */
	gsize network_bytes_outstanding; /* the number of bytes which have been written to libsoup but not the network (signalled by write_cond) */
	gsize network_bytes_written; /* the number of bytes which have been written to the network (signalled by write_cond) */
	GCond *write_cond; /* signalled when a chunk has been written (protected by write_mutex) */

	guint response_status; /* set once we finish receiving the response (SOUP_STATUS_NONE otherwise) (protected by response_mutex) */
	GCond *finished_cond; /* signalled when sending the message (and receiving the response) is finished (protected by response_mutex) */
	GError *response_error; /* error asynchronously set by the network thread, and picked up by the main thread when appropriate */
	GStaticMutex response_mutex; /* mutex for ->response_error, ->response_status and ->finished_cond */
};

enum {
	PROP_SERVICE = 1,
	PROP_UPLOAD_URI,
	PROP_ENTRY,
	PROP_SLUG,
	PROP_CONTENT_TYPE,
	PROP_METHOD,
	PROP_CANCELLABLE
};

G_DEFINE_TYPE (GDataUploadStream, gdata_upload_stream, G_TYPE_OUTPUT_STREAM)

static void
gdata_upload_stream_class_init (GDataUploadStreamClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GOutputStreamClass *stream_class = G_OUTPUT_STREAM_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataUploadStreamPrivate));

	gobject_class->constructor = gdata_upload_stream_constructor;
	gobject_class->dispose = gdata_upload_stream_dispose;
	gobject_class->finalize = gdata_upload_stream_finalize;
	gobject_class->get_property = gdata_upload_stream_get_property;
	gobject_class->set_property = gdata_upload_stream_set_property;

	/* We use the default implementations of the async functions, which just run
	 * our implementation of the sync function in a thread. */
	stream_class->write_fn = gdata_upload_stream_write;
	stream_class->flush = gdata_upload_stream_flush;
	stream_class->close_fn = gdata_upload_stream_close;

	/**
	 * GDataUploadStream:service:
	 *
	 * The service which is used to authenticate the upload, and to which the upload relates.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_SERVICE,
	                                 g_param_spec_object ("service",
	                                                      "Service", "The service which is used to authenticate the upload.",
	                                                      GDATA_TYPE_SERVICE,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataUploadStream:method:
	 *
	 * The HTTP request method to use when uploading the file.
	 *
	 * Since: 0.7.0
	 **/
	g_object_class_install_property (gobject_class, PROP_METHOD,
	                                 g_param_spec_string ("method",
	                                                      "Method", "The HTTP request method to use when uploading the file.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataUploadStream:upload-uri:
	 *
	 * The URI of the file to upload.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_UPLOAD_URI,
	                                 g_param_spec_string ("upload-uri",
	                                                      "Upload URI", "The URI of the file to upload.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataUploadStream:entry:
	 *
	 * The entry used for metadata to upload.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_ENTRY,
	                                 g_param_spec_object ("entry",
	                                                      "Entry", "The entry used for metadata to upload.",
	                                                      GDATA_TYPE_ENTRY,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataUploadStream:slug:
	 *
	 * The slug of the file being uploaded.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_SLUG,
	                                 g_param_spec_string ("slug",
	                                                      "Slug", "The slug of the file being uploaded.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataUploadStream:content-type:
	 *
	 * The content type of the file being uploaded.
	 *
	 * Since: 0.5.0
	 **/
	g_object_class_install_property (gobject_class, PROP_CONTENT_TYPE,
	                                 g_param_spec_string ("content-type",
	                                                      "Content type", "The content type of the file being uploaded.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataUploadStream:cancellable:
	 *
	 * An optional cancellable used to cancel the entire upload operation. If a #GCancellable instance isn't provided for this property at
	 * construction time (i.e. to gdata_upload_stream_new()), one will be created internally and can be retrieved using
	 * gdata_upload_stream_get_cancellable() and used to cancel the upload operation with g_cancellable_cancel() just as if it was passed to
	 * gdata_upload_stream_new().
	 *
	 * If the upload operation is cancelled using this #GCancellable, any ongoing network activity will be stopped, and any pending or future calls
	 * to #GOutputStream API on the #GDataUploadStream will return %G_IO_ERROR_CANCELLED. Note that the #GCancellable objects which can be passed
	 * to individual #GOutputStream operations will not cancel the upload operation proper if cancelled — they will merely cancel that API call.
	 * The only way to cancel the upload operation completely is using #GDataUploadStream:cancellable.
	 *
	 * Since: 0.8.0
	 **/
	g_object_class_install_property (gobject_class, PROP_CANCELLABLE,
	                                 g_param_spec_object ("cancellable",
	                                                      "Cancellable", "An optional cancellable used to cancel the entire upload operation.",
	                                                      G_TYPE_CANCELLABLE,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_upload_stream_init (GDataUploadStream *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_UPLOAD_STREAM, GDataUploadStreamPrivate);
	self->priv->buffer = gdata_buffer_new ();
	g_static_mutex_init (&(self->priv->write_mutex));
	self->priv->write_cond = g_cond_new ();
	self->priv->finished_cond = g_cond_new ();
	g_static_mutex_init (&(self->priv->response_mutex));
}

static GObject *
gdata_upload_stream_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
	GDataUploadStreamPrivate *priv;
	GDataServiceClass *klass;
	GObject *object;

	/* Chain up to the parent class */
	object = G_OBJECT_CLASS (gdata_upload_stream_parent_class)->constructor (type, n_construct_params, construct_params);
	priv = GDATA_UPLOAD_STREAM (object)->priv;

	/* Create a #GCancellable for the entire upload operation if one wasn't specified for #GDataUploadStream:cancellable during construction */
	if (priv->cancellable == NULL)
		priv->cancellable = g_cancellable_new ();

	/* Build the message */
	priv->message = soup_message_new (priv->method, priv->upload_uri);

	/* Make sure the headers are set */
	klass = GDATA_SERVICE_GET_CLASS (priv->service);
	if (klass->append_query_headers != NULL)
		klass->append_query_headers (priv->service, priv->message);

	if (priv->slug != NULL)
		soup_message_headers_append (priv->message->request_headers, "Slug", priv->slug);

	/* We don't want to accumulate chunks */
	soup_message_body_set_accumulate (priv->message->request_body, FALSE);
	soup_message_headers_set_encoding (priv->message->request_headers, SOUP_ENCODING_CHUNKED);

	/* The Content-Type should be multipart/related if we're also uploading the metadata (entry != NULL),
	 * and the given content_type otherwise.
	 * Note that the Content-Length header is set when we first start writing to the network. */
	if (priv->entry != NULL)
		soup_message_headers_set_content_type (priv->message->request_headers, "multipart/related; boundary=" BOUNDARY_STRING, NULL);
	else
		soup_message_headers_set_content_type (priv->message->request_headers, priv->content_type, NULL);

	/* If the entry exists and has an ETag, we assume we're updating the entry, so we can set the If-Match header */
	if (priv->entry != NULL && gdata_entry_get_etag (priv->entry) != NULL)
		soup_message_headers_append (priv->message->request_headers, "If-Match", gdata_entry_get_etag (priv->entry));

	/* Uploading doesn't actually start until the first call to write() */

	return object;
}

static void
gdata_upload_stream_dispose (GObject *object)
{
	GDataUploadStreamPrivate *priv = GDATA_UPLOAD_STREAM (object)->priv;

	/* Close the stream before unreffing things like priv->service, which stops crashes like bgo#602156 if the stream is unreffed in the middle
	 * of network operations */
	g_output_stream_close (G_OUTPUT_STREAM (object), NULL, NULL);

	if (priv->cancellable != NULL)
		g_object_unref (priv->cancellable);
	priv->cancellable = NULL;

	if (priv->service != NULL)
		g_object_unref (priv->service);
	priv->service = NULL;

	if (priv->message != NULL)
		g_object_unref (priv->message);
	priv->message = NULL;

	if (priv->entry != NULL)
		g_object_unref (priv->entry);
	priv->entry = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_upload_stream_parent_class)->dispose (object);
}

static void
gdata_upload_stream_finalize (GObject *object)
{
	GDataUploadStreamPrivate *priv = GDATA_UPLOAD_STREAM (object)->priv;

	g_static_mutex_free (&(priv->response_mutex));
	g_cond_free (priv->finished_cond);
	g_cond_free (priv->write_cond);
	g_static_mutex_free (&(priv->write_mutex));
	gdata_buffer_free (priv->buffer);
	g_clear_error (&(priv->response_error));
	g_free (priv->upload_uri);
	g_free (priv->method);
	g_free (priv->slug);
	g_free (priv->content_type);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_upload_stream_parent_class)->finalize (object);
}

static void
gdata_upload_stream_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataUploadStreamPrivate *priv = GDATA_UPLOAD_STREAM (object)->priv;

	switch (property_id) {
		case PROP_SERVICE:
			g_value_set_object (value, priv->service);
			break;
		case PROP_METHOD:
			g_value_set_string (value, priv->method);
			break;
		case PROP_UPLOAD_URI:
			g_value_set_string (value, priv->upload_uri);
			break;
		case PROP_ENTRY:
			g_value_set_object (value, priv->entry);
			break;
		case PROP_SLUG:
			g_value_set_string (value, priv->slug);
			break;
		case PROP_CONTENT_TYPE:
			g_value_set_string (value, priv->content_type);
			break;
		case PROP_CANCELLABLE:
			g_value_set_object (value, priv->cancellable);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_upload_stream_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataUploadStreamPrivate *priv = GDATA_UPLOAD_STREAM (object)->priv;

	switch (property_id) {
		case PROP_SERVICE:
			priv->service = g_value_dup_object (value);
			priv->session = _gdata_service_get_session (priv->service);
			break;
		case PROP_METHOD:
			priv->method = g_value_dup_string (value);
			break;
		case PROP_UPLOAD_URI:
			priv->upload_uri = g_value_dup_string (value);
			break;
		case PROP_ENTRY:
			priv->entry = g_value_dup_object (value);
			break;
		case PROP_SLUG:
			priv->slug = g_value_dup_string (value);
			break;
		case PROP_CONTENT_TYPE:
			priv->content_type = g_value_dup_string (value);
			break;
		case PROP_CANCELLABLE:
			/* Construction only */
			priv->cancellable = g_value_dup_object (value);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

typedef struct {
	GDataUploadStream *upload_stream;
	gboolean *cancelled;
} CancelledData;

static void
write_cancelled_cb (GCancellable *cancellable, CancelledData *data)
{
	GDataUploadStreamPrivate *priv = data->upload_stream->priv;

	/* Signal the gdata_upload_stream_write() function that it should stop blocking and cancel */
	g_static_mutex_lock (&(priv->write_mutex));
	*(data->cancelled) = TRUE;
	g_cond_signal (priv->write_cond);
	g_static_mutex_unlock (&(priv->write_mutex));
}

static gssize
gdata_upload_stream_write (GOutputStream *stream, const void *buffer, gsize count, GCancellable *cancellable, GError **error)
{
	GDataUploadStreamPrivate *priv = GDATA_UPLOAD_STREAM (stream)->priv;
	gssize length_written = -1;
	gulong cancelled_signal = 0, global_cancelled_signal = 0;
	gboolean cancelled = FALSE; /* must only be touched with ->write_mutex held */
	gsize old_network_bytes_written;
	CancelledData data;

	/* Listen for cancellation events */
	data.upload_stream = GDATA_UPLOAD_STREAM (stream);
	data.cancelled = &cancelled;

	global_cancelled_signal = g_cancellable_connect (priv->cancellable, (GCallback) write_cancelled_cb, &data, NULL);

	if (cancellable != NULL)
		cancelled_signal = g_cancellable_connect (cancellable, (GCallback) write_cancelled_cb, &data, NULL);

	/* Check for an error and return if necessary */
	g_static_mutex_lock (&(priv->write_mutex));

	if (cancelled == TRUE) {
		g_assert (g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE ||
		          g_cancellable_set_error_if_cancelled (priv->cancellable, error) == TRUE);
		g_static_mutex_unlock (&(priv->write_mutex));

		length_written = -1;
		goto done;
	}

	/* Increment the number of bytes outstanding for the new write, and keep a record of the old number written so we know if the write's
	 * finished before we reach write_cond. */
	old_network_bytes_written = priv->network_bytes_written;
	priv->message_bytes_outstanding += count;

	g_static_mutex_unlock (&(priv->write_mutex));

	/* Handle the more common case of the network thread already having been created first */
	if (priv->network_thread != NULL) {
		/* Push the new data into the buffer */
		gdata_buffer_push_data (priv->buffer, buffer, count);
		goto write;
	}

	/* We're lazy about starting the network operation so we don't time out before we've even started */
	if (priv->entry != NULL) {
		/* Start by writing out the entry; then the thread has something to write to the network when it's created */
		const gchar *first_part_header;
		gchar *entry_xml, *second_part_header;

		first_part_header = "--" BOUNDARY_STRING "\nContent-Type: application/atom+xml; charset=UTF-8\n\n";
		entry_xml = gdata_parsable_get_xml (GDATA_PARSABLE (priv->entry));
		second_part_header = g_strdup_printf ("\n--" BOUNDARY_STRING "\nContent-Type: %s\nContent-Transfer-Encoding: binary\n\n",
		                                      priv->content_type);

		/* Push the message parts onto the message body; we can skip the buffer, since the network thread hasn't yet been created,
		 * so we're the sole thread accessing the SoupMessage. */
		soup_message_body_append (priv->message->request_body, SOUP_MEMORY_STATIC, first_part_header, strlen (first_part_header));
		soup_message_body_append (priv->message->request_body, SOUP_MEMORY_TAKE, entry_xml, strlen (entry_xml));
		soup_message_body_append (priv->message->request_body, SOUP_MEMORY_TAKE, second_part_header, strlen (second_part_header));

		g_static_mutex_lock (&(priv->write_mutex));
		priv->network_bytes_outstanding += priv->message->request_body->length;
		g_static_mutex_unlock (&(priv->write_mutex));
	}

	/* Also write out the first chunk of data, so there's guaranteed to be something in the buffer */
	gdata_buffer_push_data (priv->buffer, buffer, count);

	/* Create the thread and let the writing commence! */
	create_network_thread (GDATA_UPLOAD_STREAM (stream), error);
	if (priv->network_thread == NULL) {
		length_written = -1;
		goto done;
	}

write:
	g_static_mutex_lock (&(priv->write_mutex));

	/* Wait for it to be written */
	while (priv->network_bytes_written - old_network_bytes_written < count && cancelled == FALSE)
		g_cond_wait (priv->write_cond, g_static_mutex_get_mutex (&(priv->write_mutex)));
	length_written = MIN (count, priv->network_bytes_written - old_network_bytes_written);

	/* Check for an error and return if necessary */
	if (cancelled == TRUE && length_written < 1) {
		g_assert (g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE ||
		          g_cancellable_set_error_if_cancelled (priv->cancellable, error) == TRUE);
		length_written = -1;
	}

	g_static_mutex_unlock (&(priv->write_mutex));

done:
	/* Disconnect from the cancelled signals. Note that we have to do this with @write_mutex not held, as g_cancellable_disconnect() blocks
	 * until any outstanding cancellation callbacks return, and they will block on @write_mutex. */
	if (cancelled_signal != 0)
		g_cancellable_disconnect (cancellable, cancelled_signal);
	if (global_cancelled_signal != 0)
		g_cancellable_disconnect (priv->cancellable, global_cancelled_signal);

	g_assert (cancelled == TRUE || length_written > 0);

	return length_written;
}

static void
flush_cancelled_cb (GCancellable *cancellable, CancelledData *data)
{
	GDataUploadStreamPrivate *priv = data->upload_stream->priv;

	/* Signal the gdata_upload_stream_flush() function that it should stop blocking and cancel */
	g_static_mutex_lock (&(priv->write_mutex));
	*(data->cancelled) = TRUE;
	g_cond_signal (priv->write_cond);
	g_static_mutex_unlock (&(priv->write_mutex));
}

/* Block until ->network_bytes_outstanding reaches zero. Cancelling the cancellable passed to gdata_upload_stream_flush() breaks out of the wait(),
 * but doesn't stop the network thread from continuing to write the remaining bytes to the network.
 * The wrapper function, g_output_stream_flush(), calls g_output_stream_set_pending() before calling this function, and calls
 * g_output_stream_clear_pending() afterwards, so we don't need to worry about other operations happening concurrently. We also don't need to worry
 * about being called after the stream has been closed (though the network activity could finish before or during this function). */
static gboolean
gdata_upload_stream_flush (GOutputStream *stream, GCancellable *cancellable, GError **error)
{
	GDataUploadStreamPrivate *priv = GDATA_UPLOAD_STREAM (stream)->priv;
	gulong cancelled_signal = 0, global_cancelled_signal = 0;
	gboolean cancelled = FALSE; /* must only be touched with ->write_mutex held */
	gboolean success = TRUE;
	CancelledData data;

	/* Listen for cancellation events */
	data.upload_stream = GDATA_UPLOAD_STREAM (stream);
	data.cancelled = &cancelled;

	global_cancelled_signal = g_cancellable_connect (priv->cancellable, (GCallback) flush_cancelled_cb, &data, NULL);

	if (cancellable != NULL)
		cancelled_signal = g_cancellable_connect (cancellable, (GCallback) flush_cancelled_cb, &data, NULL);

	/* Start the flush operation proper */
	g_static_mutex_lock (&(priv->write_mutex));

	/* Wait for all outstanding bytes to be written to the network */
	while (priv->network_bytes_outstanding > 0 && cancelled == FALSE)
		g_cond_wait (priv->write_cond, g_static_mutex_get_mutex (&(priv->write_mutex)));

	/* Check for an error and return if necessary */
	if (cancelled == TRUE) {
		g_assert (g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE ||
		          g_cancellable_set_error_if_cancelled (priv->cancellable, error) == TRUE);
		success = FALSE;
	}

	g_static_mutex_unlock (&(priv->write_mutex));

	/* Disconnect from the cancelled signals. Note that we have to do this without @write_mutex held, as g_cancellable_disconnect() blocks
	 * until any outstanding cancellation callbacks return, and they will block on @write_mutex. */
	if (cancelled_signal != 0)
		g_cancellable_disconnect (cancellable, cancelled_signal);
	if (global_cancelled_signal != 0)
		g_cancellable_disconnect (priv->cancellable, global_cancelled_signal);

	return success;
}

static void
close_cancelled_cb (GCancellable *cancellable, CancelledData *data)
{
	GDataUploadStreamPrivate *priv = data->upload_stream->priv;

	/* Signal the gdata_upload_stream_close() function that it should stop blocking and cancel */
	g_static_mutex_lock (&(priv->response_mutex));
	*(data->cancelled) = TRUE;
	g_cond_signal (priv->finished_cond);
	g_static_mutex_unlock (&(priv->response_mutex));
}

/* It's guaranteed that we have set ->response_status and ->response_error and are done with *all* network activity before this returns, unless it's
 * cancelled. This means that it's safe to call gdata_upload_stream_get_response() once a call to close() has returned without being cancelled.
 *
 * If g_output_stream_close() has already been called once on this stream, g_output_stream_is_closed() is set to %TRUE, and all future calls to
 * g_output_stream_close() will immediately return, i.e. gdata_upload_stream_close() is guaranteed to only ever run once (even if the first call was
 * cancelled or returned an error).
 *
 * If the network thread hasn't yet been started (i.e. gdata_upload_stream_write() hasn't been called at all yet), %TRUE will be returned immediately.
 *
 * If the global cancellable, ->cancellable, or @cancellable are cancelled before the call to gdata_upload_stream_close(), gdata_upload_stream_close()
 * should return immediately with %G_IO_ERROR_CANCELLED. If they're cancelled during the call, gdata_upload_stream_close() should stop waiting for
 * any outstanding data to be flushed to the network and return %G_IO_ERROR_CANCELLED (though the operation to finish off network activity and close
 * the stream will still continue).
 *
 * If the call to gdata_upload_stream_close() is not cancelled by any #GCancellable, it will wait until all the data has been flushed to the network
 * and a response has been received. At this point, ->response_status and ->response_error have been set (and won't ever change) and we can return
 * either success or an error code. */
static gboolean
gdata_upload_stream_close (GOutputStream *stream, GCancellable *cancellable, GError **error)
{
	GDataUploadStreamPrivate *priv = GDATA_UPLOAD_STREAM (stream)->priv;
	gboolean success = TRUE;
	gboolean cancelled = FALSE; /* must only be touched with ->response_mutex held */
	gulong cancelled_signal = 0, global_cancelled_signal = 0;
	CancelledData data;
	GError *child_error = NULL;

	/* If the operation was never started, return successfully immediately */
	if (priv->network_thread == NULL)
		return TRUE;

	/* Allow cancellation */
	data.upload_stream = GDATA_UPLOAD_STREAM (stream);
	data.cancelled = &cancelled;

	global_cancelled_signal = g_cancellable_connect (priv->cancellable, (GCallback) close_cancelled_cb, &data, NULL);

	if (cancellable != NULL)
		cancelled_signal = g_cancellable_connect (cancellable, (GCallback) close_cancelled_cb, &data, NULL);

	g_static_mutex_lock (&(priv->response_mutex));

	/* If an operation is still in progress, and no response has been received yet… */
	if (priv->response_status == SOUP_STATUS_NONE) {
		/* Mark the buffer as having reached EOF, and the write operation will close in its own time */
		gdata_buffer_push_data (priv->buffer, NULL, 0);

		/* Wait for the signal that we've finished. Cancelling the call to gdata_upload_stream_close() will cause this wait to be aborted,
		 * but won't actually prevent the stream being closed (i.e. all it means is that the stream isn't guaranteed to have been closed by
		 * the time gdata_upload_stream_close() returns — whereas normally it would be). */
		if (cancelled == FALSE)
			g_cond_wait (priv->finished_cond, g_static_mutex_get_mutex (&(priv->response_mutex)));
	}

	/* Error handling */
	if (priv->response_status == SOUP_STATUS_NONE && cancelled == TRUE) {
		/* Cancelled? If response_status is non-zero, the network activity finished before the gdata_upload_stream_close() operation was
		 * cancelled, so we don't need to return an error. */
		g_assert (g_cancellable_set_error_if_cancelled (cancellable, &child_error) == TRUE ||
		          g_cancellable_set_error_if_cancelled (priv->cancellable, &child_error) == TRUE);
		priv->response_status = SOUP_STATUS_CANCELLED;
		success = FALSE;
	} else if (priv->response_error != NULL) {
		/* Report any errors which have been set by the network thread */
		g_propagate_error (&child_error, priv->response_error);
		priv->response_error = NULL;
		success = FALSE;
	}

	g_assert (priv->response_status != SOUP_STATUS_NONE && (SOUP_STATUS_IS_SUCCESSFUL (priv->response_status) || child_error != NULL));

	g_static_mutex_unlock (&(priv->response_mutex));

	/* Disconnect from the signal handler. Note that we have to do this with @response_mutex not held, as g_cancellable_disconnect() blocks
	 * until any outstanding cancellation callbacks return, and they will block on @response_mutex. */
	if (cancelled_signal != 0)
		g_cancellable_disconnect (cancellable, cancelled_signal);
	if (global_cancelled_signal != 0)
		g_cancellable_disconnect (priv->cancellable, global_cancelled_signal);

	g_assert ((success == TRUE && child_error == NULL) || (success == FALSE && child_error != NULL));

	if (child_error != NULL)
		g_propagate_error (error, child_error);

	return success;
}

/* In the network thread context, called just after writing the headers, or just after writing a chunk, to write the next chunk to libsoup.
 * We don't let it return until we've finished pushing all the data into the buffer.
 * This is due to http://bugzilla.gnome.org/show_bug.cgi?id=522147, which means that
 * we can't use soup_session_(un)?pause_message() with a SoupSessionSync.
 * If we don't return from this signal handler, the message is never paused, and thus
 * Bad Things don't happen (due to the bug, messages can be paused, but not unpaused).
 * Obviously this means that our memory usage will increase, and we'll eventually end
 * up storing the entire request body in memory, but that's unavoidable at this point. */
static void
write_next_chunk (GDataUploadStream *self, SoupMessage *message)
{
	#define CHUNK_SIZE 8192 /* 8KiB */

	GDataUploadStreamPrivate *priv = self->priv;
	gsize length;
	gboolean reached_eof = FALSE;
	guint8 next_buffer[CHUNK_SIZE];

	g_static_mutex_lock (&(priv->write_mutex));

	/* If there are still bytes in libsoup's buffer, don't block on getting new bytes into the stream */
	if (priv->network_bytes_outstanding > 0) {
		g_static_mutex_unlock (&(priv->write_mutex));
		return;
	}

	g_static_mutex_unlock (&(priv->write_mutex));

	/* Append the next chunk to the message body so it can join in the fun.
	 * Note that this call isn't blocking, and can return less than the CHUNK_SIZE. This is because
	 * we could deadlock if we block on getting CHUNK_SIZE bytes at the end of the stream. write() could
	 * easily be called with fewer bytes, but has no way to notify us that we've reached the end of the
	 * stream, so we'd happily block on receiving more bytes which weren't forthcoming. */
	length = gdata_buffer_pop_data_limited (priv->buffer, next_buffer, CHUNK_SIZE, &reached_eof);

	g_static_mutex_lock (&(priv->write_mutex));
	priv->message_bytes_outstanding -= length;
	priv->network_bytes_outstanding += length;
	g_static_mutex_unlock (&(priv->write_mutex));

	/* Append whatever data was returned */
	if (length > 0)
		soup_message_body_append (priv->message->request_body, SOUP_MEMORY_COPY, next_buffer, length);

	if (reached_eof == TRUE) {
		/* We've reached the end of the stream, so append the footer (if appropriate) and stop */
		g_static_mutex_lock (&(priv->response_mutex));
		g_static_mutex_lock (&(priv->write_mutex));

		if (priv->entry != NULL) {
			const gchar *footer = "\n--" BOUNDARY_STRING "--";
			gsize footer_length = strlen (footer);

			soup_message_body_append (priv->message->request_body, SOUP_MEMORY_STATIC, footer, footer_length);
			priv->network_bytes_outstanding += footer_length;
		}

		soup_message_body_complete (priv->message->request_body);
		g_assert (priv->message_bytes_outstanding == 0);

		g_static_mutex_unlock (&(priv->write_mutex));
		g_static_mutex_unlock (&(priv->response_mutex));
	}

	soup_session_unpause_message (priv->session, priv->message);
}

static void
wrote_headers_cb (SoupMessage *message, GDataUploadStream *self)
{
	GDataUploadStreamPrivate *priv = self->priv;

	/* Signal the main thread that the headers have been written */
	g_static_mutex_lock (&(priv->write_mutex));
	g_cond_signal (priv->write_cond);
	g_static_mutex_unlock (&(priv->write_mutex));

	/* Send the first chunk to libsoup */
	write_next_chunk (self, message);
}

static void
wrote_body_data_cb (SoupMessage *message, SoupBuffer *buffer, GDataUploadStream *self)
{
	GDataUploadStreamPrivate *priv = self->priv;

	/* Signal the main thread that the chunk has been written */
	g_static_mutex_lock (&(priv->write_mutex));
	g_assert (priv->network_bytes_outstanding > 0);
	priv->network_bytes_outstanding -= buffer->length;
	priv->network_bytes_written += buffer->length;
	g_cond_signal (priv->write_cond);
	g_static_mutex_unlock (&(priv->write_mutex));

	/* Send the next chunk to libsoup */
	write_next_chunk (self, message);
}

static gpointer
upload_thread (GDataUploadStream *self)
{
	GDataUploadStreamPrivate *priv = self->priv;

	g_assert (priv->cancellable != NULL);

	/* Connect to the wrote-* signals so we can prepare the next chunk for transmission */
	g_signal_connect (priv->message, "wrote-headers", (GCallback) wrote_headers_cb, self);
	g_signal_connect (priv->message, "wrote-body-data", (GCallback) wrote_body_data_cb, self);

	_gdata_service_actually_send_message (priv->session, priv->message, priv->cancellable, NULL);

	/* Signal write_cond, just in case we errored out and finished sending in the middle of a write */
	g_static_mutex_lock (&(priv->response_mutex));
	g_static_mutex_lock (&(priv->write_mutex));
	if (priv->message_bytes_outstanding > 0 || priv->network_bytes_outstanding > 0)
		g_cond_signal (priv->write_cond);
	g_static_mutex_unlock (&(priv->write_mutex));

	g_assert (priv->response_status == SOUP_STATUS_NONE);
	g_assert (priv->response_error == NULL);

	/* Deal with the response if it was unsuccessful */
	if (SOUP_STATUS_IS_SUCCESSFUL (priv->message->status_code) == FALSE) {
		GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (priv->service);

		/* Error! Store it in the structure, and it'll be returned by the next function in the main thread
		 * which can give an error response.*/
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (priv->service, GDATA_OPERATION_UPLOAD, priv->message->status_code, priv->message->reason_phrase,
		                             priv->message->response_body->data, priv->message->response_body->length, &(priv->response_error));
	}

	/* Signal the main thread that the response is ready (good or bad) */
	priv->response_status = priv->message->status_code;
	g_cond_signal (priv->finished_cond);

	g_static_mutex_unlock (&(priv->response_mutex));

	return NULL;
}

static void
create_network_thread (GDataUploadStream *self, GError **error)
{
	GDataUploadStreamPrivate *priv = self->priv;

	g_assert (priv->network_thread == NULL);
	priv->network_thread = g_thread_create ((GThreadFunc) upload_thread, self, TRUE, error);
}

/**
 * gdata_upload_stream_new:
 * @service: a #GDataService
 * @method: the HTTP method to use
 * @upload_uri: the URI to upload
 * @entry: (allow-none): the entry to upload as metadata, or %NULL
 * @slug: the file's slug (filename)
 * @content_type: the content type of the file being uploaded
 * @cancellable: (allow-none): a #GCancellable for the entire upload stream, or %NULL
 *
 * Creates a new #GDataUploadStream, allowing a file to be uploaded from a GData service using standard #GOutputStream API.
 *
 * The HTTP method to use should be specified in @method, and will typically be either %SOUP_METHOD_POST (for insertions) or %SOUP_METHOD_PUT
 * (for updates), according to the server and the @upload_uri.
 *
 * If @entry is specified, it will be attached to the upload as the entry to which the file being uploaded belongs. Otherwise, just the file
 * written to the stream will be uploaded, and given a default entry as determined by the server.
 *
 * @slug and @content_type must be specified before the upload begins, as they describe the file being streamed. @slug is the filename given to the
 * file, which will typically be stored on the server and made available when downloading the file again. @content_type must be the correct
 * content type for the file, and should be in the service's list of acceptable content types.
 *
 * As well as the standard GIO errors, calls to the #GOutputStream API on a #GDataUploadStream can also return any relevant specific error from
 * #GDataServiceError, or %GDATA_SERVICE_ERROR_PROTOCOL_ERROR in the general case.
 *
 * If a #GCancellable is provided in @cancellable, the upload operation may be cancelled at any time from another thread using g_cancellable_cancel().
 * In this case, any ongoing network activity will be stopped, and any pending or future calls to #GOutputStream API on the #GDataUploadStream will
 * return %G_IO_ERROR_CANCELLED. Note that the #GCancellable objects which can be passed to individual #GOutputStream operations will not cancel the
 * upload operation proper if cancelled — they will merely cancel that API call. The only way to cancel the upload operation completely is using this
 * @cancellable.
 *
 * Note that network communication won't begin until the first call to g_output_stream_write() on the #GDataUploadStream.
 *
 * Return value: a new #GOutputStream, or %NULL; unref with g_object_unref()
 *
 * Since: 0.8.0
 **/
GOutputStream *
gdata_upload_stream_new (GDataService *service, const gchar *method, const gchar *upload_uri, GDataEntry *entry,
                         const gchar *slug, const gchar *content_type, GCancellable *cancellable)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (method != NULL, NULL);
	g_return_val_if_fail (upload_uri != NULL, NULL);
	g_return_val_if_fail (entry == NULL || GDATA_IS_ENTRY (entry), NULL);
	g_return_val_if_fail (slug != NULL, NULL);
	g_return_val_if_fail (content_type != NULL, NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);

	/* Create the upload stream */
	return G_OUTPUT_STREAM (g_object_new (GDATA_TYPE_UPLOAD_STREAM, "method", method, "upload-uri", upload_uri, "service", service,
	                                      "entry", entry, "slug", slug, "content-type", content_type, "cancellable", cancellable, NULL));
}

/**
 * gdata_upload_stream_get_response:
 * @self: a #GDataUploadStream
 * @length: (allow-none) (out caller-allocates): return location for the length of the response, or %NULL
 *
 * Returns the server's response to the upload operation performed by the #GDataUploadStream. If the operation
 * is still underway, or the server's response hasn't been received yet, %NULL is returned and @length is set to <code class="literal">-1</code>.
 *
 * If there was an error during the upload operation (but it is complete), %NULL is returned, and @length is set to <code class="literal">0</code>.
 *
 * While it is safe to call this function from any thread at any time during the network operation, the only way to guarantee that the response has
 * been set before calling this function is to have closed the #GDataUploadStream by calling gdata_output_stream_close() on it, without cancelling
 * the close operation. Once the stream has been closed, all network communication is guaranteed to have finished. Note that if a call to
 * g_output_stream_close() is cancelled, g_output_stream_is_closed() will immediately start to return %TRUE, even if the #GDataUploadStream is still
 * attempting to flush the network buffers asynchronously — consequently, gdata_upload_stream_get_response() may still return %NULL and a @length of
 * <code class="literal">-1</code>. The only reliable way to determine if the stream has been fully closed in this situation is to check the results
 * of gdata_upload_stream_get_response(), rather than g_output_stream_is_closed().
 *
 * Return value: the server's response to the upload, or %NULL
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_upload_stream_get_response (GDataUploadStream *self, gssize *length)
{
	gssize _length;
	const gchar *_response;

	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), NULL);

	g_static_mutex_lock (&(self->priv->response_mutex));

	if (self->priv->response_status == SOUP_STATUS_NONE) {
		/* We can't touch the message until the network thread has finished using it, since it isn't threadsafe */
		_length = -1;
		_response = NULL;
	} else if (SOUP_STATUS_IS_SUCCESSFUL (self->priv->response_status) == FALSE) {
		/* The response has been received, and was unsuccessful */
		_length = 0;
		_response = NULL;
	} else {
		/* The response has been received, and was successful */
		_length = self->priv->message->response_body->length;
		_response = self->priv->message->response_body->data;
	}

	g_static_mutex_unlock (&(self->priv->response_mutex));

	if (length != NULL)
		*length = _length;
	return _response;
}

/**
 * gdata_upload_stream_get_service:
 * @self: a #GDataUploadStream
 *
 * Gets the service used to authenticate the upload, as passed to gdata_upload_stream_new().
 *
 * Return value: (transfer none): the #GDataService used to authenticate the upload
 *
 * Since: 0.5.0
 **/
GDataService *
gdata_upload_stream_get_service (GDataUploadStream *self)
{
	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), NULL);
	return self->priv->service;
}

/**
 * gdata_upload_stream_get_method:
 * @self: a #GDataUploadStream
 *
 * Gets the HTTP request method being used to upload the file, as passed to gdata_upload_stream_new().
 *
 * Return value: the HTTP request method in use
 *
 * Since: 0.7.0
 **/
const gchar *
gdata_upload_stream_get_method (GDataUploadStream *self)
{
	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), NULL);
	return self->priv->method;
}

/**
 * gdata_upload_stream_get_upload_uri:
 * @self: a #GDataUploadStream
 *
 * Gets the URI the file is being uploaded to, as passed to gdata_upload_stream_new().
 *
 * Return value: the URI which the file is being uploaded to
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_upload_stream_get_upload_uri (GDataUploadStream *self)
{
	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), NULL);
	return self->priv->upload_uri;
}

/**
 * gdata_upload_stream_get_entry:
 * @self: a #GDataUploadStream
 *
 * Gets the entry being used to upload metadata, if one was passed to gdata_upload_stream_new().
 *
 * Return value: (transfer none): the entry used for metadata, or %NULL
 *
 * Since: 0.5.0
 **/
GDataEntry *
gdata_upload_stream_get_entry (GDataUploadStream *self)
{
	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), NULL);
	return self->priv->entry;
}

/**
 * gdata_upload_stream_get_slug:
 * @self: a #GDataUploadStream
 *
 * Gets the slug (filename) of the file being uploaded.
 *
 * Return value: the slug of the file being uploaded
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_upload_stream_get_slug (GDataUploadStream *self)
{
	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), NULL);
	return self->priv->slug;
}

/**
 * gdata_upload_stream_get_content_type:
 * @self: a #GDataUploadStream
 *
 * Gets the content type of the file being uploaded.
 *
 * Return value: the content type of the file being uploaded
 *
 * Since: 0.5.0
 **/
const gchar *
gdata_upload_stream_get_content_type (GDataUploadStream *self)
{
	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), NULL);
	return self->priv->content_type;
}

/**
 * gdata_upload_stream_get_cancellable:
 * @self: a #GDataUploadStream
 *
 * Gets the #GCancellable for the entire upload operation, #GDataUploadStream:cancellable.
 *
 * Return value: (transfer none): the #GCancellable for the entire upload operation
 *
 * Since: 0.8.0
 **/
GCancellable *
gdata_upload_stream_get_cancellable (GDataUploadStream *self)
{
	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), NULL);
	g_assert (self->priv->cancellable != NULL);
	return self->priv->cancellable;
}
