/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2010 <philip@tecnocode.co.uk>
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

#ifndef GDATA_APP_CATEGORIES_H
#define GDATA_APP_CATEGORIES_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

#define GDATA_TYPE_APP_CATEGORIES		(gdata_app_categories_get_type ())
#define GDATA_APP_CATEGORIES(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_APP_CATEGORIES, GDataAPPCategories))
#define GDATA_APP_CATEGORIES_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_APP_CATEGORIES, GDataAPPCategoriesClass))
#define GDATA_IS_APP_CATEGORIES(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_APP_CATEGORIES))
#define GDATA_IS_APP_CATEGORIES_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_APP_CATEGORIES))
#define GDATA_APP_CATEGORIES_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_APP_CATEGORIES, GDataAPPCategoriesClass))

typedef struct _GDataAPPCategoriesPrivate	GDataAPPCategoriesPrivate;

/**
 * GDataAPPCategories:
 *
 * All the fields in the #GDataAPPCategories structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	/*< private >*/
	GDataParsable parent;
	GDataAPPCategoriesPrivate *priv;
} GDataAPPCategories;

/**
 * GDataAPPCategoriesClass:
 *
 * All the fields in the #GDataAPPCategoriesClass structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	/*< private >*/
	GDataParsableClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataAPPCategoriesClass;

GType gdata_app_categories_get_type (void) G_GNUC_CONST;
G_DEFINE_AUTOPTR_CLEANUP_FUNC (GDataAPPCategories, g_object_unref)

GList *gdata_app_categories_get_categories (GDataAPPCategories *self) G_GNUC_PURE;
gboolean gdata_app_categories_is_fixed (GDataAPPCategories *self) G_GNUC_PURE;

G_END_DECLS

#endif /* !GDATA_APP_CATEGORIES_H */
