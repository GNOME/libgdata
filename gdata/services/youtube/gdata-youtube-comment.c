/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * GData Client
 * Copyright (C) Philip Withnall 2011, 2015 <philip@tecnocode.co.uk>
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
 * @stability: Stable
 * @include: gdata/services/youtube/gdata-youtube-comment.h
 *
 * #GDataYouTubeComment is a subclass of #GDataComment to represent a comment on a #GDataYouTubeVideo. It is returned by the #GDataCommentable
 * interface implementation on #GDataYouTubeVideo.
 *
 * It's possible to query for and add #GDataYouTubeComments, but it is not possible to delete #GDataYouTubeComments from any video
 * using the GData API.
 *
 * Comments on YouTube videos can be arranged in a hierarchy by their #GDataYouTubeComment:parent-comment-uris. If a
 * #GDataYouTubeComment<!-- -->'s parent comment URI is non-%NULL, it should match the %GDATA_LINK_SELF #GDataLink of another #GDataYouTubeComment on
 * the same video (as retrieved using gdata_entry_look_up_link() on the comments). Comments with #GDataYouTubeComment:parent-comment-uri set to %NULL
 * are top-level comments.
 *
 * Since: 0.10.0
 */

#include <config.h>
#include <glib.h>

#include "gdata-parser.h"
#include "gdata-private.h"
#include "gdata-youtube-comment.h"

#define GDATA_LINK_PARENT_COMMENT_URI "http://gdata.youtube.com/schemas/2007#in-reply-to"

static void gdata_youtube_comment_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec);
static void gdata_youtube_comment_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec);
static void gdata_youtube_comment_finalize (GObject *object);
static gboolean parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error);
static void get_json (GDataParsable *parsable, JsonBuilder *builder);
static const gchar *get_content_type (void);

struct _GDataYouTubeCommentPrivate {
	gchar *channel_id;  /* owned */
	gchar *video_id;  /* owned */
	gboolean can_reply;
};

enum {
	PROP_PARENT_COMMENT_URI = 1,
};

G_DEFINE_TYPE_WITH_PRIVATE (GDataYouTubeComment, gdata_youtube_comment, GDATA_TYPE_COMMENT)

static void
gdata_youtube_comment_class_init (GDataYouTubeCommentClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
	GDataParsableClass *parsable_class = GDATA_PARSABLE_CLASS (klass);
	GDataEntryClass *entry_class = GDATA_ENTRY_CLASS (klass);

	gobject_class->get_property = gdata_youtube_comment_get_property;
	gobject_class->set_property = gdata_youtube_comment_set_property;
	gobject_class->finalize = gdata_youtube_comment_finalize;

	parsable_class->parse_json = parse_json;
	parsable_class->get_json = get_json;
	parsable_class->get_content_type = get_content_type;

	entry_class->kind_term = "youtube#commentThread";

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
	self->priv = gdata_youtube_comment_get_instance_private (self);
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

static void
gdata_youtube_comment_finalize (GObject *object)
{
	GDataYouTubeCommentPrivate *priv = GDATA_YOUTUBE_COMMENT (object)->priv;

	g_free (priv->channel_id);
	g_free (priv->video_id);

	/* Chain up to the parent class */
	G_OBJECT_CLASS (gdata_youtube_comment_parent_class)->finalize (object);
}

/* Reference: https://developers.google.com/youtube/v3/docs/comments#resource */
static gboolean
parse_comment (GDataParsable *parsable, JsonReader *reader, GError **error)
{
	GDataYouTubeComment *self = GDATA_YOUTUBE_COMMENT (parsable);
	const gchar *id, *etag, *parent_id, *author_name, *author_uri;
	const gchar *published_at, *updated_at;
	gint64 published, updated;

	/* Check this is an object. */
	if (!json_reader_is_object (reader)) {
		return gdata_parser_error_required_json_content_missing (reader, error);
	}

	/* id */
	json_reader_read_member (reader, "id");
	id = json_reader_get_string_value (reader);
	json_reader_end_member (reader);

	/* Empty ID? */
	if (id == NULL || *id == '\0') {
		return gdata_parser_error_required_json_content_missing (reader, error);
	}

	_gdata_entry_set_id (GDATA_ENTRY (parsable), id);

	/* etag */
	json_reader_read_member (reader, "etag");
	etag = json_reader_get_string_value (reader);
	json_reader_end_member (reader);

	/* Empty ETag? */
	if (etag != NULL && *id == '\0') {
		return gdata_parser_error_required_json_content_missing (reader, error);
	}

	_gdata_entry_set_etag (GDATA_ENTRY (parsable), etag);

	/* snippet */
	json_reader_read_member (reader, "snippet");

	if (!json_reader_is_object (reader)) {
		json_reader_end_member (reader);
		return gdata_parser_error_required_json_content_missing (reader, error);
	}

	json_reader_read_member (reader, "textDisplay");
	gdata_entry_set_content (GDATA_ENTRY (self),
	                         json_reader_get_string_value (reader));
	json_reader_end_member (reader);

	json_reader_read_member (reader, "parentId");
	parent_id = json_reader_get_string_value (reader);
	json_reader_end_member (reader);

	if (parent_id != NULL) {
		gchar *uri = NULL;

		uri = _gdata_service_build_uri ("https://www.googleapis.com"
		                                "/youtube/v3/comments"
		                                "?part=snippet"
		                                "&id=%s", parent_id);
		gdata_youtube_comment_set_parent_comment_uri (self, uri);
		g_free (uri);
	}

	json_reader_read_member (reader, "authorDisplayName");
	author_name = json_reader_get_string_value (reader);
	json_reader_end_member (reader);

	json_reader_read_member (reader, "authorChannelUrl");
	author_uri = json_reader_get_string_value (reader);
	json_reader_end_member (reader);

	if (author_name != NULL && *author_name != '\0') {
		GDataAuthor *author = NULL;

		author = gdata_author_new (author_name, author_uri, NULL);
		gdata_entry_add_author (GDATA_ENTRY (self), author);
	}

	json_reader_read_member (reader, "publishedAt");
	published_at = json_reader_get_string_value (reader);
	json_reader_end_member (reader);

	if (published_at != NULL &&
	    gdata_parser_int64_from_iso8601 (published_at, &published)) {
		_gdata_entry_set_published (GDATA_ENTRY (self), published);
	} else if (published_at != NULL) {
		/* Error */
		gdata_parser_error_not_iso8601_format_json (reader,
		                                            published_at,
		                                            error);
		json_reader_end_member (reader);
		return FALSE;
	}

	json_reader_read_member (reader, "updatedAt");
	updated_at = json_reader_get_string_value (reader);
	json_reader_end_member (reader);

	if (updated_at != NULL &&
	    gdata_parser_int64_from_iso8601 (updated_at, &updated)) {
		_gdata_entry_set_updated (GDATA_ENTRY (self), updated);
	} else if (updated_at != NULL) {
		/* Error */
		gdata_parser_error_not_iso8601_format_json (reader,
		                                            updated_at,
		                                            error);
		json_reader_end_member (reader);
		return FALSE;
	}

	/* FIXME: Implement:
	 *  - channelId
	 *  - videoId
	 *  - textOriginal
	 *  - canRate
	 *  - viewerRating
	 *  - likeCount
	 *  - moderationStatus
	 *  - authorProfileImageUrl
	 *  - authorChannelId
	 *  - authorGoogleplusProfileUrl
	 */

	json_reader_end_member (reader);

	return TRUE;
}

/* Reference: https://developers.google.com/youtube/v3/docs/commentThreads#resource */
static gboolean
parse_json (GDataParsable *parsable, JsonReader *reader, gpointer user_data, GError **error)
{
	gboolean success;
	GDataYouTubeComment *self = GDATA_YOUTUBE_COMMENT (parsable);
	GDataYouTubeCommentPrivate *priv = self->priv;

	if (g_strcmp0 (json_reader_get_member_name (reader), "snippet") == 0) {
		guint i;

		/* Check this is an object. */
		if (!json_reader_is_object (reader)) {
			return gdata_parser_error_required_json_content_missing (reader, error);
		}

		for (i = 0; i < (guint) json_reader_count_members (reader); i++) {
			json_reader_read_element (reader, i);

			if (gdata_parser_string_from_json_member (reader, "channelId", P_DEFAULT, &priv->channel_id, &success, error) ||
			    gdata_parser_string_from_json_member (reader, "videoId", P_DEFAULT, &priv->video_id, &success, error) ||
			    gdata_parser_boolean_from_json_member (reader, "canReply", P_DEFAULT, &priv->can_reply, &success, error)) {
				/* Fall through. */
			} else if (g_strcmp0 (json_reader_get_member_name (reader), "topLevelComment") == 0) {
				success = parse_comment (parsable, reader, error);
			}

			json_reader_end_element (reader);

			if (!success) {
				return FALSE;
			}
		}

		return TRUE;
	} else {
		return GDATA_PARSABLE_CLASS (gdata_youtube_comment_parent_class)->parse_json (parsable, reader, user_data, error);
	}

	return TRUE;
}

/* Reference: https://developers.google.com/youtube/v3/docs/comments#resource */
static void
get_comment (GDataParsable *parsable, JsonBuilder *builder)
{
	GDataYouTubeComment *self = GDATA_YOUTUBE_COMMENT (parsable);
	GDataEntry *entry = GDATA_ENTRY (parsable);
	GDataYouTubeCommentPrivate *priv = GDATA_YOUTUBE_COMMENT (parsable)->priv;

	json_builder_set_member_name (builder, "kind");
	json_builder_add_string_value (builder, "youtube#comment");

	if (gdata_entry_get_etag (entry) != NULL) {
		json_builder_set_member_name (builder, "etag");
		json_builder_add_string_value (builder,
		                               gdata_entry_get_etag (entry));
	}

	if (gdata_entry_get_id (entry) != NULL) {
		json_builder_set_member_name (builder, "id");
		json_builder_add_string_value (builder,
		                               gdata_entry_get_id (entry));
	}

	json_builder_set_member_name (builder, "snippet");
	json_builder_begin_object (builder);

	if (priv->channel_id != NULL) {
		json_builder_set_member_name (builder, "channelId");
		json_builder_add_string_value (builder, priv->channel_id);
	}

	if (priv->video_id != NULL) {
		json_builder_set_member_name (builder, "videoId");
		json_builder_add_string_value (builder, priv->video_id);
	}

	/* Note we build textOriginal and parse textDisplay. */
	if (gdata_entry_get_content (entry) != NULL) {
		json_builder_set_member_name (builder, "textOriginal");
		json_builder_add_string_value (builder,
		                               gdata_entry_get_content (entry));
	}

	if (gdata_youtube_comment_get_parent_comment_uri (self) != NULL) {
		json_builder_set_member_name (builder, "parentId");
		json_builder_add_string_value (builder,
		                               gdata_youtube_comment_get_parent_comment_uri (self));
	}

	json_builder_end_object (builder);
}

/* Reference: https://developers.google.com/youtube/v3/docs/commentThreads#resource
 *
 * Sort of. If creating a new top-level comment, we need to create a
 * commentThread; otherwise we need to create a comment. */
static void
get_json (GDataParsable *parsable, JsonBuilder *builder)
{
	GDataEntry *entry = GDATA_ENTRY (parsable);
	GDataYouTubeCommentPrivate *priv = GDATA_YOUTUBE_COMMENT (parsable)->priv;

	/* Don’t chain up because it’s mostly irrelevant. */
	json_builder_set_member_name (builder, "kind");
	json_builder_add_string_value (builder, "youtube#commentThread");

	if (gdata_entry_get_etag (entry) != NULL) {
		json_builder_set_member_name (builder, "etag");
		json_builder_add_string_value (builder,
		                               gdata_entry_get_etag (entry));
	}

	if (gdata_entry_get_id (entry) != NULL) {
		json_builder_set_member_name (builder, "id");
		json_builder_add_string_value (builder,
		                               gdata_entry_get_id (entry));
	}

	/* snippet object. */
	json_builder_set_member_name (builder, "snippet");
	json_builder_begin_object (builder);

	if (priv->channel_id != NULL) {
		json_builder_set_member_name (builder, "channelId");
		json_builder_add_string_value (builder, priv->channel_id);
	}

	if (priv->video_id != NULL) {
		json_builder_set_member_name (builder, "videoId");
		json_builder_add_string_value (builder, priv->video_id);
	}

	json_builder_set_member_name (builder, "topLevelComment");
	json_builder_begin_object (builder);
	get_comment (parsable, builder);
	json_builder_end_object (builder);

	json_builder_end_object (builder);
}

static const gchar *
get_content_type (void)
{
	return "application/json";
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

G_GNUC_INTERNAL void
_gdata_youtube_comment_set_video_id (GDataYouTubeComment *self,
                                     const gchar *video_id);
G_GNUC_INTERNAL void
_gdata_youtube_comment_set_channel_id (GDataYouTubeComment *self,
                                       const gchar *channel_id);

/**
 * _gdata_youtube_comment_set_video_id:
 * @self: a #GDataYouTubeComment
 * @video_id: (nullable): the comment’s video ID, or %NULL
 *
 * Set the ID of the video the comment is attached to. This may be %NULL if the
 * comment has not yet been inserted, or if it is just attached to a channel
 * rather than a video.
 *
 * Since: 0.17.2
 */
void
_gdata_youtube_comment_set_video_id (GDataYouTubeComment *self,
                                     const gchar *video_id)
{
	GDataYouTubeCommentPrivate *priv;

	g_return_if_fail (GDATA_IS_YOUTUBE_COMMENT (self));
	g_return_if_fail (video_id == NULL || *video_id != '\0');

	priv = self->priv;

	g_free (priv->video_id);
	priv->video_id = g_strdup (video_id);
}

/**
 * _gdata_youtube_comment_set_channel_id:
 * @self: a #GDataYouTubeComment
 * @channel_id: (nullable): the comment’s channel ID, or %NULL
 *
 * Set the ID of the channel the comment is attached to. This may be %NULL if
 * the comment has not yet been inserted, but must be set otherwise.
 *
 * Since: 0.17.2
 */
void
_gdata_youtube_comment_set_channel_id (GDataYouTubeComment *self,
                                       const gchar *channel_id)
{
	GDataYouTubeCommentPrivate *priv;

	g_return_if_fail (GDATA_IS_YOUTUBE_COMMENT (self));
	g_return_if_fail (channel_id == NULL || *channel_id != '\0');

	priv = self->priv;

	g_free (priv->channel_id);
	priv->channel_id = g_strdup (channel_id);
}
