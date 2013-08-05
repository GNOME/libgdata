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

/**
 * GDataMockServer:
 *
 * This is a mock HTTPS server which can be used to run unit tests of network client code on a loopback interface rather than on the real Internet.
 * At its core, it's a simple HTTPS server which runs on a loopback address on an arbitrary port. The code under test must be modified to send its
 * requests to this port, although #GDataMockResolver may be used to transparently redirect all IP addresses to the mock server.
 * A convenience layer on the mock server provides loading of and recording to trace files, which are sequences of request–response HTTPS message pairs
 * where each request is expected by the server (in order). On receiving an expected request, the mock server will return the relevant response and move
 * to expecting the next request in the trace file.
 *
 * The mock server currently only operates on a single network interface, on HTTPS only. This may change in future. A dummy TLS certificate is used
 * to authenticate the server. This certificate is not signed by a CA, so the SoupSession:ssl-strict property must be set to %FALSE in client code
 * during (and only during!) testing.
 *
 * The server can operate in three modes: logging, testing, and comparing. These are set by #GDataMockServer:enable-logging and #GDataMockServer:enable-online.
 *  • Logging mode (#GDataMockServer:enable-logging: %TRUE, #GDataMockServer:enable-online: %TRUE): Requests are sent to the real server online, and the
 *    request–response pairs recorded to a log file.
 *  • Testing mode (#GDataMockServer:enable-logging: %FALSE, #GDataMockServer:enable-online: %FALSE): Requests are sent to the mock server, which responds
 *    from the trace file.
 *  • Comparing mode (#GDataMockServer:enable-logging: %FALSE, #GDataMockServer:enable-online: %TRUE): Requests are sent to the real server online, and
 *    the request–response pairs are compared against those in an existing log file to see if the log file is up-to-date.
 */

#include <glib.h>
#include <glib/gi18n.h>
#include <libsoup/soup.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "mock-resolver.h"
#include "mock-server.h"

static void gdata_mock_server_dispose (GObject *object);
static void gdata_mock_server_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_mock_server_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static gboolean real_handle_message (GDataMockServer *self, SoupMessage *message, SoupClientContext *client);

static void server_handler_cb (SoupServer *server, SoupMessage *message, const gchar *path, GHashTable *query, SoupClientContext *client, gpointer user_data);
static void load_file_stream_thread_cb (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);
static void load_file_iteration_thread_cb (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable);

static GFileInputStream *load_file_stream (GFile *trace_file, GCancellable *cancellable, GError **error);
static SoupMessage *load_file_iteration (GFileInputStream *input_stream, SoupURI *base_uri, GCancellable *cancellable, GError **error);

struct _GDataMockServerPrivate {
	SoupServer *server;
	GDataMockResolver *resolver;
	GThread *server_thread;

	/* Server interface. */
	SoupAddress *address; /* unowned */
	guint port;

	GFile *trace_file;
	GFileInputStream *input_stream;
	GFileOutputStream *output_stream;
	SoupMessage *next_message;
	guint message_counter; /* ID of the message within the current trace file */

	GFile *trace_directory;
	gboolean enable_online;
	gboolean enable_logging;

	GByteArray *comparison_message;
	enum {
		UNKNOWN,
		REQUEST_DATA,
		REQUEST_TERMINATOR,
		RESPONSE_DATA,
		RESPONSE_TERMINATOR,
	} received_message_state;
};

enum {
	PROP_TRACE_DIRECTORY = 1,
	PROP_ENABLE_ONLINE,
	PROP_ENABLE_LOGGING,
	PROP_ADDRESS,
	PROP_PORT,
	PROP_RESOLVER,
};

enum {
	SIGNAL_HANDLE_MESSAGE = 1,
	LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (GDataMockServer, gdata_mock_server, G_TYPE_OBJECT)

static void
gdata_mock_server_class_init (GDataMockServerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataMockServerPrivate));

	gobject_class->get_property = gdata_mock_server_get_property;
	gobject_class->set_property = gdata_mock_server_set_property;
	gobject_class->dispose = gdata_mock_server_dispose;

	klass->handle_message = real_handle_message;

	/**
	 * GDataMockServer:trace-directory:
	 *
	 * Directory relative to which all trace files specified in calls to gdata_mock_server_start_trace() will be resolved.
	 * This is not used for any other methods, but must be non-%NULL if gdata_mock_server_start_trace() is called.
	 */
	g_object_class_install_property (gobject_class, PROP_TRACE_DIRECTORY,
	                                 g_param_spec_object ("trace-directory",
	                                                      "Trace Directory", "Directory relative to which all trace files will be resolved.",
	                                                      G_TYPE_FILE,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataMockServer:enable-online:
	 *
	 * %TRUE if network traffic should reach the Internet as normal; %FALSE to redirect it to the local mock server.
	 * Use this in conjunction with #GDataMockServer:enable-logging to either log online traffic, or replay logged traffic locally.
	 */
	g_object_class_install_property (gobject_class, PROP_ENABLE_ONLINE,
	                                 g_param_spec_boolean ("enable-online",
	                                                       "Enable Online", "Whether network traffic should reach the Internet as normal.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataMockServer:enable-logging:
	 *
	 * %TRUE if network traffic should be logged to a trace file (specified by calling gdata_mock_server_start_trace()). This operates independently
	 * of whether traffic is online or being handled locally by the mock server.
	 * Use this in conjunction with #GDataMockServer:enable-online to either log online traffic, or replay logged traffic locally.
	 */
	g_object_class_install_property (gobject_class, PROP_ENABLE_LOGGING,
	                                 g_param_spec_boolean ("enable-logging",
	                                                       "Enable Logging", "Whether network traffic should be logged to a trace file.",
	                                                       FALSE,
	                                                       G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataMockServer:address:
	 *
	 * Address of the local mock server if it's running, or %NULL otherwise. This will be non-%NULL between calls to gdata_mock_server_run() and
	 * gdata_mock_server_stop().
	 *
	 * This should not normally need to be passed into client code under test, unless the code references IP addresses specifically. The mock server
	 * runs a DNS resolver which automatically redirects client requests for known domain names to this address (#GDataMockServer:resolver).
	 */
	g_object_class_install_property (gobject_class, PROP_ADDRESS,
	                                 g_param_spec_object ("address",
	                                                      "Server Address", "Address of the local mock server if it's running.",
	                                                      SOUP_TYPE_ADDRESS,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataMockServer:port:
	 *
	 * Port of the local mock server if it's running, or <code class="literal">0</code> otherwise. This will be non-<code class="literal">0</code> between
	 * calls to gdata_mock_server_run() and gdata_mock_server_stop().
	 *
	 * It is intended that this port be passed into the client code under test, to substitute for the default HTTPS port (443) which it would otherwise
	 * use.
	 */
	g_object_class_install_property (gobject_class, PROP_PORT,
	                                 g_param_spec_uint ("port",
	                                                    "Server Port", "Port of the local mock server if it's running",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataMockServer:resolver:
	 *
	 * Mock resolver used to redirect HTTP requests from specified domain names to the local mock server instance. This will always be set while the
	 * server is running (between calls to gdata_mock_server_run() and gdata_mock_server_stop()), and is %NULL otherwise.
	 *
	 * Use the resolver specified in this property to add domain names which are expected to be requested by the current trace. Domain names not added
	 * to the resolver will be rejected by the mock server. The set of domain names in the resolver will be reset when gdata_mock_server_stop() is
	 * called.
	 */
	g_object_class_install_property (gobject_class, PROP_RESOLVER,
	                                 g_param_spec_object ("resolver",
	                                                      "Resolver", "Mock resolver used to redirect HTTP requests to the local mock server instance.",
	                                                      GDATA_TYPE_MOCK_RESOLVER,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataMockServer::handle-message:
	 *
	 * Emitted whenever the mock server is running and receives a request from a client. Test code may connect to this signal and implement a handler
	 * which builds and returns a suitable response for a given message. The default handler reads a request–response pair from the current trace file,
	 * matches the requests and then returns the given response. If the requests don't match, an error is raised.
	 *
	 * Signal handlers should return %TRUE if they have handled the request and set an appropriate response; and %FALSE otherwise.
	 */
	signals[SIGNAL_HANDLE_MESSAGE] = g_signal_new ("handle-message", G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST,
	                                               G_STRUCT_OFFSET (GDataMockServerClass, handle_message),
	                                               g_signal_accumulator_true_handled, NULL,
	                                               g_cclosure_marshal_generic,
	                                               G_TYPE_BOOLEAN, 2,
	                                               SOUP_TYPE_MESSAGE, SOUP_TYPE_CLIENT_CONTEXT);
}

static void
gdata_mock_server_init (GDataMockServer *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_MOCK_SERVER, GDataMockServerPrivate);
}

static void
gdata_mock_server_dispose (GObject *object)
{
	GDataMockServerPrivate *priv = GDATA_MOCK_SERVER (object)->priv;

	g_clear_object (&priv->resolver);
	g_clear_object (&priv->server);
	g_clear_object (&priv->input_stream);
	g_clear_object (&priv->trace_file);
	g_clear_object (&priv->input_stream);
	g_clear_object (&priv->output_stream);
	g_clear_object (&priv->next_message);
	g_clear_object (&priv->trace_directory);
	g_clear_pointer (&priv->server_thread, g_thread_unref);
	g_clear_pointer (&priv->comparison_message, g_byte_array_unref);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_mock_server_parent_class)->dispose (object);
}

static void
gdata_mock_server_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataMockServerPrivate *priv = GDATA_MOCK_SERVER (object)->priv;

	switch (property_id) {
		case PROP_TRACE_DIRECTORY:
			g_value_set_object (value, priv->trace_directory);
			break;
		case PROP_ENABLE_ONLINE:
			g_value_set_boolean (value, priv->enable_online);
			break;
		case PROP_ENABLE_LOGGING:
			g_value_set_boolean (value, priv->enable_logging);
			break;
		case PROP_ADDRESS:
			g_value_set_object (value, priv->address);
			break;
		case PROP_PORT:
			g_value_set_uint (value, priv->port);
			break;
		case PROP_RESOLVER:
			g_value_set_object (value, priv->resolver);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_mock_server_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataMockServer *self = GDATA_MOCK_SERVER (object);

	switch (property_id) {
		case PROP_TRACE_DIRECTORY:
			gdata_mock_server_set_trace_directory (self, g_value_get_object (value));
			break;
		case PROP_ENABLE_ONLINE:
			gdata_mock_server_set_enable_online (self, g_value_get_boolean (value));
			break;
		case PROP_ENABLE_LOGGING:
			gdata_mock_server_set_enable_logging (self, g_value_get_boolean (value));
			break;
		case PROP_ADDRESS:
		case PROP_PORT:
		case PROP_RESOLVER:
			/* Read-only. */
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

typedef struct {
	GFileInputStream *input_stream;
	SoupURI *base_uri;
} LoadFileIterationData;

static void
load_file_iteration_data_free (LoadFileIterationData *data)
{
	g_object_unref (data->input_stream);
	soup_uri_free (data->base_uri);
	g_slice_free (LoadFileIterationData, data);
}

static SoupURI * /* transfer full */
build_base_uri (GDataMockServer *self)
{
	GDataMockServerPrivate *priv = self->priv;
	gchar *base_uri_string;
	SoupURI *base_uri;

	if (priv->enable_online == FALSE) {
		base_uri_string = g_strdup_printf ("https://%s:%u", soup_address_get_physical (self->priv->address), self->priv->port);
	} else {
		base_uri_string = g_strdup ("https://localhost"); /* FIXME */
	}

	base_uri = soup_uri_new (base_uri_string);
	g_free (base_uri_string);

	return base_uri;
}

static inline gboolean
parts_equal (const char *one, const char *two, gboolean insensitive)
{
	if (!one && !two)
		return TRUE;
	if (!one || !two)
		return FALSE;
	return insensitive ? !g_ascii_strcasecmp (one, two) : !strcmp (one, two);
}

/* strcmp()-like return value: 0 means the messages compare equal. */
static gint
compare_incoming_message (SoupMessage *expected_message, SoupMessage *actual_message, SoupClientContext *actual_client)
{
	SoupURI *expected_uri, *actual_uri;

	/* Compare method. */
	if (g_strcmp0 (expected_message->method, actual_message->method) != 0) {
		return 1;
	}

	/* Compare URIs. */
	expected_uri = soup_message_get_uri (expected_message);
	actual_uri = soup_message_get_uri (actual_message);

	/* TODO: Also compare actual_client auth domains and address? HTTP version? Method? */

	if (!parts_equal (expected_uri->user, actual_uri->user, FALSE) ||
	    !parts_equal (expected_uri->password, actual_uri->password, FALSE) ||
	    !parts_equal (expected_uri->path, actual_uri->path, FALSE) ||
	    !parts_equal (expected_uri->query, actual_uri->query, FALSE) ||
	    !parts_equal (expected_uri->fragment, actual_uri->fragment, FALSE)) {
		return 1;
	}

	return 0;
}

static void
header_append_cb (const gchar *name, const gchar *value, gpointer user_data)
{
	SoupMessage *message = user_data;

	soup_message_headers_append (message->response_headers, name, value);
}

static void
server_process_message (GDataMockServer *self, SoupMessage *message, SoupClientContext *client)
{
	GDataMockServerPrivate *priv = self->priv;
	SoupBuffer *message_body;
	goffset expected_content_length;
	gchar *trace_file_name, *trace_file_offset;

	g_assert (priv->next_message != NULL);
	priv->message_counter++;

	if (compare_incoming_message (priv->next_message, message, client) != 0) {
		gchar *body, *next_uri, *actual_uri;

		/* Received message is not what we expected. Return an error. */
		soup_message_set_status_full (message, SOUP_STATUS_BAD_REQUEST, "Unexpected request to mock server");

		next_uri = soup_uri_to_string (soup_message_get_uri (priv->next_message), TRUE);
		actual_uri = soup_uri_to_string (soup_message_get_uri (message), TRUE);
		body = g_strdup_printf ("Expected %s URI ‘%s’, but got %s ‘%s’.", priv->next_message->method, next_uri, message->method, actual_uri);
		g_free (actual_uri);
		g_free (next_uri);
		soup_message_body_append_take (message->response_body, (guchar *) body, strlen (body));

		return;
	}

	/* The incoming message matches what we expected, so copy the headers and body from the expected response and return it. */
	soup_message_set_http_version (message, soup_message_get_http_version (priv->next_message));
	soup_message_set_status_full (message, priv->next_message->status_code, priv->next_message->reason_phrase);
	soup_message_headers_foreach (priv->next_message->response_headers, header_append_cb, message);

	/* Add debug headers to identify the message and trace file. */
	trace_file_name = g_file_get_uri (priv->trace_file);
	soup_message_headers_append (message->response_headers, "X-Mock-Trace-File", trace_file_name);
	g_free (trace_file_name);

	trace_file_offset = g_strdup_printf ("%u", priv->message_counter);
	soup_message_headers_append (message->response_headers, "X-Mock-Trace-File-Offset", trace_file_offset);
	g_free (trace_file_offset);

	message_body = soup_message_body_flatten (priv->next_message->response_body);
	if (message_body->length > 0) {
		soup_message_body_append_buffer (message->response_body, message_body);
	}

	/* If the log file doesn't contain the full response body (e.g. because it's a huge binary file containing a nul byte somewhere),
	 * make one up (all zeros). */
	expected_content_length = soup_message_headers_get_content_length (message->response_headers);
	if (expected_content_length > 0 && message_body->length < (guint64) expected_content_length) {
		guint8 *buf;

		buf = g_malloc0 (expected_content_length - message_body->length);
		soup_message_body_append_take (message->response_body, buf, expected_content_length - message_body->length);
	}

	soup_buffer_free (message_body);

	soup_message_body_complete (message->response_body);

	/* Clear the expected message. */
	g_clear_object (&priv->next_message);
}

static void
server_handler_cb (SoupServer *server, SoupMessage *message, const gchar *path, GHashTable *query, SoupClientContext *client, gpointer user_data)
{
	GDataMockServer *self = user_data;
	gboolean message_handled = FALSE;

	soup_server_pause_message (server, message);
	g_signal_emit (self, signals[SIGNAL_HANDLE_MESSAGE], 0, message, client, &message_handled);
	soup_server_unpause_message (server, message);

	/* The message should always be handled by real_handle_message() at least. */
	g_assert (message_handled == TRUE);
}

static gboolean
real_handle_message (GDataMockServer *self, SoupMessage *message, SoupClientContext *client)
{
	GDataMockServerPrivate *priv = self->priv;
	gboolean handled = FALSE;

	/* Asynchronously load the next expected message from the trace file. */
	if (priv->next_message == NULL) {
		GTask *task;
		LoadFileIterationData *data;
		GError *child_error = NULL;

		data = g_slice_new (LoadFileIterationData);
		data->input_stream = g_object_ref (priv->input_stream);
		data->base_uri = build_base_uri (self);

		task = g_task_new (self, NULL, NULL, NULL);
		g_task_set_task_data (task, data, (GDestroyNotify) load_file_iteration_data_free);
		g_task_run_in_thread_sync (task, load_file_iteration_thread_cb);

		/* Handle the results. */
		priv->next_message = g_task_propagate_pointer (task, &child_error);

		g_object_unref (task);

		if (child_error != NULL) {
			gchar *body;

			soup_message_set_status_full (message, SOUP_STATUS_INTERNAL_SERVER_ERROR, "Error loading expected request");

			body = g_strdup_printf ("Error: %s", child_error->message);
			soup_message_body_append_take (message->response_body, (guchar *) body, strlen (body));
			handled = TRUE;

			g_error_free (child_error);
		} else if (priv->next_message == NULL) {
			gchar *body, *actual_uri;

			/* Received message is not what we expected. Return an error. */
			soup_message_set_status_full (message, SOUP_STATUS_BAD_REQUEST, "Unexpected request to mock server");

			actual_uri = soup_uri_to_string (soup_message_get_uri (message), TRUE);
			body = g_strdup_printf ("Expected no request, but got %s ‘%s’.", message->method, actual_uri);
			g_free (actual_uri);
			soup_message_body_append_take (message->response_body, (guchar *) body, strlen (body));
			handled = TRUE;
		}
	}

	/* Process the actual message if we already know the expected message. */
	g_assert (priv->next_message != NULL || handled == TRUE);
	if (handled == FALSE) {
		server_process_message (self, message, client);
		handled = TRUE;
	}

	g_assert (handled == TRUE);
	return handled;
}

/**
 * gdata_mock_server_new:
 *
 * Creates a new #GDataMockServer with default properties.
 *
 * Return value: (transfer full): a new #GDataMockServer; unref with g_object_unref()
 */
GDataMockServer *
gdata_mock_server_new (void)
{
	return g_object_new (GDATA_TYPE_MOCK_SERVER, NULL);
}

static gboolean
trace_to_soup_message_headers_and_body (SoupMessageHeaders *message_headers, SoupMessageBody *message_body, const gchar message_direction, const gchar **_trace)
{
	const gchar *i;
	const gchar *trace = *_trace;

	/* Parse headers. */
	while (TRUE) {
		gchar *header_name, *header_value;

		if (*trace == '\0') {
			/* No body. */
			goto done;
		} else if (*trace == ' ' && *(trace + 1) == ' ' && *(trace + 2) == '\n') {
			/* No body. */
			trace += 3;
			goto done;
		} else if (*trace != message_direction || *(trace + 1) != ' ') {
			g_warning ("Unrecognised start sequence ‘%c%c’.", *trace, *(trace + 1));
			goto error;
		}
		trace += 2;

		if (*trace == '\n') {
			/* Reached the end of the headers. */
			trace++;
			break;
		}

		i = strchr (trace, ':');
		if (i == NULL || *(i + 1) != ' ') {
			g_warning ("Missing spacer ‘: ’.");
			goto error;
		}

		header_name = g_strndup (trace, i - trace);
		trace += (i - trace) + 2;

		i = strchr (trace, '\n');
		if (i == NULL) {
			g_warning ("Missing spacer ‘\\n’.");
			goto error;
		}

		header_value = g_strndup (trace, i - trace);
		trace += (i - trace) + 1;

		/* Append the header. */
		soup_message_headers_append (message_headers, header_name, header_value);

		g_free (header_value);
		g_free (header_name);
	}

	/* Parse the body. */
	while (TRUE) {
		if (*trace == ' ' && *(trace + 1) == ' ' && *(trace + 2) == '\n') {
			/* End of the body. */
			trace += 3;
			break;
		} else if (*trace == '\0') {
			/* End of the body. */
			break;
		} else if (*trace != message_direction || *(trace + 1) != ' ') {
			g_warning ("Unrecognised start sequence ‘%c%c’.", *trace, *(trace + 1));
			goto error;
		}
		trace += 2;

		i = strchr (trace, '\n');
		if (i == NULL) {
			g_warning ("Missing spacer ‘\\n’.");
			goto error;
		}

		soup_message_body_append (message_body, SOUP_MEMORY_COPY, trace, i - trace + 1); /* include trailing \n */
		trace += (i - trace) + 1;
	}

done:
	/* Done. Update the output trace pointer. */
	soup_message_body_complete (message_body);
	*_trace = trace;

	return TRUE;

error:
	return FALSE;
}

/* base_uri is the base URI for the server, e.g. https://127.0.0.1:1431. */
static SoupMessage *
trace_to_soup_message (const gchar *trace, SoupURI *base_uri)
{
	SoupMessage *message;
	const gchar *i, *j, *method;
	gchar *uri_string, *response_message;
	SoupHTTPVersion http_version;
	guint response_status;
	SoupURI *uri;

	g_return_val_if_fail (trace != NULL, NULL);

	/* The traces look somewhat like this:
	 * > POST /unauth HTTP/1.1
	 * > Soup-Debug-Timestamp: 1200171744
	 * > Soup-Debug: SoupSessionAsync 1 (0x612190), SoupMessage 1 (0x617000), SoupSocket 1 (0x612220)
	 * > Host: localhost
	 * > Content-Type: text/plain
	 * > Connection: close
	 * > 
	 * > This is a test.
	 *   
	 * < HTTP/1.1 201 Created
	 * < Soup-Debug-Timestamp: 1200171744
	 * < Soup-Debug: SoupMessage 1 (0x617000)
	 * < Date: Sun, 12 Jan 2008 21:02:24 GMT
	 * < Content-Length: 0
	 *
	 * This function parses a single request–response pair.
	 */

	/* Parse the method, URI and HTTP version first. */
	if (*trace != '>' || *(trace + 1) != ' ') {
		g_warning ("Unrecognised start sequence ‘%c%c’.", *trace, *(trace + 1));
		goto error;
	}
	trace += 2;

	/* Parse “POST /unauth HTTP/1.1”. */
	if (strncmp (trace, "POST", strlen ("POST")) == 0) {
		method = SOUP_METHOD_POST;
		trace += strlen ("POST");
	} else if (strncmp (trace, "GET", strlen ("GET")) == 0) {
		method = SOUP_METHOD_GET;
		trace += strlen ("GET");
	} else if (strncmp (trace, "DELETE", strlen ("DELETE")) == 0) {
		method = SOUP_METHOD_DELETE;
		trace += strlen ("DELETE");
	} else if (strncmp (trace, "PUT", strlen ("PUT")) == 0) {
		method = SOUP_METHOD_PUT;
		trace += strlen ("PUT");
	} else {
		g_warning ("Unknown method ‘%s’.", trace);
		goto error;
	}

	if (*trace != ' ') {
		g_warning ("Unrecognised spacer ‘%c’.", *trace);
		goto error;
	}
	trace++;

	i = strchr (trace, ' ');
	if (i == NULL) {
		g_warning ("Missing spacer ‘ ’.");
		goto error;
	}

	uri_string = g_strndup (trace, i - trace);
	trace += (i - trace) + 1;

	if (strncmp (trace, "HTTP/1.1", strlen ("HTTP/1.1")) == 0) {
		http_version = SOUP_HTTP_1_1;
		trace += strlen ("HTTP/1.1");
	} else if (strncmp (trace, "HTTP/1.0", strlen ("HTTP/1.0")) == 0) {
		http_version = SOUP_HTTP_1_0;
		trace += strlen ("HTTP/1.0");
	} else {
		g_warning ("Unrecognised HTTP version ‘%s’.", trace);
	}

	if (*trace != '\n') {
		g_warning ("Unrecognised spacer ‘%c’.", *trace);
		goto error;
	}
	trace++;

	/* Build the message. */
	uri = soup_uri_new_with_base (base_uri, uri_string);
	message = soup_message_new_from_uri (method, uri);
	soup_uri_free (uri);

	if (message == NULL) {
		g_warning ("Invalid URI ‘%s’.", uri_string);
		goto error;
	}

	soup_message_set_http_version (message, http_version);
	g_free (uri_string);

	/* Parse the request headers and body. */
	if (trace_to_soup_message_headers_and_body (message->request_headers, message->request_body, '>', &trace) == FALSE) {
		goto error;
	}

	/* Parse the response, starting with “HTTP/1.1 201 Created”. */
	if (*trace != '<' || *(trace + 1) != ' ') {
		g_warning ("Unrecognised start sequence ‘%c%c’.", *trace, *(trace + 1));
		goto error;
	}
	trace += 2;

	if (strncmp (trace, "HTTP/1.1", strlen ("HTTP/1.1")) == 0) {
		http_version = SOUP_HTTP_1_1;
		trace += strlen ("HTTP/1.1");
	} else if (strncmp (trace, "HTTP/1.0", strlen ("HTTP/1.0")) == 0) {
		http_version = SOUP_HTTP_1_0;
		trace += strlen ("HTTP/1.0");
	} else {
		g_warning ("Unrecognised HTTP version ‘%s’.", trace);
	}

	if (*trace != ' ') {
		g_warning ("Unrecognised spacer ‘%c’.", *trace);
		goto error;
	}
	trace++;

	i = strchr (trace, ' ');
	if (i == NULL) {
		g_warning ("Missing spacer ‘ ’.");
		goto error;
	}

	response_status = g_ascii_strtoull (trace, (gchar **) &j, 10);
	if (j != i) {
		g_warning ("Invalid status ‘%s’.", trace);
		goto error;
	}
	trace += (i - trace) + 1;

	i = strchr (trace, '\n');
	if (i == NULL) {
		g_warning ("Missing spacer ‘\n’.");
		goto error;
	}

	response_message = g_strndup (trace, i - trace);
	trace += (i - trace) + 1;

	soup_message_set_status_full (message, response_status, response_message);

	/* Parse the response headers and body. */
	if (trace_to_soup_message_headers_and_body (message->response_headers, message->response_body, '<', &trace) == FALSE) {
		goto error;
	}

	return message;

error:
	g_object_unref (message);

	return NULL;
}

static GFileInputStream *
load_file_stream (GFile *trace_file, GCancellable *cancellable, GError **error)
{
	return g_file_read (trace_file, cancellable, error);
}

static gboolean
load_message_half (GFileInputStream *input_stream, GString *current_message, gchar half_direction, GCancellable *cancellable, GError **error)
{
	guint8 buf[1024];
	gssize len;

	while (TRUE) {
		len = g_input_stream_read (G_INPUT_STREAM (input_stream), buf, sizeof (buf), cancellable, error);

		if (len == -1) {
			/* Error. */
			return FALSE;
		} else if (len == 0) {
			/* EOF. Try again to grab a response. */
			return TRUE;
		} else {
			const guint8 *i;

			/* Got some data. Parse it and see if we've reached the end of a message (i.e. read both the request and the response). */
			for (i = memchr (buf, half_direction, sizeof (buf)); i != NULL; i = memchr (i + 1, half_direction, buf + sizeof (buf) - i)) {
				if (*(i + 1) == ' ' && (i == buf || (i > buf && *(i - 1) == '\n'))) {
					/* Found the boundary between request and response.
					 * Fall through to try and find the boundary between this response and the next request.
					 * To make things simpler, seek back to the boundary and make a second read request below. */
					if (g_seekable_seek (G_SEEKABLE (input_stream), i - (buf + len), G_SEEK_CUR, cancellable, error) == FALSE) {
						/* Error. */
						return FALSE;
					}

					g_string_append_len (current_message, (gchar *) buf, i - buf);

					return TRUE;
				}
			}

			/* Reached the end of the buffer without finding a change in message. Loop around and load another buffer-full. */
			g_string_append_len (current_message, (gchar *) buf, len);
		}
	}

	return TRUE;
}

/* Returns TRUE iff the given message from a trace file should be ignored and not used by the mock server. */
static gboolean
should_ignore_soup_message (SoupMessage *message)
{
	switch (message->status_code) {
		case SOUP_STATUS_NONE:
		case SOUP_STATUS_CANCELLED:
		case SOUP_STATUS_CANT_RESOLVE:
		case SOUP_STATUS_CANT_RESOLVE_PROXY:
		case SOUP_STATUS_CANT_CONNECT:
		case SOUP_STATUS_CANT_CONNECT_PROXY:
		case SOUP_STATUS_SSL_FAILED:
		case SOUP_STATUS_IO_ERROR:
		case SOUP_STATUS_MALFORMED:
		case SOUP_STATUS_TRY_AGAIN:
		case SOUP_STATUS_TOO_MANY_REDIRECTS:
		case SOUP_STATUS_TLS_FAILED:
			return TRUE;
		default:
			return FALSE;
	}
}

static SoupMessage *
load_file_iteration (GFileInputStream *input_stream, SoupURI *base_uri, GCancellable *cancellable, GError **error)
{
	SoupMessage *output_message = NULL;
	GString *current_message = NULL;

	current_message = g_string_new (NULL);

	do {
		/* Start loading from the stream. */
		g_string_truncate (current_message, 0);

		/* We should be at the start of a request (>). Search for the start of the response (<), then for the start of the next request (>). */
		if (load_message_half (input_stream, current_message, '<', cancellable, error) == FALSE ||
		    load_message_half (input_stream, current_message, '>', cancellable, error) == FALSE) {
			goto done;
		}

		if (current_message->len > 0) {
			output_message = trace_to_soup_message (current_message->str, base_uri);
		} else {
			/* Reached the end of the file. */
			output_message = NULL;
		}
	} while (output_message != NULL && should_ignore_soup_message (output_message) == TRUE);

done:
	/* Tidy up. */
	g_string_free (current_message, TRUE);

	/* Postcondition: (output_message != NULL) => (*error == NULL). */
	g_assert (output_message == NULL || (error == NULL || *error == NULL));

	return output_message;
}

static void
load_file_stream_thread_cb (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GFile *trace_file;
	GFileInputStream *input_stream;
	GError *child_error = NULL;

	trace_file = task_data;
	g_assert (G_IS_FILE (trace_file));

	input_stream = load_file_stream (trace_file, cancellable, &child_error);

	if (child_error != NULL) {
		g_task_return_error (task, child_error);
	} else {
		g_task_return_pointer (task, input_stream, g_object_unref);
	}
}

static void
load_file_iteration_thread_cb (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	LoadFileIterationData *data = task_data;
	GFileInputStream *input_stream;
	SoupMessage *output_message;
	SoupURI *base_uri;
	GError *child_error = NULL;

	input_stream = data->input_stream;
	g_assert (G_IS_FILE_INPUT_STREAM (input_stream));
	base_uri = data->base_uri;

	output_message = load_file_iteration (input_stream, base_uri, cancellable, &child_error);

	if (child_error != NULL) {
		g_task_return_error (task, child_error);
	} else {
		g_task_return_pointer (task, output_message, g_object_unref);
	}
}

/**
 * gdata_mock_server_unload_trace:
 * @self: a #GDataMockServer
 *
 * Unloads the current trace file of network messages, as loaded by gdata_mock_server_load_trace() or gdata_mock_server_load_trace_async().
 */
void
gdata_mock_server_unload_trace (GDataMockServer *self)
{
	GDataMockServerPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));

	g_clear_object (&priv->next_message);
	g_clear_object (&priv->input_stream);
	g_clear_object (&priv->trace_file);
	g_clear_pointer (&priv->comparison_message, g_byte_array_unref);
	priv->message_counter = 0;
	priv->received_message_state = UNKNOWN;
}

/**
 * gdata_mock_server_load_trace:
 * @self: a #GDataMockServer
 * @trace_file: trace file to load
 * @cancellable: (allow-none): a #GCancellable, or %NULL
 * @error: (allow-none): return location for a #GError, or %NULL
 *
 * Synchronously loads the given @trace_file of network messages, ready to simulate a network conversation by matching
 * requests against the file and returning the associated responses. Call gdata_mock_server_run() to start the mock
 * server afterwards.
 *
 * Loading the trace file may be cancelled from another thread using @cancellable.
 *
 * On error, @error will be set and the state of the #GDataMockServer will not change.
 */
void
gdata_mock_server_load_trace (GDataMockServer *self, GFile *trace_file, GCancellable *cancellable, GError **error)
{
	GDataMockServerPrivate *priv = self->priv;
	SoupURI *base_uri;

	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));
	g_return_if_fail (G_IS_FILE (trace_file));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (error == NULL || *error == NULL);
	g_return_if_fail (priv->trace_file == NULL && priv->input_stream == NULL && priv->next_message == NULL);

	base_uri = build_base_uri (self);

	priv->trace_file = g_object_ref (trace_file);
	priv->input_stream = load_file_stream (priv->trace_file, cancellable, error);

	if (priv->input_stream != NULL) {
		GError *child_error = NULL;

		priv->next_message = load_file_iteration (priv->input_stream, base_uri, cancellable, &child_error);
		priv->message_counter = 0;
		priv->comparison_message = g_byte_array_new ();
		priv->received_message_state = UNKNOWN;

		if (child_error != NULL) {
			g_clear_object (&priv->trace_file);
			g_propagate_error (error, child_error);
		}
	} else {
		/* Error. */
		g_clear_object (&priv->trace_file);
	}

	soup_uri_free (base_uri);
}

typedef struct {
	GAsyncReadyCallback callback;
	gpointer user_data;
	SoupURI *base_uri;
} LoadTraceData;

static void
load_trace_async_cb (GObject *source_object, GAsyncResult *result, gpointer user_data)
{
	GDataMockServer *self = GDATA_MOCK_SERVER (source_object);
	LoadTraceData *data = user_data;
	LoadFileIterationData *iteration_data;
	GTask *task;
	GError *child_error = NULL;

	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));
	g_return_if_fail (G_IS_ASYNC_RESULT (result));
	g_return_if_fail (g_task_is_valid (result, self));

	self->priv->input_stream = g_task_propagate_pointer (G_TASK (result), &child_error);

	iteration_data = g_slice_new (LoadFileIterationData);
	iteration_data->input_stream = g_object_ref (self->priv->input_stream);
	iteration_data->base_uri = data->base_uri; /* transfer ownership */
	data->base_uri = NULL;

	task = g_task_new (g_task_get_source_object (G_TASK (result)), g_task_get_cancellable (G_TASK (result)), data->callback, data->user_data);
	g_task_set_task_data (task, iteration_data, (GDestroyNotify) load_file_iteration_data_free);

	if (child_error != NULL) {
		g_task_return_error (task, child_error);
	} else {
		g_task_run_in_thread (task, load_file_iteration_thread_cb);
	}

	g_object_unref (task);

	g_slice_free (LoadTraceData, data);
}

/**
 * gdata_mock_server_load_trace_async:
 * @self: a #GDataMockServer
 * @trace_file: trace file to load
 * @cancellable: (allow-none): a #GCancellable, or %NULL
 * @callback: function to call once the async operation is complete
 * @user_data: (allow-none): user data to pass to @callback, or %NULL
 *
 * Asynchronous version of gdata_mock_server_load_trace(). In @callback, call gdata_mock_server_load_trace_finish() to complete the operation.
 */
void
gdata_mock_server_load_trace_async (GDataMockServer *self, GFile *trace_file, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GTask *task;
	LoadTraceData *data;

	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));
	g_return_if_fail (G_IS_FILE (trace_file));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (self->priv->trace_file == NULL && self->priv->input_stream == NULL && self->priv->next_message == NULL);

	self->priv->trace_file = g_object_ref (trace_file);

	data = g_slice_new (LoadTraceData);
	data->callback = callback;
	data->user_data = user_data;
	data->base_uri = build_base_uri (self);

	task = g_task_new (self, cancellable, load_trace_async_cb, data);
	g_task_set_task_data (task, g_object_ref (self->priv->trace_file), g_object_unref);
	g_task_run_in_thread (task, load_file_stream_thread_cb);
	g_object_unref (task);
}

/**
 * gdata_mock_server_load_trace_finish:
 * @self: a #GDataMockServer
 * @result: asynchronous operation result passed to the callback
 * @error: (allow-none): return location for a #GError, or %NULL
 *
 * Finishes an asynchronous operation started by gdata_mock_server_load_trace_async().
 *
 * On error, @error will be set and the state of the #GDataMockServer will not change.
 */
void
gdata_mock_server_load_trace_finish (GDataMockServer *self, GAsyncResult *result, GError **error)
{
	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));
	g_return_if_fail (G_IS_ASYNC_RESULT (result));
	g_return_if_fail (error == NULL || *error == NULL);
	g_return_if_fail (g_task_is_valid (result, self));

	self->priv->next_message = g_task_propagate_pointer (G_TASK (result), error);
	self->priv->message_counter = 0;
	self->priv->comparison_message = g_byte_array_new ();
	self->priv->received_message_state = UNKNOWN;
}

static gpointer
server_thread_cb (gpointer user_data)
{
	GDataMockServer *self = user_data;
	GDataMockServerPrivate *priv = self->priv;

	/* Run the server. */
	soup_server_run (priv->server);

	return NULL;
}

/**
 * gdata_mock_server_run:
 * @self: a #GDataMockServer
 *
 * Runs the mock server, binding to a loopback TCP/IP interface and preparing a HTTPS server which is ready to accept requests.
 * The TCP/IP address and port number are chosen randomly out of the loopback addresses, and are exposed as #GDataMockServer:address and #GDataMockServer:port
 * once this function has returned. A #GDataMockResolver (exposed as #GDataMockServer:resolver) is set as the default #GResolver while the server is running.
 *
 * The server is started in a worker thread, so this function returns immediately and the server continues to run in the background. Use gdata_mock_server_stop()
 * to shut it down.
 *
 * This function always succeeds.
 */
void
gdata_mock_server_run (GDataMockServer *self)
{
	GDataMockServerPrivate *priv = self->priv;
	struct sockaddr_in sock;
	SoupAddress *addr;
	GMainContext *thread_context;

	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));
	g_return_if_fail (priv->resolver == NULL);
	g_return_if_fail (priv->server == NULL);

	/* Grab a loopback IP to use. */
	memset (&sock, 0, sizeof (sock));
	sock.sin_family = AF_INET;
	sock.sin_addr.s_addr = htonl (INADDR_LOOPBACK);
	sock.sin_port = htons (0); /* random port */

	addr = soup_address_new_from_sockaddr ((struct sockaddr *) &sock, sizeof (sock));
	g_assert (addr != NULL);

	/* Set up the server. The SSL certificate can be generated using:
	 *     openssl req -x509 -newkey rsa:2048 -keyout key.pem -out cert.pem -nodes */
	thread_context = g_main_context_new ();
	priv->server = soup_server_new ("interface", addr,
	                                "ssl-cert-file", TEST_FILE_DIR "cert.pem",
	                                "ssl-key-file", TEST_FILE_DIR "key.pem",
	                                "async-context", thread_context,
	                                "raw-paths", TRUE,
	                                NULL);
	soup_server_add_handler (priv->server, "/", server_handler_cb, self, NULL);

	g_main_context_unref (thread_context);
	g_object_unref (addr);

	/* Grab the randomly selected address and port. */
	priv->address = soup_socket_get_local_address (soup_server_get_listener (priv->server));
	priv->port = soup_server_get_port (priv->server);

	/* Set up the resolver. It is expected that callers will grab the resolver (by calling gdata_mock_server_get_resolver())
	 * immediately after this function returns, and add some expected hostnames by calling gdata_mock_resolver_add_A() one or
	 * more times, before starting the next test. */
	priv->resolver = gdata_mock_resolver_new ();
	g_resolver_set_default (G_RESOLVER (priv->resolver));

	g_object_freeze_notify (G_OBJECT (self));
	g_object_notify (G_OBJECT (self), "address");
	g_object_notify (G_OBJECT (self), "port");
	g_object_notify (G_OBJECT (self), "resolver");
	g_object_thaw_notify (G_OBJECT (self));

	/* Start the network thread. */
	priv->server_thread = g_thread_new ("mock-server-thread", server_thread_cb, self);
}

/**
 * gdata_mock_server_stop:
 * @self: a #GDataMockServer
 *
 * Stops a mock server started by calling gdata_mock_server_run(). This shuts down the server's worker thread and unbinds it from its TCP/IP socket.
 *
 * This unloads any trace file loaded by calling gdata_mock_server_load_trace() (or its asynchronous counterpart). It also resets the set of domain
 * names loaded into the #GDataMockServer:resolver.
 *
 * This function always succeeds.
 */
void
gdata_mock_server_stop (GDataMockServer *self)
{
	GDataMockServerPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));
	g_return_if_fail (priv->server != NULL);
	g_return_if_fail (priv->resolver != NULL);

	/* Stop the server. */
	soup_server_disconnect (priv->server);
	g_thread_join (priv->server_thread);
	priv->server_thread = NULL;
	gdata_mock_resolver_reset (priv->resolver);

	g_clear_object (&priv->server);
	g_clear_object (&priv->resolver);

	priv->address = NULL;
	priv->port = 0;

	g_object_freeze_notify (G_OBJECT (self));
	g_object_notify (G_OBJECT (self), "address");
	g_object_notify (G_OBJECT (self), "port");
	g_object_notify (G_OBJECT (self), "resolver");
	g_object_thaw_notify (G_OBJECT (self));

	/* Reset the trace file. */
	gdata_mock_server_unload_trace (self);
}

/**
 * gdata_mock_server_get_trace_directory:
 * @self: a #GDataMockServer
 *
 * Gets the value of the #GDataMockServer:trace-directory property.
 *
 * Return value: (allow-none) (transfer none): the directory to load/store trace files from, or %NULL
 */
GFile *
gdata_mock_server_get_trace_directory (GDataMockServer *self)
{
	g_return_val_if_fail (GDATA_IS_MOCK_SERVER (self), NULL);

	return self->priv->trace_directory;
}

/**
 * gdata_mock_server_set_trace_directory:
 * @self: a #GDataMockServer
 * @trace_directory: (allow-none) (transfer none): a directory to load/store trace files from, or %NULL to unset it
 *
 * Sets the value of the #GDataMockServer:trace-directory property.
 */
void
gdata_mock_server_set_trace_directory (GDataMockServer *self, GFile *trace_directory)
{
	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));
	g_return_if_fail (trace_directory == NULL || G_IS_FILE (trace_directory));

	if (trace_directory != NULL) {
		g_object_ref (trace_directory);
	}

	g_clear_object (&self->priv->trace_directory);
	self->priv->trace_directory = trace_directory;
	g_object_notify (G_OBJECT (self), "trace-directory");
}

/**
 * gdata_mock_server_start_trace:
 * @self: a #GDataMockServer
 * @trace_name: name of the trace
 *
 * Starts a mock server which follows the trace file of filename @trace_name in the #GDataMockServer:trace-directory directory.
 * See gdata_mock_server_start_trace_full() for further documentation.
 *
 * This function has undefined behaviour if #GDataMockServer:trace-directory is %NULL.
 */
void
gdata_mock_server_start_trace (GDataMockServer *self, const gchar *trace_name)
{
	GFile *trace_file;

	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));
	g_return_if_fail (trace_name != NULL && *trace_name != '\0');

	g_assert (self->priv->trace_directory != NULL);

	trace_file = g_file_get_child (self->priv->trace_directory, trace_name);
	gdata_mock_server_start_trace_full (self, trace_file);
	g_object_unref (trace_file);
}

/**
 * gdata_mock_server_start_trace_full:
 * @self: a #GDataMockServer
 * @trace_file: a trace file to load
 *
 * Convenience function to start logging to or reading from the given @trace_file, depending on the values of #GDataMockServer:enable-logging and
 * #GDataMockServer:enable-online.
 *
 * If #GDataMockServer:enable-logging is %TRUE, a log handler will be set up to redirect all client network activity into the given @trace_file.
 * If @trace_file already exists, it will be overwritten.
 *
 * If #GDataMockServer:enable-online is %FALSE, the given @trace_file is loaded using gdata_mock_server_load_trace() and then a mock server is
 * started using gdata_mock_server_run().
 *
 * On error, a warning message will be printed. FIXME: Ewww.
 */
void
gdata_mock_server_start_trace_full (GDataMockServer *self, GFile *trace_file)
{
	GDataMockServerPrivate *priv = self->priv;
	GError *child_error = NULL;

	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));
	g_return_if_fail (G_IS_FILE (trace_file));

	if (priv->output_stream != NULL) {
		g_warning ("%s: Nested trace files are not supported. Call gdata_mock_server_end_trace() before calling %s again.", G_STRFUNC, G_STRFUNC);
	}
	g_return_if_fail (priv->output_stream == NULL);

	/* Start writing out a trace file if logging is enabled. */
	if (priv->enable_logging == TRUE) {
		priv->output_stream = g_file_replace (trace_file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &child_error);

		if (child_error != NULL) {
			gchar *trace_file_path;

			trace_file_path = g_file_get_path (trace_file);
			g_warning ("Error replacing trace file ‘%s’: %s", trace_file_path, child_error->message);
			g_free (trace_file_path);

			g_error_free (child_error);

			return;
		}
	}

	/* Start reading from a trace file if online testing is disabled or if we need to compare server responses to the trace file. */
	if (priv->enable_online == FALSE) {
		gdata_mock_server_run (self);
		gdata_mock_server_load_trace (self, trace_file, NULL, &child_error);

		if (child_error != NULL) {
			gchar *trace_file_path = g_file_get_path (trace_file);
			g_error ("Error loading trace file ‘%s’: %s", trace_file_path, child_error->message);
			g_free (trace_file_path);

			g_error_free (child_error);

			gdata_mock_server_stop (self);

			return;
		}
	} else if (priv->enable_online == TRUE && priv->enable_logging == FALSE) {
		gdata_mock_server_load_trace (self, trace_file, NULL, &child_error);

		if (child_error != NULL) {
			gchar *trace_file_path = g_file_get_path (trace_file);
			g_error ("Error loading trace file ‘%s’: %s", trace_file_path, child_error->message);
			g_free (trace_file_path);

			g_error_free (child_error);

			return;
		}
	}
}

/**
 * gdata_mock_server_end_trace:
 * @self: a #GDataMockServer
 *
 * Convenience function to finish logging to or reading from a trace file previously passed to gdata_mock_server_start_trace() or
 * gdata_mock_server_start_trace_full().
 *
 * If #GDataMockServer:enable-online is %FALSE, this will shut down the mock server (as if gdata_mock_server_stop() had been called).
 */
void
gdata_mock_server_end_trace (GDataMockServer *self)
{
	GDataMockServerPrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));

	if (priv->enable_online == FALSE) {
		gdata_mock_server_stop (self);
	} else if (priv->enable_online == TRUE && priv->enable_logging == FALSE) {
		gdata_mock_server_unload_trace (self);
	}

	if (priv->enable_logging == TRUE) {
		g_clear_object (&self->priv->output_stream);
	}
}

/**
 * gdata_mock_server_get_enable_online:
 * @self: a #GDataMockServer
 *
 * Gets the value of the #GDataMockServer:enable-online property.
 *
 * Return value: %TRUE if the server does not intercept and handle network connections from client code; %FALSE otherwise
 */
gboolean
gdata_mock_server_get_enable_online (GDataMockServer *self)
{
	g_return_val_if_fail (GDATA_IS_MOCK_SERVER (self), FALSE);

	return self->priv->enable_online;
}

/**
 * gdata_mock_server_set_enable_online:
 * @self: a #GDataMockServer
 * @enable_online: %TRUE to not intercept and handle network connections from client code; %FALSE otherwise
 *
 * Sets the value of the #GDataMockServer:enable-online property.
 */
void
gdata_mock_server_set_enable_online (GDataMockServer *self, gboolean enable_online)
{
	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));

	self->priv->enable_online = enable_online;
	g_object_notify (G_OBJECT (self), "enable-online");
}

/**
 * gdata_mock_server_get_enable_logging:
 * @self: a #GDataMockServer
 *
 * Gets the value of the #GDataMockServer:enable-logging property.
 *
 * Return value: %TRUE if client network traffic is being logged to a trace file; %FALSE otherwise
 */
gboolean
gdata_mock_server_get_enable_logging (GDataMockServer *self)
{
	g_return_val_if_fail (GDATA_IS_MOCK_SERVER (self), FALSE);

	return self->priv->enable_logging;
}

/**
 * gdata_mock_server_set_enable_logging:
 * @self: a #GDataMockServer
 * @enable_logging: %TRUE to log client network traffic to a trace file; %FALSE otherwise
 *
 * Sets the value of the #GDataMockServer:enable-logging property.
 */
void
gdata_mock_server_set_enable_logging (GDataMockServer *self, gboolean enable_logging)
{
	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));

	self->priv->enable_logging = enable_logging;
	g_object_notify (G_OBJECT (self), "enable-logging");
}

/**
 * gdata_mock_server_received_message_chunk:
 * @self: a #GDataMockServer
 * @message_chunk: single line of a message which was received
 * @message_chunk_length: length of @message_chunk in bytes
 *
 * Indicates to the mock server that a single new line of a message was received from the real server. The message line may be
 * appended to the current trace file if logging is enabled (#GDataMockServer:enable-logging is %TRUE), adding a newline character
 * at the end. If logging is disabled but online mode is enabled (#GDataMockServer:enable-online is %TRUE), the message line will
 * be compared to the next expected line in the existing trace file. Otherwise, this function is a no-op.
 *
 * On error, a warning will be printed. FIXME: That's icky.
 */
void
gdata_mock_server_received_message_chunk (GDataMockServer *self, const gchar *message_chunk, goffset message_chunk_length)
{
	GDataMockServerPrivate *priv = self->priv;
	GError *child_error = NULL;

	g_return_if_fail (GDATA_IS_MOCK_SERVER (self));
	g_return_if_fail (message_chunk != NULL);

	/* Silently ignore the call if logging is disabled and we're offline, or if a trace file hasn't been specified. */
	if ((priv->enable_logging == FALSE && priv->enable_online == FALSE) || (priv->enable_logging == TRUE && priv->output_stream == NULL)) {
		return;
	}

	/* Simple state machine to track where we are in the soup log format. */
	switch (priv->received_message_state) {
		case UNKNOWN:
			if (strncmp (message_chunk, "> ", 2) == 0) {
				priv->received_message_state = REQUEST_DATA;
			}
			break;
		case REQUEST_DATA:
			if (strcmp (message_chunk, "  ") == 0) {
				priv->received_message_state = REQUEST_TERMINATOR;
			} else if (strncmp (message_chunk, "> ", 2) != 0) {
				priv->received_message_state = UNKNOWN;
			}
			break;
		case REQUEST_TERMINATOR:
			if (strncmp (message_chunk, "< ", 2) == 0) {
				priv->received_message_state = RESPONSE_DATA;
			} else {
				priv->received_message_state = UNKNOWN;
			}
			break;
		case RESPONSE_DATA:
			if (strcmp (message_chunk, "  ") == 0) {
				priv->received_message_state = RESPONSE_TERMINATOR;
			} else if (strncmp (message_chunk, "< ", 2) != 0) {
				priv->received_message_state = UNKNOWN;
			}
			break;
		case RESPONSE_TERMINATOR:
			if (strncmp (message_chunk, "> ", 2) == 0) {
				priv->received_message_state = REQUEST_DATA;
			} else {
				priv->received_message_state = UNKNOWN;
			}
			break;
		default:
			g_assert_not_reached ();
	}

	/* Silently ignore responses outputted by libsoup before the requests. This can happen when a SoupMessage is cancelled part-way through
	 * sending the request; in which case libsoup logs only a response of the form:
	 *     < HTTP/1.1 1 Cancelled
	 *     < Soup-Debug-Timestamp: 1375190963
	 *     < Soup-Debug: SoupMessage 0 (0x7fffe00261c0)
	 */
	if (priv->received_message_state == UNKNOWN) {
		return;
	}

	/* Append to the trace file. */
	if (priv->enable_logging == TRUE &&
	    (g_output_stream_write_all (G_OUTPUT_STREAM (priv->output_stream), message_chunk, message_chunk_length, NULL, NULL, &child_error) == FALSE ||
	     g_output_stream_write_all (G_OUTPUT_STREAM (priv->output_stream), "\n", 1, NULL, NULL, &child_error) == FALSE)) {
		gchar *trace_file_path = g_file_get_path (priv->trace_file);
		g_warning ("Error appending to log file ‘%s’: %s", trace_file_path, child_error->message);
		g_free (trace_file_path);

		g_error_free (child_error);

		return;
	}

	/* Or compare to the existing trace file. */
	if (priv->enable_logging == FALSE && priv->enable_online == TRUE) {
		/* Build up the message to compare. */
		/* TODO: escape null bytes? */
		g_byte_array_append (priv->comparison_message, (const guint8 *) message_chunk, message_chunk_length);
		g_byte_array_append (priv->comparison_message, (const guint8 *) '\n', 1);

		if (strcmp (message_chunk, "  ") == 0) {
			/* Received the last chunk of the response, so compare the message from the trace file and that from online. */
			SoupMessage *online_message;
			SoupURI *base_uri;

			/* End of a message. */
			base_uri = soup_uri_new ("https://localhost/"); /* FIXME */
			online_message = trace_to_soup_message ((const gchar *) priv->comparison_message->data, base_uri);
			soup_uri_free (base_uri);

			g_byte_array_set_size (priv->comparison_message, 0);
			priv->received_message_state = UNKNOWN;

			g_assert (priv->next_message != NULL);

			/* Compare the message from the server with the message in the log file. */
			if (compare_incoming_message (online_message, priv->next_message, NULL) != 0) {
				gchar *next_uri, *actual_uri;

				next_uri = soup_uri_to_string (soup_message_get_uri (priv->next_message), TRUE);
				actual_uri = soup_uri_to_string (soup_message_get_uri (online_message), TRUE);
				g_warning ("Expected URI ‘%s’, but got ‘%s’.", next_uri, actual_uri);
				g_free (actual_uri);
				g_free (next_uri);

				g_object_unref (online_message);

				return;
			}

			g_object_unref (online_message);
		}
	}
}

/**
 * gdata_mock_server_get_address:
 * @self: a #GDataMockServer
 *
 * Gets the value of the #GDataMockServer:address property.
 *
 * Return value: (allow-none) (transfer none): the address of the listening socket the server is currently bound to; or %NULL if the server is not running
 */
SoupAddress *
gdata_mock_server_get_address (GDataMockServer *self)
{
	g_return_val_if_fail (GDATA_IS_MOCK_SERVER (self), NULL);

	return self->priv->address;
}

/**
 * gdata_mock_server_get_port:
 * @self: a #GDataMockServer
 *
 * Gets the value of the #GDataMockServer:port property.
 *
 * Return value: the port of the listening socket the server is currently bound to; or <code class="literal">0</code> if the server is not running
 */
guint
gdata_mock_server_get_port (GDataMockServer *self)
{
	g_return_val_if_fail (GDATA_IS_MOCK_SERVER (self), 0);

	return self->priv->port;
}

/**
 * gdata_mock_server_get_resolver:
 * @self: a #GDataMockServer
 *
 * Gets the value of the #GDataMockServer:resolver property.
 *
 * Return value: (allow-none) (transfer none): the mock resolver in use by the mock server, or %NULL if no resolver is active
 */
GDataMockResolver *
gdata_mock_server_get_resolver (GDataMockServer *self)
{
	g_return_val_if_fail (GDATA_IS_MOCK_SERVER (self), NULL);

	return self->priv->resolver;
}
