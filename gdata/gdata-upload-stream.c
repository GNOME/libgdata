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
 * @stability: Stable
 * @include: gdata/gdata-upload-stream.h
 *
 * #GDataUploadStream is a #GOutputStream subclass to allow uploading of files from GData services with authorization from a #GDataService under
 * the given #GDataAuthorizationDomain. If authorization is not required to perform the upload, a #GDataAuthorizationDomain doesn't have to be
 * specified.
 *
 * Once a #GDataUploadStream is instantiated with gdata_upload_stream_new(), the standard #GOutputStream API can be used on the stream to upload
 * the file. Network communication may not actually begin until the first call to g_output_stream_write(), so having a #GDataUploadStream around is no
 * guarantee that data is being uploaded.
 *
 * Uploads of a file, or a file with associated metadata (a #GDataEntry) should use #GDataUploadStream, but if you want to simply upload a single
 * #GDataEntry, use gdata_service_insert_entry() instead. #GDataUploadStream is for large streaming uploads.
 *
 * Once an upload is complete, the server's response can be retrieved from the #GDataUploadStream using gdata_upload_stream_get_response(). In order
 * for network communication to be guaranteed to have stopped (and thus the response definitely available), g_output_stream_close() must be called
 * on the #GDataUploadStream first. Otherwise, gdata_upload_stream_get_response() may return saying that the operation is still in progress.
 *
 * If the server returns an error instead of a success response, the error will be returned by g_output_stream_close() as a #GDataServiceError.
 *
 * The entire upload operation can be cancelled using the #GCancellable instance provided to gdata_upload_stream_new(), or returned by
 * gdata_upload_stream_get_cancellable(). Cancelling this at any time will cause all future #GOutputStream method calls to return
 * %G_IO_ERROR_CANCELLED. If any #GOutputStream methods are in the process of being called, they will be cancelled and return %G_IO_ERROR_CANCELLED as
 * soon as possible.
 *
 * Note that cancelling an individual method call (such as a call to g_output_stream_write()) using the #GCancellable parameter of the method will not
 * cancel the upload as a whole — just that particular method call. In the case of g_output_stream_write(), this will cause it to return the number of
 * bytes it has successfully written up to the point of cancellation (up to the requested number of bytes), or return a %G_IO_ERROR_CANCELLED if it
 * had not managed to write any bytes to the network by that point. This is also the behaviour of g_output_stream_write() when the upload operation as
 * a whole is cancelled.
 *
 * In the case of g_output_stream_close(), the call will return immediately if network activity hasn't yet started. If it has, the network activity
 * will be cancelled, regardless of whether the call to g_output_stream_close() is cancelled. Cancelling a pending call to g_output_stream_close()
 * (either using the method's #GCancellable, or by cancelling the upload stream as a whole) will cause it to stop waiting for the network activity to
 * finish, and return %G_IO_ERROR_CANCELLED immediately. Network activity will continue to be shut down in the background.
 *
 * Any outstanding data is guaranteed to be written to the network successfully even if a call to g_output_stream_close() is cancelled. However, if
 * the upload stream as a whole is cancelled using #GDataUploadStream:cancellable, no more data will be sent over the network, and the network
 * connection will be closed immediately. i.e. #GDataUploadStream will do its best to instruct the server to cancel the upload and any associated
 * server-side changes of state.
 *
 * If the server returns an error message (for example, if the user is not correctly authenticated/authorized or doesn't have suitable permissions
 * to upload from the given URI), it will be returned as a #GDataServiceError by g_output_stream_close().
 *
 * <example>
 * 	<title>Uploading from a File</title>
 * 	<programlisting>
 *	GDataService *service;
 *	GDataAuthorizationDomain *domain;
 *	GCancellable *cancellable;
 *	GInputStream *input_stream;
 *	GOutputStream *upload_stream;
 *	GFile *file;
 *	GFileInfo *file_info;
 *	GError *error = NULL;
 *
 *	/<!-- -->* Get the file to upload *<!-- -->/
 *	file = get_file_to_upload ();
 *	file_info = g_file_query_info (file, G_FILE_ATTRIBUTE_STANDARD_DISPLAY_NAME "," G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE ","
 *	                               G_FILE_ATTRIBUTE_STANDARD_SIZE,
 *	                               G_FILE_QUERY_INFO_NONE, NULL, &error);
 *
 *	if (file_info == NULL) {
 *		g_error ("Error getting file info: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (file);
 *		return;
 *	}
 *
 *	input_stream = g_file_read (file, NULL, &error);
 *	g_object_unref (file);
 *
 *	if (input_stream == NULL) {
 *		g_error ("Error getting file input stream: %s", error->message);
 *		g_error_free (error);
 *		g_object_unref (file_info);
 *		return;
 *	}
 *
 *	/<!-- -->* Create the upload stream *<!-- -->/
 *	service = create_my_service ();
 *	domain = get_my_authorization_domain_from_service (service);
 *	cancellable = g_cancellable_new (); /<!-- -->* cancel this to cancel the entire upload operation *<!-- -->/
 *	upload_stream = gdata_upload_stream_new_resumable (service, domain, SOUP_METHOD_POST, upload_uri, NULL,
 *	                                                   g_file_info_get_display_name (file_info), g_file_info_get_content_type (file_info),
 *	                                                   g_file_info_get_size (file_info), cancellable);
 *	g_object_unref (file_info);
 *
 *	/<!-- -->* Perform the upload asynchronously *<!-- -->/
 *	g_output_stream_splice_async (upload_stream, input_stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
 *	                              G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback) upload_splice_cb, NULL);
 *
 *	g_object_unref (upload_stream);
 *	g_object_unref (input_stream);
 *	g_object_unref (cancellable);
 *	g_object_unref (domain);
 *	g_object_unref (service);
 *
 *	static void
 *	upload_splice_cb (GOutputStream *upload_stream, GAsyncResult *result, gpointer user_data)
 *	{
 *		gssize length;
 *		GError *error = NULL;
 *
 *		g_output_stream_splice_finish (upload_stream, result, &error);
 *
 *		if (error != NULL && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) == FALSE)) {
 *			/<!-- -->* Error upload the file; potentially an I/O error (GIOError), or an error response from the server
 *			 * (GDataServiceError). *<!-- -->/
 *			g_error ("Error uploading file: %s", error->message);
 *			g_error_free (error);
 *		}
 *
 *		/<!-- -->* If the upload was successful, carry on to parse the result. Note that this will normally be handled by methods like
 *		 * gdata_youtube_service_finish_video_upload(), gdata_picasaweb_service_finish_file_upload() and
 *		 * gdata_documents_service_finish_upload() *<!-- -->/
 *		parse_server_result (gdata_upload_stream_get_response (GDATA_UPLOAD_STREAM (upload_stream), &length), length);
 *	}
 * 	</programlisting>
 * </example>
 *
 * Since: 0.5.0
 */

/*
 * We have a network thread which does all the uploading work. We send the message encoded as chunks, but cannot use the SoupMessageBody as a
 * data buffer, since it can only ever be touched by the network thread. Instead, we pass data to the network thread through a GDataBuffer, with
 * the main thread pushing it on as and when write() is called. The network thread cannot block on popping data off the buffer, as it requests fixed-
 * size chunks, and there's no way to notify it that we've reached EOF; so when it gets to popping the last chunk off the buffer, which may well be
 * smaller than its chunk size, it would block for more data and therefore hang. Consequently, the network thread instead pops as much data as it can
 * off the buffer, up to its chunk size, which is a non-blocking operation.
 *
 * The write() and close() operations on the output stream are synchronised with the network thread, so that the write() call only returns once the
 * network thread has written at least as many bytes as were passed to the write() call, and the close() call only returns once all network activity
 * has finished (including receiving the response from the server). Async versions of these calls are provided by GOutputStream.
 *
 * The number of bytes in the various buffers are recorded using:
 *  • message_bytes_outstanding: the number of bytes in the GDataBuffer which are waiting to be written to the SoupMessageBody
 *  • network_bytes_outstanding: the number of bytes which have been written to the SoupMessageBody, and are waiting to be written to the network
 *  • network_bytes_written: the total number of bytes which have been successfully written to the network
 *
 * Mutex locking order:
 *  1. response_mutex
 *  2. write_mutex
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-upload-stream.h"
#include "gdata-buffer.h"
#include "gdata-private.h"

#define BOUNDARY_STRING "0003Z5W789deadbeefRTE456KlemsnoZV"
#define MAX_RESUMABLE_CHUNK_SIZE (512 * 1024) /* bytes = 512 KiB */

static void gdata_upload_stream_constructed (GObject *object);
static void gdata_upload_stream_dispose (GObject *object);
static void gdata_upload_stream_finalize (GObject *object);
static void gdata_upload_stream_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_upload_stream_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static gssize gdata_upload_stream_write (GOutputStream *stream, const void *buffer, gsize count, GCancellable *cancellable, GError **error);
static gboolean gdata_upload_stream_flush (GOutputStream *stream, GCancellable *cancellable, GError **error);
static gboolean gdata_upload_stream_close (GOutputStream *stream, GCancellable *cancellable, GError **error);

static void create_network_thread (GDataUploadStream *self, GError **error);

typedef enum {
	STATE_INITIAL_REQUEST, /* initial POST request to the resumable-create-media link (unused for non-resumable uploads) */
	STATE_DATA_REQUESTS, /* one or more subsequent PUT requests (only state used for non-resumable uploads) */
	STATE_FINISHED, /* finished successfully or in error */
} UploadState;

struct _GDataUploadStreamPrivate {
	gchar *method;
	gchar *upload_uri;
	GDataService *service;
	GDataAuthorizationDomain *authorization_domain;
	GDataEntry *entry;
	gchar *slug;
	gchar *content_type;
	goffset content_length; /* -1 for non-resumable uploads; 0 or greater for resumable ones */
	SoupSession *session;
	SoupMessage *message;
	GDataBuffer *buffer;

	GCancellable *cancellable;
	GThread *network_thread;

	UploadState state; /* protected by write_mutex */
	GMutex write_mutex; /* mutex for write operations (specifically, write_finished) */
	/* This persists across all resumable upload chunks. Note that it doesn't count bytes from the entry XML. */
	gsize total_network_bytes_written; /* the number of bytes which have been written to the network in STATE_DATA_REQUESTS */

	/* All of the following apply only to the current resumable upload chunk. */
	gsize message_bytes_outstanding; /* the number of bytes which have been written to the buffer but not libsoup (signalled by write_cond) */
	gsize network_bytes_outstanding; /* the number of bytes which have been written to libsoup but not the network (signalled by write_cond) */
	gsize network_bytes_written; /* the number of bytes which have been written to the network (signalled by write_cond) */
	gsize chunk_size; /* the size of the current chunk (in bytes); 0 iff content_length <= 0; must be <= MAX_RESUMABLE_CHUNK_SIZE */
	GCond write_cond; /* signalled when a chunk has been written (protected by write_mutex) */

	GCond finished_cond; /* signalled when sending the message (and receiving the response) is finished (protected by response_mutex) */
	guint response_status; /* set once we finish receiving the response (SOUP_STATUS_NONE otherwise) (protected by response_mutex) */
	GError *response_error; /* error asynchronously set by the network thread, and picked up by the main thread when appropriate */
	GMutex response_mutex; /* mutex for ->response_error, ->response_status and ->finished_cond */
};

enum {
	PROP_SERVICE = 1,
	PROP_UPLOAD_URI,
	PROP_ENTRY,
	PROP_SLUG,
	PROP_CONTENT_TYPE,
	PROP_METHOD,
	PROP_CANCELLABLE,
	PROP_AUTHORIZATION_DOMAIN,
	PROP_CONTENT_LENGTH,
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataUploadStream, gdata_upload_stream, G_TYPE_OUTPUT_STREAM)

static void
gdata_upload_stream_class_init (GDataUploadStreamClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GOutputStreamClass *stream_class = G_OUTPUT_STREAM_CLASS (klass);

	gobject_class->constructed = gdata_upload_stream_constructed;
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
	 * The service which is used to authorize the upload, and to which the upload relates.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_SERVICE,
	                                 g_param_spec_object ("service",
	                                                      "Service", "The service which is used to authorize the upload.",
	                                                      GDATA_TYPE_SERVICE,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataUploadStream:authorization-domain:
	 *
	 * The authorization domain for the upload, against which the #GDataService:authorizer for the #GDataDownloadStream:service should be
	 * authorized. This may be %NULL if authorization is not needed for the upload.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_AUTHORIZATION_DOMAIN,
	                                 g_param_spec_object ("authorization-domain",
	                                                      "Authorization domain", "The authorization domain for the upload.",
	                                                      GDATA_TYPE_AUTHORIZATION_DOMAIN,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataUploadStream:method:
	 *
	 * The HTTP request method to use when uploading the file.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_METHOD,
	                                 g_param_spec_string ("method",
	                                                      "Method", "The HTTP request method to use when uploading the file.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataUploadStream:upload-uri:
	 *
	 * The URI to upload the data and metadata to. This must be HTTPS.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_UPLOAD_URI,
	                                 g_param_spec_string ("upload-uri",
	                                                      "Upload URI", "The URI to upload the data and metadata to.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataUploadStream:entry:
	 *
	 * The entry used for metadata to upload.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_ENTRY,
	                                 g_param_spec_object ("entry",
	                                                      "Entry", "The entry used for metadata to upload.",
	                                                      GDATA_TYPE_ENTRY,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataUploadStream:slug:
	 *
	 * The slug of the file being uploaded. This is usually the display name of the file (i.e. as returned by g_file_info_get_display_name()).
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_SLUG,
	                                 g_param_spec_string ("slug",
	                                                      "Slug", "The slug of the file being uploaded.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataUploadStream:content-length:
	 *
	 * The content length (in bytes) of the file being uploaded (i.e. as returned by g_file_info_get_size()). Note that this does not include the
	 * length of the XML serialisation of #GDataUploadStream:entry, if set.
	 *
	 * If this is <code class="literal">-1</code> the upload will be non-resumable; if it is non-negative, the upload will be resumable.
	 *
	 * Since: 0.13.0
	 */
	g_object_class_install_property (gobject_class, PROP_CONTENT_LENGTH,
	                                 g_param_spec_int64 ("content-length",
	                                                     "Content length", "The content length (in bytes) of the file being uploaded.",
	                                                     -1, G_MAXINT64, -1,
	                                                     G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataUploadStream:content-type:
	 *
	 * The content type of the file being uploaded (i.e. as returned by g_file_info_get_content_type()).
	 *
	 * Since: 0.5.0
	 */
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
	 */
	g_object_class_install_property (gobject_class, PROP_CANCELLABLE,
	                                 g_param_spec_object ("cancellable",
	                                                      "Cancellable", "An optional cancellable used to cancel the entire upload operation.",
	                                                      G_TYPE_CANCELLABLE,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_upload_stream_init (GDataUploadStream *self)
{
	self->priv = gdata_upload_stream_get_instance_private (self);
	self->priv->buffer = gdata_buffer_new ();
	g_mutex_init (&(self->priv->write_mutex));
	g_cond_init (&(self->priv->write_cond));
	g_cond_init (&(self->priv->finished_cond));
	g_mutex_init (&(self->priv->response_mutex));
}

static SoupMessage *
build_message (GDataUploadStream *self, const gchar *method, const gchar *upload_uri)
{
	SoupMessage *new_message;
	SoupURI *_uri;

	/* Build the message */
	_uri = soup_uri_new (upload_uri);
	soup_uri_set_port (_uri, _gdata_service_get_https_port ());
	new_message = soup_message_new_from_uri (method, _uri);
	soup_uri_free (_uri);

	/* We don't want to accumulate chunks */
	soup_message_body_set_accumulate (new_message->request_body, FALSE);

	return new_message;
}

static void
gdata_upload_stream_constructed (GObject *object)
{
	GDataUploadStreamPrivate *priv;
	GDataServiceClass *service_klass;
	SoupURI *uri = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_upload_stream_parent_class)->constructed (object);
	priv = GDATA_UPLOAD_STREAM (object)->priv;

	/* The upload URI must be HTTPS. */
	uri = soup_uri_new (priv->upload_uri);
	g_assert_cmpstr (soup_uri_get_scheme (uri), ==, SOUP_URI_SCHEME_HTTPS);
	soup_uri_free (uri);

	/* Create a #GCancellable for the entire upload operation if one wasn't specified for #GDataUploadStream:cancellable during construction */
	if (priv->cancellable == NULL)
		priv->cancellable = g_cancellable_new ();

	/* Build the message */
	priv->message = build_message (GDATA_UPLOAD_STREAM (object), priv->method, priv->upload_uri);

	if (priv->slug != NULL)
		soup_message_headers_append (priv->message->request_headers, "Slug", priv->slug);

	if (priv->content_length == -1) {
		/* Non-resumable upload */
		soup_message_headers_set_encoding (priv->message->request_headers, SOUP_ENCODING_CHUNKED);

		/* The Content-Type should be multipart/related if we're also uploading the metadata (entry != NULL),
		 * and the given content_type otherwise. */
		if (priv->entry != NULL) {
			gchar *first_part_header, *upload_data;
			gchar *second_part_header;
			GDataParsableClass *parsable_klass;

			parsable_klass = GDATA_PARSABLE_GET_CLASS (priv->entry);
			g_assert (parsable_klass->get_content_type != NULL);

			soup_message_headers_set_content_type (priv->message->request_headers, "multipart/related; boundary=" BOUNDARY_STRING, NULL);

			if (g_strcmp0 (parsable_klass->get_content_type (), "application/json") == 0) {
				upload_data = gdata_parsable_get_json (GDATA_PARSABLE (priv->entry));
			} else {
				upload_data = gdata_parsable_get_xml (GDATA_PARSABLE (priv->entry));
			}

			/* Start by writing out the entry; then the thread has something to write to the network when it's created */
			first_part_header = g_strdup_printf ("--" BOUNDARY_STRING "\n"
			                                     "Content-Type: %s; charset=UTF-8\n\n",
			                                     parsable_klass->get_content_type ());
			second_part_header = g_strdup_printf ("\n--" BOUNDARY_STRING "\n"
			                                      "Content-Type: %s\n"
			                                      "Content-Transfer-Encoding: binary\n\n",
			                                      priv->content_type);

			/* Push the message parts onto the message body; we can skip the buffer, since the network thread hasn't yet been created,
			 * so we're the sole thread accessing the SoupMessage. */
			soup_message_body_append (priv->message->request_body,
			                          SOUP_MEMORY_TAKE,
			                          first_part_header,
			                          strlen (first_part_header));
			soup_message_body_append (priv->message->request_body,
			                          SOUP_MEMORY_TAKE, upload_data,
			                          strlen (upload_data));
			soup_message_body_append (priv->message->request_body,
			                          SOUP_MEMORY_TAKE,
			                          second_part_header,
			                          strlen (second_part_header));

			first_part_header = NULL;
			upload_data = NULL;
			second_part_header = NULL;

			priv->network_bytes_outstanding = priv->message->request_body->length;
		} else {
			soup_message_headers_set_content_type (priv->message->request_headers, priv->content_type, NULL);
		}

		/* Non-resumable uploads start with the data requests immediately. */
		priv->state = STATE_DATA_REQUESTS;
	} else {
		gchar *content_length_str;

		/* Resumable upload's initial request */
		soup_message_headers_set_encoding (priv->message->request_headers, SOUP_ENCODING_CONTENT_LENGTH);
		soup_message_headers_replace (priv->message->request_headers, "X-Upload-Content-Type", priv->content_type);

		content_length_str = g_strdup_printf ("%" G_GOFFSET_FORMAT, priv->content_length);
		soup_message_headers_replace (priv->message->request_headers, "X-Upload-Content-Length", content_length_str);
		g_free (content_length_str);

		if (priv->entry != NULL) {
			GDataParsableClass *parsable_klass;
			gchar *content_type, *upload_data;

			parsable_klass = GDATA_PARSABLE_GET_CLASS (priv->entry);
			g_assert (parsable_klass->get_content_type != NULL);

			if (g_strcmp0 (parsable_klass->get_content_type (), "application/json") == 0) {
				upload_data = gdata_parsable_get_json (GDATA_PARSABLE (priv->entry));
			} else {
				upload_data = gdata_parsable_get_xml (GDATA_PARSABLE (priv->entry));
			}

			content_type = g_strdup_printf ("%s; charset=UTF-8",
			                                parsable_klass->get_content_type ());
			soup_message_headers_set_content_type (priv->message->request_headers,
			                                       content_type,
			                                       NULL);
			g_free (content_type);

			soup_message_body_append (priv->message->request_body,
			                          SOUP_MEMORY_TAKE,
			                          upload_data,
			                          strlen (upload_data));
			upload_data = NULL;

			priv->network_bytes_outstanding = priv->message->request_body->length;
		} else {
			soup_message_headers_set_content_length (priv->message->request_headers, 0);
		}

		/* Resumable uploads always start with an initial request, which either contains the XML or is empty. */
		priv->state = STATE_INITIAL_REQUEST;
		priv->chunk_size = MIN (priv->content_length, MAX_RESUMABLE_CHUNK_SIZE);
	}

	/* Make sure the headers are set. HACK: This should actually be in build_message(), but we have to work around
	 * http://code.google.com/a/google.com/p/apps-api-issues/issues/detail?id=3033 in GDataDocumentsService's append_query_headers(). */
	service_klass = GDATA_SERVICE_GET_CLASS (priv->service);
	if (service_klass->append_query_headers != NULL) {
		service_klass->append_query_headers (priv->service, priv->authorization_domain, priv->message);
	}

	/* If the entry exists and has an ETag, we assume we're updating the entry, so we can set the If-Match header */
	if (priv->entry != NULL && gdata_entry_get_etag (priv->entry) != NULL)
		soup_message_headers_append (priv->message->request_headers, "If-Match", gdata_entry_get_etag (priv->entry));

	/* Uploading doesn't actually start until the first call to write() */
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

	if (priv->authorization_domain != NULL)
		g_object_unref (priv->authorization_domain);
	priv->authorization_domain = NULL;

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

	g_mutex_clear (&(priv->response_mutex));
	g_cond_clear (&(priv->finished_cond));
	g_cond_clear (&(priv->write_cond));
	g_mutex_clear (&(priv->write_mutex));
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
		case PROP_AUTHORIZATION_DOMAIN:
			g_value_set_object (value, priv->authorization_domain);
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
		case PROP_CONTENT_LENGTH:
			g_value_set_int64 (value, priv->content_length);
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
		case PROP_AUTHORIZATION_DOMAIN:
			priv->authorization_domain = g_value_dup_object (value);
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
		case PROP_CONTENT_LENGTH:
			priv->content_length = g_value_get_int64 (value);
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
	g_mutex_lock (&(priv->write_mutex));
	*(data->cancelled) = TRUE;
	g_cond_signal (&(priv->write_cond));
	g_mutex_unlock (&(priv->write_mutex));
}

static gssize
gdata_upload_stream_write (GOutputStream *stream, const void *buffer, gsize count, GCancellable *cancellable, GError **error_out)
{
	GDataUploadStreamPrivate *priv = GDATA_UPLOAD_STREAM (stream)->priv;
	gssize length_written = -1;
	gulong cancelled_signal = 0, global_cancelled_signal = 0;
	gboolean cancelled = FALSE; /* must only be touched with ->write_mutex held */
	gsize old_total_network_bytes_written;
	CancelledData data;
	GError *error = NULL;

	/* Listen for cancellation events */
	data.upload_stream = GDATA_UPLOAD_STREAM (stream);
	data.cancelled = &cancelled;

	global_cancelled_signal = g_cancellable_connect (priv->cancellable, (GCallback) write_cancelled_cb, &data, NULL);

	if (cancellable != NULL)
		cancelled_signal = g_cancellable_connect (cancellable, (GCallback) write_cancelled_cb, &data, NULL);

	/* Check for an error and return if necessary */
	g_mutex_lock (&(priv->write_mutex));

	if (cancelled == TRUE) {
		g_assert (g_cancellable_set_error_if_cancelled (cancellable, &error) == TRUE ||
		          g_cancellable_set_error_if_cancelled (priv->cancellable, &error) == TRUE);
		g_mutex_unlock (&(priv->write_mutex));

		length_written = -1;
		goto done;
	}

	g_mutex_unlock (&(priv->write_mutex));

	/* Increment the number of bytes outstanding for the new write, and keep a record of the old number written so we know if the write's
	 * finished before we reach write_cond. */
	old_total_network_bytes_written = priv->total_network_bytes_written;
	priv->message_bytes_outstanding += count;

	/* Handle the more common case of the network thread already having been created first */
	if (priv->network_thread != NULL) {
		/* Push the new data into the buffer */
		gdata_buffer_push_data (priv->buffer, buffer, count);
		goto write;
	}

	/* Write out the first chunk of data, so there's guaranteed to be something in the buffer */
	gdata_buffer_push_data (priv->buffer, buffer, count);

	/* Create the thread and let the writing commence! */
	create_network_thread (GDATA_UPLOAD_STREAM (stream), &error);
	if (priv->network_thread == NULL) {
		length_written = -1;
		goto done;
	}

write:
	g_mutex_lock (&(priv->write_mutex));

	/* Wait for it to be written */
	while (priv->total_network_bytes_written - old_total_network_bytes_written < count && cancelled == FALSE && priv->state != STATE_FINISHED) {
		g_cond_wait (&(priv->write_cond), &(priv->write_mutex));
	}
	length_written = MIN (count, priv->total_network_bytes_written - old_total_network_bytes_written);

	/* Check for an error and return if necessary */
	if (cancelled == TRUE && length_written < 1) {
		/* Cancellation. */
		g_assert (g_cancellable_set_error_if_cancelled (cancellable, &error) == TRUE ||
		          g_cancellable_set_error_if_cancelled (priv->cancellable, &error) == TRUE);
		length_written = -1;
	} else if (priv->state == STATE_FINISHED && (length_written < 0 || (gsize) length_written < count)) {
		/* Resumable upload error. */
		g_set_error (&error, G_IO_ERROR, G_IO_ERROR_FAILED, _("Error received from server after uploading a resumable upload chunk."));
		length_written = -1;
	}

	g_mutex_unlock (&(priv->write_mutex));

done:
	/* Disconnect from the cancelled signals. Note that we have to do this with @write_mutex not held, as g_cancellable_disconnect() blocks
	 * until any outstanding cancellation callbacks return, and they will block on @write_mutex. */
	if (cancelled_signal != 0)
		g_cancellable_disconnect (cancellable, cancelled_signal);
	if (global_cancelled_signal != 0)
		g_cancellable_disconnect (priv->cancellable, global_cancelled_signal);

	g_assert (error != NULL || length_written > 0);

	if (error != NULL) {
		g_propagate_error (error_out, error);
	}

	return length_written;
}

static void
flush_cancelled_cb (GCancellable *cancellable, CancelledData *data)
{
	GDataUploadStreamPrivate *priv = data->upload_stream->priv;

	/* Signal the gdata_upload_stream_flush() function that it should stop blocking and cancel */
	g_mutex_lock (&(priv->write_mutex));
	*(data->cancelled) = TRUE;
	g_cond_signal (&(priv->write_cond));
	g_mutex_unlock (&(priv->write_mutex));
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

	/* Create the thread if it hasn't been created already. This can happen if flush() is called immediately after creating the stream. */
	if (priv->network_thread == NULL) {
		create_network_thread (GDATA_UPLOAD_STREAM (stream), error);
		if (priv->network_thread == NULL) {
			success = FALSE;
			goto done;
		}
	}

	/* Start the flush operation proper */
	g_mutex_lock (&(priv->write_mutex));

	/* Wait for all outstanding bytes to be written to the network */
	while (priv->network_bytes_outstanding > 0 && cancelled == FALSE && priv->state != STATE_FINISHED) {
		g_cond_wait (&(priv->write_cond), &(priv->write_mutex));
	}

	/* Check for an error and return if necessary */
	if (cancelled == TRUE) {
		g_assert (g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE ||
		          g_cancellable_set_error_if_cancelled (priv->cancellable, error) == TRUE);
		success = FALSE;
	} else if (priv->state == STATE_FINISHED && priv->network_bytes_outstanding > 0) {
		/* Resumable upload error. */
		g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, _("Error received from server after uploading a resumable upload chunk."));
		success = FALSE;
	}

	g_mutex_unlock (&(priv->write_mutex));

done:
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
	g_mutex_lock (&(priv->response_mutex));
	*(data->cancelled) = TRUE;
	g_cond_signal (&(priv->finished_cond));
	g_mutex_unlock (&(priv->response_mutex));
}

/* It's guaranteed that we have set ->response_status and ->response_error and are done with *all* network activity before this returns, unless it's
 * cancelled. This means that it's safe to call gdata_upload_stream_get_response() once a call to close() has returned without being cancelled.
 *
 * Even though calling g_output_stream_close() multiple times on this stream is guaranteed to call gdata_upload_stream_close() at most once, other
 * GIO methods (notably g_output_stream_splice()) can call gdata_upload_stream_close() directly. Consequently, we need to be careful to be idempotent
 * after the first call.
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
	gboolean is_finished;
	GError *child_error = NULL;

	/* If the operation was never started, return successfully immediately */
	if (priv->network_thread == NULL)
		return TRUE;

	/* If we've already closed the stream, return G_IO_ERROR_CLOSED */
	g_mutex_lock (&(priv->response_mutex));

	if (priv->response_status != SOUP_STATUS_NONE) {
		g_mutex_unlock (&(priv->response_mutex));
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_CLOSED, _("Stream is already closed"));
		return FALSE;
	}

	g_mutex_unlock (&(priv->response_mutex));

	/* Allow cancellation */
	data.upload_stream = GDATA_UPLOAD_STREAM (stream);
	data.cancelled = &cancelled;

	global_cancelled_signal = g_cancellable_connect (priv->cancellable, (GCallback) close_cancelled_cb, &data, NULL);

	if (cancellable != NULL)
		cancelled_signal = g_cancellable_connect (cancellable, (GCallback) close_cancelled_cb, &data, NULL);

	g_mutex_lock (&(priv->response_mutex));

	g_mutex_lock (&(priv->write_mutex));
	is_finished = (priv->state == STATE_FINISHED);
	g_mutex_unlock (&(priv->write_mutex));

	/* If an operation is still in progress, the upload thread hasn't finished yet… */
	if (!is_finished) {
		/* We've reached the end of the stream, so append the footer if the entire operation hasn't been cancelled. */
		if (priv->entry != NULL && g_cancellable_is_cancelled (priv->cancellable) == FALSE) {
			const gchar *footer = "\n--" BOUNDARY_STRING "--";
			gsize footer_length = strlen (footer);

			gdata_buffer_push_data (priv->buffer, (const guint8*) footer, footer_length);

			g_mutex_lock (&(priv->write_mutex));
			priv->message_bytes_outstanding += footer_length;
			g_mutex_unlock (&(priv->write_mutex));
		}

		/* Mark the buffer as having reached EOF, and the write operation will close in its own time */
		gdata_buffer_push_data (priv->buffer, NULL, 0);

		/* Wait for the signal that we've finished. Cancelling the call to gdata_upload_stream_close() will cause this wait to be aborted,
		 * but won't actually prevent the stream being closed (i.e. all it means is that the stream isn't guaranteed to have been closed by
		 * the time gdata_upload_stream_close() returns — whereas normally it would be). */
		if (cancelled == FALSE) {
			g_cond_wait (&(priv->finished_cond), &(priv->response_mutex));
		}
	}

	g_assert (priv->response_status == SOUP_STATUS_NONE);
	g_assert (priv->response_error == NULL);

	g_mutex_lock (&(priv->write_mutex));
	is_finished = (priv->state == STATE_FINISHED);
	g_mutex_unlock (&(priv->write_mutex));

	/* Error handling */
	if (!is_finished && cancelled == TRUE) {
		/* Cancelled? If ->state == STATE_FINISHED, the network activity finished before the gdata_upload_stream_close() operation was
		 * cancelled, so we don't need to return an error. */
		g_assert (g_cancellable_set_error_if_cancelled (cancellable, &child_error) == TRUE ||
		          g_cancellable_set_error_if_cancelled (priv->cancellable, &child_error) == TRUE);
		priv->response_status = SOUP_STATUS_CANCELLED;
		success = FALSE;
	} else if (SOUP_STATUS_IS_SUCCESSFUL (priv->message->status_code) == FALSE) {
		GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (priv->service);

		/* Parse the error */
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (priv->service, GDATA_OPERATION_UPLOAD, priv->message->status_code, priv->message->reason_phrase,
		                             priv->message->response_body->data, priv->message->response_body->length, &child_error);
		priv->response_status = priv->message->status_code;
		success = FALSE;
	} else {
		/* Success! Set the response status */
		priv->response_status = priv->message->status_code;
	}

	g_assert (priv->response_status != SOUP_STATUS_NONE && (SOUP_STATUS_IS_SUCCESSFUL (priv->response_status) || child_error != NULL));

	g_mutex_unlock (&(priv->response_mutex));

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
	gboolean has_network_bytes_outstanding, is_complete;
	gsize length;
	gboolean reached_eof = FALSE;
	guint8 next_buffer[CHUNK_SIZE];

	g_mutex_lock (&(priv->write_mutex));
	has_network_bytes_outstanding = (priv->network_bytes_outstanding > 0);
	is_complete = (priv->state == STATE_INITIAL_REQUEST ||
	               (priv->content_length != -1 && priv->network_bytes_written + priv->network_bytes_outstanding == priv->chunk_size));
	g_mutex_unlock (&(priv->write_mutex));

	/* If there are still bytes in libsoup's buffer, don't block on getting new bytes into the stream. Also, if we're making the initial request
	 * of a resumable upload, don't push new data onto the network, since all of the XML was pushed into the buffer when we started. */
	if (has_network_bytes_outstanding) {
		return;
	} else if (is_complete) {
		soup_message_body_complete (priv->message->request_body);

		return;
	}

	/* Append the next chunk to the message body so it can join in the fun.
	 * Note that this call isn't necessarily blocking, and can return less than the CHUNK_SIZE. This is because
	 * we could deadlock if we block on getting CHUNK_SIZE bytes at the end of the stream. write() could
	 * easily be called with fewer bytes, but has no way to notify us that we've reached the end of the
	 * stream, so we'd happily block on receiving more bytes which weren't forthcoming.
	 *
	 * Note also that we can't block on this call with write_mutex locked, or we could get into a deadlock if the stream is flushed at the same
	 * time (in the case that we don't know the content length ahead of time). */
	if (priv->content_length == -1) {
		/* Non-resumable upload. */
		length = gdata_buffer_pop_data_limited (priv->buffer, next_buffer, CHUNK_SIZE, &reached_eof);
	} else {
		/* Resumable upload. Ensure we don't exceed the chunk size. */
		length = gdata_buffer_pop_data_limited (priv->buffer, next_buffer,
		                                        MIN (CHUNK_SIZE, priv->chunk_size - (priv->network_bytes_written +
		                                                                             priv->network_bytes_outstanding)), &reached_eof);
	}

	g_mutex_lock (&(priv->write_mutex));

	priv->message_bytes_outstanding -= length;
	priv->network_bytes_outstanding += length;

	/* Append whatever data was returned */
	if (length > 0)
		soup_message_body_append (priv->message->request_body, SOUP_MEMORY_COPY, next_buffer, length);

	/* Finish off the request body if we've reached EOF (i.e. the stream has been closed), or if we're doing a resumable upload and we reach
	 * the maximum chunk size. */
	if (reached_eof == TRUE ||
	    (priv->content_length != -1 && priv->network_bytes_written + priv->network_bytes_outstanding == priv->chunk_size)) {
		g_assert (reached_eof == FALSE || priv->message_bytes_outstanding == 0);

		soup_message_body_complete (priv->message->request_body);
	}

	g_mutex_unlock (&(priv->write_mutex));
}

static void
wrote_headers_cb (SoupMessage *message, GDataUploadStream *self)
{
	GDataUploadStreamPrivate *priv = self->priv;

	/* Signal the main thread that the headers have been written */
	g_mutex_lock (&(priv->write_mutex));
	g_cond_signal (&(priv->write_cond));
	g_mutex_unlock (&(priv->write_mutex));

	/* Send the first chunk to libsoup */
	write_next_chunk (self, message);
}

static void
wrote_body_data_cb (SoupMessage *message, SoupBuffer *buffer, GDataUploadStream *self)
{
	GDataUploadStreamPrivate *priv = self->priv;

	/* Signal the main thread that the chunk has been written */
	g_mutex_lock (&(priv->write_mutex));
	g_assert (priv->network_bytes_outstanding > 0);
	priv->network_bytes_outstanding -= buffer->length;
	priv->network_bytes_written += buffer->length;

	if (priv->state == STATE_DATA_REQUESTS) {
		priv->total_network_bytes_written += buffer->length;
	}

	g_cond_signal (&(priv->write_cond));
	g_mutex_unlock (&(priv->write_mutex));

	/* Send the next chunk to libsoup */
	write_next_chunk (self, message);
}

static gpointer
upload_thread (GDataUploadStream *self)
{
	GDataUploadStreamPrivate *priv = self->priv;
	GDataAuthorizer *authorizer;

	g_assert (priv->cancellable != NULL);

	/* FIXME: Refresh authorization before sending message in order to prevent authorization errors during transfer.
	 * See: https://gitlab.gnome.org/GNOME/libgdata/issues/23 */
	authorizer = gdata_service_get_authorizer (priv->service);
	if (authorizer) {
		g_autoptr(GError) error = NULL;

		gdata_authorizer_refresh_authorization (authorizer, priv->cancellable, &error);
		if (error != NULL)
			g_debug ("Error returned when refreshing authorization: %s", error->message);
		else
			gdata_authorizer_process_request (authorizer, priv->authorization_domain, priv->message);
	}

	while (TRUE) {
		GDataServiceClass *klass;
		gulong wrote_headers_signal, wrote_body_data_signal;
		gchar *new_uri;
		SoupMessage *new_message;
		gsize next_chunk_length;

		/* Connect to the wrote-* signals so we can prepare the next chunk for transmission */
		wrote_headers_signal = g_signal_connect (priv->message, "wrote-headers", (GCallback) wrote_headers_cb, self);
		wrote_body_data_signal = g_signal_connect (priv->message, "wrote-body-data", (GCallback) wrote_body_data_cb, self);

		_gdata_service_actually_send_message (priv->session, priv->message, priv->cancellable, NULL);

		g_mutex_lock (&(priv->write_mutex));

		/* If this is a resumable upload, continue to the next chunk. If it's a non-resumable upload, we're done. We have several cases:
		 *  • Non-resumable upload:
		 *     - Content only: STATE_DATA_REQUESTS → STATE_FINISHED
		 *     - Metadata only: not supported
		 *     - Content and metadata: STATE_DATA_REQUESTS → STATE_FINISHED
		 *  • Resumable upload:
		 *     - Content only:
		 *        * STATE_INITIAL_REQUEST → STATE_DATA_REQUESTS
		 *        * STATE_DATA_REQUESTS → STATE_DATA_REQUESTS
		 *        * STATE_DATA_REQUESTS → STATE_FINISHED
		 *     - Metadata only: STATE_INITIAL_REQUEST → STATE_FINISHED
		 *     - Content and metadata:
		 *        * STATE_INITIAL_REQUEST → STATE_DATA_REQUESTS
		 *        * STATE_DATA_REQUESTS → STATE_DATA_REQUESTS
		 *        * STATE_DATA_REQUESTS → STATE_FINISHED
		 */
		switch (priv->state) {
			case STATE_INITIAL_REQUEST:
				/* We're either a content-only or a content-and-metadata resumable upload. */
				priv->state = STATE_DATA_REQUESTS;

				/* Check the response. On success it should be empty, status 200, with a Location header telling us where to upload
				 * next. If it's an error response, bail out and let the code in gdata_upload_stream_close() parse the error..*/
				if (!SOUP_STATUS_IS_SUCCESSFUL (priv->message->status_code)) {
					goto finished;
				} else if (priv->content_length == 0 && priv->message->status_code == SOUP_STATUS_CREATED) {
					/* If this was a metadata-only upload, we're done. */
					goto finished;
				}

				/* Fall out and prepare the next message */
				g_assert (priv->total_network_bytes_written == 0); /* haven't written any data yet */

				break;
			case STATE_DATA_REQUESTS:
				/* Check the response. On completion it should contain the resulting entry's XML, status 201. On continuation it should
				 * be empty, status 308, with a Range header and potentially a Location header telling us what/where to upload next.
				 * If it's an error response, bail out and let the code in gdata_upload_stream_close() parse the error..*/
				if (priv->message->status_code == 308) {
					/* Continuation: fall out and prepare the next message */
					g_assert (priv->content_length == -1 || priv->total_network_bytes_written < (gsize) priv->content_length);
				} else if (SOUP_STATUS_IS_SUCCESSFUL (priv->message->status_code)) {
					/* Completion. Check the server isn't misbehaving. */
					g_assert (priv->content_length == -1 || priv->total_network_bytes_written == (gsize) priv->content_length);

					goto finished;
				} else {
					/* Error */
					goto finished;
				}

				/* Fall out and prepare the next message */
				g_assert (priv->total_network_bytes_written > 0);

				break;
			case STATE_FINISHED:
			default:
				g_assert_not_reached ();
		}

		/* Prepare the next message. */
		g_assert (priv->content_length != -1);

		next_chunk_length = MIN (priv->content_length - priv->total_network_bytes_written, MAX_RESUMABLE_CHUNK_SIZE);

		new_uri = g_strdup (soup_message_headers_get_one (priv->message->response_headers, "Location"));
		if (new_uri == NULL) {
			new_uri = soup_uri_to_string (soup_message_get_uri (priv->message), FALSE);
		}

		new_message = build_message (self, SOUP_METHOD_PUT, new_uri);

		g_free (new_uri);

		soup_message_headers_set_encoding (new_message->request_headers, SOUP_ENCODING_CONTENT_LENGTH);
		soup_message_headers_set_content_type (new_message->request_headers, priv->content_type, NULL);
		soup_message_headers_set_content_length (new_message->request_headers, next_chunk_length);
		soup_message_headers_set_content_range (new_message->request_headers, priv->total_network_bytes_written,
		                                        priv->total_network_bytes_written + next_chunk_length - 1, priv->content_length);

		/* Make sure the headers are set. HACK: This should actually be in build_message(), but we have to work around
		 * http://code.google.com/a/google.com/p/apps-api-issues/issues/detail?id=3033 in GDataDocumentsService's append_query_headers(). */
		klass = GDATA_SERVICE_GET_CLASS (priv->service);
		if (klass->append_query_headers != NULL) {
			klass->append_query_headers (priv->service, priv->authorization_domain, new_message);
		}

		g_signal_handler_disconnect (priv->message, wrote_body_data_signal);
		g_signal_handler_disconnect (priv->message, wrote_headers_signal);

		g_object_unref (priv->message);
		priv->message = new_message;

		/* Reset various counters for the next upload. Note that message_bytes_outstanding may be > 0 at this point, since the client may
		 * have pushed some content into the buffer while we were waiting for the response to this request. */
		g_assert (priv->network_bytes_outstanding == 0);
		priv->chunk_size = next_chunk_length;
		priv->network_bytes_written = 0;

		/* Loop round and upload this chunk now. */
		g_mutex_unlock (&(priv->write_mutex));

		continue;

finished:
		g_mutex_unlock (&(priv->write_mutex));

		goto finished_outer;
	}

finished_outer:
	/* Signal that the operation has finished (either successfully or in error).
	 * Also signal write_cond, just in case we errored out and finished sending in the middle of a write. */
	g_mutex_lock (&(priv->write_mutex));
	priv->state = STATE_FINISHED;
	g_cond_signal (&(priv->write_cond));
	g_mutex_unlock (&(priv->write_mutex));

	g_cond_signal (&(priv->finished_cond));

	/* Referenced in create_network_thread(). */
	g_object_unref (self);

	return NULL;
}

static void
create_network_thread (GDataUploadStream *self, GError **error)
{
	GDataUploadStreamPrivate *priv = self->priv;

	g_assert (priv->network_thread == NULL);
	g_object_ref (self); /* ownership transferred to thread */
	priv->network_thread = g_thread_try_new ("upload-thread", (GThreadFunc) upload_thread, self, error);
}

/**
 * gdata_upload_stream_new:
 * @service: a #GDataService
 * @domain: (allow-none): the #GDataAuthorizationDomain to authorize the upload, or %NULL
 * @method: the HTTP method to use
 * @upload_uri: the URI to upload, which must be HTTPS
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
 * Since: 0.9.0
 */
GOutputStream *
gdata_upload_stream_new (GDataService *service, GDataAuthorizationDomain *domain, const gchar *method, const gchar *upload_uri, GDataEntry *entry,
                         const gchar *slug, const gchar *content_type, GCancellable *cancellable)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain), NULL);
	g_return_val_if_fail (method != NULL, NULL);
	g_return_val_if_fail (upload_uri != NULL, NULL);
	g_return_val_if_fail (entry == NULL || GDATA_IS_ENTRY (entry), NULL);
	g_return_val_if_fail (slug != NULL, NULL);
	g_return_val_if_fail (content_type != NULL, NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);

	/* Create the upload stream */
	return G_OUTPUT_STREAM (g_object_new (GDATA_TYPE_UPLOAD_STREAM,
	                                      "method", method,
	                                      "upload-uri", upload_uri,
	                                      "service", service,
	                                      "authorization-domain", domain,
	                                      "entry", entry,
	                                      "slug", slug,
	                                      "content-type", content_type,
	                                      "cancellable", cancellable,
	                                      NULL));
}

/**
 * gdata_upload_stream_new_resumable:
 * @service: a #GDataService
 * @domain: (allow-none): the #GDataAuthorizationDomain to authorize the upload, or %NULL
 * @method: the HTTP method to use
 * @upload_uri: the URI to upload
 * @entry: (allow-none): the entry to upload as metadata, or %NULL
 * @slug: the file's slug (filename)
 * @content_type: the content type of the file being uploaded
 * @content_length: the size (in bytes) of the file being uploaded
 * @cancellable: (allow-none): a #GCancellable for the entire upload stream, or %NULL
 *
 * Creates a new resumable #GDataUploadStream, allowing a file to be uploaded from a GData service using standard #GOutputStream API. The upload will
 * use GData's resumable upload API, so should be more reliable than a normal upload (especially if the file is large). See the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/resumable_upload.html">GData documentation on resumable uploads</ulink> for more
 * information.
 *
 * The HTTP method to use should be specified in @method, and will typically be either %SOUP_METHOD_POST (for insertions) or %SOUP_METHOD_PUT
 * (for updates), according to the server and the @upload_uri.
 *
 * If @entry is specified, it will be attached to the upload as the entry to which the file being uploaded belongs. Otherwise, just the file
 * written to the stream will be uploaded, and given a default entry as determined by the server.
 *
 * @slug, @content_type and @content_length must be specified before the upload begins, as they describe the file being streamed. @slug is the filename
 * given to the file, which will typically be stored on the server and made available when downloading the file again. @content_type must be the
 * correct content type for the file, and should be in the service's list of acceptable content types. @content_length must be the size of the file
 * being uploaded (not including the XML for any associated #GDataEntry) in bytes. Zero is accepted if a metadata-only upload is being performed.
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
 * Since: 0.13.0
 */
GOutputStream *
gdata_upload_stream_new_resumable (GDataService *service, GDataAuthorizationDomain *domain, const gchar *method, const gchar *upload_uri,
                                   GDataEntry *entry, const gchar *slug, const gchar *content_type, goffset content_length, GCancellable *cancellable)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain), NULL);
	g_return_val_if_fail (method != NULL, NULL);
	g_return_val_if_fail (upload_uri != NULL, NULL);
	g_return_val_if_fail (entry == NULL || GDATA_IS_ENTRY (entry), NULL);
	g_return_val_if_fail (slug != NULL, NULL);
	g_return_val_if_fail (content_type != NULL, NULL);
	g_return_val_if_fail (content_length >= 0, NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);

	/* Create the upload stream */
	return G_OUTPUT_STREAM (g_object_new (GDATA_TYPE_UPLOAD_STREAM,
	                                      "method", method,
	                                      "upload-uri", upload_uri,
	                                      "service", service,
	                                      "authorization-domain", domain,
	                                      "entry", entry,
	                                      "slug", slug,
	                                      "content-type", content_type,
	                                      "content-length", content_length,
	                                      "cancellable", cancellable,
	                                      NULL));
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
 * been set before calling this function is to have closed the #GDataUploadStream by calling g_output_stream_close() on it, without cancelling
 * the close operation. Once the stream has been closed, all network communication is guaranteed to have finished. Note that if a call to
 * g_output_stream_close() is cancelled, g_output_stream_is_closed() will immediately start to return %TRUE, even if the #GDataUploadStream is still
 * attempting to flush the network buffers asynchronously — consequently, gdata_upload_stream_get_response() may still return %NULL and a @length of
 * <code class="literal">-1</code>. The only reliable way to determine if the stream has been fully closed in this situation is to check the results
 * of gdata_upload_stream_get_response(), rather than g_output_stream_is_closed().
 *
 * Return value: the server's response to the upload, or %NULL
 *
 * Since: 0.5.0
 */
const gchar *
gdata_upload_stream_get_response (GDataUploadStream *self, gssize *length)
{
	gssize _length;
	const gchar *_response;

	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), NULL);

	g_mutex_lock (&(self->priv->response_mutex));

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

	g_mutex_unlock (&(self->priv->response_mutex));

	if (length != NULL)
		*length = _length;
	return _response;
}

/**
 * gdata_upload_stream_get_service:
 * @self: a #GDataUploadStream
 *
 * Gets the service used to authorize the upload, as passed to gdata_upload_stream_new().
 *
 * Return value: (transfer none): the #GDataService used to authorize the upload
 *
 * Since: 0.5.0
 */
GDataService *
gdata_upload_stream_get_service (GDataUploadStream *self)
{
	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), NULL);
	return self->priv->service;
}

/**
 * gdata_upload_stream_get_authorization_domain:
 * @self: a #GDataUploadStream
 *
 * Gets the authorization domain used to authorize the upload, as passed to gdata_upload_stream_new(). It may be %NULL if authorization is not
 * needed for the upload.
 *
 * Return value: (transfer none) (allow-none): the #GDataAuthorizationDomain used to authorize the upload, or %NULL
 *
 * Since: 0.9.0
 */
GDataAuthorizationDomain *
gdata_upload_stream_get_authorization_domain (GDataUploadStream *self)
{
	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), NULL);
	return self->priv->authorization_domain;
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
 */
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
 */
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
 */
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
 */
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
 */
const gchar *
gdata_upload_stream_get_content_type (GDataUploadStream *self)
{
	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), NULL);
	return self->priv->content_type;
}

/**
 * gdata_upload_stream_get_content_length:
 * @self: a #GDataUploadStream
 *
 * Gets the size (in bytes) of the file being uploaded. This will be <code class="literal">-1</code> for a non-resumable upload, and zero or greater
 * for a resumable upload.
 *
 * Return value: the size of the file being uploaded
 *
 * Since: 0.13.0
 */
goffset
gdata_upload_stream_get_content_length (GDataUploadStream *self)
{
	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), -1);
	return self->priv->content_length;
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
 */
GCancellable *
gdata_upload_stream_get_cancellable (GDataUploadStream *self)
{
	g_return_val_if_fail (GDATA_IS_UPLOAD_STREAM (self), NULL);
	g_assert (self->priv->cancellable != NULL);
	return self->priv->cancellable;
}
