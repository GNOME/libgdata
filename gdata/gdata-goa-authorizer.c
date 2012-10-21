/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Matthew Barnes 2011 <mbarnes@redhat.com>
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
 * SECTION:gdata-goa-authorizer
 * @short_description: GData GOA authorization interface
 * @stability: Unstable
 * @include: gdata/gdata-goa-authorizer.h
 *
 * #GDataGoaAuthorizer provides an implementation of the #GDataAuthorizer interface for authentication and authorization using GNOME Online Accounts
 * (GOA) over D-Bus. This allows a single login session (managed by the GOA daemon) to be used by multiple applications simultaneously, without each
 * of those applications having to go through the authentication process themselves. Applications making use of #GDataGoaAuthorizer don't get access
 * to the user's password (it's handled solely by the GOA daemon).
 *
 * Internally, GOA authenticates with the Google servers using the
 * <ulink type="http" url="http://code.google.com/apis/accounts/docs/OAuthForInstalledApps.html">OAuth 1.0</ulink> process.
 *
 * #GDataGoaAuthorizer natively supports authorization against multiple services (unlike #GDataClientLoginAuthorizer), depending entirely on which
 * services the user has enabled for their Google account in GOA. #GDataGoaAuthorizer cannot authenticate for more services than are enabled in GOA.
 *
 * <example>
 *	<title>Authenticating Using GOA</title>
 *	<programlisting>
 *	GDataSomeService *service;
 *	GoaObject *goa_object;
 *	GDataGoaAuthorizer *authorizer;
 *
 *	/<!-- -->* Create an authorizer and pass it an existing #GoaObject. *<!-- -->/
 *	goa_object = get_goa_object ();
 *	authorizer = gdata_goa_authorizer_new (goa_object);
 *
 *	/<!-- -->* Create a service object and link it with the authorizer *<!-- -->/
 *	service = gdata_some_service_new (GDATA_AUTHORIZER (authorizer));
 *
 *	/<!-- -->* Use the service! *<!-- -->/
 *
 *	g_object_unref (service);
 *	g_object_unref (authorizer);
 *	g_object_unref (goa_object);
 *	</programlisting>
 * </example>
 *
 * Since: 0.13.1
 */

#include <stdlib.h>
#include <string.h>
#include <oauth.h>
#include <glib.h>

#include "gdata-goa-authorizer.h"
#include "gdata-authorizer.h"
#include "gdata-service.h"

#include "services/calendar/gdata-calendar-service.h"
#include "services/contacts/gdata-contacts-service.h"
#include "services/documents/gdata-documents-service.h"

#define HMAC_SHA1_LEN 20 /* bytes, raw */

static void gdata_goa_authorizer_interface_init (GDataAuthorizerInterface *interface);

/* GDataAuthorizer methods must be thread-safe. */
static GStaticMutex mutex = G_STATIC_MUTEX_INIT;

struct _GDataGoaAuthorizerPrivate {
	/* GoaObject is already thread-safe. */
	GoaObject *goa_object;

	/* These members are protected by the global mutex (above). */
	gchar *access_token;
	gchar *access_token_secret;
	GHashTable *authorization_domains;
};

enum {
	PROP_GOA_OBJECT = 1,
};

G_DEFINE_TYPE_WITH_CODE (GDataGoaAuthorizer, gdata_goa_authorizer, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_AUTHORIZER, gdata_goa_authorizer_interface_init))

static GHashTable *
gdata_goa_authorizer_get_oauth1_parameters (SoupMessage *message, const gchar *consumer_key, const gchar *consumer_secret, const gchar *access_token,
                                            const gchar *access_token_secret)
{
	GString *query;
	GString *base_string;
	GString *signing_key;
	GHashTable *parameters;
	GHashTableIter iter;
	SoupURI *soup_uri;
	GList *keys, *i;
	gchar *string;
	gchar *request_uri;
	gpointer key;
	guchar signature_buf[HMAC_SHA1_LEN];
	gsize signature_buf_len;
	GHmac *signature_hmac;

	parameters = g_hash_table_new_full ((GHashFunc) g_str_hash, (GEqualFunc) g_str_equal, (GDestroyNotify) NULL, (GDestroyNotify) g_free);

	/* soup_form_decode() uses an awkward allocation style for
	 * its hash table entries, so it's easier to copy its content
	 * into our own hash table than try to use the returned hash
	 * table directly. */

	soup_uri = soup_message_get_uri (message);
	if (soup_uri->query != NULL) {
		GHashTable *hash_table;
		gpointer value;

		hash_table = soup_form_decode (soup_uri->query);
		g_hash_table_iter_init (&iter, hash_table);
		while (g_hash_table_iter_next (&iter, &key, &value)) {
			key = (gpointer) g_intern_string (key);
			value = g_strdup (value);
			g_hash_table_insert (parameters, key, value);
		}
		g_hash_table_destroy (hash_table);
	}

	/* Add OAuth parameters. */

	key = (gpointer) "oauth_version";
	g_hash_table_insert (parameters, key, g_strdup ("1.0"));

	string = oauth_gen_nonce ();
	key = (gpointer) "oauth_nonce";
	g_hash_table_insert (parameters, key, g_strdup (string));
	free (string);  /* do not use g_free () */

	key = (gpointer) "oauth_timestamp";
	string = g_strdup_printf ("%" G_GINT64_FORMAT, (gint64) time (NULL));
	g_hash_table_insert (parameters, key, string); /* takes ownership */

	key = (gpointer) "oauth_consumer_key";
	g_hash_table_insert (parameters, key, g_strdup (consumer_key));

	key = (gpointer) "oauth_token";
	g_hash_table_insert (parameters, key, g_strdup (access_token));

	key = (gpointer) "oauth_signature_method";
	g_hash_table_insert (parameters, key, g_strdup ("HMAC-SHA1"));

	/* Build the query part of the signature base string. */

	query = g_string_sized_new (512);
	keys = g_hash_table_get_keys (parameters);
	keys = g_list_sort (keys, (GCompareFunc) g_strcmp0);
	for (i = keys; i != NULL; i = g_list_next (i)) {
		const gchar *_key = i->data;
		const gchar *val;

		val = g_hash_table_lookup (parameters, _key);

		if (i != keys) {
			g_string_append_c (query, '&');
		}

		g_string_append_uri_escaped (query, _key, NULL, FALSE);
		g_string_append_c (query, '=');
		g_string_append_uri_escaped (query, val, NULL, FALSE);
	}
	g_list_free (keys);

	/* Build the signature base string. */

	soup_uri = soup_uri_copy (soup_uri);
	soup_uri_set_query (soup_uri, NULL);
	soup_uri_set_fragment (soup_uri, NULL);
	request_uri = soup_uri_to_string (soup_uri, FALSE);
	soup_uri_free (soup_uri);

	base_string = g_string_sized_new (512);
	g_string_append_uri_escaped (base_string, message->method, NULL, FALSE);
	g_string_append_c (base_string, '&');
	g_string_append_uri_escaped (base_string, request_uri, NULL, FALSE);
	g_string_append_c (base_string, '&');
	g_string_append_uri_escaped (base_string, query->str, NULL, FALSE);

	/* Build the HMAC-SHA1 signing key. */

	signing_key = g_string_sized_new (512);
	g_string_append_uri_escaped (signing_key, consumer_secret, NULL, FALSE);
	g_string_append_c (signing_key, '&');
	g_string_append_uri_escaped (signing_key, access_token_secret, NULL, FALSE);

	/* Sign the request. */
	signature_hmac = g_hmac_new (G_CHECKSUM_SHA1, (const guchar*) signing_key->str, signing_key->len);
	g_hmac_update (signature_hmac, (const guchar*) base_string->str, base_string->len);

	signature_buf_len = G_N_ELEMENTS (signature_buf);
	g_hmac_get_digest (signature_hmac, signature_buf, &signature_buf_len);

	g_hmac_unref (signature_hmac);

	key = (gpointer) "oauth_signature";
	string = g_base64_encode (signature_buf, signature_buf_len);
	g_hash_table_insert (parameters, key, g_strdup (string));
	g_free (string);

	g_free (request_uri);

	g_string_free (query, TRUE);
	g_string_free (base_string, TRUE);
	g_string_free (signing_key, TRUE);

	return parameters;
}

static void
gdata_goa_authorizer_add_oauth1_authorization (GDataAuthorizer *authorizer, SoupMessage *message)
{
	GDataGoaAuthorizerPrivate *priv;
	GoaOAuthBased *goa_oauth_based;
	GHashTable *parameters;
	GString *authorization;
	const gchar *consumer_key;
	const gchar *consumer_secret;
	guint ii;

	const gchar *oauth_keys[] = {
		"oauth_version",
		"oauth_nonce",
		"oauth_timestamp",
		"oauth_consumer_key",
		"oauth_token",
		"oauth_signature_method",
		"oauth_signature"
	};

	/* This MUST be called with the mutex already locked. */

	priv = GDATA_GOA_AUTHORIZER (authorizer)->priv;

	/* We can't add an Authorization header without an access token.
	 * Let the request fail.  GData should refresh us if it gets back
	 * a "401 Authorization required" response from Google, and then
	 * automatically retry the request. */
	if (priv->access_token == NULL) {
		return;
	}

	goa_oauth_based = goa_object_get_oauth_based (priv->goa_object);

	consumer_key = goa_oauth_based_get_consumer_key (goa_oauth_based);
	consumer_secret = goa_oauth_based_get_consumer_secret (goa_oauth_based);

	parameters = gdata_goa_authorizer_get_oauth1_parameters (message, consumer_key, consumer_secret,
	                                                         priv->access_token, priv->access_token_secret);

	authorization = g_string_new ("OAuth ");

	for (ii = 0; ii < G_N_ELEMENTS (oauth_keys); ii++) {
		const gchar *key;
		const gchar *val;

		key = oauth_keys[ii];
		val = g_hash_table_lookup (parameters, key);

		if (ii > 0) {
			g_string_append (authorization, ", ");
		}

		g_string_append (authorization, key);
		g_string_append_c (authorization, '=');
		g_string_append_c (authorization, '"');
		g_string_append_uri_escaped (authorization, val, NULL, FALSE);
		g_string_append_c (authorization, '"');
	}

	/* Use replace here, not append, to make sure
	 * there's only one "Authorization" header. */
	soup_message_headers_replace (message->request_headers, "Authorization", authorization->str);

	g_string_free (authorization, TRUE);
	g_hash_table_destroy (parameters);

	g_object_unref (goa_oauth_based);
}

static void
gdata_goa_authorizer_add_oauth2_authorization (GDataAuthorizer *authorizer, SoupMessage *message)
{
	GDataGoaAuthorizerPrivate *priv;
	GString *authorization;

	/* This MUST be called with the mutex already locked. */

	priv = GDATA_GOA_AUTHORIZER (authorizer)->priv;

	/* We can't add an Authorization header without an access token. Let the request fail. GData should refresh us if it gets back a
	 * "401 Authorization required" response from Google, and then automatically retry the request. */
	if (priv->access_token == NULL) {
		return;
	}

	authorization = g_string_new ("OAuth ");
	g_string_append (authorization, priv->access_token);

	/* Use replace here, not append, to make sure there's only one "Authorization" header. */
	soup_message_headers_replace (message->request_headers, "Authorization", authorization->str);

	g_string_free (authorization, TRUE);
}

static void
gdata_goa_authorizer_add_authorization (GDataAuthorizer *authorizer, SoupMessage *message)
{
	GDataGoaAuthorizerPrivate *priv;

	/* This MUST be called with the mutex already locked. */

	priv = GDATA_GOA_AUTHORIZER (authorizer)->priv;

	/* Prefer OAuth 2.0 over OAuth 1.0. */
	if (goa_object_peek_oauth2_based (priv->goa_object) != NULL) {
		gdata_goa_authorizer_add_oauth2_authorization (authorizer, message);
	} else if (goa_object_peek_oauth_based (priv->goa_object) != NULL) {
		gdata_goa_authorizer_add_oauth1_authorization (authorizer, message);
	}
}

static gboolean
gdata_goa_authorizer_is_authorized (GDataAuthorizer *authorizer, GDataAuthorizationDomain *domain)
{
	/* This MUST be called with the mutex already locked. */

	if (domain == NULL) {
		return TRUE;
	}

	domain = g_hash_table_lookup (GDATA_GOA_AUTHORIZER (authorizer)->priv->authorization_domains, domain);

	return (domain != NULL);
}

static void
add_authorization_domains (GDataGoaAuthorizer *self, GType service_type)
{
	GList/*<GDataAuthorizationDomain>*/ *domains;

	domains = gdata_service_get_authorization_domains (service_type);

	while (domains != NULL) {
		g_hash_table_insert (self->priv->authorization_domains, g_object_ref (domains->data), domains->data);
		domains = g_list_delete_link (domains, domains);
	}
}

static void
gdata_goa_authorizer_set_goa_object (GDataGoaAuthorizer *self, GoaObject *goa_object)
{
	g_return_if_fail (GOA_IS_OBJECT (goa_object));
	g_return_if_fail (self->priv->goa_object == NULL);

	self->priv->goa_object = g_object_ref (goa_object);

	/* Add authorisation domains for all the services supported by our GoaObject. */
	if (goa_object_peek_calendar (goa_object) != NULL) {
		add_authorization_domains (self, GDATA_TYPE_CALENDAR_SERVICE);
	}

	if (goa_object_peek_contacts (goa_object) != NULL) {
		add_authorization_domains (self, GDATA_TYPE_CONTACTS_SERVICE);
	}

	if (goa_object_peek_documents (goa_object) != NULL) {
		add_authorization_domains (self, GDATA_TYPE_DOCUMENTS_SERVICE);
	}
}

static void
gdata_goa_authorizer_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_GOA_OBJECT:
			gdata_goa_authorizer_set_goa_object (GDATA_GOA_AUTHORIZER (object), g_value_get_object (value));
			return;
		default:
			g_assert_not_reached ();
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
gdata_goa_authorizer_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_GOA_OBJECT:
			g_value_set_object (value, gdata_goa_authorizer_get_goa_object (GDATA_GOA_AUTHORIZER (object)));
			return;
		default:
			g_assert_not_reached ();
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
gdata_goa_authorizer_dispose (GObject *object)
{
	GDataGoaAuthorizerPrivate *priv;

	priv = GDATA_GOA_AUTHORIZER (object)->priv;

	g_clear_object (&priv->goa_object);
	g_hash_table_remove_all (priv->authorization_domains);

	/* Chain up to parent's dispose() method. */
	G_OBJECT_CLASS (gdata_goa_authorizer_parent_class)->dispose (object);
}

static void
gdata_goa_authorizer_finalize (GObject *object)
{
	GDataGoaAuthorizerPrivate *priv;

	priv = GDATA_GOA_AUTHORIZER (object)->priv;

	g_free (priv->access_token);
	g_free (priv->access_token_secret);
	g_hash_table_destroy (priv->authorization_domains);

	/* Chain up to parent's finalize() method. */
	G_OBJECT_CLASS (gdata_goa_authorizer_parent_class)->finalize (object);
}

static void
gdata_goa_authorizer_process_request (GDataAuthorizer *authorizer, GDataAuthorizationDomain *domain, SoupMessage *message)
{
	g_static_mutex_lock (&mutex);

	if (gdata_goa_authorizer_is_authorized (authorizer, domain)) {
		gdata_goa_authorizer_add_authorization (authorizer, message);
	}

	g_static_mutex_unlock (&mutex);
}

static gboolean
gdata_goa_authorizer_is_authorized_for_domain (GDataAuthorizer *authorizer, GDataAuthorizationDomain *domain)
{
	gboolean authorized;

	g_static_mutex_lock (&mutex);

	authorized = gdata_goa_authorizer_is_authorized (authorizer, domain);

	g_static_mutex_unlock (&mutex);

	return authorized;
}

static gboolean
gdata_goa_authorizer_refresh_authorization (GDataAuthorizer *authorizer, GCancellable *cancellable, GError **error)
{
	GDataGoaAuthorizerPrivate *priv;
	GoaOAuthBased *goa_oauth1_based;
	GoaOAuth2Based *goa_oauth2_based;
	GoaAccount *goa_account;
	gboolean success = FALSE;

	priv = GDATA_GOA_AUTHORIZER (authorizer)->priv;

	g_static_mutex_lock (&mutex);

	g_free (priv->access_token);
	priv->access_token = NULL;

	g_free (priv->access_token_secret);
	priv->access_token_secret = NULL;

	goa_account = goa_object_get_account (priv->goa_object);
	goa_oauth1_based = goa_object_get_oauth_based (priv->goa_object);
	goa_oauth2_based = goa_object_get_oauth2_based (priv->goa_object);

	success = goa_account_call_ensure_credentials_sync (goa_account, NULL, cancellable, error);

	if (success == FALSE) {
		goto exit;
	}

	/* Prefer OAuth 2.0 over OAuth 1.0. */
	if (goa_oauth2_based != NULL) {
		success = goa_oauth2_based_call_get_access_token_sync (goa_oauth2_based, &priv->access_token, NULL, cancellable, error);
	} else if (goa_oauth1_based != NULL) {
		success = goa_oauth_based_call_get_access_token_sync (goa_oauth1_based, &priv->access_token, &priv->access_token_secret, NULL,
		                                                      cancellable, error);
	} else {
		g_warn_if_reached (); /* should never happen */
	}

exit:
	g_clear_object (&goa_account);
	g_clear_object (&goa_oauth1_based);
	g_clear_object (&goa_oauth2_based);

	g_static_mutex_unlock (&mutex);

	return success;
}

static void
gdata_goa_authorizer_class_init (GDataGoaAuthorizerClass *class)
{
	GObjectClass *object_class;

	g_type_class_add_private (class, sizeof (GDataGoaAuthorizerPrivate));

	object_class = G_OBJECT_CLASS (class);
	object_class->set_property = gdata_goa_authorizer_set_property;
	object_class->get_property = gdata_goa_authorizer_get_property;
	object_class->dispose = gdata_goa_authorizer_dispose;
	object_class->finalize = gdata_goa_authorizer_finalize;

	/**
	 * GDataGoaAuthorizer:goa-object:
	 *
	 * The GOA account providing authentication. This should have all the necessary services enabled on it.
	 *
	 * Since: 0.13.1
	 */
	g_object_class_install_property (object_class, PROP_GOA_OBJECT,
	                                 g_param_spec_object ("goa-object", "GOA object", "The GOA account providing authentication.",
	                                                      GOA_TYPE_OBJECT, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_STRINGS));
}

static void
gdata_goa_authorizer_interface_init (GDataAuthorizerInterface *interface)
{
	interface->process_request = gdata_goa_authorizer_process_request;
	interface->is_authorized_for_domain = gdata_goa_authorizer_is_authorized_for_domain;
	interface->refresh_authorization = gdata_goa_authorizer_refresh_authorization;
}

static void
gdata_goa_authorizer_init (GDataGoaAuthorizer *authorizer)
{
	GHashTable *authorization_domains;

	authorization_domains = g_hash_table_new_full ((GHashFunc) g_direct_hash, (GEqualFunc) g_direct_equal,
	                                               (GDestroyNotify) g_object_unref, (GDestroyNotify) NULL);

	authorizer->priv = G_TYPE_INSTANCE_GET_PRIVATE (authorizer, GDATA_TYPE_GOA_AUTHORIZER, GDataGoaAuthorizerPrivate);
	authorizer->priv->authorization_domains = authorization_domains;
}

/**
 * gdata_goa_authorizer_new:
 * @goa_object: (transfer none): the GOA account providing authentication
 *
 * Create a new #GDataGoaAuthorizer using the authentication token from the given @goa_object.
 *
 * Return value: (transfer full): a new #GDataGoaAuthorizer; unref with g_object_unref()
 *
 * Since: 0.13.1
 */
GDataGoaAuthorizer *
gdata_goa_authorizer_new (GoaObject *goa_object)
{
	g_return_val_if_fail (GOA_IS_OBJECT (goa_object), NULL);

	return g_object_new (GDATA_TYPE_GOA_AUTHORIZER, "goa-object", goa_object, NULL);
}

/**
 * gdata_goa_authorizer_get_goa_object:
 * @self: a #GDataGoaAuthorizer
 *
 * The GOA account providing authentication. This is the same as #GDataGoaAuthorizer:goa-object.
 *
 * Return value: (transfer none): the GOA account providing authentication
 *
 * Since: 0.13.1
 */
GoaObject *
gdata_goa_authorizer_get_goa_object (GDataGoaAuthorizer *self)
{
	g_return_val_if_fail (GDATA_IS_GOA_AUTHORIZER (self), NULL);

	return self->priv->goa_object;
}
