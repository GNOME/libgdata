/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009 <philip@tecnocode.co.uk>
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

#ifndef GDATA_GD_NAME_H
#define GDATA_GD_NAME_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

#define GDATA_TYPE_GD_NAME		(gdata_gd_name_get_type ())
#define GDATA_GD_NAME(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_GD_NAME, GDataGDName))
#define GDATA_GD_NAME_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_GD_NAME, GDataGDNameClass))
#define GDATA_IS_GD_NAME(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_GD_NAME))
#define GDATA_IS_GD_NAME_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_GD_NAME))
#define GDATA_GD_NAME_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_GD_NAME, GDataGDNameClass))

typedef struct _GDataGDNamePrivate	GDataGDNamePrivate;

/**
 * GDataGDName:
 *
 * All the fields in the #GDataGDName structure are private and should never be accessed directly.
 *
 * Since: 0.5.0
 */
typedef struct {
	GDataParsable parent;
	GDataGDNamePrivate *priv;
} GDataGDName;

/**
 * GDataGDNameClass:
 *
 * All the fields in the #GDataGDNameClass structure are private and should never be accessed directly.
 *
 * Since: 0.5.0
 */
typedef struct {
	/*< private >*/
	GDataParsableClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataGDNameClass;

GType gdata_gd_name_get_type (void) G_GNUC_CONST;

GDataGDName *gdata_gd_name_new (const gchar *given_name, const gchar *family_name) G_GNUC_WARN_UNUSED_RESULT G_GNUC_MALLOC;

const gchar *gdata_gd_name_get_given_name (GDataGDName *self) G_GNUC_PURE;
void gdata_gd_name_set_given_name (GDataGDName *self, const gchar *given_name);

const gchar *gdata_gd_name_get_additional_name (GDataGDName *self) G_GNUC_PURE;
void gdata_gd_name_set_additional_name (GDataGDName *self, const gchar *additional_name);

const gchar *gdata_gd_name_get_family_name (GDataGDName *self) G_GNUC_PURE;
void gdata_gd_name_set_family_name (GDataGDName *self, const gchar *family_name);

const gchar *gdata_gd_name_get_prefix (GDataGDName *self) G_GNUC_PURE;
void gdata_gd_name_set_prefix (GDataGDName *self, const gchar *prefix);

const gchar *gdata_gd_name_get_suffix (GDataGDName *self) G_GNUC_PURE;
void gdata_gd_name_set_suffix (GDataGDName *self, const gchar *suffix);

const gchar *gdata_gd_name_get_full_name (GDataGDName *self) G_GNUC_PURE;
void gdata_gd_name_set_full_name (GDataGDName *self, const gchar *full_name);

G_END_DECLS

#endif /* !GDATA_GD_NAME_H */
