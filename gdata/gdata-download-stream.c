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
 * @stability: Stable
 * @include: gdata/gdata-download-stream.h
 *
 * #GDataDownloadStream is a #GInputStream subclass to allow downloading of files from GData services with authorization from a #GDataService under
 * the given #GDataAuthorizationDomain. If authorization is not required to perform the download, a #GDataAuthorizationDomain doesn't have to be
 * specified.
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
 * The entire download operation can be cancelled using the #GCancellable instance provided to gdata_download_stream_new(), or returned by
 * gdata_download_stream_get_cancellable(). Cancelling this at any time will cause all future #GInputStream method calls to return
 * %G_IO_ERROR_CANCELLED. If any #GInputStream methods are in the process of being called, they will be cancelled and return %G_IO_ERROR_CANCELLED as
 * soon as possible.
 *
 * Note that cancelling an individual method call (such as a call to g_input_stream_read()) using the #GCancellable parameter of the method will not
 * cancel the download as a whole — just that particular method call. In the case of g_input_stream_read(), this will cause it to successfully return
 * any data that it has in memory at the moment (up to the requested number of bytes), or return a %G_IO_ERROR_CANCELLED if it was blocking on receiving
 * data from the network. This is also the behaviour of g_input_stream_read() when the download operation as a whole is cancelled.
 *
 * In the case of g_input_stream_close(), the call will return immediately if network activity hasn't yet started. If it has, the network activity will
 * be cancelled, regardless of whether the call to g_input_stream_close() is cancelled. Cancelling a pending call to g_input_stream_close() (either
 * using the method's #GCancellable, or by cancelling the download stream as a whole) will cause it to stop waiting for the network activity to finish,
 * and return %G_IO_ERROR_CANCELLED immediately. Network activity will continue to be shut down in the background.
 *
 * If the server returns an error message (for example, if the user is not correctly authenticated/authorized or doesn't have suitable permissions to
 * download from the given URI), it will be returned as a #GDataServiceError by the first call to g_input_stream_read().
 *
 * <example>
 * 	<title>Downloading to a File</title>
 * 	<programlisting>
 *	GDataService *service;
 *	GDataAuthorizationDomain *domain;
 *	GCancellable *cancellable;
 *	GInputStream *download_stream;
 *	GOutputStream *output_stream;
 *
 *	/<!-- -->* Create the download stream *<!-- -->/
 *	service = create_my_service ();
 *	domain = get_my_authorization_domain_from_service (service);
 *	cancellable = g_cancellable_new (); /<!-- -->* cancel this to cancel the entire download operation *<!-- -->/
 *	download_stream = gdata_download_stream_new (service, domain, download_uri, cancellable);
 *	output_stream = create_file_and_return_output_stream ();
 *
 *	/<!-- -->* Perform the download asynchronously *<!-- -->/
 *	g_output_stream_splice_async (output_stream, download_stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
 *	                              G_PRIORITY_DEFAULT, NULL, (GAsyncReadyCallback) download_splice_cb, NULL);
 *
 *	g_object_unref (output_stream);
 *	g_object_unref (download_stream);
 *	g_object_unref (cancellable);
 *	g_object_unref (domain);
 *	g_object_unref (service);
 *
 *	static void
 *	download_splice_cb (GOutputStream *output_stream, GAsyncResult *result, gpointer user_data)
 *	{
 *		GError *error = NULL;
 *
 *		g_output_stream_splice_finish (output_stream, result, &error);
 *
 *		if (error != NULL && g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED) == FALSE)) {
 *			/<!-- -->* Error downloading the file; potentially an I/O error (GIOError), or an error response from the server
 *			 * (GDataServiceError). You might want to delete the newly created file because of the error. *<!-- -->/
 *			g_error ("Error downloading file: %s", error->message);
 *			g_error_free (error);
 *		}
 *	}
 * 	</programlisting>
 * </example>
 *
 * Since: 0.5.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

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
static void reset_network_thread (GDataDownloadStream *self);

/*
 * The GDataDownloadStream can be in one of several states:
 *  1. Pre-network activity. This is the state that the stream is created in. @network_thread and @cancellable are both %NULL, and @finished is %FALSE.
 *     The stream will remain in this state until gdata_download_stream_read() or gdata_download_stream_seek() are called for the first time.
 *     @content_type and @content_length are at their default values (NULL and -1, respectively).
 *  2. Network activity. This state is entered when gdata_download_stream_read() is called for the first time.
 *     @network_thread, @buffer and @cancellable are created, while @finished remains %FALSE.
 *     As soon as the headers are downloaded, which is guaranteed to be before the first call to gdata_download_stream_read() returns, @content_type
 *     and @content_length are set from the headers. From this point onwards, they are immutable.
 *  3. Reset network activity. This state is entered only if case 3 is encountered in a call to gdata_download_stream_seek(): a seek to an offset which
 *     has already been read out of the buffer. In this state, @buffer is freed and set to %NULL, @network_thread is cancelled (then set to %NULL),
 *     and @offset is set to the seeked-to offset. @finished remains at %FALSE.
 *     When the next call to gdata_download_stream_read() is made, the download stream will go back to state 2 as if this was the first call to
 *     gdata_download_stream_read().
 *  4. Post-network activity. This state is reached once the download thread finishes downloading, due to having downloaded everything.
 *     @buffer is non-%NULL, @network_thread is non-%NULL, but meaningless; @cancellable is still a valid #GCancellable instance; and @finished is set
 *     to %TRUE. At the same time, @finished_cond is signalled.
 *     This state can be exited either by making a call to gdata_download_stream_seek(), in which case the stream will go back to state 3; or by
 *     calling gdata_download_stream_close(), in which case the stream will return errors for all operations, as the underlying %GInputStream will be
 *     marked as closed.
 */
struct _GDataDownloadStreamPrivate {
	gchar *download_uri;
	GDataService *service;
	GDataAuthorizationDomain *authorization_domain;
	SoupSession *session;
	SoupMessage *message;
	GDataBuffer *buffer;
	goffset offset; /* current position in the stream */

	GThread *network_thread;
	GCancellable *cancellable;
	GCancellable *network_cancellable; /* see the comment in gdata_download_stream_constructor() about the relationship between these two */
	gulong network_cancellable_id;

	gboolean finished;
	GCond finished_cond;
	GMutex finished_mutex; /* mutex for ->finished, protected by ->finished_cond */

	/* Cached data from the SoupMessage */
	gchar *content_type;
	gssize content_length;
	GMutex content_mutex; /* mutex to protect them */
};

enum {
	PROP_SERVICE = 1,
	PROP_DOWNLOAD_URI,
	PROP_CONTENT_TYPE,
	PROP_CONTENT_LENGTH,
	PROP_CANCELLABLE,
	PROP_AUTHORIZATION_DOMAIN,
};

G_DEFINE_TYPE_WITH_CODE (GDataDownloadStream, gdata_download_stream, G_TYPE_INPUT_STREAM,
                         G_ADD_PRIVATE (GDataDownloadStream)
                         G_IMPLEMENT_INTERFACE (G_TYPE_SEEKABLE, gdata_download_stream_seekable_iface_init))

static void
gdata_download_stream_class_init (GDataDownloadStreamClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GInputStreamClass *stream_class = G_INPUT_STREAM_CLASS (klass);

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
	 * The service which is used to authorize the download, and to which the download relates.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_SERVICE,
	                                 g_param_spec_object ("service",
	                                                      "Service", "The service which is used to authorize the download.",
	                                                      GDATA_TYPE_SERVICE,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDownloadStream:authorization-domain:
	 *
	 * The authorization domain for the download, against which the #GDataService:authorizer for the #GDataDownloadStream:service should be
	 * authorized. This may be %NULL if authorization is not needed for the download.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_AUTHORIZATION_DOMAIN,
	                                 g_param_spec_object ("authorization-domain",
	                                                      "Authorization domain", "The authorization domain for the download.",
	                                                      GDATA_TYPE_AUTHORIZATION_DOMAIN,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDownloadStream:download-uri:
	 *
	 * The URI of the file to download. This must be HTTPS.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_DOWNLOAD_URI,
	                                 g_param_spec_string ("download-uri",
	                                                      "Download URI", "The URI of the file to download.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDownloadStream:content-type:
	 *
	 * The content type of the file being downloaded. This will initially be %NULL, and will be populated as soon as the appropriate header is
	 * received from the server. Its value will never change after this.
	 *
	 * Note that change notifications for this property (#GObject::notify emissions) may be emitted in threads other than the one which created
	 * the #GDataDownloadStream. It is the client's responsibility to ensure that any notification signal handlers are either multi-thread safe
	 * or marshal the notification to the thread which owns the #GDataDownloadStream as appropriate.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_CONTENT_TYPE,
	                                 g_param_spec_string ("content-type",
	                                                      "Content type", "The content type of the file being downloaded.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDownloadStream:content-length:
	 *
	 * The length (in bytes) of the file being downloaded. This will initially be <code class="literal">-1</code>, and will be populated as soon as
	 * the appropriate header is received from the server. Its value will never change after this.
	 *
	 * Note that change notifications for this property (#GObject::notify emissions) may be emitted in threads other than the one which created
	 * the #GDataDownloadStream. It is the client's responsibility to ensure that any notification signal handlers are either multi-thread safe
	 * or marshal the notification to the thread which owns the #GDataDownloadStream as appropriate.
	 *
	 * Since: 0.5.0
	 */
	g_object_class_install_property (gobject_class, PROP_CONTENT_LENGTH,
	                                 g_param_spec_long ("content-length",
	                                                    "Content length", "The length (in bytes) of the file being downloaded.",
	                                                    -1, G_MAXSSIZE, -1,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataDownloadStream:cancellable:
	 *
	 * An optional cancellable used to cancel the entire download operation. If a #GCancellable instance isn't provided for this property at
	 * construction time (i.e. to gdata_download_stream_new()), one will be created internally and can be retrieved using
	 * gdata_download_stream_get_cancellable() and used to cancel the download operation with g_cancellable_cancel() just as if it was passed to
	 * gdata_download_stream_new().
	 *
	 * If the download operation is cancelled using this #GCancellable, any ongoing network activity will be stopped, and any pending or future
	 * calls to #GInputStream API on the #GDataDownloadStream will return %G_IO_ERROR_CANCELLED. Note that the #GCancellable objects which can be
	 * passed to individual #GInputStream operations will not cancel the download operation proper if cancelled — they will merely cancel that API
	 * call. The only way to cancel the download operation completely is using #GDataDownloadStream:cancellable.
	 *
	 * Since: 0.8.0
	 */
	g_object_class_install_property (gobject_class, PROP_CANCELLABLE,
	                                 g_param_spec_object ("cancellable",
	                                                      "Cancellable", "An optional cancellable used to cancel the entire download operation.",
	                                                      G_TYPE_CANCELLABLE,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
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
	self->priv = gdata_download_stream_get_instance_private (self);
	self->priv->buffer = NULL; /* created when the network thread is started and destroyed when the stream is closed */

	self->priv->finished = FALSE;
	g_cond_init (&(self->priv->finished_cond));
	g_mutex_init (&(self->priv->finished_mutex));

	self->priv->content_type = NULL;
	self->priv->content_length = -1;
	g_mutex_init (&(self->priv->content_mutex));
}

static void
cancellable_cancel_cb (GCancellable *cancellable, GCancellable *network_cancellable)
{
	g_cancellable_cancel (network_cancellable);
}

static GObject *
gdata_download_stream_constructor (GType type, guint n_construct_params, GObjectConstructParam *construct_params)
{
	GDataDownloadStreamPrivate *priv;
	GDataServiceClass *klass;
	GObject *object;
	SoupURI *_uri;

	/* Chain up to the parent class */
	object = G_OBJECT_CLASS (gdata_download_stream_parent_class)->constructor (type, n_construct_params, construct_params);
	priv = GDATA_DOWNLOAD_STREAM (object)->priv;

	/* Create a #GCancellable for the network. Cancellation of ->cancellable is chained to this one, so that if ->cancellable is cancelled,
	 * ->network_cancellable is also cancelled. However, if ->network_cancellable is cancelled, the cancellation doesn't propagate back upwards
	 * to ->cancellable. This allows closing the stream part-way through a download to be implemented by cancelling ->network_cancellable, without
	 * causing ->cancellable to be unnecessarily cancelled (which would be a nasty side-effect of closing the stream early otherwise). */
	priv->network_cancellable = g_cancellable_new ();

	/* Create a #GCancellable for the entire download operation if one wasn't specified for #GDataDownloadStream:cancellable during construction */
	if (priv->cancellable == NULL)
		priv->cancellable = g_cancellable_new ();
	priv->network_cancellable_id = g_cancellable_connect (priv->cancellable, (GCallback) cancellable_cancel_cb, priv->network_cancellable, NULL);

	/* Build the message. The URI must be HTTPS. */
	_uri = soup_uri_new (priv->download_uri);
	soup_uri_set_port (_uri, _gdata_service_get_https_port ());
	g_assert_cmpstr (soup_uri_get_scheme (_uri), ==, SOUP_URI_SCHEME_HTTPS);
	priv->message = soup_message_new_from_uri (SOUP_METHOD_GET, _uri);
	soup_uri_free (_uri);

	/* Make sure the headers are set */
	klass = GDATA_SERVICE_GET_CLASS (priv->service);
	if (klass->append_query_headers != NULL) {
		klass->append_query_headers (priv->service, priv->authorization_domain, priv->message);
	}

	/* We don't want to accumulate chunks */
	soup_message_body_set_accumulate (priv->message->request_body, FALSE);

	/* Downloading doesn't actually start until the first call to read() */

	return object;
}

static void
gdata_download_stream_dispose (GObject *object)
{
	GDataDownloadStreamPrivate *priv = GDATA_DOWNLOAD_STREAM (object)->priv;

	/* Block on closing the stream */
	g_input_stream_close (G_INPUT_STREAM (object), NULL, NULL);

	if (priv->cancellable != NULL) {
		if (priv->network_cancellable_id != 0) {
			g_cancellable_disconnect (priv->cancellable, priv->network_cancellable_id);
		}

		g_object_unref (priv->cancellable);
	}

	priv->network_cancellable_id = 0;
	priv->cancellable = NULL;

	if (priv->network_cancellable != NULL)
		g_object_unref (priv->network_cancellable);
	priv->network_cancellable = NULL;

	if (priv->authorization_domain != NULL)
		g_object_unref (priv->authorization_domain);
	priv->authorization_domain = NULL;

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

	reset_network_thread (GDATA_DOWNLOAD_STREAM (object));

	g_cond_clear (&(priv->finished_cond));
	g_mutex_clear (&(priv->finished_mutex));

	g_mutex_clear (&(priv->content_mutex));

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
		case PROP_AUTHORIZATION_DOMAIN:
			g_value_set_object (value, priv->authorization_domain);
			break;
		case PROP_DOWNLOAD_URI:
			g_value_set_string (value, priv->download_uri);
			break;
		case PROP_CONTENT_TYPE:
			g_mutex_lock (&(priv->content_mutex));
			g_value_set_string (value, priv->content_type);
			g_mutex_unlock (&(priv->content_mutex));
			break;
		case PROP_CONTENT_LENGTH:
			g_mutex_lock (&(priv->content_mutex));
			g_value_set_long (value, priv->content_length);
			g_mutex_unlock (&(priv->content_mutex));
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
gdata_download_stream_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataDownloadStreamPrivate *priv = GDATA_DOWNLOAD_STREAM (object)->priv;

	switch (property_id) {
		case PROP_SERVICE:
			priv->service = g_value_dup_object (value);
			priv->session = _gdata_service_get_session (priv->service);
			break;
		case PROP_AUTHORIZATION_DOMAIN:
			priv->authorization_domain = g_value_dup_object (value);
			break;
		case PROP_DOWNLOAD_URI:
			priv->download_uri = g_value_dup_string (value);
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

static void
read_cancelled_cb (GCancellable *cancellable, GCancellable *child_cancellable)
{
	g_cancellable_cancel (child_cancellable);
}

static gssize
gdata_download_stream_read (GInputStream *stream, void *buffer, gsize count, GCancellable *cancellable, GError **error)
{
	GDataDownloadStreamPrivate *priv = GDATA_DOWNLOAD_STREAM (stream)->priv;
	gssize length_read = -1;
	gboolean reached_eof = FALSE;
	gulong cancelled_signal = 0, global_cancelled_signal = 0;
	GCancellable *child_cancellable;
	GError *child_error = NULL;

	/* Listen for cancellation from either @cancellable or @priv->cancellable. We have to multiplex cancellation signals from the two sources
	 * into a single #GCancellabel, @child_cancellable. */
	child_cancellable = g_cancellable_new ();

	global_cancelled_signal = g_cancellable_connect (priv->cancellable, (GCallback) read_cancelled_cb, child_cancellable, NULL);

	if (cancellable != NULL)
		cancelled_signal = g_cancellable_connect (cancellable, (GCallback) read_cancelled_cb, child_cancellable, NULL);

	/* We're lazy about starting the network operation so we don't end up with a massive buffer */
	if (priv->network_thread == NULL) {
		/* Handle early cancellation so that we don't create the network thread unnecessarily */
		if (g_cancellable_set_error_if_cancelled (child_cancellable, &child_error) == TRUE) {
			length_read = -1;
			goto done;
		}

		/* Create the network thread */
		create_network_thread (GDATA_DOWNLOAD_STREAM (stream), &child_error);
		if (priv->network_thread == NULL) {
			length_read = -1;
			goto done;
		}
	}

	/* Read the data off the buffer. If the operation is cancelled, it'll probably still return a positive number of bytes read — if it does, we
	 * can return without error. Iff it returns a non-positive number of bytes should we return an error. */
	g_assert (priv->buffer != NULL);
	length_read = (gssize) gdata_buffer_pop_data (priv->buffer, buffer, count, &reached_eof, child_cancellable);

	if (length_read < 1 && g_cancellable_set_error_if_cancelled (child_cancellable, &child_error) == TRUE) {
		/* Handle cancellation */
		length_read = -1;

		goto done;
	} else if (SOUP_STATUS_IS_SUCCESSFUL (priv->message->status_code) == FALSE) {
		GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (priv->service);

		/* Set an appropriate error */
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (priv->service, GDATA_OPERATION_DOWNLOAD, priv->message->status_code, priv->message->reason_phrase,
		                             NULL, 0, &child_error);
		length_read = -1;

		goto done;
	}

done:
	/* Disconnect from the cancelled signals. */
	if (cancelled_signal != 0)
		g_cancellable_disconnect (cancellable, cancelled_signal);
	if (global_cancelled_signal != 0)
		g_cancellable_disconnect (priv->cancellable, global_cancelled_signal);

	g_object_unref (child_cancellable);

	g_assert ((reached_eof == FALSE && length_read > 0 && length_read <= (gssize) count && child_error == NULL) ||
	          (reached_eof == TRUE && length_read >= 0 && length_read <= (gssize) count && child_error == NULL) ||
	          (length_read == -1 && child_error != NULL));

	if (child_error != NULL)
		g_propagate_error (error, child_error);

	/* Update our internal offset */
	if (length_read > 0) {
		priv->offset += length_read;
	}

	return length_read;
}

typedef struct {
	GDataDownloadStream *download_stream;
	gboolean *cancelled;
} CancelledData;

static void
close_cancelled_cb (GCancellable *cancellable, CancelledData *data)
{
	GDataDownloadStreamPrivate *priv = data->download_stream->priv;

	g_mutex_lock (&(priv->finished_mutex));
	*(data->cancelled) = TRUE;
	g_cond_signal (&(priv->finished_cond));
	g_mutex_unlock (&(priv->finished_mutex));
}

/* Even though calling g_input_stream_close() multiple times on this stream is guaranteed to call gdata_download_stream_close() at most once, other
 * GIO methods (notably g_output_stream_splice()) can call gdata_download_stream_close() directly. Consequently, we need to be careful to be idempotent
 * after the first call.
 *
 * If the network thread hasn't yet been started (i.e. gdata_download_stream_read() hasn't been called at all yet), %TRUE will be returned immediately.
 *
 * If the global cancellable, ->cancellable, or @cancellable are cancelled before the call to gdata_download_stream_close(),
 * gdata_download_stream_close() should return immediately with %G_IO_ERROR_CANCELLED. If they're cancelled during the call,
 * gdata_download_stream_close() should stop waiting for any outstanding network activity to finish and return %G_IO_ERROR_CANCELLED (though the
 * operation to finish off network activity and close the stream will still continue).
 *
 * If the call to gdata_download_stream_close() is not cancelled by any #GCancellable, it will cancel the ongoing network activity, and wait until
 * the operation has been cleaned up before returning success. */
static gboolean
gdata_download_stream_close (GInputStream *stream, GCancellable *cancellable, GError **error)
{
	GDataDownloadStreamPrivate *priv = GDATA_DOWNLOAD_STREAM (stream)->priv;
	gulong cancelled_signal = 0, global_cancelled_signal = 0;
	gboolean cancelled = FALSE; /* must only be touched with ->finished_mutex held */
	gboolean success = TRUE;
	CancelledData data;
	GError *child_error = NULL;

	/* If the operation was never started, return successfully immediately */
	if (priv->network_thread == NULL) {
		goto done;
	}

	/* Allow cancellation */
	data.download_stream = GDATA_DOWNLOAD_STREAM (stream);
	data.cancelled = &cancelled;

	global_cancelled_signal = g_cancellable_connect (priv->cancellable, (GCallback) close_cancelled_cb, &data, NULL);

	if (cancellable != NULL)
		cancelled_signal = g_cancellable_connect (cancellable, (GCallback) close_cancelled_cb, &data, NULL);

	g_mutex_lock (&(priv->finished_mutex));

	/* If the operation has started but hasn't already finished, cancel the network thread and wait for it to finish before returning */
	if (priv->finished == FALSE) {
		g_cancellable_cancel (priv->network_cancellable);

		/* Allow the close() call to be cancelled by cancelling either @cancellable or ->cancellable. Note that this won't prevent the stream
		 * from continuing to be closed in the background — it'll just stop waiting on the operation to finish being cancelled. */
		if (cancelled == FALSE) {
			g_cond_wait (&(priv->finished_cond), &(priv->finished_mutex));
		}
	}

	/* Error handling */
	if (priv->finished == FALSE && cancelled == TRUE) {
		/* Cancelled? If ->finished is TRUE, the network activity finished before the gdata_download_stream_close() operation was cancelled,
		 * so we don't need to return an error. */
		g_assert (g_cancellable_set_error_if_cancelled (cancellable, &child_error) == TRUE ||
		          g_cancellable_set_error_if_cancelled (priv->cancellable, &child_error) == TRUE);
		success = FALSE;
	}

	g_mutex_unlock (&(priv->finished_mutex));

	/* Disconnect from the signal handlers. Note that we have to do this without @finished_mutex held, as g_cancellable_disconnect() blocks
	 * until any outstanding cancellation callbacks return, and they will block on @finished_mutex. */
	if (cancelled_signal != 0)
		g_cancellable_disconnect (cancellable, cancelled_signal);
	if (global_cancelled_signal != 0)
		g_cancellable_disconnect (priv->cancellable, global_cancelled_signal);

done:
	/* If we were successful, tidy up various bits of state */
	g_mutex_lock (&(priv->finished_mutex));

	if (success == TRUE && priv->finished == TRUE) {
		reset_network_thread (GDATA_DOWNLOAD_STREAM (stream));
	}

	g_mutex_unlock (&(priv->finished_mutex));

	g_assert ((success == TRUE && child_error == NULL) || (success == FALSE && child_error != NULL));

	if (child_error != NULL)
		g_propagate_error (error, child_error);

	return success;
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

static gboolean
gdata_download_stream_seek (GSeekable *seekable, goffset offset, GSeekType type, GCancellable *cancellable, GError **error)
{
	GDataDownloadStreamPrivate *priv = GDATA_DOWNLOAD_STREAM (seekable)->priv;
	GError *child_error = NULL;

	if (type == G_SEEK_END && priv->content_length == -1) {
		/* If we don't have the Content-Length, we can't calculate the offset from the start of the stream properly, so just give up.
		 * We could technically use a HEAD request to get the Content-Length, but this hasn't been tried yet (FIXME). */
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED, "G_SEEK_END not currently supported");
		return FALSE;
	}

	if (g_input_stream_set_pending (G_INPUT_STREAM (seekable), error) == FALSE) {
		return FALSE;
	}

	/* Ensure that offset is relative to the start of the stream. */
	switch (type) {
		case G_SEEK_CUR:
			offset += priv->offset;
			break;
		case G_SEEK_SET:
			/* Nothing needs doing */
			break;
		case G_SEEK_END:
			offset += priv->content_length;
			break;
		default:
			g_assert_not_reached ();
	}

	/* There are three cases to consider:
	 *  1. The network thread hasn't been started. In this case, we need to set the offset and do nothing. When the network thread is started
	 *     (in the next read() call), a Range header will be set on it which will give the correct seek.
	 *  2. The network thread has been started and the seek is to a position greater than our current position (i.e. one which already does, or
	 *     will soon, exist in the buffer). In this case, we need to pop the intervening bytes off the buffer (which may block) and update the
	 *     offset.
	 *  3. The network thread has been started and the seek is to a position which has already been popped off the buffer. In this case, we need
	 *     to set the offset and cancel the network thread. When the network thread is restarted (in the next read() call), a Range header will
	 *     be set on it which will give the correct seek.
	 */

	if (priv->network_thread == NULL) {
		/* Case 1. Set the offset and we're done. */
		priv->offset = offset;

		goto done;
	}

	/* Cases 2 and 3. The network thread has already been started. */
	if (offset >= priv->offset) {
		goffset num_intervening_bytes;
		gssize length_read;

		/* Case 2. Pop off the intervening bytes and update the offset. If we can't pop enough bytes off, we throw an error. */
		num_intervening_bytes = offset - priv->offset;
		g_assert (priv->buffer != NULL);
		length_read = (gssize) gdata_buffer_pop_data (priv->buffer, NULL, num_intervening_bytes, NULL, cancellable);

		if (length_read != num_intervening_bytes) {
			if (g_cancellable_set_error_if_cancelled (cancellable, &child_error) == FALSE) {
				/* Tried to seek too far */
				g_set_error_literal (&child_error, G_IO_ERROR, G_IO_ERROR_INVALID_ARGUMENT, _("Invalid seek request"));
			}

			goto done;
		}

		/* Update the offset */
		priv->offset = offset;

		goto done;
	} else {
		/* Case 3. Cancel the current network thread. Note that we don't allow cancellation of this call, as we depend on it waiting for
		 * the network thread to join. */
		if (gdata_download_stream_close (G_INPUT_STREAM (seekable), NULL, &child_error) == FALSE) {
			goto done;
		}

		/* Update the offset */
		priv->offset = offset;

		/* Mark the thread as unfinished */
		g_mutex_lock (&(priv->finished_mutex));
		priv->finished = FALSE;
		g_mutex_unlock (&(priv->finished_mutex));

		goto done;
	}

done:
	g_input_stream_clear_pending (G_INPUT_STREAM (seekable));

	if (child_error != NULL) {
		g_propagate_error (error, child_error);
		return FALSE;
	}

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
	goffset end;
	goffset start;
	goffset total_length;

	/* Don't get the client's hopes up by setting the Content-Type or -Length if the response
	 * is actually unsuccessful. */
	if (SOUP_STATUS_IS_SUCCESSFUL (message->status_code) == FALSE)
		return;

	g_mutex_lock (&(self->priv->content_mutex));
	self->priv->content_type = g_strdup (soup_message_headers_get_content_type (message->response_headers, NULL));
	self->priv->content_length = soup_message_headers_get_content_length (message->response_headers);
	if (soup_message_headers_get_content_range (message->response_headers, &start, &end, &total_length)) {
		self->priv->content_length = (gssize) total_length;
	}
	g_mutex_unlock (&(self->priv->content_mutex));

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
	g_assert (self->priv->buffer != NULL);
	gdata_buffer_push_data (self->priv->buffer, (const guint8*) buffer->data, buffer->length);
}

static gpointer
download_thread (GDataDownloadStream *self)
{
	GDataDownloadStreamPrivate *priv = self->priv;
	GDataAuthorizer *authorizer;

	g_object_ref (self);

	g_assert (priv->network_cancellable != NULL);

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
	/* Connect to the got-headers signal so we can notify clients of the values of content-type and content-length */
	g_signal_connect (priv->message, "got-headers", (GCallback) got_headers_cb, self);
	g_signal_connect (priv->message, "got-chunk", (GCallback) got_chunk_cb, self);

	/* Set a Range header if our starting offset is non-zero */
	if (priv->offset > 0) {
		soup_message_headers_set_range (priv->message->request_headers, priv->offset, -1);
	} else {
		soup_message_headers_remove (priv->message->request_headers, "Range");
	}

	_gdata_service_actually_send_message (priv->session, priv->message, priv->network_cancellable, NULL);

	/* Mark the buffer as having reached EOF */
	g_assert (priv->buffer != NULL);
	gdata_buffer_push_data (priv->buffer, NULL, 0);

	/* Mark the download as finished */
	g_mutex_lock (&(priv->finished_mutex));
	priv->finished = TRUE;
	g_cond_signal (&(priv->finished_cond));
	g_mutex_unlock (&(priv->finished_mutex));

	g_object_unref (self);

	return NULL;
}

static void
create_network_thread (GDataDownloadStream *self, GError **error)
{
	GDataDownloadStreamPrivate *priv = self->priv;

	g_assert (priv->buffer == NULL);
	priv->buffer = gdata_buffer_new ();

	g_assert (priv->network_thread == NULL);
	priv->network_thread = g_thread_try_new ("download-thread", (GThreadFunc) download_thread, self, error);
}

static void
reset_network_thread (GDataDownloadStream *self)
{
	GDataDownloadStreamPrivate *priv = self->priv;

	priv->network_thread = NULL;

	if (priv->buffer != NULL) {
		gdata_buffer_free (priv->buffer);
		priv->buffer = NULL;
	}

	if (priv->message != NULL) {
		soup_session_cancel_message (priv->session, priv->message, SOUP_STATUS_CANCELLED);
		g_signal_handlers_disconnect_by_func (priv->message, got_headers_cb, self);
		g_signal_handlers_disconnect_by_func (priv->message, got_chunk_cb, self);
	}

	priv->offset = 0;

	if (priv->network_cancellable != NULL) {
		g_cancellable_reset (priv->network_cancellable);
	}
}

/**
 * gdata_download_stream_new:
 * @service: a #GDataService
 * @domain: (allow-none): the #GDataAuthorizationDomain to authorize the download, or %NULL
 * @download_uri: the URI to download; this must be HTTPS
 * @cancellable: (allow-none): a #GCancellable for the entire download stream, or %NULL
 *
 * Creates a new #GDataDownloadStream, allowing a file to be downloaded from a GData service using standard #GInputStream API.
 *
 * As well as the standard GIO errors, calls to the #GInputStream API on a #GDataDownloadStream can also return any relevant specific error from
 * #GDataServiceError, or %GDATA_SERVICE_ERROR_PROTOCOL_ERROR in the general case.
 *
 * If a #GCancellable is provided in @cancellable, the download operation may be cancelled at any time from another thread using g_cancellable_cancel().
 * In this case, any ongoing network activity will be stopped, and any pending or future calls to #GInputStream API on the #GDataDownloadStream will
 * return %G_IO_ERROR_CANCELLED. Note that the #GCancellable objects which can be passed to individual #GInputStream operations will not cancel the
 * download operation proper if cancelled — they will merely cancel that API call. The only way to cancel the download operation completely is using
 * this @cancellable.
 *
 * Return value: a new #GInputStream, or %NULL; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GInputStream *
gdata_download_stream_new (GDataService *service, GDataAuthorizationDomain *domain, const gchar *download_uri, GCancellable *cancellable)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (service), NULL);
	g_return_val_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain), NULL);
	g_return_val_if_fail (download_uri != NULL, NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);

	return G_INPUT_STREAM (g_object_new (GDATA_TYPE_DOWNLOAD_STREAM,
	                                     "download-uri", download_uri,
	                                     "service", service,
	                                     "authorization-domain", domain,
	                                     "cancellable", cancellable,
	                                     NULL));
}

/**
 * gdata_download_stream_get_service:
 * @self: a #GDataDownloadStream
 *
 * Gets the service used to authorize the download, as passed to gdata_download_stream_new().
 *
 * Return value: (transfer none): the #GDataService used to authorize the download
 *
 * Since: 0.5.0
 */
GDataService *
gdata_download_stream_get_service (GDataDownloadStream *self)
{
	g_return_val_if_fail (GDATA_IS_DOWNLOAD_STREAM (self), NULL);
	return self->priv->service;
}

/**
 * gdata_download_stream_get_authorization_domain:
 * @self: a #GDataDownloadStream
 *
 * Gets the authorization domain used to authorize the download, as passed to gdata_download_stream_new(). It may be %NULL if authorization is not
 * needed for the download.
 *
 * Return value: (transfer none) (allow-none): the #GDataAuthorizationDomain used to authorize the download, or %NULL
 *
 * Since: 0.9.0
 */
GDataAuthorizationDomain *
gdata_download_stream_get_authorization_domain (GDataDownloadStream *self)
{
	g_return_val_if_fail (GDATA_IS_DOWNLOAD_STREAM (self), NULL);
	return self->priv->authorization_domain;
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
 */
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
 */
const gchar *
gdata_download_stream_get_content_type (GDataDownloadStream *self)
{
	const gchar *content_type;

	g_return_val_if_fail (GDATA_IS_DOWNLOAD_STREAM (self), NULL);

	g_mutex_lock (&(self->priv->content_mutex));
	content_type = self->priv->content_type;
	g_mutex_unlock (&(self->priv->content_mutex));

	/* It's safe to return this, even though we're not taking a copy of it, as it's immutable once set. */
	return content_type;
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
 */
gssize
gdata_download_stream_get_content_length (GDataDownloadStream *self)
{
	gssize content_length;

	g_return_val_if_fail (GDATA_IS_DOWNLOAD_STREAM (self), -1);

	g_mutex_lock (&(self->priv->content_mutex));
	content_length = self->priv->content_length;
	g_mutex_unlock (&(self->priv->content_mutex));

	g_assert (content_length >= -1);

	return content_length;
}

/**
 * gdata_download_stream_get_cancellable:
 * @self: a #GDataDownloadStream
 *
 * Gets the #GCancellable for the entire download operation, #GDataDownloadStream:cancellable.
 *
 * Return value: (transfer none): the #GCancellable for the entire download operation
 *
 * Since: 0.8.0
 */
GCancellable *
gdata_download_stream_get_cancellable (GDataDownloadStream *self)
{
	g_return_val_if_fail (GDATA_IS_DOWNLOAD_STREAM (self), NULL);
	g_assert (self->priv->cancellable != NULL);
	return self->priv->cancellable;
}
