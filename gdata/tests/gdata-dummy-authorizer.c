/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2014 <philip@tecnocode.co.uk>
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

/*
 * SECTION:gdata-dummy-authorizer
 * @short_description: GData dummy authorization interface
 * @stability: Stable
 * @include: tests/gdata-dummy-authorizer.h
 *
 * #GDataDummyAuthorizer is a dummy #GDataAuthorizer implementation intended for
 * use in prototyping and testing code. It should not be used in production
 * code.
 *
 * It adds a constant ‘Authorization’ header to all requests it processes whose
 * domain is in the set of domains the authorizer was initialised with. All
 * requests for other domains have no header added, and are considered
 * non-authorized.
 *
 * Since: 0.16.0
 */

#include <config.h>
#include <string.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "gdata-dummy-authorizer.h"
#include "gdata-private.h"

static void authorizer_init (GDataAuthorizerInterface *iface);
static void finalize (GObject *object);

static void process_request (GDataAuthorizer *self,
                             GDataAuthorizationDomain *domain,
                             SoupMessage *message);
static gboolean is_authorized_for_domain (GDataAuthorizer *self,
                                          GDataAuthorizationDomain *domain);

struct _GDataDummyAuthorizerPrivate {
	GMutex mutex; /* mutex for authorization_domains */

	/* Mapping from GDataAuthorizationDomain to itself; a set of domains
	 * which are authorised. */
	GHashTable *authorization_domains;
};

G_DEFINE_TYPE_WITH_CODE (GDataDummyAuthorizer, gdata_dummy_authorizer,
                         G_TYPE_OBJECT,
                         G_ADD_PRIVATE (GDataDummyAuthorizer)
                         G_IMPLEMENT_INTERFACE (GDATA_TYPE_AUTHORIZER,
                                                authorizer_init))

static void
gdata_dummy_authorizer_class_init (GDataDummyAuthorizerClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->finalize = finalize;
}

static void
authorizer_init (GDataAuthorizerInterface *iface)
{
	iface->process_request = process_request;
	iface->is_authorized_for_domain = is_authorized_for_domain;
}

static void
gdata_dummy_authorizer_init (GDataDummyAuthorizer *self)
{
	self->priv = gdata_dummy_authorizer_get_instance_private (self);

	/* Set up the authorizer's mutex */
	g_mutex_init (&(self->priv->mutex));
	self->priv->authorization_domains = g_hash_table_new_full (g_direct_hash,
	                                                           g_direct_equal,
	                                                           g_object_unref,
	                                                           NULL);
}

static void
finalize (GObject *object)
{
	GDataDummyAuthorizerPrivate *priv = GDATA_DUMMY_AUTHORIZER (object)->priv;

	g_hash_table_destroy (priv->authorization_domains);
	g_mutex_clear (&(priv->mutex));

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_dummy_authorizer_parent_class)->finalize (object);
}

static void
process_request (GDataAuthorizer *self, GDataAuthorizationDomain *domain,
                 SoupMessage *message)
{
	GDataDummyAuthorizerPrivate *priv = GDATA_DUMMY_AUTHORIZER (self)->priv;

	/* Set the authorisation header */
	g_mutex_lock (&(priv->mutex));

	if (g_hash_table_lookup (priv->authorization_domains, domain) != NULL) {
		soup_message_headers_replace (message->request_headers,
		                              "Authorization", "dummy");
	}

	g_mutex_unlock (&(priv->mutex));
}

static gboolean
is_authorized_for_domain (GDataAuthorizer *self,
                          GDataAuthorizationDomain *domain)
{
	GDataDummyAuthorizerPrivate *priv = GDATA_DUMMY_AUTHORIZER (self)->priv;
	gpointer result;

	g_mutex_lock (&(priv->mutex));
	result = g_hash_table_lookup (priv->authorization_domains, domain);
	g_mutex_unlock (&(priv->mutex));

	/* Sanity check */
	g_assert (result == NULL || result == domain);

	return (result != NULL);
}

/*
 * gdata_dummy_authorizer_new:
 * @service_type: the #GType of a #GDataService subclass which the
 * #GDataDummyAuthorizer will be used with
 *
 * Creates a new #GDataDummyAuthorizer.
 *
 * The #GDataAuthorizationDomains for the given @service_type (i.e. as
 * returned by gdata_service_get_authorization_domains()) will be authorized,
 * and all others will not.
 *
 * Return value: (transfer full): a new #GDataDummyAuthorizer; unref with
 * g_object_unref()
 *
 * Since: 0.16.0
 */
GDataDummyAuthorizer *
gdata_dummy_authorizer_new (GType service_type)
{
	GList/*<unowned GDataAuthorizationDomain>*/ *domains;  /* owned */
	GDataDummyAuthorizer *retval = NULL;  /* owned */

	g_return_val_if_fail (g_type_is_a (service_type, GDATA_TYPE_SERVICE),
	                      NULL);

	domains = gdata_service_get_authorization_domains (service_type);
	retval = gdata_dummy_authorizer_new_for_authorization_domains (domains);
	g_list_free (domains);

	return retval;
}

/*
 * gdata_dummy_authorizer_new_for_authorization_domains:
 * @authorization_domains: (element-type GDataAuthorizationDomain) (transfer none):
 * a non-empty list of #GDataAuthorizationDomains to be authorized
 * against by the #GDataDummyAuthorizer
 *
 * Creates a new #GDataDummyAuthorizer. This function is intended to be used
 * only when the default authorization domain list for a single #GDataService,
 * as used by gdata_dummy_authorizer_new(), isn't suitable. For example, this
 * could be because the #GDataDummyAuthorizer will be used with multiple
 * #GDataService subclasses, or because the client requires a specific set of
 * authorization domains.
 *
 * Return value: (transfer full): a new #GDataDummyAuthorizer; unref with
 * g_object_unref()
 *
 * Since: 0.16.0
 */
GDataDummyAuthorizer *
gdata_dummy_authorizer_new_for_authorization_domains (GList *authorization_domains)
{
	GList *i;
	GDataDummyAuthorizer *authorizer;

	g_return_val_if_fail (authorization_domains != NULL, NULL);

	authorizer = GDATA_DUMMY_AUTHORIZER (g_object_new (GDATA_TYPE_DUMMY_AUTHORIZER,
	                                                   NULL));

	/* Register all the domains with the authorizer */
	for (i = authorization_domains; i != NULL; i = i->next) {
		GDataAuthorizationDomain *domain;

		g_return_val_if_fail (GDATA_IS_AUTHORIZATION_DOMAIN (i->data),
		                      NULL);

		/* We don't have to lock the authoriser's mutex here as no other
		 * code has seen the authoriser yet */
		domain = GDATA_AUTHORIZATION_DOMAIN (i->data);
		g_hash_table_insert (authorizer->priv->authorization_domains,
		                     g_object_ref (domain), domain);
	}

	return authorizer;
}
