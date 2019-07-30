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

/**
 * SECTION:gdata-picasaweb-comment
 * @short_description: GData PicasaWeb comment object
 * @stability: Stable
 * @include: gdata/services/picasaweb/gdata-picasaweb-comment.h
 *
 * #GDataPicasaWebComment is a subclass of #GDataComment to represent a comment on a #GDataPicasaWebFile. It is returned by the #GDataCommentable
 * interface implementation on #GDataPicasaWebFile.
 *
 * It's possible to query for, add and delete #GDataPicasaWebComments from #GDataPicasaWebFiles.
 *
 * Since: 0.10.0
 */

#include <config.h>
#include <glib.h>

#include "gdata-picasaweb-comment.h"

static void gdata_picasaweb_comment_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_picasaweb_comment_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);

enum {
	PROP_ETAG = 1,
};

G_DEFINE_TYPE (GDataPicasaWebComment, gdata_picasaweb_comment, GDATA_TYPE_COMMENT)

static void
gdata_picasaweb_comment_class_init (GDataPicasaWebCommentClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	gobject_class->get_property = gdata_picasaweb_comment_get_property;
	gobject_class->set_property = gdata_picasaweb_comment_set_property;

	entry_class->kind_term = "http://schemas.google.com/photos/2007#comment";

	/* Override the ETag property since ETags don't seem to be supported for PicasaWeb comments. */
	g_object_class_override_property (gobject_class, PROP_ETAG, "etag");
}

static void
gdata_picasaweb_comment_init (GDataPicasaWebComment *self)
{
	/* Nothing to see here. */
}

static void
gdata_picasaweb_comment_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_ETAG:
			/* Never return an ETag */
			g_value_set_string (value, NULL);
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_picasaweb_comment_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
		case PROP_ETAG:
			/* Never set an ETag (note that this doesn't stop it being set in GDataEntry due to XML parsing) */
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/**
 * gdata_picasaweb_comment_new:
 * @id: the comment's ID, or %NULL
 *
 * Creates a new #GDataPicasaWebComment with the given ID and default properties.
 *
 * Return value: a new #GDataPicasaWebComment; unref with g_object_unref()
 *
 * Since: 0.10.0
 */
GDataPicasaWebComment *
gdata_picasaweb_comment_new (const gchar *id)
{
	return GDATA_PICASAWEB_COMMENT (g_object_new (GDATA_TYPE_PICASAWEB_COMMENT,
	                                              "id", id,
	                                              NULL));
}
