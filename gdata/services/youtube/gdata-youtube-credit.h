/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2009–2010, 2015 <philip@tecnocode.co.uk>
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

#ifndef GDATA_YOUTUBE_CREDIT_H
#define GDATA_YOUTUBE_CREDIT_H

#include <glib.h>
#include <glib-object.h>

#include <gdata/gdata-parsable.h>
#include <gdata/media/gdata-media-credit.h>

G_BEGIN_DECLS

#ifndef LIBGDATA_DISABLE_DEPRECATED

/**
 * GDATA_YOUTUBE_CREDIT_ENTITY_PARTNER:
 *
 * The credited entity is a YouTube partner.
 *
 * Since: 0.7.0
 * Deprecated: 0.17.0: This is no longer supported by Google. There is no
 *   replacement.
 */
#define GDATA_YOUTUBE_CREDIT_ENTITY_PARTNER "partner"

#define GDATA_TYPE_YOUTUBE_CREDIT		(gdata_youtube_credit_get_type ())
#define GDATA_YOUTUBE_CREDIT(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), GDATA_TYPE_YOUTUBE_CREDIT, GDataYouTubeCredit))
#define GDATA_YOUTUBE_CREDIT_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GDATA_TYPE_YOUTUBE_CREDIT, GDataYouTubeCreditClass))
#define GDATA_IS_YOUTUBE_CREDIT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GDATA_TYPE_YOUTUBE_CREDIT))
#define GDATA_IS_YOUTUBE_CREDIT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GDATA_TYPE_YOUTUBE_CREDIT))
#define GDATA_YOUTUBE_CREDIT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GDATA_TYPE_YOUTUBE_CREDIT, GDataYouTubeCreditClass))

typedef struct _GDataYouTubeCreditPrivate	GDataYouTubeCreditPrivate G_GNUC_DEPRECATED;

/**
 * GDataYouTubeCredit:
 *
 * All the fields in the #GDataYouTubeCredit structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 * Deprecated: 0.17.0: This is no longer supported by Google. There is no
 *   replacement.
 */
typedef struct {
	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	GDataMediaCredit parent;
	GDataYouTubeCreditPrivate *priv;
	G_GNUC_END_IGNORE_DEPRECATIONS
} GDataYouTubeCredit;

/**
 * GDataYouTubeCreditClass:
 *
 * All the fields in the #GDataYouTubeCreditClass structure are private and should never be accessed directly.
 *
 * Since: 0.4.0
 * Deprecated: 0.17.0: This is no longer supported by Google. There is no
 *   replacement.
 */
typedef struct {
	/*< private >*/
	GDataMediaCreditClass parent;

	/*< private >*/
	/* Padding for future expansion */
	void (*_g_reserved0) (void);
	void (*_g_reserved1) (void);
} GDataYouTubeCreditClass;

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
GType gdata_youtube_credit_get_type (void) G_GNUC_CONST G_GNUC_DEPRECATED;
const gchar *gdata_youtube_credit_get_entity_type (GDataYouTubeCredit *self) G_GNUC_PURE G_GNUC_DEPRECATED;
G_GNUC_END_IGNORE_DEPRECATIONS

#endif /* !LIBGDATA_DISABLE_DEPRECATED */

G_END_DECLS

#endif /* !GDATA_YOUTUBE_CREDIT_H */
