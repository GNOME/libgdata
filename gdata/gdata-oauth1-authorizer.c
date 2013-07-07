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
 * SECTION:gdata-oauth1-authorizer
 * @short_description: GData OAuth 1.0 authorization interface
 * @stability: Unstable
 * @include: gdata/gdata-oauth1-authorizer.h
 *
 * #GDataOAuth1Authorizer provides an implementation of the #GDataAuthorizer interface for authentication and authorization using the
 * <ulink type="http" url="http://code.google.com/apis/accounts/docs/OAuthForInstalledApps.html">OAuth 1.0</ulink> process, which is Google's
 * currently preferred authentication and authorization process, though OAuth 2.0 will be transitioned to in future.
 *
 * OAuth 1.0 replaces the deprecated ClientLogin process. One of the main reasons for this is to allow two-factor authentication to be supported, by
 * moving the authentication interface to a web page under Google's control.
 *
 * The OAuth 1.0 process as implemented by Google follows the <ulink type="http" url="http://tools.ietf.org/html/rfc5849">OAuth 1.0 protocol as
 * specified by IETF in RFC 5849</ulink>, with a few additions to support scopes (implemented in libgdata by #GDataAuthorizationDomain<!-- -->s),
 * locales and custom domains. Briefly, the process is initiated by requesting an authenticated request token from the Google accounts service
 * (using gdata_oauth1_authorizer_request_authentication_uri()), going to the authentication URI for the request token, authenticating and authorizing
 * access to the desired scopes, then providing the verifier returned by Google to the Google accounts service again (using
 * gdata_oauth1_authorizer_request_authorization()) to authorize the token. This results in an access token which is attached to all future requests
 * to the online service.
 *
 * While Google supports unregistered and registered modes for OAuth 1.0 authorization, it only supports unregistered mode for installed applications.
 * Consequently, libgdata also only supports unregistered mode. For this purpose, the application name to be presented to the user on the
 * authentication page at the URI returned by gdata_oauth1_authorizer_request_authentication_uri() can be specified when constructing the
 * #GDataOAuth1Authorizer.
 *
 * As described, each authentication/authorization operation is in two parts: gdata_oauth1_authorizer_request_authentication_uri() and
 * gdata_oauth1_authorizer_request_authorization(). #GDataOAuth1Authorizer stores no state about ongoing authentication operations (i.e. ones which
 * have successfully called gdata_oauth1_authorizer_request_authentication_uri(), but are yet to successfully call
 * gdata_oauth1_authorizer_request_authorization()). Consequently, operations can be abandoned before calling
 * gdata_oauth1_authorizer_request_authorization() without problems. The only state necessary between the calls is the request token and request token
 * secret, as returned by gdata_oauth1_authorizer_request_authentication_uri() and taken as parameters to
 * gdata_oauth1_authorizer_request_authorization().
 *
 * #GDataOAuth1Authorizer natively supports authorization against multiple services in a single authorization request (unlike
 * #GDataClientLoginAuthorizer).
 *
 * Each access token is long lived, so reauthorization is rarely necessary with #GDataOAuth1Authorizer. Consequently, refreshing authorization using
 * gdata_authorizer_refresh_authorization() is not supported by #GDataOAuth1Authorizer, and will immediately return %FALSE with no error set.
 *
 * <example>
 *	<title>Authenticating Asynchronously Using OAuth 1.0</title>
 *	<programlisting>
 *	GDataSomeService *service;
 *	GDataOAuth1Authorizer *authorizer;
 *
 *	/<!-- -->* Create an authorizer and authenticate and authorize the service we're using, asynchronously. *<!-- -->/
 *	authorizer = gdata_oauth1_authorizer_new (_("My libgdata application"), GDATA_TYPE_SOME_SERVICE);
 *	gdata_oauth1_authorizer_request_authentication_uri_async (authorizer, cancellable,
 *	                                                          (GAsyncReadyCallback) request_authentication_uri_cb, user_data);
 *
 *	/<!-- -->* Create a service object and link it with the authorizer *<!-- -->/
 *	service = gdata_some_service_new (GDATA_AUTHORIZER (authorizer));
 *
 *	static void
 *	request_authentication_uri_cb (GDataOAuth1Authorizer *authorizer, GAsyncResult *async_result, gpointer user_data)
 *	{
 *		gchar *authentication_uri, *token, *token_secret, *verifier;
 *		GError *error = NULL;
 *
 *		authentication_uri = gdata_oauth1_authorizer_request_authentication_uri_finish (authorizer, async_result, &token, &token_secret,
 *		                                                                                &error);
 *
 *		if (error != NULL) {
 *			/<!-- -->* Notify the user of all errors except cancellation errors *<!-- -->/
 *			if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
 *				g_error ("Requesting a token failed: %s", error->message);
 *			}
 *
 *			g_error_free (error);
 *			goto finish;
 *		}
 *
 *		/<!-- -->* (Present the page at the authentication URI to the user, either in an embedded or stand-alone web browser, and
 *		 * ask them to grant access to the application and return the verifier Google gives them.) *<!-- -->/
 *		verifier = ask_user_for_verifier (authentication_uri);
 *
 *		gdata_oauth1_authorizer_request_authorization_async (authorizer, token, token_secret, verifier, cancellable,
 *		                                                     (GAsyncReadyCallback) request_authorization_cb, user_data);
 *
 *	finish:
 *		g_free (verifier);
 *		g_free (authentication_uri);
 *		g_free (token);
 *
 *		/<!-- -->* Zero out the secret before freeing. *<!-- -->/
 *		if (token_secret != NULL) {
 *			memset (token_secret, 0, strlen (token_secret));
 *		}
 *
 *		g_free (token_secret);
 *	}
 *
 *	static void
 *	request_authorization_cb (GDataOAuth1Authorizer *authorizer, GAsyncResult *async_result, gpointer user_data)
 *	{
 *		GError *error = NULL;
 *
 *		if (gdata_oauth1_authorizer_request_authorization_finish (authorizer, async_result, &error) == FALSE) {
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
 * Since: 0.9.0
 */

#include <config.h>
#include <string.h>
#include <oauth.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "gdata-oauth1-authorizer.h"
#include "gdata-private.h"

#define HMAC_SHA1_LEN 20 /* bytes, raw */

static void authorizer_init (GDataAuthorizerInterface *iface);
static void dispose (GObject *object);
static void finalize (GObject *object);
static void get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

static void process_request (GDataAuthorizer *self, GDataAuthorizationDomain *domain, SoupMessage *message);
static gboolean is_authorized_for_domain (GDataAuthorizer *self, GDataAuthorizationDomain *domain);

static void sign_message (GDataOAuth1Authorizer *self, SoupMessage *message, const gchar *token, const gchar *token_secret, GHashTable *parameters);

static void notify_proxy_uri_cb (GObject *object, GParamSpec *pspec, GDataOAuth1Authorizer *self);
static void notify_timeout_cb (GObject *gobject, GParamSpec *pspec, GObject *self);

struct _GDataOAuth1AuthorizerPrivate {
	SoupSession *session;
	SoupURI *proxy_uri; /* cached version only set if gdata_oauth1_authorizer_get_proxy_uri() is called */

	gchar *application_name;
	gchar *locale;

	GMutex mutex; /* mutex for token, token_secret and authorization_domains */

	/* Note: This is the access token, not the request token returned by gdata_oauth1_authorizer_request_authentication_uri().
	 * It's NULL iff the authorizer isn't authenticated. token_secret must be NULL iff token is NULL. */
	gchar *token;
	GDataSecureString token_secret; /* must be allocated by _gdata_service_secure_strdup() */

	/* Mapping from GDataAuthorizationDomain to itself; a set of domains for which ->access_token is valid. */
	GHashTable *authorization_domains;
};

enum {
	PROP_APPLICATION_NAME = 1,
	PROP_LOCALE,
	PROP_PROXY_URI,
	PROP_TIMEOUT,
};

G_DEFINE_TYPE_WITH_CODE (GDataOAuth1Authorizer, gdata_oauth1_authorizer, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_AUTHORIZER, authorizer_init))

static void
gdata_oauth1_authorizer_class_init (GDataOAuth1AuthorizerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (GDataOAuth1AuthorizerPrivate));

	gobject_class->get_property = get_property;
	gobject_class->set_property = set_property;
	gobject_class->dispose = dispose;
	gobject_class->finalize = finalize;

	/**
	 * GDataOAuth1Authorizer:application-name:
	 *
	 * The human-readable, translated application name for the client, to be presented to the user on the authentication page at the URI
	 * returned by gdata_oauth1_authorizer_request_authentication_uri().
	 *
	 * If %NULL is provided in the constructor to #GDataOAuth1Authorizer, the value returned by g_get_application_name() will be used as a
	 * fallback. Note that this may also be %NULL: in this case, the authentication page will use the application name “anonymous”.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_APPLICATION_NAME,
	                                 g_param_spec_string ("application-name",
	                                                      "Application name", "The human-readable, translated application name for the client.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataOAuth1Authorizer:locale:
	 *
	 * The locale to use for network requests, in Unix locale format. (e.g. "en_GB", "cs", "de_DE".) Use %NULL for the default "C" locale
	 * (typically "en_US").
	 *
	 * This locale will be used by the server-side software to localise the authentication and authorization pages at the URI returned by
	 * gdata_oauth1_authorizer_request_authentication_uri().
	 *
	 * The server-side behaviour is undefined if it doesn't support a given locale.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_LOCALE,
	                                 g_param_spec_string ("locale",
	                                                      "Locale", "The locale to use for network requests, in Unix locale format.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataOAuth1Authorizer:proxy-uri:
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
	 * GDataOAuth1Authorizer:timeout:
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
}

static void
authorizer_init (GDataAuthorizerInterface *iface)
{
	iface->process_request = process_request;
	iface->is_authorized_for_domain = is_authorized_for_domain;
}

static void
gdata_oauth1_authorizer_init (GDataOAuth1Authorizer *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, GDATA_TYPE_OAUTH1_AUTHORIZER, GDataOAuth1AuthorizerPrivate);

	/* Set up the authorizer's mutex */
	g_mutex_init (&(self->priv->mutex));
	self->priv->authorization_domains = g_hash_table_new_full (g_direct_hash, g_direct_equal, g_object_unref, NULL);

	/* Set up the session */
	self->priv->session = _gdata_service_build_session ();

	/* Proxy the SoupSession's proxy-uri and timeout properties */
	g_signal_connect (self->priv->session, "notify::proxy-uri", (GCallback) notify_proxy_uri_cb, self);
	g_signal_connect (self->priv->session, "notify::timeout", (GCallback) notify_timeout_cb, self);
}

static void
dispose (GObject *object)
{
	GDataOAuth1AuthorizerPrivate *priv = GDATA_OAUTH1_AUTHORIZER (object)->priv;

	if (priv->session != NULL)
		g_object_unref (priv->session);
	priv->session = NULL;

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_oauth1_authorizer_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
	GDataOAuth1AuthorizerPrivate *priv = GDATA_OAUTH1_AUTHORIZER (object)->priv;

	g_free (priv->application_name);
	g_free (priv->locale);

	g_hash_table_destroy (priv->authorization_domains);
	g_mutex_clear (&(priv->mutex));

	if (priv->proxy_uri != NULL) {
		soup_uri_free (priv->proxy_uri);
	}

	g_free (priv->token);
	_gdata_service_secure_strfree (priv->token_secret);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_oauth1_authorizer_parent_class)->finalize (object);
}

static void
get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataOAuth1AuthorizerPrivate *priv = GDATA_OAUTH1_AUTHORIZER (object)->priv;

	switch (property_id) {
		case PROP_APPLICATION_NAME:
			g_value_set_string (value, priv->application_name);
			break;
		case PROP_LOCALE:
			g_value_set_string (value, priv->locale);
			break;
		case PROP_PROXY_URI:
			g_value_set_boxed (value, gdata_oauth1_authorizer_get_proxy_uri (GDATA_OAUTH1_AUTHORIZER (object)));
			break;
		case PROP_TIMEOUT:
			g_value_set_uint (value, gdata_oauth1_authorizer_get_timeout (GDATA_OAUTH1_AUTHORIZER (object)));
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
	GDataOAuth1AuthorizerPrivate *priv = GDATA_OAUTH1_AUTHORIZER (object)->priv;

	switch (property_id) {
		/* Construct only */
		case PROP_APPLICATION_NAME:
			priv->application_name = g_value_dup_string (value);

			/* Default to the value of g_get_application_name() */
			if (priv->application_name == NULL || *(priv->application_name) == '\0') {
				g_free (priv->application_name);
				priv->application_name = g_strdup (g_get_application_name ());
			}

			break;
		case PROP_LOCALE:
			gdata_oauth1_authorizer_set_locale (GDATA_OAUTH1_AUTHORIZER (object), g_value_get_string (value));
			break;
		case PROP_PROXY_URI:
			gdata_oauth1_authorizer_set_proxy_uri (GDATA_OAUTH1_AUTHORIZER (object), g_value_get_boxed (value));
			break;
		case PROP_TIMEOUT:
			gdata_oauth1_authorizer_set_timeout (GDATA_OAUTH1_AUTHORIZER (object), g_value_get_uint (value));
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
	GDataOAuth1AuthorizerPrivate *priv = GDATA_OAUTH1_AUTHORIZER (self)->priv;

	/* Set the authorisation header */
	g_mutex_lock (&(priv->mutex));

	/* Sanity check */
	g_assert ((priv->token == NULL) == (priv->token_secret == NULL));

	if (priv->token != NULL && g_hash_table_lookup (priv->authorization_domains, domain) != NULL) {
		sign_message (GDATA_OAUTH1_AUTHORIZER (self), message, priv->token, priv->token_secret, NULL);
	}

	g_mutex_unlock (&(priv->mutex));
}

static gboolean
is_authorized_for_domain (GDataAuthorizer *self, GDataAuthorizationDomain *domain)
{
	GDataOAuth1AuthorizerPrivate *priv = GDATA_OAUTH1_AUTHORIZER (self)->priv;
	gpointer result;
	const gchar *token;

	g_mutex_lock (&(priv->mutex));
	token = priv->token;
	result = g_hash_table_lookup (priv->authorization_domains, domain);
	g_mutex_unlock (&(priv->mutex));

	/* Sanity check */
	g_assert (result == NULL || result == domain);

	return (token != NULL && result != NULL) ? TRUE : FALSE;
}

/* Sign the message and add the Authorization header to it containing the signature.
 * NOTE: This must not lock priv->mutex, as it's called from within a critical section in process_request() and priv->mutex isn't recursive. */
static void
sign_message (GDataOAuth1Authorizer *self, SoupMessage *message, const gchar *token, const gchar *token_secret, GHashTable *parameters)
{
	GHashTableIter iter;
	const gchar *key, *value, *consumer_key, *consumer_secret, *signature_method;
	gsize params_length = 0;
	GList *sorted_params = NULL, *i;
	GString *query_string, *signature_base_string, *secret_string, *authorization_header;
	SoupURI *normalised_uri;
	gchar *uri, *signature, *timestamp;
	char *nonce;
	gboolean is_first = TRUE;
	GTimeVal time_val;
	guchar signature_buf[HMAC_SHA1_LEN];
	gsize signature_buf_len;
	GHmac *signature_hmac;

	g_return_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self));
	g_return_if_fail (SOUP_IS_MESSAGE (message));
	g_return_if_fail (token == NULL || *token != '\0');
	g_return_if_fail (token_secret == NULL || *token_secret != '\0');
	g_return_if_fail ((token == NULL) == (token_secret == NULL));

	/* Build and return a HMAC-SHA1 signature for the given SoupMessage. We always use HMAC-SHA1, since installed applications have to be
	 * unregistered (see: http://code.google.com/apis/accounts/docs/OAuth_ref.html#SigningOAuth).
	 * Reference: http://tools.ietf.org/html/rfc5849#section-3.4 */
	signature_method = "HMAC-SHA1";

	/* As described here, we use an anonymous consumer key and secret, since we're designed for installed applications:
	 * http://code.google.com/apis/accounts/docs/OAuth_ref.html#SigningOAuth */
	consumer_key = "anonymous";
	consumer_secret = "anonymous";

	/* Add various standard parameters to the list (note: this modifies the hash table belonging to the caller) */
	nonce = oauth_gen_nonce ();
	g_get_current_time (&time_val);
	timestamp = g_strdup_printf ("%li", time_val.tv_sec);

	if (parameters == NULL) {
		parameters = g_hash_table_new (g_str_hash, g_str_equal);
	} else {
		g_hash_table_ref (parameters);
	}

	g_hash_table_insert (parameters, (gpointer) "oauth_signature_method", (gpointer) signature_method);
	g_hash_table_insert (parameters, (gpointer) "oauth_consumer_key", (gpointer) consumer_key);
	g_hash_table_insert (parameters, (gpointer) "oauth_nonce", nonce);
	g_hash_table_insert (parameters, (gpointer) "oauth_timestamp", timestamp);
	g_hash_table_insert (parameters, (gpointer) "oauth_version", (gpointer) "1.0");

	/* Only add the token if it's been provided */
	if (token != NULL) {
		g_hash_table_insert (parameters, (gpointer) "oauth_token", (gpointer) token);
	}

	/* Sort the parameters and build a query string, as defined here: http://tools.ietf.org/html/rfc5849#section-3.4.1.3 */
	g_hash_table_iter_init (&iter, parameters);

	while (g_hash_table_iter_next (&iter, (gpointer*) &key, (gpointer*) &value) == TRUE) {
		GString *pair = g_string_new (NULL);

		g_string_append_uri_escaped (pair, key, NULL, FALSE);
		g_string_append_c (pair, '=');
		g_string_append_uri_escaped (pair, value, NULL, FALSE);

		/* Append the pair to the list for sorting, and keep track of the total length of the strings in the list so far */
		params_length += pair->len + 1 /* sep */;
		sorted_params = g_list_prepend (sorted_params, g_string_free (pair, FALSE));
	}

	g_hash_table_unref (parameters);

	sorted_params = g_list_sort (sorted_params, (GCompareFunc) g_strcmp0);

	/* Concatenate the parameters to give the query string */
	query_string = g_string_sized_new (params_length);

	for (i = sorted_params; i != NULL; i = i->next) {
		if (is_first == FALSE) {
			g_string_append_c (query_string, '&');
		}

		g_string_append (query_string, i->data);

		g_free (i->data);
		is_first = FALSE;
	}

	g_list_free (sorted_params);

	/* Normalise the URI as described here: http://tools.ietf.org/html/rfc5849#section-3.4.1.2 */
	normalised_uri = soup_uri_copy (soup_message_get_uri (message));
	soup_uri_set_query (normalised_uri, NULL);
	soup_uri_set_fragment (normalised_uri, NULL);

	/* Append it to the signature base string */
	uri = soup_uri_to_string (normalised_uri, FALSE);

	/* Start building the signature base string as described here: http://tools.ietf.org/html/rfc5849#section-3.4.1.1 */
	signature_base_string = g_string_sized_new (4 /* method */ + 1 /* sep */ + strlen (uri) + 1 /* sep */ + params_length /* query string */);
	g_string_append_uri_escaped (signature_base_string, message->method, NULL, FALSE);
	g_string_append_c (signature_base_string, '&');
	g_string_append_uri_escaped (signature_base_string, uri, NULL, FALSE);
	g_string_append_c (signature_base_string, '&');
	g_string_append_uri_escaped (signature_base_string, query_string->str, NULL, FALSE);

	g_free (uri);
	soup_uri_free (normalised_uri);
	g_string_free (query_string, TRUE);

	/* Build the secret key to use in the HMAC */
	secret_string = g_string_new (NULL);
	g_string_append_uri_escaped (secret_string, consumer_secret, NULL, FALSE);
	g_string_append_c (secret_string, '&');

	/* Only add token_secret if it was provided */
	if (token_secret != NULL) {
		g_string_append_uri_escaped (secret_string, token_secret, NULL, FALSE);
	}

	/* Create the signature as described here: http://tools.ietf.org/html/rfc5849#section-3.4.2 */
	signature_hmac = g_hmac_new (G_CHECKSUM_SHA1, (const guchar*) secret_string->str, secret_string->len);
	g_hmac_update (signature_hmac, (const guchar*) signature_base_string->str, signature_base_string->len);

	signature_buf_len = G_N_ELEMENTS (signature_buf);
	g_hmac_get_digest (signature_hmac, signature_buf, &signature_buf_len);

	g_hmac_unref (signature_hmac);

	signature = g_base64_encode (signature_buf, signature_buf_len);

	/*g_debug ("Signing message using Signature Base String: “%s” and key “%s” using method “%s” to give signature: “%s”.",
	         signature_base_string->str, secret_string->str, signature_method, signature);*/

	/* Zero out the secret_string before freeing it, to reduce the chance of secrets hitting disk. */
	memset (secret_string->str, 0, secret_string->allocated_len);

	g_string_free (secret_string, TRUE);
	g_string_free (signature_base_string, TRUE);

	/* Build the Authorization header and append it to the message */
	authorization_header = g_string_new ("OAuth oauth_consumer_key=\"");
	g_string_append_uri_escaped (authorization_header, consumer_key, NULL, FALSE);

	/* Only add the token if it's been provided */
	if (token != NULL) {
		g_string_append (authorization_header, "\",oauth_token=\"");
		g_string_append_uri_escaped (authorization_header, token, NULL, FALSE);
	}

	g_string_append (authorization_header, "\",oauth_signature_method=\"");
	g_string_append_uri_escaped (authorization_header, signature_method, NULL, FALSE);
	g_string_append (authorization_header, "\",oauth_signature=\"");
	g_string_append_uri_escaped (authorization_header, signature, NULL, FALSE);
	g_string_append (authorization_header, "\",oauth_timestamp=\"");
	g_string_append_uri_escaped (authorization_header, timestamp, NULL, FALSE);
	g_string_append (authorization_header, "\",oauth_nonce=\"");
	g_string_append_uri_escaped (authorization_header, nonce, NULL, FALSE);
	g_string_append (authorization_header, "\",oauth_version=\"1.0\"");

	soup_message_headers_replace (message->request_headers, "Authorization", authorization_header->str);

	g_string_free (authorization_header, TRUE);
	free (signature);
	g_free (timestamp);
	free (nonce);
}

/**
 * gdata_oauth1_authorizer_new:
 * @application_name: (allow-none): a human-readable, translated application name to use on authentication pages, or %NULL
 * @service_type: the #GType of a #GDataService subclass which the #GDataOAuth1Authorizer will be used with
 *
 * Creates a new #GDataOAuth1Authorizer.
 *
 * The #GDataAuthorizationDomain<!-- -->s for the given @service_type (i.e. as returned by gdata_service_get_authorization_domains()) are the ones the
 * user will be requested to authorize access to on the page at the URI returned by gdata_oauth1_authorizer_request_authentication_uri().
 *
 * The given @application_name will set the value of #GDataOAuth1Authorizer:application-name and will be displayed to the user on authentication pages
 * returned by Google. If %NULL is provided, the value of g_get_application_name() will be used as a fallback.
 *
 * Return value: (transfer full): a new #GDataOAuth1Authorizer; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GDataOAuth1Authorizer *
gdata_oauth1_authorizer_new (const gchar *application_name, GType service_type)
{
	g_return_val_if_fail (g_type_is_a (service_type, GDATA_TYPE_SERVICE), NULL);

	return gdata_oauth1_authorizer_new_for_authorization_domains (application_name, gdata_service_get_authorization_domains (service_type));
}

/**
 * gdata_oauth1_authorizer_new_for_authorization_domains:
 * @application_name: (allow-none): a human-readable, translated application name to use on authentication pages, or %NULL
 * @authorization_domains: (element-type GDataAuthorizationDomain) (transfer none): a non-empty list of #GDataAuthorizationDomain<!-- -->s to be
 * authorized against by the #GDataOAuth1Authorizer
 *
 * Creates a new #GDataOAuth1Authorizer. This function is intended to be used only when the default authorization domain list for a single
 * #GDataService, as used by gdata_oauth1_authorizer_new(), isn't suitable. For example, this could be because the #GDataOAuth1Authorizer will be used
 * with multiple #GDataService subclasses, or because the client requires a specific set of authorization domains.
 *
 * The specified #GDataAuthorizationDomain<!-- -->s are the ones the user will be requested to authorize access to on the page at the URI returned by
 * gdata_oauth1_authorizer_request_authentication_uri().
 *
 * The given @application_name will set the value of #GDataOAuth1Authorizer:application-name and will be displayed to the user on authentication pages
 * returned by Google. If %NULL is provided, the value of g_get_application_name() will be used as a fallback.
 *
 * Return value: (transfer full): a new #GDataOAuth1Authorizer; unref with g_object_unref()
 *
 * Since: 0.9.0
 */
GDataOAuth1Authorizer *
gdata_oauth1_authorizer_new_for_authorization_domains (const gchar *application_name, GList *authorization_domains)
{
	GList *i;
	GDataOAuth1Authorizer *authorizer;

	g_return_val_if_fail (authorization_domains != NULL, NULL);

	authorizer = GDATA_OAUTH1_AUTHORIZER (g_object_new (GDATA_TYPE_OAUTH1_AUTHORIZER,
	                                                    "application-name", application_name,
	                                                    NULL));

	/* Register all the domains with the authorizer */
	for (i = authorization_domains; i != NULL; i = i->next) {
		g_return_val_if_fail (GDATA_IS_AUTHORIZATION_DOMAIN (i->data), NULL);

		/* We don't have to lock the authoriser's mutex here as no other code has seen the authoriser yet */
		g_hash_table_insert (authorizer->priv->authorization_domains, g_object_ref (GDATA_AUTHORIZATION_DOMAIN (i->data)), i->data);
	}

	return authorizer;
}

/**
 * gdata_oauth1_authorizer_request_authentication_uri:
 * @self: a #GDataOAuth1Authorizer
 * @token: (out callee-allocates): return location for the temporary credentials token returned by the authentication service; free with g_free()
 * @token_secret: (out callee-allocates): return location for the temporary credentials token secret returned by the authentication service; free with
 * g_free()
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Requests a fresh unauthenticated token from the Google accounts service and builds and returns the URI of an authentication page for that token.
 * This should then be presented to the user (e.g. in an embedded or stand alone web browser). The authentication page will ask the user to log in
 * using their Google account, then ask them to grant access to the #GDataAuthorizationDomain<!-- -->s passed to the constructor of the
 * #GDataOAuth1Authorizer. If the user grants access, they will be given a verifier, which can then be passed to
 * gdata_oauth1_authorizer_request_authorization() (along with the @token and @token_secret values returned by this method) to authorize the token.
 *
 * This method can fail if the server returns an error, but this is unlikely. If it does happen, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR will be
 * raised, @token and @token_secret will be set to %NULL and %NULL will be returned.
 *
 * This method implements <ulink type="http" url="http://tools.ietf.org/html/rfc5849#section-2.1">Section 2.1</ulink> and
 * <ulink type="http" url="http://tools.ietf.org/html/rfc5849#section-2.2">Section 2.2</ulink> of the
 * <ulink type="http" url="http://tools.ietf.org/html/rfc5849">OAuth 1.0 protocol</ulink>.
 *
 * When freeing @token_secret, it's advisable to set it to all zeros first, to reduce the chance of the sensitive token being recoverable from the
 * free memory pool and (accidentally) leaked by a different part of the process. This can be achieved with the following code:
 * |[
 *	if (token_secret != NULL) {
 *		memset (token_secret, 0, strlen (token_secret));
 *		g_free (token_secret);
 *	}
 * ]|
 *
 * Return value: (transfer full): the URI of an authentication page for the user to use; free with g_free()
 *
 * Since: 0.9.0
 */
gchar *
gdata_oauth1_authorizer_request_authentication_uri (GDataOAuth1Authorizer *self, gchar **token, gchar **token_secret,
                                                    GCancellable *cancellable, GError **error)
{
	GDataOAuth1AuthorizerPrivate *priv;
	SoupMessage *message;
	guint status;
	gchar *request_body;
	GString *scope_string, *authentication_uri;
	GHashTable *parameters;
	GHashTableIter iter;
	gboolean is_first = TRUE;
	GDataAuthorizationDomain *domain;
	GHashTable *response_details;
	const gchar *callback_uri, *_token, *_token_secret, *callback_confirmed;
	SoupURI *_uri;

	g_return_val_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self), NULL);
	g_return_val_if_fail (token != NULL, NULL);
	g_return_val_if_fail (token_secret != NULL, NULL);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, NULL);

	priv = self->priv;

	/* This implements OAuthGetRequestToken and returns the URI for OAuthAuthorizeToken, which the client must then use themselves (e.g. in an
	 * embedded web browser) to authorise the temporary credentials token. They then pass the request token and verification code they get back
	 * from that to gdata_oauth1_authorizer_request_authorization(). */

	/* We default to out-of-band callbacks */
	callback_uri = "oob";

	/* Set the output parameters to NULL in case of failure */
	*token = NULL;
	*token_secret = NULL;

	/* Build up the space-separated list of scopes we're requesting authorisation for */
	g_mutex_lock (&(priv->mutex));

	scope_string = g_string_new (NULL);
	g_hash_table_iter_init (&iter, priv->authorization_domains);

	while (g_hash_table_iter_next (&iter, (gpointer*) &domain, NULL) == TRUE) {
		if (is_first == FALSE) {
			/* Delimiter */
			g_string_append_c (scope_string, ' ');
		}

		g_string_append (scope_string, gdata_authorization_domain_get_scope (domain));

		is_first = FALSE;
	}

	g_mutex_unlock (&(priv->mutex));

	/* Build the request body and the set of parameters to be signed */
	parameters = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (parameters, (gpointer) "scope", scope_string->str);
	g_hash_table_insert (parameters, (gpointer) "xoauth_displayname", priv->application_name);
	g_hash_table_insert (parameters, (gpointer) "oauth_callback", (gpointer) callback_uri);
	request_body = soup_form_encode_hash (parameters);

	/* Build the message */
	_uri = soup_uri_new ("https://www.google.com/accounts/OAuthGetRequestToken");
	soup_uri_set_port (_uri, _gdata_service_get_https_port ());
	message = soup_message_new_from_uri (SOUP_METHOD_POST, _uri);
	soup_uri_free (_uri);

	soup_message_set_request (message, "application/x-www-form-urlencoded", SOUP_MEMORY_TAKE, request_body, strlen (request_body));

	sign_message (self, message, NULL, NULL, parameters);

	g_hash_table_destroy (parameters);
	g_string_free (scope_string, TRUE);

	/* Send the message */
	_gdata_service_actually_send_message (priv->session, message, cancellable, error);
	status = message->status_code;

	if (status == SOUP_STATUS_CANCELLED) {
		/* Cancelled (the error has already been set) */
		g_object_unref (message);
		return NULL;
	} else if (status != SOUP_STATUS_OK) {
		/* Server returned an error. Not much we can do, since the error codes aren't documented and it shouldn't normally ever happen
		 * anyway. */
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
		                     _("The server rejected the temporary credentials request."));
		g_object_unref (message);

		return NULL;
	}

	g_assert (message->response_body->data != NULL);

	/* Parse the response. We expect something like:
	 *   oauth_token=ab3cd9j4ks73hf7g&oauth_token_secret=ZXhhbXBsZS5jb20&oauth_callback_confirmed=true
	 * See: http://code.google.com/apis/accounts/docs/OAuth_ref.html#RequestToken and
	 * http://tools.ietf.org/html/rfc5849#section-2.1 for details. */
	response_details = soup_form_decode (message->response_body->data);

	g_object_unref (message);

	_token = g_hash_table_lookup (response_details, "oauth_token");
	_token_secret = g_hash_table_lookup (response_details, "oauth_token_secret");
	callback_confirmed = g_hash_table_lookup (response_details, "oauth_callback_confirmed");

	/* Validate the returned values */
	if (_token == NULL || _token_secret == NULL || callback_confirmed == NULL ||
	    *_token == '\0' || *_token_secret == '\0' ||
	    strcmp (callback_confirmed, "true") != 0) {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR,
		                     _("The server returned a malformed response."));
		g_hash_table_destroy (response_details);

		return NULL;
	}

	/* Build the authentication URI which the user will then open in a web browser and use to authenticate and authorise our application.
	 * We expect to build something like this:
	 *   https://www.google.com/accounts/OAuthAuthorizeToken?oauth_token=ab3cd9j4ks73hf7g&hd=mycollege.edu&hl=en&btmpl=mobile
	 * See: http://code.google.com/apis/accounts/docs/OAuth_ref.html#GetAuth for more details. */
	authentication_uri = g_string_new ("https://www.google.com/accounts/OAuthAuthorizeToken?oauth_token=");
	g_string_append_uri_escaped (authentication_uri, g_hash_table_lookup (response_details, "oauth_token"), NULL, TRUE);

	if (priv->locale != NULL) {
		g_string_append (authentication_uri, "&hl=");
		g_string_append_uri_escaped (authentication_uri, priv->locale, NULL, TRUE);
	}

	/* Return the token and token secret */
	*token = g_strdup (_token);
	*token_secret = g_strdup (_token_secret); /* NOTE: Ideally this would be allocated in non-pageable memory, but changing that would break API */

	g_hash_table_destroy (response_details);

	return g_string_free (authentication_uri, FALSE);
}

typedef struct {
	/* All return values */
	gchar *token;
	gchar *token_secret; /* NOTE: Ideally this would be allocated in non-pageable memory, but changing that would break API */
	gchar *authentication_uri;
} RequestAuthenticationUriAsyncData;

static void
request_authentication_uri_async_data_free (RequestAuthenticationUriAsyncData *data)
{
	g_free (data->token);
	g_free (data->token_secret);
	g_free (data->authentication_uri);

	g_slice_free (RequestAuthenticationUriAsyncData, data);
}

static void
request_authentication_uri_thread (GSimpleAsyncResult *result, GDataOAuth1Authorizer *authorizer, GCancellable *cancellable)
{
	RequestAuthenticationUriAsyncData *data;
	GError *error = NULL;

	data = g_simple_async_result_get_op_res_gpointer (result);

	data->authentication_uri = gdata_oauth1_authorizer_request_authentication_uri (authorizer, &(data->token), &(data->token_secret),
	                                                                               cancellable, &error);

	if (error != NULL) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
	}
}

/**
 * gdata_oauth1_authorizer_request_authentication_uri_async:
 * @self: a #GDataOAuth1Authorizer
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @callback: a #GAsyncReadyCallback to call when building the URI is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Requests a fresh unauthenticated token from the Google accounts service and builds and returns the URI of an authentication page for that token.
 * @self is reffed when this method is called, so can safely be unreffed after this method returns.
 *
 * For more details, see gdata_oauth1_authorizer_request_authentication_uri(), which is the synchronous version of this method.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_oauth1_authorizer_request_authentication_uri_finish() to get the
 * results of the operation.
 *
 * Since: 0.9.0
 */
void
gdata_oauth1_authorizer_request_authentication_uri_async (GDataOAuth1Authorizer *self, GCancellable *cancellable,
                                                          GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *result;
	RequestAuthenticationUriAsyncData *data;

	g_return_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self));
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));
	g_return_if_fail (callback != NULL);

	data = g_slice_new (RequestAuthenticationUriAsyncData);
	data->token = NULL;
	data->token_secret = NULL;
	data->authentication_uri = NULL;

	result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_oauth1_authorizer_request_authentication_uri_async);
	g_simple_async_result_set_op_res_gpointer (result, data, (GDestroyNotify) request_authentication_uri_async_data_free);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) request_authentication_uri_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
}

/**
 * gdata_oauth1_authorizer_request_authentication_uri_finish:
 * @self: a #GDataOAuth1Authorizer
 * @async_result: a #GAsyncResult
 * @token: (out callee-allocates): return location for the temporary credentials token returned by the authentication service; free with g_free()
 * @token_secret: (out callee-allocates): return location for the temporary credentials token secret returned by the authentication service; free with
 * g_free()
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous authentication URI building operation started with gdata_oauth1_authorizer_request_authentication_uri_async().
 *
 * This method can fail if the server has returned an error, but this is unlikely. If it does happen, a %GDATA_SERVICE_ERROR_PROTOCOL_ERROR will be
 * raised, @token and @token_secret will be set to %NULL and %NULL will be returned.
 *
 * When freeing @token_secret, it's advisable to set it to all zeros first, to reduce the chance of the sensitive token being recoverable from the
 * free memory pool and (accidentally) leaked by a different part of the process. This can be achieved with the following code:
 * |[
 *	if (token_secret != NULL) {
 *		memset (token_secret, 0, strlen (token_secret));
 *		g_free (token_secret);
 *	}
 * ]|
 *
 * Return value: (transfer full): the URI of an authentication page for the user to use; free with g_free()
 *
 * Since: 0.9.0
 */
gchar *
gdata_oauth1_authorizer_request_authentication_uri_finish (GDataOAuth1Authorizer *self, GAsyncResult *async_result, gchar **token,
                                                           gchar **token_secret, GError **error)
{
	RequestAuthenticationUriAsyncData *data;
	gchar *authentication_uri;

	g_return_val_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self), FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), FALSE);
	g_return_val_if_fail (token != NULL, NULL);
	g_return_val_if_fail (token_secret != NULL, NULL);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_warn_if_fail (g_simple_async_result_is_valid (async_result, G_OBJECT (self), gdata_oauth1_authorizer_request_authentication_uri_async));

	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (async_result), error) == TRUE) {
		/* Return the error and set all of the output parameters to NULL */
		*token = NULL;
		*token_secret = NULL;

		return NULL;
	}

	/* Success! Transfer the output to the appropriate parameters and nullify it so it doesn't get freed when the async result gets finalised */
	data = g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (async_result));

	*token = data->token;
	*token_secret = data->token_secret;
	authentication_uri = data->authentication_uri;

	data->token = NULL;
	data->token_secret = NULL;
	data->authentication_uri = NULL;

	return authentication_uri;
}

/**
 * gdata_oauth1_authorizer_request_authorization:
 * @self: a #GDataOAuth1Authorizer
 * @token: the request token returned by gdata_oauth1_authorizer_request_authentication_uri()
 * @token_secret: the request token secret returned by gdata_oauth1_authorizer_request_authentication_uri()
 * @verifier: the verifier entered by the user from the authentication page
 * @cancellable: (allow-none): optional #GCancellable object, or %NULL
 * @error: a #GError, or %NULL
 *
 * Requests authorization of the given request @token from the Google accounts service using the given @verifier as entered by the user from the
 * authentication page at the URI returned by gdata_oauth1_authorizer_request_authentication_uri(). @token and @token_secret must be the same values
 * as were returned by gdata_oauth1_authorizer_request_authentication_uri() if it was successful.
 *
 * If the verifier is valid (i.e. the user granted access to the application and the Google accounts service has no reason to distrust the client),
 * %TRUE will be returned and any operations performed from that point onwards on #GDataService<!-- -->s using this #GDataAuthorizer will be
 * authorized.
 *
 * If the user denies access to the application or the Google accounts service distrusts it, a bogus verifier could be returned. In this case, %FALSE
 * will be returned and a %GDATA_SERVICE_ERROR_FORBIDDEN error will be raised.
 *
 * Note that if the user denies access to the application, it may be the case that they have no verifier to enter. In this case, the client can simply
 * not call this method. The #GDataOAuth1Authorizer stores no state for authentication operations which have succeeded in calling
 * gdata_oauth1_authorizer_request_authentication_uri() but not yet successfully called gdata_oauth1_authorizer_request_authorization().
 *
 * This method implements <ulink type="http" url="http://tools.ietf.org/html/rfc5849#section-2.3">Section 2.3</ulink> of the
 * <ulink type="http" url="http://tools.ietf.org/html/rfc5849">OAuth 1.0 protocol</ulink>.
 *
 * Return value: %TRUE if authorization was successful, %FALSE otherwise
 *
 * Since: 0.9.0
 */
gboolean
gdata_oauth1_authorizer_request_authorization (GDataOAuth1Authorizer *self, const gchar *token, const gchar *token_secret, const gchar *verifier,
                                               GCancellable *cancellable, GError **error)
{
	GDataOAuth1AuthorizerPrivate *priv;
	SoupMessage *message;
	guint status;
	gchar *request_body;
	GHashTable *parameters;
	GHashTable *response_details;
	const gchar *_token, *_token_secret;
	SoupURI *_uri;

	g_return_val_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self), FALSE);
	g_return_val_if_fail (token != NULL && *token != '\0', FALSE);
	g_return_val_if_fail (token_secret != NULL && *token_secret != '\0', FALSE);
	g_return_val_if_fail (verifier != NULL && *verifier != '\0', FALSE);
	g_return_val_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	/* This implements OAuthGetAccessToken using the request token returned by OAuthGetRequestToken and the verification code returned by
	 * OAuthAuthorizeToken. See:
	 *  • http://code.google.com/apis/accounts/docs/OAuth_ref.html#AccessToken
	 *  • http://tools.ietf.org/html/rfc5849#section-2.3
	 */

	priv = self->priv;

	/* Build the request body and the set of parameters to be signed */
	parameters = g_hash_table_new (g_str_hash, g_str_equal);
	g_hash_table_insert (parameters, (gpointer) "oauth_verifier", (gpointer) verifier);
	request_body = soup_form_encode_hash (parameters);

	/* Build the message */
	_uri = soup_uri_new ("https://www.google.com/accounts/OAuthGetAccessToken");
	soup_uri_set_port (_uri, _gdata_service_get_https_port ());
	message = soup_message_new_from_uri (SOUP_METHOD_POST, _uri);
	soup_uri_free (_uri);
	soup_message_set_request (message, "application/x-www-form-urlencoded", SOUP_MEMORY_TAKE, request_body, strlen (request_body));

	sign_message (self, message, token, token_secret, parameters);

	g_hash_table_destroy (parameters);

	/* Send the message */
	_gdata_service_actually_send_message (priv->session, message, cancellable, error);
	status = message->status_code;

	if (status == SOUP_STATUS_CANCELLED) {
		/* Cancelled (the error has already been set) */
		g_object_unref (message);
		return FALSE;
	} else if (status != SOUP_STATUS_OK) {
		/* Server returned an error. This either means that there was a server error or, more likely, the server doesn't trust the client
		 * or the user denied authorization to the token on the authorization web page. */
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_FORBIDDEN, _("Access was denied by the user or server."));
		g_object_unref (message);

		return FALSE;
	}

	g_assert (message->response_body->data != NULL);

	/* Parse the response. We expect something like:
	 *   oauth_token=ab3cd9j4ks73hf7g&oauth_token_secret=ZXhhbXBsZS5jb20&oauth_callback_confirmed=true
	 * See: http://code.google.com/apis/accounts/docs/OAuth_ref.html#AccessToken and
	 * http://tools.ietf.org/html/rfc5849#section-2.3 for details. */
	response_details = soup_form_decode (message->response_body->data);

	/* Zero out the response body to lower the chance of it (with all the auth. tokens it contains) hitting disk or getting leaked in
	 * free memory. */
	memset ((void*) message->response_body->data, 0, message->response_body->length);

	g_object_unref (message);

	_token = g_hash_table_lookup (response_details, "oauth_token");
	_token_secret = g_hash_table_lookup (response_details, "oauth_token_secret");

	/* Validate the returned values */
	if (_token == NULL || _token_secret == NULL ||
	    *_token == '\0' || *_token_secret == '\0') {
		g_set_error_literal (error, GDATA_SERVICE_ERROR, GDATA_SERVICE_ERROR_PROTOCOL_ERROR, _("The server returned a malformed response."));
		g_hash_table_destroy (response_details);

		return FALSE;
	}

	/* Store the token and token secret in the authoriser */
	g_mutex_lock (&(priv->mutex));

	g_free (priv->token);
	priv->token = g_strdup (_token);

	_gdata_service_secure_strfree (priv->token_secret);
	priv->token_secret = _gdata_service_secure_strdup (_token_secret);

	g_mutex_unlock (&(priv->mutex));

	/* Zero out the secret token before freeing the hash table, to reduce the chance of it hitting disk later. */
	memset ((void*) _token_secret, 0, strlen (_token_secret));

	g_hash_table_destroy (response_details);

	return TRUE;
}

typedef struct {
	/* Input */
	gchar *token;
	GDataSecureString token_secret; /* must be allocated by _gdata_service_secure_strdup() */
	gchar *verifier;
} RequestAuthorizationAsyncData;

static void
request_authorization_async_data_free (RequestAuthorizationAsyncData *data)
{
	g_free (data->token);
	_gdata_service_secure_strfree (data->token_secret);
	g_free (data->verifier);

	g_slice_free (RequestAuthorizationAsyncData, data);
}

static void
request_authorization_thread (GSimpleAsyncResult *result, GDataOAuth1Authorizer *authorizer, GCancellable *cancellable)
{
	RequestAuthorizationAsyncData *data;
	gboolean success;
	GError *error = NULL;

	data = g_simple_async_result_get_op_res_gpointer (result);

	success = gdata_oauth1_authorizer_request_authorization (authorizer, data->token, data->token_secret, data->verifier, cancellable, &error);
	g_simple_async_result_set_op_res_gboolean (result, success);

	if (error != NULL) {
		g_simple_async_result_set_from_error (result, error);
		g_error_free (error);
	}
}

/**
 * gdata_oauth1_authorizer_request_authorization_async:
 * @self: a #GDataOAuth1Authorizer
 * @token: the request token returned by gdata_oauth1_authorizer_request_authentication_uri()
 * @token_secret: the request token secret returned by gdata_oauth1_authorizer_request_authentication_uri()
 * @verifier: the verifier entered by the user from the authentication page
 * @cancellable: (allow-none): an optional #GCancellable, or %NULL
 * @callback: a #GAsyncReadyCallback to call when authorization is finished
 * @user_data: (closure): data to pass to the @callback function
 *
 * Requests authorization of the given request @token from the Google accounts service using the given @verifier as entered by the user.
 * @self, @token, @token_secret and @verifier are reffed/copied when this method is called, so can safely be freed after this method returns.
 *
 * For more details, see gdata_oauth1_authorizer_request_authorization(), which is the synchronous version of this method.
 *
 * When the operation is finished, @callback will be called. You can then call gdata_oauth1_authorizer_request_authorization_finish() to get the
 * results of the operation.
 *
 * Since: 0.9.0
 */
void
gdata_oauth1_authorizer_request_authorization_async (GDataOAuth1Authorizer *self, const gchar *token, const gchar *token_secret,
                                                     const gchar *verifier,
                                                     GCancellable *cancellable, GAsyncReadyCallback callback, gpointer user_data)
{
	GSimpleAsyncResult *result;
	RequestAuthorizationAsyncData *data;

	g_return_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self));
	g_return_if_fail (token != NULL && *token != '\0');
	g_return_if_fail (token_secret != NULL && *token_secret != '\0');
	g_return_if_fail (verifier != NULL && *verifier != '\0');
	g_return_if_fail (cancellable == NULL || G_IS_CANCELLABLE (cancellable));

	data = g_slice_new (RequestAuthorizationAsyncData);
	data->token = g_strdup (token);
	data->token_secret = _gdata_service_secure_strdup (token_secret);
	data->verifier = g_strdup (verifier);

	result = g_simple_async_result_new (G_OBJECT (self), callback, user_data, gdata_oauth1_authorizer_request_authorization_async);
	g_simple_async_result_set_op_res_gpointer (result, data, (GDestroyNotify) request_authorization_async_data_free);
	g_simple_async_result_run_in_thread (result, (GSimpleAsyncThreadFunc) request_authorization_thread, G_PRIORITY_DEFAULT, cancellable);
	g_object_unref (result);
}

/**
 * gdata_oauth1_authorizer_request_authorization_finish:
 * @self: a #GDataOAuth1Authorizer
 * @async_result: a #GAsyncResult
 * @error: a #GError, or %NULL
 *
 * Finishes an asynchronous authorization operation started with gdata_oauth1_authorizer_request_authorization_async().
 *
 * Return value: %TRUE if authorization was successful, %FALSE otherwise
 *
 * Since: 0.9.0
 */
gboolean
gdata_oauth1_authorizer_request_authorization_finish (GDataOAuth1Authorizer *self, GAsyncResult *async_result, GError **error)
{
	g_return_val_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self), FALSE);
	g_return_val_if_fail (G_IS_ASYNC_RESULT (async_result), FALSE);
	g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

	g_warn_if_fail (g_simple_async_result_is_valid (async_result, G_OBJECT (self), gdata_oauth1_authorizer_request_authorization_async));

	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (async_result), error) == TRUE) {
		return FALSE;
	}

	return TRUE;
}

/**
 * gdata_oauth1_authorizer_get_application_name:
 * @self: a #GDataOAuth1Authorizer
 *
 * Returns the application name being used on the authentication page at the URI returned by gdata_oauth1_authorizer_request_authentication_uri();
 * i.e. the value of #GDataOAuth1Authorizer:application-name.
 *
 * Return value: (allow-none): the application name, or %NULL if one isn't set
 *
 * Since: 0.9.0
 */
const gchar *
gdata_oauth1_authorizer_get_application_name (GDataOAuth1Authorizer *self)
{
	g_return_val_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self), NULL);
	return self->priv->application_name;
}

/**
 * gdata_oauth1_authorizer_get_locale:
 * @self: a #GDataOAuth1Authorizer
 *
 * Returns the locale currently being used for network requests, or %NULL if the locale is the default.
 *
 * Return value: (allow-none): the current locale
 *
 * Since: 0.9.0
 */
const gchar *
gdata_oauth1_authorizer_get_locale (GDataOAuth1Authorizer *self)
{
	g_return_val_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self), NULL);
	return self->priv->locale;
}

/**
 * gdata_oauth1_authorizer_set_locale:
 * @self: a #GDataOAuth1Authorizer
 * @locale: (allow-none): the new locale in Unix locale format, or %NULL for the default locale
 *
 * Set the locale used for network requests to @locale, given in standard Unix locale format. See #GDataOAuth1Authorizer:locale for more details.
 *
 * Note that while it's possible to change the locale after sending network requests (i.e. calling
 * gdata_oauth1_authorizer_request_authentication_uri() for the first time), it is unsupported, as the server-side software may behave unexpectedly.
 * The only supported use of this method is after creation of the authorizer, but before any network requests are made.
 *
 * Since: 0.9.0
 */
void
gdata_oauth1_authorizer_set_locale (GDataOAuth1Authorizer *self, const gchar *locale)
{
	g_return_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self));

	if (g_strcmp0 (locale, self->priv->locale) == 0) {
		/* Already has this value */
		return;
	}

	g_free (self->priv->locale);
	self->priv->locale = g_strdup (locale);
	g_object_notify (G_OBJECT (self), "locale");
}

static void
notify_proxy_uri_cb (GObject *gobject, GParamSpec *pspec, GDataOAuth1Authorizer *self)
{
	/* Flush our cached version */
	if (self->priv->proxy_uri != NULL) {
		soup_uri_free (self->priv->proxy_uri);
		self->priv->proxy_uri = NULL;
	}

	g_object_notify (G_OBJECT (self), "proxy-uri");
}

/**
 * gdata_oauth1_authorizer_get_proxy_uri:
 * @self: a #GDataOAuth1Authorizer
 *
 * Gets the proxy URI on the #GDataOAuth1Authorizer's #SoupSession.
 *
 * Return value: (transfer full) (allow-none): the proxy URI, or %NULL; free with soup_uri_free()
 *
 * Since: 0.9.0
 */
SoupURI *
gdata_oauth1_authorizer_get_proxy_uri (GDataOAuth1Authorizer *self)
{
	SoupURI *proxy_uri;

	g_return_val_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self), NULL);

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
 * gdata_oauth1_authorizer_set_proxy_uri:
 * @self: a #GDataOAuth1Authorizer
 * @proxy_uri: (allow-none): the proxy URI, or %NULL
 *
 * Sets the proxy URI on the #SoupSession used internally by the #GDataOAuth1Authorizer. This forces all requests through the given proxy.
 *
 * If @proxy_uri is %NULL, no proxy will be used.
 *
 * Since: 0.9.0
 */
void
gdata_oauth1_authorizer_set_proxy_uri (GDataOAuth1Authorizer *self, SoupURI *proxy_uri)
{
	g_return_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self));

	g_object_set (self->priv->session, SOUP_SESSION_PROXY_URI, proxy_uri, NULL);

	/* Notification is handled in notify_proxy_uri_cb() which is called as a result of setting the property on the session */
}

static void
notify_timeout_cb (GObject *gobject, GParamSpec *pspec, GObject *self)
{
	g_object_notify (self, "timeout");
}

/**
 * gdata_oauth1_authorizer_get_timeout:
 * @self: a #GDataOAuth1Authorizer
 *
 * Gets the #GDataOAuth1Authorizer:timeout property; the network timeout, in seconds.
 *
 * Return value: the timeout, or <code class="literal">0</code>
 *
 * Since: 0.9.0
 */
guint
gdata_oauth1_authorizer_get_timeout (GDataOAuth1Authorizer *self)
{
	guint timeout;

	g_return_val_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self), 0);

	g_object_get (self->priv->session, SOUP_SESSION_TIMEOUT, &timeout, NULL);

	return timeout;
}

/**
 * gdata_oauth1_authorizer_set_timeout:
 * @self: a #GDataOAuth1Authorizer
 * @timeout: the timeout, or <code class="literal">0</code>
 *
 * Sets the #GDataOAuth1Authorizer:timeout property; the network timeout, in seconds.
 *
 * If @timeout is <code class="literal">0</code>, network operations will never time out.
 *
 * Since: 0.9.0
 */
void
gdata_oauth1_authorizer_set_timeout (GDataOAuth1Authorizer *self, guint timeout)
{
	g_return_if_fail (GDATA_IS_OAUTH1_AUTHORIZER (self));

	if (gdata_oauth1_authorizer_get_timeout (self) == timeout) {
		/* Already has this value */
		return;
	}

	g_object_set (self->priv->session, SOUP_SESSION_TIMEOUT, timeout, NULL);

	/* Notification is handled in notify_timeout_cb() which is called as a result of setting the property on the session */
}
