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
 * SECTION:gdata-youtube-comment
 * @short_description: GData YouTube comment object
 * @stability: Unstable
 * @include: gdata/services/youtube/gdata-youtube-comment.h
 *
 * #GDataYouTubeComment is a subclass of #GDataComment to represent a comment on a #GDataYouTubeVideo. It is returned by the #GDataCommentable
 * interface implementation on #GDataYouTubeVideo.
 *
 * It's possible to query for and add #GDataYouTubeComment<!-- -->s, but it is not possible to delete #GDataYouTubeComment<!-- -->s from any video
 * using the GData API.
 *
 * Comments on YouTube videos can be arranged in a hierarchy by their #GDataYouTubeComment:parent-comment-uri<!-- -->s. If a
 * #GDataYouTubeComment<!-- -->'s parent comment URI is non-%NULL, it should match the %GDATA_LINK_SELF #GDataLink of another #GDataYouTubeComment on
 * the same video (as retrieved using gdata_entry_look_up_link() on the comments). Comments with #GDataYouTubeComment:parent-comment-uri set to %NULL
 * are top-level comments.
 *
 * Since: 0.10.0
 */

#include <config.h>
#include <glib.h>

#include "gdata-youtube-comment.h"

#define GDATA_LINK_PARENT_COMMENT_URI "http://gdata.youtube.com/schemas/2007#in-reply-to"

static void gdata_youtube_comment_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void gdata_youtube_comment_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);

enum {
	PROP_PARENT_COMMENT_URI = 1,
};

G_DEFINE_TYPE (GDataYouTubeComment, gdata_youtube_comment, GDATA_TYPE_COMMENT)

static void
gdata_youtube_comment_class_init (GDataYouTubeCommentClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	gobject_class->get_property = gdata_youtube_comment_get_property;
	gobject_class->set_property = gdata_youtube_comment_set_property;

	entry_class->kind_term = "http://gdata.youtube.com/schemas/2007#comment";

	/**
	 * GDataYouTubeComment:parent-comment-uri:
	 *
	 * The URI of the parent comment to this one, or %NULL if this comment is a top-level comment.
	 *
	 * See the documentation for #GDataYouTubeComment for an explanation of the semantics of parent comment URIs.
	 *
	 * Since: 0.10.0
	 */
	g_object_class_install_property (gobject_class, PROP_PARENT_COMMENT_URI,
	                                 g_param_spec_string ("parent-comment-uri",
	                                                      "Parent comment URI", "The URI of the parent comment to this one.",
	                                                      NULL,
	                                                      G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gdata_youtube_comment_init (GDataYouTubeComment *self)
{
	/* Nothing to see here. */
}

static void
gdata_youtube_comment_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
	GDataYouTubeComment *self = GDATA_YOUTUBE_COMMENT (object);

	switch (property_id) {
		case PROP_PARENT_COMMENT_URI:
			g_value_set_string (value, gdata_youtube_comment_get_parent_comment_uri (self));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
gdata_youtube_comment_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
	GDataYouTubeComment *self = GDATA_YOUTUBE_COMMENT (object);

	switch (property_id) {
		case PROP_PARENT_COMMENT_URI:
			gdata_youtube_comment_set_parent_comment_uri (self, g_value_get_string (value));
			break;
		default:
			/* We don't have any other property... */
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

/**
 * gdata_youtube_comment_new:
 * @id: the comment's ID, or %NULL
 *
 * Creates a new #GDataYouTubeComment with the given ID and default properties.
 *
 * Return value: a new #GDataYouTubeComment; unref with g_object_unref()
 *
 * Since: 0.10.0
 */
GDataYouTubeComment *
gdata_youtube_comment_new (const gchar *id)
{
	return GDATA_YOUTUBE_COMMENT (g_object_new (GDATA_TYPE_YOUTUBE_COMMENT,
	                                            "id", id,
	                                            NULL));
}

/**
 * gdata_youtube_comment_get_parent_comment_uri:
 * @self: a #GDataYouTubeComment
 *
 * Gets the #GDataYouTubeComment:parent-comment-uri property.
 *
 * Return value: the parent comment URI, or %NULL
 *
 * Since: 0.10.0
 */
const gchar *
gdata_youtube_comment_get_parent_comment_uri (GDataYouTubeComment *self)
{
	GDataLink *link_;

	g_return_val_if_fail (GDATA_IS_YOUTUBE_COMMENT (self), NULL);

	link_ = gdata_entry_look_up_link (GDATA_ENTRY (self), GDATA_LINK_PARENT_COMMENT_URI);
	if (link_ == NULL) {
		return NULL;
	}

	return gdata_link_get_uri (link_);
}

/**
 * gdata_youtube_comment_set_parent_comment_uri:
 * @self: a #GDataYouTubeComment
 * @parent_comment_uri: a new parent comment URI, or %NULL
 *
 * Sets the #GDataYouTubeComment:parent-comment-uri property to @parent_comment_uri.
 *
 * Set @parent_comment_uri to %NULL to unset the #GDataYouTubeComment:parent-comment-uri property in the comment (i.e. make the comment a top-level
 * comment).
 *
 * See the <ulink type="http" url="http://code.google.com/apis/youtube/2.0/developers_guide_protocol_comments.html#Retrieve_comments">online
 * documentation</ulink> for more information.
 *
 * Since: 0.10.0
 */
void
gdata_youtube_comment_set_parent_comment_uri (GDataYouTubeComment *self, const gchar *parent_comment_uri)
{
	GDataLink *link_;

	g_return_if_fail (GDATA_IS_YOUTUBE_COMMENT (self));
	g_return_if_fail (parent_comment_uri == NULL || *parent_comment_uri != '\0');

	link_ = gdata_entry_look_up_link (GDATA_ENTRY (self), GDATA_LINK_PARENT_COMMENT_URI);

	if ((link_ == NULL && parent_comment_uri == NULL) ||
	    (link_ != NULL && parent_comment_uri != NULL && g_strcmp0 (gdata_link_get_uri (link_), parent_comment_uri) == 0)) {
		/* Nothing to do. */
		return;
	} else if (link_ == NULL && parent_comment_uri != NULL) {
		/* Add the new link. */
		link_ = gdata_link_new (parent_comment_uri, GDATA_LINK_PARENT_COMMENT_URI);
		gdata_entry_add_link (GDATA_ENTRY (self), link_);
		g_object_unref (link_);
	} else if (link_ != NULL && parent_comment_uri == NULL) {
		/* Remove the old link. */
		gdata_entry_remove_link (GDATA_ENTRY (self), link_);
	} else if (link_ != NULL && parent_comment_uri != NULL) {
		/* Update the existing link. */
		gdata_link_set_uri (link_, parent_comment_uri);
	}

	g_object_notify (G_OBJECT (self), "parent-comment-uri");
}
