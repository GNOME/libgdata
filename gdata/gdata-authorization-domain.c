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
 * SECTION:gdata-authorization-domain
 * @short_description: GData authorization domain
 * @stability: Stable
 * @include: gdata/gdata-authorization-domain.h
 *
 * A #GDataAuthorizationDomain represents a single data domain which a user can authorize libgdata to access. This might be a domain covering the
 * whole of the user's Google Contacts account, for example. Typically, #GDataServices map to #GDataAuthorizationDomains in a one-to-one
 * fashion, though some services (such as #GDataDocumentsService) use multiple authorization domains.
 *
 * The #GDataAuthorizationDomainss used by a service can be retrieved using gdata_service_get_authorization_domains(). The set of domains
 * used by a given service is static and will never change at runtime.
 *
 * #GDataAuthorizationDomains are used by a #GDataAuthorizer instance to request authorization to interact with data in those domains when
 * first authenticating and authorizing with the online service. Typically, a given #GDataAuthorizer will be passed a set of domains (or a service
 * type, from which it can retrieve the service's set of domains) at construction time, and will use those domains when initially asking the user for
 * authorization and whenever the authorization is refreshed afterwards. It's not expected that the set of domains used by a #GDataAuthorizer will
 * change after construction time.
 *
 * Note that it's not expected that #GDataAuthorizationDomains will have to be constructed manually. All #GDataServices should provide
 * accessor functions to return instances of all the authorization domains they support.
 *
 * Since: 0.9.0
 */

#include <glib.h>

#include "gdata-authorization-domain.h"

static void finalize (GObject *object);
static void get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

struct _GDataAuthorizationDomainPrivate {
	gchar *service_name;
	gchar *scope;
};

enum {
	PROP_SERVICE_NAME = 1,
	PROP_SCOPE,
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataAuthorizationDomain, gdata_authorization_domain, G_TYPE_OBJECT)

static void
gdata_authorization_domain_class_init (GDataAuthorizationDomainClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->get_property = get_property;
	gobject_class->set_property = set_property;
	gobject_class->finalize = finalize;

	/**
	 * GDataAuthorizationDomain:service-name:
	 *
	 * The name of the service which contains the authorization domain, as enumerated in the
	 * <ulink type="http" url="http://code.google.com/apis/documents/faq_gdata.html#clientlogin">online documentation</ulink>.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_SERVICE_NAME,
	                                 g_param_spec_string ("service-name",
	                                                      "Service name", "The name of the service which contains the authorization domain.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

	/**
	 * GDataAuthorizationDomain:scope:
	 *
	 * A URI detailing the scope of the authorization domain, as enumerated in the
	 * <ulink type="http" url="http://code.google.com/apis/documents/faq_gdata.html#AuthScopes">online documentation</ulink>.
	 *
	 * Since: 0.9.0
	 */
	g_object_class_install_property (gobject_class, PROP_SCOPE,
	                                 g_param_spec_string ("scope",
	                                                      "Scope", "A URI detailing the scope of the authorization domain.",
	                                                      NULL,
	                                                      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_authorization_domain_init (GDataAuthorizationDomain *self)
{
	self->priv = gdata_authorization_domain_get_instance_private (self);
}

static void
finalize (GObject *object)
{
	GDataAuthorizationDomainPrivate *priv = GDATA_AUTHORIZATION_DOMAIN (object)->priv;

	g_free (priv->service_name);
	g_free (priv->scope);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_authorization_domain_parent_class)->finalize (object);
}

static void
get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataAuthorizationDomainPrivate *priv = GDATA_AUTHORIZATION_DOMAIN (object)->priv;

	switch (property_id) {
		case PROP_SERVICE_NAME:
			g_value_set_string (value, priv->service_name);
			break;
		case PROP_SCOPE:
			g_value_set_string (value, priv->scope);
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
	GDataAuthorizationDomainPrivate *priv = GDATA_AUTHORIZATION_DOMAIN (object)->priv;

	switch (property_id) {
		/* Construct only */
		case PROP_SERVICE_NAME:
			priv->service_name = g_value_dup_string (value);
			break;
		/* Construct only */
		case PROP_SCOPE:
			priv->scope = g_value_dup_string (value);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/**
 * gdata_authorization_domain_get_service_name:
 * @self: a #GDataAuthorizationDomain
 *
 * Returns the name of the service containing the authorization domain. See #GDataAuthorizationDomain:service-name for more details.
 *
 * Return value: name of the service containing the authorization domain
 *
 * Since: 0.9.0
 */
const gchar *
gdata_authorization_domain_get_service_name (GDataAuthorizationDomain *self)
{
	g_return_val_if_fail (GDATA_IS_AUTHORIZATION_DOMAIN (self), NULL);

	return self->priv->service_name;
}

/**
 * gdata_authorization_domain_get_scope:
 * @self: a #GDataAuthorizationDomain
 *
 * Returns a URI detailing the scope of the authorization domain. See #GDataAuthorizationDomain:scope for more details.
 *
 * Return value: URI detailing the scope of the authorization domain
 *
 * Since: 0.9.0
 */
const gchar *
gdata_authorization_domain_get_scope (GDataAuthorizationDomain *self)
{
	g_return_val_if_fail (GDATA_IS_AUTHORIZATION_DOMAIN (self), NULL);

	return self->priv->scope;
}
