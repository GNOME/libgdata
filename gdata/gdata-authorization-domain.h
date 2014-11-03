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

#ifndef GDATA_AUTHORIZATION_DOMAIN_H
#define GDATA_AUTHORIZATION_DOMAIN_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GDATA_TYPE_AUTHORIZATION_DOMAIN		(gdata_authorization_domain_get_type ())
#define GDATA_AUTHORIZATION_DOMAIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_AUTHORIZATION_DOMAIN, GDataAuthorizationDomain))
#define GDATA_AUTHORIZATION_DOMAIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_AUTHORIZATION_DOMAIN, GDataAuthorizationDomainClass))
#define GDATA_IS_AUTHORIZATION_DOMAIN(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_AUTHORIZATION_DOMAIN))
#define GDATA_IS_AUTHORIZATION_DOMAIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_AUTHORIZATION_DOMAIN))
#define GDATA_AUTHORIZATION_DOMAIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_AUTHORIZATION_DOMAIN, GDataAuthorizationDomainClass))

typedef struct _GDataAuthorizationDomainPrivate	GDataAuthorizationDomainPrivate;

/**
 * GDataAuthorizationDomain:
 *
 * All the fields in the #GDataAuthorizationDomain structure are private and should never be accessed directly.
 *
 * Since: 0.9.0
 */
typedef struct {
	/*< private >*/
	GObject parent;
	GDataAuthorizationDomainPrivate *priv;
} GDataAuthorizationDomain;

/**
 * GDataAuthorizationDomainClass:
 *
 * All the fields in the #GDataAuthorizationDomainClass structure are private and should never be accessed directly.
 *
 * Since: 0.9.0
 */
typedef struct {
	/*< private >*/
	GObjectClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
	void (*_g_reserved2) (void);
	void (*_g_reserved3) (void);
	void (*_g_reserved4) (void);
	void (*_g_reserved5) (void);
} GDataAuthorizationDomainClass;

GType gdata_authorization_domain_get_type (void) G_GNUC_CONST;

const gchar *gdata_authorization_domain_get_service_name (GDataAuthorizationDomain *self) G_GNUC_PURE;
const gchar *gdata_authorization_domain_get_scope (GDataAuthorizationDomain *self) G_GNUC_PURE;

G_END_DECLS

#endif /* !GDATA_AUTHORIZATION_DOMAIN_H */
