/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2011 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-client-login-authorizer
 * @short_description: GData ClientLogin authorization interface
 * @stability: Unstable
 * @include: gdata/gdata-client-login-authorizer.h
 *
 * #GDataClientLoginAuthorizer provides an implementation of the #GDataAuthorizer interface for authentication and authorization using the deprecated
 * <ulink type="http" url="http://code.google.com/apis/accounts/docs/AuthForInstalledApps.html">ClientLogin</ulink> process.
 *
 * As noted, the ClientLogin process is being deprecated in favour of OAuth 2.0. This API is not (yet) deprecated, however. One of the main reasons
 * for ClientLogin being deprecated is that it cannot support two-factor authentication as now available to Google Accounts. Any account which has
 * two-factor authentication enabled has to use a service-specific one-time password instead if a client is authenticating with
 * #GDataClientLoginAuthorizer. More documentation about this is
 * <ulink type="http" url="http://www.google.com/support/accounts/bin/static.py?page=guide.cs&guide=1056283&topic=1056286">available online</ulink>.
 *
 * The ClientLogin process is a simple one whereby the user's Google Account username and password are sent over an HTTPS connection to the Google
 * Account servers (when gdata_client_login_authorizer_authenticate() is called), which return an authorization token. This token is then attached to
 * all future requests to the online service. A slight complication is that the Google Accounts service may return a CAPTCHA challenge instead of
 * immediately returning an authorization token. In this case, the #GDataClientLoginAuthorizer::captcha-challenge signal will be emitted, and the
 * user's response to the CAPTCHA should be returned by the handler.
 *
 * ClientLogin does not natively support authorization against multiple authorization domains concurrently with a single authorization token, so it
 * has to be simulated by maintaining multiple authorization tokens if multiple authorization domains are used. This means that proportionally more
 * network requests are made when gdata_client_login_authorizer_authenticate() is called, which will be proportionally slower. Handling of the
 * multiple authorization tokens is otherwise transparent to the client.
 *
 * Each authorization token is long lived, so reauthorization is rarely necessary with #GDataClientLoginAuthorizer. Consequently, refreshing
 * authorization using gdata_authorizer_refresh_authorization() is not supported by #GDataClientLoginAuthorizer, and will immediately return %FALSE
 * with no error set.
 *
 * <example>
 * 	<title>Authenticating Asynchronously Using ClientLogin</title>
 * 	<programlisting>
 *	GDataSomeService *service;
 *	GDataClientLoginAuthorizer *authorizer;
 *
 *	/<!-- -->* Create an authorizer and authenticate and authorize the service we're using, asynchronously. *<!-- -->/
 *	authorizer = gdata_client_login_authorizer_new ("companyName-applicationName-versionID", GDATA_TYPE_SOME_SERVICE);
 *	gdata_client_login_authorizer_authenticate_async (authorizer, username, password, cancellable,
 *	                                                  (GAsyncReadyCallback) authenticate_cb, user_data);
 *
 *	/<!-- -->* Create a service object and link it with the authorizer *<!-- -->/
 *	service = gdata_some_service_new (GDATA_AUTHORIZER (authorizer));
 *
 *	static void
 *	authenticate_cb (GDataClientLoginAuthorizer *authorizer, GAsyncResult *async_result, gpointer user_data)
 *	{
 *		GError *error = NULL;
 *
 *		if (gdata_client_login_authorizer_authenticate_finish (authorizer, async_result, &error) == FALSE) {
 *			/<!-- -->* Notify the user of all errors except cancellation errors *<!-- -->/
 *			if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
 *				g_error ("Authentication failed: %s", error->message);
 *			}
 *			g_error_free (error);
 *			return;
 *		}
 *
 *		/<!-- -->* (The client is now authenticated and authorized against the service.
 *		 * It can now proceed to execute queries on the service object which require the user to be authenticated.) *<!-- -->/
 *	}
 *
 *	g_object_unref (service);
 *	g_object_unref (authorizer);
 *	</programlisting>
 * </example>
 *
 * Since: 0.9.0
 */

#include <config.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <string.h>

#include "gdata-service.h"
#include "gdata-private.h"
#include "gdata-marshal.h"
#include "gdata-client-login-authorizer.h"

/* The default e-mail domain to use for usernames */
#define EMAIL_DOMAIN "gmail.com"

GQuark
gdata_client_login_authorizer_error_quark (void)
{
	return g_quark_from_static_string ("gdata-client-login-authorizer-error-quark");
}

static void authorizer_init (GDataAuthorizerInterface *iface);
static void dispose (GObject *object);
static void finalize (GObject *object);
static void get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void process_request (GDataAuthorizer *self, GDataAuthorizationDomain *domain, SoupMessage *message);
static gboolean is_authorized_for_domain (GDataAuthorizer *self, GDataAuthorizationDomain *domain);

static void notify_proxy_uri_cb (GObject *gobject, GParamSpec *pspec, GDataClientLoginAuthorizer *self);
static void notify_timeout_cb (GObject *gobject, GParamSpec *pspec, GObject *self);

struct _GDataClientLoginAuthorizerPrivate {
	SoupSession *session;
	SoupURI *proxy_uri; /* cached version only set if gdata_client_login_authorizer_get_proxy_uri() is called */

	gchar *client_id;

	/* Mutex for username, password and auth_tokens. It has to be recursive as the top-level authentication functions need to hold a lock on
	 * auth_tokens while looping over it, but lower-level functions also need to modify auth_tokens to add the auth_tokens themselves once they're
	 * returned by the online service. */
	GRecMutex mutex;

	gchar *username;
	GDataSecureString password; /* must be allocated by _gdata_service_secure_strdup() */

	/* Mapping from GDataAuthorizationDomain to string? auth_token; auth_token is NULL for domains which aren't authorised at the moment */
	GHashTable *auth_tokens;
};

enum {
	PROP_CLIENT_ID = 1,
	PROP_USERNAME,
	PROP_PASSWORD,
	PROP_PROXY_URI,
	PROP_TIMEOUT,
};

enum {
	SIGNAL_CAPTCHA_CHALLENGE,
	LAST_SIGNAL
};

static guint authorizer_signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE_WITH_CODE (GDataClientLoginAuthorizer, gdata_client_login_authorizer, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_AUTHORIZER, authorizer_init))

static void
gdata_client_login_authorizer_class_init (GDataClientLoginAuthorizerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataClientLoginAuthorizerPrivate));

	gobject_class->get_property = get_property;
	gobject_class->set_property = set_property;
	gobject_class->dispose = dispose;
	gobject_class->finalize = finalize;

	/**
	 * GDataClientLoginAuthorizer:client-id:
	 *
	 * A client ID for your application (see the
	 * <ulink url="http://code.google.com/apis/accounts/docs/AuthForInstalledApps.html#Request" type="http">reference documentation</ulink>).
	 *
	 * It is recommended that the ID is of the form <literal><replaceable>company name</replaceable>-<replaceable>application name</replaceable>-
	 * <replaceable>version ID</replaceable></literal>.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_CLIENT_ID,
	                                 g_param_spec_string ("client-id",
	                                                      "Client ID", "A client ID for your application.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataClientLoginAuthorizer:username:
	 *
	 * The user's Google username for authentication. This will always be a full e-mail address.
	 *
	 * This will only be set after authentication using gdata_client_login_authorizer_authenticate() is completed successfully. It will
	 * then be set to the username passed to gdata_client_login_authorizer_authenticate(), and a #GObject::notify signal will be emitted. If
	 * authentication fails, it will be set to %NULL.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_USERNAME,
	                                 g_param_spec_string ("username",
	                                                      "Username", "The user's Google username for authentication.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataClientLoginAuthorizer:password:
	 *
	 * The user's account password for authentication.
	 *
	 * This will only be set after authentication using gdata_client_login_authorizer_authenticate() is completed successfully. It will
	 * then be set to the password passed to gdata_client_login_authorizer_authenticate(), and a #GObject::notify signal will be emitted. If
	 * authentication fails, it will be set to %NULL.
	 *
	 * If libgdata is compiled with libgcr support, the password will be stored in non-pageable memory. However, if it is retrieved
	 * using g_object_get() (or related functions) it will be copied to non-pageable memory and could end up being written to disk. Accessing
	 * the password using gdata_client_login_authorizer_get_password() will not perform any copies, and so maintains privacy.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_PASSWORD,
	                                 g_param_spec_string ("password",
	                                                      "Password", "The user's account password for authentication.",
	                                                      NULL,
	                                                      G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataClientLoginAuthorizer:proxy-uri:
	 *
	 * The proxy URI used internally for all network requests.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_PROXY_URI,
	                                 g_param_spec_boxed ("proxy-uri",
	                                                     "Proxy URI", "The proxy URI used internally for all network requests.",
	                                                     SOUP_TYPE_URI,
	                                                     G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataClientLoginAuthorizer:timeout:
	 *
	 * A timeout, in seconds, for network operations. If the timeout is exceeded, the operation will be cancelled and
	 * %GDATA_SERVICE_ERROR_NETWORK_ERROR will be returned.
	 *
	 * If the timeout is <code class="literal">0</code>, operations will never time out.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_TIMEOUT,
	                                 g_param_spec_uint ("timeout",
	                                                    "Timeout", "A timeout, in seconds, for network operations.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataClientLoginAuthorizer::captcha-challenge:
	 * @authorizer: the #GDataClientLoginAuthorizer which received the challenge
	 * @uri: the URI of the CAPTCHA image to be used
	 *
	 * The #GDataClientLoginAuthorizer::captcha-challenge signal is emitted during the authentication process if the authorizer requires a CAPTCHA
	 * to be completed. The URI of a CAPTCHA image is given, and the program should display this to the user, and return their response (the text
	 * displayed in the image). There is no timeout imposed by the library for the response.
	 *
	 * Return value: a newly allocated string containing the text in the CAPTCHA image
	 *
	 * Since: 0.9.0
	 */
	authorizer_signals[SIGNAL_CAPTCHA_CHALLENGE] = g_signal_new ("captcha-challenge",
	                                                             G_TYPE_FROM_CLASS (klass),
	                                                             G_SIGNAL_RUN_LAST,
	                                                             0, NULL, NULL,
	                                                             gdata_marshal_STRING__OBJECT_STRING,
	                                                             G_TYPE_STRING, 1, G_TYPE_STRING);
}

static void
authorizer_init (GDataAuthorizerInterface *iface)
{
	iface->process_request = process_request;
	iface->is_authorized_for_domain = is_authorized_for_domain;
}

static void
gdata_client_login_authorizer_init (GDataClientLoginAuthorizer *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_CLIENT_LOGIN_AUTHORIZER, GDataClientLoginAuthorizerPrivate);

	/* Set up the authentication mutex */
	g_rec_mutex_init (&(self->priv->mutex));
	self->priv->auth_tokens = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, (GDestroyNotify) _gdata_service_secure_strfree);

	/* Set up the session */
	self->priv->session = _gdata_service_build_session ();

	/* Proxy the SoupSession's proxy-uri and timeout properties */
	g_signal_connect (self->priv->session, "notify::proxy-uri", (GCallback) notify_proxy_uri_cb, self);
	g_signal_connect (self->priv->session, "notify::timeout", (GCallback) notify_timeout_cb, self);
}

static void
dispose (GObject *object)
{
	GDataClientLoginAuthorizerPrivate *priv = GDATA_CLIENT_LOGIN_AUTHORIZER (object)->priv;

	if (priv->session != NULL) {
		g_object_unref (priv->session);
	}
	priv->session = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_client_login_authorizer_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
	GDataClientLoginAuthorizerPrivate *priv = GDATA_CLIENT_LOGIN_AUTHORIZER (object)->priv;

	g_free (priv->username);
	_gdata_service_secure_strfree (priv->password);
	g_free (priv->client_id);
	g_hash_table_destroy (priv->auth_tokens);
	g_rec_mutex_clear (&(priv->mutex));

	if (priv->proxy_uri != NULL) {
		soup_uri_free (priv->proxy_uri);
	}

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_client_login_authorizer_parent_class)->finalize (object);
}

static void
get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataClientLoginAuthorizerPrivate *priv = GDATA_CLIENT_LOGIN_AUTHORIZER (object)->priv;

	switch (property_id) {
		case PROP_CLIENT_ID:
			g_value_set_string (value, priv->client_id);
			break;
		case PROP_USERNAME:
			g_rec_mutex_lock (&(priv->mutex));
			g_value_set_string (value, priv->username);
			g_rec_mutex_unlock (&(priv->mutex));
			break;
		case PROP_PASSWORD:
			/* NOTE: This takes a pageable copy of non-pageable memory and thus could result in the password hitting disk. */
			g_rec_mutex_lock (&(priv->mutex));
			g_value_set_string (value, priv->password);
			g_rec_mutex_unlock (&(priv->mutex));
			break;
		case PROP_PROXY_URI:
			g_value_set_boxed (value, gdata_client_login_authorizer_get_proxy_uri (GDATA_CLIENT_LOGIN_AUTHORIZER (object)));
			break;
		case PROP_TIMEOUT:
			g_value_set_uint (value, gdata_client_login_authorizer_get_timeout (GDATA_CLIENT_LOGIN_AUTHORIZER (object)));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataClientLoginAuthorizerPrivate *priv = GDATA_CLIENT_LOGIN_AUTHORIZER (object)->priv;

	switch (property_id) {
		case PROP_CLIENT_ID:
			priv->client_id = g_value_dup_string (value);
			break;
		case PROP_PROXY_URI:
			gdata_client_login_authorizer_set_proxy_uri (GDATA_CLIENT_LOGIN_AUTHORIZER (object), g_value_get_boxed (value));
			break;
		case PROP_TIMEOUT:
			gdata_client_login_authorizer_set_timeout (GDATA_CLIENT_LOGIN_AUTHORIZER (object), g_value_get_uint (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
process_request (GDataAuthorizer *self, GDataAuthorizationDomain *domain, SoupMessage *message)
{
	GDataConstSecureString auth_token; /* privacy sensitive */
	GDataClientLoginAuthorizerPrivate *priv = GDATA_CLIENT_LOGIN_AUTHORIZER (self)->priv;

	/* If the domain's NULL, return immediately */
	if (domain == NULL) {
		return;
	}

	/* Set the authorisation header */
	g_rec_mutex_lock (&(priv->mutex));

	auth_token = (GDataConstSecureString) g_hash_table_lookup (priv->auth_tokens, domain);

	if (auth_token != NULL) {
		/* Ensure that we're using HTTPS: if not, we shouldn't set the Authorization header or we could be revealing the auth token to
		 * anyone snooping the connection, which would give them the same rights as us on the user's data. Generally a bad thing to happen. */
		if (soup_message_get_uri (message)->scheme != SOUP_URI_SCHEME_HTTPS) {
			g_warning ("Not authorizing a non-HTTPS message with the user's ClientLogin auth token as the connection isn't secure.");
		} else {
			/* Ideally, authorisation_header would be allocated in non-pageable memory. However, it's copied by
			 * soup_message_headers_replace() immediately anyway, so there's not much point. However, we do ensure we zero it out before
			 * freeing. */
			gchar *authorisation_header = g_strdup_printf ("GoogleLogin auth=%s", auth_token);
			soup_message_headers_replace (message->request_headers, "Authorization", authorisation_header);
			memset (authorisation_header, 0, strlen (authorisation_header));
			g_free (authorisation_header);
		}
	}

	g_rec_mutex_unlock (&(priv->mutex));
}

static gboolean
is_authorized_for_domain (GDataAuthorizer *self, GDataAuthorizationDomain *domain)
{
	GDataClientLoginAuthorizerPrivate *priv = GDATA_CLIENT_LOGIN_AUTHORIZER (self)->priv;
	gpointer result;

	g_rec_mutex_lock (&(priv->mutex));
	result = g_hash_table_lookup (priv->auth_tokens, domain);
	g_rec_mutex_unlock (&(priv->mutex));

	return (result != NULL) ? TRUE : FALSE;
}

/**
 * gdata_client_login_authorizer_new:
 * @client_id: your application's client ID
 * @service_type: the #GType of a #GDataService subclass which the #GDataClientLoginAuthorizer will be used with
 *
 * Creates a new #GDataClientLoginAuthorizer. The @client_id must be unique for your application, and as registered with Google.
 *
 * The #GDataAuthorizationDomain<!-- -->s for the given @service_type (i.e. as returned by gdata_service_get_authorization_domains()) are the ones the
 * user will be logged in to using the provided username and password when gdata_client_login_authorizer_authenticate() is called. Note that the same
 * username and password will be used for all domains.
 *
 * Return value: (transfer full): a new #GDataClientLoginAuthorizer, or %NULL; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GDataClientLoginAuthorizer *
gdata_client_login_authorizer_new (const gchar *client_id, GType service_type)
{
	g_return_val_if_fail (client_id != NULL && *client_id != '\0', NULL);
	g_return_val_if_fail (g_type_is_a (service_type, GDATA_TYPE_SERVICE), NULL);

	return gdata_client_login_authorizer_new_for_authorization_domains (client_id,
	                                                                    gdata_service_get_authorization_domains (service_type));
}

/**
 * gdata_client_login_authorizer_new_for_authorization_domains:
 * @client_id: your application's client ID
 * @authorization_domains: (element-type GDataAuthorizationDomain) (transfer none): a non-empty list of #GDataAuthorizationDomain<!-- -->s to be
 * authorized against by the #GDataClientLoginAuthorizer
 *
 * Creates a new #GDataClientLoginAuthorizer. The @client_id must be unique for your application, and as registered with Google. This function is
 * intended to be used only when the default authorization domain list for a single #GDataService, as used by gdata_client_login_authorizer_new(),
 * isn't suitable. For example, this could be because the #GDataClientLoginAuthorizer will be used with multiple #GDataService subclasses, or because
 * the client requires a specific set of authorization domains.
 *
 * The specified #GDataAuthorizationDomain<!-- -->s are the ones the user will be logged in to using the provided username and password when
 * gdata_client_login_authorizer_authenticate() is called. Note that the same username and password will be used for all domains.
 *
 * Return value: (transfer full): a new #GDataClientLoginAuthorizer, or %NULL; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GDataClientLoginAuthorizer *
gdata_client_login_authorizer_new_for_authorization_domains (const gchar *client_id, GList *authorization_domains)
{
	GList *i;
	GDataClientLoginAuthorizer *authorizer;

	g_return_val_if_fail (client_id != NULL && *client_id != '\0', NULL);
	g_return_val_if_fail (authorization_domains != NULL, NULL);

	authorizer = GDATA_CLIENT_LOGIN_AUTHORIZER (g_object_new (GDATA_TYPE_CLIENT_LOGIN_AUTHORIZER,
	                                                          "client-id", client_id,
	                                                          NULL));

	/* Register all the domains with the authorizer */
	for (i = authorization_domains; i != NULL; i = i->next) {
		g_return_val_if_fail (GDATA_IS_AUTHORIZATION_DOMAIN (i->data), NULL);

		/* We don't have to lock the authoriser's mutex here as no other code has seen the authoriser yet */
		g_hash_table_insert (authorizer->priv->auth_tokens, g_object_ref (GDATA_AUTHORIZATION_DOMAIN (i->data)), NULL);
	}

	return authorizer;
}

/* Called in the main thread to notify of changes to the username and password properties from the authentication thread. It swallows a reference
 * the authoriser. */
static gboolean
notify_authentication_details_cb (GDataClientLoginAuthorizer *self)
{
	GObject *authorizer = G_OBJECT (self);

	g_object_freeze_notify (authorizer);
	g_object_notify (authorizer, "username");
	g_object_notify (authorizer, "password");
	g_object_thaw_notify (authorizer);

	g_object_unref (self);

	/* Only execute once */
	return FALSE;
}

static void
set_authentication_details (GDataClientLoginAuthorizer *self, const gchar *username, const gchar *password, GHashTable *new_auth_tokens,
                            gboolean is_async)
{
	GDataClientLoginAuthorizerPrivate *priv = self->priv;
	GHashTableIter iter;

	g_rec_mutex_lock (&(priv->mutex));

	/* Ensure the username is always a full e-mail address */
	g_free (priv->username);
	if (username != NULL && strchr (username, '@') == NULL) {
		priv->username = g_strdup_printf ("%s@" EMAIL_DOMAIN, username);
	} else {
		priv->username = g_strdup (username);
	}

	_gdata_service_secure_strfree (priv->password);
	priv->password = _gdata_service_secure_strdup (password);

	/* Transfer all successful auth. tokens to the object-wide auth. token store. */
	if (new_auth_tokens == NULL) {
		/* Reset ->auth_tokens to contain no auth. tokens, just the domains. */
		g_hash_table_iter_init (&iter, priv->auth_tokens);

		while (g_hash_table_iter_next (&iter, NULL, NULL) == TRUE) {
			g_hash_table_iter_replace (&iter, NULL);
		}
	} else {
		/* Replace the existing ->auth_tokens with the new one, which contains all the shiny new auth. tokens. */
		g_hash_table_ref (new_auth_tokens);
		g_hash_table_unref (priv->auth_tokens);
		priv->auth_tokens = new_auth_tokens;
	}

	g_rec_mutex_unlock (&(priv->mutex));

	/* Notify of the property changes in the main thread; i.e. if we're running an async operation, schedule the notification in an idle
	 * callback; but if we're running a sync operation, emit them immediately.
	 * This guarantees that:
	 *  • notifications will always be emitted before gdata_client_login_authorizer_authenticate() returns; and
	 *  • notifications will always be emitted in the main thread for calls to gdata_client_login_authorizer_authenticate_async(). */
	if (is_async == TRUE) {
		g_idle_add ((GSourceFunc) notify_authentication_details_cb, g_object_ref (self));
	} else {
		notify_authentication_details_cb (g_object_ref (self));
	}
}

static GDataSecureString
parse_authentication_response (GDataClientLoginAuthorizer *self, GDataAuthorizationDomain *domain, guint status,
                               const gchar *response_body, gint length, GError **error)
{
	gchar *auth_start, *auth_end;
	GDataSecureString auth_token; /* NOTE: auth_token must be allocated using _gdata_service_secure_strdup() and friends */

	/* Parse the response */
	auth_start = strstr (response_body, "Auth=");
	if (auth_start == NULL) {
		goto protocol_error;
	}
	auth_start += strlen ("Auth=");

	auth_end = strstr (auth_start, "\n");
	if (auth_end == NULL) {
		goto protocol_error;
	}

	auth_token = _gdata_service_secure_strndup (auth_start, auth_end - auth_start);
	if (auth_token == NULL || strlen (auth_token) == 0) {
		_gdata_service_secure_strfree (auth_token);
		goto protocol_error;
	}

	return auth_token;

protocol_error:
	g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
	                     _("The server returned a malformed response."));
	return NULL;
}

static void
parse_error_response (GDataClientLoginAuthorizer *self, guint status, const gchar *reason_phrase, const gchar *response_body, gint length,
                      GError **error)
{
	/* We prefer to include the @response_body in the error message, but if it's empty, fall back to the @reason_phrase */
	if (response_body == NULL || *response_body == '\0') {
		response_body = reason_phrase;
	}

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
			/* We'll fall back to a generic error, below */
			break;
	}

	/* If the error hasn't been handled already, throw a generic error */
	g_set_error (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
	             /* Translators: the first parameter is an HTTP status,
	              * and the second is an error message returned by the server. */
	             _("Error code %u when authenticating: %s"), status, response_body);
}

static GDataSecureString
authenticate (GDataClientLoginAuthorizer *self, GDataAuthorizationDomain *domain, const gchar *username, const gchar *password,
              gchar *captcha_token, gchar *captcha_answer, GCancellable *cancellable, GError **error)
{
	GDataClientLoginAuthorizerPrivate *priv = self->priv;
	SoupMessage *message;
	gchar *request_body;
	const gchar *service_name;
	guint status;
	GDataSecureString auth_token;
	SoupURI *_uri;

	/* Prepare the request.
	 * NOTE: At this point, our non-pageable password is copied into a pageable HTTP request structure. We can't do much about this
	 * except note that the request is transient and so the chance of it getting paged out is low (but still positive). */
	service_name = gdata_authorization_domain_get_service_name (domain);
	request_body = soup_form_encode ("accountType", "HOSTED_OR_GOOGLE",
	                                 "Email", username,
	                                 "Passwd", password,
	                                 "service", service_name,
	                                 "source", priv->client_id,
	                                 (captcha_token == NULL) ? NULL : "logintoken", captcha_token,
	                                 "loginanswer", captcha_answer,
	                                 NULL);

	/* Free the CAPTCHA token and answer if necessary */
	g_free (captcha_token);
	g_free (captcha_answer);

	/* Build the message */
	_uri = soup_uri_new ("https://www.google.com/accounts/ClientLogin");
	soup_uri_set_port (_uri, _gdata_service_get_https_port ());
	message = soup_message_new_from_uri (SOUP_METHOD_POST, _uri);
	soup_uri_free (_uri);
	soup_message_set_request (message, "application/x-www-form-urlencoded", SOUP_MEMORY_TAKE, request_body, strlen (request_body));

	/* Send the message */
	_gdata_service_actually_send_message (priv->session, message, cancellable, error);
	status = message->status_code;

	if (status == SOUP_STATUS_CANCELLED) {
		/* Cancelled (the error has already been set) */
		g_object_unref (message);
		return NULL;
	} else if (status != SOUP_STATUS_OK) {
		const gchar *response_body = message->response_body->data;
		gchar *error_start, *error_end, *uri_start, *uri_end, *uri = NULL;

		/* Parse the error response; see: http://code.google.com/apis/accounts/docs/AuthForInstalledApps.html#Errors */
		if (response_body == NULL) {
			goto protocol_error;
		}

		/* Error */
		error_start = strstr (response_body, "Error=");
		if (error_start == NULL) {
			goto protocol_error;
		}
		error_start += strlen ("Error=");

		error_end = strstr (error_start, "\n");
		if (error_end == NULL) {
			goto protocol_error;
		}

		if (strncmp (error_start, "CaptchaRequired", error_end - error_start) == 0) {
			const gchar *captcha_base_uri = "http://www.google.com/accounts/";
			gchar *captcha_start, *captcha_end, *captcha_uri, *new_captcha_answer;
			guint captcha_base_uri_length;

			/* CAPTCHA required to log in */
			captcha_start = strstr (response_body, "CaptchaUrl=");
			if (captcha_start == NULL) {
				goto protocol_error;
			}
			captcha_start += strlen ("CaptchaUrl=");

			captcha_end = strstr (captcha_start, "\n");
			if (captcha_end == NULL) {
				goto protocol_error;
			}

			/* Do some fancy memory stuff to save ourselves another alloc */
			captcha_base_uri_length = strlen (captcha_base_uri);
			captcha_uri = g_malloc (captcha_base_uri_length + (captcha_end - captcha_start) + 1);
			memcpy (captcha_uri, captcha_base_uri, captcha_base_uri_length);
			memcpy (captcha_uri + captcha_base_uri_length, captcha_start, (captcha_end - captcha_start));
			captcha_uri[captcha_base_uri_length + (captcha_end - captcha_start)] = '\0';

			/* Request a CAPTCHA answer from the application */
			g_signal_emit (self, authorizer_signals[SIGNAL_CAPTCHA_CHALLENGE], 0, captcha_uri, &new_captcha_answer);
			g_free (captcha_uri);

			if (new_captcha_answer == NULL || *new_captcha_answer == '\0') {
				/* Translators: see http://en.wikipedia.org/wiki/CAPTCHA for information about CAPTCHAs */
				g_set_error_literal (error, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_CAPTCHA_REQUIRED,
				                     _("A CAPTCHA must be filled out to log in."));
				goto login_error;
			}

			/* Get the CAPTCHA token */
			captcha_start = strstr (response_body, "CaptchaToken=");
			if (captcha_start == NULL) {
				goto protocol_error;
			}
			captcha_start += strlen ("CaptchaToken=");

			captcha_end = strstr (captcha_start, "\n");
			if (captcha_end == NULL) {
				goto protocol_error;
			}

			/* Save the CAPTCHA token and answer, and attempt to log in with them */
			g_object_unref (message);

			return authenticate (self, domain, username, password,
			                     g_strndup (captcha_start, captcha_end - captcha_start), new_captcha_answer,
			                     cancellable, error);
		} else if (strncmp (error_start, "Unknown", error_end - error_start) == 0) {
			goto protocol_error;
		} else if (strncmp (error_start, "BadAuthentication", error_end - error_start) == 0) {
			/* Looks like Error=BadAuthentication errors don't return a URI */
			gchar *info_start, *info_end;

			info_start = strstr (response_body, "Info=");
			if (info_start != NULL) {
				info_start += strlen ("Info=");
				info_end = strstr (info_start, "\n");
			}

			/* If Info=InvalidSecondFactor, the user needs to generate an application-specific password and use that instead */
			if (info_start != NULL && info_end != NULL && strncmp (info_start, "InvalidSecondFactor", info_end - info_start) == 0) {
				g_set_error (error, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_INVALID_SECOND_FACTOR,
				             /* Translators: the parameter is a URI for further information. */
				             _("This account requires an application-specific password. (%s)"),
				             "http://www.google.com/support/accounts/bin/static.py?page=guide.cs&guide=1056283&topic=1056286");
				goto login_error;
			}

			/* Fall back to a generic "bad authentication details" message */
			g_set_error_literal (error, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_BAD_AUTHENTICATION,
			                     _("Your username or password were incorrect."));
			goto login_error;
		}

		/* Get the information URI */
		uri_start = strstr (response_body, "Url=");
		if (uri_start == NULL) {
			goto protocol_error;
		}
		uri_start += strlen ("Url=");

		uri_end = strstr (uri_start, "\n");
		if (uri_end == NULL) {
			goto protocol_error;
		}

		uri = g_strndup (uri_start, uri_end - uri_start);

		if (strncmp (error_start, "NotVerified", error_end - error_start) == 0) {
			g_set_error (error, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_NOT_VERIFIED,
			             /* Translators: the parameter is a URI for further information. */
			             _("Your account's e-mail address has not been verified. (%s)"), uri);
			goto login_error;
		} else if (strncmp (error_start, "TermsNotAgreed", error_end - error_start) == 0) {
			g_set_error (error, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_TERMS_NOT_AGREED,
			             /* Translators: the parameter is a URI for further information. */
			             _("You have not agreed to the service's terms and conditions. (%s)"), uri);
			goto login_error;
		} else if (strncmp (error_start, "AccountMigrated", error_end - error_start) == 0) {
			/* This is non-standard, and used by YouTube since it's got messed-up accounts */
			g_set_error (error, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_ACCOUNT_MIGRATED,
			             /* Translators: the parameter is a URI for further information. */
			             _("This account has been migrated. Please log in online to receive your new username and password. (%s)"), uri);
			goto login_error;
		} else if (strncmp (error_start, "AccountDeleted", error_end - error_start) == 0) {
			g_set_error (error, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_ACCOUNT_DELETED,
			             /* Translators: the parameter is a URI for further information. */
			             _("This account has been deleted. (%s)"), uri);
			goto login_error;
		} else if (strncmp (error_start, "AccountDisabled", error_end - error_start) == 0) {
			g_set_error (error, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_ACCOUNT_DISABLED,
			             /* Translators: the parameter is a URI for further information. */
			             _("This account has been disabled. (%s)"), uri);
			goto login_error;
		} else if (strncmp (error_start, "ServiceDisabled", error_end - error_start) == 0) {
			g_set_error (error, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR, GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_SERVICE_DISABLED,
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

		return NULL;
	}

	g_assert (message->response_body->data != NULL);

	auth_token = parse_authentication_response (self, domain, status, message->response_body->data, message->response_body->length, error);

	/* Zero out the response body to lower the chance of it (with all the juicy passwords and auth. tokens it contains) hitting disk or getting
	 * leaked in free memory. */
	memset ((void*) message->response_body->data, 0, message->response_body->length);

	g_object_unref (message);

	return auth_token;

protocol_error:
	parse_error_response (self, status, message->reason_phrase, message->response_body->data, message->response_body->length, error);

	g_object_unref (message);

	return NULL;
}

static gboolean
authenticate_loop (GDataClientLoginAuthorizer *authorizer, gboolean is_async, const gchar *username, const gchar *password, GCancellable *cancellable,
                   GError **error)
{
	GDataClientLoginAuthorizerPrivate *priv = authorizer->priv;
	gboolean cumulative_success = TRUE;
	GHashTable *new_auth_tokens;
	GHashTableIter iter;
	GDataAuthorizationDomain *domain;
	GDataSecureString auth_token;

	g_rec_mutex_lock (&(priv->mutex));

	/* Authenticate and authorize against each of the services registered with the authorizer */
	new_auth_tokens = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, (GDestroyNotify) _gdata_service_secure_strfree);
	g_hash_table_iter_init (&iter, priv->auth_tokens);

	while (g_hash_table_iter_next (&iter, (gpointer*) &domain, NULL) == TRUE) {
		GError *authenticate_error = NULL;

		auth_token = authenticate (authorizer, domain, username, password, NULL, NULL, cancellable, &authenticate_error);

		if (auth_token == NULL && cumulative_success == TRUE) {
			/* Only propagate the first error which occurs. */
			g_propagate_error (error, authenticate_error);
			authenticate_error = NULL;
		}

		cumulative_success = (auth_token != NULL) && cumulative_success;

		/* Store the auth. token (or lack thereof if authentication failed). */
		g_hash_table_insert (new_auth_tokens, g_object_ref (domain), auth_token);

		g_clear_error (&authenticate_error);
	}

	g_rec_mutex_unlock (&(priv->mutex));

	/* Set or clear the authentication details and return now that we're done */
	if (cumulative_success == TRUE) {
		set_authentication_details (authorizer, username, password, new_auth_tokens, is_async);
	} else {
		set_authentication_details (authorizer, NULL, NULL, NULL, is_async);
	}

	g_hash_table_unref (new_auth_tokens);

	return cumulative_success;
}

typedef struct {
	gchar *username;
	GDataSecureString password; /* NOTE: This must be allocated in non-pageable memory using _gdata_service_secure_strdup(). */
} AuthenticateAsyncData;

static void
authenticate_async_data_free (AuthenticateAsyncData *self)
{
	g_free (self->username);
	_gdata_service_secure_strfree (self->password);

	g_slice_free (AuthenticateAsyncData, self);
}

static void
authenticate_thread (GSimpleAsyncResult *result, GDataClientLoginAuthorizer *authorizer, GCancellable *cancellable)
{
	GError *error = NULL;
	gboolean success;
	AuthenticateAsyncData *data = g_simple_async_result_get_op_res_gpointer (result);

	success = authenticate_loop (authorizer, TRUE, data->username, data->password, cancellable, &error);

	g_simple_async_result_set_op_res_gboolean (result, success);

	if (success == FALSE) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
	}
}

/**
 * gdata_client_login_authorizer_authenticate_async:
 * @self: a #GDataClientLoginAuthorizer
 * @username: the user's username
 * @password: the user's password
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when authentication is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Authenticates the #GDataClientLoginAuthorizer with the Google accounts service using the given @username and @password. @self, @username and
 * @password are all reffed/copied when this function is called, so can safely be freed after this function returns.
 *
 * For more details, see gdata_client_login_authorizer_authenticate(), which is the synchronous version of this function.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_client_login_authorizer_authenticate_finish()
 * to get the results of the operation.
 *
 * Since: 0.9.0
 */
void
gdata_client_login_authorizer_authenticate_async (GDataClientLoginAuthorizer *self, const gchar *username, const gchar *password,
                                                  GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *result;
	AuthenticateAsyncData *data;

	g_return_if_fail (GDATA_IS_CLIENT_LOGIN_AUTHORIZER (self));
	g_return_if_fail (username != NULL);
	g_return_if_fail (password != NULL);
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	data = g_slice_new (AuthenticateAsyncData);
	data->username = g_strdup (username);
	data->password = _gdata_service_secure_strdup (password);

	result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_client_login_authorizer_authenticate_async);
	g_simple_async_result_set_handle_cancellation (result, FALSE); /* we handle our own cancellation so we can set ::username and ::password */
	g_simple_async_result_set_op_res_gpointer (result, data, (GDestroyNotify) authenticate_async_data_free);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) authenticate_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
}

/**
 * gdata_client_login_authorizer_authenticate_finish:
 * @self: a #GDataClientLoginAuthorizer
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous authentication operation started with gdata_client_login_authorizer_authenticate_async().
 *
 * Return value: %TRUE if authentication was successful, %FALSE otherwise
 *
 * Since: 0.9.0
 */
gboolean
gdata_client_login_authorizer_authenticate_finish (GDataClientLoginAuthorizer *self, GAsyncResult *async_result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_CLIENT_LOGIN_AUTHORIZER (self), FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_warn_if_fail (g_simple_async_result_is_valid (async_result, G_OBJECT (self), gdata_client_login_authorizer_authenticate_async));

	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (async_result), error) == TRUE) {
		return FALSE;
	}

	return TRUE;
}

/**
 * gdata_client_login_authorizer_authenticate:
 * @self: a #GDataClientLoginAuthorizer
 * @username: the user's username
 * @password: the user's password
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Authenticates the #GDataClientLoginAuthorizer with the Google Accounts service using @username and @password and authorizes it against all the
 * service types passed to gdata_client_login_authorizer_new(); i.e. logs into the service with the given user account. @username should be a full
 * e-mail address (e.g. <literal>john.smith\@gmail.com</literal>). If a full e-mail address is not given, @username will have
 * <literal>\@gmail.com</literal> appended to create an e-mail address
 *
 * If @cancellable is not %NULL, then the operation can be cancelled by triggering the @cancellable object from another thread.
 * If the operation was cancelled, the error %G_IO_ERROR_CANCELLED will be returned.
 *
 * If the operation errors or is cancelled part-way through, gdata_authorizer_is_authorized_for_domain() is guaranteed to return %FALSE
 * for all #GDataAuthorizationDomain<!-- -->s, even if authentication has succeeded for some of them already.
 *
 * A %GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_BAD_AUTHENTICATION will be returned if authentication failed due to an incorrect username or password.
 * Other #GDataClientLoginAuthorizerError errors can be returned for other conditions.
 *
 * If the service requires a CAPTCHA to be completed, the #GDataClientLoginAuthorizer::captcha-challenge signal will be emitted.
 * The return value from a signal handler for the signal should be a newly allocated string containing the text from the image. If the text is %NULL
 * or empty, authentication will fail with a %GDATA_CLIENT_LOGIN_AUTHORIZER_ERROR_CAPTCHA_REQUIRED error. Otherwise, authentication will be
 * automatically and transparently restarted with the new CAPTCHA details.
 *
 * A %GDATA_SERVICE_ERROR_PROTOCOL_ERROR will be returned if the server's responses were invalid.
 *
 * Return value: %TRUE if authentication and authorization was successful against all the services, %FALSE otherwise
 *
 * Since: 0.9.0
 */
gboolean
gdata_client_login_authorizer_authenticate (GDataClientLoginAuthorizer *self, const gchar *username, const gchar *password,
                                            GCancellable *cancellable, GError **error)
{
	g_return_val_if_fail (GDATA_IS_CLIENT_LOGIN_AUTHORIZER (self), FALSE);
	g_return_val_if_fail (username != NULL, FALSE);
	g_return_val_if_fail (password != NULL, FALSE);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	return authenticate_loop (self, FALSE, username, password, cancellable, error);
}

static void
notify_proxy_uri_cb (GObject *object, GParamSpec *pspec, GDataClientLoginAuthorizer *self)
{
	/* Flush our cached version */
	if (self->priv->proxy_uri != NULL) {
		soup_uri_free (self->priv->proxy_uri);
		self->priv->proxy_uri = NULL;
	}

	g_object_notify (G_OBJECT (self), "proxy-uri");
}

/**
 * gdata_client_login_authorizer_get_proxy_uri:
 * @self: a #GDataClientLoginAuthorizer
 *
 * Gets the proxy URI on the #GDataClientLoginAuthorizer's #SoupSession.
 *
 * Return value: (transfer full): the proxy URI, or %NULL; free with soup_uri_free()
 *
 * Since: 0.9.0
 */
SoupURI *
gdata_client_login_authorizer_get_proxy_uri (GDataClientLoginAuthorizer *self)
{
	SoupURI *proxy_uri;

	g_return_val_if_fail (GDATA_IS_CLIENT_LOGIN_AUTHORIZER (self), NULL);

	/* If we have a cached version, return that */
	if (self->priv->proxy_uri != NULL) {
		return self->priv->proxy_uri;
	}

	g_object_get (self->priv->session, SOUP_SESSION_PROXY_URI, &proxy_uri, NULL);

	/* Update the cache; it takes ownership of the URI */
	self->priv->proxy_uri = proxy_uri;

	return proxy_uri;
}

/**
 * gdata_client_login_authorizer_set_proxy_uri:
 * @self: a #GDataClientLoginAuthorizer
 * @proxy_uri: (allow-none): the proxy URI, or %NULL
 *
 * Sets the proxy URI on the #SoupSession used internally by the #GDataClientLoginAuthorizer. This forces all requests through the given proxy.
 *
 * If @proxy_uri is %NULL, no proxy will be used.
 *
 * Since: 0.9.0
 */
void
gdata_client_login_authorizer_set_proxy_uri (GDataClientLoginAuthorizer *self, SoupURI *proxy_uri)
{
	g_return_if_fail (GDATA_IS_CLIENT_LOGIN_AUTHORIZER (self));

	g_object_set (self->priv->session, SOUP_SESSION_PROXY_URI, proxy_uri, NULL);

	/* Notification is handled in notify_proxy_uri_cb() which is called as a result of setting the property on the session */
}

static void
notify_timeout_cb (GObject *gobject, GParamSpec *pspec, GObject *self)
{
	g_object_notify (self, "timeout");
}

/**
 * gdata_client_login_authorizer_get_timeout:
 * @self: a #GDataClientLoginAuthorizer
 *
 * Gets the #GDataClientLoginAuthorizer:timeout property; the network timeout, in seconds.
 *
 * Return value: the timeout, or <code class="literal">0</code>
 *
 * Since: 0.9.0
 */
guint
gdata_client_login_authorizer_get_timeout (GDataClientLoginAuthorizer *self)
{
	guint timeout;

	g_return_val_if_fail (GDATA_IS_CLIENT_LOGIN_AUTHORIZER (self), 0);

	g_object_get (self->priv->session, SOUP_SESSION_TIMEOUT, &timeout, NULL);

	return timeout;
}

/**
 * gdata_client_login_authorizer_set_timeout:
 * @self: a #GDataClientLoginAuthorizer
 * @timeout: the timeout, or <code class="literal">0</code>
 *
 * Sets the #GDataClientLoginAuthorizer:timeout property; the network timeout, in seconds.
 *
 * If @timeout is <code class="literal">0</code>, network operations will never time out.
 *
 * Since: 0.9.0
 */
void
gdata_client_login_authorizer_set_timeout (GDataClientLoginAuthorizer *self, guint timeout)
{
	g_return_if_fail (GDATA_IS_CLIENT_LOGIN_AUTHORIZER (self));

	g_object_set (self->priv->session, SOUP_SESSION_TIMEOUT, timeout, NULL);

	/* Notification is handled in notify_proxy_uri_cb() which is called as a result of setting the property on the session */
}

/**
 * gdata_client_login_authorizer_get_client_id:
 * @self: a #GDataClientLoginAuthorizer
 *
 * Returns the authorizer's client ID, as specified on constructing the #GDataClientLoginAuthorizer.
 *
 * Return value: the authorizer's client ID
 *
 * Since: 0.9.0
 */
const gchar *
gdata_client_login_authorizer_get_client_id (GDataClientLoginAuthorizer *self)
{
	g_return_val_if_fail (GDATA_IS_CLIENT_LOGIN_AUTHORIZER (self), NULL);

	return self->priv->client_id;
}

/**
 * gdata_client_login_authorizer_get_username:
 * @self: a #GDataClientLoginAuthorizer
 *
 * Returns the username of the currently authenticated user, or %NULL if nobody is authenticated.
 *
 * It is not safe to call this while an authentication operation is ongoing.
 *
 * Return value: the username of the currently authenticated user, or %NULL
 *
 * Since: 0.9.0
 */
const gchar *
gdata_client_login_authorizer_get_username (GDataClientLoginAuthorizer *self)
{
	g_return_val_if_fail (GDATA_IS_CLIENT_LOGIN_AUTHORIZER (self), NULL);

	/* There's little point protecting this with ->mutex, as the data's meaningless if accessed during an authentication operation,
	 * and not being accessed concurrently otherwise. */
	return self->priv->username;
}

/**
 * gdata_client_login_authorizer_get_password:
 * @self: a #GDataClientLoginAuthorizer
 *
 * Returns the password of the currently authenticated user, or %NULL if nobody is authenticated.
 *
 * It is not safe to call this while an authentication operation is ongoing.
 *
 * If libgdata is compiled with libgcr support, the password will be stored in non-pageable memory. Since this function doesn't return
 * a copy of the password, the returned value is guaranteed to not hit disk. It's advised that any copies of the password made in client programs
 * also use non-pageable memory.
 *
 * Return value: the password of the currently authenticated user, or %NULL
 *
 * Since: 0.9.0
 */
const gchar *
gdata_client_login_authorizer_get_password (GDataClientLoginAuthorizer *self)
{
	g_return_val_if_fail (GDATA_IS_CLIENT_LOGIN_AUTHORIZER (self), NULL);

	/* There's little point protecting this with ->mutex, as the data's meaningless if accessed during an authentication operation,
	 * and not being accessed concurrently otherwise. */
	return self->priv->password;
}
