/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Mayank Sharma <mayank8019@gmail.com>
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

#ifndef GDATA_PROPERTY_H
#define GDATA_PROPERTY_H

#include <glib.h>
#include <glib-object.h>

#include "gdata-parsable.h"
#include "gdata-private.h"

G_BEGIN_DECLS

#define GDATA_TYPE_PROPERTY		(gdata_property_get_type ())
#define GDATA_PROPERTY(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_PROPERTY, GDataProperty))
#define GDATA_PROPERTY_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_PROPERTY, GDataPropertyClass))
#define GDATA_IS_PROPERTY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_PROPERTY))
#define GDATA_IS_PROPERTY_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_PROPERTY))
#define GDATA_PROPERTY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_PROPERTY, GDataPropertyClass))

#define GDATA_PROPERTY_VISIBILITY_PUBLIC "PUBLIC"
#define GDATA_PROPERTY_VISIBILITY_PRIVATE "PRIVATE"

typedef struct _GDataPropertyPrivate	GDataPropertyPrivate;

typedef struct {
	GDataParsable parent;
	GDataPropertyPrivate *priv;
} GDataProperty;

typedef struct {
	/*< private >*/
	GDataParsableClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataPropertyClass;

GType gdata_property_get_type (void) G_GNUC_CONST;

GDataProperty *gdata_property_new (const gchar *key) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_property_get_key (GDataProperty *self) G_GNUC_PURE;
void _gdata_property_set_key (GDataProperty *self, const gchar *key);

const gchar *gdata_property_get_etag (GDataProperty *self) G_GNUC_PURE;
void gdata_property_set_etag (GDataProperty *self, const gchar *etag);

const gchar *gdata_property_get_value (GDataProperty *self) G_GNUC_PURE;
void gdata_property_set_value (GDataProperty *self, const gchar *value);

gboolean gdata_property_get_is_publicly_visible (GDataProperty *self) G_GNUC_PURE;
void gdata_property_set_is_publicly_visible (GDataProperty *self, gboolean is_publicly_visible);

G_END_DECLS

#endif /* !GDATA_PROPERTY_H */
