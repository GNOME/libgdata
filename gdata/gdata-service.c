/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2008–2010 <philip@tecnocode.co.uk>
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
 * @stability: Unstable
 * @include: gdata/gdata-service.h
 *
 * #GDataService represents a GData API service, typically a website using the GData API, such as YouTube or Google Calendar. One
 * #GDataService instance is required to issue queries to the service, handle insertions, updates and deletions, and generally
 * communicate with the online service.
 **/

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <libsoup/soup.h>
#include <string.h>

#ifdef HAVE_GNOME
#include <libsoup/soup-gnome-features.h>
#endif /* HAVE_GNOME */

#include "gdata-service.h"
#include "gdata-private.h"
#include "gdata-marshal.h"
#include "gdata-types.h"

/* The default e-mail domain to use for usernames */
#define EMAIL_DOMAIN "gmail.com"

GQuark
gdata_service_error_quark (void)
{
	return g_quark_from_static_string ("gdata-service-error-quark");
}

GQuark
gdata_authentication_error_quark (void)
{
	return g_quark_from_static_string ("gdata-authentication-error-quark");
}

static void gdata_service_dispose (GObject *object);
static void gdata_service_finalize (GObject *object);
static void gdata_service_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_service_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static gboolean real_parse_authentication_response (GDataService *self, guint status, const gchar *response_body, gint length, GError **error);
static void real_append_query_headers (GDataService *self, SoupMessage *message);
static void real_parse_error_response (GDataService *self, GDataOperationType operation_type, guint status, const gchar *reason_phrase,
                                       const gchar *response_body, gint length, GError **error);
static void notify_proxy_uri_cb (GObject *gobject, GParamSpec *pspec, GObject *self);
static void notify_timeout_cb (GObject *gobject, GParamSpec *pspec, GObject *self);
static void debug_handler (const char *log_domain, GLogLevelFlags log_level, const char *message, gpointer user_data);
static void soup_log_printer (SoupLogger *logger, SoupLoggerLogLevel level, char direction, const char *data, gpointer user_data);

struct _GDataServicePrivate {
	SoupSession *session;

	gchar *username;
	gchar *password;
	gchar *auth_token;
	gchar *client_id;
	gboolean authenticated;
	gchar *locale;
};

enum {
	PROP_CLIENT_ID = 1,
	PROP_USERNAME,
	PROP_PASSWORD,
	PROP_AUTHENTICATED,
	PROP_PROXY_URI,
	PROP_TIMEOUT,
	PROP_LOCALE
};

enum {
	SIGNAL_CAPTCHA_CHALLENGE,
	LAST_SIGNAL
};

static guint service_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (GDataService, gdata_service, G_TYPE_OBJECT)

static void
gdata_service_class_init (GDataServiceClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataServicePrivate));

	gobject_class->set_property = gdata_service_set_property;
	gobject_class->get_property = gdata_service_get_property;
	gobject_class->dispose = gdata_service_dispose;
	gobject_class->finalize = gdata_service_finalize;

	klass->service_name = "xapi";
	klass->authentication_uri = "https://www.google.com/accounts/ClientLogin";
	klass->api_version = "2";
	klass->feed_type = GDATA_TYPE_FEED;
	klass->parse_authentication_response = real_parse_authentication_response;
	klass->append_query_headers = real_append_query_headers;
	klass->parse_error_response = real_parse_error_response;

	/**
	 * GDataService:client-id:
	 *
	 * A client ID for your application (see the
	 * <ulink url="http://code.google.com/apis/accounts/docs/AuthForInstalledApps.html#Request" type="http">reference documentation</ulink>).
	 *
	 * It is recommended that the ID is of the form <literal><replaceable>company name</replaceable>-<replaceable>application name</replaceable>-
	 * <replaceable>version ID</replaceable></literal>.
	 **/
	g_object_class_install_property (gobject_class, PROP_CLIENT_ID,
	                                 g_param_spec_string ("client-id",
	                                                      "Client ID", "A client ID for your application.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataService:username:
	 *
	 * The user's Google username for authentication. This will always be a full e-mail address.
	 **/
	g_object_class_install_property (gobject_class, PROP_USERNAME,
	                                 g_param_spec_string ("username",
	                                                      "Username", "The user's Google username for authentication.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataService:password:
	 *
	 * The user's account password for authentication.
	 **/
	g_object_class_install_property (gobject_class, PROP_PASSWORD,
	                                 g_param_spec_string ("password",
	                                                      "Password", "The user's account password for authentication.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataService:authenticated:
	 *
	 * Whether the user is authenticated (logged in) with the service.
	 **/
	g_object_class_install_property (gobject_class, PROP_AUTHENTICATED,
	                                 g_param_spec_boolean ("authenticated",
	                                                       "Authenticated", "Whether the user is authenticated (logged in) with the service.",
	                                                       FALSE,
	                                                       G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataService:proxy-uri:
	 *
	 * The proxy URI used internally for all network requests.
	 *
	 * Since: 0.2.0
	 **/
	g_object_class_install_property (gobject_class, PROP_PROXY_URI,
	                                 g_param_spec_boxed ("proxy-uri",
	                                                     "Proxy URI", "The proxy URI used internally for all network requests.",
	                                                     SOUP_TYPE_URI,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataService:timeout:
	 *
	 * A timeout, in seconds, for network operations. If the timeout is exceeded, the operation will be cancelled and
	 * %GDATA_SERVICE_ERROR_NETWORK_ERROR will be returned.
	 *
	 * If the timeout is <code class="literal">0</code>, operations will never time out.
	 *
	 * Since: 0.7.0
	 **/
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
	 **/
	g_object_class_install_property (gobject_class, PROP_LOCALE,
	                                 g_param_spec_string ("locale",
	                                                      "Locale", "The locale to use for network requests, in Unix locale format.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataService::captcha-challenge:
	 * @service: the #GDataService which received the challenge
	 * @uri: the URI of the CAPTCHA image to be used
	 *
	 * The #GDataService::captcha-challenge signal is emitted during the authentication process if
	 * the service requires a CAPTCHA to be completed. The URI of a CAPTCHA image is given, and the
	 * program should display this to the user, and return their response (the text displayed in the
	 * image). There is no timeout imposed by the library for the response.
	 *
	 * Return value: the text in the CAPTCHA image
	 **/
	service_signals[SIGNAL_CAPTCHA_CHALLENGE] = g_signal_new ("captcha-challenge",
	                                                          G_TYPE_FROM_CLASS (klass),
	                                                          G_SIGNAL_RUN_LAST,
	                                                          0, NULL, NULL,
	                                                          gdata_marshal_STRING__OBJECT_STRING,
	                                                          G_TYPE_STRING, 1, G_TYPE_STRING);
}

static void
gdata_service_init (GDataService *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_SERVICE, GDataServicePrivate);
	self->priv->session = soup_session_sync_new ();

#ifdef HAVE_GNOME
	soup_session_add_feature_by_type (self->priv->session, SOUP_TYPE_GNOME_FEATURES_2_26);
#endif /* HAVE_GNOME */

	/* Debug log handling */
	g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG, (GLogFunc) debug_handler, self);

	/* Log all libsoup traffic if debugging's turned on */
	if (_gdata_service_get_log_level () > GDATA_LOG_MESSAGES) {
		SoupLoggerLogLevel level;
		SoupLogger *logger;

		switch (_gdata_service_get_log_level ()) {
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
		soup_logger_set_printer (logger, (SoupLoggerPrinter) soup_log_printer, self, NULL);

		soup_session_add_feature (self->priv->session, SOUP_SESSION_FEATURE (logger));
	}

	/* Proxy the SoupSession's proxy-uri and timeout properties */
	g_signal_connect (self->priv->session, "notify::proxy-uri", (GCallback) notify_proxy_uri_cb, self);
	g_signal_connect (self->priv->session, "notify::timeout", (GCallback) notify_timeout_cb, self);
}

static void
gdata_service_dispose (GObject *object)
{
	GDataServicePrivate *priv = GDATA_SERVICE (object)->priv;

	if (priv->session != NULL)
		g_object_unref (priv->session);
	priv->session = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_service_parent_class)->dispose (object);
}

static void
gdata_service_finalize (GObject *object)
{
	GDataServicePrivate *priv = GDATA_SERVICE (object)->priv;

	g_free (priv->username);
	g_free (priv->password);
	g_free (priv->auth_token);
	g_free (priv->client_id);
	g_free (priv->locale);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_service_parent_class)->finalize (object);
}

static void
gdata_service_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataServicePrivate *priv = GDATA_SERVICE (object)->priv;

	switch (property_id) {
		case PROP_CLIENT_ID:
			g_value_set_string (value, priv->client_id);
			break;
		case PROP_USERNAME:
			g_value_set_string (value, priv->username);
			break;
		case PROP_PASSWORD:
			g_value_set_string (value, priv->password);
			break;
		case PROP_AUTHENTICATED:
			g_value_set_boolean (value, priv->authenticated);
			break;
		case PROP_PROXY_URI:
			g_value_set_boxed (value, gdata_service_get_proxy_uri (GDATA_SERVICE (object)));
			break;
		case PROP_TIMEOUT:
			g_value_set_uint (value, gdata_service_get_timeout (GDATA_SERVICE (object)));
			break;
		case PROP_LOCALE:
			g_value_set_string (value, priv->locale);
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
	GDataServicePrivate *priv = GDATA_SERVICE (object)->priv;

	switch (property_id) {
		case PROP_CLIENT_ID:
			priv->client_id = g_value_dup_string (value);
			break;
		case PROP_PROXY_URI:
			gdata_service_set_proxy_uri (GDATA_SERVICE (object), g_value_get_boxed (value));
			break;
		case PROP_TIMEOUT:
			gdata_service_set_timeout (GDATA_SERVICE (object), g_value_get_uint (value));
			break;
		case PROP_LOCALE:
			gdata_service_set_locale (GDATA_SERVICE (object), g_value_get_string (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static gboolean
real_parse_authentication_response (GDataService *self, guint status, const gchar *response_body, gint length, GError **error)
{
	gchar *auth_start, *auth_end;

	/* Parse the response */
	auth_start = strstr (response_body, "Auth=");
	if (auth_start == NULL)
		goto protocol_error;
	auth_start += strlen ("Auth=");

	auth_end = strstr (auth_start, "\n");
	if (auth_end == NULL)
		goto protocol_error;

	self->priv->auth_token = g_strndup (auth_start, auth_end - auth_start);
	if (self->priv->auth_token == NULL || strlen (self->priv->auth_token) == 0)
		goto protocol_error;

	return TRUE;

protocol_error:
	g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
	                     _("The server returned a malformed response."));
	return FALSE;
}

static void
real_append_query_headers (GDataService *self, SoupMessage *message)
{
	gchar *authorisation_header;

	g_assert (message != NULL);

	/* Set the authorisation header */
	if (self->priv->auth_token != NULL) {
		authorisation_header = g_strdup_printf ("GoogleLogin auth=%s", self->priv->auth_token);
		soup_message_headers_append (message->request_headers, "Authorization", authorisation_header);
		g_free (authorisation_header);
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
	/* See: http://code.google.com/apis/gdata/docs/2.0/reference.html#HTTPStatusCodes */
	switch (status) {
		case SOUP_STATUS_CANT_RESOLVE:
		case SOUP_STATUS_CANT_CONNECT:
		case SOUP_STATUS_SSL_FAILED:
		case SOUP_STATUS_IO_ERROR:
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_NETWORK_ERROR,
			             _("Cannot connect to the service's server."));
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
		default:
			/* We should not be called with anything other than the above operation types */
			g_assert_not_reached ();
	}
}

static void
set_authentication_details (GDataService *self, const gchar *username, const gchar *password, gboolean authenticated)
{
	GObject *service = G_OBJECT (self);
	GDataServicePrivate *priv = self->priv;

	g_object_freeze_notify (service);
	priv->authenticated = authenticated;

	if (authenticated == TRUE) {
		/* Update several properties the service holds */
		g_free (priv->username);

		/* Ensure the username is always a full e-mail address */
		if (strchr (username, '@') == NULL)
			priv->username = g_strdup_printf ("%s@" EMAIL_DOMAIN, username);
		else
			priv->username = g_strdup (username);

		g_free (priv->password);
		priv->password = g_strdup (password);

		g_object_notify (service, "username");
		g_object_notify (service, "password");
	}

	g_object_notify (service, "authenticated");
	g_object_thaw_notify (service);
}

static gboolean
authenticate (GDataService *self, const gchar *username, const gchar *password, gchar *captcha_token, gchar *captcha_answer,
              GCancellable *cancellable, GError **error)
{
	GDataServicePrivate *priv = self->priv;
	GDataServiceClass *klass;
	SoupMessage *message;
	gchar *request_body;
	guint status;
	gboolean retval;

	/* Prepare the request */
	klass = GDATA_SERVICE_GET_CLASS (self);
	request_body = soup_form_encode ("accountType", "HOSTED_OR_GOOGLE",
	                                 "Email", username,
	                                 "Passwd", password,
	                                 "service", klass->service_name,
	                                 "source", priv->client_id,
	                                 (captcha_token == NULL) ? NULL : "logintoken", captcha_token,
	                                 "loginanswer", captcha_answer,
	                                 NULL);

	/* Free the CAPTCHA token and answer if necessary */
	g_free (captcha_token);
	g_free (captcha_answer);

	/* Build the message */
	message = soup_message_new (SOUP_METHOD_POST, klass->authentication_uri);
	g_object_set_data_full (G_OBJECT (message), "session", g_object_ref (self->priv->session), (GDestroyNotify) g_object_unref);
	soup_message_set_request (message, "application/x-www-form-urlencoded", SOUP_MEMORY_TAKE, request_body, strlen (request_body));

	/* Send the message */
	status = soup_session_send_message (priv->session, message);

	/* Check for cancellation */
	if (g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE) {
		g_object_unref (message);
		return FALSE;
	}

	if (status != SOUP_STATUS_OK) {
		const gchar *response_body = message->response_body->data;
		gchar *error_start, *error_end, *uri_start, *uri_end, *uri = NULL;

		/* Parse the error response; see: http://code.google.com/apis/accounts/docs/AuthForInstalledApps.html#Errors */
		if (response_body == NULL)
			goto protocol_error;

		/* Error */
		error_start = strstr (response_body, "Error=");
		if (error_start == NULL)
			goto protocol_error;
		error_start += strlen ("Error=");

		error_end = strstr (error_start, "\n");
		if (error_end == NULL)
			goto protocol_error;

		if (strncmp (error_start, "CaptchaRequired", error_end - error_start) == 0) {
			const gchar *captcha_base_uri = "http://www.google.com/accounts/";
			gchar *captcha_start, *captcha_end, *captcha_uri, *new_captcha_answer;
			guint captcha_base_uri_length;

			/* CAPTCHA required to log in */
			captcha_start = strstr (response_body, "CaptchaUrl=");
			if (captcha_start == NULL)
				goto protocol_error;
			captcha_start += strlen ("CaptchaUrl=");

			captcha_end = strstr (captcha_start, "\n");
			if (captcha_end == NULL)
				goto protocol_error;

			/* Do some fancy memory stuff to save ourselves another alloc */
			captcha_base_uri_length = strlen (captcha_base_uri);
			captcha_uri = g_malloc (captcha_base_uri_length + (captcha_end - captcha_start) + 1);
			memcpy (captcha_uri, captcha_base_uri, captcha_base_uri_length);
			memcpy (captcha_uri + captcha_base_uri_length, captcha_start, (captcha_end - captcha_start));
			captcha_uri[captcha_base_uri_length + (captcha_end - captcha_start)] = '\0';

			/* Request a CAPTCHA answer from the application */
			g_signal_emit (self, service_signals[SIGNAL_CAPTCHA_CHALLENGE], 0, captcha_uri, &new_captcha_answer);
			g_free (captcha_uri);

			if (new_captcha_answer == NULL || *new_captcha_answer == '\0') {
				/* Translators: see http://en.wikipedia.org/wiki/CAPTCHA for information about CAPTCHAs */
				g_set_error_literal (error, GDATA_AUTHENTICATION_ERROR, GDATA_AUTHENTICATION_ERROR_CAPTCHA_REQUIRED,
				                     _("A CAPTCHA must be filled out to log in."));
				goto login_error;
			}

			/* Get the CAPTCHA token */
			captcha_start = strstr (response_body, "CaptchaToken=");
			if (captcha_start == NULL)
				goto protocol_error;
			captcha_start += strlen ("CaptchaToken=");

			captcha_end = strstr (captcha_start, "\n");
			if (captcha_end == NULL)
				goto protocol_error;

			/* Save the CAPTCHA token and answer, and attempt to log in with them */
			g_object_unref (message);

			return authenticate (self, username, password, g_strndup (captcha_start, captcha_end - captcha_start), new_captcha_answer,
			                     cancellable, error);
		} else if (strncmp (error_start, "Unknown", error_end - error_start) == 0) {
			goto protocol_error;
		} else if (strncmp (error_start, "BadAuthentication", error_end - error_start) == 0) {
			/* Looks like Error=BadAuthentication errors don't return a URI */
			g_set_error_literal (error, GDATA_AUTHENTICATION_ERROR, GDATA_AUTHENTICATION_ERROR_BAD_AUTHENTICATION,
			                     _("Your username or password were incorrect."));
			goto login_error;
		}

		/* Get the information URI */
		uri_start = strstr (response_body, "Url=");
		if (uri_start == NULL)
			goto protocol_error;
		uri_start += strlen ("Url=");

		uri_end = strstr (uri_start, "\n");
		if (uri_end == NULL)
			goto protocol_error;

		uri = g_strndup (uri_start, uri_end - uri_start);

		if (strncmp (error_start, "NotVerified", error_end - error_start) == 0) {
			g_set_error (error, GDATA_AUTHENTICATION_ERROR, GDATA_AUTHENTICATION_ERROR_NOT_VERIFIED,
			             /* Translators: the parameter is a URI for further information. */
			             _("Your account's e-mail address has not been verified. (%s)"), uri);
			goto login_error;
		} else if (strncmp (error_start, "TermsNotAgreed", error_end - error_start) == 0) {
			g_set_error (error, GDATA_AUTHENTICATION_ERROR, GDATA_AUTHENTICATION_ERROR_TERMS_NOT_AGREED,
			             /* Translators: the parameter is a URI for further information. */
			             _("You have not agreed to the service's terms and conditions. (%s)"), uri);
			goto login_error;
		} else if (strncmp (error_start, "AccountDeleted", error_end - error_start) == 0) {
			g_set_error (error, GDATA_AUTHENTICATION_ERROR, GDATA_AUTHENTICATION_ERROR_ACCOUNT_DELETED,
			             /* Translators: the parameter is a URI for further information. */
			             _("This account has been deleted. (%s)"), uri);
			goto login_error;
		} else if (strncmp (error_start, "AccountDisabled", error_end - error_start) == 0) {
			g_set_error (error, GDATA_AUTHENTICATION_ERROR, GDATA_AUTHENTICATION_ERROR_ACCOUNT_DISABLED,
			             /* Translators: the parameter is a URI for further information. */
			             _("This account has been disabled. (%s)"), uri);
			goto login_error;
		} else if (strncmp (error_start, "ServiceDisabled", error_end - error_start) == 0) {
			g_set_error (error, GDATA_AUTHENTICATION_ERROR, GDATA_AUTHENTICATION_ERROR_SERVICE_DISABLED,
			             /* Translators: the parameter is a URI for further information. */
			             _("This account's access to this service has been disabled. (%s)"), uri);
			goto login_error;
		} else if (strncmp (error_start, "ServiceUnavailable", error_end - error_start) == 0) {
			g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_UNAVAILABLE,
			             /* Translators: the parameter is a URI for further information. */
			             _("This service is not available at the moment. (%s)"), uri);
			goto login_error;
		}

		/* Unknown error type! */
		goto protocol_error;

login_error:
		g_free (uri);
		g_object_unref (message);

		return FALSE;
	}

	g_assert (message->response_body->data != NULL);

	retval = klass->parse_authentication_response (self, status, message->response_body->data, message->response_body->length, error);
	g_object_unref (message);

	return retval;

protocol_error:
	g_assert (klass->parse_error_response != NULL);
	klass->parse_error_response (self, GDATA_OPERATION_AUTHENTICATION, status, message->reason_phrase, message->response_body->data,
	                             message->response_body->length, error);

	g_object_unref (message);

	return FALSE;
}

typedef struct {
	/* Input */
	gchar *username;
	gchar *password;

	/* Output */
	GDataService *service;
	gboolean success;
} AuthenticateAsyncData;

static void
authenticate_async_data_free (AuthenticateAsyncData *self)
{
	g_free (self->username);
	g_free (self->password);
	if (self->service != NULL)
		g_object_unref (self->service);

	g_slice_free (AuthenticateAsyncData, self);
}

/* This is always called in the main thread via an idle function */
static gboolean
set_authentication_details_cb (AuthenticateAsyncData *data)
{
	set_authentication_details (data->service, data->username, data->password, data->success);
	authenticate_async_data_free (data);

	return FALSE;
}

static void
authenticate_thread (GSimpleAsyncResult *result, GDataService *service, GCancellable *cancellable)
{
	GError *error = NULL;
	AuthenticateAsyncData *data = g_simple_async_result_get_op_res_gpointer (result);

	/* Check to see if it's been cancelled already */
	if (g_cancellable_set_error_if_cancelled (cancellable, &error) == TRUE) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
		authenticate_async_data_free (data);
		return;
	}

	/* Authenticate and return */
	data->success = authenticate (service, data->username, data->password, NULL, NULL, cancellable, &error);
	if (data->success == FALSE) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
	}

	/* Update the authentication details held by the service in an idle function so that
	 * the service is only ever modified in the main thread */
	data->service = g_object_ref (service);
	g_idle_add ((GSourceFunc) set_authentication_details_cb, data);
}

/**
 * gdata_service_authenticate_async:
 * @self: a #GDataService
 * @username: the user's username
 * @password: the user's password
 * @cancellable: optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: data to pass to the @callback function
 *
 * Authenticates the #GDataService with the online service using the given @username and @password. @self, @username and
 * @password are all reffed/copied when this function is called, so can safely be freed after this function returns.
 *
 * For more details, see gdata_service_authenticate(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_authenticate_finish()
 * to get the results of the operation.
 **/
void
gdata_service_authenticate_async (GDataService *self, const gchar *username, const gchar *password,
				  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *result;
	AuthenticateAsyncData *data;

	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_return_if_fail (username != NULL);
	g_return_if_fail (password != NULL);
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	data = g_slice_new (AuthenticateAsyncData);
	data->username = g_strdup (username);
	data->password = g_strdup (password);
	data->service = NULL; /* set in authenticate_thread() */

	result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_service_authenticate_async);
	g_simple_async_result_set_op_res_gpointer (result, data, NULL);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) authenticate_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
}

/**
 * gdata_service_authenticate_finish:
 * @self: a #GDataService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous authentication operation started with gdata_service_authenticate_async().
 *
 * Return value: %TRUE if authentication was successful, %FALSE otherwise
 **/
gboolean
gdata_service_authenticate_finish (GDataService *self, GAsyncResult *async_result, GError **error)
{
	GSimpleAsyncResult *result = G_SIMPLE_ASYNC_RESULT (async_result);
	AuthenticateAsyncData *data;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_warn_if_fail (g_simple_async_result_get_source_tag (result) == gdata_service_authenticate_async);

	if (g_simple_async_result_propagate_error (result, error) == TRUE)
		return FALSE;

	data = g_simple_async_result_get_op_res_gpointer (result);
	if (data->success == TRUE)
		return TRUE;

	g_assert_not_reached ();
}

/**
 * gdata_service_authenticate:
 * @self: a #GDataService
 * @username: the user's username
 * @password: the user's password
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Authenticates the #GDataService with the online service using @username and @password; i.e. logs into the service with the given
 * user account. @username should be a full e-mail address (e.g. <literal>john.smith@gmail.com</literal>). If a full e-mail address is
 * not given, @username will have <literal>@gmail.com</literal> appended to create an e-mail address
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * A %GDATA_AUTHENTICATION_ERROR_BAD_AUTHENTICATION will be returned if authentication failed due to an incorrect username or password.
 * Other #GDataAuthenticationError errors can be returned for other conditions.
 *
 * If the service requires a CAPTCHA to be completed, the #GDataService::captcha-challenge signal will be emitted. The return value from
 * a signal handler for the signal should be the text from the image. If the text is %NULL or empty, authentication will fail with a
 * %GDATA_AUTHENTICATION_ERROR_CAPTCHA_REQUIRED error. Otherwise, authentication will be automatically and transparently restarted with
 * the new CAPTCHA details.
 *
 * A %GDATA_SERVICE_ERROR_PROTOCOL_ERROR will be returned if the server's responses were invalid. Subclasses of #GDataService can override
 * parsing the authentication response, and may return their own error codes. See their documentation for more details.
 *
 * Return value: %TRUE if authentication was successful, %FALSE otherwise
 **/
gboolean
gdata_service_authenticate (GDataService *self, const gchar *username, const gchar *password, GCancellable *cancellable, GError **error)
{
	gboolean retval;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), FALSE);
	g_return_val_if_fail (username != NULL, FALSE);
	g_return_val_if_fail (password != NULL, FALSE);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	retval = authenticate (self, username, password, NULL, NULL, cancellable, error);
	set_authentication_details (self, username, password, retval);

	return retval;
}

SoupMessage *
_gdata_service_build_message (GDataService *self, const gchar *method, const gchar *uri, const gchar *etag, gboolean etag_if_match)
{
	SoupMessage *message;
	GDataServiceClass *klass;

	/* Create the message and store a pointer to the session in it,
	 * so we can cancel the message in message_cancel_cb() from _gdata_service_send_message() */
	message = soup_message_new (method, uri);
	g_object_set_data_full (G_OBJECT (message), "session", g_object_ref (self->priv->session), (GDestroyNotify) g_object_unref);

	/* Make sure subclasses set their headers */
	klass = GDATA_SERVICE_GET_CLASS (self);
	if (klass->append_query_headers != NULL)
		klass->append_query_headers (self, message);

	/* Append the ETag header if possible */
	if (etag != NULL)
		soup_message_headers_append (message->request_headers, (etag_if_match == TRUE) ? "If-Match" : "If-None-Match", etag);

	return message;
}

static void
message_cancel_cb (GCancellable *cancellable, SoupMessage *message)
{
	if (message != NULL)
		soup_session_cancel_message (g_object_get_data (G_OBJECT (message), "session"), message, SOUP_STATUS_CANCELLED);
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

	gulong cancel_signal = 0;

	if (cancellable != NULL)
		cancel_signal = g_cancellable_connect (cancellable, (GCallback) message_cancel_cb, message, NULL);

	soup_message_set_flags (message, SOUP_MESSAGE_NO_REDIRECT);
	soup_session_send_message (self->priv->session, message);
	soup_message_set_flags (message, 0);

	if (cancel_signal != 0)
		g_cancellable_disconnect (cancellable, cancel_signal);

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

		soup_message_set_uri (message, new_uri);
		soup_uri_free (new_uri);

		/* Send the message again */
		if (cancellable != NULL)
			cancel_signal = g_cancellable_connect (cancellable, (GCallback) message_cancel_cb, message, NULL);

		soup_session_send_message (self->priv->session, message);

		if (cancel_signal != 0)
			g_cancellable_disconnect (cancellable, cancel_signal);
	}

	if (message->status_code == SOUP_STATUS_CANCELLED)
		g_assert (cancellable != NULL && g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE);

	return message->status_code;
}

typedef struct {
	/* Input */
	gchar *feed_uri;
	GDataQuery *query;
	GType entry_type;

	/* Output */
	GDataFeed *feed;
	GDataQueryProgressCallback progress_callback;
	gpointer progress_user_data;
} QueryAsyncData;

static void
query_async_data_free (QueryAsyncData *self)
{
	g_free (self->feed_uri);
	if (self->query)
		g_object_unref (self->query);
	if (self->feed)
		g_object_unref (self->feed);

	g_slice_free (QueryAsyncData, self);
}

static void
query_thread (GSimpleAsyncResult *result, GDataService *service, GCancellable *cancellable)
{
	GError *error = NULL;
	QueryAsyncData *data = g_simple_async_result_get_op_res_gpointer (result);

	/* Execute the query and return */
	data->feed = gdata_service_query (service, data->feed_uri, data->query, data->entry_type, cancellable,
	                                  data->progress_callback, data->progress_user_data, &error);
	if (data->feed == NULL && error != NULL) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
	}
}

/**
 * gdata_service_query_async:
 * @self: a #GDataService
 * @feed_uri: the feed URI to query, including the host name and protocol
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @entry_type: a #GType for the #GDataEntry<!-- -->s to build from the XML
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @callback: a #GAsyncReadyCallback to call when the query is finished
 * @user_data: data to pass to the @callback function
 *
 * Queries the service's @feed_uri feed to build a #GDataFeed. @self, @feed_uri and
 * @query are all reffed/copied when this function is called, so can safely be freed after this function returns.
 *
 * For more details, see gdata_service_query(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_query_finish()
 * to get the results of the operation.
 **/
void
gdata_service_query_async (GDataService *self, const gchar *feed_uri, GDataQuery *query, GType entry_type, GCancellable *cancellable,
                           GDataQueryProgressCallback progress_callback, gpointer progress_user_data,
                           GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *result;
	QueryAsyncData *data;

	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_return_if_fail (feed_uri != NULL);
	g_return_if_fail (g_type_is_a (entry_type, GDATA_TYPE_ENTRY));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	data = g_slice_new (QueryAsyncData);
	data->feed_uri = g_strdup (feed_uri);
	data->query = (query != NULL) ? g_object_ref (query) : NULL;
	data->entry_type = entry_type;
	data->progress_callback = progress_callback;
	data->progress_user_data = progress_user_data;

	result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_service_query_async);
	g_simple_async_result_set_op_res_gpointer (result, data, (GDestroyNotify) query_async_data_free);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) query_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
}

/**
 * gdata_service_query_finish:
 * @self: a #GDataService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous query operation started with gdata_service_query_async().
 *
 * Return value: a #GDataFeed of query results, or %NULL; unref with g_object_unref()
 **/
GDataFeed *
gdata_service_query_finish (GDataService *self, GAsyncResult *async_result, GError **error)
{
	GSimpleAsyncResult *result = G_SIMPLE_ASYNC_RESULT (async_result);
	QueryAsyncData *data;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_warn_if_fail (g_simple_async_result_get_source_tag (result) == gdata_service_query_async);

	if (g_simple_async_result_propagate_error (result, error) == TRUE)
		return NULL;

	data = g_simple_async_result_get_op_res_gpointer (result);
	if (data->feed != NULL)
		return g_object_ref (data->feed);
	return NULL;
}

/* Does the bulk of the work of gdata_service_query. Split out because certain queries (such as that done by
 * gdata_service_query_single_entry()) only return a single entry, and thus need special parsing code. */
SoupMessage *
_gdata_service_query (GDataService *self, const gchar *feed_uri, GDataQuery *query, GCancellable *cancellable, GError **error)
{
	SoupMessage *message;
	guint status;
	gulong cancel_signal = 0;
	const gchar *etag = NULL;

	/* Append the ETag header if possible */
	if (query != NULL)
		etag = gdata_query_get_etag (query);

	/* Build the message */
	if (query != NULL) {
		gchar *query_uri = gdata_query_get_query_uri (query, feed_uri);
		message = _gdata_service_build_message (self, SOUP_METHOD_GET, query_uri, etag, FALSE);
		g_free (query_uri);
	} else {
		message = _gdata_service_build_message (self, SOUP_METHOD_GET, feed_uri, etag, FALSE);
	}

	/* TODO: Document that cancellation only applies to network activity; not to the processing done afterwards */

	/* Send the message */
	if (cancellable != NULL)
		cancel_signal = g_cancellable_connect (cancellable, (GCallback) message_cancel_cb, message, NULL);

	status = soup_session_send_message (self->priv->session, message);

	if (cancel_signal != 0)
		g_cancellable_disconnect (cancellable, cancel_signal);

	if (status == SOUP_STATUS_NOT_MODIFIED) {
		/* Not modified; ETag has worked */
		g_object_unref (message);
		return NULL;
	} else if (status == SOUP_STATUS_CANCELLED) {
		/* Cancelled */
		g_assert (g_cancellable_set_error_if_cancelled (cancellable, error) == TRUE);
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

/**
 * gdata_service_query:
 * @self: a #GDataService
 * @feed_uri: the feed URI to query, including the host name and protocol
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @entry_type: a #GType for the #GDataEntry<!-- -->s to build from the XML
 * @cancellable: optional #GCancellable object, or %NULL
 * @progress_callback: a #GDataQueryProgressCallback to call when an entry is loaded, or %NULL
 * @progress_user_data: data to pass to the @progress_callback function
 * @error: a #GError, or %NULL
 *
 * Queries the service's @feed_uri feed to build a #GDataFeed.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
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
 * Return value: a #GDataFeed of query results, or %NULL; unref with g_object_unref()
 **/
GDataFeed *
gdata_service_query (GDataService *self, const gchar *feed_uri, GDataQuery *query, GType entry_type,
                     GCancellable *cancellable, GDataQueryProgressCallback progress_callback, gpointer progress_user_data, GError **error)
{
	GDataServiceClass *klass;
	GDataFeed *feed;
	SoupMessage *message;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (feed_uri != NULL, NULL);
	g_return_val_if_fail (g_type_is_a (entry_type, GDATA_TYPE_ENTRY), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	message = _gdata_service_query (self, feed_uri, query, cancellable, error);
	if (message == NULL)
		return NULL;

	g_assert (message->response_body->data != NULL);
	klass = GDATA_SERVICE_GET_CLASS (self);
	feed = _gdata_feed_new_from_xml (klass->feed_type, message->response_body->data, message->response_body->length, entry_type,
	                                 progress_callback, progress_user_data, error);
	g_object_unref (message);

	if (feed == NULL)
		return NULL;

	/* Update the query with the feed's ETag */
	if (query != NULL && feed != NULL && gdata_feed_get_etag (feed) != NULL)
		gdata_query_set_etag (query, gdata_feed_get_etag (feed));

	/* Update the query with the next and previous URIs from the feed */
	if (query != NULL && feed != NULL) {
		GDataLink *link;

		link = gdata_feed_look_up_link (feed, "next");
		if (link != NULL)
			_gdata_query_set_next_uri (query, gdata_link_get_uri (link));
		link = gdata_feed_look_up_link (feed, "previous");
		if (link != NULL)
			_gdata_query_set_previous_uri (query, gdata_link_get_uri (link));
	}

	return feed;
}

/**
 * gdata_service_query_single_entry:
 * @self: a #GDataService
 * @entry_id: the entry ID of the desired entry
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @entry_type: a #GType for the #GDataEntry to build from the XML
 * @cancellable: a #GCancellable, or %NULL
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
 * Return value: a #GDataEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.7.0
 **/
GDataEntry *
gdata_service_query_single_entry (GDataService *self, const gchar *entry_id, GDataQuery *query, GType entry_type,
                                  GCancellable *cancellable, GError **error)
{
	GDataEntryClass *klass;
	GDataEntry *entry;
	gchar *entry_uri;
	SoupMessage *message;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (entry_id != NULL, NULL);
	g_return_val_if_fail (query == NULL || GDATA_IS_QUERY (query), NULL);
	g_return_val_if_fail (g_type_is_a (entry_type, GDATA_TYPE_ENTRY) == TRUE, NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	/* Query for just the specified entry */
	klass = GDATA_ENTRY_CLASS (g_type_class_ref (entry_type));
	g_assert (klass->get_entry_uri != NULL);

	entry_uri = klass->get_entry_uri (entry_id);
	message = _gdata_service_query (GDATA_SERVICE (self), entry_uri, query, cancellable, error);
	g_free (entry_uri);

	if (message == NULL) {
		g_type_class_unref (klass);
		return NULL;
	}

	g_assert (message->response_body->data != NULL);
	entry = GDATA_ENTRY (gdata_parsable_new_from_xml (entry_type, message->response_body->data, message->response_body->length, error));
	g_object_unref (message);
	g_type_class_unref (klass);

	return entry;
}

typedef struct {
	gchar *entry_id;
	GDataQuery *query;
	GType entry_type;
} QuerySingleEntryAsyncData;

static void
query_single_entry_async_data_free (QuerySingleEntryAsyncData *data)
{
	g_free (data->entry_id);
	if (data->query != NULL)
		g_object_unref (data->query);
	g_slice_free (QuerySingleEntryAsyncData, data);
}

static void
query_single_entry_thread (GSimpleAsyncResult *result, GDataService *service, GCancellable *cancellable)
{
	GDataEntry *entry;
	GError *error = NULL;
	QuerySingleEntryAsyncData *data = g_simple_async_result_get_op_res_gpointer (result);

	/* Execute the query and return */
	entry = gdata_service_query_single_entry (service, data->entry_id, data->query, data->entry_type, cancellable, &error);
	if (entry == NULL && error != NULL) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
	}

	g_simple_async_result_set_op_res_gpointer (result, entry, (GDestroyNotify) g_object_unref);
}

/**
 * gdata_service_query_single_entry_async:
 * @self: a #GDataService
 * @entry_id: the entry ID of the desired entry
 * @query: a #GDataQuery with the query parameters, or %NULL
 * @entry_type: a #GType for the #GDataEntry to build from the XML
 * @cancellable: optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the query is finished
 * @user_data: data to pass to the @callback function
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
 * Since: 0.7.0
 **/
void
gdata_service_query_single_entry_async (GDataService *self, const gchar *entry_id, GDataQuery *query, GType entry_type,
                                        GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *result;
	QuerySingleEntryAsyncData *data;

	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_return_if_fail (entry_id != NULL);
	g_return_if_fail (query == NULL || GDATA_IS_QUERY (query));
	g_return_if_fail (g_type_is_a (entry_type, GDATA_TYPE_ENTRY) == TRUE);
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	data = g_slice_new (QuerySingleEntryAsyncData);
	data->query = (query != NULL) ? g_object_ref (query) : NULL;
	data->entry_id = g_strdup (entry_id);
	data->entry_type = entry_type;

	result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_service_query_single_entry_async);
	g_simple_async_result_set_op_res_gpointer (result, data, (GDestroyNotify) query_single_entry_async_data_free);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) query_single_entry_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
}

/**
 * gdata_service_query_single_entry_finish:
 * @self: a #GDataService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous query operation for a single entry, as started with gdata_service_query_single_entry_async().
 *
 * Return value: a #GDataEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.7.0
 **/
GDataEntry *
gdata_service_query_single_entry_finish (GDataService *self, GAsyncResult *async_result, GError **error)
{
	GSimpleAsyncResult *result = G_SIMPLE_ASYNC_RESULT (async_result);
	GDataEntry *entry;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	g_warn_if_fail (g_simple_async_result_get_source_tag (result) == gdata_service_query_single_entry_async);

	if (g_simple_async_result_propagate_error (result, error) == TRUE)
		return NULL;

	entry = g_simple_async_result_get_op_res_gpointer (result);
	if (entry != NULL)
		return g_object_ref (entry);
	return NULL;
}

typedef struct {
	gchar *upload_uri;
	GDataEntry *entry;
} InsertEntryAsyncData;

static void
insert_entry_async_data_free (InsertEntryAsyncData *self)
{
	g_free (self->upload_uri);
	if (self->entry)
		g_object_unref (self->entry);

	g_slice_free (InsertEntryAsyncData, self);
}

static void
insert_entry_thread (GSimpleAsyncResult *result, GDataService *service, GCancellable *cancellable)
{
	GDataEntry *updated_entry;
	GError *error = NULL;
	InsertEntryAsyncData *data = g_simple_async_result_get_op_res_gpointer (result);

	/* Insert the entry and return */
	updated_entry = gdata_service_insert_entry (service, data->upload_uri, data->entry, cancellable, &error);
	if (updated_entry == NULL) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
		return;
	}

	/* Swap the old entry with the new one */
	g_simple_async_result_set_op_res_gpointer (result, updated_entry, (GDestroyNotify) g_object_unref);
}

/**
 * gdata_service_insert_entry_async:
 * @self: a #GDataService
 * @upload_uri: the URI to which the upload should be sent
 * @entry: the #GDataEntry to insert
 * @cancellable: optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when insertion is finished, or %NULL
 * @user_data: data to pass to the @callback function
 *
 * Inserts @entry by uploading it to the online service at @upload_uri. @self, @upload_uri and
 * @entry are all reffed/copied when this function is called, so can safely be freed after this function returns.
 *
 * For more details, see gdata_service_insert_entry(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_insert_entry_finish()
 * to get the results of the operation.
 *
 * Since: 0.3.0
 **/
void
gdata_service_insert_entry_async (GDataService *self, const gchar *upload_uri, GDataEntry *entry, GCancellable *cancellable,
                                  GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *result;
	InsertEntryAsyncData *data;

	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_return_if_fail (upload_uri != NULL);
	g_return_if_fail (GDATA_IS_ENTRY (entry));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	data = g_slice_new (InsertEntryAsyncData);
	data->upload_uri = g_strdup (upload_uri);
	data->entry = g_object_ref (entry);

	result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_service_insert_entry_async);
	g_simple_async_result_set_op_res_gpointer (result, data, (GDestroyNotify) insert_entry_async_data_free);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) insert_entry_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
}

/**
 * gdata_service_insert_entry_finish:
 * @self: a #GDataService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous entry insertion operation started with gdata_service_insert_entry_async().
 *
 * Return value: an updated #GDataEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.3.0
 **/
GDataEntry *
gdata_service_insert_entry_finish (GDataService *self, GAsyncResult *async_result, GError **error)
{
	GSimpleAsyncResult *result = G_SIMPLE_ASYNC_RESULT (async_result);
	GDataEntry *entry;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_warn_if_fail (g_simple_async_result_get_source_tag (result) == gdata_service_insert_entry_async);

	if (g_simple_async_result_propagate_error (result, error) == TRUE)
		return NULL;

	entry = g_simple_async_result_get_op_res_gpointer (result);
	if (entry != NULL)
		return g_object_ref (entry);

	g_assert_not_reached ();
}

/**
 * gdata_service_insert_entry:
 * @self: a #GDataService
 * @upload_uri: the URI to which the upload should be sent
 * @entry: the #GDataEntry to insert
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Inserts @entry by uploading it to the online service at @upload_uri. For more information about the concept of inserting entries, see
 * the <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/basics.html#InsertingEntry">online documentation</ulink> for the GData
 * protocol.
 *
 * The service will return an updated version of the entry, which is the return value of this function on success.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If the entry is marked as already having been inserted a %GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED error will be returned immediately
 * (there will be no network requests).
 *
 * If there is an error inserting the entry, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned. Currently, subclasses
 * <emphasis>cannot</emphasis> cannot override this or provide more specific errors.
 *
 * Return value: an updated #GDataEntry, or %NULL
 **/
GDataEntry *
gdata_service_insert_entry (GDataService *self, const gchar *upload_uri, GDataEntry *entry, GCancellable *cancellable, GError **error)
{
	GDataEntry *updated_entry;
	SoupMessage *message;
	gchar *upload_data;
	guint status;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (upload_uri != NULL, NULL);
	g_return_val_if_fail (GDATA_IS_ENTRY (entry), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	if (gdata_entry_is_inserted (entry) == TRUE) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_ENTRY_ALREADY_INSERTED,
		                     _("The entry has already been inserted."));
		return NULL;
	}

	message = _gdata_service_build_message (self, SOUP_METHOD_POST, upload_uri, NULL, FALSE);

	/* Append the data */
	upload_data = gdata_parsable_get_xml (GDATA_PARSABLE (entry));
	soup_message_set_request (message, "application/atom+xml", SOUP_MEMORY_TAKE, upload_data, strlen (upload_data));

	/* Send the message */
	status = _gdata_service_send_message (self, message, cancellable, error);

	if (status == SOUP_STATUS_NONE || status == SOUP_STATUS_CANCELLED) {
		/* Redirect error or cancelled */
		g_object_unref (message);
		return NULL;
	} else if (status != SOUP_STATUS_CREATED) {
		/* Error */
		GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (self);
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (self, GDATA_OPERATION_INSERTION, status, message->reason_phrase, message->response_body->data,
		                             message->response_body->length, error);
		g_object_unref (message);
		return NULL;
	}

	/* Parse the XML; create and return a new GDataEntry of the same type as @entry */
	g_assert (message->response_body->data != NULL);
	updated_entry = GDATA_ENTRY (gdata_parsable_new_from_xml (G_OBJECT_TYPE (entry), message->response_body->data, message->response_body->length,
	                                                          error));
	g_object_unref (message);

	return updated_entry;
}

static void
update_entry_thread (GSimpleAsyncResult *result, GDataService *service, GCancellable *cancellable)
{
	GDataEntry *updated_entry;
	GError *error = NULL;

	/* Update the entry and return */
	updated_entry = gdata_service_update_entry (service, g_simple_async_result_get_op_res_gpointer (result), cancellable, &error);
	if (updated_entry == NULL) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
		return;
	}

	/* Swap the old entry with the new one */
	g_simple_async_result_set_op_res_gpointer (result, updated_entry, (GDestroyNotify) g_object_unref);
}

/**
 * gdata_service_update_entry_async:
 * @self: a #GDataService
 * @entry: the #GDataEntry to update
 * @cancellable: optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when the update is finished, or %NULL
 * @user_data: data to pass to the @callback function
 *
 * Updates @entry by PUTting it to its <literal>edit</literal> link's URI. @self and
 * @entry are both reffed when this function is called, so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_service_update_entry(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_update_entry_finish()
 * to get the results of the operation.
 *
 * Since: 0.3.0
 **/
void
gdata_service_update_entry_async (GDataService *self, GDataEntry *entry, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *result;

	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_return_if_fail (GDATA_IS_ENTRY (entry));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_service_update_entry_async);
	g_simple_async_result_set_op_res_gpointer (result, g_object_ref (entry), (GDestroyNotify) g_object_unref);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) update_entry_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
}

/**
 * gdata_service_update_entry_finish:
 * @self: a #GDataService
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous entry update operation started with gdata_service_update_entry_async().
 *
 * Return value: an updated #GDataEntry, or %NULL; unref with g_object_unref()
 *
 * Since: 0.3.0
 **/
GDataEntry *
gdata_service_update_entry_finish (GDataService *self, GAsyncResult *async_result, GError **error)
{
	GSimpleAsyncResult *result = G_SIMPLE_ASYNC_RESULT (async_result);
	GDataEntry *entry;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_warn_if_fail (g_simple_async_result_get_source_tag (result) == gdata_service_update_entry_async);

	if (g_simple_async_result_propagate_error (result, error) == TRUE)
		return NULL;

	entry = g_simple_async_result_get_op_res_gpointer (result);
	if (entry != NULL)
		return g_object_ref (entry);

	g_assert_not_reached ();
}

/**
 * gdata_service_update_entry:
 * @self: a #GDataService
 * @entry: the #GDataEntry to update
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Updates @entry by PUTting it to its <literal>edit</literal> link's URI. For more information about the concept of updating entries, see
 * the <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/basics.html#UpdatingEntry">online documentation</ulink> for the GData
 * protocol.
 *
 * The service will return an updated version of the entry, which is the return value of this function on success.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If there is an error updating the entry, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned. Currently, subclasses
 * <emphasis>cannot</emphasis> cannot override this or provide more specific errors.
 *
 * Return value: an updated #GDataEntry, or %NULL
 *
 * Since: 0.2.0
 **/
GDataEntry *
gdata_service_update_entry (GDataService *self, GDataEntry *entry, GCancellable *cancellable, GError **error)
{
	GDataEntry *updated_entry;
	GDataLink *link;
	SoupMessage *message;
	gchar *upload_data;
	guint status;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	g_return_val_if_fail (GDATA_IS_ENTRY (entry), NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	/* Get the edit URI */
	link = gdata_entry_look_up_link (entry, GDATA_LINK_EDIT);
	g_assert (link != NULL);
	message = _gdata_service_build_message (self, SOUP_METHOD_PUT, gdata_link_get_uri (link), gdata_entry_get_etag (entry), TRUE);

	/* Append the data */
	upload_data = gdata_parsable_get_xml (GDATA_PARSABLE (entry));
	soup_message_set_request (message, "application/atom+xml", SOUP_MEMORY_TAKE, upload_data, strlen (upload_data));

	/* Send the message */
	status = _gdata_service_send_message (self, message, cancellable, error);

	if (status == SOUP_STATUS_NONE || status == SOUP_STATUS_CANCELLED) {
		/* Redirect error or cancelled */
		g_object_unref (message);
		return NULL;
	} else if (status != SOUP_STATUS_OK) {
		/* Error */
		GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (self);
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (self, GDATA_OPERATION_UPDATE, status, message->reason_phrase, message->response_body->data,
		                             message->response_body->length, error);
		g_object_unref (message);
		return NULL;
	}

	/* Parse the XML; create and return a new GDataEntry of the same type as @entry */
	g_assert (message->response_body->data != NULL);
	updated_entry = GDATA_ENTRY (gdata_parsable_new_from_xml (G_OBJECT_TYPE (entry), message->response_body->data, message->response_body->length,
	                                                          error));
	g_object_unref (message);

	return updated_entry;
}

static void
delete_entry_thread (GSimpleAsyncResult *result, GDataService *service, GCancellable *cancellable)
{
	gboolean success;
	GError *error = NULL;

	/* Delete the entry and return */
	success = gdata_service_delete_entry (service, g_simple_async_result_get_op_res_gpointer (result), cancellable, &error);
	if (success == FALSE) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
		return;
	}

	/* Replace the entry with the success value */
	g_simple_async_result_set_op_res_gboolean (result, success);
}

/**
 * gdata_service_delete_entry_async:
 * @self: a #GDataService
 * @entry: the #GDataEntry to delete
 * @cancellable: optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when deletion is finished, or %NULL
 * @user_data: data to pass to the @callback function
 *
 * Deletes @entry from the server. @self and @entry are both reffed when this function is called,
 * so can safely be unreffed after this function returns.
 *
 * For more details, see gdata_service_delete_entry(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_service_delete_entry_finish()
 * to get the results of the operation.
 *
 * Since: 0.3.0
 **/
void
gdata_service_delete_entry_async (GDataService *self, GDataEntry *entry, GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *result;

	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_return_if_fail (GDATA_IS_ENTRY (entry));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_service_delete_entry_async);
	g_simple_async_result_set_op_res_gpointer (result, g_object_ref (entry), (GDestroyNotify) g_object_unref);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) delete_entry_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
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
 **/
gboolean
gdata_service_delete_entry_finish (GDataService *self, GAsyncResult *async_result, GError **error)
{
	GSimpleAsyncResult *result = G_SIMPLE_ASYNC_RESULT (async_result);

	g_return_val_if_fail (GDATA_IS_SERVICE (self), FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_warn_if_fail (g_simple_async_result_get_source_tag (result) == gdata_service_delete_entry_async);

	if (g_simple_async_result_propagate_error (result, error) == TRUE)
		return FALSE;

	return g_simple_async_result_get_op_res_gboolean (result);
}

/**
 * gdata_service_delete_entry:
 * @self: a #GDataService
 * @entry: the #GDataEntry to delete
 * @cancellable: optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Deletes @entry from the server. For more information about the concept of deleting entries, see the
 * <ulink type="http" url="http://code.google.com/apis/gdata/docs/2.0/basics.html#DeletingEntry">online documentation</ulink> for the GData
 * protocol.
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If there is an error deleting the entry, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR error will be returned. Currently, subclasses
 * <emphasis>cannot</emphasis> cannot override this or provide more specific errors.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: 0.2.0
 **/
gboolean
gdata_service_delete_entry (GDataService *self, GDataEntry *entry, GCancellable *cancellable, GError **error)
{
	GDataLink *link;
	SoupMessage *message;
	guint status;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), FALSE);
	g_return_val_if_fail (GDATA_IS_ENTRY (entry), FALSE);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	/* Get the edit URI */
	link = gdata_entry_look_up_link (entry, GDATA_LINK_EDIT);
	g_assert (link != NULL);
	message = _gdata_service_build_message (self, SOUP_METHOD_DELETE, gdata_link_get_uri (link), gdata_entry_get_etag (entry), TRUE);

	/* Send the message */
	status = _gdata_service_send_message (self, message, cancellable, error);

	if (status == SOUP_STATUS_NONE || status == SOUP_STATUS_CANCELLED) {
		/* Redirect error or cancelled */
		g_object_unref (message);
		return FALSE;
	} else if (status != SOUP_STATUS_OK) {
		/* Error */
		GDataServiceClass *klass = GDATA_SERVICE_GET_CLASS (self);
		g_assert (klass->parse_error_response != NULL);
		klass->parse_error_response (self, GDATA_OPERATION_DELETION, status, message->reason_phrase, message->response_body->data,
		                             message->response_body->length, error);
		g_object_unref (message);
		return FALSE;
	}

	g_object_unref (message);

	return TRUE;
}

static void
notify_proxy_uri_cb (GObject *gobject, GParamSpec *pspec, GObject *self)
{
	g_object_notify (self, "proxy-uri");
}

/**
 * gdata_service_get_proxy_uri:
 * @self: a #GDataService
 *
 * Gets the proxy URI on the #GDataService's #SoupSession.
 *
 * Return value: the proxy URI, or %NULL
 *
 * Since: 0.2.0
 **/
SoupURI *
gdata_service_get_proxy_uri (GDataService *self)
{
	SoupURI *proxy_uri;

	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);

	g_object_get (self->priv->session, SOUP_SESSION_PROXY_URI, &proxy_uri, NULL);
	g_object_unref (proxy_uri); /* remove the ref added by g_object_get */

	return proxy_uri;
}

/**
 * gdata_service_set_proxy_uri:
 * @self: a #GDataService
 * @proxy_uri: the proxy URI, or %NULL
 *
 * Sets the proxy URI on the #SoupSession used internally by the given #GDataService.
 * This forces all requests through the given proxy.
 *
 * If @proxy_uri is %NULL, no proxy will be used.
 *
 * Since: 0.2.0
 **/
void
gdata_service_set_proxy_uri (GDataService *self, SoupURI *proxy_uri)
{
	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_object_set (self->priv->session, SOUP_SESSION_PROXY_URI, proxy_uri, NULL);
	g_object_notify (G_OBJECT (self), "proxy-uri");
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
 **/
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
 * Since: 0.7.0
 **/
void
gdata_service_set_timeout (GDataService *self, guint timeout)
{
	g_return_if_fail (GDATA_IS_SERVICE (self));
	g_object_set (self->priv->session, SOUP_SESSION_TIMEOUT, timeout, NULL);
	g_object_notify (G_OBJECT (self), "timeout");
}

/**
 * gdata_service_is_authenticated:
 * @self: a #GDataService
 *
 * Returns whether a user is authenticated with the online service through @self.
 * Authentication is performed by calling gdata_service_authenticate() or gdata_service_authenticate_async().
 *
 * Return value: %TRUE if a user is authenticated, %FALSE otherwise
 **/
gboolean
gdata_service_is_authenticated (GDataService *self)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), FALSE);
	return self->priv->authenticated;
}

/* This should only ever be called in the main thread */
void
_gdata_service_set_authenticated (GDataService *self, gboolean authenticated)
{
	g_return_if_fail (GDATA_IS_SERVICE (self));
	self->priv->authenticated = authenticated;
	g_object_notify (G_OBJECT (self), "authenticated");
}

/**
 * gdata_service_get_client_id:
 * @self: a #GDataService
 *
 * Returns the service's client ID, as specified on constructing the #GDataService.
 *
 * Return value: the service's client ID
 **/
const gchar *
gdata_service_get_client_id (GDataService *self)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	return self->priv->client_id;
}

/**
 * gdata_service_get_username:
 * @self: a #GDataService
 *
 * Returns the username of the currently-authenticated user, or %NULL if nobody is authenticated.
 *
 * Return value: the username of the currently-authenticated user, or %NULL
 **/
const gchar *
gdata_service_get_username (GDataService *self)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	return self->priv->username;
}

/**
 * gdata_service_get_password:
 * @self: a #GDataService
 *
 * Returns the password of the currently-authenticated user, or %NULL if nobody is authenticated.
 *
 * Return value: the password of the currently-authenticated user, or %NULL
 **/
const gchar *
gdata_service_get_password (GDataService *self)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	return self->priv->password;
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
 * Returns the name of the scheme to use (either "http" or "https") for network operations if a request should use HTTPS. This allows
 * requests to normally use HTTPS, but have the option of using HTTP for debugging purposes. If a request should normally use HTTP, that
 * should be hard-coded in the relevant code, and this function needn't be called.
 *
 * Return value: the scheme to use
 *
 * Since: 0.6.0
 */
const gchar *
_gdata_service_get_scheme (void)
{
	static gint force_http = -1;

	if (force_http == -1)
		force_http = (g_getenv ("LIBGDATA_FORCE_HTTP") != NULL);

	if (force_http)
		return "http";
	return "https";
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
	g_debug ("%c %s", direction, data);
}

/**
 * _gdata_service_get_log_level:
 *
 * Returns the logging level for the library, currently set by an environment variable.
 *
 * Return value: the log level
 *
 * Since: 0.7.0
 **/
GDataLogLevel
_gdata_service_get_log_level (void)
{
	static int level = -1;

	if (level < 0) {
		const gchar *envvar = g_getenv ("LIBGDATA_DEBUG");
		if (envvar != NULL)
			level = atoi (envvar);
		level = MIN (MAX (level, 0), GDATA_LOG_FULL);
	}

	return level;
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
 **/
const gchar *
gdata_service_get_locale (GDataService *self)
{
	g_return_val_if_fail (GDATA_IS_SERVICE (self), NULL);
	return self->priv->locale;
}

/**
 * gdata_service_set_locale:
 * @self: a #GDataService
 * @locale: the new locale in Unix locale format, or %NULL for the default locale
 *
 * Set the locale used for network requests to @locale, given in standard Unix locale format. See #GDataService:locale for more details.
 *
 * Note that while it's possible to change the locale after sending network requests, it is unsupported, as the server-side software may behave
 * unexpectedly. The only supported use of this function is after creation of a service, but before any network requests are made.
 *
 * Since: 0.7.0
 **/
void
gdata_service_set_locale (GDataService *self, const gchar *locale)
{
	g_return_if_fail (GDATA_IS_SERVICE (self));

	g_free (self->priv->locale);
	self->priv->locale = g_strdup (locale);
	g_object_notify (G_OBJECT (self), "locale");
}
