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

#ifndef GDATA_YOUTUBE_CATEGORY_H
#define GDATA_YOUTUBE_CATEGORY_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>
#include <gdata/atom/gdata-category.h>

G_BEGIN_DECLS

#define GDATA_TYPE_YOUTUBE_CATEGORY		(gdata_youtube_category_get_type ())
#define GDATA_YOUTUBE_CATEGORY(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_YOUTUBE_CATEGORY, GDataYouTubeCategory))
#define GDATA_YOUTUBE_CATEGORY_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_YOUTUBE_CATEGORY, GDataYouTubeCategoryClass))
#define GDATA_IS_YOUTUBE_CATEGORY(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_YOUTUBE_CATEGORY))
#define GDATA_IS_YOUTUBE_CATEGORY_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_YOUTUBE_CATEGORY))
#define GDATA_YOUTUBE_CATEGORY_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_YOUTUBE_CATEGORY, GDataYouTubeCategoryClass))

typedef struct _GDataYouTubeCategoryPrivate	GDataYouTubeCategoryPrivate;

/**
 * GDataYouTubeCategory:
 *
 * All the fields in the #GDataYouTubeCategory structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	GDataCategory parent;
	GDataYouTubeCategoryPrivate *priv;
} GDataYouTubeCategory;

/**
 * GDataYouTubeCategoryClass:
 *
 * All the fields in the #GDataYouTubeCategoryClass structure are private and should never be accessed directly.
 *
 * Since: 0.7.0
 */
typedef struct {
	/*< private >*/
	GDataCategoryClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataYouTubeCategoryClass;

GType gdata_youtube_category_get_type (void) G_GNUC_CONST;

gboolean gdata_youtube_category_is_assignable (GDataYouTubeCategory *self) G_GNUC_PURE;
gboolean gdata_youtube_category_is_browsable (GDataYouTubeCategory *self, const gchar *region) G_GNUC_PURE;
gboolean gdata_youtube_category_is_deprecated (GDataYouTubeCategory *self) G_GNUC_PURE;

G_END_DECLS

#endif /* !GDATA_YOUTUBE_CATEGORY_H */
