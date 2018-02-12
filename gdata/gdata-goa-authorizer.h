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

#ifndef GDATA_GOA_AUTHORIZER_H
#define GDATA_GOA_AUTHORIZER_H

#include <glib.h>
#include <glib-object.h>

#define GOA_API_IS_SUBJECT_TO_CHANGE
#include <goa/goa.h>

/* Standard GObject macros */
#define GDATA_TYPE_GOA_AUTHORIZER		(gdata_goa_authorizer_get_type ())
#define GDATA_GOA_AUTHORIZER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GDATA_TYPE_GOA_AUTHORIZER, GDataGoaAuthorizer))
#define GDATA_GOA_AUTHORIZER_CLASS(cls)		(G_TYPE_CHECK_CLASS_CAST ((cls), GDATA_TYPE_GOA_AUTHORIZER, GDataGoaAuthorizerClass))
#define GDATA_IS_GOA_AUTHORIZER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GDATA_TYPE_GOA_AUTHORIZER))
#define GDATA_IS_GOA_AUTHORIZER_CLASS(cls)	(G_TYPE_CHECK_CLASS_TYPE ((cls), GDATA_TYPE_GOA_AUTHORIZER))
#define GDATA_GOA_AUTHORIZER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GDATA_TYPE_GOA_AUTHORIZER, GDataGoaAuthorizerClass))

G_BEGIN_DECLS

typedef struct _GDataGoaAuthorizerPrivate	GDataGoaAuthorizerPrivate;

/**
 * GDataGoaAuthorizer:
 *
 * All the fields in the #GDataGoaAuthorizer structure are private and should never be accessed directly.
 *
 * Since: 0.13.1
 */
typedef struct {
	/*< private >*/
	GObject parent;
	GDataGoaAuthorizerPrivate *priv;
} GDataGoaAuthorizer;

/**
 * GDataGoaAuthorizerClass:
 *
 * All the fields in the #GDataGoaAuthorizerClass structure are private and should never be accessed directly.
 *
 * Since: 0.13.1
 */
typedef struct {
	/*< private >*/
	GObjectClass parent_class;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataGoaAuthorizerClass;

GType gdata_goa_authorizer_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataGoaAuthorizer, g_object_unref)

GDataGoaAuthorizer *gdata_goa_authorizer_new (GoaObject *goa_object) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
GoaObject *gdata_goa_authorizer_get_goa_object (GDataGoaAuthorizer *self) G_GNUC_PURE;

G_END_DECLS

#endif /* GDATA_GOA_AUTHORIZER_H */
