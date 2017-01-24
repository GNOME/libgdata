/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Richard Schwarting 2009 <aquarichy@gmail.com>
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

#ifndef GDATA_EXIF_TAGS_H
#define GDATA_EXIF_TAGS_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>

G_BEGIN_DECLS

#define GDATA_TYPE_EXIF_TAGS		(gdata_exif_tags_get_type ())
#define GDATA_EXIF_TAGS(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_EXIF_TAGS, GDataExifTags))
#define GDATA_EXIF_TAGS_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_EXIF_TAGS, GDataExifTagsClass))
#define GDATA_IS_EXIF_TAGS(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_EXIF_TAGS))
#define GDATA_IS_EXIF_TAGS_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_EXIF_TAGS))
#define GDATA_EXIF_TAGS_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_EXIF_TAGS, GDataExifTagsClass))

typedef struct _GDataExifTagsPrivate	GDataExifTagsPrivate;

/**
 * GDataExifTags:
 *
 * All the fields in the #GDataExifTags structure are private and should never be accessed directly.
 *
 * Since: 0.5.0
 */
typedef struct {
	GDataParsable parent;
	GDataExifTagsPrivate *priv;
} GDataExifTags;

/**
 * GDataExifTagsClass:
 *
 * All the fields in the #GDataExifTagsClass structure are private and should never be accessed directly.
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
} GDataExifTagsClass;

GType gdata_exif_tags_get_type (void) G_GNUC_CONST;

gdouble gdata_exif_tags_get_distance (GDataExifTags *self) G_GNUC_PURE;
gdouble gdata_exif_tags_get_exposure (GDataExifTags *self) G_GNUC_PURE;
gboolean gdata_exif_tags_get_flash (GDataExifTags *self) G_GNUC_PURE;
gdouble gdata_exif_tags_get_focal_length (GDataExifTags *self) G_GNUC_PURE;
gdouble gdata_exif_tags_get_fstop (GDataExifTags *self) G_GNUC_PURE;
const gchar *gdata_exif_tags_get_image_unique_id (GDataExifTags *self) G_GNUC_PURE;
gint gdata_exif_tags_get_iso (GDataExifTags *self) G_GNUC_PURE;
const gchar *gdata_exif_tags_get_make (GDataExifTags *self) G_GNUC_PURE;
const gchar *gdata_exif_tags_get_model (GDataExifTags *self) G_GNUC_PURE;
gint64 gdata_exif_tags_get_time (GDataExifTags *self);

G_END_DECLS

#endif /* !GDATA_EXIF_TAGS_H */
