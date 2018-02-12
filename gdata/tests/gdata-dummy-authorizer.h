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

#ifndef GDATA_DUMMY_AUTHORIZER_H
#define GDATA_DUMMY_AUTHORIZER_H

#include <glib.h>
#include <glib-object.h>

#include "gdata-authorizer.h"

G_BEGIN_DECLS

#define GDATA_TYPE_DUMMY_AUTHORIZER		(gdata_dummy_authorizer_get_type ())
#define GDATA_DUMMY_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DUMMY_AUTHORIZER, GDataDummyAuthorizer))
#define GDATA_DUMMY_AUTHORIZER_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DUMMY_AUTHORIZER, GDataDummyAuthorizerClass))
#define GDATA_IS_DUMMY_AUTHORIZER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DUMMY_AUTHORIZER))
#define GDATA_IS_DUMMY_AUTHORIZER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DUMMY_AUTHORIZER))
#define GDATA_DUMMY_AUTHORIZER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DUMMY_AUTHORIZER, GDataDummyAuthorizerClass))

typedef struct _GDataDummyAuthorizerPrivate	GDataDummyAuthorizerPrivate;

/**
 * GDataDummyAuthorizer:
 *
 * All the fields in the #GDataDummyAuthorizer structure are private and should never be accessed directly.
 *
 * Since: 0.16.0
 */
typedef struct {
	/*< private >*/
	GObject parent;
	GDataDummyAuthorizerPrivate *priv;
} GDataDummyAuthorizer;

/**
 * GDataDummyAuthorizerClass:
 *
 * All the fields in the #GDataDummyAuthorizerClass structure are private and should never be accessed directly.
 *
 * Since: 0.16.0
 */
typedef struct {
	/*< private >*/
	GObjectClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataDummyAuthorizerClass;

GType gdata_dummy_authorizer_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataDummyAuthorizer, g_object_unref)

GDataDummyAuthorizer *gdata_dummy_authorizer_new (GType service_type) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;
GDataDummyAuthorizer *gdata_dummy_authorizer_new_for_authorization_domains (GList *authorization_domains) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

G_END_DECLS

#endif /* !GDATA_DUMMY_AUTHORIZER_H */
