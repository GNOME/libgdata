/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008, 2009, 2010, 2014 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-service
 * @short_description: GData service object
 * @stability: Stable
 * @include: gdata/gdata-service.h
 *
 * #GDataService represents a GData API service, typically a website using the GData API, such as YouTube or Google Calendar. One
 * #GDataService instance is required to issue queries to the service, handle insertions, updates and deletions, and generally
 * communicate with the online service.
 *
 * If operations performed on a #GDataService need authorization (such as uploading a video to YouTube or querying the user's personal calendar on
 * Google Calendar), the service needs a #GDataAuthorizer instance set as #GDataService:authorizer. Once the user is appropriately authenticated and
 * authorized by the #GDataAuthorizer implementation (see the documentation for #GDataAuthorizer for details on how this is achieved for specific
 * implementations), all operations will be automatically authorized.
 *
 * Note that it's not always necessary to supply a #GDataAuthorizer instance to a #GDataService. If the only operations to be performed on the
 * #GDataService don't need authorization (e.g. they only query public information), setting up a #GDataAuthorizer is just extra overhead. See the
 * documentation for the operations on individual #GDataService subclasses to see which need authorization and which don't.
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libsoup/soup.h>
#include <string.h>
#include <stdarg.h>

#ifdef HAVE_GNOME
#define GCR_API_SUBJECT_TO_CHANGE
#include <gcr/gcr.h>
#endif /* HAVE_GNOME */

#include "gdata-service.h"
#include "gdata-private.h"
#include "gdata-marshal.h"
#include "gdata-types.h"

GQuark
gdata_service_error_quark (void)
{
	return g_quark_from_static_string ("gdata-service-error-quark");
}

static void gdata_service_dispose (GObject *object);
static void gdata_service_finalize (GObject *object);
static void gdata_service_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_service_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void real_append_query_headers (GDataService *self, GDataAuthorizationDomain *domain, SoupMessage *message);
static void real_parse_error_response (GDataService *self, GDataOperationType operation_type, guint status, const gchar *reason_phrase,
                                       const gchar *response_body, gint length, GError **error);
static GDataFeed *
real_parse_feed (GDataService *self,
                 GDataAuthorizationDomain *domain,
                 GDataQuery *query,
                 GType entry_type,
                 SoupMessage *message,
                 GCancellable *cancellable,
                 GDataQueryProgressCallback progress_callback,
                 gpointer progress_user_data,
                 GError **error);
static void notify_timeout_cb (GObject *gobject, GParamSpec *pspec, GObject *self);
static void debug_handler (const char *log_domain, GLogLevelFlags log_level, const char *message, gpointer user_data);
static void soup_log_printer (SoupLogger *logger, SoupLoggerLogLevel level, char direction, const char *data, gpointer user_data);

static GDataFeed *__gdata_service_query (GDataService *self, GDataAuthorizationDomain *domain, const gchar *feed_uri, GDataQuery *query,
                                         GType entry_type, GCancellable *cancellable, GDataQueryProgressCallback progress_callback,
                                         gpointer progress_user_data, GError **error);

struct _GDataServicePrivate {
	SoupSession *session;
	gchar *locale;
	GDataAuthorizer *authorizer;
	GProxyResolver *proxy_resolver;
};

enum {
	PROP_TIMEOUT = 1,
	PROP_LOCALE,
	PROP_AUTHORIZER,
	PROP_PROXY_RESOLVER,
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataService, gdata_service, G_TYPE_OBJECT)

static void
gdata_service_class_init (GDataServiceClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->set_property = gdata_service_set_property;
	gobject_class->get_property = gdata_service_get_property;
	gobject_class->dispose = gdata_service_dispose;
	gobject_class->finalize = gdata_service_finalize;

	klass->api_version = "2";
	klass->feed_type = GDATA_TYPE_FEED;
	klass->append_query_headers = real_append_query_headers;
	klass->parse_error_response = real_parse_error_response;
	klass->parse_feed = real_parse_feed;
	klass->get_authorization_domains = NULL; /* equivalent to returning an empty list of domains */

	/**
	 * GDataService:timeout:
	 *
	 * A timeout, in seconds, for network operations. If the timeout is exceeded, the operation will be cancelled and
	 * %GDATA_SERVICE_ERROR_NETWORK_ERROR will be returned.
	 *
	 * If the timeout is <code class="literal">0</code>, operations will never time out.
	 *
	 * Note that if a #GDataAuthorizer is being used with this #GDataService, the authorizer might also need its timeout setting.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_TIMEOUT,
	                                 g_param_spec_uint ("timeout",
	                                                    "Timeout", "A timeout, in seconds, for network operations.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataService:locale:
	 *
	 * The locale to use for network requests, in Unix locale format. (e.g. "en_GB", "cs", "de_DE".) Use %NULL for the default "C" locale
	 * (typically "en_US").
	 *
	 * Typically, this locale will be used by the server-side software to localise results, such as by translating category names, or by choosing
	 * geographically relevant search results. This will vary from service to service.
	 *
	 * The server-side behaviour is undefined if it doesn't support a given locale.
	 *
	 * Since: 0.7.0
	 */
	g_object_class_install_property (gobject_class, PROP_LOCALE,
	                                 g_param_spec_string ("locale",
	                                                      "Locale", "The locale to use for network requests, in Unix locale format.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataService:authorizer:
	 *
	 * An object which implements #GDataAuthorizer. This should have previously been authenticated authorized against this service type (and
	 * potentially other service types). The service will use the authorizer to add an authorization token to each request it performs.
	 *
	 * Your application should call methods on the #GDataAuthorizer object itself in order to authenticate with the Google accounts service and
	 * authorize against this service type. See the documentation for the particular #GDataAuthorizer implementation being used for more details.
	 *
	 * The authorizer for a service can be changed at runtime for a different #GDataAuthorizer object or %NULL without affecting ongoing requests
	 * and operations.
	 *
	 * Note that it's only necessary to set an authorizer on the service if your application is going to make requests of the service which
	 * require authorization. For example, listing the current most popular videos on YouTube does not require authorization, but uploading a
	 * video to YouTube does. It's an unnecessary overhead to require the user to authorize against a service when not strictly required.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_AUTHORIZER,
	                                 g_param_spec_object ("authorizer",
	                                                      "Authorizer", "An authorizer object to provide an authorization token for each request.",
	                                                      GDATA_TYPE_AUTHORIZER,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataService:proxy-resolver:
	 *
	 * The #GProxyResolver used to determine a proxy URI.
	 *
	 * Since: 0.15.0
	 */
	g_object_class_install_property (gobject_class, PROP_PROXY_RESOLVER,
	                                 g_param_spec_object ("proxy-resolver",
	                                                      "Proxy Resolver", "A GProxyResolver used to determine a proxy URI.",
	                                                      G_TYPE_PROXY_RESOLVER,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_service_init (GDataService *self)
{
	self->priv = gdata_service_get_instance_private (self);
	self->priv->session = _gdata_service_build_session ();

	/* Log handling for all message types except debug */
	g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_ERROR | G_LOG_LEVEL_INFO | G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_WARNING, (GLogFunc) debug_handler, self);

	/* Proxy the SoupSession's timeout property */
	g_signal_connect (self->priv->session, "notify::timeout", (GCallback) notify_timeout_cb, self);

	/* Keep our GProxyResolver synchronized with SoupSession's. */
	g_object_bind_property (self->priv->session, "proxy-resolver", self, "proxy-resolver", G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
gdata_service_dispose (GObject *object)
{
	GDataServicePrivate *priv = GDATA_SERVICE (object)->priv;

	if (priv->authorizer != NULL)
		g_object_unref (priv->authorizer);
	priv->authorizer = NULL;

	if (priv->session != NULL)
		g_object_unref (priv->session);
	priv->session = NULL;

	g_clear_object (&priv->proxy_resolver);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_service_parent_class)->dispose (object);
}

static void
gdata_service_finalize (GObject *object)
{
	GDataServicePrivate *priv = GDATA_SERVICE (object)->priv;

	g_free (priv->locale);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_service_parent_class)->finalize (object);
}

static void
gdata_service_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataServicePrivate *priv = GDATA_SERVICE (object)->priv;

	switch (property_id) {
		case PROP_TIMEOUT:
			g_value_set_uint (value, gdata_service_get_timeout (GDATA_SERVICE (object)));
			break;
		case PROP_LOCALE:
			g_value_set_string (value, priv->locale);
			break;
		case PROP_AUTHORIZER:
			g_value_set_object (value, priv->authorizer);
			break;
		case PROP_PROXY_RESOLVER:
			g_value_set_object (value, priv->proxy_resolver);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_service_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_TIMEOUT:
			gdata_service_set_timeout (GDATA_SERVICE (object), g_value_get_uint (value));
			break;
		case PROP_LOCALE:
			gdata_service_set_locale (GDATA_SERVICE (object), g_value_get_string (value));
			break;
		case PROP_AUTHORIZER:
			gdata_service_set_authorizer (GDATA_SERVICE (object), g_value_get_object (value));
			break;
		case PROP_PROXY_RESOLVER:
			gdata_service_set_proxy_resolver (GDATA_SERVICE (object), g_value_get_object (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
real_append_query_headers (GDataService *self, GDataAuthorizationDomain *domain, SoupMessage *message)
{
	g_assert (message != NULL);

	/* Set the authorisation header */
	if (self->priv->authorizer != NULL) {
		gdata_authorizer_process_request (self->priv->authorizer, domain, message);

		if (domain != NULL) {
			/* Store the authorisation domain on the message so that we can access it again after refreshing authorisation if necessary.
			 * See _gdata_service_send_message(). */
			g_object_set_data_full (G_OBJECT (message), "gdata-authorization-domain", g_object_ref (domain),
			                        (GDestroyNotify) g_object_unref);
		}
	}

	/* Set the GData-Version header to tell it we want to use the v2 API */
	soup_message_headers_append (message->request_headers, "GData-Version", GDATA_SERVICE_GET_CLASS (self)->api_version);

	/* Set the locale, if it's been set for the service */
	if (self->priv->locale != NULL)
		soup_message_headers_append (message->request_headers, "Accept-Language", self->priv->locale);
}

static void
real_parse_error_response (GDataService *self, GDataOperationType operation_type, guint status, const gchar *reason_phrase,
                           const gchar *response_body, gint length, GError **error)
{
	/* We prefer to include the @response_body in the error message, but if it's empty, fall back to the @reason_phrase */
	if (response_body == NULL || *response_body == '\0')
		response_body = reason_phrase;

	/* See: http://code.google.com/apis/gdata/docs/2.0/reference.html#HTTPStatusCodes */
	switch (status) {
		case SOUP_STATUS_CANT_RESOLVE:
		case SOUP_STATUS_CANT_CONNECT:
		case SOUP_STATUS_SSL_FAILED:
		case SOUP_STATUS_IO_ERROR:
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_NETWORK_ERROR,
			             _("Cannot connect to the service’s server."));
			return;
		case SOUP_STATUS_CANT_RESOLVE_PROXY:
		case SOUP_STATUS_CANT_CONNECT_PROXY:
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROXY_ERROR,
			             _("Cannot connect to the proxy server."));
			return;
		case SOUP_STATUS_MALFORMED:
		case SOUP_STATUS_BAD_REQUEST: /* 400 */
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the parameter is an error message returned by the server. */
			             _("Invalid request URI or header, or unsupported nonstandard parameter: %s"), response_body);
			return;
		case SOUP_STATUS_UNAUTHORIZED: /* 401 */
		case SOUP_STATUS_FORBIDDEN: /* 403 */
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_AUTHENTICATION_REQUIRED,
			             /* Translators: the parameter is an error message returned by the server. */
			             _("Authentication required: %s"), response_body);
			return;
		case SOUP_STATUS_NOT_FOUND: /* 404 */
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_NOT_FOUND,
			             /* Translators: the parameter is an error message returned by the server. */
			             _("The requested resource was not found: %s"), response_body);
			return;
		case SOUP_STATUS_CONFLICT: /* 409 */
		case SOUP_STATUS_PRECONDITION_FAILED: /* 412 */
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_CONFLICT,
			             /* Translators: the parameter is an error message returned by the server. */
			             _("The entry has been modified since it was downloaded: %s"), response_body);
			return;
		case SOUP_STATUS_INTERNAL_SERVER_ERROR: /* 500 */
		default:
			/* We'll fall back to generic errors, below */
			break;
	}

	/* If the error hasn't been handled already, throw a generic error */
	switch (operation_type) {
		case GDATA_OPERATION_AUTHENTICATION:
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the first parameter is an HTTP status,
			              * and the second is an error message returned by the server. */
			             _("Error code %u when authenticating: %s"), status, response_body);
			break;
		case GDATA_OPERATION_QUERY:
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the first parameter is an HTTP status,
			              * and the second is an error message returned by the server. */
			             _("Error code %u when querying: %s"), status, response_body);
			break;
		case GDATA_OPERATION_INSERTION:
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the first parameter is an HTTP status,
			              * and the second is an error message returned by the server. */
			             _("Error code %u when inserting an entry: %s"), status, response_body);
			break;
		case GDATA_OPERATION_UPDATE:
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the first parameter is an HTTP status,
			              * and the second is an error message returned by the server. */
			             _("Error code %u when updating an entry: %s"), status, response_body);
			break;
		case GDATA_OPERATION_DELETION:
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the first parameter is an HTTP status,
			              * and the second is an error message returned by the server. */
			             _("Error code %u when deleting an entry: %s"), status, response_body);
			break;
		case GDATA_OPERATION_DOWNLOAD:
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the first parameter is an HTTP status,
			              * and the second is an error message returned by the server. */
			             _("Error code %u when downloading: %s"), status, response_body);
			break;
		case GDATA_OPERATION_UPLOAD:
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the first parameter is an HTTP status,
			              * and the second is an error message returned by the server. */
			             _("Error code %u when uploading: %s"), status, response_body);
			break;
		case GDATA_OPERATION_BATCH:
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_WITH_BATCH_OPERATION,
			             /* Translators: the first parameter is a HTTP status,
			              * and the second is an error message returned by the server. */
			             _("Error code %u when running a batch operation: %s"), status, response_body);
			break;
		default:
			/* We should not be called with anything other than the above operation types */
			g_assert_not_reached ();
	}
}

/**
 * gdata_service_is_authorized:
 * @self: a #GDataService
 *
 * Determines whether the service is authorized for all the #GDataAuthorizationDomains it belongs to (as returned by
 * gdata_service_get_authorization_domains()). If the service's #GDataService:authorizer is %NULL, %FALSE is always returned.
 *
 * This is basically a convenience method for checking that the service's #GDataAuthorizer is authorized for all the service's
 * #GDataAuthorizationDomains.
 *
 * Return value: %TRUE if the service is authorized for all its domains, %FALSE otherwise
 *
 * Since: 0.9.0
 */
gboolean
gdata_service_is_authorized (GDataService *self)
{
	GList *domains, *i;
	gboolean authorised = TRUE;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), FALSE);

	/* If we don't have an authoriser set, we can't be authorised */
	if (self->priv->authorizer == NULL) {
		return FALSE;
	}

	domains = gdata_service_get_authorization_domains (G_OBJECT_TYPE (self));

	/* Find any domains which we're not authorised for */
	for (i = domains; i != NULL; i = i->next) {
		if (gdata_authorizer_is_authorized_for_domain (self->priv->authorizer, GDATA_AUTHORIZATION_DOMAIN (i->data)) == FALSE) {
			authorised = FALSE;
			break;
		}
	}

	g_list_free (domains);

	return authorised;
}

/**
 * gdata_service_get_authorizer:
 * @self: a #GDataService
 *
 * Gets the #GDataAuthorizer object currently in use by the service. See the documentation for #GDataService:authorizer for more details.
 *
 * Return value: (transfer none): the authorizer object for this service, or %NULL
 *
 * Since: 0.9.0
 */
GDataAuthorizer *
gdata_service_get_authorizer (GDataService *self)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);

	return self->priv->authorizer;
}

/**
 * gdata_service_set_authorizer:
 * @self: a #GDataService
 * @authorizer: a new authorizer object for the service, or %NULL
 *
 * Sets #GDataService:authorizer to @authorizer. This may be %NULL if the service will only make requests in future which don't require authorization.
 * See the documentation for #GDataService:authorizer for more information.
 *
 * Since: 0.9.0
 */
void
gdata_service_set_authorizer (GDataService *self, GDataAuthorizer *authorizer)
{
	GDataServicePrivate *priv = self->priv;

	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_return_if_fail (authorizer == NULL || GDATA_IS_AUTHORIZER (authorizer));

	if (priv->authorizer != NULL) {
		g_object_unref (priv->authorizer);
	}

	priv->authorizer = authorizer;

	if (priv->authorizer != NULL) {
		g_object_ref (priv->authorizer);
	}

	g_object_notify (G_OBJECT (self), "authorizer");
}

/**
 * gdata_service_get_authorization_domains:
 * @service_type: the #GType of the #GDataService subclass to retrieve the authorization domains for
 *
 * Retrieves the full list of #GDataAuthorizationDomains which relate to the specified @service_type. All the
 * #GDataAuthorizationDomains are unique and interned, so can be compared with other domains by simple pointer comparison.
 *
 * Note that in addition to this method, #GDataService subclasses may expose some or all of their authorization domains individually by means of
 * individual accessor functions.
 *
 * Return value: (transfer container) (element-type GDataAuthorizationDomain): an unordered list of #GDataAuthorizationDomains; free with
 * g_list_free()
 *
 * Since: 0.9.0
 */
GList *
gdata_service_get_authorization_domains (GType service_type)
{
	GDataServiceClass *klass;
	GList *domains = NULL;

	g_return_val_if_fail (g_type_is_a (service_type, GDATA_TYPE_SERVICE), NULL);

	klass = GDATA_SERVICE_CLASS (g_type_class_ref (service_type));
	if (klass->get_authorization_domains != NULL) {
		domains = klass->get_authorization_domains ();
	}
	g_type_class_unref (klass);

	return domains;
}

SoupMessage *
_gdata_service_build_message (GDataService *self, GDataAuthorizationDomain *domain, const gchar *method, const gchar *uri,
                              const gchar *etag, gboolean etag_if_match)
{
	SoupMessage *message;
	GDataServiceClass *klass;
	SoupURI *_uri;

	/* Create the message. Allow changing the HTTPS port just for testing,
	 * but require that the URI is always HTTPS for privacy. */
	_uri = soup_uri_new (uri);
	soup_uri_set_port (_uri, _gdata_service_get_https_port ());
	g_assert_cmpstr (soup_uri_get_scheme (_uri), ==, SOUP_URI_SCHEME_HTTPS);
	message = soup_message_new_from_uri (method, _uri);
	soup_uri_free (_uri);

	/* Make sure subclasses set their headers */
	klass = GDATA_SERVICE_GET_CLASS (self);
	if (klass->append_query_headers != NULL)
		klass->append_query_headers (self, domain, message);

	/* Append the ETag header if possible */
	if (etag != NULL)
		soup_message_headers_append (message->request_headers, (etag_if_match == TRUE) ? "If-Match" : "If-None-Match", etag);

	return message;
}

typedef struct {
	GMutex mutex; /* mutex to prevent cancellation before the message has been added to the session's message queue */
	SoupSession *session;
	SoupMessage *message;
} MessageData;

static void
message_cancel_cb (GCancellable *cancellable, MessageData *data)
{
	g_mutex_lock (&(data->mutex));
	soup_session_cancel_message (data->session, data->message, SOUP_STATUS_CANCELLED);
	g_mutex_unlock (&(data->mutex));
}

static void
message_request_queued_cb (SoupSession *session, SoupMessage *message, MessageData *data)
{
	if (message == data->message) {
		g_mutex_unlock (&(data->mutex));
	}
}

/* Synchronously send @message via @service, handling asynchronous cancellation as best we can. If @cancellable has been cancelled before we start
 * network activity, return without doing any network activity. Otherwise, if @cancellable is cancelled (from another thread) after network activity
 * has started, we wait until the message has been queued by the session, then cancel the network activity and return as soon as possible.
 *
 * If cancellation has been handled, @error is guaranteed to be set to %G_IO_ERROR_CANCELLED. Otherwise, @error is guaranteed to be unset. */
void
_gdata_service_actually_send_message (SoupSession *session, SoupMessage *message, GCancellable *cancellable, GError **error)
{
	MessageData data;
	gulong cancel_signal = 0, request_queued_signal = 0;

	/* Hold references to the session and message so they can't be freed by other threads. For example, if the SoupSession was freed by another
	 * thread while we were making a request, the request would be unexpectedly cancelled. See bgo#650835 for an example of this breaking things.
	 */
	g_object_ref (session);
	g_object_ref (message);

	/* Listen for cancellation */
	if (cancellable != NULL) {
		g_mutex_init (&(data.mutex));
		data.session = session;
		data.message = message;

		cancel_signal = g_cancellable_connect (cancellable, (GCallback) message_cancel_cb, &data, NULL);
		request_queued_signal = g_signal_connect (session, "request-queued", (GCallback) message_request_queued_cb, &data);

		/* We lock this mutex until the message has been queued by the session (i.e. it's unlocked in the request-queued callback), and require
		 * the mutex to be held to cancel the message. Consequently, if the message is cancelled (in another thread) any time between this lock
		 * and the request being queued, the cancellation will wait until the request has been queued before taking effect.
		 * This is a little ugly, but is the only way I can think of to avoid a race condition between calling soup_session_cancel_message()
		 * and soup_session_send_message(), as the former doesn't have any effect until the request has been queued, and once the latter has
		 * returned, all network activity has been finished so cancellation is pointless. */
		g_mutex_lock (&(data.mutex));
	}

	/* Only send the message if it hasn't already been cancelled. There is no race condition here for the above reasons: if the cancellable has
	 * been cancelled, it's because it was cancelled before we called g_cancellable_connect().
	 *
	 * Otherwise, manually set the message's status code to SOUP_STATUS_CANCELLED, as the message was cancelled before even being queued to be
	 * sent. */
	if (cancellable == NULL || g_cancellable_is_cancelled (cancellable) == FALSE)
		soup_session_send_message (session, message);
	else {
		if (cancellable != NULL) {
			g_mutex_unlock (&data.mutex);
		}

		soup_message_set_status (message, SOUP_STATUS_CANCELLED);
	}

	/* Clean up the cancellation code */
	if (cancellable != NULL) {
		g_signal_handler_disconnect (session, request_queued_signal);

		if (cancel_signal != 0)
			g_cancellable_disconnect (cancellable, cancel_signal);

		g_mutex_clear (&(data.mutex));
	}

	/* Set the cancellation error if applicable. We can't assume that our GCancellable has been cancelled just because the message has;
	 * libsoup may internally cancel messages if, for example, the proxy URI of the SoupSession is changed.
	 * libsoup also sometimes seems to return a SOUP_STATUS_IO_ERROR when we cancel a message, even though we've specified SOUP_STATUS_CANCELLED
	 * at cancellation time. Ho Hum. */
	g_assert (message->status_code != SOUP_STATUS_NONE);

	if (message->status_code == SOUP_STATUS_CANCELLED ||
	    ((message->status_code == SOUP_STATUS_IO_ERROR || message->status_code == SOUP_STATUS_SSL_FAILED ||
	      message->status_code == SOUP_STATUS_CANT_CONNECT || message->status_code == SOUP_STATUS_CANT_RESOLVE) &&
	     cancellable != NULL && g_cancellable_is_cancelled (cancellable) == TRUE)) {
		/* We hackily create and cancel a new GCancellable so that we can set the error using it and therefore save ourselves a translatable
		 * string and the associated maintenance. */
		GCancellable *error_cancellable = g_cancellable_new ();
		g_cancellable_cancel (error_cancellable);
		g_assert (g_cancellable_set_error_if_cancelled (error_cancellable, error) == TRUE);
		g_object_unref (error_cancellable);

		/* As per the above comment, force the status to be SOUP_STATUS_CANCELLED. */
		soup_message_set_status (message, SOUP_STATUS_CANCELLED);
	}

	/* Free things */
	g_object_unref (message);
	g_object_unref (session);
}

guint
_gdata_service_send_message (GDataService *self, SoupMessage *message, GCancellable *cancellable, GError **error)
{
	/* Based on code from evolution-data-server's libgdata:
	 *  Ebby Wiselyn <ebbywiselyn@gmail.com>
	 *  Jason Willis <zenbrother@gmail.com>
	 *
	 * Copyright (C) 1999-2008 Novell, Inc. (www.novell.com)
	 */

	soup_message_set_flags (message, SOUP_MESSAGE_NO_REDIRECT);
	_gdata_service_actually_send_message (self->priv->session, message, cancellable, error);
	soup_message_set_flags (message, 0);

	/* Handle redirections specially so we don't lose our custom headers when making the second request */
	if (SOUP_STATUS_IS_REDIRECTION (message->status_code)) {
		SoupURI *new_uri;
		const gchar *new_location;

		new_location = soup_message_headers_get_one (message->response_headers, "Location");
		g_return_val_if_fail (new_location != NULL, SOUP_STATUS_NONE);

		new_uri = soup_uri_new_with_base (soup_message_get_uri (message), new_location);
		if (new_uri == NULL) {
			gchar *uri_string = soup_uri_to_string (new_uri, FALSE);
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
			             /* Translators: the parameter is the URI which is invalid. */
			             _("Invalid redirect URI: %s"), uri_string);
			g_free (uri_string);
			return SOUP_STATUS_NONE;
		}

		/* Allow overriding the URI for testing. */
		soup_uri_set_port (new_uri, _gdata_service_get_https_port ());

		soup_message_set_uri (message, new_uri);
		soup_uri_free (new_uri);

		/* Send the message again */
		_gdata_service_actually_send_message (self->priv->session, message, cancellable, error);
	}

	/* Not authorised, or authorisation has expired. If we were authorised in the first place, attempt to refresh the authorisation and
	 * try sending the message again (but only once, so we don't get caught in an infinite loop of denied authorisation errors).
	 *
	 * Note that we have to re-process the message with the authoriser so that its authorisation headers get updated after the refresh
	 * (bgo#653535). */
	if (message->status_code == SOUP_STATUS_UNAUTHORIZED ||
	    message->status_code == SOUP_STATUS_FORBIDDEN ||
	    message->status_code == SOUP_STATUS_NOT_FOUND) {
		GDataAuthorizer *authorizer = self->priv->authorizer;

		if (authorizer != NULL && gdata_authorizer_refresh_authorization (authorizer, cancellable, NULL) == TRUE) {
			GDataAuthorizationDomain *domain;

			/* Re-process the request */
			domain = g_object_get_data (G_OBJECT (message), "gdata-authorization-domain");
			g_assert (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain));

			gdata_authorizer_process_request (authorizer, domain, message);

			/* Send the message again */
			g_clear_error (error);
			_gdata_service_actually_send_message (self->priv->session, message, cancellable, error);
		}
	}

	return message->status_code;
}

typedef struct {
	/* Input */
	GDataAuthorizationDomain *domain;
	gchar *feed_uri;
	GDataQuery *query;
	GType entry_type;

	/* Output */
	GDataQueryProgressCallback progress_callback;
	gpointer progress_user_data;
	GDestroyNotify destroy_progress_user_data;
} QueryAsyncData;

static void
query_async_data_free (QueryAsyncData *self)
{
	if (self->domain != NULL)
		g_object_unref (self->domain);

	g_free (self->feed_uri);
	if (self->query)
		g_object_unref (self->query);

	g_slice_free (QueryAsyncData, self);
}

static void
query_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataService *service = GDATA_SERVICE (source_object);
	g_autoptr(GError) error = NULL;
	QueryAsyncData *data = task_data;
	g_autoptr(GDataFeed) feed = NULL;

	/* Execute the query and return */
	feed = __gdata_service_query (service, data->domain, data->feed_uri, data->query, data->entry_type, cancellable,
	                              data->progress_callback, data->progress_user_data, &error);
	if (feed == NULL && error != NULL)
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_pointer (task, g_steal_pointer (&feed), g_object_unref);

	if (data->destroy_progress_user_data != NULL) {
		data->destroy_progress_user_data (data->progress_user_data);
	}
}

/**
 * gdata_service_query_async:
 * @self: a #GDataService
 * @domain: (allow-none): the #GDataAuthorizationDomain the query falls under, or %NULL
 * @feed_uri: the feed URI to query, including the host name and protocol
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @entry_type: a #GType for the #GDataEntrys to build from the XML
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @destroy_progress_user_data: (allow-none): the function to call when @progress_callback will not be called any more, or %NULL. This function will be
 * called with @progress_user_data as a parameter and can be used to free any memory allocated for it.
 * @callback: a #GAsyncReadyCallback to call when the query is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Queries the service's @feed_uri feed to build a #GDataFeed. @self, @feed_uri and
 * @query are all reffed/copied when this function is called, so can safely be freed after this function returns.
 *
 * For more details, see gdata_service_query(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_query_finish()
 * to get the results of the operation.
 *
 * Since: 0.9.1
 */
void
gdata_service_query_async (GDataService *self, GDataAuthorizationDomain *domain, const gchar *feed_uri, GDataQuery *query, GType entry_type,
                           GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                           GDestroyNotify destroy_progress_user_data, GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;
	QueryAsyncData *data;

	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_return_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain));
	g_return_if_fail (feed_uri != NULL);
	g_return_if_fail (g_type_is_a (entry_type, GDATA_TYPE_ENTRY));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	data = g_slice_new (QueryAsyncData);
	data->domain = (domain != NULL) ? g_object_ref (domain) : NULL;
	data->feed_uri = g_strdup (feed_uri);
	data->query = (query != NULL) ? g_object_ref (query) : NULL;
	data->entry_type = entry_type;
	data->progress_callback = progress_callback;
	data->progress_user_data = progress_user_data;
	data->destroy_progress_user_data = destroy_progress_user_data;

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_service_query_async);
	g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) query_async_data_free);
	g_task_run_in_thread (task, query_thread);
}

/**
 * gdata_service_query_finish:
 * @self: a #GDataService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous query operation started with gdata_service_query_async().
 *
 * Return value: (transfer full): a #GDataFeed of query results, or %NULL; unref with g_object_unref()
 */
GDataFeed *
gdata_service_query_finish (GDataService *self, GAsyncResult *async_result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	g_return_val_if_fail (g_task_is_valid (async_result, self), NULL);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_service_query_async), NULL);

	return g_task_propagate_pointer (G_TASK (async_result), error);
}

/* Does the bulk of the work of gdata_service_query. Split out because certain queries (such as that done by
 * gdata_service_query_single_entry()) only return a single entry, and thus need special parsing code. */
SoupMessage *
_gdata_service_query (GDataService *self, GDataAuthorizationDomain *domain, const gchar *feed_uri, GDataQuery *query,
                      GCancellable *cancellable, GError **error)
{
	SoupMessage *message;
	guint status;
	const gchar *etag = NULL;

	/* Append the ETag header if possible */
	if (query != NULL)
		etag = gdata_query_get_etag (query);

	/* Build the message */
	if (query != NULL) {
		gchar *query_uri = gdata_query_get_query_uri (query, feed_uri);
		message = _gdata_service_build_message (self, domain, SOUP_METHOD_GET, query_uri, etag, FALSE);
		g_free (query_uri);
	} else {
		message = _gdata_service_build_message (self, domain, SOUP_METHOD_GET, feed_uri, etag, FALSE);
	}

	/* Note that cancellation only applies to network activity; not to the processing done afterwards */
	status = _gdata_service_send_message (self, message, cancellable, error);

	if (status == SOUP_STATUS_NOT_MODIFIED || status == SOUP_STATUS_CANCELLED) {
		/* Not modified (ETag has worked), or cancelled (in which case the error has been set) */
		g_object_unref (message);
		return NULL;
	} else if (status != SOUP_STATUS_OK) {
		/* Error */
		GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (self);
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (self, GDATA_OPERATION_QUERY, status, message->reason_phrase, message->response_body->data,
		                             message->response_body->length, error);
		g_object_unref (message);
		return NULL;
	}

	return message;
}

static GDataFeed *
__gdata_service_query (GDataService *self, GDataAuthorizationDomain *domain, const gchar *feed_uri, GDataQuery *query, GType entry_type,
                       GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	GDataServiceClass *klass;
	SoupMessage *message;
	GDataFeed *feed;

	klass = GDATA_SERVICE_GET_CLASS (self);

	/* Are we off the end of the final page? */
	if (query != NULL && _gdata_query_is_finished (query)) {
		/* Build an empty dummy feed to signify the end of the list. */
		return _gdata_feed_new (klass->feed_type, "Empty feed", "feed1",
		                        g_get_real_time () / G_USEC_PER_SEC);
	}

	/* Send the request. */
	message = _gdata_service_query (self, domain, feed_uri, query, cancellable, error);
	if (message == NULL)
		return NULL;

	g_assert (message->response_body->data != NULL);
	g_assert (klass->parse_feed != NULL);

	/* Parse the response. */
	feed = klass->parse_feed (self, domain, query, entry_type,
	                          message, cancellable, progress_callback,
	                          progress_user_data, error);

	g_object_unref (message);

	return feed;
}

static GDataFeed *
real_parse_feed (GDataService *self,
                 GDataAuthorizationDomain *domain,
                 GDataQuery *query,
                 GType entry_type,
                 SoupMessage *message,
                 GCancellable *cancellable,
                 GDataQueryProgressCallback progress_callback,
                 gpointer progress_user_data,
                 GError **error)
{
	GDataServiceClass *klass;
	GDataFeed *feed = NULL;
	SoupMessageHeaders *headers;
	const gchar *content_type;

	klass = GDATA_SERVICE_GET_CLASS (self);
	headers = message->response_headers;
	content_type = soup_message_headers_get_content_type (headers, NULL);

	if (content_type != NULL && strcmp (content_type, "application/json") == 0) {
		/* Definitely JSON. */
		g_debug("JSON content type detected.");
		feed = _gdata_feed_new_from_json (klass->feed_type, message->response_body->data, message->response_body->length, entry_type,
		                                  progress_callback, progress_user_data, error);
	} else {
		/* Potentially XML. Don't bother checking the Content-Type, since the parser
		 * will fail gracefully if the response body is not valid XML. */
		g_debug("XML content type detected.");
		feed = _gdata_feed_new_from_xml (klass->feed_type, message->response_body->data, message->response_body->length, entry_type,
		                                 progress_callback, progress_user_data, error);
	}

	/* Update the query with the feed's ETag */
	if (query != NULL && feed != NULL && gdata_feed_get_etag (feed) != NULL)
		gdata_query_set_etag (query, gdata_feed_get_etag (feed));

	/* Update the query with the next and previous URIs from the feed */
	if (query != NULL && feed != NULL) {
		GDataLink *_link;
		const gchar *token;

		_gdata_query_clear_pagination (query);

		/* Atom-style next and previous page links. */
		_link = gdata_feed_look_up_link (feed, "http://www.iana.org/assignments/relation/next");
		if (_link != NULL)
			_gdata_query_set_next_uri (query, gdata_link_get_uri (_link));
		_link = gdata_feed_look_up_link (feed, "http://www.iana.org/assignments/relation/previous");
		if (_link != NULL)
			_gdata_query_set_previous_uri (query, gdata_link_get_uri (_link));

		/* JSON-style next page token. (There is no previous page
		 * token.) */
		token = gdata_feed_get_next_page_token (feed);
		if (token != NULL)
			_gdata_query_set_next_page_token (query, token);
	}

	return feed;
}

/**
 * gdata_service_query:
 * @self: a #GDataService
 * @domain: (allow-none): the #GDataAuthorizationDomain the query falls under, or %NULL
 * @feed_uri: the feed URI to query, including the host name and protocol
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @entry_type: a #GType for the #GDataEntrys to build from the XML
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @progress_callback: (allow-none) (scope call) (closure progress_user_data): a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: (closure): data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service's @feed_uri feed to build a #GDataFeed.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled before or during network activity, the error %G_IO_ERROR_CANCELLED will be returned. Cancellation has no effect
 * after network activity has finished, however, and the query will return successfully (or return an error sent by the server) if it is first
 * cancelled after network activity has finished. See the <link linkend="cancellable-support">overview of cancellation</link> for
 * more details.
 *
 * A %GDATA_SERVICE_ERROR_PROTOCOL_ERROR will be returned if the server indicates there is a problem with the query, but subclasses may override
 * this and return their own errors. See their documentation for more details.
 *
 * For each entry in the response feed, @progress_callback will be called in the main thread. If there was an error parsing the XML response,
 * a #GDataParserError will be returned.
 *
 * If the query is successful and the feed supports pagination, @query will be updated with the pagination URIs, and the next or previous page
 * can then be loaded by calling gdata_query_next_page() or gdata_query_previous_page() before running the query again.
 *
 * If the #GDataQuery's ETag is set and it finds a match on the server, %NULL will be returned, but @error will remain unset. Otherwise,
 * @query's ETag will be updated with the ETag from the returned feed, if available.
 *
 * Return value: (transfer full): a #GDataFeed of query results, or %NULL; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GDataFeed *
gdata_service_query (GDataService *self, GDataAuthorizationDomain *domain, const gchar *feed_uri, GDataQuery *query, GType entry_type,
                     GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain), NULL);
	g_return_val_if_fail (feed_uri != NULL, NULL);
	g_return_val_if_fail (g_type_is_a (entry_type, GDATA_TYPE_ENTRY), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	return __gdata_service_query (self, domain, feed_uri, query, entry_type, cancellable, progress_callback, progress_user_data, error);
}

/**
 * gdata_service_query_single_entry:
 * @self: a #GDataService
 * @domain: (allow-none): the #GDataAuthorizationDomain the query falls under, or %NULL
 * @entry_id: the entry ID of the desired entry
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @entry_type: a #GType for the #GDataEntry to build from the XML
 * @cancellable: (allow-none): a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Retrieves information about the single entry with the given @entry_id. @entry_id should be as returned by
 * gdata_entry_get_id().
 *
 * Parameters and errors are as for gdata_service_query(). Most of the properties of @query aren't relevant, and
 * will cause a server-side error if used. The most useful property to use is #GDataQuery:etag, which will cause the
 * server to not return anything if the entry hasn't been modified since it was given the specified ETag; thus saving
 * bandwidth. If the server does not return anything for this reason, gdata_service_query_single_entry() will return
 * %NULL, but will not set an error in @error.
 *
 * Return value: (transfer full): a #GDataEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GDataEntry *
gdata_service_query_single_entry (GDataService *self, GDataAuthorizationDomain *domain, const gchar *entry_id, GDataQuery *query, GType entry_type,
                                  GCancellable *cancellable, GError **error)
{
	GDataEntryClass *klass;
	GDataEntry *entry;
	gchar *entry_uri;
	SoupMessage *message;
	SoupMessageHeaders *headers;
	const gchar *content_type;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain), NULL);
	g_return_val_if_fail (entry_id != NULL, NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (g_type_is_a (entry_type, GDATA_TYPE_ENTRY) == TRUE, NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Query for just the specified entry */
	klass = GDATA_ENTRY_CLASS (g_type_class_ref (entry_type));
	g_assert (klass->get_entry_uri != NULL);

	entry_uri = klass->get_entry_uri (entry_id);
	message = _gdata_service_query (GDATA_SERVICE (self), domain, entry_uri, query, cancellable, error);
	g_free (entry_uri);

	if (message == NULL) {
		g_type_class_unref (klass);
		return NULL;
	}

	g_assert (message->response_body->data != NULL);

	headers = message->response_headers;
	content_type = soup_message_headers_get_content_type (headers, NULL);

	if (g_strcmp0 (content_type, "application/json") == 0) {
		entry = GDATA_ENTRY (gdata_parsable_new_from_json (entry_type, message->response_body->data, message->response_body->length, error));
	} else {
		entry = GDATA_ENTRY (gdata_parsable_new_from_xml (entry_type, message->response_body->data, message->response_body->length, error));
	}

	g_object_unref (message);
	g_type_class_unref (klass);

	return entry;
}

typedef struct {
	GDataAuthorizationDomain *domain;
	gchar *entry_id;
	GDataQuery *query;
	GType entry_type;
} QuerySingleEntryAsyncData;

static void
query_single_entry_async_data_free (QuerySingleEntryAsyncData *data)
{
	if (data->domain != NULL)
		g_object_unref (data->domain);

	g_free (data->entry_id);
	if (data->query != NULL)
		g_object_unref (data->query);
	g_slice_free (QuerySingleEntryAsyncData, data);
}

static void
query_single_entry_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataService *service = GDATA_SERVICE (source_object);
	g_autoptr(GDataEntry) entry = NULL;
	g_autoptr(GError) error = NULL;
	QuerySingleEntryAsyncData *data = task_data;

	/* Execute the query and return */
	entry = gdata_service_query_single_entry (service, data->domain, data->entry_id, data->query, data->entry_type, cancellable, &error);
	if (entry == NULL && error != NULL)
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_pointer (task, g_steal_pointer (&entry), g_object_unref);
}

/**
 * gdata_service_query_single_entry_async:
 * @self: a #GDataService
 * @domain: (allow-none): the #GDataAuthorizationDomain the query falls under, or %NULL
 * @entry_id: the entry ID of the desired entry
 * @query: (allow-none): a #GDataQuery with the query parameters, or %NULL
 * @entry_type: a #GType for the #GDataEntry to build from the XML
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the query is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Retrieves information about the single entry with the given @entry_id. @entry_id should be as returned by
 * gdata_entry_get_id(). @self, @query and @entry_id are reffed/copied when this
 * function is called, so can safely be freed after this function returns.
 *
 * For more details, see gdata_service_query_single_entry(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_query_single_entry_finish()
 * to get the results of the operation.
 *
 * Since: 0.9.0
 */
void
gdata_service_query_single_entry_async (GDataService *self, GDataAuthorizationDomain *domain, const gchar *entry_id, GDataQuery *query,
                                        GType entry_type, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;
	QuerySingleEntryAsyncData *data;

	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_return_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain));
	g_return_if_fail (entry_id != NULL);
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (g_type_is_a (entry_type, GDATA_TYPE_ENTRY) == TRUE);
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	data = g_slice_new (QuerySingleEntryAsyncData);
	data->domain = (domain != NULL) ? g_object_ref (domain) : NULL;
	data->query = (query != NULL) ? g_object_ref (query) : NULL;
	data->entry_id = g_strdup (entry_id);
	data->entry_type = entry_type;

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_service_query_single_entry_async);
	g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) query_single_entry_async_data_free);
	g_task_run_in_thread (task, query_single_entry_thread);
}

/**
 * gdata_service_query_single_entry_finish:
 * @self: a #GDataService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous query operation for a single entry, as started with gdata_service_query_single_entry_async().
 *
 * Return value: (transfer full): a #GDataEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.7.0
 */
GDataEntry *
gdata_service_query_single_entry_finish (GDataService *self, GAsyncResult *async_result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	g_return_val_if_fail (g_task_is_valid (async_result, self), NULL);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_service_query_single_entry_async), NULL);

	return g_task_propagate_pointer (G_TASK (async_result), error);
}

typedef struct {
	GDataAuthorizationDomain *domain;
	gchar *upload_uri;
	GDataEntry *entry;
} InsertEntryAsyncData;

static void
insert_entry_async_data_free (InsertEntryAsyncData *self)
{
	if (self->domain != NULL)
		g_object_unref (self->domain);

	g_free (self->upload_uri);
	if (self->entry)
		g_object_unref (self->entry);

	g_slice_free (InsertEntryAsyncData, self);
}

static void
insert_entry_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataService *service = GDATA_SERVICE (source_object);
	g_autoptr(GDataEntry) updated_entry = NULL;
	g_autoptr(GError) error = NULL;
	InsertEntryAsyncData *data = task_data;

	/* Insert the entry and return */
	updated_entry = gdata_service_insert_entry (service, data->domain, data->upload_uri, data->entry, cancellable, &error);
	if (updated_entry == NULL)
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_pointer (task, g_steal_pointer (&updated_entry), g_object_unref);
}

/**
 * gdata_service_insert_entry_async:
 * @self: a #GDataService
 * @domain: (allow-none): the #GDataAuthorizationDomain the insertion operation falls under, or %NULL
 * @upload_uri: the URI to which the upload should be sent
 * @entry: the #GDataEntry to insert
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when insertion is finished, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Inserts @entry by uploading it to the online service at @upload_uri. @self, @upload_uri and
 * @entry are all reffed/copied when this function is called, so can safely be freed after this function returns.
 *
 * For more details, see gdata_service_insert_entry(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_insert_entry_finish()
 * to get the results of the operation.
 *
 * Since: 0.9.0
 */
void
gdata_service_insert_entry_async (GDataService *self, GDataAuthorizationDomain *domain, const gchar *upload_uri, GDataEntry *entry,
                                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;
	InsertEntryAsyncData *data;

	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_return_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain));
	g_return_if_fail (upload_uri != NULL);
	g_return_if_fail (GDATA_IS_ENTRY (entry));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	data = g_slice_new (InsertEntryAsyncData);
	data->domain = (domain != NULL) ? g_object_ref (domain) : NULL;
	data->upload_uri = g_strdup (upload_uri);
	data->entry = g_object_ref (entry);

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_service_insert_entry_async);
	g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) insert_entry_async_data_free);
	g_task_run_in_thread (task, insert_entry_thread);
}

/**
 * gdata_service_insert_entry_finish:
 * @self: a #GDataService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous entry insertion operation started with gdata_service_insert_entry_async().
 *
 * Return value: (transfer full): an updated #GDataEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.3.0
 */
GDataEntry *
gdata_service_insert_entry_finish (GDataService *self, GAsyncResult *async_result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	g_return_val_if_fail (g_task_is_valid (async_result, self), NULL);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_service_insert_entry_async), NULL);

	return g_task_propagate_pointer (G_TASK (async_result), error);
}

/**
 * gdata_service_insert_entry:
 * @self: a #GDataService
 * @domain: (allow-none): the #GDataAuthorizationDomain the insertion operation falls under, or %NULL
 * @upload_uri: the URI to which the upload should be sent
 * @entry: the #GDataEntry to insert
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Inserts @entry by uploading it to the online service at @upload_uri. For more information about the concept of inserting entries, see
 * the <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/basics.html#InsertingEntry">online documentation</ulink> for the GData
 * protocol.
 *
 * The service will return an updated version of the entry, which is the return value of this function on success.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled before or during network activity, the error %G_IO_ERROR_CANCELLED will be returned. Cancellation has no effect
 * after network activity has finished, however, and the insertion will return successfully (or return an error sent by the server) if it is first
 * cancelled after network activity has finished. See the <link linkend="cancellable-support">overview of cancellation</link> for
 * more details.
 *
 * If the entry is marked as already having been inserted a %GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED error will be returned immediately
 * (there will be no network requests).
 *
 * If there is an error inserting the entry, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned. Currently, subclasses
 * <emphasis>cannot</emphasis> cannot override this or provide more specific errors.
 *
 * Return value: (transfer full): an updated #GDataEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GDataEntry *
gdata_service_insert_entry (GDataService *self, GDataAuthorizationDomain *domain, const gchar *upload_uri, GDataEntry *entry,
                            GCancellable *cancellable, GError **error)
{
	GDataEntry *updated_entry;
	SoupMessage *message;
	gchar *upload_data;
	guint status;
	GDataParsableClass *klass;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain), NULL);
	g_return_val_if_fail (upload_uri != NULL, NULL);
	g_return_val_if_fail (GDATA_IS_ENTRY (entry), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	if (gdata_entry_is_inserted (entry) == TRUE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED,
		                     _("The entry has already been inserted."));
		return NULL;
	}

	message = _gdata_service_build_message (self, domain, SOUP_METHOD_POST, upload_uri, NULL, FALSE);

	/* Append the data */
	klass = GDATA_PARSABLE_GET_CLASS (entry);
	g_assert (klass->get_content_type != NULL);
	if (g_strcmp0 (klass->get_content_type (), "application/json") == 0) {
		upload_data = gdata_parsable_get_json (GDATA_PARSABLE (entry));
		soup_message_set_request (message, "application/json", SOUP_MEMORY_TAKE, upload_data, strlen (upload_data));
	} else {
		upload_data = gdata_parsable_get_xml (GDATA_PARSABLE (entry));
		soup_message_set_request (message, "application/atom+xml", SOUP_MEMORY_TAKE, upload_data, strlen (upload_data));
	}

	/* Send the message */
	status = _gdata_service_send_message (self, message, cancellable, error);

	if (status == SOUP_STATUS_NONE || status == SOUP_STATUS_CANCELLED) {
		/* Redirect error or cancelled */
		g_object_unref (message);
		return NULL;
	} else if (status != SOUP_STATUS_CREATED && status != SOUP_STATUS_OK) {
		/* Error: for XML APIs Google returns CREATED and for JSON it returns OK. */
		GDataServiceClass *service_klass = GDATA_SERVICE_GET_CLASS (self);
		g_assert (service_klass->parse_error_response != NULL);
		service_klass->parse_error_response (self, GDATA_OPERATION_INSERTION, status, message->reason_phrase, message->response_body->data,
		                                     message->response_body->length, error);
		g_object_unref (message);
		return NULL;
	}

	/* Parse the XML or JSON according to GDataEntry type; create and return a new GDataEntry of the same type as @entry */
	g_assert (message->response_body->data != NULL);
	if (g_strcmp0 (klass->get_content_type (), "application/json") == 0) {
		updated_entry = GDATA_ENTRY (gdata_parsable_new_from_json (G_OBJECT_TYPE (entry), message->response_body->data,
		                             message->response_body->length, error));
	} else {
		updated_entry = GDATA_ENTRY (gdata_parsable_new_from_xml (G_OBJECT_TYPE (entry), message->response_body->data,
		                             message->response_body->length, error));
	}
	g_object_unref (message);

	return updated_entry;
}

typedef struct {
	GDataAuthorizationDomain *domain;
	GDataEntry *entry;
} UpdateEntryAsyncData;

static void
update_entry_async_data_free (UpdateEntryAsyncData *data)
{
	if (data->domain != NULL)
		g_object_unref (data->domain);

	if (data->entry != NULL)
		g_object_unref (data->entry);

	g_slice_free (UpdateEntryAsyncData, data);
}

static void
update_entry_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataService *service = GDATA_SERVICE (source_object);
	g_autoptr(GDataEntry) updated_entry = NULL;
	g_autoptr(GError) error = NULL;
	UpdateEntryAsyncData *data = task_data;

	/* Update the entry and return */
	updated_entry = gdata_service_update_entry (service, data->domain, data->entry, cancellable, &error);
	if (updated_entry == NULL)
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_pointer (task, g_steal_pointer (&updated_entry), g_object_unref);
}

/**
 * gdata_service_update_entry_async:
 * @self: a #GDataService
 * @domain: (allow-none): the #GDataAuthorizationDomain the update operation falls under, or %NULL
 * @entry: the #GDataEntry to update
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the update is finished, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Updates @entry by PUTting it to its <literal>edit</literal> link's URI. @self and
 * @entry are both reffed when this function is called, so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_service_update_entry(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_update_entry_finish()
 * to get the results of the operation.
 *
 * Since: 0.9.0
 */
void
gdata_service_update_entry_async (GDataService *self, GDataAuthorizationDomain *domain, GDataEntry *entry,
                                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;
	UpdateEntryAsyncData *data;

	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_return_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain));
	g_return_if_fail (GDATA_IS_ENTRY (entry));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	data = g_slice_new (UpdateEntryAsyncData);
	data->domain = (domain != NULL) ? g_object_ref (domain) : NULL;
	data->entry = g_object_ref (entry);

	task = g_task_new (task, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_service_update_entry_async);
	g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) update_entry_async_data_free);
	g_task_run_in_thread (task, update_entry_thread);
}

/**
 * gdata_service_update_entry_finish:
 * @self: a #GDataService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous entry update operation started with gdata_service_update_entry_async().
 *
 * Return value: (transfer full): an updated #GDataEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.3.0
 */
GDataEntry *
gdata_service_update_entry_finish (GDataService *self, GAsyncResult *async_result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);
	g_return_val_if_fail (g_task_is_valid (async_result, self), NULL);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_service_update_entry_async), NULL);

	return g_task_propagate_pointer (G_TASK (async_result), error);
}

/**
 * gdata_service_update_entry:
 * @self: a #GDataService
 * @domain: (allow-none): the #GDataAuthorizationDomain the update operation falls under, or %NULL
 * @entry: the #GDataEntry to update
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Updates @entry by PUTting it to its <literal>edit</literal> link's URI. For more information about the concept of updating entries, see
 * the <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/basics.html#UpdatingEntry">online documentation</ulink> for the GData
 * protocol.
 *
 * The service will return an updated version of the entry, which is the return value of this function on success.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled before or during network activity, the error %G_IO_ERROR_CANCELLED will be returned. Cancellation has no effect
 * after network activity has finished, however, and the update will return successfully (or return an error sent by the server) if it is first
 * cancelled after network activity has finished. See the <link linkend="cancellable-support">overview of cancellation</link> for
 * more details.
 *
 * If there is an error updating the entry, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned. Currently, subclasses
 * <emphasis>cannot</emphasis> cannot override this or provide more specific errors.
 *
 * Return value: (transfer full): an updated #GDataEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GDataEntry *
gdata_service_update_entry (GDataService *self, GDataAuthorizationDomain *domain, GDataEntry *entry, GCancellable *cancellable, GError **error)
{
	GDataEntry *updated_entry;
	GDataLink *_link;
	SoupMessage *message;
	gchar *upload_data;
	guint status;
	GDataParsableClass *klass;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain), NULL);
	g_return_val_if_fail (GDATA_IS_ENTRY (entry), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	/* Append the data */
	klass = GDATA_PARSABLE_GET_CLASS (entry);
	g_assert (klass->get_content_type != NULL);
	if (g_strcmp0 (klass->get_content_type (), "application/json") == 0) {
		/* Get the edit URI */
		_link = gdata_entry_look_up_link (entry, GDATA_LINK_SELF);
		g_assert (_link != NULL);
		message = _gdata_service_build_message (self, domain, SOUP_METHOD_PUT, gdata_link_get_uri (_link), gdata_entry_get_etag (entry), TRUE);
		upload_data = gdata_parsable_get_json (GDATA_PARSABLE (entry));
		soup_message_set_request (message, "application/json", SOUP_MEMORY_TAKE, upload_data, strlen (upload_data));
	} else {
		/* Get the edit URI */
		_link = gdata_entry_look_up_link (entry, GDATA_LINK_EDIT);
		g_assert (_link != NULL);
		message = _gdata_service_build_message (self, domain, SOUP_METHOD_PUT, gdata_link_get_uri (_link), gdata_entry_get_etag (entry), TRUE);
		upload_data = gdata_parsable_get_xml (GDATA_PARSABLE (entry));
		soup_message_set_request (message, "application/atom+xml", SOUP_MEMORY_TAKE, upload_data, strlen (upload_data));
	}

	/* Send the message */
	status = _gdata_service_send_message (self, message, cancellable, error);

	if (status == SOUP_STATUS_NONE || status == SOUP_STATUS_CANCELLED) {
		/* Redirect error or cancelled */
		g_object_unref (message);
		return NULL;
	} else if (status != SOUP_STATUS_OK) {
		/* Error */
		GDataServiceClass *service_klass = GDATA_SERVICE_GET_CLASS (self);
		g_assert (service_klass->parse_error_response != NULL);
		service_klass->parse_error_response (self, GDATA_OPERATION_UPDATE, status, message->reason_phrase, message->response_body->data,
		                                     message->response_body->length, error);
		g_object_unref (message);
		return NULL;
	}

	/* Parse the XML; create and return a new GDataEntry of the same type as @entry */
	if (g_strcmp0 (klass->get_content_type (), "application/json") == 0) {
		updated_entry = GDATA_ENTRY (gdata_parsable_new_from_json (G_OBJECT_TYPE (entry), message->response_body->data,
		                         message->response_body->length, error));
	} else {
		updated_entry = GDATA_ENTRY (gdata_parsable_new_from_xml (G_OBJECT_TYPE (entry), message->response_body->data,
		                             message->response_body->length, error));
	}
	g_object_unref (message);

	return updated_entry;
}

typedef struct {
	GDataAuthorizationDomain *domain;
	GDataEntry *entry;
} DeleteEntryAsyncData;

static void
delete_entry_async_data_free (DeleteEntryAsyncData *data)
{
	if (data->domain != NULL)
		g_object_unref (data->domain);

	if (data->entry != NULL)
		g_object_unref (data->entry);

	g_slice_free (DeleteEntryAsyncData, data);
}

static void
delete_entry_thread (GTask *task, gpointer source_object, gpointer task_data, GCancellable *cancellable)
{
	GDataService *service = GDATA_SERVICE (source_object);
	g_autoptr(GError) error = NULL;
	DeleteEntryAsyncData *data = task_data;

	/* Delete the entry and return */
	if (!gdata_service_delete_entry (service, data->domain, data->entry, cancellable, &error))
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_boolean (task, TRUE);
}

/**
 * gdata_service_delete_entry_async:
 * @self: a #GDataService
 * @domain: (allow-none): the #GDataAuthorizationDomain the deletion falls under, or %NULL
 * @entry: the #GDataEntry to delete
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when deletion is finished, or %NULL
 * @user_data: (closure): data to pass to the @callback function
 *
 * Deletes @entry from the server. @self and @entry are both reffed when this function is called,
 * so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_service_delete_entry(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_delete_entry_finish()
 * to get the results of the operation.
 *
 * Since: 0.9.0
 */
void
gdata_service_delete_entry_async (GDataService *self, GDataAuthorizationDomain *domain, GDataEntry *entry,
                                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	g_autoptr(GTask) task = NULL;
	DeleteEntryAsyncData *data;

	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_return_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain));
	g_return_if_fail (GDATA_IS_ENTRY (entry));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	data = g_slice_new (DeleteEntryAsyncData);
	data->domain = (domain != NULL) ? g_object_ref (domain) : NULL;
	data->entry = g_object_ref (entry);

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_service_delete_entry_async);
	g_task_set_task_data (task, g_steal_pointer (&data), (GDestroyNotify) delete_entry_async_data_free);
	g_task_run_in_thread (task, delete_entry_thread);
}

/**
 * gdata_service_delete_entry_finish:
 * @self: a #GDataService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous entry deletion operation started with gdata_service_delete_entry_async().
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: 0.3.0
 */
gboolean
gdata_service_delete_entry_finish (GDataService *self, GAsyncResult *async_result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	g_return_val_if_fail (g_task_is_valid (async_result, self), FALSE);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_service_delete_entry_async), FALSE);

	return g_task_propagate_boolean (G_TASK (async_result), error);
}

/**
 * gdata_service_delete_entry:
 * @self: a #GDataService
 * @domain: (allow-none): the #GDataAuthorizationDomain the deletion falls under, or %NULL
 * @entry: the #GDataEntry to delete
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Deletes @entry from the server. For more information about the concept of deleting entries, see the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/basics.html#DeletingEntry">online documentation</ulink> for the GData
 * protocol.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled before or during network activity, the error %G_IO_ERROR_CANCELLED will be returned. Cancellation has no effect
 * after network activity has finished, however, and the deletion will return successfully (or return an error sent by the server) if it is first
 * cancelled after network activity has finished. See the <link linkend="cancellable-support">overview of cancellation</link> for
 * more details.
 *
 * If there is an error deleting the entry, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned. Currently, subclasses
 * <emphasis>cannot</emphasis> cannot override this or provide more specific errors.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: 0.9.0
 */
gboolean
gdata_service_delete_entry (GDataService *self, GDataAuthorizationDomain *domain, GDataEntry *entry, GCancellable *cancellable, GError **error)
{
	GDataLink *_link;
	SoupMessage *message;
	guint status;
	gchar *fixed_uri;
	GDataParsableClass *klass;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), FALSE);
	g_return_val_if_fail (domain == NULL || GDATA_IS_AUTHORIZATION_DOMAIN (domain), FALSE);
	g_return_val_if_fail (GDATA_IS_ENTRY (entry), FALSE);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	/* Get the edit URI. We have to fix it to always use HTTPS as YouTube videos appear to incorrectly return a HTTP URI as their edit URI. */
	klass = GDATA_PARSABLE_GET_CLASS (entry);
	g_assert (klass->get_content_type != NULL);
	if (g_strcmp0 (klass->get_content_type (), "application/json") == 0) {
		_link = gdata_entry_look_up_link (entry, GDATA_LINK_SELF);
	} else {
		_link = gdata_entry_look_up_link (entry, GDATA_LINK_EDIT);
	}
	g_assert (_link != NULL);

	fixed_uri = _gdata_service_fix_uri_scheme (gdata_link_get_uri (_link));
	message = _gdata_service_build_message (self, domain, SOUP_METHOD_DELETE, fixed_uri, gdata_entry_get_etag (entry), TRUE);
	g_free (fixed_uri);

	/* Send the message */
	status = _gdata_service_send_message (self, message, cancellable, error);

	if (status == SOUP_STATUS_NONE || status == SOUP_STATUS_CANCELLED) {
		/* Redirect error or cancelled */
		g_object_unref (message);
		return FALSE;
	} else if (status != SOUP_STATUS_OK && status != SOUP_STATUS_NO_CONTENT) {
		/* Error */
		GDataServiceClass *service_klass = GDATA_SERVICE_GET_CLASS (self);
		g_assert (service_klass->parse_error_response != NULL);
		service_klass->parse_error_response (self, GDATA_OPERATION_DELETION, status, message->reason_phrase, message->response_body->data,
		                                     message->response_body->length, error);
		g_object_unref (message);
		return FALSE;
	}

	g_object_unref (message);

	return TRUE;
}

/**
 * gdata_service_get_proxy_resolver:
 * @self: a #GDataService
 *
 * Gets the #GProxyResolver on the #GDataService's #SoupSession.
 *
 * Return value: (transfer none) (allow-none): a #GProxyResolver, or %NULL
 *
 * Since: 0.15.0
 */
GProxyResolver *
gdata_service_get_proxy_resolver (GDataService *self)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);

	return self->priv->proxy_resolver;
}

/**
 * gdata_service_set_proxy_resolver:
 * @self: a #GDataService
 * @proxy_resolver: (allow-none): a #GProxyResolver, or %NULL
 *
 * Sets the #GProxyResolver on the #SoupSession used internally by the given #GDataService.
 *
 * Since: 0.15.0
 */
void
gdata_service_set_proxy_resolver (GDataService *self, GProxyResolver *proxy_resolver)
{
	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_return_if_fail (proxy_resolver == NULL || G_IS_PROXY_RESOLVER (proxy_resolver));

	if (proxy_resolver != NULL) {
		g_object_ref (proxy_resolver);
	}

	g_clear_object (&self->priv->proxy_resolver);
	self->priv->proxy_resolver = proxy_resolver;

	g_object_notify (G_OBJECT (self), "proxy-resolver");
}

static void
notify_timeout_cb (GObject *gobject, GParamSpec *pspec, GObject *self)
{
	g_object_notify (self, "timeout");
}

/**
 * gdata_service_get_timeout:
 * @self: a #GDataService
 *
 * Gets the #GDataService:timeout property; the network timeout, in seconds.
 *
 * Return value: the timeout, or <code class="literal">0</code>
 *
 * Since: 0.7.0
 */
guint
gdata_service_get_timeout (GDataService *self)
{
	guint timeout;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), 0);

	g_object_get (self->priv->session, SOUP_SESSION_TIMEOUT, &timeout, NULL);

	return timeout;
}

/**
 * gdata_service_set_timeout:
 * @self: a #GDataService
 * @timeout: the timeout, or <code class="literal">0</code>
 *
 * Sets the #GDataService:timeout property; the network timeout, in seconds.
 *
 * If @timeout is <code class="literal">0</code>, network operations will never time out.
 *
 * Note that if a #GDataAuthorizer is being used with this #GDataService, the authorizer might also need its timeout setting.
 *
 * Since: 0.7.0
 */
void
gdata_service_set_timeout (GDataService *self, guint timeout)
{
	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_object_set (self->priv->session, SOUP_SESSION_TIMEOUT, timeout, NULL);
	g_object_notify (G_OBJECT (self), "timeout");
}

SoupSession *
_gdata_service_get_session (GDataService *self)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	return self->priv->session;
}

/*
 * _gdata_service_get_scheme:
 *
 * Returns the name of the scheme to use, which will always be <code class="literal">https</code>. The return type used to vary according to the
 * environment variable <code class="literal">LIBGDATA_FORCE_HTTP</code>, but Google has since switched to using HTTPS exclusively.
 *
 * See <ulink type="http" url="http://googlecode.blogspot.com/2011/03/improving-security-of-google-apis-with.html">Improving the security of Google
 * APIs with SSL</ulink>.
 *
 * Return value: the scheme to use (<code class="literal">https</code>)
 *
 * Since: 0.6.0
 */
const gchar *
_gdata_service_get_scheme (void)
{
	return "https";
}

/*
 * _gdata_service_build_uri:
 * @format: a standard printf() format string
 * @...: the arguments to insert in the output
 *
 * Builds a URI from the given @format string, replacing each <code class="literal">%%s</code> format placeholder with a URI-escaped version of the
 * corresponding argument, and each <code class="literal">%%p</code> format placeholder with a non-escaped version of the corresponding argument. No
 * other printf() format placeholders are supported at the moment except <code class="literal">%%d</code>, which prints a signed integer; and
 * <code class="literal">%%</code>, which prints a literal percent symbol.
 *
 * The returned URI is guaranteed to use the scheme returned by _gdata_service_get_scheme(). The format string, once all the arguments have been
 * inserted into it, must include a scheme, but it doesn't matter which one.
 *
 * Return value: a newly allocated URI string; free with g_free()
 */
gchar *
_gdata_service_build_uri (const gchar *format, ...)
{
	const gchar *p;
	gchar *fixed_uri;
	GString *uri;
	va_list args;

	g_return_val_if_fail (format != NULL, NULL);

	/* Allocate a GString to build the URI in with at least as much space as the format string */
	uri = g_string_sized_new (strlen (format));

	/* Build the URI */
	va_start (args, format);

	for (p = format; *p != '\0'; p++) {
		if (*p != '%') {
			g_string_append_c (uri, *p);
			continue;
		}

		switch(*++p) {
			case 's':
				g_string_append_uri_escaped (uri, va_arg (args, gchar*), NULL, TRUE);
				break;
			case 'p':
				g_string_append (uri, va_arg (args, gchar*));
				break;
			case 'd':
				g_string_append_printf (uri, "%d", va_arg (args, gint));
				break;
			case '%':
				g_string_append_c (uri, '%');
				break;
			default:
				g_error ("Unrecognized format placeholder '%%%c' in format '%s'. This is a programmer error.", *p, format);
				break;
		}
	}

	va_end (args);

	/* Fix the scheme to always be HTTPS */
	fixed_uri = _gdata_service_fix_uri_scheme (uri->str);
	g_string_free (uri, TRUE);

	return fixed_uri;
}

/**
 * _gdata_service_fix_uri_scheme:
 * @uri: an URI with either HTTP or HTTPS as the scheme
 *
 * Fixes the given URI to always have HTTPS as its scheme.
 *
 * Return value: (transfer full): the URI with HTTPS as its scheme
 *
 * Since: 0.9.0
 */
gchar *
_gdata_service_fix_uri_scheme (const gchar *uri)
{
	g_return_val_if_fail (uri != NULL && *uri != '\0', NULL);

	/* Ensure we're using the correct scheme (HTTP or HTTPS) */
	if (g_str_has_prefix (uri, "https") == FALSE) {
		gchar *fixed_uri, **pieces;

		pieces = g_strsplit (uri, ":", 2);
		g_assert (pieces[0] != NULL && pieces[1] != NULL && pieces[2] == NULL);

		fixed_uri = g_strconcat ("https:", pieces[1], NULL);

		g_strfreev (pieces);

		return fixed_uri;
	}

	return g_strdup (uri);
}

/**
 * _gdata_service_get_https_port:
 *
 * Gets the destination TCP/IP port number which libgdata should use for all outbound HTTPS traffic.
 * This defaults to 443, but may be overridden using the <code class="literal">LIBGDATA_HTTPS_PORT</code>
 * environment variable. This is intended to allow network traffic to be redirected to a local server for
 * unit testing, with a listening port above 1024 so the tests don't need root privileges.
 *
 * The value returned by this function may change at any time (e.g. between unit tests), so callers must not cache the result.
 *
 * Return value: port number to use for HTTPS traffic
 */
guint
_gdata_service_get_https_port (void)
{
	const gchar *port_string;

	/* Allow changing the HTTPS port just for testing. */
	port_string = g_getenv ("LIBGDATA_HTTPS_PORT");
	if (port_string != NULL) {
		const gchar *end;

		guint64 port = g_ascii_strtoull (port_string, (gchar **) &end, 10);

		if (port != 0 && *end == '\0') {
			g_debug ("Overriding message port to %" G_GUINT64_FORMAT ".", port);
			return port;
		}
	}

	/* Return the default. */
	return 443;
}

/*
 * debug_handler:
 *
 * GLib debug message handler, which is passed all messages from g_debug() calls, and decides whether to print them.
 */
static void
debug_handler (const char *log_domain, GLogLevelFlags log_level, const char *message, gpointer user_data)
{
	if (_gdata_service_get_log_level () != GDATA_LOG_NONE)
		g_log_default_handler (log_domain, log_level, message, NULL);
}

/*
 * soup_log_printer:
 *
 * Log printer for the libsoup logging functionality, which just marshals all soup log output to the standard GLib logging framework
 * (and thus to debug_handler(), above).
 */
static void
soup_log_printer (SoupLogger *logger, SoupLoggerLogLevel level, char direction, const char *data, gpointer user_data)
{
	gboolean filter_data;
	gchar *_data = NULL;

	filter_data = (_gdata_service_get_log_level () > GDATA_LOG_NONE && _gdata_service_get_log_level () < GDATA_LOG_FULL_UNREDACTED) ? TRUE : FALSE;

	if (filter_data == TRUE) {
		/* Filter out lines which look like they might contain usernames, passwords or auth. tokens. */
		if (direction == '>' && g_str_has_prefix (data, "Authorization: GoogleLogin ") == TRUE) {
			_data = g_strdup ("Authorization: GoogleLogin <redacted>");
		} else if (direction == '>' && g_str_has_prefix (data, "Authorization: OAuth ") == TRUE) {
			_data = g_strdup ("Authorization: OAuth <redacted>");
		} else if (direction == '<' && g_str_has_prefix (data, "Set-Cookie: ") == TRUE) {
			_data = g_strdup ("Set-Cookie: <redacted>");
		} else if (direction == '<' && g_str_has_prefix (data, "Location: ") == TRUE) {
			/* Looks like:
			 * "Location: https://www.google.com/calendar/feeds/default/owncalendars/full?gsessionid=sBjmp05m5i67exYA51XjDA". */
			SoupURI *uri;
			gchar *_uri;
			GHashTable *params;

			uri = soup_uri_new (data + strlen ("Location: "));

			if (uri->query != NULL) {
				params = soup_form_decode (uri->query);

				/* strdup()s are necessary because the hash table's set up to free keys. */
				if (g_hash_table_lookup (params, "gsessionid") != NULL) {
					g_hash_table_insert (params, (gpointer) g_strdup ("gsessionid"), (gpointer) "<redacted>");
				}

				soup_uri_set_query_from_form (uri, params);
				g_hash_table_destroy (params);
			}

			_uri = soup_uri_to_string (uri, FALSE);
			_data = g_strconcat ("Location: ", _uri, NULL);
			g_free (_uri);

			soup_uri_free (uri);
		} else if (direction == '<' && g_str_has_prefix (data, "SID=") == TRUE) {
			_data = g_strdup ("SID=<redacted>");
		} else if (direction == '<' && g_str_has_prefix (data, "LSID=") == TRUE) {
			_data = g_strdup ("LSID=<redacted>");
		} else if (direction == '<' && g_str_has_prefix (data, "Auth=") == TRUE) {
			_data = g_strdup ("Auth=<redacted>");
		} else if (direction == '>' && g_str_has_prefix (data, "accountType=") == TRUE) {
			/* Looks like: "> accountType=HOSTED%5FOR%5FGOOGLE&Email=[e-mail address]&Passwd=[plaintex password]"
			               "&service=[service name]&source=ytapi%2DGNOME%2Dlibgdata%2D444fubtt%2D0". */
			GHashTable *params = soup_form_decode (data);

			/* strdup()s are necessary because the hash table's set up to free keys. */
			if (g_hash_table_lookup (params, "Email") != NULL) {
				g_hash_table_insert (params, (gpointer) g_strdup ("Email"), (gpointer) "<redacted>");
			}
			if (g_hash_table_lookup (params, "Passwd") != NULL) {
				g_hash_table_insert (params, (gpointer) g_strdup ("Passwd"), (gpointer) "<redacted>");
			}

			_data = soup_form_encode_hash (params);

			g_hash_table_destroy (params);
		} else if (direction == '<' && g_str_has_prefix (data, "oauth_token=") == TRUE) {
			/* Looks like: "< oauth_token=4%2FI-WU7sBzKk5GhGlQUF8a_TCZRnb7&oauth_token_secret=qTTTJg3no25auiiWFerzjW4I"
			               "&oauth_callback_confirmed=true". */
			GHashTable *params = soup_form_decode (data);

			/* strdup()s are necessary because the hash table's set up to free keys. */
			if (g_hash_table_lookup (params, "oauth_token") != NULL) {
				g_hash_table_insert (params, (gpointer) g_strdup ("oauth_token"), (gpointer) "<redacted>");
			}
			if (g_hash_table_lookup (params, "oauth_token_secret") != NULL) {
				g_hash_table_insert (params, (gpointer) g_strdup ("oauth_token_secret"), (gpointer) "<redacted>");
			}

			_data = soup_form_encode_hash (params);

			g_hash_table_destroy (params);
		} else if (direction == '>' && g_str_has_prefix (data, "X-GData-Key: key=") == TRUE) {
			/* Looks like: "> X-GData-Key: key=[dev key in hex]". */
			_data = g_strdup ("X-GData-Key: key=<redacted>");
		} else {
			/* Nothing to redact. */
			_data = g_strdup (data);
		}
	} else {
		/* Don't dupe the string. */
		_data = (gchar*) data;
	}

	/* Log the data. */
	g_debug ("%c %s", direction, _data);

	if (filter_data == TRUE) {
		g_free (_data);
	}
}

/**
 * _gdata_service_get_log_level:
 *
 * Returns the logging level for the library, currently set by an environment variable.
 *
 * Return value: the log level
 *
 * Since: 0.7.0
 */
GDataLogLevel
_gdata_service_get_log_level (void)
{
	static int level = -1;

	if (level < 0) {
		const gchar *envvar = g_getenv ("LIBGDATA_DEBUG");
		if (envvar != NULL)
			level = atoi (envvar);
		level = MIN (MAX (level, 0), GDATA_LOG_FULL_UNREDACTED);
	}

	return level;
}

/* Build a User-Agent value to send to the server.
 *
 * If we support gzip, we can request gzip from the server by both including
 * the appropriate Accept-Encoding header and putting 'gzip' in the User-Agent
 * header:
 *  - https://developers.google.com/drive/web/performance#gzip
 *  - http://googleappsdeveloper.blogspot.co.uk/2011/12/optimizing-bandwidth-usage-with-gzip.html
 */
static gchar *
build_user_agent (gboolean supports_gzip)
{
	if (supports_gzip) {
		return g_strdup_printf ("libgdata/%s - gzip", VERSION);
	} else {
		return g_strdup_printf ("libgdata/%s", VERSION);
	}
}

/**
 * _gdata_service_build_session:
 *
 * Build a new #SoupSession, enabling GNOME features if support has been compiled for them, and adding a log printer which is hooked into
 * libgdata's logging functionality.
 *
 * Return value: a new #SoupSession; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
SoupSession *
_gdata_service_build_session (void)
{
	SoupSession *session;
	gboolean ssl_strict = TRUE;
	gchar *user_agent;

	/* Iff LIBGDATA_LAX_SSL_CERTIFICATES=1, relax SSL certificate validation to allow using invalid/unsigned certificates for testing. */
	if (g_strcmp0 (g_getenv ("LIBGDATA_LAX_SSL_CERTIFICATES"), "1") == 0) {
		ssl_strict = FALSE;
	}

	session = soup_session_new_with_options ("ssl-strict", ssl_strict,
	                                         "timeout", 0,
	                                         NULL);

	user_agent = build_user_agent (soup_session_has_feature (session, SOUP_TYPE_CONTENT_DECODER));
	g_object_set (session, "user-agent", user_agent, NULL);
	g_free (user_agent);

	soup_session_add_feature_by_type (session, SOUP_TYPE_PROXY_RESOLVER_DEFAULT);

	/* Log all libsoup traffic if debugging's turned on */
	if (_gdata_service_get_log_level () > GDATA_LOG_MESSAGES) {
		SoupLoggerLogLevel level;
		SoupLogger *logger;

		switch (_gdata_service_get_log_level ()) {
			case GDATA_LOG_FULL_UNREDACTED:
			case GDATA_LOG_FULL:
				level = SOUP_LOGGER_LOG_BODY;
				break;
			case GDATA_LOG_HEADERS:
				level = SOUP_LOGGER_LOG_HEADERS;
				break;
			case GDATA_LOG_MESSAGES:
			case GDATA_LOG_NONE:
			default:
				g_assert_not_reached ();
		}

		logger = soup_logger_new (level, -1);
		soup_logger_set_printer (logger, (SoupLoggerPrinter) soup_log_printer, NULL, NULL);

		soup_session_add_feature (session, SOUP_SESSION_FEATURE (logger));

		g_object_unref (logger);
	}

	return session;
}

/**
 * gdata_service_get_locale:
 * @self: a #GDataService
 *
 * Returns the locale currently being used for network requests, or %NULL if the locale is the default.
 *
 * Return value: the current locale
 *
 * Since: 0.7.0
 */
const gchar *
gdata_service_get_locale (GDataService *self)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	return self->priv->locale;
}

/**
 * gdata_service_set_locale:
 * @self: a #GDataService
 * @locale: (allow-none): the new locale in Unix locale format, or %NULL for the default locale
 *
 * Set the locale used for network requests to @locale, given in standard Unix locale format. See #GDataService:locale for more details.
 *
 * Note that while it's possible to change the locale after sending network requests, it is unsupported, as the server-side software may behave
 * unexpectedly. The only supported use of this function is after creation of a service, but before any network requests are made.
 *
 * Since: 0.7.0
 */
void
gdata_service_set_locale (GDataService *self, const gchar *locale)
{
	g_return_if_fail (GDATA_IS_SERVICE (self));

	g_free (self->priv->locale);
	self->priv->locale = g_strdup (locale);
	g_object_notify (G_OBJECT (self), "locale");
}

/*
 * _gdata_service_secure_strdup:
 * @str: string (which may be in pageable memory) to be duplicated, or %NULL
 *
 * Duplicate a string into non-pageable memory (if libgdata has been compiled with HAVE_GNOME) or just fall back to g_strdup() (if libgdata hasn't).
 * Passing %NULL to this function will cause %NULL to be returned.
 *
 * Strings allocated using this function must be freed using _gdata_service_secure_strfree().
 *
 * Return value: non-pageable copy of @str, or %NULL
 * Since: 0.11.0
 */
GDataSecureString
_gdata_service_secure_strdup (const gchar *str)
{
#ifdef HAVE_GNOME
	return gcr_secure_memory_strdup (str);
#else /* if !HAVE_GNOME */
	return g_strdup (str);
#endif /* !HAVE_GNOME */
}

/*
 * _gdata_service_secure_strndup:
 * @str: string (which may be in pageable memory) to be duplicated, or %NULL
 * @n_bytes: maximum number of bytes to copy from @str
 *
 * Duplicate at most @n_bytes bytes from @str into non-pageable memory. See _gdata_service_secure_strdup() for more information; this function is just
 * a version of that with the same semantics as strndup().
 *
 * Return value: non-pageable copy of at most the first @n_bytes bytes of @str, or %NULL
 * Since: 0.11.0
 */
GDataSecureString
_gdata_service_secure_strndup (const gchar *str, gsize n_bytes)
{
#ifdef HAVE_GNOME
	gsize str_len;
	GDataSecureString duped_str;

	if (str == NULL) {
		return NULL;
	}

	str_len = MIN (strlen (str), n_bytes);
	duped_str = (GDataSecureString) gcr_secure_memory_alloc (str_len + 1);
	strncpy (duped_str, str, str_len);
	*(duped_str + str_len) = '\0';

	return duped_str;
#else /* if !HAVE_GNOME */
	return g_strndup (str, n_bytes);
#endif /* !HAVE_GNOME */
}

/*
 * _gdata_service_secure_strfree:
 * @str: a string to free, or %NULL
 *
 * Free a string which was allocated securely using _gdata_service_secure_strdup().
 * Passing %NULL to this function is safe.
 *
 * Since: 0.11.0
 */
void
_gdata_service_secure_strfree (GDataSecureString str)
{
#ifdef HAVE_GNOME
	gcr_secure_memory_free (str);
#else /* if !HAVE_GNOME */
	/* Poor man's approximation to non-pageable memory: the best we can do is ensure that we don't leak it in free memory.
	 * This can't guarantee that it hasn't hit disk at some point, but does mean it can't hit disk in future. */
	if (str != NULL) {
		memset (str, 0, strlen (str));
	}

	g_free (str);
#endif /* !HAVE_GNOME */
}
