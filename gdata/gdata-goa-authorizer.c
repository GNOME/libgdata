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
 * @stability: Stable
 * @include: gdata/gdata-goa-authorizer.h
 *
 * #GDataGoaAuthorizer provides an implementation of the #GDataAuthorizer interface for authentication and authorization using GNOME Online Accounts
 * (GOA) over D-Bus. This allows a single login session (managed by the GOA daemon) to be used by multiple applications simultaneously, without each
 * of those applications having to go through the authentication process themselves. Applications making use of #GDataGoaAuthorizer don't get access
 * to the user's password (it's handled solely by the GOA daemon).
 *
 * Internally, GOA authenticates with the Google servers using the
 * <ulink type="http" url="http://code.google.com/apis/accounts/docs/OAuthForInstalledApps.html">OAuth 1.0</ulink> or
 * <ulink type="http" url="https://developers.google.com/identity/protocols/OAuth2">OAuth 2.0</ulink> processes.
 *
 * #GDataGoaAuthorizer natively supports authorization against multiple services, depending entirely on which
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

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "gdata-goa-authorizer.h"
#include "gdata-authorizer.h"
#include "gdata-service.h"

#include "services/calendar/gdata-calendar-service.h"
#include "services/documents/gdata-documents-service.h"
#include "services/picasaweb/gdata-picasaweb-service.h"

#define HMAC_SHA1_LEN 20 /* bytes, raw */

static void gdata_goa_authorizer_interface_init (GDataAuthorizerInterface *interface);

/* GDataAuthorizer methods must be thread-safe. */
static GMutex mutex;

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
                         G_ADD_PRIVATE (GDataGoaAuthorizer)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_AUTHORIZER, gdata_goa_authorizer_interface_init))

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

	authorization = g_string_new ("Bearer ");
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

	/* Only support OAuth 2.0. OAuth 1.0 was deprecated in 2012. */
	if (goa_object_peek_oauth2_based (priv->goa_object) != NULL) {
		gdata_goa_authorizer_add_oauth2_authorization (authorizer, message);
	} else {
		g_warn_if_reached ();
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

	if (goa_object_peek_documents (goa_object) != NULL || goa_object_peek_files (goa_object) != NULL) {
		add_authorization_domains (self, GDATA_TYPE_DOCUMENTS_SERVICE);
	}
	
	if (goa_object_peek_photos (goa_object) != NULL) {
		add_authorization_domains (self, GDATA_TYPE_PICASAWEB_SERVICE);
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
	g_mutex_lock (&mutex);

	if (gdata_goa_authorizer_is_authorized (authorizer, domain)) {
		gdata_goa_authorizer_add_authorization (authorizer, message);
	}

	g_mutex_unlock (&mutex);
}

static gboolean
gdata_goa_authorizer_is_authorized_for_domain (GDataAuthorizer *authorizer, GDataAuthorizationDomain *domain)
{
	gboolean authorized;

	g_mutex_lock (&mutex);

	authorized = gdata_goa_authorizer_is_authorized (authorizer, domain);

	g_mutex_unlock (&mutex);

	return authorized;
}

static gboolean
gdata_goa_authorizer_refresh_authorization (GDataAuthorizer *authorizer, GCancellable *cancellable, GError **error)
{
	GDataGoaAuthorizerPrivate *priv;
	GoaOAuth2Based *goa_oauth2_based;
	GoaAccount *goa_account;
	gboolean success = FALSE;

	priv = GDATA_GOA_AUTHORIZER (authorizer)->priv;

	g_mutex_lock (&mutex);

	g_free (priv->access_token);
	priv->access_token = NULL;

	g_free (priv->access_token_secret);
	priv->access_token_secret = NULL;

	goa_account = goa_object_get_account (priv->goa_object);
	goa_oauth2_based = goa_object_get_oauth2_based (priv->goa_object);

	success = goa_account_call_ensure_credentials_sync (goa_account, NULL, cancellable, error);

	if (success == FALSE) {
		goto exit;
	}

	/* Only support OAuth 2.0. */
	if (goa_oauth2_based != NULL) {
		success = goa_oauth2_based_call_get_access_token_sync (goa_oauth2_based, &priv->access_token, NULL, cancellable, error);
	} else {
		g_warn_if_reached (); /* should never happen */
	}

exit:
	g_clear_object (&goa_account);
	g_clear_object (&goa_oauth2_based);

	g_mutex_unlock (&mutex);

	return success;
}

static void
gdata_goa_authorizer_class_init (GDataGoaAuthorizerClass *class)
{
	GObjectClass *object_class;

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

	authorizer->priv = gdata_goa_authorizer_get_instance_private (authorizer);
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
