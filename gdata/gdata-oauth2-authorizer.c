/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2011, 2014, 2015 <philip@tecnocode.co.uk>
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
 * SECTION:gdata-oauth2-authorizer
 * @short_description: GData OAuth 2.0 authorization interface
 * @stability: Stable
 * @include: gdata/gdata-oauth2-authorizer.h
 *
 * #GDataOAuth2Authorizer provides an implementation of the #GDataAuthorizer
 * interface for authentication and authorization using the
 * <ulink type="http" url="https://developers.google.com/accounts/docs/OAuth2InstalledApp">OAuth 2.0</ulink>
 * process, which is Google’s currently preferred authentication and
 * authorization process.
 *
 * OAuth 2.0 replaces the deprecated OAuth 1.0 and ClientLogin processes. One of
 * the main reasons for this is to allow two-factor authentication to be
 * supported, by moving the authentication interface to a web page under
 * Google’s control.
 *
 * The OAuth 2.0 process as implemented by Google follows the
 * <ulink type="http" url="http://tools.ietf.org/html/rfc6749">OAuth 2.0
 * protocol as specified by IETF in RFC 6749</ulink>, with a few additions to
 * support scopes (implemented in libgdata by #GDataAuthorizationDomains),
 * locales and custom domains. Briefly, the process is initiated by building an
 * authentication URI (using gdata_oauth2_authorizer_build_authentication_uri())
 * and opening it in the user’s web browser. The user authenticates and
 * authorizes the requested scopes on Google’s website, then an authorization
 * code is returned (via #GDataOAuth2Authorizer:redirect-uri) to the
 * application, which then converts the code into an access and refresh token
 * (using gdata_oauth2_authorizer_request_authorization()). The access token is
 * then attached to all future requests to the online service, and the refresh
 * token can be used in future (with gdata_authorizer_refresh_authorization())
 * to refresh authorization after the access token expires.
 *
 * The refresh token may also be accessed as
 * #GDataOAuth2Authorizer:refresh-token and saved by the application. It may
 * later be set on a new instance of #GDataOAuth2Authorizer, and
 * gdata_authorizer_refresh_authorization_async() called to establish a new
 * access token without requiring the user to re-authenticate unless they have
 * explicitly revoked the refresh token.
 *
 * For an overview of the standard OAuth 2.0 flow, see
 * <ulink type="http" url="http://tools.ietf.org/html/rfc6749#section-1.2">RFC 6749</ulink>.
 *
 * Before an application can be authorized using OAuth 2.0, it must be
 * registered with
 * <ulink type="http" url="https://console.developers.google.com/project">Google’s
 * Developer Console</ulink>, and a client ID, client secret and redirect URI
 * retrieved. These must be built into your application, and knowledge of them
 * will allow any application to impersonate yours, so it is recommended that
 * they are kept secret (e.g. as a configure-time option).
 *
 * libgdata supports
 * <ulink type="http" url="https://developers.google.com/accounts/docs/OAuth2InstalledApp#incrementalAuth">incremental
 * authorization</ulink>, where multiple #GDataOAuth2Authorizers can be used to
 * incrementally build up authorizations against multiple scopes. Typically,
 * you should use one #GDataOAuth2Authorizer per #GDataService your application
 * uses, limit the scope of each authorizer, and enable incremental
 * authorization when calling
 * gdata_oauth2_authorizer_build_authentication_uri().
 *
 * Each access token is long lived, so reauthorization is rarely necessary with
 * #GDataOAuth2Authorizer. It is supported using
 * gdata_authorizer_refresh_authorization().
 *
 * <example>
 *	<title>Authenticating Asynchronously Using OAuth 2.0</title>
 *	<programlisting>
 *	GDataSomeService *service;
 *	GDataOAuth2Authorizer *authorizer;
 *	gchar *authentication_uri, *authorization_code;
 *
 *	/<!-- -->* Create an authorizer and authenticate and authorize the service we're using, asynchronously. *<!-- -->/
 *	authorizer = gdata_oauth2_authorizer_new ("some-client-id", "some-client-secret",
 *	                                          GDATA_OAUTH2_REDIRECT_URI_OOB, GDATA_TYPE_SOME_SERVICE);
 *	authentication_uri = gdata_oauth2_authorizer_build_authentication_uri (authorizer, NULL, FALSE);
 *
 *	/<!-- -->* (Present the page at the authentication URI to the user, either in an embedded or stand-alone web browser, and
 *	 * ask them to grant access to the application and return the code Google gives them.) *<!-- -->/
 *	authorization_code = ask_user_for_code (authentication_uri);
 *
 *	gdata_oauth2_authorizer_request_authorization_async (authorizer, authorization_code, cancellable,
 *	                                                     (GAsyncReadyCallback) request_authorization_cb, user_data);
 *
 *	g_free (authentication_uri);
 *
 *	/<!-- -->* Zero out the code before freeing. *<!-- -->/
 *	if (token_secret != NULL) {
 *		memset (authorization_code, 0, strlen (authorization_code));
 *	}
 *
 *	g_free (authorization_code);
 *
 *	/<!-- -->* Create a service object and link it with the authorizer *<!-- -->/
 *	service = gdata_some_service_new (GDATA_AUTHORIZER (authorizer));
 *
 *	static void
 *	request_authorization_cb (GDataOAuth2Authorizer *authorizer, GAsyncResult *async_result, gpointer user_data)
 *	{
 *		GError *error = NULL;
 *
 *		if (gdata_oauth2_authorizer_request_authorization_finish (authorizer, async_result, &error) == FALSE) {
 *			/<!-- -->* Notify the user of all errors except cancellation errors *<!-- -->/
 *			if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
 *				g_error ("Authorization failed: %s", error->message);
 *			}
 *
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
 * Since: 0.17.0
 */

#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "gdata-oauth2-authorizer.h"
#include "gdata-private.h"

static void authorizer_init (GDataAuthorizerInterface *iface);
static void dispose (GObject *object);
static void finalize (GObject *object);
static void get_property (GObject *object, guint property_id, GValue *value,
                          GParamSpec *pspec);
static void set_property (GObject *object, guint property_id,
                          const GValue *value, GParamSpec *pspec);

static void process_request (GDataAuthorizer *self,
                             GDataAuthorizationDomain *domain,
                             SoupMessage *message);
static void sign_message_locked (GDataOAuth2Authorizer *self,
                                 SoupMessage *message,
                                 const gchar *access_token);
static gboolean is_authorized_for_domain (GDataAuthorizer *self,
                                          GDataAuthorizationDomain *domain);
static gboolean refresh_authorization (GDataAuthorizer *self,
                                       GCancellable *cancellable,
                                       GError **error);

static void parse_grant_response (GDataOAuth2Authorizer *self, guint status,
                                  const gchar *reason_phrase,
                                  const gchar *response_body, gssize length,
                                  GError **error);
static void parse_grant_error (GDataOAuth2Authorizer *self, guint status,
                               const gchar *reason_phrase,
                               const gchar *response_body, gssize length,
                               GError **error);

static void notify_timeout_cb (GObject *gobject, GParamSpec *pspec,
                               GObject *self);

struct _GDataOAuth2AuthorizerPrivate {
	SoupSession *session;  /* owned */
	GProxyResolver *proxy_resolver;  /* owned */

	gchar *client_id;  /* owned */
	gchar *redirect_uri;  /* owned */
	gchar *client_secret;  /* owned */
	gchar *locale;  /* owned */

	/* Mutex for access_token, refresh_token and authentication_domains. */
	GMutex mutex;

	/* These are both non-NULL when authorised. refresh_token may be
	 * non-NULL if access_token is NULL and refresh_authorization() has not
	 * yet been called on this authorizer. They may be both NULL. */
	gchar *access_token;  /* owned */
	gchar *refresh_token;  /* owned */

	/* Mapping from GDataAuthorizationDomain to itself; a set of domains for
	 * which ->access_token is valid. */
	GHashTable *authentication_domains;  /* owned */
};

enum {
	PROP_CLIENT_ID = 1,
	PROP_REDIRECT_URI,
	PROP_CLIENT_SECRET,
	PROP_LOCALE,
	PROP_TIMEOUT,
	PROP_PROXY_RESOLVER,
	PROP_REFRESH_TOKEN,
};

G_DEFINE_TYPE_WITH_CODE (GDataOAuth2Authorizer, gdata_oauth2_authorizer,
                         G_TYPE_OBJECT,
                         G_ADD_PRIVATE (GDataOAuth2Authorizer)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_AUTHORIZER,
                                                authorizer_init))

static void
gdata_oauth2_authorizer_class_init (GDataOAuth2AuthorizerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->get_property = get_property;
	gobject_class->set_property = set_property;
	gobject_class->dispose = dispose;
	gobject_class->finalize = finalize;

	/**
	 * GDataOAuth2Authorizer:client-id:
	 *
	 * A client ID for your application (see the
	 * <ulink url="https://developers.google.com/accounts/docs/OAuth2InstalledApp#handlingtheresponse" type="http">reference documentation</ulink>).
	 *
	 * It is recommended that the ID is of the form
	 * <literal><replaceable>company name</replaceable>-
	 * <replaceable>application name</replaceable>-
	 * <replaceable>version ID</replaceable></literal>.
	 *
	 * Since: 0.17.0
	 */
	g_object_class_install_property (gobject_class, PROP_CLIENT_ID,
	                                 g_param_spec_string ("client-id",
	                                                      "Client ID",
	                                                      "A client ID for your application.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataOAuth2Authorizer:redirect-uri:
	 *
	 * Redirect URI to send the response from the authorisation request to.
	 * This must either be %GDATA_OAUTH2_REDIRECT_URI_OOB,
	 * %GDATA_OAUTH2_REDIRECT_URI_OOB_AUTO, or a
	 * <code>http://localhost</code> URI with any port number (optionally)
	 * specified.
	 *
	 * This URI is where the authorisation server will redirect the user
	 * after they have completed interacting with the authentication page
	 * (gdata_oauth2_authorizer_build_authentication_uri()). If it is
	 * %GDATA_OAUTH2_REDIRECT_URI_OOB, a page will be returned in the user’s
	 * browser with the authorisation code in its title and also embedded in
	 * the page for the user to copy if it is not possible to automatically
	 * extract the code from the page title. If it is
	 * %GDATA_OAUTH2_REDIRECT_URI_OOB_AUTO, a similar page will be returned
	 * with the authorisation code in its title, but without displaying the
	 * code to the user — the user will simply be asked to close the page.
	 * If it is a localhost URI, the authentication page will redirect to
	 * that URI with the authorisation code appended as a <code>code</code>
	 * query parameter. If the user denies the authentication request, the
	 * authentication page will redirect to that URI with
	 * <code>error=access_denied</code> appended as a query parameter.
	 *
	 * Note that the redirect URI used must match that registered in
	 * Google’s Developer Console for your application.
	 *
	 * See the <ulink type="http" url="https://developers.google.com/accounts/docs/OAuth2InstalledApp#choosingredirecturi">reference
	 * documentation</ulink> for details about choosing a redirect URI.
	 *
	 * Since: 0.17.0
	 */
	g_object_class_install_property (gobject_class, PROP_REDIRECT_URI,
	                                 g_param_spec_string ("redirect-uri",
	                                                      "Redirect URI",
	                                                      "Redirect URI to send the response from the authorisation request to.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataOAuth2Authorizer:client-secret:
	 *
	 * Client secret provided by Google. This is unique for each application
	 * and is accessible from Google’s Developer Console when registering
	 * an application. It must be paired with the
	 * #GDataOAuth2Authorizer:client-id.
	 *
	 * See the
	 * <ulink url="https://developers.google.com/accounts/docs/OAuth2InstalledApp#handlingtheresponse" type="http">reference
	 * documentation</ulink> for details.
	 *
	 * Since: 0.17.0
	 */
	g_object_class_install_property (gobject_class, PROP_CLIENT_SECRET,
	                                 g_param_spec_string ("client-secret",
	                                                      "Client secret",
	                                                      "Client secret provided by Google.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataOAuth2Authorizer:locale:
	 *
	 * The locale to use for network requests, in UNIX locale format.
	 * (e.g. "en_GB", "cs", "de_DE".) Use %NULL for the default "C" locale
	 * (typically "en_US").
	 *
	 * This locale will be used by the server-side software to localise the
	 * authentication and authorization pages at the URI returned by
	 * gdata_oauth2_authorizer_build_authentication_uri().
	 *
	 * The server-side behaviour is undefined if it doesn't support a given
	 * locale.
	 *
	 * Since: 0.17.0
	 */
	g_object_class_install_property (gobject_class, PROP_LOCALE,
	                                 g_param_spec_string ("locale",
	                                                      "Locale",
	                                                      "The locale to use for network requests, in UNIX locale format.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataOAuth2Authorizer:timeout:
	 *
	 * A timeout, in seconds, for network operations. If the timeout is
	 * exceeded, the operation will be cancelled and
	 * %GDATA_SERVICE_ERROR_NETWORK_ERROR will be returned.
	 *
	 * If the timeout is <code class="literal">0</code>, operations will
	 * never time out.
	 *
	 * Since: 0.17.0
	 */
	g_object_class_install_property (gobject_class, PROP_TIMEOUT,
	                                 g_param_spec_uint ("timeout",
	                                                    "Timeout",
	                                                    "A timeout, in seconds, for network operations.",
	                                                    0, G_MAXUINT, 0,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataOAuth2Authorizer:proxy-resolver:
	 *
	 * The #GProxyResolver used to determine a proxy URI.
	 *
	 * Since: 0.17.0
	 */
	g_object_class_install_property (gobject_class, PROP_PROXY_RESOLVER,
	                                 g_param_spec_object ("proxy-resolver",
	                                                      "Proxy Resolver",
	                                                      "A GProxyResolver used to determine a proxy URI.",
	                                                      G_TYPE_PROXY_RESOLVER,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataOAuth2Authorizer:refresh-token:
	 *
	 * The server provided refresh token, which can be stored and passed in
	 * to new #GDataOAuth2Authorizer instances before calling
	 * gdata_authorizer_refresh_authorization_async() to create a new
	 * short-lived access token.
	 *
	 * The refresh token is opaque data and must not be parsed.
	 *
	 * Since: 0.17.2
	 */
	g_object_class_install_property (gobject_class, PROP_REFRESH_TOKEN,
	                                 g_param_spec_string ("refresh-token",
	                                                      "Refresh Token",
	                                                      "The server provided refresh token.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
authorizer_init (GDataAuthorizerInterface *iface)
{
	iface->process_request = process_request;
	iface->is_authorized_for_domain = is_authorized_for_domain;

	/* We only implement the synchronous version, as GDataAuthorizer will
	 * automatically wrap it in a thread for the asynchronous versions if
	 * they’re not specifically implemented, which is fine for our needs. We
	 * couldn’t do any better by implementing the asynchronous versions
	 * ourselves. */
	iface->refresh_authorization = refresh_authorization;
}

static void
gdata_oauth2_authorizer_init (GDataOAuth2Authorizer *self)
{
	self->priv = gdata_oauth2_authorizer_get_instance_private (self);

	/* Set up the authorizer's mutex */
	g_mutex_init (&self->priv->mutex);
	self->priv->authentication_domains = g_hash_table_new_full (g_direct_hash,
	                                                            g_direct_equal,
	                                                            g_object_unref,
	                                                            NULL);

	/* Set up the session */
	self->priv->session = _gdata_service_build_session ();

	/* Proxy the SoupSession’s timeout property. */
	g_signal_connect (self->priv->session, "notify::timeout",
	                  (GCallback) notify_timeout_cb, self);

	/* Keep our GProxyResolver synchronized with SoupSession’s. */
	g_object_bind_property (self->priv->session, "proxy-resolver",
	                        self, "proxy-resolver",
	                        G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
dispose (GObject *object)
{
	GDataOAuth2AuthorizerPrivate *priv;

	priv = GDATA_OAUTH2_AUTHORIZER (object)->priv;

	g_clear_object (&priv->session);
	g_clear_object (&priv->proxy_resolver);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_oauth2_authorizer_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
	GDataOAuth2AuthorizerPrivate *priv;

	priv = GDATA_OAUTH2_AUTHORIZER (object)->priv;

	g_free (priv->client_id);
	g_free (priv->client_secret);
	g_free (priv->redirect_uri);
	g_free (priv->locale);

	g_free (priv->access_token);
	g_free (priv->refresh_token);

	g_hash_table_unref (priv->authentication_domains);
	g_mutex_clear (&priv->mutex);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_oauth2_authorizer_parent_class)->finalize (object);
}

static void
get_property (GObject *object, guint property_id, GValue *value,
              GParamSpec *pspec)
{
	GDataOAuth2Authorizer *self;
	GDataOAuth2AuthorizerPrivate *priv;

	self = GDATA_OAUTH2_AUTHORIZER (object);
	priv = self->priv;

	switch (property_id) {
	case PROP_CLIENT_ID:
		g_value_set_string (value, priv->client_id);
		break;
	case PROP_REDIRECT_URI:
		g_value_set_string (value, priv->redirect_uri);
		break;
	case PROP_CLIENT_SECRET:
		g_value_set_string (value, priv->client_secret);
		break;
	case PROP_LOCALE:
		g_value_set_string (value, priv->locale);
		break;
	case PROP_TIMEOUT:
		g_value_set_uint (value,
		                  gdata_oauth2_authorizer_get_timeout (self));
		break;
	case PROP_PROXY_RESOLVER:
		g_value_set_object (value,
		                    gdata_oauth2_authorizer_get_proxy_resolver (self));
		break;
	case PROP_REFRESH_TOKEN:
		g_mutex_lock (&priv->mutex);
		g_value_set_string (value, priv->refresh_token);
		g_mutex_unlock (&priv->mutex);
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
	GDataOAuth2Authorizer *self;
	GDataOAuth2AuthorizerPrivate *priv;

	self = GDATA_OAUTH2_AUTHORIZER (object);
	priv = self->priv;

	switch (property_id) {
	/* Construct only. */
	case PROP_CLIENT_ID:
		priv->client_id = g_value_dup_string (value);
		break;
	/* Construct only. */
	case PROP_REDIRECT_URI:
		priv->redirect_uri = g_value_dup_string (value);
		break;
	/* Construct only. */
	case PROP_CLIENT_SECRET:
		priv->client_secret = g_value_dup_string (value);
		break;
	case PROP_LOCALE:
		gdata_oauth2_authorizer_set_locale (self,
		                                    g_value_get_string (value));
		break;
	case PROP_TIMEOUT:
		gdata_oauth2_authorizer_set_timeout (self,
		                                     g_value_get_uint (value));
		break;
	case PROP_PROXY_RESOLVER:
		gdata_oauth2_authorizer_set_proxy_resolver (self,
		                                            g_value_get_object (value));
		break;
	case PROP_REFRESH_TOKEN:
		gdata_oauth2_authorizer_set_refresh_token (self,
		                                           g_value_get_string (value));
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}

static void
process_request (GDataAuthorizer *self, GDataAuthorizationDomain *domain,
                 SoupMessage *message)
{
	GDataOAuth2AuthorizerPrivate *priv;

	priv = GDATA_OAUTH2_AUTHORIZER (self)->priv;

	/* Set the authorisation header */
	g_mutex_lock (&priv->mutex);

	/* Sanity check */
	g_assert ((priv->access_token == NULL) ||
	          (priv->refresh_token != NULL));

	if (priv->access_token != NULL &&
	    g_hash_table_lookup (priv->authentication_domains,
	                         domain) != NULL) {
		sign_message_locked (GDATA_OAUTH2_AUTHORIZER (self), message,
		                     priv->access_token);
	}

	g_mutex_unlock (&priv->mutex);
}

static gboolean
is_authorized_for_domain (GDataAuthorizer *self,
                          GDataAuthorizationDomain *domain)
{
	GDataOAuth2AuthorizerPrivate *priv;
	gpointer result;
	const gchar *access_token;

	priv = GDATA_OAUTH2_AUTHORIZER (self)->priv;

	g_mutex_lock (&priv->mutex);
	access_token = priv->access_token;
	result = g_hash_table_lookup (priv->authentication_domains, domain);
	g_mutex_unlock (&priv->mutex);

	/* Sanity check */
	g_assert (result == NULL || result == domain);

	return (access_token != NULL && result != NULL);
}

/* Sign the message and add the Authorization header to it containing the
 * signature.
 *
 * Reference: https://developers.google.com/accounts/docs/OAuth2InstalledApp#callinganapi
 *
 * NOTE: This must be called with the mutex locked. */
static void
sign_message_locked (GDataOAuth2Authorizer *self, SoupMessage *message,
                     const gchar *access_token)
{
	SoupURI *message_uri;  /* unowned */
	gchar *auth_header = NULL;  /* owned */

	g_return_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self));
	g_return_if_fail (SOUP_IS_MESSAGE (message));
	g_return_if_fail (access_token != NULL && *access_token != '\0');

	/* Ensure that we’re using HTTPS: if not, we shouldn’t set the
	 * Authorization header or we could be revealing the access
	 * token to anyone snooping the connection, which would give
	 * them the same rights as us on the user’s data. Generally a
	 * bad thing to happen. */
	message_uri = soup_message_get_uri (message);

	if (message_uri->scheme != SOUP_URI_SCHEME_HTTPS) {
		g_warning ("Not authorizing a non-HTTPS message with the "
		           "user’s OAuth 2.0 access token as the connection "
		           "isn’t secure.");
		return;
	}

	/* Add the authorisation header. */
	auth_header = g_strdup_printf ("Bearer %s", access_token);
	soup_message_headers_append (message->request_headers,
	                             "Authorization", auth_header);
	g_free (auth_header);
}

static gboolean
refresh_authorization (GDataAuthorizer *self, GCancellable *cancellable,
                       GError **error)
{
	/* See http://code.google.com/apis/accounts/docs/OAuth2.html#IAMoreToken */
	GDataOAuth2AuthorizerPrivate *priv;
	SoupMessage *message = NULL;  /* owned */
	SoupURI *_uri = NULL;  /* owned */
	gchar *request_body;
	guint status;
	GError *child_error = NULL;

	g_return_val_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self), FALSE);

	priv = GDATA_OAUTH2_AUTHORIZER (self)->priv;

	g_mutex_lock (&priv->mutex);

	/* If we don’t have a refresh token, we can’t refresh the
	 * authorisation. Do not set @error, as we haven’t been successfully
	 * authorised previously. */
	if (priv->refresh_token == NULL) {
		g_mutex_unlock (&priv->mutex);
		return FALSE;
	}

	/* Prepare the request */
	request_body = soup_form_encode ("client_id", priv->client_id,
	                                 "client_secret", priv->client_secret,
	                                 "refresh_token", priv->refresh_token,
	                                 "grant_type", "refresh_token",
	                                 NULL);

	g_mutex_unlock (&priv->mutex);

	/* Build the message */
	_uri = soup_uri_new ("https://accounts.google.com/o/oauth2/token");
	soup_uri_set_port (_uri, _gdata_service_get_https_port ());
	message = soup_message_new_from_uri (SOUP_METHOD_POST, _uri);
	soup_uri_free (_uri);

	soup_message_set_request (message, "application/x-www-form-urlencoded",
	                          SOUP_MEMORY_TAKE, request_body,
	                          strlen (request_body));

	/* Send the message */
	_gdata_service_actually_send_message (priv->session, message,
	                                      cancellable, error);
	status = message->status_code;

	if (status == SOUP_STATUS_CANCELLED) {
		/* Cancelled (the error has already been set) */
		g_object_unref (message);
		return FALSE;
	} else if (status != SOUP_STATUS_OK) {
		parse_grant_error (GDATA_OAUTH2_AUTHORIZER (self),
		                   status, message->reason_phrase,
		                   message->response_body->data,
		                   message->response_body->length,
		                   error);
		g_object_unref (message);

		return FALSE;
	}

	g_assert (message->response_body->data != NULL);

	/* Parse and handle the response */
	parse_grant_response (GDATA_OAUTH2_AUTHORIZER (self),
	                      status, message->reason_phrase,
	                      message->response_body->data,
	                      message->response_body->length, &child_error);

	g_object_unref (message);

	if (child_error != NULL) {
		g_propagate_error (error, child_error);
		return FALSE;
	}

	return TRUE;
}

/**
 * gdata_oauth2_authorizer_new:
 * @client_id: your application’s client ID
 * @client_secret: your application’s client secret
 * @redirect_uri: authorisation redirect URI
 * @service_type: the #GType of a #GDataService subclass which the
 * #GDataOAuth2Authorizer will be used with
 *
 * Creates a new #GDataOAuth2Authorizer. The @client_id must be unique for your
 * application, and as registered with Google, and the @client_secret must be
 * paired with it.
 *
 * Return value: (transfer full): a new #GDataOAuth2Authorizer; unref with
 * g_object_unref()
 *
 * Since: 0.17.0
 */
GDataOAuth2Authorizer *
gdata_oauth2_authorizer_new (const gchar *client_id, const gchar *client_secret,
                             const gchar *redirect_uri, GType service_type)
{
	GList/*<unowned GDataAuthorizationDomain>*/ *domains;  /* owned */
	GDataOAuth2Authorizer *retval = NULL;  /* owned */

	g_return_val_if_fail (client_id != NULL && *client_id != '\0', NULL);
	g_return_val_if_fail (client_secret != NULL && *client_secret != '\0',
	                      NULL);
	g_return_val_if_fail (redirect_uri != NULL && *redirect_uri != '\0',
	                      NULL);
	g_return_val_if_fail (g_type_is_a (service_type, GDATA_TYPE_SERVICE),
	                      NULL);

	domains = gdata_service_get_authorization_domains (service_type);

	retval = gdata_oauth2_authorizer_new_for_authorization_domains (client_id,
	                                                                client_secret,
	                                                                redirect_uri,
	                                                                domains);
	g_list_free (domains);

	return retval;
}

/**
 * gdata_oauth2_authorizer_new_for_authorization_domains:
 * @client_id: your application’s client ID
 * @client_secret: your application’s client secret
 * @redirect_uri: authorisation redirect URI
 * @authorization_domains: (element-type GDataAuthorizationDomain) (transfer none):
 * a non-empty list of #GDataAuthorizationDomains to be authorized against by
 * the #GDataOAuth2Authorizer
 *
 * Creates a new #GDataOAuth2Authorizer. The @client_id must be unique for your
 * application, and as registered with Google, and the @client_secret must be
 * paired with it.
 *
 * Return value: (transfer full): a new #GDataOAuth2Authorizer; unref with
 * g_object_unref()
 *
 * Since: 0.17.0
 */
GDataOAuth2Authorizer *
gdata_oauth2_authorizer_new_for_authorization_domains (const gchar *client_id,
                                                       const gchar *client_secret,
                                                       const gchar *redirect_uri,
                                                       GList *authorization_domains)
{
	GList *i;
	GDataOAuth2Authorizer *authorizer;

	g_return_val_if_fail (client_id != NULL && *client_id != '\0', NULL);
	g_return_val_if_fail (client_secret != NULL && *client_secret != '\0',
	                      NULL);
	g_return_val_if_fail (redirect_uri != NULL && *redirect_uri != '\0',
	                      NULL);
	g_return_val_if_fail (authorization_domains != NULL, NULL);

	authorizer = GDATA_OAUTH2_AUTHORIZER (g_object_new (GDATA_TYPE_OAUTH2_AUTHORIZER,
	                                                    "client-id", client_id,
	                                                    "client-secret", client_secret,
	                                                    "redirect-uri", redirect_uri,
	                                                    NULL));

	/* Register all the domains with the authorizer */
	for (i = authorization_domains; i != NULL; i = i->next) {
		GDataAuthorizationDomain *domain;  /* unowned */

		g_return_val_if_fail (GDATA_IS_AUTHORIZATION_DOMAIN (i->data),
		                      NULL);

		/* We don’t have to lock the authoriser’s mutex here as no other
		 * code has seen the authoriser yet */
		domain = GDATA_AUTHORIZATION_DOMAIN (i->data);
		g_hash_table_insert (authorizer->priv->authentication_domains,
		                     g_object_ref (domain), domain);
	}

	return authorizer;
}

/**
 * gdata_oauth2_authorizer_build_authentication_uri:
 * @self: a #GDataOAuth2Authorizer
 * @login_hint: (nullable): optional e-mail address or sub identifier for the
 * user
 * @include_granted_scopes: %TRUE to enable incremental authorisation
 *
 * Build an authentication URI to open in the user’s web browser (or an embedded
 * browser widget). This will display an authentication page from Google,
 * including an authentication form and confirmation of the authorisation
 * domains being requested by this #GDataAuthorizer. The user will authenticate
 * in the browser, then an authorisation code will be returned via the
 * #GDataOAuth2Authorizer:redirect-uri, ready to be passed to
 * gdata_oauth2_authorizer_request_authorization().
 *
 * If @login_hint is non-%NULL, it will be passed to the server as a hint of
 * which user is attempting to authenticate, which can be used to pre-fill the
 * e-mail address box in the authentication form.
 *
 * If @include_granted_scopes is %TRUE, the authentication request will
 * automatically include all authorisation domains previously granted to this
 * user/application pair, allowing for incremental authentication — asking for
 * permissions as needed, rather than all in one large bundle at the first
 * opportunity. If @include_granted_scopes is %FALSE, incremental authentication
 * will not be enabled, and only the domains passed to the
 * #GDataOAuth2Authorizer constructor will eventually be authenticated.
 * See the
 * <ulink type="http" url="https://developers.google.com/accounts/docs/OAuth2WebServer#incrementalAuth">reference
 * documentation</ulink> for more details.
 *
 * Return value: (transfer full): the authentication URI to open in a web
 * browser; free with g_free()
 *
 * Since: 0.17.0
 */
gchar *
gdata_oauth2_authorizer_build_authentication_uri (GDataOAuth2Authorizer *self,
                                                  const gchar *login_hint,
                                                  gboolean include_granted_scopes)
{
	GDataOAuth2AuthorizerPrivate *priv;
	GString *uri = NULL;  /* owned */
	GDataAuthorizationDomain *domain;  /* unowned */
	GHashTableIter iter;
	gboolean is_first = TRUE;

	g_return_val_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self), NULL);

	priv = self->priv;

	g_mutex_lock (&priv->mutex);

	/* Build and memoise the URI.
	 *
	 * Reference: https://developers.google.com/accounts/docs/OAuth2InstalledApp#formingtheurl
	 */
	g_assert (g_hash_table_size (priv->authentication_domains) > 0);

	uri = g_string_new ("https://accounts.google.com/o/oauth2/auth"
	                    "?response_type=code"
	                    "&client_id=");
	g_string_append_uri_escaped (uri, priv->client_id, NULL, TRUE);
	g_string_append (uri, "&redirect_uri=");
	g_string_append_uri_escaped (uri, priv->redirect_uri, NULL, TRUE);
	g_string_append (uri, "&scope=");

	/* Add the scopes of all our domains */
	g_hash_table_iter_init (&iter, priv->authentication_domains);

	while (g_hash_table_iter_next (&iter, (gpointer *) &domain, NULL)) {
		const gchar *scope;

		if (!is_first) {
			/* Delimiter */
			g_string_append (uri, "%20");
		}

		scope = gdata_authorization_domain_get_scope (domain);
		g_string_append_uri_escaped (uri, scope, NULL, TRUE);

		is_first = FALSE;
	}

	if (login_hint != NULL && *login_hint != '\0') {
		g_string_append (uri, "&login_hint=");
		g_string_append_uri_escaped (uri, login_hint, NULL, TRUE);
	}

	if (priv->locale != NULL) {
		g_string_append (uri, "&hl=");
		g_string_append_uri_escaped (uri, priv->locale, NULL, TRUE);
	}

	if (include_granted_scopes) {
		g_string_append (uri, "&include_granted_scopes=true");
	} else {
		g_string_append (uri, "&include_granted_scopes=false");
	}

	g_mutex_unlock (&priv->mutex);

	return g_string_free (uri, FALSE);
}

/* NOTE: This has to be thread safe, as it can be called from
 * refresh_authorization() at any time.
 *
 * Reference: https://developers.google.com/accounts/docs/OAuth2InstalledApp#handlingtheresponse
 */
static void
parse_grant_response (GDataOAuth2Authorizer *self, guint status,
                      const gchar *reason_phrase, const gchar *response_body,
                      gssize length, GError **error)
{
	GDataOAuth2AuthorizerPrivate *priv;
	JsonParser *parser = NULL;  /* owned */
	JsonNode *root_node;  /* unowned */
	JsonObject *root_object;  /* unowned */
	const gchar *access_token = NULL, *refresh_token = NULL;
	GError *child_error = NULL;

	priv = self->priv;

	/* Parse the successful response */
	parser = json_parser_new ();

	json_parser_load_from_data (parser, response_body, length,
	                            &child_error);

	if (child_error != NULL) {
		g_clear_error (&child_error);
		g_set_error_literal (&child_error, GDATA_SERVICE_ERROR,
		                     GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
		                     _("The server returned a malformed response."));

		goto done;
	}

	/* Extract the access token, TTL and refresh token */
	root_node = json_parser_get_root (parser);

	if (JSON_NODE_HOLDS_OBJECT (root_node) == FALSE) {
		g_set_error_literal (&child_error, GDATA_SERVICE_ERROR,
		                     GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
		                     _("The server returned a malformed response."));
		goto done;
	}

	root_object = json_node_get_object (root_node);

	if (json_object_has_member (root_object, "access_token")) {
		access_token = json_object_get_string_member (root_object,
		                                              "access_token");
	}
	if (json_object_has_member (root_object, "refresh_token")) {
		refresh_token = json_object_get_string_member (root_object,
		                                               "refresh_token");
	}

	/* Always require an access token. */
	if (access_token == NULL || *access_token == '\0') {
		g_set_error_literal (&child_error, GDATA_SERVICE_ERROR,
		                     GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
		                     _("The server returned a malformed response."));

		access_token = NULL;
		refresh_token = NULL;

		goto done;
	}

	/* Only require a refresh token if this is the first authentication.
	 * See the documentation for refreshing authentication:
	 * https://developers.google.com/accounts/docs/OAuth2InstalledApp#refresh
	 */
	if ((refresh_token == NULL || *refresh_token == '\0') &&
	    priv->refresh_token == NULL) {
		g_set_error_literal (&child_error, GDATA_SERVICE_ERROR,
		                     GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
		                     _("The server returned a malformed response."));

		access_token = NULL;
		refresh_token = NULL;

		goto done;
	}

done:
	/* Postconditions. */
	g_assert ((refresh_token == NULL) || (access_token != NULL));
	g_assert ((child_error != NULL) == (access_token == NULL));

	/* Update state. */
	g_mutex_lock (&priv->mutex);

	g_free (priv->access_token);
	priv->access_token = g_strdup (access_token);

	if (refresh_token != NULL) {
		g_free (priv->refresh_token);
		priv->refresh_token = g_strdup (refresh_token);
	}

	g_mutex_unlock (&priv->mutex);

	if (child_error != NULL) {
		g_propagate_error (error, child_error);
	}

	g_object_unref (parser);
}

/* NOTE: This has to be thread safe, as it can be called from
 * refresh_authorization() at any time.
 *
 * There is no reference for this, because Google apparently don’t deem it
 * necessary to document.
 *
 * Example response:
 *     HTTP/1.1 400 Bad Request
 *     Content-Type: application/json
 *
 *     {
 *       "error" : "invalid_grant"
 *     }
 */
static void
parse_grant_error (GDataOAuth2Authorizer *self, guint status,
                   const gchar *reason_phrase, const gchar *response_body,
                   gssize length, GError **error)
{
	JsonParser *parser = NULL;  /* owned */
	JsonNode *root_node;  /* unowned */
	JsonObject *root_object;  /* unowned */
	const gchar *error_code = NULL;
	GError *child_error = NULL;

	/* Parse the error response */
	parser = json_parser_new ();

	if (response_body == NULL) {
		g_clear_error (&child_error);
		g_set_error_literal (&child_error, GDATA_SERVICE_ERROR,
		                     GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
		                     _("The server returned a malformed response."));

		goto done;
	}

	json_parser_load_from_data (parser, response_body, length,
	                            &child_error);

	if (child_error != NULL) {
		g_clear_error (&child_error);
		g_set_error_literal (&child_error, GDATA_SERVICE_ERROR,
		                     GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
		                     _("The server returned a malformed response."));

		goto done;
	}

	/* Extract the error code. */
	root_node = json_parser_get_root (parser);

	if (JSON_NODE_HOLDS_OBJECT (root_node) == FALSE) {
		g_set_error_literal (&child_error, GDATA_SERVICE_ERROR,
		                     GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
		                     _("The server returned a malformed response."));
		goto done;
	}

	root_object = json_node_get_object (root_node);

	if (json_object_has_member (root_object, "error")) {
		error_code = json_object_get_string_member (root_object,
		                                            "error");
	}

	/* Always require an error_code. */
	if (error_code == NULL || *error_code == '\0') {
		g_set_error_literal (&child_error, GDATA_SERVICE_ERROR,
		                     GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
		                     _("The server returned a malformed response."));

		error_code = NULL;

		goto done;
	}

	/* Parse the error code. */
	if (strcmp (error_code, "invalid_grant") == 0) {
		g_set_error_literal (&child_error, GDATA_SERVICE_ERROR,
		                     GDATA_SERVICE_ERROR_FORBIDDEN,
		                     _("Access was denied by the user or server."));
	} else {
		/* Unknown error code. */
		g_set_error_literal (&child_error, GDATA_SERVICE_ERROR,
		                     GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
		                     _("The server returned a malformed response."));
	}

done:
	/* Postconditions. */
	g_assert (child_error != NULL);

	if (child_error != NULL) {
		g_propagate_error (error, child_error);
	}

	g_object_unref (parser);
}

/**
 * gdata_oauth2_authorizer_request_authorization:
 * @self: a #GDataOAuth2Authorizer
 * @authorization_code: code returned from the authentication page
 * @cancellable: (allow-none): a #GCancellable, or %NULL
 * @error: return location for a #GError, or %NULL
 *
 * Request an authorisation code from the user’s web browser is converted to
 * authorisation (access and refresh) tokens. This is the final step in the
 * authentication process; once complete, the #GDataOAuth2Authorizer should be
 * fully authorised for its domains.
 *
 * On failure, %GDATA_SERVICE_ERROR_FORBIDDEN will be returned if the user or
 * server denied the authorisation request. %GDATA_SERVICE_ERROR_PROTOCOL_ERROR
 * will be returned if the server didn’t follow the expected protocol.
 * %G_IO_ERROR_CANCELLED will be returned if the operation was cancelled using
 * @cancellable.
 *
 * Return value: %TRUE on success, %FALSE otherwise
 *
 * Since: 0.17.0
 */
gboolean
gdata_oauth2_authorizer_request_authorization (GDataOAuth2Authorizer *self,
                                               const gchar *authorization_code,
                                               GCancellable *cancellable,
                                               GError **error)
{
	GDataOAuth2AuthorizerPrivate *priv;
	SoupMessage *message = NULL;  /* owned */
	SoupURI *_uri = NULL;  /* owned */
	gchar *request_body = NULL;  /* owned */
	guint status;
	GError *child_error = NULL;

	g_return_val_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self), FALSE);
	g_return_val_if_fail (authorization_code != NULL &&
	                      *authorization_code != '\0', FALSE);
	g_return_val_if_fail (cancellable == NULL ||
	                      G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	priv = self->priv;

	/* Prepare the request.
	 *
	 * Reference: https://developers.google.com/accounts/docs/OAuth2InstalledApp#handlingtheresponse
	 */
	request_body = soup_form_encode ("client_id", priv->client_id,
	                                 "client_secret", priv->client_secret,
	                                 "code", authorization_code,
	                                 "redirect_uri", priv->redirect_uri,
	                                 "grant_type", "authorization_code",
	                                 NULL);

	/* Build the message */
	_uri = soup_uri_new ("https://accounts.google.com/o/oauth2/token");
	soup_uri_set_port (_uri, _gdata_service_get_https_port ());
	message = soup_message_new_from_uri (SOUP_METHOD_POST, _uri);
	soup_uri_free (_uri);

	soup_message_set_request (message, "application/x-www-form-urlencoded",
	                          SOUP_MEMORY_TAKE, request_body,
	                          strlen (request_body));
	request_body = NULL;

	/* Send the message */
	_gdata_service_actually_send_message (priv->session, message,
	                                      cancellable, error);
	status = message->status_code;

	if (status == SOUP_STATUS_CANCELLED) {
		/* Cancelled (the error has already been set) */
		g_object_unref (message);
		return FALSE;
	} else if (status != SOUP_STATUS_OK) {
		parse_grant_error (self, status, message->reason_phrase,
		                   message->response_body->data,
		                   message->response_body->length,
		                   error);
		g_object_unref (message);

		return FALSE;
	}

	g_assert (message->response_body->data != NULL);

	/* Parse and handle the response */
	parse_grant_response (self, status, message->reason_phrase,
	                      message->response_body->data,
	                      message->response_body->length, &child_error);

	g_object_unref (message);

	if (child_error != NULL) {
		g_propagate_error (error, child_error);
		return FALSE;
	}

	return TRUE;
}

static void
request_authorization_thread (GTask        *task,
                              gpointer      source_object,
                              gpointer      task_data,
                              GCancellable *cancellable)
{
	GDataOAuth2Authorizer *authorizer = GDATA_OAUTH2_AUTHORIZER (source_object);
	g_autoptr(GError) error = NULL;
	const gchar *authorization_code = task_data;

	if (!gdata_oauth2_authorizer_request_authorization (authorizer,
	                                                    authorization_code,
	                                                    cancellable,
	                                                    &error))
		g_task_return_error (task, g_steal_pointer (&error));
	else
		g_task_return_boolean (task, TRUE);
}

/**
 * gdata_oauth2_authorizer_request_authorization_async:
 * @self: a #GDataOAuth2Authorizer
 * @authorization_code: code returned from the authentication page
 * @cancellable: (allow-none): an optional #GCancellable, or %NULL
 * @callback: a #GAsyncReadyCallback to call when authorization is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Asynchronous version of gdata_oauth2_authorizer_request_authorization().
 *
 * Since: 0.17.0
 */
void
gdata_oauth2_authorizer_request_authorization_async (GDataOAuth2Authorizer *self,
                                                     const gchar *authorization_code,
                                                     GCancellable *cancellable,
                                                     GAsyncReadyCallback callback,
                                                     gpointer user_data)
{
	g_autoptr(GTask) task = NULL;

	g_return_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self));
	g_return_if_fail (authorization_code != NULL &&
	                  *authorization_code != '\0');
	g_return_if_fail (cancellable == NULL ||
	                  G_IS_CANCELLABLE (cancellable));

	task = g_task_new (self, cancellable, callback, user_data);
	g_task_set_source_tag (task, gdata_oauth2_authorizer_request_authorization_async);
	g_task_set_task_data (task, g_strdup (authorization_code),
	                      (GDestroyNotify) g_free);
	g_task_run_in_thread (task, request_authorization_thread);
}

/**
 * gdata_oauth2_authorizer_request_authorization_finish:
 * @self: a #GDataOAuth2Authorizer
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous authorization operation started with
 * gdata_oauth2_authorizer_request_authorization_async().
 *
 * Return value: %TRUE if authorization was successful, %FALSE otherwise
 *
 * Since: 0.17.0
 */
gboolean
gdata_oauth2_authorizer_request_authorization_finish (GDataOAuth2Authorizer *self,
                                                      GAsyncResult *async_result,
                                                      GError **error)
{
	g_return_val_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self), FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);
	g_return_val_if_fail (g_task_is_valid (async_result, self), FALSE);
	g_return_val_if_fail (g_async_result_is_tagged (async_result, gdata_oauth2_authorizer_request_authorization_async), FALSE);

	return g_task_propagate_boolean (G_TASK (async_result), error);
}

/**
 * gdata_oauth2_authorizer_get_client_id:
 * @self: a #GDataOAuth2Authorizer
 *
 * Returns the authorizer's client ID, #GDataOAuth2Authorizer:client-id, as
 * specified on constructing the #GDataOAuth2Authorizer.
 *
 * Return value: the authorizer's client ID
 *
 * Since: 0.17.0
 */
const gchar *
gdata_oauth2_authorizer_get_client_id (GDataOAuth2Authorizer *self)
{
	g_return_val_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self), NULL);
	return self->priv->client_id;
}

/**
 * gdata_oauth2_authorizer_get_redirect_uri:
 * @self: a #GDataOAuth2Authorizer
 *
 * Returns the authorizer’s redirect URI, #GDataOAuth2Authorizer:redirect-uri,
 * as specified on constructing the #GDataOAuth2Authorizer.
 *
 * Return value: the authorizer’s redirect URI
 *
 * Since: 0.17.0
 */
const gchar *
gdata_oauth2_authorizer_get_redirect_uri (GDataOAuth2Authorizer *self)
{
	g_return_val_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self), NULL);
	return self->priv->redirect_uri;
}

/**
 * gdata_oauth2_authorizer_get_client_secret:
 * @self: a #GDataOAuth2Authorizer
 *
 * Returns the authorizer's client secret, #GDataOAuth2Authorizer:client-secret,
 * as specified on constructing the #GDataOAuth2Authorizer.
 *
 * Return value: the authorizer's client secret
 *
 * Since: 0.17.0
 */
const gchar *
gdata_oauth2_authorizer_get_client_secret (GDataOAuth2Authorizer *self)
{
	g_return_val_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self), NULL);
	return self->priv->client_secret;
}

/**
 * gdata_oauth2_authorizer_dup_refresh_token:
 * @self: a #GDataOAuth2Authorizer
 *
 * Returns the authorizer's refresh token, #GDataOAuth2Authorizer:refresh-token,
 * as set by client code previously on the #GDataOAuth2Authorizer, or as
 * returned by the most recent authentication operation.
 *
 * Return value: (transfer full): the authorizer's refresh token
 *
 * Since: 0.17.2
 */
gchar *
gdata_oauth2_authorizer_dup_refresh_token (GDataOAuth2Authorizer *self)
{
	GDataOAuth2AuthorizerPrivate *priv;
	gchar *refresh_token;  /* owned */

	g_return_val_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self), NULL);

	priv = self->priv;

	g_mutex_lock (&priv->mutex);
	refresh_token = g_strdup (priv->refresh_token);
	g_mutex_unlock (&priv->mutex);

	return refresh_token;
}

/**
 * gdata_oauth2_authorizer_set_refresh_token:
 * @self: a #GDataOAuth2Authorizer
 * @refresh_token: (nullable): the new refresh token, or %NULL to clear
 *   authorization
 *
 * Sets the authorizer's refresh token, #GDataOAuth2Authorizer:refresh-token.
 * This is used to periodically refresh the access token. Set it to %NULL to
 * clear the current authentication from the authorizer.
 *
 * Since: 0.17.2
 */
void
gdata_oauth2_authorizer_set_refresh_token (GDataOAuth2Authorizer *self,
                                           const gchar *refresh_token)
{
	GDataOAuth2AuthorizerPrivate *priv;

	g_return_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self));

	priv = self->priv;

	g_mutex_lock (&priv->mutex);

	if (g_strcmp0 (priv->refresh_token, refresh_token) == 0) {
		g_mutex_unlock (&priv->mutex);
		return;
	}

	/* Clear the access token; if the refresh token has changed, it can
	 * no longer be valid, and we must avoid the situation:
	 *    (access_token != NULL) && (refresh_token == NULL) */
	g_free (priv->access_token);
	priv->access_token = NULL;

	/* Update the refresh token. */
	g_free (priv->refresh_token);
	priv->refresh_token = g_strdup (refresh_token);

	g_mutex_unlock (&priv->mutex);

	g_object_notify (G_OBJECT (self), "refresh-token");
}

/**
 * gdata_oauth2_authorizer_get_locale:
 * @self: a #GDataOAuth2Authorizer
 *
 * Returns the locale currently being used for network requests, or %NULL if the
 * locale is the default.
 *
 * Return value: (allow-none): the current locale
 *
 * Since: 0.17.0
 */
const gchar *
gdata_oauth2_authorizer_get_locale (GDataOAuth2Authorizer *self)
{
	g_return_val_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self), NULL);
	return self->priv->locale;
}

/**
 * gdata_oauth2_authorizer_set_locale:
 * @self: a #GDataOAuth2Authorizer
 * @locale: (allow-none): the new locale in UNIX locale format, or %NULL for the
 * default locale
 *
 * Set the locale used for network requests to @locale, given in standard UNIX
 * locale format. See #GDataOAuth2Authorizer:locale for more details.
 *
 * Note that while it’s possible to change the locale after sending network
 * requests (i.e. calling gdata_oauth2_authorizer_build_authentication_uri() for
 * the first time), it is unsupported, as the server-side software may behave
 * unexpectedly. The only supported use of this method is after creation of the
 * authorizer, but before any network requests are made.
 *
 * Since: 0.17.0
 */
void
gdata_oauth2_authorizer_set_locale (GDataOAuth2Authorizer *self,
                                    const gchar *locale)
{
	g_return_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self));

	if (g_strcmp0 (locale, self->priv->locale) == 0) {
		/* Already has this value */
		return;
	}

	g_free (self->priv->locale);
	self->priv->locale = g_strdup (locale);
	g_object_notify (G_OBJECT (self), "locale");
}

static void
notify_timeout_cb (GObject *gobject, GParamSpec *pspec, GObject *self)
{
	g_object_notify (self, "timeout");
}

/**
 * gdata_oauth2_authorizer_get_timeout:
 * @self: a #GDataOAuth2Authorizer
 *
 * Gets the #GDataOAuth2Authorizer:timeout property; the network timeout, in
 * seconds.
 *
 * Return value: the timeout, or <code class="literal">0</code>
 *
 * Since: 0.17.0
 */
guint
gdata_oauth2_authorizer_get_timeout (GDataOAuth2Authorizer *self)
{
	guint timeout;

	g_return_val_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self), 0);

	g_object_get (self->priv->session,
	              SOUP_SESSION_TIMEOUT, &timeout,
	              NULL);

	return timeout;
}

/**
 * gdata_oauth2_authorizer_set_timeout:
 * @self: a #GDataOAuth2Authorizer
 * @timeout: the timeout, or <code class="literal">0</code>
 *
 * Sets the #GDataOAuth2Authorizer:timeout property; the network timeout, in
 * seconds.
 *
 * If @timeout is <code class="literal">0</code>, network operations will never
 * time out.
 *
 * Since: 0.17.0
 */
void
gdata_oauth2_authorizer_set_timeout (GDataOAuth2Authorizer *self, guint timeout)
{
	g_return_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self));

	if (timeout == gdata_oauth2_authorizer_get_timeout (self)) {
		return;
	}

	g_object_set (self->priv->session, SOUP_SESSION_TIMEOUT, timeout, NULL);
}

/**
 * gdata_oauth2_authorizer_get_proxy_resolver:
 * @self: a #GDataOAuth2Authorizer
 *
 * Gets the #GProxyResolver on the #GDataOAuth2Authorizer's #SoupSession.
 *
 * Return value: (transfer none) (allow-none): a #GProxyResolver, or %NULL
 *
 * Since: 0.17.0
 */
GProxyResolver *
gdata_oauth2_authorizer_get_proxy_resolver (GDataOAuth2Authorizer *self)
{
	g_return_val_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self), NULL);

	return self->priv->proxy_resolver;
}

/**
 * gdata_oauth2_authorizer_set_proxy_resolver:
 * @self: a #GDataOAuth2Authorizer
 * @proxy_resolver: (allow-none): a #GProxyResolver, or %NULL
 *
 * Sets the #GProxyResolver on the #SoupSession used internally by the given
 * #GDataOAuth2Authorizer.
 *
 * Since: 0.17.0
 */
void
gdata_oauth2_authorizer_set_proxy_resolver (GDataOAuth2Authorizer *self,
                                            GProxyResolver *proxy_resolver)
{
	g_return_if_fail (GDATA_IS_OAUTH2_AUTHORIZER (self));
	g_return_if_fail (proxy_resolver == NULL ||
	                  G_IS_PROXY_RESOLVER (proxy_resolver));

	if (proxy_resolver != NULL) {
		g_object_ref (proxy_resolver);
	}

	g_clear_object (&self->priv->proxy_resolver);
	self->priv->proxy_resolver = proxy_resolver;

	g_object_notify (G_OBJECT (self), "proxy-resolver");
}
