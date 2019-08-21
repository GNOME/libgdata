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

#ifndef GDATA_DOCUMENTS_PROPERTY_H
#define GDATA_DOCUMENTS_PROPERTY_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

/**
 * GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC:
 *
 * The #GDataDocumentsProperty having the visibility set to TRUE corresponds to having the visibility property
 * on a Drive Property Resource
 * set to "PUBLIC". This makes the Property Resource visible to other apps.
 *
 * Since: 0.17.11
 */
#define GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PUBLIC "PUBLIC"

/**
 * GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE:
 *
 * The #GDataDocumentsProperty having the visibility set to FALSE (default) corresponds to having the visibility property on a Drive Property Resource
 * set to "PRIVATE". This makes the Property Resource accessible only by the app that created it.
 *
 * Since: 0.17.11
 */
#define GDATA_DOCUMENTS_PROPERTY_VISIBILITY_PRIVATE "PRIVATE"

#define GDATA_TYPE_DOCUMENTS_PROPERTY		(gdata_documents_property_get_type ())
#define GDATA_DOCUMENTS_PROPERTY(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_DOCUMENTS_PROPERTY, GDataDocumentsProperty))
#define GDATA_DOCUMENTS_PROPERTY_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_DOCUMENTS_PROPERTY, GDataDocumentsPropertyClass))
#define GDATA_IS_DOCUMENTS_PROPERTY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_DOCUMENTS_PROPERTY))
#define GDATA_IS_DOCUMENTS_PROPERTY_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_DOCUMENTS_PROPERTY))
#define GDATA_DOCUMENTS_PROPERTY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_DOCUMENTS_PROPERTY, GDataDocumentsPropertyClass))

typedef struct _GDataDocumentsPropertyPrivate	GDataDocumentsPropertyPrivate;

/**
 * GDataDocumentsProperty:
 *
 * All the fields in the #GDataDocumentsProperty structure are private and should never be accessed directly.
 *
 * Since: 0.17.11
 */
typedef struct {
	GDataParsable parent;
	GDataDocumentsPropertyPrivate *priv;
} GDataDocumentsProperty;

/**
 * GDataDocumentsPropertyClass:
 *
 * All the fields in the #GDataDocumentsPropertyClass structure are private and should never be accessed directly.
 *
 * Since: 0.17.11
 */
typedef struct {
	/*< private >*/
	GDataParsableClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataDocumentsPropertyClass;

GType gdata_documents_property_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataDocumentsProperty, g_object_unref)

GDataDocumentsProperty *gdata_documents_property_new (const gchar *key) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_documents_property_get_key (GDataDocumentsProperty *self) G_GNUC_PURE;

const gchar *gdata_documents_property_get_etag (GDataDocumentsProperty *self) G_GNUC_PURE;

const gchar *gdata_documents_property_get_value (GDataDocumentsProperty *self) G_GNUC_PURE;
void gdata_documents_property_set_value (GDataDocumentsProperty *self, const gchar *value);

const gchar *gdata_documents_property_get_visibility (GDataDocumentsProperty *self) G_GNUC_PURE;
void gdata_documents_property_set_visibility (GDataDocumentsProperty *self, const gchar *visibility);

G_END_DECLS

#endif /* !GDATA_DOCUMENTS_PROPERTY_H */
